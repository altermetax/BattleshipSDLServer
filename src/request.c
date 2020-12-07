#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <SDL2/SDL_net.h>
#include "request.h"
#include "client.h"
#include "globals.h"

void handleRequests(Client* client, char* buffer) {
    printf("[%s]%s[/%s]\n", client->hostname, buffer, client->hostname);
    char* reqEnd = strstr(buffer, "\r\n\r\n");
    if(reqEnd) {
        // End buffer at reqEnd & add it to client request
        int i, len = strlen(client->currentRequest);
        char* p;
        for(i = 0, p = buffer; p < reqEnd + 4; p++, i++) {
            client->currentRequest[len + i] = *p;
        }
        client->currentRequest[len + i] = 0;
        // Handle this request
        handleRequest(client);
        // Then look for further requests
        handleRequests(client, reqEnd + 4);
    }
    else {
        // This means no request end is found in buffer - then just copy entire buffer
        strcat(client->currentRequest, buffer);
        handleRequest(client);
    }
}

void handleRequest(Client* client) {
    char req[65536];
    memcpy(req, client->currentRequest, strlen(client->currentRequest) + 1);
    int len = strlen(req);
    if(len == 0 || strcmp(req + len - 4, "\r\n\r\n") != 0) return; // Ignore if request is not complete yet (i.e. doesn't end with double newline)
    req[strlen(req) - 4] = 0; // Trunk string before 1st \n
    if(strlen(req) == 0) { // Request was just double newline with nothing before
        client->currentRequest[0] = 0;
        return;
    }
    
    char* lineSavePtr;
    char* header = strtok_r(req, "\r\n", &lineSavePtr);
    if(header == NULL) return; // Ignore if the string was just newlines

    if(strcmp(header, "ping") == 0) sendToClient(client, "pong\r\n\r\n");
    else if(strcmp(header, "hello") == 0) handleHelloRequest(client, &lineSavePtr);
    else if(strcmp(header, "ready") == 0) handleReadyRequest(client, &lineSavePtr);
    else if(strcmp(header, "attack") == 0) handleAttackRequest(client, &lineSavePtr);
    else if(strcmp(header, "quit") == 0) closeClient(client);

    else sendToClient(client, "wrong_header\r\n\r\n");

    client->currentRequest[0] = 0;
}

void handleHelloRequest(Client* client, char** lineSavePtr) {
    if(client->state != NO_HELLO) {
        sendToClient(client, "forbidden\r\n\r\n");
        return;
    }

    enum {
        VERSION = 1u << 0,
        NAME = 1u << 1,
        ROWS = 1u << 2,
        COLS = 1u << 3
    } requestState = 0;

    char* line;
    while(1) {
        line = strtok_r(NULL, "\r\n", lineSavePtr);
        if(line == NULL) break;
        
        char* argSavePtr;
        char* param = strtok_r(line, " ", &argSavePtr);
        
        if(strcmp(param, "version") == 0) {
            char* v = strtok_r(NULL, " ", &argSavePtr);
            if(v == NULL) {
                sendToClient(client, "wrong_parameter_syntax\r\n\r\n");
                return;
            }
            if(strcmp(v, PROTOCOL_VERSION) != 0) {
                sendToClient(client, "wrong_version\r\n\r\n");
                return;
            }
            requestState |= VERSION;
        }

        else if(strcmp(param, "name") == 0) {
            char* name = strtok_r(NULL, " ", &argSavePtr);
            if(name == NULL) {
                sendToClient(client, "wrong_parameter_syntax\r\n\r\n");
                return;
            }
            strcpy(client->name, name);
            requestState |= NAME;
        }

        else if(strcmp(param, "rows") == 0) {
            char* v = strtok_r(NULL, " ", &argSavePtr);
            if(strcmp(v, ACCEPT_ROWS) != 0) {
                sendToClient(client, "unsupported_rows\r\n\r\n");
                return;
            }
            requestState |= ROWS;
        }

        else if(strcmp(param, "cols") == 0) {
            char* v = strtok_r(NULL, " ", &argSavePtr);
            if(strcmp(v, ACCEPT_COLS) != 0) {
                sendToClient(client, "unsupported_cols\r\n\r\n");
                return;
            }
            requestState |= COLS;
        }

        else {
            sendToClient(client, "wrong_parameter\r\n\r\n");
            return;
        }
    }

    char complete = VERSION | NAME | COLS | ROWS;
    if((requestState & complete) != complete) {
        sendToClient(client, "not_enough_parameters\r\n\r\n");
        return;
    }

    if(waitingClient) {
        client->opponent = waitingClient;
        client->state = PLACING_SHIPS;
        waitingClient->opponent = client;
        waitingClient->state = PLACING_SHIPS;

        char clientString[256] = "matched\r\nname ";
        char waitingClientString[256] = "matched\r\nname ";
        strcat(clientString, waitingClient->name);
        strcat(clientString, "\r\n\r\n");
        strcat(waitingClientString, client->name);
        strcat(waitingClientString, "\r\n\r\n");

        sendToClient(client, clientString);
        sendToClient(waitingClient, waitingClientString);

        waitingClient = NULL;
    }
    else {
        waitingClient = client;
        client->state = WAITING_MATCH;
        sendToClient(client, "wait_match\r\n\r\n");
    }
}

