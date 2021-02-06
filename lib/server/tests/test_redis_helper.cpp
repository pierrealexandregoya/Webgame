#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <gtest/gtest.h>

#include <webgame/redis_helper.hpp>

namespace asio = boost::asio;

std::chrono::seconds const time_out(10);

TEST(redis_helper, all)
{
    // connect and prepare: select + flushdb
    asio::io_context ioc;
    asio::ip::tcp::resolver resolver(ioc);
    asio::ip::basic_resolver_results<asio::ip::tcp> resolve_results = resolver.resolve("localhost", "6379");
    asio::ip::tcp::socket socket(ioc);
    asio::connect(socket, resolve_results.cbegin(), resolve_results.cend());
    std::shared_ptr<webgame::redis_helper> rh_p = std::make_shared<webgame::redis_helper>(socket);
    webgame::redis_helper &rh = *rh_p;
    ASSERT_NO_THROW(rh.select(1));
    ASSERT_NO_THROW(rh.flushdb());
    // dbsize: check db is empty
    {
        size_t dbsize;
        ASSERT_NO_THROW(dbsize = rh.dbsize());
        ASSERT_EQ(0, dbsize);
    }
    // async_multi_set
    {
        bool called1 = false;
        ASSERT_NO_THROW(rh.async_multi_set({ {"a:1", "v1"}, {"a:2", "v2"} }, [&called1] {called1 = true; }));
        bool called2 = false;
        rh.async_multi_set({ { "b:1", "v3" },{ "b:2", "v4" } }, [&called2] {called2 = true; });
        size_t nb_op;
        ASSERT_NO_THROW(nb_op = ioc.run_for(time_out));
        // ASSERT_EQ(2 * 2, nb_op);
        ASSERT_TRUE(ioc.stopped());
        ASSERT_TRUE(called1);
        ASSERT_TRUE(called2);
        ioc.restart();
    }
    // dbsize: check db size is correct
    {
        size_t dbsize;
        ASSERT_NO_THROW(dbsize = rh.dbsize());
        ASSERT_EQ(4, dbsize);
    }
    // keys
    {
        std::vector<std::string> akeys;
        ASSERT_NO_THROW(akeys = rh.keys("a:*"));
        ASSERT_EQ(2, akeys.size());
        ASSERT_TRUE(std::find(akeys.cbegin(), akeys.cend(), "a:1") != akeys.cend());
        ASSERT_TRUE(std::find(akeys.cbegin(), akeys.cend(), "a:2") != akeys.cend());
        std::vector<std::string> bkeys;
        ASSERT_NO_THROW(bkeys = rh.keys("b:*"));
        ASSERT_EQ(2, bkeys.size());
        ASSERT_TRUE(std::find(bkeys.cbegin(), bkeys.cend(), "b:1") != bkeys.cend());
        ASSERT_TRUE(std::find(bkeys.cbegin(), bkeys.cend(), "b:2") != bkeys.cend());
        size_t nb_keys;
        ASSERT_NO_THROW(nb_keys = rh.keys("*").size());
        ASSERT_EQ(4, nb_keys);
    }
    // async_get
    {
        bool called1 = false; bool success1; std::string value1;
        rh.async_get("a:1", [&called1, &success1, &value1](bool success, std::string &&value) {called1 = true; success1 = success; value1 = std::move(value); });
        bool called2 = false; bool success2; std::string value2;
        rh.async_get("ne_key", [&called2, &success2, &value2](bool success, std::string &&value) {called2 = true; success2 = success; value2 = std::move(value); });
        bool called3 = false; bool success3; std::string value3;
        rh.async_get("b:2", [&called3, &success3, &value3](bool success, std::string &&value) {called3 = true; success3 = success; value3 = std::move(value); });
        size_t nb_op;
        ASSERT_NO_THROW(nb_op = ioc.run_for(time_out));
        // ASSERT_EQ(3 * 2, nb_op);
        ASSERT_TRUE(ioc.stopped());
        ASSERT_TRUE(called1);
        ASSERT_TRUE(success1);
        ASSERT_EQ("v1", value1);
        ASSERT_TRUE(called2);
        ASSERT_FALSE(success2);
        ASSERT_EQ("", value2);
        ASSERT_TRUE(called3);
        ASSERT_TRUE(success3);
        ASSERT_EQ("v4", value3);
        ioc.restart();
    }
    // async_set and async_get pipeline
    {
        bool called1 = false;
        rh.async_set("c:1", "v5", [&called1] {called1 = true; });
        bool called2 = false;
        rh.async_set("c:2", "v6", [&called2] {called2 = true; });
        bool called3 = false; bool success1; std::string value1;
        rh.async_get("c:2", [&called3, &success1, &value1](bool success, std::string &&value) {called3 = true; success1 = success; value1 = std::move(value); });
        bool called4 = false; bool success2; std::string value2;
        rh.async_get("c:1", [&called4, &success2, &value2](bool success, std::string &&value) {called4 = true; success2 = success; value2 = std::move(value); });
        ASSERT_FALSE(called1);
        ASSERT_FALSE(called2);
        ASSERT_FALSE(called3);
        ASSERT_FALSE(called4);
        size_t nb_op;
        ASSERT_NO_THROW(nb_op = ioc.run_for(time_out));
        // ASSERT_EQ(4 * 2, nb_op);
        ASSERT_TRUE(ioc.stopped());
        ASSERT_TRUE(called1);
        ASSERT_TRUE(called2);
        ASSERT_TRUE(called3);
        ASSERT_TRUE(success1);
        ASSERT_EQ("v6", value1);
        ASSERT_TRUE(called4);
        ASSERT_TRUE(success2);
        ASSERT_EQ("v5", value2);
        ioc.restart();
    }
    // multi_get
    {
        std::vector<std::string> values;
        ASSERT_NO_THROW(values = rh.multi_get({ "a:1", "b:1", "a:2", "b:2" }));
        ASSERT_EQ(4, values.size());
        ASSERT_EQ("v1", values[0]);
        ASSERT_EQ("v3", values[1]);
        ASSERT_EQ("v2", values[2]);
        ASSERT_EQ("v4", values[3]);
    }
    // async_get handler throw
    {
        rh.async_get("a:1", [](bool, std::string &&) {
            throw std::string();
        });
        ASSERT_THROW(ioc.run_for(time_out), std::string);
        ioc.restart();
    }
}
