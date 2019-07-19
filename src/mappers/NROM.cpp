#include "NROM.hpp"

NROM::NROM(uint8_t PRG_ROM_MULTIPLIER)
{
	if(PRG_ROM_MULTIPLIER == 1)
	{
		PRG_Mirroring = true;
		PRG_ROM = new uint8_t[0x4000];
	}
	else
	{
		PRG_Mirroring = false;
		PRG_ROM = new uint8_t[0x8000];
	}
	CHR_ROM = new uint8_t[0x2000];
}

void NROM::writePRG(uint16_t address, uint8_t data)
{
	(void)address; (void)data;
	throw;
	//TODO: Figure out what happens when a write happens to NROM
	//Probably throw an exception
}

uint8_t NROM::readPRG(uint16_t address)
{
	if(address < 0x8000)
		throw;
		//TODO: Throw exception

	if(PRG_Mirroring)
		address = (address % 0xC000) + ((address / 0xC000) * 0x8000) - 0x8000;
	else
		address -= 0x8000;

	return PRG_ROM[address];
}

void NROM::writeCHR(uint16_t address, uint8_t data)
{
	(void)address; (void)data;
	throw;
	//TODO: Figure out what happens when a write happens to NROM
	//Probably throw an exception
}

uint8_t NROM::readCHR(uint16_t address)
{
	if(address > 0x1FFF)
		throw;
		//TODO: Throw exception
	return CHR_ROM[address];
}

NROM::~NROM()
{
	delete[] PRG_ROM;
	delete[] CHR_ROM;
}