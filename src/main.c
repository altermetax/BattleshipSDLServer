#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_thread.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "client.h"
#include "request.h"

int main(int argc, char** argv) {
    if(argc == 2) {
        usedPort = atoi(argv[1]);
    } else {
        usedPort = DEFAULT_PORT;
    }

    printf("Using port %d\n", usedPort);

    if(SDL_Init(SDL_INIT_EVENTS & SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "Error: couldn't initialize SDL:\n%s\n", SDL_GetError());
        exit(1);
    }

    if(SDLNet_Init() != 0) {
        fprintf(stderr, "Error: couldn't initialize SDL2_net:\n%s\n", SDLNet_GetError());
        exit(1);
    }
    
    IPaddress ipAddress;
    if(SDLNet_ResolveHost(&ipAddress, NULL, usedPort) != 0) {
        fprintf(stderr, "Error: couldn't resolve host:\n%s\n", SDLNet_GetError());
        exit(1);
    }

    if(!(socketSet = SDLNet_AllocSocketSet(MAX_CONNECTIONS))) {
        fprintf(stderr, "Error: couldn't allocate socket set:\n%s\n", SDLNet_GetError());
        exit(1);
    }

    TCPsocket serverSocket;
    if(!(serverSocket = SDLNet_TCP_Open(&ipAddress))) {
        fprintf(stderr, "Error: couldn't open TCP port for listening:\n%s\n", SDLNet_GetError());
        exit(1);
    }

    if(!SDLNet_TCP_AddSocket(socketSet, serverSocket)) {
        fprintf(stderr, "Error: couldn't add socket to socket set:\n%s\n", SDLNet_GetError());
        exit(1);
    }

    Client** clients = allocClients(MAX_CONNECTIONS);
    waitingClient = NULL;

    int nextIndex = 0;
    unsigned char running = 1;
    while(running) {
        SDLNet_CheckSockets(socketSet, 1000);
        if(SDLNet_SocketReady(serverSocket)) {
            TCPsocket clientSocket = SDLNet_TCP_Accept(serverSocket);
            SDLNet_TCP_AddSocket(socketSet, clientSocket);
            unsigned char assigned = 0;
            Client* client;
            for(int i = 0; i < MAX_CONNECTIONS && !assigned; i++) {
                if(clients[i]->socket == NULL) {
                    initClient(clients[i], clientSocket);
                    client = clients[i];
                    assigned = 1;
                }
            }
            if(!assigned) {
                fprintf(stderr, "Error: MAX_CONNECTIONS (%d) reached, and client is attempting connection.\n");
                continue;
            }
            IPaddress* clientIp = SDLNet_TCP_GetPeerAddress(clientSocket);
            if(!clientIp) fprintf(stderr, "Error: got a connection, but can't obtain IP address:\n%s\n", SDLNet_GetError());
            const char* hostname = SDLNet_ResolveIP(clientIp);
            if(!hostname) fprintf(stderr, "Error: got a connection, IP address obtained, but can't resolve it:\n%s\n", SDLNet_GetError());
            printf("Accepted connection from %s.\n", hostname);
            strcpy(client->hostname, hostname);
        } else {
            for(int i = 0; i < MAX_CONNECTIONS; i++) {
                if(clients[i]->socket != NULL && SDLNet_SocketReady(clients[i]->socket)) {
                    // Get 256 characters at a time
                    char buf[256] = "";
                    int result = SDLNet_TCP_Recv(clients[i]->socket, buf, 255);
                    if(result < 0) {
                        fprintf(stderr, "Error: couldn't receive from client:\n%s\n", SDLNet_GetError());
                    }
                    else if(result == 0) {
                        printf("[%s quit]\n", clients[i]->hostname);
                        closeClient(clients[i]);
                    }
                    else {
                        handleRequests(clients[i], buf);
                    }
                    /*char* cr = clients[i]->currentRequest;
                    char c;
                    char quit = 0;
                    int len;
                    do {
                        int result = SDLNet_TCP_Recv(clients[i]->socket, &c, 1);
                        if(result < 0) {
                            fprintf(stderr, "Error: couldn't receive from client:\n%s\n", SDLNet_GetError());
                            quit = 1;
                        }
                        else if(result == 0) {
                            printf("[%s quit]\n", clients[i]->hostname);
                            closeClient(clients[i]);
                            quit = 1;
                        }
                        else {
                            len = strlen(cr);
                            cr[len] = c;
                            cr[len + 1] = 0;
                        }
                    } while(!quit && (len < 4 || cr[len - 3] != '\r' || cr[len - 2] != '\n' || cr[len - 1] != '\r' || cr[len] != '\n'));

                    if(!quit) {
                        printf("[%s] %s", clients[i]->hostname, cr);
                        handleRequest(clients[i]);
                        if(strcmp(cr, "quit\r\n\r\n") == 0) {
                            closeClient(clients[i]);
                        }
                    }*/
                }
            }
        }
    }

    SDLNet_FreeSocketSet(socketSet);
    SDLNet_TCP_Close(serverSocket);
}