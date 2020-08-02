#pragma once

#include "Arduino.h"
#include "IContextedStream.h"
#include "ISimpleStream.h"
#include "StreamRouterParser.h"
#include "SimpleStreamAdapter.h"
#define VIRTUAL_SIMPLE_STREAM_RECEIVE_BUFFER 64
#define STREAM_RECEIVER_INPUT_BUFFER 64


template <uint16_t distantStreamCount>
class StreamReceiver {
   public:
    StreamReceiver(Stream& stream) : _physicalStream(stream) {
        for (write_context_t i = 0; i < distantStreamCount; ++i) {
            _virtualStreams[i].setStreamReceiver(*this, i);
        }
    }

    // ---------------------
    // IContextedStream

    void update() {
        while (_physicalStream.available()) {
            char currentChar = _physicalStream.read();

            if (currentChar == '\0') {
                parsed_routed_stream_t parsed;
                if (StreamRouterParser::parse(parsed, _inputBuffer, currentBufferSize)) {
                    if (parsed.context < distantStreamCount) {
                        _virtualStreams[parsed.context].physicalReceived(parsed.parsedByte);
                    }
                }

                currentBufferSize = 0;
            } else {
                _inputBuffer[currentBufferSize++] = currentChar;
            }
        }
    }

    /**
         * @brief Get the Stream with the given Number
         * 
         * @param streamNumber 
         * @return ISimpleStream& 
         */
    ISimpleStream& getStream(uint16_t streamNumber) {
        return _virtualStreams[streamNumber];
    }

   private:
    Stream& _physicalStream;
    char _inputBuffer[STREAM_RECEIVER_INPUT_BUFFER];
    uint16_t currentBufferSize = 0;

    /**
         * @brief Only important for the VirtualSimpleStream. Sending a Byte with context.
         * 
         * @param b 
         * @param context 
         * @return size_t 
         */
    size_t write(uint8_t b, write_context_t context) {
        if (context >= distantStreamCount)
            return 0;
        
        size_t writtenBytes = 0;
        
        writtenBytes += _physicalStream.print(context);
        writtenBytes += _physicalStream.write(b);
        writtenBytes += _physicalStream.write(endOfRoutingCommand);

        return writtenBytes;
    }


    class VirtualSimpleStream : public ISimpleStream {
       public:
        void setStreamReceiver(StreamReceiver& contextedWriteable, write_context_t context) {
            _contextedWriteable = &contextedWriteable;
            _context = context;
        }

        void physicalReceived(uint8_t b) {
            _inputBuffer.push(b);
        }

        // -------------------------------------
        // ISimpleStream

        int available() {
            return _inputBuffer.size();
        }

        size_t write(uint8_t b) {
            if (_contextedWriteable != nullptr) {
                return _contextedWriteable->write(b, _context);
            }

            return 0;
        }

        int read() {
            if (_contextedWriteable != nullptr) {
                _contextedWriteable->update();
            }

            if (_inputBuffer.empty())
                return -1;

            uint8_t returnVal;
            _inputBuffer.pop(returnVal);

            return returnVal;
        }

       private:
        CircularBuffer<uint8_t, VIRTUAL_SIMPLE_STREAM_RECEIVE_BUFFER> _inputBuffer;
        StreamReceiver* _contextedWriteable;
        write_context_t _context = 0;
    };

    VirtualSimpleStream _virtualStreams[distantStreamCount];
};