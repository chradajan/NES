#include "CPU.hpp"
#include "Exceptions.hpp"
#include <iostream>
#include <fstream>

CPU::CPU(const char* file)
{
	registers = new CPU_Registers();

	registers->P = 0x34;
	registers->A = 0;
	registers->X = 0;
	registers->Y = 0;
	registers->S = 0xFD;
	
	memory[0x4017] = 0x00;
	memory[0x4015] = 0x00;
	for(int i = 0x4000; i <= 0x400F; ++i)
		memory[i] = 0x00;
	//TODO: Set Noise Channel to 0x0000

	loadROM(file);
}

CPU::~CPU()
{
	delete registers;
}

void CPU::loadROM(const char* file)
{
	uint16_t loc = 0x8000;
	std::ifstream rom(file);
	uint8_t temp1, temp2;
	uint8_t value;

	while(rom >> std::hex >> temp1)
	{
		rom >> std::hex >> temp2;
		temp1 = convertAscii(temp1);
		temp2 = convertAscii(temp2);

		value = (unsigned int)temp1*16 + (unsigned int)temp2;

		memory[loc] = value;
		++loc;
	}

	if(loc <= 0xBFFF)
	{
		uint16_t mirrorLoc = 0xC000;
		for(uint16_t i = 0x8000; i <= loc; ++i)
		{
			memory[mirrorLoc] = memory[i];
			++mirrorLoc;
		}
	}
	
	rom.close();
}

uint8_t CPU::convertAscii(uint8_t c)
{
	if(c >= 48 && c <= 57)
			return c - 48;
		else if(c >= 65 && c <= 70)
			return c - 55;
		else if(c >= 97 && c <= 102)
			return c - 87;
		else
			throw BadRom{};
}


void CPU::CPU_TESTING()
{

}