#include <iostream>
#include <fstream>
#include "include/NES.hpp"

int main()
{
	NES nes("../roms/Test.nes");
	nes.printRegisters();
	nes.tick();
	nes.printRegisters();
	nes.tick();
	nes.printRegisters();
	nes.tick();
	nes.printRegisters();
	nes.tick();
	nes.printRegisters();
	nes.tick();
	nes.printRegisters();
	nes.tick();
	nes.printRegisters();
	return 0;
}