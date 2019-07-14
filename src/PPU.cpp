#include "PPU.hpp"
#include <iostream> //TODO: remove when done testing

PPU::PPU(std::ifstream& rom)
{
	loadROM(rom);
}

void PPU::tick()
{

}

PPU::~PPU()
{

}

void PPU::loadROM(std::ifstream& rom)
{
	loadNROM(rom);
}

void PPU::loadNROM(std::ifstream& rom)
{

}

uint8_t PPU::readMEMORY(uint16_t address)
{
	if(address <= 0x2FFF)
		return memory[address];
	else if(address <= 0x3EFF)
		return memory[address - 0x1000];
	else if(address <= 0x3F1F)
		return memory[address];
	else
		return memory[address - ((address / 0x20) * 0x20) + 0x3F00];
}

void PPU::writeMEMORY(uint16_t address, uint8_t data)
{
	//TODO: check for illegal writes
	memory[address] = data;
}


void PPU::PPU_TESTING()
{
}