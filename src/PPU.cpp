#include "include/PPU.hpp"

PPU::PPU(Mapper* map, PPU_Registers& ppu_reg)
: mapper(map), ppu_registers(ppu_reg)
{

}

void PPU::tick()
{

}

PPU::~PPU()
{

}

uint8_t PPU::read(uint16_t address) const
{
	if(address < 0x2000) //Pattern Table
		return mapper->readCHR(address);
	else if(address < 0x3000) //Nametable
	{
		if(mapper->verticalMirroring())
		{
			address = (address - 0x2000) - (address / 0x2800 * 0x800);
		}
		else
		{
			address = (address - 0x2000) - (address / 0x2400 * 0x400) - (address / 0x2C00 * 0x400);
		}
		return VRAM[address];
	}
	else if(address < 0x3F00) //Nametable Mirroring
	{
		if(mapper->verticalMirroring())
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
		mapper->writeCHR(address, data);
	else if(address < 0x3000) //Nametable
	{
		if(mapper->verticalMirroring())
		{
			address = (address - 0x2000) - (address / 0x2800 * 0x800);
		}
		else
		{
			address = (address - 0x2000) - (address / 0x2400 * 0x400) - (address / 0x2C00 * 0x400);
		}
		VRAM[address] = data;
	}
	else if(address < 0x3F00) //Nametable Mirroring
	{
		if(mapper->verticalMirroring())
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
		ppu_registers.PPUSTATUS |= 0x1 << 5;
	else
		ppu_registers.PPUSTATUS &= ~(0x1 << 5);
}

bool PPU::ifSpriteOverflow()
{
	return (ppu_registers.PPUSTATUS >> 5) & 0x1;
}

void PPU::evaluateSprites()
{
	if(scanlineX <= 64)
		clearSecondaryOAMByte();
	else if(scanlineX == 65)
	{
		N = read(OAMADDR);
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
		{
			N += 4;
		}
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
		{
			++M;
		}
	}
	else if(found8Sprites)
	{
		spriteOverflowEval();
	}
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
	write(OAMADDR, 0x00);
}