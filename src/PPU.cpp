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
    reg.PPUCTRL = reg.PPUMASK = reg.PPUSTATUS = reg.OAMADDR = reg.PPUDATA_Buffer = reg.x = 0x00;
    reg.v = reg.t = 0x0000;
    reg.w = false;
    scanline = -1;
    dot = 0;
    oddFrame = false;
    AT_Latch_Low = AT_Latch_High = false;
    AT_Shifter_Low = AT_Shifter_High = PT_Temp_Low = PT_Temp_High = 0x00;
    PT_Addr = PT_Shifter_Low = PT_Shifter_High = 0x0000;
    frameBufferPointer = 0;
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
                reg.v += (((reg.PPUCTRL >> 2) & 0x01) ? 0x20 : 0x01);
                return temp;
            }
            else
            {
                reg.PPUDATA_Buffer = read(reg.v - 0x1000);
                reg.v += (((reg.PPUCTRL >> 2) & 0x01) ? 0x20 : 0x01);
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
                reg.t = (reg.t & 0x7F00) | data;
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
    address %= 0x4000;
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
    if(!renderingEnabled())
    {
        if(dot == 1)
            reg.PPUSTATUS &= 0x3F; //Clear VBlank and Sprite 0 flags
        return;
    }

    if(dot == 0)
        return;
    else if(dot == 1)
    {
        reg.PPUSTATUS &= 0x3F; //Clear VBlank and Sprite 0 flags
        backgroundFetch();
    }
    else if(dot < 257)
        backgroundFetch();
    else if(dot == 257)
    {
        setHoriV();
        //TODO: implement sprite fetch
    }
    else if(dot < 280)
        //TODO: implement sprite fetch
        ;
    else if(dot < 305)
    {
        //TODO: implement sprite fetch
        setVertV();
    }
    else if(dot < 321)
        //TODO: implement sprite fetch
        ;
    else if(dot < 337)
        backgroundFetch();
    else
        read(0x2000 | (reg.v & 0x0FFF)); //Dummy NT reads
}

void PPU::visibleScanline()
{
    if(!renderingEnabled())
        return;
    
    if(dot == 0)
    {
        if(!oddFrame)
            read(0x2000 | (reg.v & 0x0FFF));
        return;
    }
    else if(dot < 257)
    {
        getPixel();
        //spriteEval();
        backgroundFetch();
    }
    else if(dot == 257)
    {
        //spriteFetch();
        setHoriV();
    }
    else if(dot < 321)
        //spriteFetch();
        ;
    else if(dot < 337)
        backgroundFetch();
    else
        read(0x2000 | (reg.v & 0x0FFF));
}

void PPU::incDot()
{
    if(scanline == -1 && dot == 339 && oddFrame)
        scanline = dot = 0;
    else if(dot == 340)
    {
        dot = 0;
        ++scanline;
        if(scanline == 261)
        {
            scanline = 0;
            oddFrame = !oddFrame;
        }
    }
    else
        ++dot;    
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

void PPU::shiftRegisters()
{
    PT_Shifter_Low <<= 1;
    PT_Shifter_High <<= 1;
    AT_Shifter_Low <<= 1;
    AT_Shifter_High <<= 1;
    AT_Shifter_Low += (AT_Latch_Low ? 1 : 0);
    AT_Shifter_High += (AT_Latch_High ? 1 : 0);
}

void PPU::backgroundFetch()
{
    shiftRegisters();
    int cycle = (dot - 1) % 8;
    switch(cycle)
    {
        case 1:
            NT_Byte = read(0x2000 | (reg.v & 0x0FFF));
            break;
        case 3:
            AT_Byte = read(0x23C0 | (reg.v & 0x0C00) | ((reg.v >> 4) & 0x38) | ((reg.v >> 2) & 0x07));
            break;
        case 5:
            PT_Addr = 0x0000 | ((reg.PPUCTRL & 0x10) << 8) | (NT_Byte << 4) | ((reg.v & 0x7000) >> 12);
            PT_Temp_Low = read(PT_Addr);
            break;
        case 7:
            PT_Addr |= 0x0008;
            PT_Temp_High = read(PT_Addr);
            setAttributeLatch();
            PT_Shifter_Low |= PT_Temp_Low;
            PT_Shifter_High |= PT_Temp_High;
            incHoriV();
            if(dot == 257)
                incVertV();
            break;
        default:
            break;
    }
}

void PPU::setAttributeLatch()
{
    bool horizontalQuad = reg.v & 0x02; //false is left, true is right
    bool verticalQuad = reg.v & 0x40;   //false is top, true is bottom
    if(!horizontalQuad && !verticalQuad) //Top left
    {
        AT_Latch_Low = AT_Byte & 0x01;
        AT_Latch_High = AT_Byte & 0x02;
    }
    else if(horizontalQuad && !verticalQuad) //Top right
    {
        AT_Latch_Low = AT_Byte & 0x04;
        AT_Latch_High = AT_Byte & 0x08;
    }
    else if(!horizontalQuad && verticalQuad) //Bottom left
    {
        AT_Latch_Low = AT_Byte & 0x10;
        AT_Latch_High = AT_Byte & 0x20;
    }
    else //Bottom right
    {
        AT_Latch_Low = AT_Byte & 0x40;
        AT_Latch_High = AT_Byte & 0x80;
    }   
}

void PPU::getPixel()
{
    uint16_t BG_Pixel_Addr = 0x3F10;
    uint8_t selector = 0x80 >> reg.x;
    BG_Pixel_Addr += ((AT_Shifter_High & selector) ? 8 : 0);
    BG_Pixel_Addr += ((AT_Shifter_Low & selector) ? 4 : 0);
    BG_Pixel_Addr += ((PT_Shifter_High & selector) ? 2 : 0);
    BG_Pixel_Addr += ((PT_Shifter_Low & selector) ? 1 : 0);
    uint8_t BG_Pixel = read(BG_Pixel_Addr);
    renderPixel(BG_Pixel);
}

void PPU::renderPixel(uint8_t pixel)
{
    RGB color = colors[pixel];
    frameBuffer[frameBufferPointer++] = color.R;
    frameBuffer[frameBufferPointer++] = color.G;
    frameBuffer[frameBufferPointer++] = color.B;

    if(frameBufferPointer >= 184320)
    {
        frameBufferPointer = 0;
        frameReady = true;
    }
}

PPU::~PPU()
{

}