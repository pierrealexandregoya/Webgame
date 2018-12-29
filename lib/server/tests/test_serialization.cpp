#include <gtest/gtest.h>

#include <webgame/behavior.hpp>
#include <webgame/entities.hpp>
#include <webgame/entity.hpp>
#include <webgame/env.hpp>
#include <webgame/npc.hpp>
#include <webgame/player.hpp>
#include <webgame/save_load.hpp>
#include <webgame/stationnary_entity.hpp>

TEST(serialization, npc_all_behaviors)
{
    std::shared_ptr<webgame::entity> orig_ent = std::make_shared<webgame::npc>("npc_enemy_1", webgame::vector({ -0.5, -1.5 }), webgame::vector({ 1.5, 2.5 }), 0.4, 1.8, webgame::npc::behaviors({
            { -10, std::make_shared<webgame::arealimit>(webgame::arealimit::square, 0.5, webgame::vector({ -0.5, -0.5 })) } ,
            { 0, std::make_shared<webgame::attack_on_sight>(0.7) },
            { 10, std::make_shared<webgame::stop>() },
            { 10, std::make_shared<webgame::walkaround>() },
        }));
    webgame::entities ents;
    webgame::env env(ents);
    orig_ent->update(5, env);
    std::string orig_str = orig_ent->save().dump();
    ASSERT_FALSE(orig_str.empty());
    //std::cout << orig_str << std::endl;

    std::shared_ptr<webgame::entity> cp_ent;
    ASSERT_NO_THROW(cp_ent = webgame::load_entity(nlohmann::json::parse(orig_str)));
    ASSERT_TRUE(std::dynamic_pointer_cast<webgame::npc>(cp_ent));
    ASSERT_EQ(*orig_ent, *cp_ent);
}

TEST(serialization, player)
{
    std::shared_ptr<webgame::upview_player> orig_ent = std::make_shared<webgame::upview_player>();

    orig_ent->move_to({ 10, 10 });
    webgame::entities ents;
    webgame::env env(ents);
    orig_ent->update(5, env);

    std::string orig_str = orig_ent->save().dump();
    ASSERT_FALSE(orig_str.empty());

    std::shared_ptr<webgame::entity> cp_ent;
    ASSERT_NO_THROW(cp_ent = webgame::load_entity(nlohmann::json::parse(orig_str)));
    ASSERT_TRUE(std::dynamic_pointer_cast<webgame::player>(cp_ent));
    ASSERT_EQ(*orig_ent, *cp_ent);
}

TEST(serialization, stationnary_entity)
{
    std::shared_ptr<webgame::entity> orig_ent = std::make_shared<webgame::stationnary_entity>("test entity", webgame::vector({-0.5, -0.5}));

    std::string orig_str = orig_ent->save().dump();
    ASSERT_FALSE(orig_str.empty());

    std::shared_ptr<webgame::entity> cp_ent;
    ASSERT_NO_THROW(cp_ent = webgame::load_entity(nlohmann::json::parse(orig_str)));
    ASSERT_TRUE(std::dynamic_pointer_cast<webgame::stationnary_entity>(cp_ent));
    ASSERT_EQ(*orig_ent, *cp_ent);
}

TEST(serialization, protocol)
{
    nlohmann::json j1;
    webgame::npc("npc_enemy_1", webgame::vector({ -0.5, -1.5 }), webgame::vector({ 1.5, 2.5 }), 0.4, 1.8).build_state_order(j1);
    ASSERT_TRUE(j1.is_object());
    ASSERT_TRUE(j1.count("id"));
    ASSERT_TRUE(j1["id"].is_number_unsigned());
    ASSERT_TRUE(j1.count("type"));
    ASSERT_TRUE(j1["type"].is_string());
    ASSERT_TRUE(j1.count("pos"));
    ASSERT_TRUE(j1["pos"].is_object());
    ASSERT_TRUE(j1["pos"].count("x"));
    ASSERT_TRUE(j1["pos"]["x"].is_number_float());
    ASSERT_TRUE(j1["pos"].count("y"));
    ASSERT_TRUE(j1["pos"]["y"].is_number_float());
    ASSERT_TRUE(j1.count("dir"));
    ASSERT_TRUE(j1["dir"].is_object());
    ASSERT_TRUE(j1["dir"].count("x"));
    ASSERT_TRUE(j1["dir"]["x"].is_number_float());
    ASSERT_TRUE(j1["dir"].count("y"));
    ASSERT_TRUE(j1["dir"]["y"].is_number_float());
    ASSERT_TRUE(j1.count("speed"));
    ASSERT_TRUE(j1["speed"].is_number_float());
    ASSERT_FALSE(j1.count("max_speed"));

    nlohmann::json j2;
    webgame::upview_player().build_state_order(j2);
    ASSERT_TRUE(j2.is_object());

    nlohmann::json j3;
    webgame::stationnary_entity("test entity", webgame::vector({ -0.5, -0.5 })).build_state_order(j3);
    ASSERT_TRUE(j3.is_object());
    ASSERT_TRUE(j3.count("id"));
    ASSERT_TRUE(j3.count("type"));
    ASSERT_TRUE(j3.count("pos"));
    ASSERT_FALSE(j3.count("dir"));
    ASSERT_FALSE(j3.count("speed"));

}

