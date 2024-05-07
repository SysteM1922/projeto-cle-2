/**
 * \file utils.c
 * \brief Utility functions for file handling and character checking.
 *
 * This file contains utility functions for handling files and checking characters.
 * It includes a function for determining the size of a file and a function for checking if a character is in a given array.
 * These utilities are essential for file processing tasks and character analysis in applications.
 *
 * \author Pedro Rasinhas - 103541
 * \author Guilherme Antunes - 103600
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iconv.h>

#include "dispatcher.h"
#include "workers.h"
#include "utils.h"

/**
 * \brief Gets the size of a file.
 *
 * Opens a file in binary mode, seeks to the end to find the file size,
 * and then closes the file. Returns -1 if the file cannot be opened.
 *
 * \param fileName The name of the file to get the size of.
 * \return The size of the file in bytes, or -1 if the file cannot be opened.
 */
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

/**
 * \brief Checks if a character is in a given array.
 *
 * Iterates through an array of characters to determine if the current character is present.
 *
 * \param currentChar The character to check.
 * \param consonants The array of characters to search through.
 * \param size The size of the consonants array.
 * \return true if the character is found in the array, false otherwise.
 */
bool isIn(unsigned int currentChar, char *consonants, int size) {
    for (int i = 0; i < size; i++) {
        if (currentChar == consonants[i]) {
            return true;
        }
    }
    return false;
}

void convertBytesToWchar(unsigned char *bytes, size_t size, wchar_t *wc) {
    // Abrir o descritor de conversão
    iconv_t cd = iconv_open("WCHAR_T", "UTF-8");
    if (cd == (iconv_t)-1) {
        perror("iconv_open");
        return;
    }
    
    // Definir as variáveis para a conversão
    size_t inbytesleft = size; // Tamanho da entrada em bytes
    size_t outbytesleft = sizeof(wchar_t); // Tamanho da saída em bytes
    char *inbuf = (char*)bytes;
    char *outbuf = (char*)wc;
    
    // Converter bytes UTF-8 em um caractere amplo
    if (iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1) {
        printf("%ld\n", size);
        perror("iconv");
        iconv_close(cd);
        return;
    }
    
    // Fechar o descritor de conversão
    iconv_close(cd);
}