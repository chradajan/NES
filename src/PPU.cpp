#include "include/PPU.hpp"

PPU::PPU(Cartridge* cart, RGB* colors, char* frameBuffer, bool& frameReady)
: cart(*cart), colors(colors), frameBuffer(frameBuffer), frameReady(frameReady)
{
    for(int i = 0; i < 0x800; ++i)
        VRAM[i] = 0x00;
    for(int i = 0; i < 0x100; ++i)
        OAM[i] = 0x00;
    for(int i = 0; i < 0x20; ++i)
        OAM_Secondary[i] = 0x00;
    reg.PPUCTRL = 0x00;
    reg.PPUMASK = 0x00;
    reg.PPUSTATUS = 0x00;
    reg.OAMADDR = 0x00;
    reg.PPUDATA_Buffer = 0x00;
    reg.v = 0x0000;
    reg.t = 0x0000;
    reg.x = 0x00;
    reg.w = false;
}

void PPU::tick()
{

}

uint8_t PPU::readMemMappedReg(uint16_t address)
{
    address = (address % 0x08) + 0x2000;
    switch(address)
    {
        case 0x2000: //PPUCTRL
            break;
        case 0x2001: //PPUMASK
            break;
        case 0x2002: //PPUSTATUS
            break;
        case 0x2003: //OAMADDR
            break;
        case 0x2004: //OAMDATA
            break;
        case 0x2005: //PPUSCROLL
            break;
        case 0x2006: //PPUADDR
            break;
        case 0x2007: //PPUDATA
            break;
    }
}

void PPU::writeMemMappedReg(uint16_t address, uint8_t data)
{

}

uint8_t PPU::read(uint16_t address)
{

}

void PPU::write(uint16_t address, uint8_t data)
{

}

bool PPU::NMI()
{

}

PPU::~PPU()
{

}