#pragma once

#include "Arduino.h"
#include "IContextedStream.h"
#include "ISimpleStream.h"
#include "StreamRouterParser.h"
#include "SimpleStreamAdapter.h"
#define STREAM_ROUTER_INPUT_BUFFER 64


template <uint16_t routedStreamCount>
class StreamRouter {
   public:
    StreamRouter(Stream& stream) : _physicalStream(stream) {}

    // ---------------------
    // IContextedStream

    void update() {
        while (_physicalStream.available()) {
            char currentChar = _physicalStream.read();

            if (currentChar == '\0') {
                parsed_routed_stream_t parsed;
                if (StreamRouterParser::parse(parsed, _inputBuffer, currentBufferSize)) {
                    if (parsed.context < routedStreamCount) {
                        _routedStreams[parsed.context].write(parsed.parsedByte);
                    }
                }

                currentBufferSize = 0;
            } else {
                _inputBuffer[currentBufferSize++] = currentChar;
            }
        }

        for (uint16_t i = 0; i < routedStreamCount; ++i) {
            _routedStreams[i].update();
        }
    }

    /**
     * @brief Set the Stream to a hardware stream
     * 
     * @param stream for Example Serial1
     * @param context the Number of the Stream
     */
    void setStream(Stream& stream, write_context_t context) {
        if (context >= routedStreamCount)
            return;

        _routedStreams[context].setStream(stream, *this, context);
    }

   private:
    Stream& _physicalStream;
    char _inputBuffer[STREAM_ROUTER_INPUT_BUFFER];
    uint16_t currentBufferSize = 0;

    /**
         * @brief Only important for the VirtualSimpleStream. Sending a Byte with context.
         * 
         * @param b 
         * @param context 
         * @return size_t 
         */
    size_t write(uint8_t b, write_context_t context) {
        if (context >= routedStreamCount)
            return 0;
        
        size_t writtenBytes = 0;
        
        writtenBytes += _physicalStream.print(context);
        writtenBytes += _physicalStream.write(b);
        writtenBytes += _physicalStream.write(endOfRoutingCommand);

        return writtenBytes;
    }


    class RoutedSimpleStream {
       public:
        void setStream(Stream& stream, StreamRouter& contextedWriteable, write_context_t context) {
            _physicalStream = &stream;
            _contextedWriteable = &contextedWriteable;
            _context = context;
        }

        // -------------------------------------
        // ISimpleStream

        // available() not needed!

        size_t write(uint8_t b) {
            if (_physicalStream != nullptr)
                return _physicalStream->write(b);
            
            return 0;
        }

        // read() not needed!

        void update() {
            if (_physicalStream != nullptr) {
                while (_physicalStream->available()) {
                    _contextedWriteable->write(_physicalStream->read(), _context);
                }
            }
        }

       private:
        Stream* _physicalStream = nullptr;
        StreamRouter* _contextedWriteable;
        write_context_t _context = 0;
    };

    RoutedSimpleStream _routedStreams[routedStreamCount];
};