#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include <cstdint>

class Controllers
{
public:
    Controllers();
    uint8_t read(uint16_t address);
    void write(uint8_t data);
    ~Controllers();
private:
    bool S = false; //Strobe
    uint8_t JOY1 = 0x00;
    uint8_t JOY2 = 0x00;
};

#endif