#pragma once

#include "Arduino.h"

const char endOfRoutingCommand = '\0';

struct parsed_routed_stream_t {
    uint16_t context;
    uint8_t parsedByte;
};

class StreamRouterParser {
   public:
    /**
         * @brief Parse a Buffer to get the byte and context of a got message. Do not include \0 in the buffer. Will Parse in Format [context][byte]
         * 
         * @param dest 
         * @param buffer 
         * @param size 
         * @return true 
         * @return false 
         */
    static bool parse(parsed_routed_stream_t& dest, const char* buffer, uint16_t size) {
        if (size < 2)
            return false;

        // Parse Context
        uint16_t context;
        if (!parseInt(context, buffer, size - 1)) {
            return false;
        }

        // Buildup return things
        dest.context = context;
        dest.parsedByte = buffer[size - 1];

        return true;
    }

   private:
    static bool parseInt(uint16_t& dest, const char* buffer, uint16_t size) {
        uint16_t returnVal = 0;

        // Add Places before the dot
        uint16_t currentMulti = 1;
        for (int16_t i = size - 1; i >= 0; --i) {
            uint8_t currentNum;
            if (getNumerical(currentNum, buffer[i])) {
                returnVal += currentNum * currentMulti;
                currentMulti *= 10;
            } else {
                return false;
            }
        }

        dest = returnVal;
        return true;
    }

    static bool getNumerical(uint8_t& dest, char inputChar) {
        uint8_t returnVal = 0;

        switch (inputChar) {
            case '0':
                returnVal = 0;
                break;

            case '1':
                returnVal = 1;
                break;

            case '2':
                returnVal = 2;
                break;

            case '3':
                returnVal = 3;
                break;

            case '4':
                returnVal = 4;
                break;

            case '5':
                returnVal = 5;
                break;

            case '6':
                returnVal = 6;
                break;

            case '7':
                returnVal = 7;
                break;

            case '8':
                returnVal = 8;
                break;

            case '9':
                returnVal = 9;
                break;

            default:
                return false;
        }

        dest = returnVal;
        return true;
    }
};