#include "../include/Sprite.hpp"

void Sprite::clear()
{
    Y = tile = attributes = X = 0xFF;
    PT_High = PT_Low = 0x00;
    offset = 0;
    sprite0 = false;
}

bool Sprite::decrementX()
{
    --X;
	return (X <= -1 && X >= -8);
}

uint8_t Sprite::getPixelNibble()
{
    uint8_t pixelNibble = 0x00 | ((attributes & 0x03) << 2);
    if(attributes & 0x40) //Flip horizontally
    {
        pixelNibble |= ((PT_High & 0x01) << 1);
        PT_High >>= 1;
        pixelNibble |= (PT_Low & 0x01);
        PT_Low >>= 1;
    }
    else
    {
        pixelNibble |= ((PT_High & 0x80) >> 6);
        PT_High <<= 1;
        pixelNibble |= ((PT_Low & 0x80) >> 7);
        PT_Low <<= 1;
    }	
    return pixelNibble;	
}