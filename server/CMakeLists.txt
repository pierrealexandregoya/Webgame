cmake_minimum_required(VERSION 3.0.0)
project(webgame-server)

set(Boost_USE_STATIC_LIBS TRUE)
# set(Boost_DEBUG TRUE)


find_package(Boost
  REQUIRED
  COMPONENTS system date_time regex
  )

set (SRC
    main.cpp
    Entity.hpp
    Entity.cpp
    server.cpp
    Server.hpp
    WsConn.hpp
    WsConn.cpp
)

include_directories(${Boost_INCLUDE_DIRS})

if(WIN32)
	add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -D_SCL_SECURE_NO_WARNINGS)
endif()

add_executable(webgame-server ${SRC})

if(WIN32)
    target_link_libraries(webgame-server ${Boost_LIBRARIES})
else()
    target_link_libraries(webgame-server ${Boost_LIBRARIES} pthread)
endif()
