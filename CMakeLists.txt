cmake_minimum_required(VERSION 3.3)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12")
  cmake_policy(VERSION 3.12)
endif()


project(webgame)


if ("cxx_std_17" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(CXX17_SUPPORTED TRUE)
else()
    set(CXX17_SUPPORTED FALSE)
endif()


set(BOOST_REQUIRED_COMPONENTS system date_time regex)

if (CXX17_SUPPORTED)
else()
    list(APPEND BOOST_REQUIRED_COMPONENTS filesystem)
endif()

find_package(Boost
    REQUIRED
    COMPONENTS ${BOOST_REQUIRED_COMPONENTS}
)

find_package(GTest REQUIRED)


if(WIN32)
    add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -D_SCL_SECURE_NO_WARNINGS)
endif()

if(MSVC)
    add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()


set(INCDIR lib/server/include)
set(SRCDIR lib/server/src)

#set(WEBGAME_STATIC TRUE)

if(WEBGAME_STATIC)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

set(WEBGAME_SRC
    ${INCDIR}/webgame/any.hpp
    ${INCDIR}/webgame/application.hpp
    ${INCDIR}/webgame/behavior.hpp
    ${INCDIR}/webgame/common.hpp
    ${INCDIR}/webgame/config.hpp
    ${INCDIR}/webgame/containers.hpp
    ${INCDIR}/webgame/entities.hpp
    ${INCDIR}/webgame/entities.hxx
    ${INCDIR}/webgame/entity.hpp
    ${INCDIR}/webgame/env.hpp
    ${INCDIR}/webgame/filesystem.hpp
    ${INCDIR}/webgame/lock.hpp
    ${INCDIR}/webgame/log.hpp
    ${INCDIR}/webgame/nmoc.hpp
    ${INCDIR}/webgame/npc.hpp
    ${INCDIR}/webgame/persistence.hpp
    ${INCDIR}/webgame/player.hpp
    ${INCDIR}/webgame/player_conn.hpp
    ${INCDIR}/webgame/protocol.hpp
    ${INCDIR}/webgame/random.hpp
    ${INCDIR}/webgame/redis_helper.hpp
    ${INCDIR}/webgame/redis_persistence.hpp
    ${INCDIR}/webgame/save_load.hpp
    ${INCDIR}/webgame/server.hpp
    ${INCDIR}/webgame/stationnary_entity.hpp
    ${INCDIR}/webgame/time.hpp
    ${INCDIR}/webgame/utils.hpp
    ${INCDIR}/webgame/vector.hpp

    ${SRCDIR}/application.cpp
    ${SRCDIR}/behavior.cpp
    ${SRCDIR}/entities.cpp
    ${SRCDIR}/entity.cpp
    ${SRCDIR}/env.cpp
    ${SRCDIR}/log.cpp
    ${SRCDIR}/npc.cpp
    ${SRCDIR}/player.cpp
    ${SRCDIR}/player_conn.cpp
    ${SRCDIR}/protocol.cpp
    ${SRCDIR}/random.cpp
    ${SRCDIR}/redis_helper.cpp
    ${SRCDIR}/redis_persistence.cpp
    ${SRCDIR}/save_load.cpp
    ${SRCDIR}/server.cpp
    ${SRCDIR}/stationnary_entity.cpp
    ${SRCDIR}/time.cpp
    ${SRCDIR}/utils.cpp
    ${SRCDIR}/vector.cpp
)

link_directories(${Boost_LIBRARY_DIRS})

if(UNIX)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)
endif()

add_library(webgame ${LIB_TYPE}
    ${WEBGAME_SRC}
)
add_library(webgame-tests ${LIB_TYPE}
    ${WEBGAME_SRC}
)
target_compile_definitions(webgame PRIVATE WEBGAME_BUILDING_THE_LIB)
target_compile_definitions(webgame-tests PRIVATE WEBGAME_BUILDING_THE_LIB)

if(WEBGAME_STATIC)
    target_compile_definitions(webgame PRIVATE WEBGAME_STATIC)
    target_compile_definitions(webgame-tests PRIVATE WEBGAME_STATIC)
endif()
target_compile_definitions(webgame-tests PRIVATE WEBGAME_TESTS)


add_executable(test-server
    lib/server/main_server.cpp
)

add_executable(test-bots
    lib/server/main_bots.cpp
)

add_executable(test-reset
    lib/server/main_reset.cpp
)

set(TESTDIR lib/server/tests)
add_executable(tests
    ${TESTDIR}/tests.hpp
    ${TESTDIR}/test_entity.cpp
    ${TESTDIR}/test_redis_helper.cpp
    ${TESTDIR}/test_serialization.cpp
    ${TESTDIR}/test_redis_persistence.cpp
    ${TESTDIR}/test_json.cpp
    ${TESTDIR}/test_server.cpp
)
target_compile_definitions(tests PRIVATE WEBGAME_TESTS)

add_executable(game
    game/main.cpp
)

if(WEBGAME_STATIC)
    target_compile_definitions(test-server PRIVATE WEBGAME_STATIC)
    target_compile_definitions(test-bots PRIVATE WEBGAME_STATIC)
    target_compile_definitions(test-reset PRIVATE WEBGAME_STATIC)
    target_compile_definitions(tests PRIVATE WEBGAME_STATIC)
endif()

if (CXX17_SUPPORTED)
    set(REQUIRED_STANDARD 17)
else()
    set(REQUIRED_STANDARD 11)
endif()

set_property(TARGET webgame PROPERTY CXX_STANDARD ${REQUIRED_STANDARD})
set_property(TARGET webgame PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET webgame-tests PROPERTY CXX_STANDARD ${REQUIRED_STANDARD})
set_property(TARGET webgame-tests PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET test-server PROPERTY CXX_STANDARD ${REQUIRED_STANDARD})
set_property(TARGET test-server PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET test-bots PROPERTY CXX_STANDARD ${REQUIRED_STANDARD})
set_property(TARGET test-bots PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET test-reset PROPERTY CXX_STANDARD ${REQUIRED_STANDARD})
set_property(TARGET test-reset PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET tests PROPERTY CXX_STANDARD ${REQUIRED_STANDARD})
set_property(TARGET tests PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories(webgame
PRIVATE
    ${INCDIR}/webgame
PUBLIC
    lib/server/third
    ${Boost_INCLUDE_DIRS}
)
target_include_directories(webgame-tests
PRIVATE
    ${INCDIR}/webgame
PUBLIC
    lib/server/third
    ${Boost_INCLUDE_DIRS}
)

target_include_directories(test-server PUBLIC ${INCDIR})
target_include_directories(test-bots PUBLIC ${INCDIR})
target_include_directories(test-reset PUBLIC ${INCDIR})
target_include_directories(tests PUBLIC ${INCDIR} ${GTEST_INCLUDE_DIRS})
target_include_directories(game PUBLIC ${INCDIR})

set(DEPS
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_REGEX_LIBRARY}
)

if(UNIX)
    list(APPEND DEPS pthread)
endif()

if (CXX17_SUPPORTED)
    if(UNIX)
        list(APPEND DEPS stdc++fs)
    endif()
else()
    list(APPEND DEPS ${Boost_FILESYSTEM_LIBRARY})
endif()

target_link_libraries(webgame ${DEPS})
target_link_libraries(webgame-tests ${DEPS})
target_link_libraries(test-server webgame)
target_link_libraries(test-bots webgame)
target_link_libraries(test-reset webgame)
target_link_libraries(tests webgame-tests ${GTEST_BOTH_LIBRARIES})
target_link_libraries(game webgame)
