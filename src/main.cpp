#include <iostream>
#include <fstream>
#include "CPU.hpp"
#include "PPU.hpp"

int main()
{
	std::ifstream rom("DonkeyKong.txt");
	CPU cpu(rom);
	PPU ppu(rom);
	//CPU cpu("test.txt");
	//cpu.tick();
	ppu.PPU_TESTING();
	return 0;
}