void handleReadyRequest(Client* client, char** lineSavePtr) {
    if(client->state != PLACING_SHIPS) {
        sendToClient(client, "forbidden\r\n\r\n");
        return;
    }
    
    char* line;
    char shipsDone = 0;
    while(1) {
        line = strtok_r(NULL, "\r\n", lineSavePtr);
        if(line == NULL) break;

        char* argSavePtr;
        char* param = strtok_r(line, " ", &argSavePtr);

        if(strcmp(param, "ships_begin") == 0) {
            if(interpretShips(client, lineSavePtr) != 0) return;
            shipsDone = 1;
        }
    }
    if(!shipsDone) {
        sendToClient(client, "not_enough_parameters\r\n\r\n");
        return;
    }

    if(client->opponent->state == WAITING_SHIPS) {
        srand(time(0));
        char firstTurn = rand() & 1;
        if(firstTurn == 0) {
            client->state = OWN_TURN;
            client->opponent->state = WAITING_TURN;
            sendToClient(client, "your_turn\r\n\r\n");
            sendToClient(client->opponent, "wait_turn\r\n\r\n");
        }
        else {
            client->opponent->state = OWN_TURN;
            client->state = WAITING_TURN;
            sendToClient(client->opponent, "your_turn\r\n\r\n");
            sendToClient(client, "wait_turn\r\n\r\n");
        }
    }
    else {
        client->state = WAITING_SHIPS;
        sendToClient(client, "wait_ships\r\n\r\n");
    }
}

void handleAttackRequest(Client* client, char** lineSavePtr) {
    if(client->state != OWN_TURN) {
        sendToClient(client, "forbidden\r\n\r\n");
        return;
    }

    char* coordsStr = strtok_r(NULL, "\r\n", lineSavePtr);
    if(coordsStr == NULL) {
        sendToClient(client, "not_enough_parameters\r\n\r\n");
        return;
    }

    char* argSavePtr;
    char* xStr = strtok_r(coordsStr, " ", &argSavePtr);
    char* yStr = strtok_r(NULL, " ", &argSavePtr);
    if(xStr == NULL || yStr == NULL) {
        sendToClient(client, "wrong_parameter_syntax\r\nsecond line should be <x> <y>\r\n\r\n");
        return;
    }

    char* endPtr;

    errno = 0;
    int x = strtol(xStr, &endPtr, 10);
    if(errno != 0 || endPtr == xStr) {
        sendToClient(client, "wrong_parameter_syntax\r\nsecond line should be <x> <y>\r\n\r\n");
        return;
    }

    errno = 0;
    int y = strtol(yStr, &endPtr, 10);
    if(errno != 0 || endPtr == yStr || *endPtr != 0) {
        sendToClient(client, "wrong_parameter_syntax\r\nsecond line should be <x> <y>\r\n\r\n");
        return;
    }

    if(client->opponent->ships[y][x] > 0) {
        char foundChar = client->opponent->ships[y][x];
        client->opponent->ships[y][x] = 1;

        // Check if there are left pieces of same ship / at all
        int countShipPieces = 0, countThisShipPieces = 0;
        for(int innerY = 0; innerY < INTERNAL_ROWS; innerY++) {
            for(int innerX = 0; innerX < INTERNAL_COLS; innerX++) {
                if(client->opponent->ships[innerY][innerX] > 1) {
                    countShipPieces++;
                    if(client->opponent->ships[innerY][innerX] == foundChar) countThisShipPieces++;
                }
            }
        }

        if(countShipPieces == 0) {
            sendToClient(client, "you_win\r\n\r\n");
            sendToClient(client->opponent, "you_lose\r\n\r\n");
            // Now place ships and play again
            client->state = PLACING_SHIPS;
            client->opponent->state = PLACING_SHIPS;
            return;
        }
        else if(countThisShipPieces == 0) {
            sendToClient(client, "hit_sunk\r\n\r\n");
            char toOpponent[64];
            sprintf(toOpponent, "hit_sunk\r\n%d %d\r\n\r\n", x, y);
            sendToClient(client->opponent, toOpponent);
        }
        else {
            sendToClient(client, "hit\r\n\r\n");
            char toOpponent[64];
            sprintf(toOpponent, "hit\r\n%d %d\r\n\r\n", x, y);
            sendToClient(client->opponent, toOpponent);
        }
    }
    else {
        sendToClient(client, "no_hit\r\n\r\n");
        char toOpponent[64];
        sprintf(toOpponent, "no_hit\r\n%d %d\r\n\r\n", x, y);
        sendToClient(client->opponent, toOpponent);
    }
    
    client->state = WAITING_TURN;
    client->opponent->state = OWN_TURN;
}
