#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include "include/NES.hpp"

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;
const int channels = 3;

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	std::fstream cpuLog("../logs/log.txt", std::ios::out);
	char* frameBuffer = new char[SCREEN_WIDTH * SCREEN_HEIGHT * channels];
	bool frameReady = false;
	int frameCounter = 0;
	//NES nes("../roms/DonkeyKong.nes", cpuLog, frameBuffer, renderFrame, frameCounter);
	NES nes("C:/Users/Chris/Desktop/NES/roms/DonkeyKong.nes", cpuLog, frameBuffer, frameReady, frameCounter);

	SDL_Window* window = nullptr;
	SDL_Surface* screenSurface = nullptr;

	screenSurface = SDL_CreateRGBSurfaceFrom((void*)frameBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, channels * 8, SCREEN_WIDTH * channels, 0x0000FF, 0x00FF00, 0xFF0000, 0);

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

	while(true)
	{
		if(!frameReady)
			nes.tick();
		else
		{
			SDL_BlitSurface(screenSurface, 0, SDL_GetWindowSurface(window), 0);
			SDL_UpdateWindowSurface(window);
			SDL_Delay(15);
			frameReady = false;
		}
	}

	SDL_Delay(1000);


	SDL_DestroyWindow(window);
	SDL_Quit();

	delete[] frameBuffer;

	return 0;
}