#pragma once
#include <SDL2/SDL_net.h>
#include "types.h"
#include "ship.h"

Client* allocClient();
Client** allocClients(int amount);
void initClient(Client* client, TCPsocket socket);
void sendToClient(Client* client, const char* msg);
void closeClient(Client* client);