#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include "include/NES.hpp"

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;

int main(int argc, char* args[])
{
	std::fstream cpuLog("../logs/log.txt", std::ios::out);
	NES nes("../roms/DonkeyKong.nes", cpuLog);
	//NES nes("C:\\Users\\Chris\\Desktop\\NES\\roms\\DonkeyKong.nes", cpuLog);

	// SDL_Window* window = nullptr;
	// SDL_Surface* screenSurface = nullptr;

	// int channels = 3;

	// char* a = new char[SCREEN_WIDTH * SCREEN_HEIGHT * channels];
	// char* b = new char[SCREEN_WIDTH * SCREEN_HEIGHT * channels];
	// char* c = new char[SCREEN_WIDTH * SCREEN_HEIGHT * channels];

	// char* pixels;

	// for(int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * channels; ++i)
	// {
	// 	if(i < 92160)
	// 	{
	// 		if(i % 3 == 0)
	// 		{
	// 			a[i] = 0xFF;
	// 			b[i] = 0x00;
	// 			c[i] = 0x00;
	// 		}
	// 		else if(i % 3 == 1)
	// 		{
	// 			a[i] = 0x00;
	// 			b[i] = 0x00;
	// 			c[i] = 0xFF;
	// 		}
	// 		else
	// 		{
	// 			a[i] = 0x00;
	// 			b[i] = 0xFF;
	// 			c[i] = 0x00;
	// 		}
			
	// 	}
	// 	else
	// 	{
	// 		if(i % 3 == 0)
	// 		{
	// 			a[i] = 0x00;
	// 			b[i] = 0xFF;
	// 			c[i] = 0x00;
	// 		}
	// 		else if(i % 3 == 1)
	// 		{
	// 			a[i] = 0xFF;
	// 			b[i] = 0x00;
	// 			c[i] = 0x00;
	// 		}
	// 		else
	// 		{
	// 			a[i] = 0x00;
	// 			b[i] = 0x00;
	// 			c[i] = 0xFF;
	// 		}
			
	// 	}
	// }

	// SDL_Init(SDL_INIT_VIDEO);

	// window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

	// for(int i = 0; i < 25; ++i)
	// {
	// 	screenSurface = SDL_CreateRGBSurfaceFrom((void*)a, SCREEN_WIDTH, SCREEN_HEIGHT, channels * 8, SCREEN_WIDTH * channels, 0x0000FF, 0x00FF00, 0xFF0000, 0);
	// 	SDL_BlitSurface(screenSurface, 0, SDL_GetWindowSurface(window), 0);
	// 	SDL_UpdateWindowSurface(window);
	// 	SDL_Delay(100);
	// 	screenSurface = SDL_CreateRGBSurfaceFrom((void*)b, SCREEN_WIDTH, SCREEN_HEIGHT, channels * 8, SCREEN_WIDTH * channels, 0x0000FF, 0x00FF00, 0xFF0000, 0);
	// 	SDL_BlitSurface(screenSurface, 0, SDL_GetWindowSurface(window), 0);
	// 	SDL_UpdateWindowSurface(window);
	// 	SDL_Delay(100);
	// 	screenSurface = SDL_CreateRGBSurfaceFrom((void*)c, SCREEN_WIDTH, SCREEN_HEIGHT, channels * 8, SCREEN_WIDTH * channels, 0x0000FF, 0x00FF00, 0xFF0000, 0);
	// 	SDL_BlitSurface(screenSurface, 0, SDL_GetWindowSurface(window), 0);
	// 	SDL_UpdateWindowSurface(window);
	// 	SDL_Delay(100);
	// }

	// SDL_DestroyWindow(window);
	// SDL_Quit();

	for(int i = 0; i < 26554; ++i)
		nes.tick();

	return 0;
}