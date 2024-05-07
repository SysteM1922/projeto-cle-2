/**
 * \file workers.c
 * \brief Worker functions for processing data chunks.
 *
 * This file contains functions for processing data chunks, specifically designed for
 * parallel processing tasks. It includes functions for identifying consonants in words,
 * checking if a word has two equal consonants, and processing chunks of data to
 * count words and words with consonants. These functions are crucial for text
 * analysis tasks in parallel computing environments.
 *
 * \author Pedro Rasinhas - 103541
 * \author Guilherme Antunes - 103600
 */

#include <stdlib.h>
#include "workers.h"
#include <stdlib.h>
#include "workers.h"
#include "utf8_utils.h"
#include "utils.h"
#include <string.h>
#include <wchar.h>
#include <wctype.h>

/**
 * \brief Checks if a character is a letter or underscore.
 *
 * \param c The wide character to check.
 * \return 1 if the character is a letter or underscore, 0 otherwise.
 */
int isLetter(wchar_t c) {
    return iswalnum(c) || c == L'_';
}

/**
 * \brief Checks if a character is a consonant.
 *
 * \param c The wide character to check.
 * \return 1 if the character is a consonant, 0 otherwise.
 */
int isConsonant(wchar_t c) {
    return wcschr(L"bcdfghjklmnpqrstvwxyz", c) != NULL;
}

/**
 * \brief Converts a complex letter to its simple form.
 *
 * \param letter The wide character to convert.
 */
void extractLetter(wchar_t *letter) {
    *letter = towlower(*letter);
    wchar_t *simpleLetters = L"c";
    wchar_t *complexLetters = L"ç";
 
    for (int i = 0; i < wcslen(complexLetters); i++) {
        if (complexLetters[i] == *letter) {
            *letter = simpleLetters[i];
            break;
        }
    }
}

/**
 * \brief Checks if a character is a consonant.
 *
 * Determines if a given character is a consonant, excluding the vowels 'a', 'e', 'i', 'o', and 'u'.
 *
 * \param c The Unicode codepoint of the character to check.
 * \return true if the character is a consonant, false otherwise.
 */
bool is_consonant(int c)
{
    return (c >= 0x61 && c <= 0x7a) && c != 0x61 && c != 0x65 && c != 0x69 && c != 0x6f && c != 0x75;
}

/**
 * \brief Checks if a word has at least two instances of the same consonant.
 *
 * Determines if a given word has at least two instances of the same consonant.
 *
 * \param word The array of Unicode codepoints representing the word.
 * \return true if the word has at least two instances of the same consonant, false otherwise.
 */
bool has_two_equal_consonants(int *word)
{
    int counts[26] = {0};

    for (int i = 0; word[i] != '\0'; i++)
    {
        int c = word[i];
        // ç or Ç
        if (c == 0xc7 || c == 0xe7)
        {
            c = 0x63; // c
        }

        int lower = to_lower_case(c);

        if (is_consonant(lower))
        {
            counts[lower - 0x61]++;
            if (counts[lower - 0x61] >= 2)
            {
                return true;
            }
        }
    }

    for (int i = 0; i < 26; i++)
    {
        counts[i] = 0;
    }

    return false;
}

/**
 * \brief Processes a chunk of data to count words and words with consonants.
 *
 * Processes a chunk of data to count the number of words and the number of words with at least two instances of the same consonant.
 *
 * \param DataBlock The data chunk to process.
 * \return The results of processing the data chunk, including the number of words and the number of words with consonants.
 */
ChunkResults processChunk(Chunk *DataBlock)
{

    unsigned char *data = DataBlock->chunk;

    ChunkResults filePartialResults;
    filePartialResults.fileIdx = DataBlock->fileIdx;
    filePartialResults.wordsCount = 0;
    filePartialResults.wordsWithConsonants = 0;

    bool inWord = false;
    bool found = false;
    wchar_t wc;
    int word_len = 0;

    wchar_t word[21];

    // get dataSize
    int dataSize = DataBlock->endPosition - DataBlock->startPosition;

    for (int i = 0; i < dataSize; i++)
    {
        unsigned char c = data[i];
        int char_size = numOfBytesInUTF8(c);
        unsigned char bytes[4] = {0, 0, 0, 0};
        bytes[0] = c;
        if (char_size > 1)
        {
            for (int j = 1; j < char_size; j++)
            {
                bytes[j] = data[++i];
            }
        }

        convertBytesToWchar(bytes, char_size, &wc);

        if (isLetter(wc))
        {
            if (found)
            {
                continue;
            }
            else
            {
                extractLetter(&wc);
                if (!inWord)
                {
                    filePartialResults.wordsCount++;
                    inWord = 1;
                }
                if (isConsonant(wc))
                {
                    for (int i = 0; i < word_len; i++)
                    {
                        if (word[i] == wc)
                        {
                            found = 1;
                            filePartialResults.wordsWithConsonants++;
                            break;
                        }
                    }
                    if (!found)
                    {
                        word[word_len] = wc;
                        word_len++;
                    }
                }
            }
        }
        else if (wc != L'’' && wc != L'‘' && wc != L'\'')
        {
            inWord = 0;
            found = 0;
            memset(word, '\0', sizeof(word));
            word_len = 0;
        }
    }

    return filePartialResults;
}