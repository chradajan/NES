#ifndef NES_HPP
#define NES_HPP
#include <cstdint>
#include <fstream>
#include "CPU.hpp"
#include "APU.hpp"
#include "PPU.hpp"
#include "../mappers/Mapper.hpp"

class NES
{
public:
	NES(const char* file);
	~NES();
private:
	struct HeaderData
	{
		uint8_t PRG_ROM_SIZE;
		uint8_t CHR_ROM_SIZE;
		uint8_t Flags6, Flags7, Flags8, Flags9, Flags10;
	};
	HeaderData header;
	uint8_t* PPU_Registers;
	CPU* cpu;
	APU* apu;
	PPU* ppu;

	//ROM Loading
	void loadROM(std::ifstream& rom);
	uint8_t readByte(std::ifstream& rom);
	uint8_t convertAscii(uint8_t c);
	void decodeHeader(std::ifstream& rom);
	void loadNROM(std::ifstream& rom);
};

#endif