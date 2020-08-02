#pragma once

#include "ISimpleStream.h"

class SimpleStreamAdapter : public ISimpleStream {
   public:
    SimpleStreamAdapter(Stream& stream) : _stream(stream) {}

    int available() {
        return _stream.available();
    }

    size_t write(uint8_t b) {
        return _stream.write(b);
    }

    int read() {
        return _stream.read();
    }

   private:
    Stream& _stream;
};