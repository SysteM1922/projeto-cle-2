#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dispatcher.h"
#include "workers.h"
#include "utils.h"


// Function to get the size of a file
long getFileSize(const char *fileName) {
    FILE *file = fopen(fileName, "rb");
    if (file == NULL) {
        // perror("Failed to open file222");
        return -1;
    }

    fseek(file, 0L, SEEK_END); // Move to the end of the file
    long fileSize = ftell(file); // Get the current position (file size)
    fclose(file); // Close the file

    return fileSize;
}


bool isIn(unsigned int currentChar, char *consonants, int size) {
    for (int i = 0; i < size; i++) {
        if (currentChar == consonants[i]) {
            return true;
        }
    }
    return false;
}
