#include "include/PPU.hpp"

PPU::PPU(Cartridge* cartridge, RGB* color, char* fb, bool& frameReady, int& frameCounter)
: cart(*cartridge), colors(color), frameBuffer(fb) , frameReady(frameReady), frameCounter(frameCounter)
{
    for(int i = 0; i < 0x800; ++i)
        VRAM[i] = 0x00;
    for(int i = 0; i < 0x20; ++i)
        paletteRAM[i] = 0x00;
}

PPU::~PPU()
{
    
}

uint8_t PPU::readMemMappedReg(uint16_t address)
{
    if(address > 0x2007)
        address = 0x2000 + (address % 0x08);
    uint8_t temp = 0x00;
    switch(address)
    {
        case 0x2002: //PPUSTATUS
            reg.w = false;
            temp = reg.PPUSTATUS;
            reg.PPUSTATUS &= 0x7F;
            break;
        case 0x2004: //OAMDATA
            temp = OAM[reg.OAMADDR];
            if(!vblank && renderingEnabled())
                ++reg.OAMADDR;
            break;
        case 0x2007: //PPUDATA
            if(reg.v <= 0x3EFF)
            {
                temp = reg.ReadBuffer;
                reg.ReadBuffer = read(reg.v & 0x3FFF);
            }
            else
            {
                temp = read(reg.v);
                reg.ReadBuffer = read(reg.v - 0x1000);
            }
            reg.v += (reg.PPUCTRL & 0x04 ? 0x20 : 0x01);
            break;
        default:
            break;
    }
    return temp;
}

void PPU::writeMemMappedReg(uint16_t address, uint8_t data)
{
    if(address > 0x2007)
        address = 0x2000 + (address % 0x08);
    switch(address)
    {
        case 0x2000: //PPUCTRL
            reg.PPUCTRL = data;
            reg.t = (reg.t & 0x73FF) | ((data & 0x03) << 10);
            setNMI();
            break;
        case 0x2001: //PPUMASk
            reg.PPUMASK = data;
            break;
        case 0x2003: //OAMADDR
            reg.OAMADDR = data;
            break;
        case 0x2004: //OAMDATA
            OAM[reg.OAMADDR++] = data;
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
            else //Second write
            {
                reg.t = (reg.t & 0x7F00) | data;
                reg.v = reg.t;
                reg.w = false;
            }
            break;
        case 0x2007: //PPUDATA
            write((reg.v & 0x3FFF), data);
            reg.v += (reg.PPUCTRL & 0x04 ? 0x20 : 0x01);
            break;
        default:
            break;
    }
}

uint8_t PPU::read(uint16_t address)
{
    address %= 0x4000;
    if(address < 0x2000) //Pattern tables
        return cart.readCHR(address);
    else if(address < 0x3F00) //Nametables
        return VRAM[nametableAddress(address)];
    else //Palettes
        return paletteRAM[paletteAddress(address)];
}

void PPU::write(uint16_t address, uint8_t data)
{
    address %= 0x4000;
    if(address < 0x2000) //Pattern tables
        cart.writeCHR(address, data);
    else if(address < 0x3F00)
        VRAM[nametableAddress(address)] = data;
    else
        paletteRAM[paletteAddress(address)] = data;
}

bool PPU::NMI()
{
    if(nmi)
    {
        nmi = false;
        return true;
    }
    else
        return false;
}

void PPU::tick()
{
    if(scanline == -1)
        prerenderScanline();
    else if(scanline < 240)
        visibleScanline();
    else if(scanline == 241 && dot == 1)
    {
        reg.PPUSTATUS |= 0x80; //Set vblank
        setNMI();
    }

    incDot();
}

void PPU::prerenderScanline()
{
    if(dot == 0)
        return;

    if(dot == 1)
        reg.PPUSTATUS &= 0x3F; //Clear vlblank and sprite 0

    if(!renderingEnabled())
        return;

    if(dot == 257)
        setHoriV();
    else if(dot >= 280 && dot <= 304)
        setVertV();
    else if(dot >= 321 && dot <= 336)
        backgroundFetch();
}

