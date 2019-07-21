#ifndef NES_HPP
#define NES_HPP
#include <cstdint>
#include <fstream>
#include "CPU.hpp"
#include "APU.hpp"
#include "PPU.hpp"
#include "Types.hpp"
#include "../mappers/Mapper.hpp"

class NES
{
public:
	NES(const char* file);
	void tick();
	~NES();

	//Debug
	void printRegisters();

private:
	HeaderData header;
	PPU_Registers ppu_registers;
	APU_IO_Registers apu_io_registers;	
	Mapper* mapper;
	CPU* cpu;
	APU* apu;
	PPU* ppu;

	//ROM Loading
	void loadROM(const char* file);
	void decodeHeader(std::ifstream& rom);
};

#endif