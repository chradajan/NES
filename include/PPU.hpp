#ifndef PPU_H
#define PPU_H

#include <cstdint>
#include "Cartridge.hpp"
#include "Types.hpp"

class PPU
{
public:
    PPU(Cartridge* cartridge, RGB* color, char* fb, bool& frameReady);
    uint8_t readMemMappedReg(uint16_t address);
    void writeMemMappedReg(uint16_t address, uint8_t data);
    void tick();
    bool NMI();
    ~PPU();
private:
    struct PPU_Registers
    {
        uint8_t PPUCTRL = 0;
        uint8_t PPUMASK = 0;
        uint8_t PPUSTATUS = 0;
        uint8_t OAMADDR = 0;
        uint8_t ReadBuffer = 0;

        uint16_t v = 0;
        uint16_t t = 0;
        uint8_t x = 0;
        bool w = false;
    };
    PPU_Registers reg;
    uint8_t VRAM[0x800];
    uint8_t OAM[0x100]; //Primary OAM
    Sprite OAM_Secondary[8]; //Used during sprite evaluation
    uint8_t paletteRAM[0x20];

    Cartridge& cart;
    RGB* colors;
    char* frameBuffer;
    bool& frameReady;

    bool vblank = true, nmi = false;
    int scanline = 0, dot = 30;
    bool oddFrame = false;

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);

    void prerenderScanline();
    void visibleScanline();

    bool renderingEnabled();
    void disabledRenderingDisplay();
    void setNMI();
    uint16_t paletteAddress(uint16_t address);

    void incHoriV();
    void incVertV();
    void setHoriV();
    void setVertV();
    void incDot();

    //Sprites
    bool checkSprite0Hit = false;
    uint16_t spritePixel = 0x0000;
    bool BG_Priority = true;
    void getSpritePixel();

    int N, M, OAM_Location, /*spriteCount,*/ spriteFetchCycle;
    //uint8_t OAM_Buffer;
    uint16_t Sprite_Pixel = 0x0000;
    void spriteEval();
    void spriteOverflowEval(int N);
    void spriteFetch();
    void sprite0Hit();

    //Background
    uint16_t BG_Pixel = 0x0000;
    void getBackgroundPixel();

    int backgroundFetchCycle = 0;
    uint8_t NT_Byte = 0x00, AT_Byte = 0x00, PT_High = 0x00, PT_Low = 0x00;
    uint16_t PT_Address = 0x0000;
    uint16_t PT_Shifter_High = 0x0000, PT_Shifter_Low = 0x0000;
    uint8_t AT_Shifter_High = 0x00, AT_Shifter_Low = 0x00;
    bool AT_Latch_High = false, AT_Latch_Low = false;
    void backgroundFetch();
    void shiftRegisters();
    void loadShiftRegisters();
    void setAttributeLatches();

    //Rendering
    int frameBufferPointer = 0;
    uint8_t pixelMultiplexer();
    void renderPixel();
};

#endif