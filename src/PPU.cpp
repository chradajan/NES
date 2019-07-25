#include "include/PPU.hpp"

PPU_Registers::PPU_Registers(PPU& ppu) : ppu(ppu)
{
	addressLatch = 0x0000;
	PPUDATA_Buffer = 0x00;
}

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
			addressLatch = 0x0000;
			PPUSTATUS &= 0x7F;
			return temp;
		case 0x2003:
			return OAMADDR;
		case 0x2004:
			//TODO: implement this
			//reads during vertical or forced blanking return the value from OAM at that address but do not increment
			return OAMDATA;
		case 0x2005:
			return PPUSTROLL;
		case 0x2006:
			return PPUADDR;
		case 0x2007:
			if(getAddressLatch() <= 0x3EFF)
			{
				temp = PPUDATA_Buffer;
				PPUDATA_Buffer = ppu.VRAM[getAddressLatch()];
			}
			else
			{
				temp = ppu.VRAM[getAddressLatch()];
				PPUDATA_Buffer = ppu.VRAM[getAddressLatch() - 0x1000]; //Not sure if this is actually correct yet
			}
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
			ppu.primaryOAM[OAMADDR] = OAMDATA = data;
			++OAMADDR;
			break;
		case 0x2005:
			addressLatch = (addressLatch << 8) + data;
			PPUSTROLL = data;
			break;
		case 0x2006:
			addressLatch = (addressLatch << 8) + data;
			PPUADDR = data;
			break;
		case 0x2007:
			ppu.VRAM[getAddressLatch()] = data;
			incremenetPPUADDR();
			break;
	}
}

uint16_t PPU_Registers::getAddressLatch()
{
	return addressLatch % 0x3FFF;
}

void PPU_Registers::incremenetPPUADDR()
{
	if(PPUCTRL >> 2 & 0x01)
		PPUADDR += 0x20;
	else
		++PPUADDR;
}

//PPU

PPU::PPU(Cartridge* cart)
: cart(cart)
{
	ppu_registers = new PPU_Registers(*this);

	//TODO: implement power up state stuff

	scanline = -1;
	dot = 0;
	oddScanline = true;
}

void PPU::tick()
{
	// ++ppu_registers->cycle;
	// if(ppu_registers->cycle == 341)
	// {
	// 	++ppu_registers->scanline;
	// 	ppu_registers->cycle = 0;
	// }
	if(scanline == -1)
		preRenderScanline();
	else if(scanline <= 239)
		visibleScanline();
	else if(scanline == 241 && dot == 1)
		setVBlankFlag(1);

	incrementDot();
}

PPU_Registers& PPU::getRegisters()
{
	return *ppu_registers;
}

PPU::~PPU()
{
	delete ppu_registers;
}

void PPU::incrementDot()
{
	if(dot == 339 && oddScanline && (ifSpriteRendering() || ifBackgroundRendering()))
	{
		dot = 0;
		++scanline;
	}
	else if(dot == 340)
	{
		dot = 0;
		scanline = (scanline == 260 ? -1 : scanline + 1);
	}
	else
		++dot;

	oddScanline = !oddScanline;
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
		ppu_registers->PPUSTATUS |= 0x01 << 5;
	else
		ppu_registers->PPUSTATUS &= ~(0x01 << 5);
}

void PPU::setVBlankFlag(bool condition)
{
	if(condition)
		ppu_registers->PPUSTATUS |= 0x80;
	else
		ppu_registers->PPUSTATUS &= 0x7F;
}

bool PPU::ifSpriteOverflow()
{
	return (ppu_registers->PPUSTATUS >> 5) & 0x01;
}

bool PPU::ifBackgroundRendering()
{
	return (ppu_registers->PPUMASK >> 3) & 0x01;
}

bool PPU::ifSpriteRendering()
{
	return (ppu_registers->PPUMASK >> 4) & 0x01;
}

void PPU::preRenderScanline()
{

}

void PPU::visibleScanline()
{

}

void PPU::vBlankScanline()
{

}

void PPU::evaluateSprites()
{
	if(dot <= 64)
		clearSecondaryOAMByte();
	else if(dot == 65)
	{
		N = read(0x2003); //OAMADDR
		M = 0;
		spriteEvalRead();
	}
	else if(dot <= 256 && dot % 2 == 1)
		spriteEvalRead();
	else if(dot <= 256 && dot % 2 == 0)
		spriteEvalWrite();
	else if(dot <= 320)
		spriteFetch();
}

void PPU::clearSecondaryOAMByte()
{
	secondaryOAM[dot % 32] = 0xFF;
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
		if(oam_buffer - 1 == scanline)
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
	else if(primaryOAM[N + M] - 1 == scanline)
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