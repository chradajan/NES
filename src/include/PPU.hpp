#ifndef PPU_H
#define PPU_H
#include <cstdint>
#include <fstream>
#include "Types.hpp"
#include "Cartridge.hpp"
#include "Exceptions.hpp"

struct PPU_Registers;

class PPU
{
public:
	PPU(Cartridge* cart);
	void tick();
	PPU_Registers& getRegisters();
	~PPU();
private:
	friend struct PPU_Registers;

	uint8_t VRAM[0x800];
	uint8_t primaryOAM[0x100];
	uint8_t secondaryOAM[0x20];
	uint8_t paletteRAM[0x20];
	Cartridge* cart;
	PPU_Registers* ppu_registers;

	int scanlineY;
	int scanlineX;

	//Read/Write
	uint8_t read(uint16_t address) const;
	void write(uint16_t address, uint8_t data);

	//Registers
	void setSpriteOverflowFlag(bool condition);
	bool ifSpriteOverflow();

	//Sprite evaluation
	uint8_t N;
	uint8_t M;
	uint8_t secondary_oam_loc;
	uint8_t oam_buffer;
	bool found8Sprites;
	void evaluateSprites();
	void clearSecondaryOAMByte();
	void spriteEvalRead();
	void spriteEvalWrite();
	void spriteOverflowEval();
	void spriteFetch();
};

struct PPU_Registers
{
	uint8_t PPUCTRL;
	uint8_t PPUMASK;
	uint8_t PPUSTATUS;
	uint8_t OAMADDR;
	uint8_t OAMDATA;
	uint8_t PPUSTROLL;
	uint8_t PPUADDR;
	uint8_t PPUDATA;

	//For CPU debugging
	int cycle;
	int scanline;

	PPU_Registers(PPU& p);
	bool NMI();
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);

private:
	bool nmi;
	uint8_t PPUDATA_Buffer;
	uint8_t addressLatch;
	PPU& ppu;
	void incremenetPPUADDR();
};

#endif