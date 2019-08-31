#include <iostream>
#include <fstream>
#include "include/NES.hpp"
#include "include/GameWindow.hpp"

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	std::fstream cpuLog("../logs/log.txt", std::ios::out);
	char* frameBuffer = new char[SCREEN_WIDTH * SCREEN_HEIGHT * channels];
	GameWindow screen(frameBuffer);
	//NES nes("../roms/DonkeyKong.nes", cpuLog, frameBuffer, renderFrame, frameCounter);
	NES nes("C:/Users/Chris/Desktop/NES/roms/nestest.nes", cpuLog, frameBuffer, screen);

	while(true)
		nes.tick();

	delete[] frameBuffer;

	return 0;
}