#include "include/PPU.hpp"

PPU_Registers::PPU_Registers(PPU& ppu) : ppu(ppu)
{
    PPUCTRL = 0x00;
    PPUMASK = 0x00;
    PPUSTATUS = 0x00;
    OAMADDR = 0x00;
    OAMDATA = 0x00;
    dataBuffer = 0x00;
    nmi = false;
}

uint8_t PPU_Registers::read(uint16_t address)
{
    if(address > 0x2007)
        address = (address % 8) + 0x2000;
    switch(address)
    {
        case 0x2000: //PPUCTRL
            return PPUCTRL;
        case 0x2001: //PPUMASK
            return PPUMASK;
        case 0x2002: //PPUSTATUS
        {
            uint8_t temp = PPUSTATUS;
            PPUSTATUS &= 0x7F;
            ppu.reg.w = false;
            return temp;
        }
        case 0x2003: //OAMADDR
            return OAMADDR;
        case 0x2004: //OAMDATA
            return 0x00; //Not sure how to handle these reads
        case 0x2005: //PPUSCROLL
            return 0x00; //Not sure how to handle these reads
        case 0x2006: //PPUADDR
            return 0x00; //Not sure how to handle these reads
        case 0x2007: //PPUDATA
        {
            if(ppu.reg.v <= 0x3EFF)
            {
                uint8_t temp = dataBuffer;
                dataBuffer = ppu.VRAM[ppu.reg.v]; //TODO: fix this
                return temp;
            }
            else
            {
                dataBuffer = ppu.VRAM[ppu.reg.v - 0x1000];
                return ppu.VRAM[ppu.reg.v];
            }
        }
    }
    return 0x00; //Shouldn't happen
}

void PPU_Registers::write(uint16_t address, uint8_t data)
{
    if(address > 0x2007)
        address = (address % 8) + 0x2000;
    switch(address)
    {
        case 0x2000: //PPUCTRL
            ppu.reg.t = (ppu.reg.t & 0x73FF) | ((data & 0x03) << 10);
            PPUCTRL = data;
            checkNMI();
            break;
        case 0x2001: //PPUMASK
            PPUMASK = data;
            break;
        case 0x2002: // PPUSTATUS
            PPUSTATUS = data;
            break;
        case 0x2003: //OAMADDR
            OAMADDR = data;
            break;
        case 0x2004: //OAMDATA
            ppu.primaryOAM[OAMADDR] = data;
            ++OAMADDR;
            break;
        case 0x2005: //PPUSCROLL
            if(!ppu.reg.w) //First write
            {
                ppu.reg.t = (ppu.reg.t & 0x7FE0) | (data >> 3);
                ppu.reg.x = data & 0x07;
                ppu.reg.w = true;
            }
            else //Second write
            {
                ppu.reg.t = (ppu.reg.t & 0x0C1F) | ((data & 0x07) << 12) | ((data & 0xF8) << 2);
                ppu.reg.w = false;
            }
            break;
        case 0x2006: //PPUADDR
            if(!ppu.reg.w) //First write
            {
                ppu.reg.t = (ppu.reg.t & 0x00FF) | ((data & 0x3F) << 8);
                ppu.reg.w = true;
            }
            else
            {
                ppu.reg.t = (ppu.reg.t & 0x7F00) | data;
                ppu.reg.v = ppu.reg.t;
                ppu.reg.w = false;
            }
            break;
        case 0x2007: //PPUDATA
            ppu.write(ppu.reg.v, data);
            // std::cout << std::hex << (uint)ppu.reg.v << std::endl;
            if((PPUCTRL >> 2) & 0x01)
                ppu.reg.v += 0x20;
            else
                ++ppu.reg.v;
            break; 
    }
}

bool PPU_Registers::renderingEnabled()
{
    return (PPUMASK >> 3) & 0x03;
}

void PPU_Registers::checkNMI()
{
    if(PPUCTRL >> 7 && PPUSTATUS >> 7)
        nmi = true;
}

//PPU