class user_behavior: public webgame::behavior
{
public:
    std::string test_str_;
    bool *op_equal_called_;
    std::string type_name_;

public:
    user_behavior() = default;

    user_behavior(std::string const& test_str, bool *op_equal_called, std::string const& type_name)
        : test_str_(test_str)
        , op_equal_called_(op_equal_called)
        , type_name_(type_name)
    {}

public:
    virtual void update(double delta, webgame::env &env) override
    {
        self_->set_speed(0);
        self_->set_pos({42, 42});
    }

    virtual nlohmann::json save() const override
    {
        return {
            {"behavior", behavior::save()},
            {"test_str", test_str_},
            {"type", type_name_}
        };
    }

    virtual void load(nlohmann::json const& j) override
    {
        behavior::load(j["behavior"]);
        test_str_ = j["test_str"];
    }

public:
    virtual bool operator==(behavior const& o) const override
    {
        *op_equal_called_ = true;
        user_behavior const& other = dynamic_cast<decltype(other)>(o);
        return behavior::operator==(other)
            && test_str_ == other.test_str_;
    }
    virtual bool operator!=(behavior const& other) const override
    {
        return !(*this == other);
    }
};

WEBGAME_REGISTER(behavior, user_behavior);

TEST(serialization, user_behavior)
{
    bool op_equal_called = false;
    std::shared_ptr<webgame::entity> orig_ent = std::make_shared<webgame::npc>("npc_enemy_1", webgame::vector({ -0.5, -1.5 }), webgame::vector({ 1.5, 2.5 }), 0.4, 1.8, webgame::npc::behaviors({
        { 0, std::make_shared<user_behavior>("TEST STR", &op_equal_called, "user_behavior") } ,
        }));
    webgame::entities ents;
    webgame::env env(ents);
    orig_ent->update(5, env);
    std::string orig_str = orig_ent->save().dump();
    ASSERT_FALSE(orig_str.empty());

    std::shared_ptr<webgame::entity> cp_ent;
    ASSERT_NO_THROW(cp_ent = webgame::load_entity(nlohmann::json::parse(orig_str)));
    ASSERT_EQ(*orig_ent, *cp_ent);
    ASSERT_TRUE(op_equal_called);
    ASSERT_EQ(webgame::vector({ 42, 42 }), std::dynamic_pointer_cast<webgame::located_entity>(cp_ent)->pos());
}

TEST(serialization, user_behavior_not_registered)
{
    bool op_equal_called = false;
    std::shared_ptr<webgame::entity> orig_ent = std::make_shared<webgame::npc>("npc_enemy_1", webgame::vector({ -0.5, -1.5 }), webgame::vector({ 1.5, 2.5 }), 0.4, 1.8, webgame::npc::behaviors({
        { 0, std::make_shared<user_behavior>("TEST STR", &op_equal_called, "not_registered_type_name") } ,
        }));
    std::string orig_str = orig_ent->save().dump();
    std::shared_ptr<webgame::entity> cp_ent;
    ASSERT_THROW(cp_ent = webgame::load_entity(nlohmann::json::parse(orig_str)), std::runtime_error);
}

class user_entity: public webgame::mobile_entity
{
public:
    std::string test_str_;
    bool *op_equal_called_;
    std::string type_name_;

public:
    user_entity() = default;

    user_entity(std::string const& test_str, bool *op_equal_called, std::string const& type_name)
        : test_str_(test_str)
        , op_equal_called_(op_equal_called)
        , type_name_(type_name)
    {}

public:
    virtual bool update(double delta, webgame::env &env) override
    {
        set_speed(0);
        set_pos({ 42, 42 });
        return true;
    }

    virtual nlohmann::json save() const override
    {
        return {
            {"mobile_entity", mobile_entity::save()},
            {"test_str", test_str_},
            {"type", "user_entity"}
        };
    }

    virtual void load(nlohmann::json const& j) override
    {
        mobile_entity::load(j["mobile_entity"]);
        test_str_ = j["test_str"];
    }

public:
    virtual bool operator==(entity const& o) const override
    {
        *op_equal_called_ = true;
        user_entity const& other = dynamic_cast<decltype(other)>(o);
        return mobile_entity::operator==(other)
            && test_str_ == other.test_str_;
    }
    virtual bool operator!=(entity const& other) const override
    {
        return !(*this == other);
    }
};