void PPU::visibleScanline()
{
    if(dot == 0)
        return;

    if(!renderingEnabled())
    {
        if(dot < 257)
        {
            disabledRenderingDisplay();
            return;
        }
        else
            return;        
    }

    if(dot < 257)
    {
        getBackgroundPixel();
        getSpritePixel();
        renderPixel();
        backgroundFetch();
        spriteEval();
    }
    else if(dot == 257)
    {
        setHoriV();
        spriteFetchCycle = 0;
        OAM_Location = 0;
        for(int i = 0; i < 8; ++i)
            sprites[i] = OAM_Secondary[i];
        spriteFetch();
    }
    else if(dot < 321)
        spriteFetch();
    else if(dot >= 321 && dot <= 336)
        backgroundFetch();
}

bool PPU::renderingEnabled()
{
    return (reg.PPUMASK & 0x18);
}

void PPU::disabledRenderingDisplay()
{
    //Show universal background color or current color if vram is in palette space
}

void PPU::setNMI()
{
    nmi = (reg.PPUCTRL & 0x80) && (reg.PPUSTATUS & 0x80);
}

uint16_t PPU::nametableAddress(uint16_t address)
{
    address &= 0x3FFF;

    if(address > 0x2FFF)
        address -= 0x1000;

     if(cart.verticalMirroring())
			address = (address - 0x2000) - (address / 0x2800 * 0x800);
    else
        address = (address - 0x2000) - (address / 0x2400 * 0x400) - (address / 0x2C00 * 0x400);

    return address;
}