PPU::PPU(Cartridge* cartridge, RGB* colors, char* frameBuffer, bool& renderFrame)
: cart(cartridge), colors(colors), frameBuffer(frameBuffer), renderFrame(renderFrame)
{
    reg.clear();
    memMappedReg = new PPU_Registers(*this);
    scanline = -1;
    dot = 0;
    fetchCycle = 0;
    PT_offset = 0;
    oddFrame = false;
    frameBufferPointer = 0;
    for(int i = 0; i < 0x800; ++i)
        VRAM[i] = 0x00;
}

PPU::~PPU()
{
    delete memMappedReg;
}

void PPU::printPatternTables()
{
    for(int i = 0; i < 256; ++i)
    {
        for(int j = 0; j < 8; ++j)
        {
            uint8_t low = read(i*16 + j);
            uint8_t high = read(i*16 + j + 8);
            for(int k = 0; k < 8; ++k)
            {
                std::cout << ((low & (0x80 >> k)) >> (7-k)) + ((high & (0x80 >> k)) >> (7-k)) << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl << std::endl;
    }
}

void PPU::tick()
{
    if(scanline == -1)
        preRenderScanline();
    else if(scanline < 240)
        visibleScanline();
    else if(scanline == 241 && dot == 1)
    {
        memMappedReg->PPUSTATUS |= 0x80; //Set VBlank flag
        memMappedReg->checkNMI();
    }
    
    incDot();
}

PPU_Registers& PPU::getRegisters()
{
    return *memMappedReg;
}

uint8_t PPU::read(uint16_t address)
{
    address %= 0x3FFF;
    if(address < 0x2000) //Pattern table
        return cart->readCHR(address);
    else if(address < 0x3F00) //Nametables
		return VRAM[nametableAddress(address)];
    else //Palette RAM
        return paletteRAM[paletteAddress(address)];
}

void PPU::write(uint16_t address, uint8_t data)
{
    address %= 0x3FFF;
    if(address < 0x2000) //Pattern table
        cart->writeCHR(address, data);
    else if(address < 0x3F00) //Nametables
        VRAM[nametableAddress(address)] = data;
    else
        paletteRAM[paletteAddress(address)] = data;
}

uint16_t PPU::nametableAddress(uint16_t address)
{
    if(address > 0x2FFF)
        address -= 0x1000;

     if(cart->verticalMirroring())
			address = (address - 0x2000) - (address / 0x2800 * 0x800);
		else
			address = (address - 0x2000) - (address / 0x2400 * 0x400) - (address / 0x2C00 * 0x400);

    return address;
}

uint16_t PPU::paletteAddress(uint16_t address)
{
    address = 0x3F00 + (address % 20);

    if(address == 0x3F04 || address == 0x3F08 || address == 0x3F0C || address == 0x3F10)
        address = 0x3F00;
    else if(address == 0x3F14)
        address = 0x3F04;
    else if(address == 0x3F18)
        address = 0x3F08;
    else if(address == 0x3F1C)
        address = 0x3F0C;

    return address - 0x3F00;
}

uint16_t PPU::tileAddress()
{
    return 0x2000 | (reg.v & 0xFFF);
}

uint16_t PPU::attributeAddress()
{
    return 0x23C0 | (reg.v & 0x0C00) | ((reg.v >> 4) & 0x38) | ((reg.v >> 2) & 0x07);
}

void PPU::setAttributeBits()
{
    NT_Addr -= 0x2000;
    NT_Addr %= 0x400;
    bool right = ((NT_Addr % 32) % 4) / 2; //false if left half, true if right half
    bool bottom = ((NT_Addr / 32) % 4) / 2; //false if top half, true if bottom half
    
    if(!right && !bottom) //Top left
        AT_Bits = AT_Byte & 0x03;
    else if(right && !bottom) //Top right
        AT_Bits = (AT_Byte & 0x0C) >> 2;
    else if(!right && bottom) //Bottom left
        AT_Bits = (AT_Byte & 0x30) >> 4;
    else if(right && bottom) //Bottom right
        AT_Bits = (AT_Byte & 0xC0) >> 6;
}

void PPU::preRenderScanline()
{
    if(dot == 0 || !memMappedReg->renderingEnabled())
        return;
    else if(dot == 1)
    {
        memMappedReg->PPUSTATUS &= 0x3F; //Clear Vblank flag and sprite 0 hit
        backgroundFetchCycleEval();
    }
    else if(dot < 257)
        backgroundFetchCycleEval();
    else if(dot == 257)
        setHoriV();
    else if(dot < 280)
        return;
    else if(dot < 305)
        setVertV();
    else if(dot < 321)
        return;
    else if(dot < 337)
        backgroundFetchCycleEval();
    else
        read(tileAddress());
}

void PPU::visibleScanline()
{
    if(!memMappedReg->renderingEnabled())
        return;
    else if(dot == 0)
    {
        if(!oddFrame)
            read(tileAddress());
        else
            return;        
    }
    // else if(dot == 1)
    // {
    //     backgroundFetchCycleEval();
    //     //spriteEval();
    // }
    // else if(dot < 257)
    // {
    //     getPixel();
    //     backgroundFetchCycleEval();
    //     //spriteEval();
    // }
    else if(dot < 257)
    {
        getPixel();
        backgroundFetchCycleEval();
    }
    else if(dot == 257)
    {
        setHoriV();
        //spriteFetch();
    }
    else if(dot < 321)
        //spriteFetch();
        return;
    else if(dot < 337)
        backgroundFetchCycleEval();
    else
        read(tileAddress());    
}

void PPU::backgroundFetchCycleEval()
{
    if(!memMappedReg->renderingEnabled())
        return;

    PT_Shift_High <<= 1;
    PT_Shift_Low <<= 1;

    ++fetchCycle;
    switch(fetchCycle)
    {
        case 2:
            NT_Addr = tileAddress();
            NT_Byte = read(NT_Addr);
            break;
        case 4:
            AT_Byte = read(attributeAddress());
            break;
        case 6:
        {
            uint16_t PT_Address = ((memMappedReg->PPUCTRL & 0x10) << 8) | (NT_Byte << 4) | (reg.v >> 12);
            // uint16_t PT_Address = (memMappedReg->PPUCTRL & 0x10) << 8;
            // PT_Address += (NT_Byte * 0x10) + PT_offset;
            BG_LowByte = read(PT_Address);
        }
            break;
        case 8:
        {
            uint16_t PT_Address = ((memMappedReg->PPUCTRL & 0x10) << 8) | (NT_Byte << 4) | 0x08 | (reg.v >> 12);
            // uint16_t PT_Address = (memMappedReg->PPUCTRL & 0x10) << 8;
            // PT_Address += (NT_Byte * 0x10) + PT_offset + 8;
            BG_HighByte = read(PT_Address);

            PT_Shift_Low |= BG_LowByte;      //Reload lower 8 bits of shift register
            PT_Shift_High |= BG_HighByte;    //Reload lower 8 bits of shift register
            setAttributeBits();

            if(dot == 256)
                incVertV();
            else
                incHoriV();
            
            fetchCycle = 0;
            break;      
        }
        default:
            break;
    }
}

void PPU::incHoriV()
{
    if((reg.v & 0x001F) == 0x001F) //If coarse x == 31
    {
        reg.v &= 0x7FE0;            //coarse x = 0
        reg.v ^= 0x0400;            //switch horizontal nametable
    }
    else
        ++reg.v;                    //Increment coarse x
}

void PPU::incVertV()
{
    if((reg.v & 0x7000) != 0x7000)           //If fine y < 7
        reg.v += 0x1000;                     //increment fine y
    else
    {
        reg.v &= 0x0FFF;                     //Fine y = 0
        uint8_t y = (reg.v & 0x03E0) >> 5;   //y = coarse y
        if(y == 0x1D)                        //if y == 29
        {
            y = 0x00;                        //coarse y = 0
            reg.v ^= 0x0800;                 //switch vertical nametable
        }
        else if(y == 0x1F)                   //if y == 31
            y = 0x00;                        //coarse y = 0, nametable not switched
        else
            ++y;                             //increment coarse y

        reg.v = (reg.v & 0x7C1F) | (y << 5); //put coarse y back into v 
    }
}

void PPU::setHoriV()
{
    //Copy horizontal related bits from t to v
    reg.v = (reg.v & 0x7BE0) | (reg.t & 0x041F);
}

void PPU::setVertV()
{
    //Copy vertical related bits from t to v
    reg.v = (reg.v & 0x041F) | (reg.t & 0x7BE0);
}

void PPU::incDot()
{
    if(scanline == -1 && dot == 339 && oddFrame)
    {
        scanline = dot = 0;
        oddFrame = false;
    }
    else if(scanline == 260 && dot == 340)
    {
        dot = 0;
        scanline = -1;
        oddFrame = !oddFrame;
    }
    else if(dot == 340)
    {
        ++scanline;
        dot = 0;
    }
    else
        ++dot;    
}

void PPU::spriteEval()
{
	if(dot == 0)
		return;
    else if(dot < 33)
        secondaryOAM[dot - 1] = 0xFF;
    else if(dot < 65)
        return;
	else if(dot == 65)
	{
		N = read(0x2003); //OAMADDR
		M = 0;
        secondaryOAM_Pointer = 0;
        found8Sprites = false;
		OAM_Data = primaryOAM[N + M];
	}
	else if(dot < 257 && dot % 2 == 1)
		OAM_Data = primaryOAM[N + M];
	else if(dot < 257 && dot % 2 == 0)
		spriteEvalWrite();
	else if(dot < 321)
		spriteFetch();
}

void PPU::spriteEvalWrite()
{
	if(N >= 64)
		return;
	else if(!found8Sprites && M == 0)
	{
		secondaryOAM[secondaryOAM_Pointer] = OAM_Data;
		if(OAM_Data == scanline)
		{
			++secondaryOAM_Pointer;
			++M;
		}
		else
			N += 4;
	}
	else if(!found8Sprites && M > 0)
	{
		secondaryOAM[secondaryOAM_Pointer++] = OAM_Data;
		found8Sprites = (secondaryOAM_Pointer == 32);
		if(M == 3)
		{
			M = 0;
			N += 4;
		}
		else
			++M;
	}
	else if(found8Sprites)
		spriteOverflowEval();
}

void PPU::spriteOverflowEval()
{
	if((memMappedReg->PPUSTATUS & 0x40) || N >= 64) //Check sprite overflow flag or if all sprites have been evaluated
		return;
	else if(primaryOAM[N + M] == scanline)
		memMappedReg->PPUSTATUS |= 0x40; //Set sprite overflow flag
	else
	{
		N += 4;
		M = (M == 3 ? 0 : M + 1);
	}
}

void PPU::spriteFetch()
{

}

void PPU::getPixel()
{
    uint8_t BG_Pixel = 0x00;//, selectMask;
    //selectMask = 0x8000 >> reg.x;
    //BG_Pixel = (AT_Bits << 2) + ((PT_Shift_High & selectMask) >> (14 - selectMask)) + ((PT_Shift_Low & selectMask) >> (15 - selectMask));
    BG_Pixel = (AT_Bits << 2) + ((PT_Shift_High & 0x8000) >> 14) + ((PT_Shift_Low & 0x7000) >> 15);
    uint16_t addr = (0x3F0 << 4) + BG_Pixel;
    uint8_t colorByte = read(addr);
    decodePixel(colorByte);
}

void PPU::decodePixel(uint8_t colorByte)
{
    RGB color = colors[colorByte];
    
    frameBuffer[frameBufferPointer++] = (char)color.R;
    frameBuffer[frameBufferPointer++] = (char)color.G;
    frameBuffer[frameBufferPointer++] = (char)color.B;
    if(frameBufferPointer >= 184320)
    {
        renderFrame = true;
        frameBufferPointer = 0;
    }
}