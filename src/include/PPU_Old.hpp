#ifndef PPU_OLD_H
#define PPU_OLD_H
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

	//Internal Registers
	uint16_t v;
	uint16_t t;
	uint8_t x;
	bool w;
	uint16_t lowPTShiftReg, highPTShiftReg;
	uint8_t lowATShiftReg, highATShiftReg;
	uint8_t spritePTData[8];
	uint8_t spriteAttribtuteData[8];
	uint8_t spriteXCounters[8];

	int scanline;
	int dot;
	bool oddFrame;

	void incrementDot();

	//Read/Write
	uint8_t read(uint16_t address) const;
	void write(uint16_t address, uint8_t data);

	//Registers
	void setSpriteOverflowFlag(bool condition);
	void setVBlankFlag(bool condition);
	bool ifSpriteOverflow();
	bool ifBackgroundRendering();
	bool ifSpriteRendering();

	//Scanlines
	void preRenderScanline();
	void visibleScanline();
	void vBlankScanline();
	uint8_t lowPT_Temp, highPT_Temp, lowAT_Temp, highAT_Temp, tileNum;
	int vramFetchCycle;
	void vramBackgroundFetch();

	//Rendering
	char* frameBuffer;
	void getPixel();

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

	PPU_Registers(PPU& p);
	bool NMI();
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);

private:
	bool nmi;
	uint8_t PPUDATA_Buffer;
	uint16_t addressLatch;
	uint16_t getAddressLatch();
	PPU& ppu;
	void incremenetPPUADDR();
};

#endif