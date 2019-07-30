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
	NES(const char* file, std::fstream& cpuLog, char* frameBuffer, bool& renderFrame);
	void tick();
	~NES();

private:
	HeaderData header;
	//PPU_Registers ppu_registers;
	APU_IO_Registers apu_io_registers;	
	Cartridge* cart;
	CPU* cpu;
	APU* apu;
	PPU* ppu;
	RGB* colors;

	//ROM Loading
	void loadROM(const char* file);
	void decodeHeader(std::ifstream& rom);

	//Palette
	void createPalette();
};

#endif