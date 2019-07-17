#include "include/PPU.hpp"
#include <iostream> //TODO: remove when done testing

PPU::PPU(uint8_t* cpu_mem, uint8_t* ppu_mem) : 
	memory(ppu_mem), PPUCTRL(cpu_mem[0x2000]), PPUMASK(cpu_mem[0x2001]), PPUSTATUS(cpu_mem[0x2002]),
	OAMADDR(cpu_mem[0x2003]), OAMDATA(cpu_mem[0x2004]), PPUSCROLL(cpu_mem[0x2005]),
	PPUADDR(cpu_mem[0x2006]), PPUDATA(cpu_mem[0x2007]), OAMDMA(cpu_mem[0x4014])
{

}

void PPU::tick()
{

}

PPU::~PPU()
{

}

uint8_t PPU::readMEMORY(uint16_t address)
{
	if(address <= 0x2FFF)
		return memory[address];
	else if(address <= 0x3EFF)
		return memory[address - 0x1000];
	else if(address <= 0x3F1F)
		return memory[address];
	else
		return memory[address - ((address / 0x20) * 0x20) + 0x3F00];
}

void PPU::writeMEMORY(uint16_t address, uint8_t data)
{
	//TODO: check for illegal writes
	memory[address] = data;
}

void PPU::setSpriteOverflowFlag(bool condition)
{
	if(condition)
		PPUSTATUS |= 0x1 << 5;
	else
		PPUSTATUS &= ~(0x1 << 5);
}

bool PPU::ifSpriteOverflow()
{
	return (PPUSTATUS >> 5) & 0x1;
}

void PPU::evaluateSprites()
{
	if(scanlineX <= 64)
		clearSecondaryOAMByte();
	else if(scanlineX == 65)
	{
		N = OAMADDR;
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
	secondary_oam[scanlineX % 32] = 0xFF;
}

void PPU::spriteEvalRead()
{
	oam_buffer = primary_oam[N + M];
}

void PPU::spriteEvalWrite()
{
	if(N >= 64)
		return;
	else if(!found8Sprites && M == 0)
	{
		secondary_oam[secondary_oam_loc] = oam_buffer;
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
		secondary_oam[secondary_oam_loc++] = oam_buffer;
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
	else if(primary_oam[N + M] - 1 == scanlineY)
		setSpriteOverflowFlag(1);
	else
	{
		N += 4;
		M = (M == 3 ? 0 : M + 1);
	}
}

void PPU::spriteFetch()
{
	OAMADDR = 0;

}

void PPU::PPU_TESTING()
{
}