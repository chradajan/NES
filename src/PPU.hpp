#ifndef PPU_H
#define PPU_H
#include <cstdint>
#include <fstream>

class PPU
{
public:
	PPU(uint8_t* cpu_mem, uint8_t* ppu_mem);
	void tick();
	~PPU();

	void PPU_TESTING();
private:
	uint8_t* memory;
	uint8_t& PPUCTRL;	//0x2000
	uint8_t& PPUMASK;	//0x2001
	uint8_t& PPUSTATUS;	//0x2002
	uint8_t& OAMADDR;	//0x2003
	uint8_t& OAMDATA;	//0x2004
	uint8_t& PPUSCROLL;	//0x2005
	uint8_t& PPUADDR;	//0x2006
	uint8_t& PPUDATA;	//0x2007
	uint8_t& OAMDMA;	//0x4014

	//Read/Write
	uint8_t readMEMORY(uint16_t address);
	void writeMEMORY(uint16_t address, uint8_t data);
};

#endif