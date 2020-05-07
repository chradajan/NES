#include "../include/PPU.hpp"

PPU::PPU(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<char[]> frameBuffer, bool& frameReady)
: cart(cartridge), frameBuffer(frameBuffer), frameReady(frameReady)
{
    init();
    readInColors();
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

void PPU::init()
{
    VRAM.fill(0x00);
    paletteRAM.fill(0x00);
    
    reg.PPUCTRL = 0x00;
    reg.PPUMASK = 0x00;
    reg.PPUSTATUS = 0x00;
    reg.OAMADDR = 0x00;
    reg.ReadBuffer = 0x00;
    reg.v = 0x0000;
    reg.t = 0x0000;
    reg.x = 0x00;
    reg.w = false;

    vblank = true;
    nmi = false;
    scanline = 0;
    dot = 30;
    oddFrame = false;

    checkSprite0Hit = false;
    spritePixel = 0x0000;
    BG_Priority = true;
    BG_Pixel = 0x0000;

    backgroundFetchCycle = 0;
    NT_Byte = 0x00;
    AT_Byte = 0x00;
    PT_High = 0x00;
    PT_Low = 0x00;
    PT_Address = 0x0000;
    PT_Shifter_High = 0x0000;
    PT_Shifter_Low = 0x0000;
    AT_Shifter_High = 0x00;
    AT_Shifter_Low = 0x00;
    AT_Latch_High = false;
    AT_Latch_Low = false;

    frameBufferPointer = 0;
}

void PPU::readInColors()
{
    //Temporary solution to wrong palette colors
    std::array<uint8_t, 192> paletteArray = 
	{
		0x7C, 0x7C, 0x7C,
		0x00, 0x00, 0xFC,
		0x00, 0x00, 0xBC,
		0x44, 0x28, 0xBC,
		0x94, 0x00, 0x84,
		0xA8, 0x00, 0x20,
		0xA8, 0x10, 0x00,
		0x88, 0x14, 0x00,
		0x50, 0x30, 0x00,
		0x00, 0x78, 0x00,
		0x00, 0x68, 0x00,
		0x00, 0x58, 0x00,
		0x00, 0x40, 0x58,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0xBC, 0xBC, 0xBC,
		0x00, 0x78, 0xF8,
		0x00, 0x58, 0xF8,
		0x68, 0x44, 0xFC,
		0xD8, 0x00, 0xCC,
		0xE4, 0x00, 0x58,
		0xF8, 0x38, 0x00,
		0xE4, 0x5C, 0x10,
		0xAC, 0x7C, 0x00,
		0x00, 0xB8, 0x00,
		0x00, 0xA8, 0x00,
		0x00, 0xA8, 0x44,
		0x00, 0x88, 0x88,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0xF8, 0xF8, 0xF8,
		0x3C, 0xBC, 0xFC,
		0x68, 0x88, 0xFC,
		0x98, 0x78, 0xF8,
		0xF8, 0x78, 0xF8,
		0xF8, 0x58, 0x98,
		0xF8, 0x78, 0x58,
		0xFC, 0xA0, 0x44,
		0xF8, 0xB8, 0x00,
		0xB8, 0xF8, 0x18,
		0x58, 0xD8, 0x54,
		0x58, 0xF8, 0x98,
		0x00, 0xE8, 0xD8,
		0x78, 0x78, 0x78,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0xFC, 0xFC, 0xFC,
		0xA4, 0xE4, 0xFC,
		0xB8, 0xB8, 0xF8,
		0xD8, 0xB8, 0xF8,
		0xF8, 0xB8, 0xF8,
		0xF8, 0xA4, 0xC0,
		0xF0, 0xD0, 0xB0,
		0xFC, 0xE0, 0xA8,
		0xF8, 0xD8, 0x78,
		0xD8, 0xF8, 0x78,
		0xB8, 0xF8, 0xB8,
		0xB8, 0xF8, 0xD8,
		0x00, 0xFC, 0xFC,
		0xF8, 0xD8, 0xF8,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00
	};
    int j = 0;
    for(int i = 0; i < 64; ++i)
	{
		colors[i].R = paletteArray[j++];
		colors[i].G = paletteArray[j++];
		colors[i].B = paletteArray[j++];
	}
}

uint8_t PPU::read(uint16_t address)
{
    address %= 0x4000;
    if(address < 0x2000) //Pattern tables
        return cart->readCHR(address);
    else if(address < 0x3F00) //Nametables
        return VRAM[cart->nametableAddress(address)];
    else //Palettes
        return paletteRAM[paletteAddress(address)];
}

void PPU::write(uint16_t address, uint8_t data)
{
    address %= 0x4000;
    if(address < 0x2000) //Pattern tables
        cart->writeCHR(address, data);
    else if(address < 0x3F00)
        VRAM[cart->nametableAddress(address)] = data;
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
    }
    else if(dot == 257)
    {
        setHoriV();
        spriteEval();
        spriteFetchCycle = 0;
        OAM_Location = 0;     
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

uint16_t PPU::paletteAddress(uint16_t address)
{
    //TODO: figure out how palette addresses are mirrored
    address %= 0x20;
    if(address == 0x10)
        address = 0x00;
    // switch(address)
    // {
    //     case 0x04:
    //     case 0x08:
    //     case 0x0C:
    //         address = 0x00;
    //         break;
    //     case 0x10:
    //     case 0x14:
    //     case 0x18:
    //     case 0x1C:
    //         address -= 0x10;
    //         break;
    //     default:
    //         break;
    // }
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
        }
    }    
}

