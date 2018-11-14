#include <gtest/gtest.h>

#include <webgame/entity.hpp>
#include <webgame/env.hpp>
#include <webgame/npc.hpp>
#include <webgame/player.hpp>

#include "tests.hpp"

TEST(entity, all)
{
    ASSERT_FALSE(almost_equal(0.0000000000001, 0.));
    ASSERT_TRUE(almost_equal(0.0000000000001, 0.0000000000001));
    webgame::entities ents;
    webgame::env empty_env(ents);
    {
        webgame::npc ent1("none", { 0, 0 }, { 0, 1 }, 0.5, 1);
        ent1.update(1, empty_env);
        ASSERT_EQ(webgame::vector({ 0, 0.5 }), ent1.pos());
        ASSERT_EQ(webgame::vector({ 0, 1 }), ent1.dir());
        ASSERT_EQ(0.5, ent1.speed());
        ASSERT_EQ(1, ent1.max_speed());
        ent1.update(0.5, empty_env);
        ASSERT_EQ(webgame::vector({ 0, 0.75 }), ent1.pos());
    }
    {
        webgame::npc ent1("none", { -1, -1 }, { 1, 1 }, 0.5, 1);
        ASSERT_EQ(webgame::vector({ 1, 1 }), ent1.dir());
        ent1.update(1, empty_env);
        ASSERT_NE(webgame::vector({ 1, 1 }), ent1.dir());
        ASSERT_TRUE(almost_equal(1.0, ent1.dir().norm()));
        webgame::vector expected_dir(webgame::vector({ 1, 1 }) / webgame::vector({ 1, 1 }).norm());
        ASSERT_EQ(expected_dir, ent1.dir());
        ASSERT_EQ(webgame::vector(webgame::vector({ -1, -1 }) + expected_dir * 0.5), ent1.pos());
    }
    {
        webgame::npc ent1("none", { 0, 0 }, { 0, 0.00000001 }, 0.00000001, 1);
        ASSERT_TRUE(ent1.update(0.00000001, empty_env));
        webgame::npc ent2("none", { 0, 0 }, { 0, 0 }, 10000000, 10000000);
        ASSERT_FALSE(ent2.update(10000000, empty_env));
    }
}

TEST(entity, player)
{
    webgame::player p;

    webgame::entities ents;
    webgame::env empty_env(ents);

    p.set_speed(1);
    p.move_to({5, 0});
    ASSERT_TRUE(p.is_moving_to());
    ASSERT_EQ(webgame::vector({ 0, 0 }), p.pos());
    p.update(2.5, empty_env);
    ASSERT_TRUE(p.is_moving_to());
    ASSERT_EQ(webgame::vector({2.5, 0}), p.pos());
    p.update(10, empty_env);
    ASSERT_FALSE(p.is_moving_to());
    ASSERT_EQ(webgame::vector({5, 0 }), p.pos());

    p.move_to({ 0, 0 });
    ASSERT_FALSE(p.is_moving_to());

    p.set_speed(0.5);
    p.move_to({ 0, 0 });
    ASSERT_TRUE(p.is_moving_to());
    p.update(9.9 , empty_env);

    ASSERT_TRUE(p.is_moving_to());
    p.update(0.1, empty_env);
    ASSERT_FALSE(p.is_moving_to());
    ASSERT_EQ(webgame::vector({ 0, 0 }), p.pos());
}
