#include "include/PPU.hpp"

PPU::PPU(Cartridge* cart, RGB* colors, char* frameBuffer, bool& frameReady, int& FC)
: cart(*cart), colors(colors), frameBuffer(frameBuffer), frameReady(frameReady), FC(FC)
{
    for(int i = 0; i < 0x800; ++i)
        VRAM[i] = 0x00;
    for(int i = 0; i < 0x100; ++i)
        OAM[i] = 0x00;
    reg.PPUCTRL = reg.PPUMASK = reg.PPUSTATUS = reg.OAMADDR = reg.PPUDATA_Buffer = reg.x = 0x00;
    reg.v = reg.t = 0x0000;
    reg.w = false;
    // scanline = -1;
    // dot = 0;
    nmi = false;
    scanline = 0;
    dot = 30;
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
    {
        reg.PPUSTATUS |= 0x80; //Set VBlank flag
        checkNMI();
    }

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
    {
        VRAM[mirrored_NT_Addr(address)] = data;
    }
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
    if(nmi)
    {
        nmi = false;
        return true;
    }
    else
        return false;    
}

void PPU::checkNMI()
{
    nmi = (reg.PPUCTRL >> 7 && reg.PPUSTATUS >> 7);
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
        spriteEval();
        backgroundFetch();
    }
    else if(dot == 257)
    {
        spriteFetch();
        setHoriV();
    }
    else if(dot < 321)
        spriteFetch();
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
        if(scanline == 260)
        {
            scanline = -1;
            oddFrame = !oddFrame;
            ++FC;
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
            if(FC > 5)
                std::cout << std::hex << (uint)AT_Byte << std::endl;
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

void PPU::spriteEval()
{
    if(dot < 8)
        OAM_Secondary[dot - 1].clear();
    else if(dot < 65)
        return;
    else if(dot == 65)
    {
        N = reg.OAMADDR;
        M = 0;
        secondaryLoc = 0;
        spriteCount = 0;
        OAM_Buffer = OAM[N + M];
    }
    else if(dot < 257 && dot % 2 == 1)
        OAM_Buffer = OAM[N + M];
    else if(dot < 257 && dot % 2 == 0)
        spriteEvalWrite();
    else if(dot == 257)
    {
        for(int i = 0; i < 8; ++i)
            OAM_Secondary_Current[i] = OAM_Secondary[i];
    }
    else if(dot < 321)
        spriteFetch();
}

void PPU::spriteEvalWrite()
{
    if(N + M >= 256)
        return;
    else if(spriteCount < 8)
    {
        switch(M)
        {
            case 0:
            {
                OAM_Secondary[secondaryLoc].Y = OAM_Buffer;
                int spriteOffset = scanline - OAM_Buffer;
                if(spriteOffset < (reg.PPUCTRL & 0x20 ? 16 : 8))
                {
                    OAM_Secondary[secondaryLoc].offset = spriteOffset;
                    ++M;
                }
                else
                    N += 4;
                break;
            }
            case 1:
                OAM_Secondary[secondaryLoc].Tile = OAM_Buffer;
                ++M;
                break;
            case 2:
                OAM_Secondary[secondaryLoc].Attributes = OAM_Buffer;
                ++M;
                break;
            case 3:
                OAM_Secondary[secondaryLoc].X = OAM_Buffer;
                ++spriteCount;
                ++secondaryLoc;
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
    if((reg.PPUSTATUS & 0x40)) //Check sprite overflow flag or if all sprites have been evaluated
		return;
	else if(OAM[N + M] == scanline)
		reg.PPUSTATUS |= 0x40; //Set sprite overflow flag
	else
	{
		N += 4;
		M = (M == 3 ? 0 : M + 1);
	}
}

void PPU::spriteFetch()
{
    int cycle = (dot - 1) % 8;
    int spriteLoc = (dot - 257) / 8;
    switch(cycle)
    {
        case 1:
            read(0x2000 | (reg.v & 0x0FFF)); //Dummy NT read
            break;
        case 3:
            read(0x23C0 | (reg.v & 0x0C00) | ((reg.v >> 4) & 0x38) | ((reg.v >> 2) & 0x07)); //Dummy AT read
            break;
        case 5:
            if(reg.PPUCTRL & 0x20) //8 x 16
            {

            }
            else //8 x 8
            {
                int offset = OAM_Secondary_Current[spriteLoc].offset;
                if(OAM_Secondary_Current[spriteLoc].Attributes & 0x80)
                    offset = (offset - 7) * -1;
                PT_Addr = 0x0000 | ((reg.PPUCTRL & 0x08) << 9) | (OAM_Secondary_Current[spriteLoc].Tile << 4) | offset;
                OAM_Secondary_Current[spriteLoc].PT_Low = read(PT_Addr);
            }           
            break;
        case 7:
            if(reg.PPUCTRL & 0x20) //8 x 16
            {
                
            }
            else
            {
                PT_Addr |= 0x80;
                OAM_Secondary_Current[spriteLoc].PT_High = read(PT_Addr);
            }            
    }
}

void PPU::getPixel()
{
    uint16_t BG_Pixel_Addr = getBackgroundPixelAddress();
    getSpritePixelAndRender(BG_Pixel_Addr);
}

uint16_t PPU::getBackgroundPixelAddress()
{
    uint16_t BG_Pixel_Addr = 0x3F00;
    uint8_t selector = 0x80 >> reg.x;
    BG_Pixel_Addr += ((AT_Shifter_High & selector) ? 8 : 0);
    BG_Pixel_Addr += ((AT_Shifter_Low & selector) ? 4 : 0);
    BG_Pixel_Addr += ((PT_Shifter_High & selector) ? 2 : 0);
    BG_Pixel_Addr += ((PT_Shifter_Low & selector) ? 1 : 0);
    return BG_Pixel_Addr;
}

void PPU::getSpritePixelAndRender(uint16_t BG_Pixel_Addr)
{
    bool spritePixelFound = false;
    uint16_t Sprite_Pixel_Addr;
    bool BG_Priority;

    for(int i = 0; i < 8; ++i)
    {
        --OAM_Secondary_Current[i].X;
        if(OAM_Secondary_Current[i].X <= 0)
        {
            if(spritePixelFound)
                OAM_Secondary_Current[i].getPixel(); //Shift registers but don't use pixel
            else
            {
                Sprite_Pixel_Addr = OAM_Secondary_Current[i].getPixel();
                spritePixelFound = true;
                BG_Priority = (OAM_Secondary_Current[i].Attributes & 0x20);
            }
        }
    }

    if(spritePixelFound && (Sprite_Pixel_Addr & 0x03)) //If there's a non-transparent sprite pixel
    {
        if(BG_Pixel_Addr & 0x03) //Non-transparent BG
        {
            if(BG_Priority)
                renderPixel(BG_Pixel_Addr);
            else
            {
                std::cout << (uint)Sprite_Pixel_Addr << std::endl;
                renderPixel(Sprite_Pixel_Addr);
            }
        }
        else
            renderPixel(Sprite_Pixel_Addr);    
    }
    else
        renderPixel(BG_Pixel_Addr);
}

void PPU::renderPixel(uint16_t pixelAddr)
{
    uint8_t pixel = read(pixelAddr);
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

void PPU::Sprite::clear()
{
    Y = Tile = Attributes = X = 0xFF;
    PT_Low = PT_High = 0x00;
    offset = 0;
}

uint16_t PPU::Sprite::getPixel()
{
    uint16_t addr = 0x3F10;
    addr |= (Attributes & 0x03) << 2;
    if(Attributes & 0x40) //Flipped horizontally
    {
        addr |= (PT_High & 0x01) << 1;
        addr |= (PT_Low & 0x01);
        PT_High >>= 1;
        PT_Low >>= 1;
    }
    else //Not flipped
    {
        addr |= (PT_High & 0x80) >> 6;
        addr |= (PT_Low & 0x80) >> 7;
        PT_High <<= 1;
        PT_Low <<= 1;
    }
    return addr;
}