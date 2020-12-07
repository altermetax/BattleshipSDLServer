#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include "client.h"

#define PROTOCOL_VERSION "1.0"
#define ACCEPT_ROWS "10"
#define ACCEPT_COLS "10"
#define INTERNAL_ROWS 10
#define INTERNAL_COLS 10
#define DEFAULT_PORT 9098
#define MAX_CONNECTIONS 50
#define NUMBER_OF_SHIPS 5

extern unsigned int usedPort;
extern SDLNet_SocketSet socketSet;
extern Client* waitingClient;