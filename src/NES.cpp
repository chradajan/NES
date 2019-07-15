#include "NES.hpp"
#include "Exceptions.hpp"
#include <cassert>

NES::NES(const char* file)
{
	CPU_memory = new uint8_t[0x10000];
	PPU_memory = new uint8_t[0x4000];
	std::ifstream rom(file);
	loadROM(rom);
	cpu = new CPU(CPU_memory);
	ppu = new PPU(CPU_memory, PPU_memory);
	cpu->tick();
}

NES::~NES()
{
	delete[] CPU_memory;
	delete[] PPU_memory;
}

void NES::loadROM(std::ifstream& rom)
{
	decodeHeader(rom);
	loadNROM(rom);
	rom.close();
}

uint8_t NES::readByte(std::ifstream& rom)
{
	uint8_t temp1, temp2, value;
	rom >> std::hex >> temp1;
	rom >> std::hex >> temp2;
	temp1 = convertAscii(temp1);
	temp2 = convertAscii(temp2);
	value = (temp1 << 4) + temp2;
	return value;
}

uint8_t NES::convertAscii(uint8_t c)
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

void NES::decodeHeader(std::ifstream& rom)
{
	uint8_t value;
	uint32_t headerCheck = 0;

	for(int i = 0; i < 4; ++i)
	{
		value = readByte(rom);
		headerCheck = (headerCheck << 8) + value;
	}

	assert(headerCheck == 0x4E45531A);
	header.PRG_ROM_SIZE = readByte(rom);
	header.CHR_ROM_SIZE = readByte(rom);
	header.Flags6 = readByte(rom);
	header.Flags7 = readByte(rom);
	header.Flags8 = readByte(rom);
	header.Flags9 = readByte(rom);
	header.Flags10 = readByte(rom);

	for(int i = 0; i < 5; ++i)
		readByte(rom);
}

void NES::loadNROM(std::ifstream& rom)
{
	uint8_t data;
	if(header.PRG_ROM_SIZE == 0x01)
	{
		for(uint16_t loc = 0x8000; loc <= 0xBFFF; ++loc)
		{
			data = readByte(rom);
			CPU_memory[loc] = data;
			CPU_memory[loc + 0x4000] = data;
		}
	}
	else
	{
		for(uint16_t loc = 0x8000; loc >= 0x8000; ++loc)
		{
			CPU_memory[loc] = readByte(rom);
		}
	}

	if(header.CHR_ROM_SIZE == 0x01)
	{
		for(uint16_t loc = 0x0000; loc <= 0x1FFF; ++loc)
		{
			data = readByte(rom);
			PPU_memory[loc] = data;
		}
	}
	else
	{
		//Uses CHR RAM or has more than 8KB of CHR ROM. Need to figure this out
	}
}