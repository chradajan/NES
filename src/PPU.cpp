#include "include/PPU.hpp"
#include <iostream> //TODO: remove when done testing

PPU::PPU(uint8_t* cpu_mem, uint8_t* ppu_mem) : 
	memory(ppu_mem), PPUCTRL(cpu_mem[0x2000]), PPUMASK(cpu_mem[0x2001]), PPUSTATUS(cpu_mem[0x2002]),
	OAMADDR(cpu_mem[0x2003]), OAMDATA(cpu_mem[0x2004]), PPUSCROLL(cpu_mem[0x2005]),
	PPUADDR(cpu_mem[0x2006]), PPUDATA(cpu_mem[0x2007]), OAMDMA(cpu_mem[0x4014])
{

}

void PPU::tick()
{

}

PPU::~PPU()
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

void PPU::evaluateSprites()
{
	if(scanlineX <= 64)
		clearSingleSecondaryOAMByte();
}

void PPU::clearSingleSecondaryOAMByte()
{
	secondary_oam[scanlineX % 32] = 0xFF;
}

void PPU::PPU_TESTING()
{
}