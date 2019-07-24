#ifndef PPU_H
#define PPU_H
#include <cstdint>
#include <fstream>
#include "Types.hpp"
#include "Cartridge.hpp"
#include "Exceptions.hpp"

class PPU
{
public:
	PPU(Cartridge* cart, PPU_Registers& ppu_reg);
	void tick();
	~PPU();
private:
	uint8_t VRAM[0x800];
	uint8_t primaryOAM[0x100];
	uint8_t secondaryOAM[0x20];
	uint8_t paletteRAM[0x20];
	Cartridge* cart;
	PPU_Registers& ppu_registers;

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

#endif