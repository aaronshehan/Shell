#include <stdio.h>
#include <string.h>

#define CMDLENGTH 1000
#define HISTORYCHUNK 20

void insertCMD(char history[][CMDLENGTH], char* cmd);

void displayHistory(char history[][CMDLENGTH], int count);