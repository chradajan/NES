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
    scanline = -1;
    dot = 0;
}

void PPU::tick()
{
    if(scanline == -1)
        prerenderScanline();
    else if(scanline < 240)
        visibleScanline();
    else if(scanline == 241 && dot == 1)
        reg.PPUSTATUS |= 0x80; //Set VBlank flag

    incDot();
}

uint8_t PPU::readMemMappedReg(uint16_t address)
{
    address = (address % 0x08) + 0x2000;
    switch(address)
    {
        case 0x2002: //PPUSTATUS
        {
            reg.w = false;
            uint8_t temp = reg.PPUSTATUS;
            reg.PPUSTATUS = (reg.PPUSTATUS & 0x7F);
            return temp;
        }
        case 0x2004: //OAMDATA
            return OAM[reg.OAMADDR];
        case 0x2007: //PPUDATA
        {
            if(reg.v <= 0x3EFF)
            {
                uint8_t temp = reg.PPUDATA_Buffer;
                reg.PPUDATA_Buffer = read(reg.v);
                return temp;
            }
            else
            {
                reg.PPUDATA_Buffer = read(reg.v - 0x1000);
                return read(reg.v);
            }
        }
    }
    return 0x00; //Shouldn't happen
}

void PPU::writeMemMappedReg(uint16_t address, uint8_t data)
{
    address = (address % 0x08) + 0x2000;
    switch(address)
    {
        case 0x2000: //PPUCTRL
            reg.PPUCTRL = data;
            checkNMI();
            reg.t = (reg.t & 0x73FF) | ((data & 0x03) << 10);
            break;
        case 0x2001: //PPUMASK
            reg.PPUMASK = data;
            break;
        case 0x2002: //PPUSTATUS
            reg.PPUSTATUS = data;
            break;
        case 0x2003: //OAMADDR
            reg.OAMADDR = data;
            break;
        case 0x2004: //OAMDATA
            OAM[reg.OAMADDR] = data;
            ++reg.OAMADDR;
            break;
        case 0x2005: //PPUSCROLL
            if(!reg.w) //First write
            {
                reg.t = (reg.t & 0x7FE0) | ((data & 0xF8) >> 3);
                reg.x = data & 0x07;
                reg.w = true;
            }
            else //Second write
            {
                reg.t = (reg.t & 0x0C1F) | ((data & 0x07) << 12) | ((data & 0xF8) << 2);
                reg.w = false;
            }            
            break;
        case 0x2006: //PPUADDR
            if(!reg.w) //First write
            {
                reg.t = (reg.t & 0x00FF) | ((data & 0x3F) << 8);
                reg.w = true;
            }
            else
            {
                reg.t = (reg.t & 0x7F) | data;
                reg.v = reg.t;
                reg.w = false;
            }            
            break;
        case 0x2007: //PPUDATA
            write(reg.v, data);
            reg.v += (((reg.PPUCTRL >> 2) & 0x01) ? 0x20 : 0x01);
            break;
    }
}

uint8_t PPU::read(uint16_t address)
{
    address %- 0x4000;
    if(address < 0x2000)
        return cart.readCHR(address);
    else if(address < 0x3F00)
        return VRAM[mirrored_NT_Addr(address)];
    else
        return paletteRAM[mirrored_Palette_Addr(address)];
}

void PPU::write(uint16_t address, uint8_t data)
{
    address %= 0x4000;
    if(address < 0x2000)
        cart.writeCHR(address, data);
    else if(address < 0x3F00)
        VRAM[mirrored_NT_Addr(address)] = data;
    else
        paletteRAM[mirrored_Palette_Addr(address)] = data;
}

uint16_t PPU::mirrored_NT_Addr(uint16_t address)
{
    if(address > 0x2FFF)
        address -= 0x1000;

     if(cart.verticalMirroring())
			address = (address - 0x2000) - (address / 0x2800 * 0x800);
    else
        address = (address - 0x2000) - (address / 0x2400 * 0x400) - (address / 0x2C00 * 0x400);

    return address;
}

uint16_t PPU::mirrored_Palette_Addr(uint16_t address)
{
    address %= 0x20;
    switch(address)
    {
        case 0x04:
        case 0x08:
        case 0x0C:
            address = 0x00;
            break;
        case 0x10:
        case 0x14:
        case 0x18:
        case 0x1C:
            address -= 0x10;
            break;
        default:
            break;
    }
    return address;
}

bool PPU::NMI()
{
    return nmi;
}

void PPU::checkNMI()
{
    if(reg.PPUCTRL >> 7 && reg.PPUSTATUS >> 7)
        nmi = true;
}

bool PPU::renderingEnabled()
{
    return(((reg.PPUMASK >> 3) & 0x01) || ((reg.PPUMASK >> 4) & 0x01));
}

void PPU::prerenderScanline()
{

}

void PPU::visibleScanline()
{

}

void PPU::incDot()
{

}

void PPU::incHoriV()
{
    if((reg.v & 0x001F) == 31)
    {
        reg.v &= 0x7FE0;
        reg.v ^= 0x0400;
    }
    else
        ++reg.v;
}

void PPU::incVertV()
{
    if((reg.v & 0x7000) != 0x7000)
        reg.v += 0x1000;
    else
    {
        reg.v &= 0x8FFF;
        int y = (reg.v & 0x03E0) >> 5;
        if(y == 29)
        {
            y = 0;
            reg.v ^= 0x8000;
        }
        else if(y == 31)
            y = 0;
        else
            ++y;
        reg.v = (reg.v & 0x7C1F) | (y << 5);
    }
}

void PPU::setHoriV()
{
    reg.v = (reg.v & 0x7BE0) | (reg.t & 0x041F);
}

void PPU::setVertV()
{
    reg.v = (reg.v & 0x041F) | (reg.t & 0x7BE0);
}

PPU::~PPU()
{

}