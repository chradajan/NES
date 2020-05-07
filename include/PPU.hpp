#ifndef PPU_HPP
#define PPU_HPP

#include "Cartridge.hpp"
#include "Sprite.hpp"
#include <cstdint>
#include <memory>
#include <array>

class PPU
{
public:
    PPU(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<char[]> frameBuffer, bool& frameReady);
    uint8_t readMemMappedReg(uint16_t address);
    void writeMemMappedReg(uint16_t address, uint8_t data);
    void tick();
    bool NMI();
private:
    struct PPU_Registers
    {
        uint8_t PPUCTRL;
        uint8_t PPUMASK;
        uint8_t PPUSTATUS;
        uint8_t OAMADDR;
        uint8_t ReadBuffer;

        uint16_t v;
        uint16_t t;
        uint8_t x;
        bool w;
    };
    PPU_Registers reg;

    struct RGB
    {
        uint8_t R, G, B;
    };
    std::array<RGB, 64> colors;

    std::shared_ptr<Cartridge> cart;
    std::shared_ptr<char[]> frameBuffer;
    bool& frameReady;

    std::array<uint8_t, 0x0800> VRAM;
    std::array<uint8_t, 0x0100> OAM;
    std::array<Sprite, 8> OAM_Secondary;
    std::array<uint8_t, 0x0020> paletteRAM;

    bool vblank;
    bool nmi;
    int scanline;
    int dot;
    bool oddFrame;

    void init();
    void readInColors();

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
    bool checkSprite0Hit;
    uint16_t spritePixel;
    bool BG_Priority;
    void getSpritePixel();

    int N, M, OAM_Location, /*spriteCount,*/ spriteFetchCycle;
    //uint8_t OAM_Buffer;
    void spriteEval();
    void spriteOverflowEval(int N);
    void spriteFetch();
    void sprite0Hit();

    //Background
    uint16_t BG_Pixel;
    void getBackgroundPixel();

    int backgroundFetchCycle;
    uint8_t NT_Byte;
    uint8_t AT_Byte;
    uint8_t PT_High;
    uint8_t PT_Low;
    uint16_t PT_Address;
    uint16_t PT_Shifter_High;
    uint16_t PT_Shifter_Low;
    uint8_t AT_Shifter_High;
    uint8_t AT_Shifter_Low;
    bool AT_Latch_High;
    bool AT_Latch_Low;
    void backgroundFetch();
    void shiftRegisters();
    void loadShiftRegisters();
    void setAttributeLatches();

    //Rendering
    int frameBufferPointer;
    uint8_t pixelMultiplexer();
    void renderPixel();
};

#endif