uint16_t PPU::paletteAddress(uint16_t address)
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
        reg.v &= 0x0FFF;
        int y = (reg.v & 0x03E0) >> 5;
        if(y == 29)
        {
            y = 0;
            reg.v ^= 0x0800;
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

void PPU::incDot()
{
    if(dot < 339)
        ++dot;
    else if(dot == 339 && scanline == -1 && oddFrame && renderingEnabled())
    {
        dot = 0;
        ++scanline;
    }
    else if(dot == 339)
        ++dot;
    else
    {
        dot = 0;
        ++scanline;
        if(scanline == 261)
        {
            scanline = -1;
            oddFrame = !oddFrame;
            ++frameCounter;
        }
    }    
}

void PPU::getSpritePixel()
{
    spritePixel = 0x3F10;
    BG_Priority = true;

    for(int i = 7; i >= 0; --i)
    {
        if(sprites[i].decrementX())
        {
            uint8_t lowSpriteNibble = sprites[i].getPixelNibble();
            if(lowSpriteNibble & 0x03)
            {
                spritePixel = 0x3F10 | lowSpriteNibble;
                BG_Priority = sprites[i].attributes & 0x20;
            }
        }
    }
}

void PPU::spriteEval()
{
    if(dot < 8)
        OAM_Secondary[dot].clear();
    else if(dot == 65)
    {
        N = reg.OAMADDR;
        M = 0;
        OAM_Location = 0;
        spriteCount = 0;
        OAM_Buffer = OAM[N + M];
    }
    else if(dot > 64 && dot < 257)
    {
        if(dot % 2 == 0)
            spriteEvalWrite();
        else
            OAM_Buffer = OAM[N + M];
    }
}

void PPU::spriteEvalWrite()
{
    if(N + M > 255)
        return;

    if(spriteCount < 8)
    {
        switch(M)
        {
            case 0:
            {
                OAM_Secondary[OAM_Location].Y = OAM_Buffer;
                int spriteOffset = scanline - OAM_Buffer;
                if(spriteOffset < ((reg.PPUCTRL & 0x20) ? 16 : 8))
                {
                    OAM_Secondary[OAM_Location].offset = spriteOffset;
                    ++M;
                }
                else
                    N += 4;  
                break;
            }
            case 1:
                OAM_Secondary[OAM_Location].tile = OAM_Buffer;
                ++M;
                break;
            case 2:
                OAM_Secondary[OAM_Location].attributes = OAM_Buffer;
                ++M;
                break;
            case 3:
                OAM_Secondary[OAM_Location].X = OAM_Buffer;
                ++spriteCount;
                ++OAM_Location;
                N += 4;
                M = 0;
                break;
        }
    }
    else
        spriteOverflowEval();    
}

void PPU::spriteOverflowEval()
{
    if(reg.PPUSTATUS & 0x20)
        return;
    else if(scanline - OAM[N + M] < ((reg.PPUCTRL & 0x20) ? 16 : 8))
        reg.PPUSTATUS |= 0x20;
    else
    {
        N += 4;
        M = (M == 3 ? 0 : M + 1);
    }
}

void PPU::spriteFetch()
{
    //TODO: implement 8x16 sprites
    ++spriteFetchCycle;

    if(spriteFetchCycle == 6)
    {
        int offset = sprites[OAM_Location].offset;
        if(sprites[OAM_Location].attributes & 0x80)
            offset = (offset - 7) * -1;
        PT_Address = 0x0000 | ((reg.PPUCTRL & 0x08) << 9) | (sprites[OAM_Location].tile << 4) | offset;
        sprites[OAM_Location].PT_Low = read(PT_Address);
    }
    else if(spriteFetchCycle == 8)
    {
        PT_Address |= 0x80;
        sprites[OAM_Location].PT_High = read(PT_Address);
        spriteFetchCycle = 0;
        ++OAM_Location;
    }
}

void PPU:: getBackgroundPixel()
{
    BG_Pixel = 0x3F00;
    uint8_t AT_Mask = 0x80 >> reg.x;
    uint16_t PT_Mask = 0x8000 >> reg.x;
    BG_Pixel |= ((AT_Shifter_High & AT_Mask) ? 0x0008 : 0x0000);
    BG_Pixel |= ((AT_Shifter_Low & AT_Mask) ? 0x0004 : 0x0000);
    BG_Pixel |= ((PT_Shifter_High & PT_Mask) ? 0x0002 : 0x0000);
    BG_Pixel |= ((PT_Shifter_Low & PT_Mask) ? 0x0001 : 0x0000);
}

void PPU::backgroundFetch()
{
    shiftRegisters();

    ++backgroundFetchCycle;
    switch(backgroundFetchCycle)
    {
        case 2:
            NT_Byte = read(0x2000 | (reg.v & 0x0FFF));
            break;
        case 4:
            AT_Byte = read(0x23C0 | (reg.v & 0x0C00) | ((reg.v >> 4) & 0x38) | ((reg.v >> 2) & 0x07));
            break;
        case 6:
            PT_Address = 0x0000 | ((reg.PPUCTRL & 0x10) << 8) | (NT_Byte << 4) | ((reg.v & 0x7000) >> 12);
            PT_Low = read(PT_Address);
            break;
        case 8:
            PT_Address |= 0x0008;
            PT_High = read(PT_Address);
            loadShiftRegisters();
            incHoriV();
            if(dot == 256)
                incVertV();
            backgroundFetchCycle = 0;
            break;
        default:
            break;
    }
}

void PPU::shiftRegisters()
{
    PT_Shifter_High <<= 1;
    PT_Shifter_Low <<= 1;
    AT_Shifter_High <<= 1;
    AT_Shifter_High |= (AT_Latch_High ? 0x01 : 0x00);
    AT_Shifter_Low <<= 1;
    AT_Shifter_Low |= (AT_Latch_Low ? 0x01 : 0x00);
}

void PPU::loadShiftRegisters()
{
    PT_Shifter_High |= PT_High;
    PT_Shifter_Low |= PT_Low;
    setAttributeLatches();
}

void PPU::setAttributeLatches()
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

void PPU::renderPixel()
{
    uint8_t colorIndex;

    if(!(spritePixel & 0x000F))
        colorIndex = read(BG_Pixel);
    else
    {
        if(BG_Priority)
            colorIndex = read(BG_Pixel);
        else
            colorIndex = read(spritePixel);        
    }

    RGB color = colors[colorIndex];
    frameBuffer[frameBufferPointer++] = color.R;
    frameBuffer[frameBufferPointer++] = color.G;
    frameBuffer[frameBufferPointer++] = color.B;

    if(frameBufferPointer >= 184320)
    {
        frameBufferPointer = 0;
        frameReady = true;
    }
}