#ifndef PPU_H
#define PPU_H
#include <cstdint>
#include "Cartridge.hpp"

struct PPU_Registers;

class PPU
{
public:
    PPU(Cartridge* cartridge);
    ~PPU();
private:
    friend struct PPU_Registers;
    struct InternalRegisters
    {
        uint16_t v, t; //15 bits
        uint8_t x; //3 bits
        bool w; //1 bit

        void clear()
        {
            x = v = t = 0x0000;
            w = false;
        }
    };
    uint8_t VRAM[0x800];
	uint8_t primaryOAM[0x100];
	uint8_t secondaryOAM[0x20];
	uint8_t paletteRAM[0x20];
    PPU_Registers* memMappedReg;
    InternalRegisters reg;
    Cartridge* cart;

    //Read/write
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);

    //Address conversions
    uint16_t nametableAddress(uint16_t address);
    uint16_t paletteAddress(uint16_t address);
    uint16_t tileAddress();
    uint16_t attributeAddress();

    //Scanline operations
    void incHoriV();
    void incVertV();
    void setHoriV();
    void setVertV();
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

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);
    bool renderingEnabled();

private:
    PPU& ppu;
    uint16_t addressLatch;
    uint8_t dataBuffer;
    bool nmi;
};

#endif