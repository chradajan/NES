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
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);
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
    PPU_Registers reg;
    uint8_t VRAM[0x800];
    uint8_t OAM[0x100];
    uint8_t OAM_Secondary[0x20];

    Cartridge& cart;
    RGB* colors;
    char* frameBuffer;
    bool& frameReady;
};

#endif