#include "redis_conn.hpp"

#include <bredis/Connection.hpp>
#include <bredis/Extract.hpp>
#include <bredis/MarkerHelpers.hpp>

#include "utils.hpp"

redis_conn::redis_conn(boost::asio::ip::tcp::socket &&socket)
    : socket_(std::move(socket))
{}

void redis_conn::save(entities const& ents)
{
    LOG("REDIS", "SAVING " << ents.size() << " ENTITIES");

    std::vector<std::shared_ptr<std::string>> args;
    args.reserve(ents.size() * 2);
    bredis::command_container_t cmds;
    cmds.reserve(ents.size());

    // Build all SET commands
    for (auto const& e : ents)
    {
        std::string key;
        if (e.second->type() == "player")
            key = "player";
        else
            key = "npe";
        key += ":" + std::to_string(e.first);

        std::string value = serialize(*e.second);

        std::string &key_ref = *args.emplace_back(std::make_shared<std::string>(std::move(key)));
        std::string &value_ref = *args.emplace_back(std::make_shared<std::string>(std::move(value)));

        cmds.push_back(bredis::single_command_t("SET", key_ref, value_ref));
    }

    // Send transaction
    std::vector<bredis::extracts::extraction_result_t> result_array = redis_send_transaction(cmds);

    // Check results of all SET commands
    for (bredis::extracts::extraction_result_t const& set_result : result_array)
    {
        if (!(boost::get<bredis::extracts::string_t>(set_result).str == "OK"))
            LOG("REDIS", "ERROR ON \"EXEC\" command, expected OK, got:" << boost::get<bredis::extracts::string_t>(set_result).str);
    }
}

entities redis_conn::load_all_npes()
{
    LOG("REDIS", "LOADING ALL NON PLAYABLE ENTITIES");

    // Get all npes' key
    socket_.write(bredis::single_command_t("KEYS", "npe:*"));

    bredis::positive_parse_result_t<it_t, policy_t> redis_result = socket_.read(read_buffer_);
    bredis::extracts::extraction_result_t extract = boost::apply_visitor(bredis::extractor<it_t>(), redis_result.result);
    read_buffer_.consume(redis_result.consumed);

    std::list<std::string> keys;

    bredis::extracts::array_holder_t &keys_results = boost::get<bredis::extracts::array_holder_t>(extract);
    for (auto const& keys_result : keys_results.elements)
        keys.emplace_back(std::move(boost::get<bredis::extracts::string_t>(keys_result).str));

    if (keys.empty())
    {
        LOG("REDIS", "NOTHING TO LOAD");
        return entities();
    }

    // Build all GET commands
    std::vector<std::shared_ptr<std::string>> args;
    args.reserve(keys.size());
    bredis::command_container_t cmds;
    cmds.reserve(keys.size());

    for (std::string const& key : keys)
    {
        std::string &key_ref = *args.emplace_back(std::make_shared<std::string>(key));
        cmds.push_back(bredis::single_command_t("GET", key_ref));
    }


    // Send transaction
    std::vector<bredis::extracts::extraction_result_t> result_array = redis_send_transaction(cmds);

    // get and deserialize received values, build an entity container
    entities ents;
    for (bredis::extracts::extraction_result_t const& get_result : result_array)
    {
        std::string const& get_result_str = boost::get<bredis::extracts::string_t>(get_result).str;
        entity ent = deserialize<entity>(get_result_str);
        auto ent_ptr = std::make_shared<entity>(std::move(ent));
        ents.add(ent_ptr);
    }
    return ents;
}

std::shared_ptr<entity> redis_conn::load_player(std::string const& name)
{
    LOG("REDIS", "LOADING PLAYER " << name);

    socket_.write(bredis::single_command_t("GET", "player:" + name));

    bredis::positive_parse_result_t<it_t, policy_t> redis_result = socket_.read(read_buffer_);

    bredis::extracts::extraction_result_t extract = boost::apply_visitor(bredis::extractor<it_t>(), redis_result.result);

    read_buffer_.consume(redis_result.consumed);

    std::string const& get_result_str = boost::get<bredis::extracts::string_t>(extract).str;

    entity ent = deserialize<decltype(ent)>(get_result_str);

    return std::make_shared<decltype(ent)>(std::move(ent));
}

void redis_conn::remove_all()
{
    LOG("REDIS", "FLUSHING DB");
    socket_.write(bredis::single_command_t("FLUSHDB"));
    auto redis_result = socket_.read(read_buffer_);
    if (!boost::apply_visitor(bredis::marker_helpers::equality<it_t>("OK"), redis_result.result))
        LOG("REDIS", "ERROR, REDIS RESPONSE=" << buffer_to_string(read_buffer_.data(), redis_result.consumed));
    read_buffer_.consume(redis_result.consumed);
}

std::vector<bredis::extracts::extraction_result_t> redis_conn::redis_send_transaction(bredis::command_container_t const& cmds)
{
    bredis::command_container_t transaction;
    transaction.reserve(cmds.size() + 2);
    transaction.push_back(bredis::single_command_t("MULTI"));
    transaction.insert(transaction.end(), cmds.cbegin(), cmds.cend());
    transaction.push_back(bredis::single_command_t("EXEC"));

    // Send transaction
    socket_.write(bredis::command_wrapper_t(transaction));

    // Read and check redis response
    bredis::positive_parse_result_t<it_t, policy_t> redis_result = socket_.read(read_buffer_);

    if (!boost::apply_visitor(bredis::marker_helpers::equality<it_t>("OK"), redis_result.result))
        LOG("REDIS", "ERROR ON \"MULTI\" command, expected OK, got:" << buffer_to_string(read_buffer_.data(), redis_result.consumed));
    read_buffer_.consume(redis_result.consumed);

    for (int i = 0; i < cmds.size(); ++i)
    {
        redis_result = socket_.read(read_buffer_);
        if (!boost::apply_visitor(bredis::marker_helpers::equality<it_t>("QUEUED"), redis_result.result))
            LOG("REDIS", "ERROR ON \"SET\" command, expected QUEUED, got:" << buffer_to_string(read_buffer_.data(), redis_result.consumed));
        read_buffer_.consume(redis_result.consumed);
    }

    redis_result = socket_.read(read_buffer_);
    bredis::extracts::extraction_result_t extract = boost::apply_visitor(bredis::extractor<it_t>(), redis_result.result);
    read_buffer_.consume(redis_result.consumed);
    return std::move(boost::get<bredis::extracts::array_holder_t>(extract).elements);
}
