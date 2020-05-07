#include "../include/NES.hpp"
#include "../mappers/include/NROM.hpp"
#include <cassert>
#include <iomanip>

NES::NES(const char* file, std::shared_ptr<char[]> frameBuffer)
{
	initCart(file);
	ppu = std::shared_ptr<PPU>(new PPU(cart, frameBuffer, frameReady));
	apu = std::shared_ptr<APU>(new APU());
	controllers = std::shared_ptr<Controllers>(new Controllers());
	cpu = std::unique_ptr<CPU>(new CPU(cart, ppu, apu, controllers));
}

void NES::prepareFrame()
{
	while(!frameReady)
	{
		cpu->tick();
		ppu->tick();
		ppu->tick();
		ppu->tick();
	}
	frameReady = false;
}

void NES::initCart(const std::string& romPath)
{
    std::ifstream rom(romPath, std::ios::binary);
    std::array<uint8_t, 16> header;
    
    for(int i = 0; i < 16; ++i)
        rom >> std::noskipws >> std::hex >> header[i];

    int mapperNumber = (header[7] & 0xF0) + (header[6] >> 4);

    switch(mapperNumber)
    {
        case 0: //NROM
            cart = std::shared_ptr<Cartridge>(new NROM(rom, header));
            break;
        //...
        default:
            throw("Unrecognized mapper number");
    }
}