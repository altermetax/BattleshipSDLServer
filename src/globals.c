#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include "globals.h"

unsigned int usedPort;
SDLNet_SocketSet socketSet;
Client* waitingClient;