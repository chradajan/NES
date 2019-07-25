#include "include/PPU.hpp"

PPU_Registers::PPU_Registers(PPU& ppu) : ppu(ppu) {}

bool PPU_Registers::NMI()
{
	return nmi;
}

uint8_t PPU_Registers::read(uint16_t address)
{
	address = (address < 0x2008 ? address : address - ((address / 0x0008) * 0x0008) + 0x2000);
	uint8_t temp;
	switch(address)
	{
		case 0x2000:
			return PPUCTRL;
		case 0x2001:
			return PPUMASK;
		case 0x2002:
			temp = PPUSTATUS;
			addressLatch = 0x00;
			PPUSTATUS &= 0x7F;
			return temp;
		case 0x2003:
			return OAMADDR;
		case 0x2004:
			return OAMDATA;
		case 0x2005:
			return PPUSTROLL;
		case 0x2006:
			return PPUADDR;
		case 0x2007:
			if(PPUADDR <= 0x3EFF)
			{
				temp = PPUDATA_Buffer;
				PPUDATA_Buffer = ppu.VRAM[PPUADDR];
			}
			else
				temp = ppu.VRAM[PPUADDR];
			incremenetPPUADDR();
			return temp;
	}
	//This shouldn't happen
	return 0x00;
}

void PPU_Registers::write(uint16_t address, uint8_t data)
{
	address = (address < 0x2008 ? address : address - ((address / 0x0008) * 0x0008) + 0x2000);
	switch(address)
	{
		case 0x2000:
			//TODO: implement NMI when bit 7 is set
			PPUCTRL = data;
			break;
		case 0x2001:
			PPUMASK = data;
			break;
		case 0x2002:
			PPUSTATUS = data;
			break;
		case 0x2003:
			OAMADDR = data;
			break;
		case 0x2004:
			OAMDATA = data;
			++OAMADDR;
			break;
		case 0x2005:
			PPUSTROLL = data;
			break;
		case 0x2006:
			PPUADDR = data;
			break;
		case 0x2007:
			PPUDATA = data;
			break;
	}
}

void PPU_Registers::incremenetPPUADDR()
{
	if(PPUCTRL >> 2 & 0x01)
		PPUADDR += 0x20;
	else
		++PPUADDR;
}

PPU::PPU(Cartridge* cart)
: cart(cart)
{
	ppu_registers = new PPU_Registers(*this);
}

void PPU::tick()
{
	++ppu_registers->cycle;
	if(ppu_registers->cycle == 341)
	{
		++ppu_registers->scanline;
		ppu_registers->cycle = 0;
	}
}

PPU_Registers& PPU::getRegisters()
{
	return *ppu_registers;
}

PPU::~PPU()
{
	delete ppu_registers;
}

uint8_t PPU::read(uint16_t address) const
{
	if(address < 0x2000) //Pattern Table
		return cart->readCHR(address);
	else if(address < 0x3000) //Nametable
	{
		if(cart->verticalMirroring())
			address = (address - 0x2000) - (address / 0x2800 * 0x800);
		else
			address = (address - 0x2000) - (address / 0x2400 * 0x400) - (address / 0x2C00 * 0x400);
		return VRAM[address];
	}
	else if(address < 0x3F00) //Nametable Mirroring
	{
		if(cart->verticalMirroring())
		{
			address -= 0x1000;
			address = (address - 0x2000) - (address / 0x2800 * 0x800);
		}
		else
		{
			address -= 0x1000;
			address = (address - 0x2000) - (address / 0x2400 * 0x400) - (address / 0x2C00 * 0x400);
		}
		return VRAM[address];
	}
	else if(address < 0x3F20) //Palette RAM
		return paletteRAM[address - 0x3F00];
	else //Palette Ram Mirroring
		return paletteRAM[address % 0x20];
}

void PPU::write(uint16_t address, uint8_t data)
{
	if(address < 0x2000) //Pattern Table
		cart->writeCHR(address, data);
	else if(address < 0x3000) //Nametable
	{
		if(cart->verticalMirroring())
			address = (address - 0x2000) - (address / 0x2800 * 0x800);
		else
			address = (address - 0x2000) - (address / 0x2400 * 0x400) - (address / 0x2C00 * 0x400);
		VRAM[address] = data;
	}
	else if(address < 0x3F00) //Nametable Mirroring
	{
		if(cart->verticalMirroring())
		{
			address -= 0x1000;
			address = (address - 0x2000) - (address / 0x2800 * 0x800);
		}
		else
		{
			address -= 0x1000;
			address = (address - 0x2000) - (address / 0x2400 * 0x400) - (address / 0x2C00 * 0x400);
		}
		VRAM[address] = data;
	}
	else if(address < 0x3F20) //Palette RAM
		paletteRAM[address - 0x3F00] = data;
	else //Palette Ram Mirroring
		paletteRAM[address % 0x20] = data;
}

void PPU::setSpriteOverflowFlag(bool condition)
{
	if(condition)
		ppu_registers->PPUSTATUS |= 0x1 << 5;
	else
		ppu_registers->PPUSTATUS &= ~(0x1 << 5);
}

bool PPU::ifSpriteOverflow()
{
	return (ppu_registers->PPUSTATUS >> 5) & 0x1;
}

void PPU::evaluateSprites()
{
	if(scanlineX <= 64)
		clearSecondaryOAMByte();
	else if(scanlineX == 65)
	{
		N = read(0x2003); //OAMADDR
		M = 0;
		spriteEvalRead();
	}
	else if(scanlineX <= 256 && scanlineX % 2 == 1)
		spriteEvalRead();
	else if(scanlineX <= 256 && scanlineX % 2 == 0)
		spriteEvalWrite();
	else if(scanlineX <= 320)
		spriteFetch();
}

void PPU::clearSecondaryOAMByte()
{
	secondaryOAM[scanlineX % 32] = 0xFF;
}

void PPU::spriteEvalRead()
{
	oam_buffer = primaryOAM[N + M];
}

void PPU::spriteEvalWrite()
{
	if(N >= 64)
		return;
	else if(!found8Sprites && M == 0)
	{
		secondaryOAM[secondary_oam_loc] = oam_buffer;
		if(oam_buffer - 1 == scanlineY)
		{
			++secondary_oam_loc;
			++M;
		}
		else
			N += 4;
	}
	else if(!found8Sprites && M > 0)
	{
		secondaryOAM[secondary_oam_loc++] = oam_buffer;
		found8Sprites = (secondary_oam_loc == 32);
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
	if(ifSpriteOverflow() || N >= 64)
		return;
	else if(primaryOAM[N + M] - 1 == scanlineY)
		setSpriteOverflowFlag(1);
	else
	{
		N += 4;
		M = (M == 3 ? 0 : M + 1);
	}
}

void PPU::spriteFetch()
{
	write(0x2003, 0x00); //OAMADDR
}