WEBGAME_REGISTER(entity, user_entity);

TEST(serialization, user_entity)
{
    bool op_equal_called = false;
    std::shared_ptr<webgame::entity> orig_ent = std::make_shared<user_entity>("TEST STR", &op_equal_called, "user_entity");
    webgame::entities ents;
    webgame::env env(ents);
    orig_ent->update(5, env);
    std::string orig_str = orig_ent->save().dump();
    ASSERT_FALSE(orig_str.empty());

    std::shared_ptr<webgame::entity> cp_ent;
    ASSERT_NO_THROW(cp_ent = webgame::load_entity(nlohmann::json::parse(orig_str)));
    ASSERT_TRUE(std::dynamic_pointer_cast<user_entity>(cp_ent));
    ASSERT_EQ(*orig_ent, *cp_ent);
    ASSERT_TRUE(op_equal_called);
    ASSERT_EQ(webgame::vector({ 42, 42 }), std::dynamic_pointer_cast<webgame::located_entity>(cp_ent)->pos());
}

TEST(serialization, json)
{
    std::shared_ptr<webgame::entity> ent;
    ASSERT_NO_THROW(ent = webgame::load_entity(nlohmann::json::parse(R"(
    {
        "mobile_entity":
        {
            "located_entity":
            {
                "entity":
                {
                    "id":42,
                    "type":"test"
                },
                "pos":
                {
                    "x":0.0,
                    "y":0.0
                }
            },
            "dir":
            {
                "x":0.0,
                "y":-1.0
            },
            "speed":0.5,
            "max_speed":1.0
        },
        "type":"npc",
        "behaviors":
        [
            [
                -10,
                {
                    "behavior":
                    {
                        "resolved":false
                    },
                    "type":"arealimit",
                    "area_type":0,
                    "radius":1.0,
                    "center":
                    {
                        "x":-0.5,
                        "y":1.5
                    }
                }
            ],
            [
                10,
                {
                    "behavior":
                    {
                        "resolved":true
                    },
                    "type":"stop"
                }
            ]
        ]
    }
    )")));
    ASSERT_TRUE(std::dynamic_pointer_cast<webgame::npc>(ent));

    ASSERT_THROW(webgame::load_entity(nlohmann::json::parse(R"({"mobile_entity":{"located_entity":{"entity":{"id":42,"type":"test"},"pos":{"x":0,"y":0.0}},"dir":{"x":0.0,"y":-1.0},"speed":0.5,"max_speed":1.0},"type":"npc","behaviors":[]})")), std::runtime_error);
    ASSERT_THROW(webgame::load_entity(nlohmann::json::parse(R"({"mobile_entity":{"located_entity":{"entity":{"id":42,"type":"test"},"pos":{"x":0.0,"y":0.0}},"dir":{"x":0.0,"y":-1.0},"speed":0.5,"max_speed":1.0},"behaviors":[]})")), std::runtime_error);
    ASSERT_THROW(webgame::load_entity(nlohmann::json::parse(R"({"mobile_entity":{"located_entity":{"entity":{"id":42,"type":"test"},"pos":{"x":0.0,"y":0.0}},"dir":{"x":0.0,"y":-1.0},"speed":0.5,"max_speed":1},"type":"npc","behaviors":[]})")), std::runtime_error);
    ASSERT_THROW(webgame::load_entity(nlohmann::json::parse(R"({"mobile_entity":{"located_entity":{"entity":{"id":42,"type":"test"},"pos":{"x":0.0,"y":0.0}},"dir":{"x":0.0,"y":-1.0},"speed":0.5,"max_speed":1.0},"type":"npc","behaviors":{}})")), std::runtime_error);
    ASSERT_THROW(webgame::load_entity(nlohmann::json::parse(R"({"mobile_entity":{"located_entity":{"entity":{"id":42,"type":"test"},"pos":{"x":0.0,"y":0.0}},"dir":{"x":0.0},"speed":0.5,"max_speed":1.0},"type":"npc","behaviors":[]})")), std::runtime_error);
    ASSERT_THROW(webgame::load_entity(nlohmann::json::parse(R"({"mobile_entity":{"located_entity":{"pos":{"x":0.0,"y":0.0}},"dir":{"x":0.0,"y":-1.0},"speed":0.5,"max_speed":1.0},"type":"npc","behaviors":[]})")), std::runtime_error);
    ASSERT_THROW(webgame::load_entity(nlohmann::json::parse(R"({"mobile_entity":{"located_entity":{"entity":{"id":42,"type":"test"},"pos":{"x":0.0,"y":0.0}},"dir":{"x":0.0,"y":-1.0},"speed":0.5,"max_speed":1.0},"type":"npc","behaviors":[["str"]]})")), std::runtime_error);
}
