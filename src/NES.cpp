#include "include/NES.hpp"
#include "mappers/NROM.hpp"
#include "mappers/MMC1.hpp"
#include <cassert>
#include <iomanip>

NES::NES(const char* file, char* frameBuffer)
{
	loadROM(file);
	createPalette();
	controllers = new Controllers();
	apu = new APU();
	ppu = new PPU(cart, colors, frameBuffer, frameReady);
	cpu = new CPU(cart, *ppu, *apu, *controllers);
}

void NES::prepareFrame()
{
	while(!frameReady)
	{
		cpu->tick();
		ppu->tick();
		ppu->tick();
		ppu->tick();
	}
	frameReady = false;
}

NES::~NES()
{
	delete cpu;
	delete ppu;
	delete apu;
	delete controllers;
	delete cart;
	delete[] colors;
}

void NES::loadROM(const char* file)
{
	std::ifstream rom(file, std::ios::binary);
	decodeHeader(rom);
	int mapperNumber = (header.Flags7 & 0xF0) + (header.Flags6 >> 4);
	switch(mapperNumber)
	{
		case 0:
			cart = new NROM(header, rom);
			break;
		case 1:
			cart = new MMC1(header, rom);
			break;
		default:
			throw;
	}
	rom.close();
}

void NES::decodeHeader(std::ifstream& rom)
{
	uint8_t temp;
	uint32_t headerCheck = 0;

	for(int i = 0; i < 4; ++i)
	{
		rom >> std::hex >> temp;
		headerCheck = (headerCheck << 8) + temp;
	}

	assert(headerCheck == 0x4E45531A);

	rom >> std::hex >> header.PRG_ROM_SIZE;
	rom >> std::hex >> header.CHR_ROM_SIZE;
	rom >> std::hex >> header.Flags6;
	rom >> std::hex >> header.Flags7;
	rom >> std::hex >> header.Flags8;
	rom >> std::hex >> header.Flags9;
	rom >> std::hex >> header.Flags10;

	for(int i = 0; i < 5; ++i)
		rom >> std::hex >> temp;
}

void NES::createPalette()
{
	uint8_t paletteArray[192] = 
	{
		0x7C, 0x7C, 0x7C,
		0x00, 0x00, 0xFC,
		0x00, 0x00, 0xBC,
		0x44, 0x28, 0xBC,
		0x94, 0x00, 0x84,
		0xA8, 0x00, 0x20,
		0xA8, 0x10, 0x00,
		0x88, 0x14, 0x00,
		0x50, 0x30, 0x00,
		0x00, 0x78, 0x00,
		0x00, 0x68, 0x00,
		0x00, 0x58, 0x00,
		0x00, 0x40, 0x58,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0xBC, 0xBC, 0xBC,
		0x00, 0x78, 0xF8,
		0x00, 0x58, 0xF8,
		0x68, 0x44, 0xFC,
		0xD8, 0x00, 0xCC,
		0xE4, 0x00, 0x58,
		0xF8, 0x38, 0x00,
		0xE4, 0x5C, 0x10,
		0xAC, 0x7C, 0x00,
		0x00, 0xB8, 0x00,
		0x00, 0xA8, 0x00,
		0x00, 0xA8, 0x44,
		0x00, 0x88, 0x88,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0xF8, 0xF8, 0xF8,
		0x3C, 0xBC, 0xFC,
		0x68, 0x88, 0xFC,
		0x98, 0x78, 0xF8,
		0xF8, 0x78, 0xF8,
		0xF8, 0x58, 0x98,
		0xF8, 0x78, 0x58,
		0xFC, 0xA0, 0x44,
		0xF8, 0xB8, 0x00,
		0xB8, 0xF8, 0x18,
		0x58, 0xD8, 0x54,
		0x58, 0xF8, 0x98,
		0x00, 0xE8, 0xD8,
		0x78, 0x78, 0x78,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0xFC, 0xFC, 0xFC,
		0xA4, 0xE4, 0xFC,
		0xB8, 0xB8, 0xF8,
		0xD8, 0xB8, 0xF8,
		0xF8, 0xB8, 0xF8,
		0xF8, 0xA4, 0xC0,
		0xF0, 0xD0, 0xB0,
		0xFC, 0xE0, 0xA8,
		0xF8, 0xD8, 0x78,
		0xD8, 0xF8, 0x78,
		0xB8, 0xF8, 0xB8,
		0xB8, 0xF8, 0xD8,
		0x00, 0xFC, 0xFC,
		0xF8, 0xD8, 0xF8,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00
	};

	colors = new RGB[64];
	std::ifstream paletteFile("../palette/palette.pal", std::ios::binary);
	int j = 0;
	for(int i = 0; i < 64; ++i)
	{
		// paletteFile >> std::hex >> colors[i].R;
		// paletteFile >> std::hex >> colors[i].G;
		// paletteFile >> std::hex >> colors[i].B;
		colors[i].R = paletteArray[j++];
		colors[i].G = paletteArray[j++];
		colors[i].B = paletteArray[j++];
	}
	paletteFile.close();
}