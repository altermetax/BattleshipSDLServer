#pragma once
#include <SDL2/SDL_net.h>

#ifndef INTERNAL_ROWS
#define INTERNAL_ROWS 10
#endif
#ifndef INTERNAL_COLS
#define INTERNAL_COLS 10
#endif

typedef struct {
    char name[20];
    int x;
    int y;
    int sizeX;
    int sizeY;
    char* matrix;
} Ship;

typedef enum {
    NO_HELLO = 0,
    WAITING_MATCH,
    PLACING_SHIPS,
    WAITING_SHIPS,
    OWN_TURN,
    WAITING_TURN
} ClientState;

typedef struct Client Client;
struct Client {
    TCPsocket socket;
    char hostname[32];
    char name[255];
    char ships[INTERNAL_ROWS][INTERNAL_COLS];
    ClientState state;
    char* currentRequest;
    Client* opponent;
};