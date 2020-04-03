#include <SDL2/SDL_net.h>
#include "client.h"
#include "globals.h"
#include "request.h"

Client* allocClient() {
    Client* client = malloc(sizeof(Client));
    if(!client) return NULL;
    client->currentRequest = malloc(65536 * sizeof(char));
    if(!client->currentRequest) {
        fprintf(stderr, "Error: couldn't allocate current request buffer for client.\n");
        free(client);
        return NULL;
    }
    initClient(client, NULL);
    return client;
}

Client** allocClients(int amount) {
    Client** array = malloc(amount * sizeof(Client*));
    for(int i = 0; i < amount; i++) {
        array[i] = allocClient();
    }
    return array;
}

void initClient(Client* client, TCPsocket socket) {
    client->socket = socket;
    client->state = NO_HELLO;
    client->currentRequest[0] = 0;
    memset(client->ships, 0, INTERNAL_ROWS * INTERNAL_COLS);
    client->opponent = NULL;
}

void sendToClient(Client* client, const char* msg) {
    if(client && client->socket) if(SDLNet_TCP_Send(client->socket, msg, strlen(msg) + 1) < strlen(msg) + 1) {
        fprintf(stderr, "Error: couldn't send to client:\n%s\n", SDLNet_GetError());
        closeClient(client);
    }
}

void closeClient(Client* client) {
    SDLNet_TCP_Close(client->socket);
    SDLNet_TCP_DelSocket(socketSet, client->socket);
    client->socket = NULL;
    client->currentRequest[0] = 0;

    if(client == waitingClient) waitingClient = NULL;

    if(client->opponent) {
        sendToClient(client->opponent, "opponent_quit\r\n\r\n");
        client->opponent->state = WAITING_MATCH;
        client->opponent->opponent = NULL;
        client->opponent = NULL;
    }
}