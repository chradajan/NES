#include "include/Controllers.hpp"

Controllers::Controllers()
{

}

uint8_t Controllers::read(uint16_t address)
{
    (void)address;
    return 0x00;
}

void Controllers::write(uint8_t data)
{
    (void)data;
}

Controllers::~Controllers()
{

}