#ifndef PPU_H
#define PPU_H

#include <iostream>
#include <cstdint>
#include "Cartridge.hpp"
#include "Types.hpp"

class PPU
{
public:
    PPU(Cartridge* cart, RGB* colors, char* frameBuffer, bool& frameReady);
    void tick();
    uint8_t readMemMappedReg(uint16_t address);
    void writeMemMappedReg(uint16_t address, uint8_t data);
    bool NMI();
    ~PPU();
private:
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

    //Memory
    PPU_Registers reg;
    uint8_t VRAM[0x800];
    uint8_t OAM[0x100];
    uint8_t OAM_Secondary[0x20];
    uint8_t paletteRAM[0x20];

    //Parameters
    Cartridge& cart;
    RGB* colors;
    char* frameBuffer;
    bool& frameReady;

    //State
    bool nmi, oddFrame;
    int scanline, dot;
    uint8_t NT_Byte, AT_Byte;

    //Read/write
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);
    uint16_t mirrored_NT_Addr(uint16_t address);
    uint16_t mirrored_Palette_Addr(uint16_t address);

    //Register checks
    void checkNMI();
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

    //Rendering
    bool AT_Latch_Low, AT_Latch_High;
    uint8_t AT_Shifter_Low, AT_Shifter_High, PT_Temp_Low, PT_Temp_High;
    uint16_t PT_Addr, PT_Shifter_Low, PT_Shifter_High;
    int frameBufferPointer;
    void getPixel();
    void renderPixel(uint8_t pixel);
};

#endif