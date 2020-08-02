#pragma once

class ISimpleStream {
   public:
    virtual int available() = 0;
    virtual size_t write(uint8_t b) = 0;
    virtual int read() = 0;
};