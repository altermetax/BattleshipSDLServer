#pragma once
#include "types.h"
#include "client.h"

void handleRequests(Client* client, char* buffer);
void handleRequest(Client* client);
void handleHelloRequest(Client* client, char** lineSavePtr);
void handleReadyRequest(Client* client, char** lineSavePtr);
void handleAttackRequest(Client* client, char** lineSavePtr);