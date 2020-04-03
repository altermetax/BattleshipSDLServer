#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ship.h"
#include "client.h"

char getTwoNumbersInShipsParam(Client* client, char** argSavePtr, int* a, int* b) {
    char* endPtr;

    char* aC = strtok_r(NULL, " ", argSavePtr);
    if(aC == NULL) {
        sendToClient(client, "wrong_ships_parameter_syntax\r\nfirst int value missing\r\n\r\n");
        return 1;
    }
    errno = 0;
    int tmpA = strtol(aC, &endPtr, 10);
    if(errno != 0 || endPtr == aC || *endPtr != 0) {
        sendToClient(client, "wrong_ships_parameter_syntax\r\nfirst int value not an int\r\n\r\n");
        return 1;
    }

    char* bC = strtok_r(NULL, " ", argSavePtr);
    if(bC == NULL) {
        sendToClient(client, "wrong_ships_parameter_syntax\r\nsecond int value missing\r\n\r\n");
        return 1;
    }
    errno = 0;
    int tmpB = strtol(bC, &endPtr, 10);
    if(errno != 0 || endPtr == bC || *endPtr != 0) {
        sendToClient(client, "wrong_ships_parameter_syntax\r\nsecond int value not an int\r\n\r\n");
        return 1;
    }

    *a = tmpA;
    *b = tmpB;
    return 0;
}

void dynamicStrcat(char** str, char* str2) {
	if(*str != NULL && str2 == NULL) { // reset *str
		free(*str);
		*str = NULL;
		return;
	}
	else if(*str == NULL) { // make new *str
		*str = malloc(strlen(str2) + 1 * sizeof(char));
		strcpy(*str, str2);
	} else { // append str2 to *str
		char* tmp = malloc((strlen(*str) + 1) * sizeof(char));
		strcpy(tmp, *str);
		*str = malloc((strlen(*str) + strlen(str2) + 1) * sizeof(char));
		strcpy(*str, tmp);
		strcpy(*str + strlen(*str), str2);
		free(tmp);
	}
}

char interpretShips(Client* client, char** lineSavePtr) {
    char* line;
    Ship ships[NUMBER_OF_SHIPS];
    int shipIndex = 0;
    while(1) {
        enum {
            NAME = 1u << 0,
            COORDS = 1u << 1,
            SIZE = 1u << 2,
            MATRIX = 1u << 3
        } shipState;

        line = strtok_r(NULL, "\r\n", lineSavePtr);
        if(strcmp(line, "ships_end") == 0) {
            if(shipIndex != 5) {
                sendToClient(client, "wrong_ships_amount\r\n\r\n");
                return 1;
            }
            if(setClientShips(client, ships) != 0) {
                sendToClient(client, "wrong_ships\r\n\r\n");
                return 1;
            }

            printf("[%s's ships:]\n", client->name);
            for(int y = 0; y < INTERNAL_ROWS; y++) {
                for(int x = 0; x < INTERNAL_COLS; x++) {
                    printf("%d", client->ships[y][x]);
                }
                printf("\n");
            }
            printf("\n\n");
            return 0;
        }
        else if(shipIndex == 5) {
            sendToClient(client, "wrong_ships_amount\r\n\r\n");
            return 1;
        }
        else if(line == NULL) { // End of request encountered before "ships_end"
            sendToClient(client, "wrong_ships_syntax\r\n\r\n");
            return 1;
        }

        // Line is a parameter
        char* argSavePtr;
        char* param = strtok_r(line, " ", &argSavePtr);

        if(strcmp(param, "name") == 0) {
            char* name = strtok_r(NULL, " ", &argSavePtr);
            if(name == NULL) {
                sendToClient(client, "wrong_ships_parameter_syntax\r\nname missing\r\n\r\n");
                return 1;
            }
            strcpy(ships[shipIndex].name, name);
            shipState |= NAME;
        }

        else if(strcmp(param, "coords") == 0) {
            if(getTwoNumbersInShipsParam(client, &argSavePtr, &ships[shipIndex].x, &ships[shipIndex].y) == 0)
                shipState |= COORDS;
            else return 1;
        }

        else if(strcmp(param, "size") == 0) {
            if(getTwoNumbersInShipsParam(client, &argSavePtr, &ships[shipIndex].sizeX, &ships[shipIndex].sizeY) == 0)
                shipState |= SIZE;
            else return 1;
        }

        else if(strcmp(param, "matrix_begin") == 0) {
            if(shipState & SIZE == 0) {
                sendToClient(client, "wrong_ships_parameter_syntax\r\nsize must be specified before matrix\r\n\r\n");
                return 1;
            }
            char* matrix = NULL;
            char* tok = strtok_r(NULL, "\r\n", lineSavePtr);
            int i;
            for(i = 0; strcmp(tok, "matrix_end"); i++) {
                if(strlen(tok) != ships[shipIndex].sizeX) {
                    sendToClient(client, "wrong_ships\r\n\r\n");
                    return 1;
                }
                dynamicStrcat(&matrix, tok);
                tok = strtok_r(NULL, "\r\n", lineSavePtr);
            }
            if(i != ships[shipIndex].sizeY) {
                sendToClient(client, "wrong_ships\r\n\r\n");
                free(matrix);
                return 1;
            }
            ships[shipIndex].matrix = matrix;
            shipState |= MATRIX;
        }

        else if(strcmp(param, "ship_end") == 0) {
            char complete = NAME | COORDS | SIZE | MATRIX;
            if((shipState & complete) != complete) {
                sendToClient(client, "wrong_ships_parameter_syntax\r\nincomplete ship\r\n\r\n");
                return 1;
            }
            shipIndex++;
        }
    }
}

char setClientShips(Client* client, Ship* ships) {
    for(int i = 0; i < NUMBER_OF_SHIPS; i++) {
        for(int y = 0; y < ships[i].sizeY; y++) {
            for(int x = 0; x < ships[i].sizeX; x++) {
                int index = x + y * ships[i].sizeX;
                int absX = ships[i].x + x;
                int absY = ships[i].y + y;
                if(ships[i].matrix[index] != '*') {
                    if(absX < 0 || absX >= INTERNAL_COLS || absY < 0 || absY >= INTERNAL_ROWS) return 1;
                    client->ships[absY][absX] = i + 2; // 0 is for no ship - 1 is for destroyed ship - other numbers are for ships
                }
            }
        }
    }
    return 0;
}