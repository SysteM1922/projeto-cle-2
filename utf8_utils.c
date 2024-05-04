#include <stdbool.h>

#include "utf8_utils.h"

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

