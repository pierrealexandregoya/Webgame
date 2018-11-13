cmake_minimum_required(VERSION 3.0.0)
project(webgame-server)

set(Boost_USE_STATIC_LIBS TRUE)
# set(Boost_DEBUG TRUE)


find_package(Boost
  REQUIRED
  COMPONENTS system date_time regex filesystem serialization
  )

set (SRC
    misc/any.hpp
    misc/common.hpp
    misc/config.hpp
    misc/containers.hpp
    misc/filesystem.hpp
    misc/globals.cpp
    misc/json.hpp
    misc/json.cpp
    misc/log.hpp
    misc/nmoc.hpp
    misc/random.hpp
    misc/serialization.hpp
    misc/time.hpp
    misc/utils.hpp
    misc/utils.cpp
    misc/vector.hpp
    misc/vector.cpp

    behavior.hpp
    behavior.cpp
    entities.hpp
    entities.cpp
    entity.hpp
    entity.cpp
    env.hpp
    env.cpp
    persistence.hpp
    redis_conn.hpp
    redis_conn.cpp
    ws_conn.hpp
    ws_conn.cpp
)

include_directories(.)
include_directories(misc)

include_directories(third)
include_directories(${Boost_INCLUDE_DIRS})


# add_definitions(-DBOOST_ASIO_NO_DEPRECATED)
# add_definitions(-DBOOST_BIND_NO_PLACEHOLDERS)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=c++17)
endif()
  
if(WIN32)
	add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -D_SCL_SECURE_NO_WARNINGS)
endif()

if(MSVC)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
   add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()



add_executable(webgame-server server.hpp server.cpp main.cpp ${SRC})
add_executable(webgame-test test.cpp ${SRC})
add_executable(webgame-reset reset.cpp ${SRC})

if(WIN32)
    target_link_libraries(webgame-server ${Boost_LIBRARIES})
    target_link_libraries(webgame-test ${Boost_LIBRARIES})
    target_link_libraries(webgame-reset ${Boost_LIBRARIES})
else()
    target_link_libraries(webgame-server ${Boost_LIBRARIES} pthread stdc++fs)
    target_link_libraries(webgame-test ${Boost_LIBRARIES} pthread stdc++fs)
    target_link_libraries(webgame-reset ${Boost_LIBRARIES} pthread stdc++fs)
endif()
