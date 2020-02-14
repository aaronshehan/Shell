#include "myHistory.h"

void insertCMD(char history[][CMDLENGTH], char* cmd) {
	for (int i = 0; i <= HISTORYCHUNK - 1; i++) {
		strcpy(history[i], history[i + 1]);
	}
	strcpy(history[HISTORYCHUNK - 1], cmd);
}

void displayHistory(char history[][CMDLENGTH], int count) {
    printf("Command History\n");
	if (count <= HISTORYCHUNK) {
	    for (int i = 0; i < count; i++) {
	        printf("[%d] %s\n", i + 1, history[i]);
	    }
	}
	else {
        for (int i = 0; i < HISTORYCHUNK; i++) {
	        printf("[%d] %s\n", count - (HISTORYCHUNK - (i + 1)), history[i]);
	    }
	}
}