void PPU::getSpritePixel()
{
    if(scanline == 0)
        return;

    spritePixel = 0x3F10;
    BG_Priority = true;
  
    for(int i = 7; i >= 0; --i)
    {
        if(OAM_Secondary[i].decrementX())
        {
            uint8_t lowSpriteNibble = OAM_Secondary[i].getPixelNibble();
            if(lowSpriteNibble & 0x03)
            {
                checkSprite0Hit = OAM_Secondary[i].sprite0;
                spritePixel = 0x3F10 | lowSpriteNibble;
                BG_Priority = OAM_Secondary[i].attributes & 0x20;
            }
        }
    }
}

void PPU::spriteEval()
{
    for(int i = 0; i < 8; ++i)
        OAM_Secondary[i].clear();

    int N = 0;
    int spriteCount = 0;
    OAM_Location = 0;
    int offset;

    while(N < 256)
    {
        if(spriteCount == 8)
        {
            spriteOverflowEval(N);
            break;
        }
        else
        {
            OAM_Secondary[OAM_Location].Y = OAM[N];
            offset = scanline - OAM[N];
            if(offset >= 0 && offset < ((reg.PPUCTRL & 0x20) ? 16 : 8))
            {
                if(N == 0)
                    OAM_Secondary[OAM_Location].sprite0 = true;

                OAM_Secondary[OAM_Location].offset = offset;

                OAM_Secondary[OAM_Location].tile = OAM[N + 1];
                OAM_Secondary[OAM_Location].attributes = OAM[N + 2];
                OAM_Secondary[OAM_Location].X = OAM[N + 3];

                ++spriteCount;
                ++OAM_Location;
            }
        }     

        N +=4;
    }
}

void PPU::spriteOverflowEval(int N)
{
    int M = 0;
    int offset;

    while(N < 256)
    {
        offset = scanline - OAM[N + M];
        if(offset >= 0 && offset < ((reg.PPUCTRL & 0x20) ? 16 : 8))
        {
            reg.PPUSTATUS |= 0x20;
            break;
        }
        else
        {
            N += 4;
            if(M == 3)
                M = 0;
            else
                ++M;            
        }        
    }
}

void PPU::spriteFetch()
{
    ++spriteFetchCycle;

    if(spriteFetchCycle == 6)
    {
        int offset = OAM_Secondary[OAM_Location].offset;
        
        if((reg.PPUCTRL & 0x20)) //8x16
        {
            if(OAM_Secondary[OAM_Location].attributes & 0x80) //Flipped vertically
                offset = (offset - 15) * -1;

            if(offset < 8)
                PT_Address = 0x0000 | ((OAM_Secondary[OAM_Location].tile & 0x01) << 12) | ((OAM_Secondary[OAM_Location].tile & 0xFE) << 4) | offset;
            else
                PT_Address = 0x0000 | ((OAM_Secondary[OAM_Location].tile & 0x01) << 12) | ((OAM_Secondary[OAM_Location].tile & 0xFE) << 4) | 0x10 | offset;
        }
        else //8x8
        {
            if(OAM_Secondary[OAM_Location].attributes & 0x80) //Flipped vertically
                offset = (offset - 7) * -1;
            PT_Address = 0x0000 | ((reg.PPUCTRL & 0x08) << 9) | (OAM_Secondary[OAM_Location].tile << 4) | offset;
        }

        OAM_Secondary[OAM_Location].PT_Low = read(PT_Address);
    }
    else if(spriteFetchCycle == 8)
    {
        PT_Address |= 0x08;
        OAM_Secondary[OAM_Location].PT_High = read(PT_Address);
        spriteFetchCycle = 0;
        ++OAM_Location;
    }
}

void PPU::sprite0Hit()
{
    if(reg.PPUSTATUS & 0x40)
        return;
    
    //Conditions when sprite 0 doesn't occur
    if( ((reg.PPUMASK & 0x18) != 0x18) ||                    //If background or sprites are disabled
        ((dot <= 8) && ((reg.PPUMASK & 0x06) != 0x06)) ||    //If left-side clipping is enabled and dot <= 8
        (dot == 255) ||                                      //Dot == 255 due to pixel pipeline related wiring
        (!(BG_Pixel & 0x03)))                                //Background pixel is transparent (sprite can't be transparent but this is only checked when an opaque sprite 0 pixel is selected for rendering)                           
       return;
    else   
        reg.PPUSTATUS |= 0x40;
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

uint8_t PPU::pixelMultiplexer()
{
    uint16_t indexAddress;

    if(dot < 9 && ((reg.PPUMASK & 0x06) != 0x06))
    {
        if((reg.PPUMASK & 0x06) == 0x02) //Sprites hidden
            indexAddress = BG_Pixel;
        else if((reg.PPUMASK & 0x06) == 0x04) //Background hidden
            indexAddress = spritePixel;
        else //Both hidden, show background color
            indexAddress = 0x3F00;        
    } 
    else
    {    
        if(!(BG_Priority && (BG_Pixel & 0x03)))
            indexAddress = spritePixel;
        else
            indexAddress = BG_Pixel;

        if(!(reg.PPUMASK & 0x08)) //Background disabled
            indexAddress = spritePixel;
        else if(!(reg.PPUMASK & 0x10) || scanline == 0) //Sprites disabled
            indexAddress = BG_Pixel;

        if(checkSprite0Hit)
            sprite0Hit();
    }

    return read(indexAddress);
}

void PPU::renderPixel()
{
    uint8_t colorIndex = pixelMultiplexer();

    RGB color = colors[colorIndex];
    frameBuffer[frameBufferPointer++] = color.R;
    frameBuffer[frameBufferPointer++] = color.G;
    frameBuffer[frameBufferPointer++] = color.B;

    if(frameBufferPointer >= 184320)
    {
        frameReady = true;
        frameBufferPointer = 0;
    }
}