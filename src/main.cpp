#include <iostream>
#include <fstream>
#include "include/NES.hpp"

int main()
{
	std::fstream cpuLog("../logs/log.txt", std::ios::out);
	NES nes("../roms/nestest.nes", cpuLog);
	
	for(int i = 0; i < 26554; ++i)
		nes.tick();

	return 0;
}