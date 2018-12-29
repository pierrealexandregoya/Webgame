#include <regex>

#include <gtest/gtest.h>

#include <webgame/behavior.hpp>
#include <webgame/entities.hpp>
#include <webgame/npc.hpp>
#include <webgame/player.hpp>
#include <webgame/protocol.hpp>

TEST(json, state_player)
{
    std::shared_ptr<webgame::player> ent = std::make_shared<webgame::upview_player>();
    ent->set_pos({1.1, -2.2});
    ent->set_dir({-3.3, 4.4});
    ent->set_speed(0.5);
    ent->set_max_speed(2);

    std::string ent_json;
    ASSERT_NO_THROW(ent_json = json_state_player(ent));

    ASSERT_EQ(std::string::npos, ent_json.find('\r'));
    ASSERT_EQ(std::string::npos, ent_json.find('\n'));

    ASSERT_EQ('{', *ent_json.cbegin());
    ASSERT_EQ('}', *ent_json.crbegin());

    auto j = nlohmann::json::parse(ent_json);

    ASSERT_TRUE(j.is_object());

    ASSERT_TRUE(j.count("order"));
    ASSERT_TRUE(j["order"].is_string());
    ASSERT_EQ("state", j["order"].get<std::string>());

    ASSERT_TRUE(j.count("suborder"));
    ASSERT_TRUE(j["suborder"].is_string());
    ASSERT_EQ("player", j["suborder"].get<std::string>());

    ASSERT_TRUE(j.count("id"));
    ASSERT_TRUE(j["id"].is_number_unsigned());
    ASSERT_EQ(ent->id(), j["id"].get<unsigned int>());

    ASSERT_TRUE(j.count("type"));
    ASSERT_TRUE(j["type"].is_string());
    ASSERT_EQ("upview_player", j["type"].get<std::string>());

    ASSERT_TRUE(j.count("speed"));
    ASSERT_TRUE(j["speed"].is_number_float());
    ASSERT_EQ(0.5, j["speed"].get<double>());

    ASSERT_FALSE(j.count("max_speed"));

    ASSERT_TRUE(j.count("pos"));
    ASSERT_TRUE(j["pos"].is_object());
    ASSERT_TRUE(j["pos"].count("x"));
    ASSERT_TRUE(j["pos"]["x"].is_number_float());
    ASSERT_EQ(1.1, j["pos"]["x"].get<double>());
    ASSERT_TRUE(j["pos"].count("y"));
    ASSERT_TRUE(j["pos"]["y"].is_number_float());
    ASSERT_EQ(-2.2, j["pos"]["y"].get<double>());

    ASSERT_TRUE(j.count("dir"));
    ASSERT_TRUE(j["dir"].is_object());
    ASSERT_TRUE(j["dir"].count("x"));
    ASSERT_TRUE(j["dir"]["x"].is_number_float());
    ASSERT_EQ(-3.3, j["dir"]["x"].get<double>());
    ASSERT_TRUE(j["dir"].count("y"));
    ASSERT_TRUE(j["dir"]["y"].is_number_float());
    ASSERT_EQ(4.4, j["dir"]["y"].get<double>());

}

TEST(json, state_entities)
{
    webgame::entities ents;
    ents.add(std::make_shared<webgame::npc>("type1", webgame::vector({ 0, 1 }), webgame::vector({ 2, 3 }), 0, 0, webgame::npc::behaviors({
        { 0, std::make_shared<webgame::arealimit>(webgame::arealimit::square, 0.5f, webgame::vector({ -0.5f, -0.5f })) } ,
        { 10, std::make_shared<webgame::attack_on_sight>(0.7f) },
        { 20, std::make_shared<webgame::stop>() },
        })));
    ents.add(std::make_shared<webgame::upview_player>());

    std::string ents_json;
    ASSERT_NO_THROW(ents_json = json_state_entities(ents));

    ASSERT_EQ(std::string::npos, ents_json.find('\r'));
    ASSERT_EQ(std::string::npos, ents_json.find('\n'));

    ASSERT_EQ('{', *ents_json.cbegin());
    ASSERT_EQ('}', *ents_json.crbegin());

    auto j = nlohmann::json::parse(ents_json);

    ASSERT_TRUE(j.is_object());

    ASSERT_TRUE(j.count("order"));
    ASSERT_TRUE(j["order"].is_string());
    ASSERT_EQ("state", j["order"].get<std::string>());

    ASSERT_TRUE(j.count("suborder"));
    ASSERT_TRUE(j["suborder"].is_string());
    ASSERT_EQ("entities", j["suborder"].get<std::string>());

    ASSERT_TRUE(j.count("data"));
    ASSERT_TRUE(j["data"].is_array());
    ASSERT_EQ(2, j["data"].size());

    ASSERT_TRUE(j["data"][0].is_object());
    ASSERT_TRUE(j["data"][0].count("id"));
    ASSERT_TRUE(j["data"][0].count("type"));
    ASSERT_TRUE(j["data"][0].count("pos"));
    ASSERT_TRUE(j["data"][0]["pos"].count("x"));
    ASSERT_TRUE(j["data"][0]["pos"].count("y"));
    ASSERT_TRUE(j["data"][0].count("dir"));
    ASSERT_TRUE(j["data"][0]["dir"].count("x"));
    ASSERT_TRUE(j["data"][0]["dir"].count("y"));
    ASSERT_TRUE(j["data"][0].count("speed"));

    ASSERT_TRUE(j["data"][1].is_object());
    ASSERT_TRUE(j["data"][1].count("id"));
    ASSERT_TRUE(j["data"][1].count("type"));
    ASSERT_TRUE(j["data"][1].count("pos"));
    ASSERT_TRUE(j["data"][1]["pos"].count("x"));
    ASSERT_TRUE(j["data"][1]["pos"].count("y"));
    ASSERT_TRUE(j["data"][1].count("dir"));
    ASSERT_TRUE(j["data"][1]["dir"].count("x"));
    ASSERT_TRUE(j["data"][1]["dir"].count("y"));
    ASSERT_TRUE(j["data"][1].count("speed"));
}
