#include <stdbool.h>

#include "utf8_utils.h"

/**
 * \brief Checks if a character is a separation character.
 *
 * Determines if a given character is a separation character, such as hyphens, quotes, or brackets.
 *
 * \param character The Unicode codepoint of the character to check.
 * \return true if the character is a separation character, false otherwise.
 */
bool isSeparationCharacter(unsigned int character) {

    switch (character) {
        case 0x2d: // -
        case 0x22: // "
        case 0xe28093: // –
        case 0xe2809c: // “
        case 0xe2809d: // ”
        case 0x5b: // [
        case 0x5d: // ]
        case 0x28: // (
        case 0x29: // )
            return true;
        default:
            return false;
    }

}


/**
 * \brief Checks if a character is a whitespace character.
 *
 * Determines if a given character is a whitespace character, such as space, tab, or newline.
 *
 * \param character The Unicode codepoint of the character to check.
 * \return true if the character is a whitespace character, false otherwise.
 */
bool isWhiteSpace(unsigned int character) {

    switch (character) {
        case 0x20: // Space
        case 0x9: // Tab
        case 0xA: // New Line
        case 0xD: // Carriage Return
            return true;
        default:
            return false;
    }

}


/**
 * \brief Checks if a character is a punctuation mark.
 *
 * Determines if a given character is a punctuation mark, such as periods, commas, or question marks.
 *
 * \param character The Unicode codepoint of the character to check.
 * \return true if the character is a punctuation mark, false otherwise.
 */
bool isPunctuationMark(unsigned int character) {

    switch (character) {
        case 0x2e: // .
        case 0x2c: // ,
        case 0x3a: // :
        case 0x3b: // ;
        case 0x3f: // ?
        case 0x21: // !
        case 0xe28093: // Dash
        case 0xe28094: // — EM Dash
        case 0xe280a6: // Ellipsis (...)
            return true;
        default:
            return false;
    }

}


/**
 * \brief Checks if a character is an aggregation mark.
 *
 * Determines if a given character is an aggregation mark, such as apostrophes.
 *
 * \param character The Unicode codepoint of the character to check.
 * \return true if the character is an aggregation mark, false otherwise.
 */
bool isAggregationMark(unsigned int character) {

    switch (character) {
        case 0x27: // '
        case 0xe28098: // ’
        case 0xe28099: // ’
            return true;
        default:
            return false;
    }

}


/**
 * \brief Determines the number of bytes in a UTF-8 character.
 *
 * Calculates the number of bytes that make up a UTF-8 character based on the first byte.
 *
 * \param character The first byte of the UTF-8 character.
 * \return The number of bytes in the UTF-8 character, or -1 if the character is invalid.
 */
int numOfBytesInUTF8(unsigned char character) {

    if ((character & 0x80) == 0)
    { // 0xxxxxxx
        return 1;
    }
    else if ((character & 0xE0) == 0xC0)
    { // 110xxxxx
        return 2;
    }
    else if ((character & 0xF0) == 0xE0)
    { // 1110xxxx
        return 3;
    }
    else if ((character & 0xF8) == 0xF0)
    { // 11110xxx
        return 4;
    }
    else
    {
        // Invalid UTF-8 character
        return -1;
    }

}


/**
 * \brief Checks if a character is alphanumeric.
 *
 * Determines if a given character is alphanumeric, including numbers, letters, and specific special characters.
 *
 * \param character The Unicode codepoint of the character to check.
 * \return true if the character is alphanumeric, false otherwise.
 */
bool isAlphanumeric(unsigned int character)
{

    // Character is a number
    if (character >= 0x30 && character <= 0x39)
    {
        return true;
    }

    // Character is a lowercase letter
    if (character >= 0x41 && character <= 0x5a)
    {
        return true;
    }

    // Character is an uppercase letter
    if (character >= 0x61 && character <= 0x7a)
    {
        return true;
    }

    // Character is a ç/Ç
    if (character == 0xc7 || character == 0xe7)
    {
        return true;
    }

    // Character is underscore
    if (character == 0x5f)
    {
        return true;
    }

    if ((character >= 0xC0 && character <= 0xC5) || (character >= 0xC8 && character <= 0xCF) || (character >= 0xD2 && character <= 0xD6) || (character >= 0xD9 && character <= 0xDC) || (character >= 0xE0 && character <= 0xE5) || (character >= 0xE8 && character <= 0xEF) || (character >= 0xF2 && character <= 0xF6) || (character >= 0xF9 && character <= 0xFC))
    {
        return true;
    }

    return false;
}

/**
 * \brief Converts UTF-8 bytes to a Unicode codepoint.
 *
 * Converts a sequence of UTF-8 bytes to their corresponding Unicode codepoint.
 *
 * \param bytes The array of UTF-8 bytes.
 * \param num_bytes The number of bytes in the sequence.
 * \return The Unicode codepoint, or -1 if the number of bytes is invalid.
 */
int utf8_to_codepoint(unsigned char *bytes, int num_bytes)
{
    int codepoint = 0;
    switch (num_bytes)
    {
    case 1:
        codepoint = bytes[0];
        break;
    case 2:
        codepoint = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
        break;
    case 3:
        codepoint = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
        break;
    case 4:
        codepoint = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) | ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
        break;
    default:
        // Invalid number of bytes
        return -1;
    }
    return codepoint;
}


/**
 * \brief Converts a character to lowercase.
 *
 * Converts a given character to its lowercase equivalent if it is an uppercase letter.
 *
 * \param c The Unicode codepoint of the character to convert.
 * \return The lowercase equivalent of the character, or the character itself if it is not an uppercase letter.
 */
int to_lower_case(int c)
{
    // if c is a uppercase letter
    if (c >= 0x41 && c <= 0x5a)
    {
        return c + 0x20;
    }

    return c;
}
