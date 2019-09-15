#ifndef NES_HPP
#define NES_HPP
#include <cstdint>
#include <fstream>
#include "CPU.hpp"
#include "APU.hpp"
#include "PPU.hpp"
#include "Types.hpp"
#include "Cartridge.hpp"
#include "Exceptions.hpp"

class NES
{
public:
	NES(const char* file, char* frameBuffer);
	void prepareFrame();
	~NES();

private:
	HeaderData header;
	Cartridge* cart;
	CPU* cpu;
	APU* apu;
	PPU* ppu;
	Controllers* controllers;
	RGB* colors;
	bool frameReady = false;

	//ROM Loading
	void loadROM(const char* file);
	void decodeHeader(std::ifstream& rom);

	//Palette
	void createPalette();
};

#endif