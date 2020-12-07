cmake_minimum_required(VERSION 3.15)
project(BattleshipSDLServer C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(BattleshipSDLServer
        src/client.c
        src/globals.c
        src/main.c
        src/request.c
        src/ship.c)

target_link_libraries(BattleshipSDLServer SDL2 SDL2_net)
