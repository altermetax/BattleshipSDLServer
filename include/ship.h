#pragma once
#include "globals.h"
#include "types.h"
#include "client.h"

char getTwoNumbersInShipsParam(Client* client, char** argSavePtr, int* a, int* b);
void dynamicStrcat(char** str, char* str2);
char interpretShips(Client* client, char** lineSavePtr);
char setClientShips(Client* client, Ship ships[]);