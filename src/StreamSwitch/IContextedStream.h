#pragma once

typedef uint16_t write_context_t;
class IContextedStream {
   public:
    virtual size_t write(uint8_t b, write_context_t id) = 0;
    virtual void update() = 0;
};