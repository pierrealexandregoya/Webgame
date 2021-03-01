

# WebGame *(abandoned)*

This project was meant as a framework to ease the development of multiplayer games proposing a C++ library for the server part and some generic scripts in JS for a frontend in browser. It ended as a way too complex solution to achieve this goal regarding the existence of professional projects like Unity.

A main() function setting up and launching the server would look like that (see full code at lib/server/src/application.cpp):
```cpp
int application(int ac, char **av)
{
    std::cout << std::boolalpha;

    unsigned short  port = 2000;
    unsigned int    nb_threads = std::thread::hardware_concurrency();

    if (ac >= 2)
        port = std::stoi(av[1]);
    if (ac >= 3)
        nb_threads = std::stoi(av[2]);


    boost::asio::io_context ioc;

    std::vector<std::thread> network_threads;

    try {
        auto game_server = std::make_shared<server>(ioc, port,
                                                    std::make_shared<redis_persistence>(ioc, "localhost"));

        game_server->start();

        for (unsigned int i = 0; i < nb_threads; ++i)
            network_threads.emplace_back([&ioc, i] {
            WEBGAME_LOG("STARTUP", "THREAD #" << i + 1 << " RUNNING");
            try {
                ioc.run();
            }
            catch (std::exception const& e) {
                _WEBGAME_MY_LOG("EXCEPTION THROWN: " << e.what());
            }
            WEBGAME_LOG("STARTUP", "THREAD #" << i + 1 << " STOPPED");
        });

        handle_user_input(game_server);
    }
    catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
    }

    for (std::thread & t : network_threads)
        t.join();

#ifdef _WIN32
    system("PAUSE");
#endif

    return 0;
}
```

------------
The following demo is a proof of the concepts developed in this project. The player (the knight) enters the area the enemy npc (the skeleton) is limited to. When he does, the npc "sees" him and starts to track him until he exits the area (delimited here by the four crosses around him). Meanwhile an example of an ally npc does a kind a patrol in a similar restricted area.

![Demo](https://raw.githubusercontent.com/pierrealexandregoya/Webgame/ce30ea8e3906ed8834888cba2b6c24723bfbe674/extra/2021-02-07%2013-22-17.gif "Demo")

In terms of code, here is how these npc and objects (crosses) were declared:
```cpp
webgame::entities ents;

// First ally NPC
auto &ally = ents.add(std::make_shared<webgame::npc>("npc_ally_1", webgame::vector({ 0.5, 0.5 }),
    webgame::vector({ 0, 0 }), 0.2, 0.2, webgame::npc::behaviors({
        { 0, std::make_shared<webgame::arealimit>(webgame::arealimit::square, 0.5, webgame::vector({ 0.5, 0.5 })) } ,
        { 10, std::make_shared<webgame::walkaround>() }
    })));

// First enemy NPC
auto &enemy = ents.add(std::make_shared<webgame::npc>("npc_enemy_1",
    webgame::vector({ -0.5, -0.5 }), webgame::vector({ 0, 0 }), 0.4, 0.4, webgame::npc::behaviors({
        { 0, std::make_shared<webgame::arealimit>(webgame::arealimit::square, 0.5f, webgame::vector({ -0.5, -0.5 })) } ,
        { 10, std::make_shared<webgame::attack_on_sight>(0.7) },
        { 20, std::make_shared<webgame::stop>() }
    })));

// 100 static objects centered at 0,0
const int s = 10;
for (int i = 0; i < s; ++i)
    for (int j = 0; j < s; ++j)
        ents.add(std::make_shared<webgame::stationnary_entity>("object1", webgame::vector({ i - s / 2. , j - s / 2. })));
````


------------

## Shipped with

- **BRedis 0.11**
- **Nlohmann JSON 3.9.1**

## Tested with

- **Boost Asio/Beast 1.74**
- **Gtest 1.10**

## Credits:

- **Sprites by Alan H**

