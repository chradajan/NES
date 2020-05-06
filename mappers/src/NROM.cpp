#include "../include/NROM.hpp"

NROM::NROM(std::ifstream& rom, const std::array<uint8_t, 16>& header)
{
	PRG_RAM.fill(0x00);

	//Check if ROM contains trainer data. Load 512 bytes of trainer data to PRG_RAM if true
    if(header[6] & 0b00000100) //True if ROM contains trainer
    {
        for(int i = 0; i < 512; ++i)
            rom >> std::hex >> PRG_RAM[0x1000 + i];
    }

	//Check how many 16K PRG ROM banks are on cartridge
    if(header[4] == 1)
    {
        PRG_Mirroring = true;
        PRG_ROM.resize(0x4000);
    }
    else
    {
        PRG_Mirroring = false;
        PRG_ROM.resize(0x8000);
    }

	//Check if ROM uses vertical or horizontal nametable mirroring
    if(header[6] & 0b00000001)
	{
        mirroringType = MirrorType::VERTICAL;
	}
    else
	{
    	mirroringType = MirrorType::HORIZONTAL;
	}
	loadROM(rom);
}

uint8_t NROM::readPRG(uint16_t addr)
{
	if(addr < 0x6000)
		throw IllegalROMRead("Attempted to read PRG ROM", addr);
	else if(addr < 0x8000)
		return PRG_RAM[addr - 0x6000];
	else
	{
		if(PRG_Mirroring)
            return PRG_ROM[(addr % 0xC000) + ((addr / 0xC000) * 0x8000) - 0x8000];
        else
            return PRG_ROM[addr - 0x8000];
	}
}

void NROM::writePRG(uint16_t addr, uint8_t data)
{
	(void)addr; (void)data;
}

uint8_t NROM::readCHR(uint16_t addr)
{
	if(addr > 0x1FFF)
		throw IllegalROMRead("Attempted to read CHR ROM", addr);
	return CHR_ROM[addr];
}

void NROM::writeCHR(uint16_t addr, uint8_t data)
{
	(void)addr; (void)data;
}

uint16_t NROM::nametableAddress(uint16_t addr)
{
    addr &= 0x3FFF;
    if(addr > 0x2FFF)
        addr -= 0x1000;

    switch (mirroringType)
    {
        case MirrorType::HORIZONTAL:
            addr = (addr - 0x2000) - (addr / 0x2400 * 0x400) - (addr / 0x2C00 * 0x400);
            break;
        case MirrorType::VERTICAL:
            addr = (addr - 0x2000) - (addr / 0x2800 * 0x800);
            break;
        default:
            throw "Unkown mirroring type";
            break;
    }
    return addr;
}

void NROM::loadROM(std::ifstream& rom)
{
	uint16_t max = PRG_Mirroring ? 0x4000 : 0x8000;
	for(uint16_t addr = 0x0000; addr < max; ++addr)
		rom >> std::noskipws >> std::hex >> PRG_ROM[addr];

	for(uint16_t addr = 0x0000; addr < 0x2000; ++addr)
		rom >> std::hex >> CHR_ROM[addr];
}