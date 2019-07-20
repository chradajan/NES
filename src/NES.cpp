#include "include/NES.hpp"
#include "mappers/NROM.hpp"
#include <cassert>

NES::NES(const char* file)
{
	loadROM(file);
	cpu = new CPU(mapper, ppu_registers, apu_io_registers);
	ppu = new PPU(mapper, ppu_registers);
}

NES::~NES()
{
	delete cpu;
	delete ppu;
	delete mapper;
}

void NES::loadROM(const char* file)
{
	std::ifstream rom(file, std::ios::binary);
	decodeHeader(rom);
	int mapperNumber = (header.Flags7 & 0xF0) + (header.Flags6 >> 4);
	switch(mapperNumber)
	{
		case 0:
			mapper = new NROM(header, rom);
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