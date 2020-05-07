#ifndef NES_HPP
#define NES_HPP

#include "CPU.hpp"
#include "APU.hpp"
#include "PPU.hpp"
#include "Types.hpp"
#include "Cartridge.hpp"
#include "Exceptions.hpp"
#include <cstdint>
#include <fstream>
#include <memory>

class NES
{
public:
	NES(const char* file, std::shared_ptr<char[]> frameBuffer);
	void prepareFrame();
	~NES();

private:
	Cartridge* cart;
	CPU* cpu;
	APU* apu;
	PPU* ppu;
	Controllers* controllers;
	RGB* colors;
	bool frameReady = false;

	//ROM Loading
	void initCart(const std::string& romPath);

	//Palette
	void createPalette();
};

#endif