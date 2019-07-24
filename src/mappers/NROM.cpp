#include "NROM.hpp"

NROM::NROM(HeaderData& header, std::ifstream& rom)
{
	if(header.PRG_ROM_SIZE == 1)
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

	mirroringIsVertical = header.Flags6 & 0x01;

	loadROM(rom);
}

uint8_t NROM::readPRG(uint16_t address) const
{
	if(address < 0x8000)
		throw IllegalROMRead("Attempted to read PRG ROM", address);

	if(PRG_Mirroring)
		address = (address % 0xC000) + ((address / 0xC000) * 0x8000) - 0x8000;
	else
		address -= 0x8000;

	return PRG_ROM[address];
}

void NROM::writePRG(uint16_t address, uint8_t data)
{
	(void)address; (void)data;
	throw IllegalROMWrite("Attempted to write PRG ROM", address, data);
}

uint8_t NROM::readCHR(uint16_t address) const
{
	if(address > 0x1FFF)
		throw IllegalROMRead("Attempted to read CHR ROM", address);
	return CHR_ROM[address];
}

void NROM::writeCHR(uint16_t address, uint8_t data)
{
	(void)address; (void)data;
	throw IllegalROMWrite("Attempted to write CHR ROM", address, data);
	//TODO: Figure out what happens when a write happens to NROM
	//Probably throw an exception
}

bool NROM::verticalMirroring() const
{
	return mirroringIsVertical;
}

NROM::~NROM()
{
	delete[] PRG_ROM;
	delete[] CHR_ROM;
}

void NROM::loadROM(std::ifstream& rom)
{
	uint16_t max = PRG_Mirroring ? 0x4000 : 0x8000;
	for(uint16_t address = 0x0000; address < max; ++address)
	{
		rom >> std::noskipws >> std::hex >> PRG_ROM[address];
	}

	for(uint16_t address = 0x0000; address < 0x2000; ++address)
		rom >> std::hex >> CHR_ROM[address];
}