#include <iostream>
#include <fstream>
#include "include/NES.hpp"

int main()
{
	NES nes("../roms/Test.nes");
	nes.tick();
	nes.tick();
	nes.tick();

	return 0;
}