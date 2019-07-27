#include <iostream>
#include <fstream>
#include "include/NES.hpp"

int main()
{
	std::fstream cpuLog("../logs/log.txt", std::ios::out);
	NES nes("../roms/DonkeyKong.nes", cpuLog);
	//NES nes("C:\\Users\\Chris\\Desktop\\NES\\roms\\DonkeyKong.nes", cpuLog);
	for(int i = 0; i < 26554; ++i)
		nes.tick();

	return 0;
}