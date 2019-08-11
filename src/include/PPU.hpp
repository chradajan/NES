#ifndef PPU_H
#define PPU_H

#include <iostream>
#include <cstdint>
#include "Cartridge.hpp"
#include "Types.hpp"

class CPU;

class PPU
{
public:
    PPU(Cartridge* cart, RGB* colors, char* frameBuffer, bool& frameReady, int& FC);
    void tick();
    uint8_t readMemMappedReg(uint16_t address);
    void writeMemMappedReg(uint16_t address, uint8_t data);
    bool NMI();
    ~PPU();

private:
    friend class CPU;

    struct PPU_Registers
    {
        uint8_t PPUCTRL;
        uint8_t PPUMASK;
        uint8_t PPUSTATUS;
        uint8_t OAMADDR;
        uint8_t PPUDATA_Buffer;

        uint16_t v;
        uint16_t t;
        uint8_t x;
        bool w;
    };

    struct Sprite
    {
        uint8_t Y;
        uint8_t Tile;
        uint8_t Attributes;
        uint8_t X;
        uint8_t PT_Low, PT_High;
        int offset;

        void clear();
        uint16_t getPixel();
    };

    //Memory
    PPU_Registers reg;
    uint8_t VRAM[0x800];
    uint8_t OAM[0x100];
    //uint8_t OAM_Secondary[0x20];
    Sprite OAM_Secondary[8];
    Sprite OAM_Secondary_Current[8];
    uint8_t paletteRAM[0x20];

    //Parameters
    Cartridge& cart;
    RGB* colors;
    char* frameBuffer;
    bool& frameReady;

    //State
    bool oddFrame;
    int scanline, dot;
    uint8_t NT_Byte, AT_Byte;

    //Read/write
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);
    uint16_t mirrored_NT_Addr(uint16_t address);
    uint16_t mirrored_Palette_Addr(uint16_t address);

    //NMI
    bool nmi;
    void checkNMI();

    //Register checks
    bool renderingEnabled();

    //Scanline operations
    void prerenderScanline();
    void visibleScanline();
    void incDot();
    void incHoriV();
    void incVertV();
    void setHoriV();
    void setVertV();
    void shiftRegisters();
    void backgroundFetch();
    void setAttributeLatch();

    //Sprites
    int N, M, secondaryLoc, spriteCount;
    uint8_t OAM_Buffer;
    void spriteEval();
    void spriteEvalWrite();
    void spriteOverflowEval();
    void spriteFetch();

    //Rendering
    bool AT_Latch_Low, AT_Latch_High;
    uint8_t AT_Shifter_Low, AT_Shifter_High, PT_Temp_Low, PT_Temp_High;
    uint16_t PT_Addr, PT_Shifter_Low, PT_Shifter_High;
    int frameBufferPointer;
    void getPixel();
    uint16_t getBackgroundPixelAddress();
    void getSpritePixelAndRender(uint16_t BG_Pixel);
    void renderPixel(uint16_t pixelAddr);

    //Temp
    int& FC;
};

#endif