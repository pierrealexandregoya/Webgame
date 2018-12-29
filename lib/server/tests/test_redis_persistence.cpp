#include <chrono>

#include <boost/asio.hpp>

#include <gtest/gtest.h>

#include <webgame/entities.hpp>
#include <webgame/entity.hpp>
#include <webgame/npc.hpp>
#include <webgame/player.hpp>
#include <webgame/stationnary_entity.hpp>
#include <webgame/redis_persistence.hpp>

namespace asio = boost::asio;

std::chrono::seconds const time_out(10L);

TEST(redis_persistence, all)
{
    asio::io_context ioc;
    {
        std::shared_ptr<webgame::persistence> p = std::make_shared<webgame::redis_persistence>(ioc, "192.168.123.321", 6379, 1);
        ASSERT_FALSE(p->start());
    }
    std::shared_ptr<webgame::persistence> p_p = std::make_shared<webgame::redis_persistence>(ioc, "localhost", 6379, 1);
    webgame::persistence &p = *p_p;
    ASSERT_TRUE(p.start());
    ASSERT_NO_THROW(p.remove_all());
    webgame::id_t npc1_id;
    webgame::id_t object1_id;
    // async_save
    {
        webgame::entities ents;
        npc1_id = ents.add(std::make_shared<webgame::npc>("npc1", webgame::vector({ 1, 2 }), webgame::vector({ 3, 4 }), 5, 6, webgame::npc::behaviors({
            { 0, std::make_shared<webgame::arealimit>(webgame::arealimit::square, 0.5f, webgame::vector({ -0.5f, -0.5f })) }
            })))->id();
        object1_id = ents.add(std::make_shared<webgame::stationnary_entity>("object1", webgame::vector({ 6, 5 })))->id();
        //ents.add(std::make_shared<stationnary_entity>("building1", vector({ 6, 5 })));
        bool called1 = false;
        ASSERT_NO_THROW(p.async_save(ents, [&called1] {called1 = true; }));
        bool called2 = false;
        ASSERT_NO_THROW(p.async_save(ents, [&called2] {called2 = true; }));
        size_t nb_op;
        ASSERT_NO_THROW(nb_op = ioc.run_for(time_out));
        // Cannot predict number of op called (at least on linux)
        // ASSERT_EQ(2 * 2, nb_op);
        ASSERT_TRUE(ioc.stopped());
        ASSERT_TRUE(called1);
        ASSERT_TRUE(called2);
        ioc.restart();
    }
    // load_all_npes
    {
        webgame::entities ents;
        ASSERT_NO_THROW(ents = p.load_all_npes());
        ASSERT_EQ(2, ents.size());
        ASSERT_TRUE(ents.find(npc1_id) != ents.cend());
        ASSERT_TRUE(std::dynamic_pointer_cast<webgame::npc>(ents[npc1_id]));
        ASSERT_TRUE(ents.find(object1_id) != ents.cend());
        ASSERT_TRUE(std::dynamic_pointer_cast<webgame::stationnary_entity>(ents[object1_id]));
    }
    // async_check/add/get_player async_save
    {
        {
            bool called1 = false;
            bool exists1 = false;
            p.async_check_player("pseudo1", [&called1, &exists1](bool exists, webgame::id_t id) { called1 = true; exists1 = exists; });
            bool called2 = false;
            bool exists2 = false;
            p.async_check_player("pseudo2", [&called2, &exists2](bool exists, webgame::id_t id) { called2 = true; exists2 = exists;  });
            size_t nb_op;
            ASSERT_NO_THROW(nb_op = ioc.run_for(time_out));
            ASSERT_TRUE(ioc.stopped());
            ASSERT_TRUE(called1);
            ASSERT_FALSE(exists1);
            ASSERT_TRUE(called2);
            ASSERT_FALSE(exists2);
            ioc.restart();
        }

        {
            bool called1 = false;
            p.async_add_player("pseudo1", std::make_shared<webgame::upview_player>(), [&called1]() { called1 = true; });
            bool called2 = false;
            p.async_add_player("pseudo2", std::make_shared<webgame::upview_player>(), [&called2]() { called2 = true; });
            size_t nb_op;
            ASSERT_NO_THROW(nb_op = ioc.run_for(time_out));
            ASSERT_TRUE(ioc.stopped());
            ASSERT_TRUE(called1);
            ASSERT_TRUE(called2);
            ioc.restart();
        }

        webgame::id_t id1;
        webgame::id_t id2;
        {
            bool called1 = false;
            bool exists1 = false;
            p.async_check_player("pseudo1", [&called1, &exists1, &id1](bool exists, webgame::id_t id) { called1 = true; exists1 = exists; id1 = id; });
            bool called2 = false;
            bool exists2 = false;
            p.async_check_player("pseudo2", [&called2, &exists2, &id2](bool exists, webgame::id_t id) { called2 = true; exists2 = exists; id2 = id; });
            size_t nb_op;
            ASSERT_NO_THROW(nb_op = ioc.run_for(time_out));
            ASSERT_TRUE(ioc.stopped());
            ASSERT_TRUE(called1);
            ASSERT_TRUE(exists1);
            ASSERT_TRUE(called2);
            ASSERT_TRUE(exists2);
            ASSERT_NE(id1, id2);
            ioc.restart();
        }

        {
            bool called = false;
            bool exists = false;
            p.async_get_player(42, [&called, &exists](bool exists, std::shared_ptr<webgame::player> const& player_p) { called = true; exists = exists; });
            size_t nb_op;
            ASSERT_NO_THROW(nb_op = ioc.run_for(time_out));
            ASSERT_TRUE(ioc.stopped());
            ASSERT_TRUE(called);
            ASSERT_FALSE(exists);
            ioc.restart();
        }

        webgame::entities ents;
        std::shared_ptr<webgame::player> ent1;
        std::shared_ptr<webgame::player> ent2;
        {
            bool called1 = false;
            bool exists1 = false;
            p.async_get_player(id1, [&called1, &ent1, &exists1](bool exists, std::shared_ptr<webgame::player> const& ent_p) {called1 = true; ent1 = ent_p; exists1 = exists; });
            bool called2 = false;
            bool exists2 = false;
            p.async_get_player(id2, [&called2, &ent2, &exists2](bool exists, std::shared_ptr<webgame::player> const& ent_p) {called2 = true; ent2 = ent_p; exists2 = exists; });
            size_t nb_op;
            ASSERT_NO_THROW(nb_op = ioc.run_for(time_out));
            // ASSERT_EQ(3 * 2 + 3 * 2, nb_op);
            ASSERT_TRUE(ioc.stopped());
            ASSERT_TRUE(called1);
            ASSERT_TRUE(ent1);
            ASSERT_TRUE(exists1);
            ASSERT_TRUE(called2);
            ASSERT_TRUE(ent2);
            ASSERT_TRUE(exists2);
            ASSERT_NE(ent1, ent2);
            ASSERT_NE(*ent1, *ent2);
            ASSERT_EQ(webgame::vector({ 0, 0 }), ent1->pos());
            ASSERT_EQ(webgame::vector({ 0, -1 }), ent1->dir());
            ASSERT_EQ(1, ent1->max_speed());
            ASSERT_EQ(0, ent1->speed());
            ASSERT_EQ("upview_player", ent1->type());
            ASSERT_EQ(1, ent1.use_count());
            ioc.restart();
            
            ents.add(ent1);
            ASSERT_EQ(2, ent1.use_count());
            ents.add(ent2);
            ent1->set_dir({3.3f, 4.4f});
            ent1->set_speed(0.2f);
            ent2->set_dir({ 6.6f, 7.7f });
            ent2->set_speed(0.5f);
            bool called3 = false;
            p.async_save(ents, [&called3] {called3 = true; });
            //ioc.run_for(time_out);
            ioc.run();
            ASSERT_TRUE(called3);
            ioc.restart();
        }
        {
            bool called1 = false; std::shared_ptr<webgame::entity> ent3;
            p.async_get_player(id1, [&called1, &ent3](bool exists, std::shared_ptr<webgame::entity> const& ent_p) {called1 = true; ent3 = ent_p; });
            bool called2 = false; std::shared_ptr<webgame::entity> ent4;
            p.async_get_player(id2, [&called2, &ent4](bool exists, std::shared_ptr<webgame::entity> const& ent_p) {called2 = true; ent4 = ent_p; });
            size_t nb_op;
            ASSERT_NO_THROW(nb_op = ioc.run_for(time_out));
            // ASSERT_EQ(2 * 2 + 2 * 2, nb_op);
            ASSERT_TRUE(ioc.stopped());
            ASSERT_TRUE(called1);
            ASSERT_TRUE(ent3);
            ASSERT_TRUE(called2);
            ASSERT_TRUE(ent4);
            ASSERT_NE(ent3, ent4);
            ASSERT_TRUE(*ent1 == *ent3);
            ASSERT_TRUE(*ent2 == *ent4);
            ioc.restart();
        }
    }
    // load_all_npes
    {
        webgame::entities ents;
        ASSERT_NO_THROW(ents = p.load_all_npes());
        ASSERT_EQ(2, ents.size());
        ASSERT_TRUE(ents.find(npc1_id) != ents.cend());
        ASSERT_TRUE(std::dynamic_pointer_cast<webgame::npc>(ents[npc1_id]));
        ASSERT_TRUE(ents.find(object1_id) != ents.cend());
        ASSERT_TRUE(std::dynamic_pointer_cast<webgame::stationnary_entity>(ents[object1_id]));
    }
}
