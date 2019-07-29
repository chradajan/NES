#include "include/PPU.hpp"

uint8_t PPU_Registers::read(uint16_t address)
{
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
            addressLatch = 0x0000;
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
}

void PPU_Registers::write(uint16_t address, uint8_t data)
{
    switch(address)
    {
        case 0x2000: //PPUCTRL
            if(PPUSTATUS >> 7 && data >> 7) //TODO: check if this is how NMI is generated
                nmi = true;
            ppu.reg.t = (ppu.reg.t & 0x73FF) | ((data & 0x03) << 10);
            PPUCTRL = data;
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
                addressLatch = data; //Not sure if address latch is really needed
            }
            else //Second write
            {
                ppu.reg.t = (ppu.reg.t & 0x0C1F) | ((data & 0x07) << 12) | ((data & 0xF8) << 2);
                ppu.reg.w = false;
                addressLatch = (addressLatch << 8) | data; //Not sure if address latch is really needed
            }
            break;
        case 0x2006: //PPUADDR
            if(!ppu.reg.w) //First write
            {
                ppu.reg.t = (ppu.reg.t & 0x00FF) | ((data & 0x3F) << 8);
                ppu.reg.w = true;
                addressLatch = data; //Not sure if address latch is really needed
            }
            else
            {
                ppu.reg.t = (ppu.reg.t & 0x7F00) | data;
                ppu.reg.v = ppu.reg.t;
                ppu.reg.w = false;
                addressLatch = (addressLatch << 8) | data; //Not sure if address latch is really needed
            }
            break;
        case 0x2007: //PPUDATA
            ppu.VRAM[ppu.reg.v] = data;
            if((PPUCTRL >> 2) & 0x01)
                ppu.reg.v += 0x20;
            else
                ++ppu.reg.v;
            break; 
    }
}

bool PPU_Registers::renderingEnabled()
{
    return (PPUCTRL >> 3) & 0x03;
}

//PPU

PPU::PPU(Cartridge* cartridge) : cart(cartridge)
{
    reg.clear();
}

void PPU::tick()
{
    if(scanline == -1)
        preRenderScanline();
    else if(scanline < 240)
        visibleScanline();
    else if(scanline == 241 && dot == 1)
        memMappedReg->PPUSTATUS |= 0x80; //Set VBlank flag
    //Else this is an idle cycle   
}

PPU_Registers& PPU::getRegisters()
{
    return *memMappedReg;
}

PPU::~PPU() {}

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
    uint8_t baseNameTableAddress = (reg.v >> 10) & 0x03;
    switch(baseNameTableAddress)
    {
        case 0x00:
            return 0x2000 | (reg.v & 0x0FFF);
        case 0x01:
            return 0x2400 | (reg.v & 0x0FFF);
        case 0x10:
              return 0x2800 | (reg.v & 0x0FFF);
        case 0x11:
            return 0x2C00 | (reg.v & 0x0FFF);
    }
}

uint16_t PPU::attributeAddress()
{
    uint8_t baseNameTableAddress = (reg.v >> 10) & 0x03;
    switch(baseNameTableAddress)
    {
        case 0x00:
            return 0x23C0 | (reg.v & 0x0C00) | ((reg.v >> 4) & 0x38) | ((reg.v >> 2) & 0x07);
        case 0x01:
            return 0x27C0 | (reg.v & 0x0C00) | ((reg.v >> 4) & 0x38) | ((reg.v >> 2) & 0x07);
        case 0x10:
              return 0x2BC0 | (reg.v & 0x0C00) | ((reg.v >> 4) & 0x38) | ((reg.v >> 2) & 0x07);
        case 0x11:
            return 0x2FC0 | (reg.v & 0x0C00) | ((reg.v >> 4) & 0x38) | ((reg.v >> 2) & 0x07);
    }
}

void PPU::setAttributeBits()
{
    NT_Addr -= 0x2000;
    NT_Addr %= 0x400;
    bool right = ((NT_Addr % 32) % 4) / 2; //fakse if left half, true if right half
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
    if(dot == 0)
        return;
    else if(dot == 1)
        memMappedReg->PPUSTATUS &= 0x3F; //Clear Vblank flag and sprite 0 hit
    else if(dot < 257)
    {
        backGroundFetchCycleEval();
        spriteEval();
        getPixel();
    }
}

void PPU::visibleScanline()
{

}

void PPU::backGroundFetchCycleEval()
{
    ++fetchCycle;
    switch(fetchCycle)
    {
        case 1:
            break;
        case 2:
            NT_Addr = tileAddress();
            NT_Byte = read(NT_Addr);
            break;
        case 3:
            break;
        case 4:
            AT_Byte = read(attributeAddress());
            break;
        case 5:
            break;
        case 6:
        {
            uint16_t PT_Address = (memMappedReg->PPUCTRL & 0x10) << 8;
            PT_Address += (NT_Byte * 0x10) + PT_offset;
            BG_LowByte = read(PT_Address);
        }
            break;
        case 7:
            break;
        case 8:
        {
            uint16_t PT_Address = (memMappedReg->PPUCTRL & 0x10) << 8;
            PT_Address += (NT_Byte * 0x10) + PT_offset + 8;
            BG_HighByte = read(PT_Address);

            PT_Shift_Low |= BG_LowByte;      //Reload lower 8 bits of shift register
            PT_Shift_High |= BG_HighByte;    //Reload lower 8 bits of shift register
            setAttributeBits();

            incHoriV();
            fetchCycle = 0;        
        }
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