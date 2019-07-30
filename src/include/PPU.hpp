#ifndef PPU_H
#define PPU_H
#include <cstdint>
#include "Cartridge.hpp"
#include "Types.hpp"

struct PPU_Registers;

class PPU
{
public:
    PPU(Cartridge* cartridge, RGB* colors, char* frameBuffer, bool& renderFrame);
    void tick();
    PPU_Registers& getRegisters();
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

    //Cartidge mapper
    Cartridge* cart;

    //Memory
    uint8_t VRAM[0x800];
	uint8_t primaryOAM[0x100];
	uint8_t secondaryOAM[0x20];
	uint8_t paletteRAM[0x20];
    RGB* colors;

    //Registers
    PPU_Registers* memMappedReg;
    InternalRegisters reg;

    //Scanline state
    int scanline, dot, fetchCycle, PT_offset;
    bool oddFrame;
    uint16_t NT_Addr;
    uint8_t NT_Byte, AT_Byte, BG_LowByte, BG_HighByte;
    uint16_t PT_Shift_Low, PT_Shift_High;
    uint8_t AT_Bits;

    //Read/write
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);

    //Address conversions
    uint16_t nametableAddress(uint16_t address);
    uint16_t paletteAddress(uint16_t address);
    uint16_t tileAddress();
    uint16_t attributeAddress();
    void setAttributeBits();

    //Scanline Operations
    void preRenderScanline();
    void visibleScanline();
    void backgroundFetchCycleEval();
    void incHoriV();
    void incVertV();
    void setHoriV();
    void setVertV();
    void incDot();

    //Sprites
    int N, M, secondaryOAM_Pointer;
    uint8_t OAM_Data;
    bool found8Sprites;
    void spriteEval();
    void spriteEvalWrite();
    void spriteOverflowEval();
    void spriteFetch();

    //Rendering
    char* frameBuffer;
    bool& renderFrame;
    int frameBufferPointer;
    void getPixel();
    void decodePixel(uint8_t paletteData);
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

    PPU_Registers(PPU& ppu);
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data, bool allowWrites);
    bool renderingEnabled();
    bool NMI() {return nmi;}
    void checkNMI();

private:
    PPU& ppu;
    uint16_t addressLatch;
    uint8_t dataBuffer;
    bool nmi;
};

#endif