#ifndef SPRITE_HPP
#define SPRITE_HPP

#include <cstdint>

class Sprite
{
public:
    uint8_t Y;
	uint8_t tile;
	uint8_t attributes;
	int X;
	uint8_t PT_High, PT_Low;

	int offset;
	bool sprite0;

    void clear();
	bool decrementX();
	uint8_t getPixelNibble();
};

#endif