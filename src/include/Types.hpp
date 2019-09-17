#ifndef TYPES_H
#define TYPES_H

#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <iomanip>

#define uint unsigned int //Mingw doesn't recognize uintg

struct RGB
{
	RGB() 
	{
		R = G = B = 0x00;
	}
	friend std::ostream& operator<<(std::ostream& os, const RGB& rgb);
    uint8_t R, G, B;
};

inline std::ostream& operator<<(std::ostream& os, const RGB& rgb)
{
	os << std::hex << std::setfill('0') << "R: " << std::setw(2) << (uint)rgb.R << "  G: " << std::setw(2) << (uint)rgb.G << "  B: " << std::setw(2) << (uint)rgb.B;
	return os;  
}

struct Sprite
{
	uint8_t Y;
	uint8_t tile;
	uint8_t attributes;
	int X;
	uint8_t PT_High, PT_Low;

	int offset;
	bool sprite0;

	void clear()
	{
		Y = tile = attributes = X = 0xFF;
		PT_High = PT_Low = 0x00;
		offset = 0;
		sprite0 = false;
	}

	bool decrementX()
	{
		--X;
		return (X <= -1 && X >= -8);	
	}

	uint8_t getPixelNibble()
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
};

struct HeaderData
{
	uint8_t PRG_ROM_SIZE;
	uint8_t CHR_ROM_SIZE;
	uint8_t Flags6, Flags7, Flags8, Flags9, Flags10;
};

#endif