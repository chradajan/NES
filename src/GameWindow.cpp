#include "../include/GameWindow.hpp"
#include <iostream>

GameWindow::GameWindow()
{
    frameBuffer = new char[SCREEN_WIDTH * SCREEN_HEIGHT * CHANNELS];
    screenSurface = SDL_CreateRGBSurfaceFrom((void*)frameBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, CHANNELS * 8, SCREEN_WIDTH * CHANNELS, 0x0000FF, 0x00FF00, 0xFF0000, 0);
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    windowSurface = SDL_GetWindowSurface(window);

    nes = new NES("../roms/Mario.nes", frameBuffer);
}

void GameWindow::runNES()
{
    bool quit = false;

    SDL_Event e;

    uint32_t startTime;
    int frameTicks;

    while(!quit)
    {
        startTime = SDL_GetTicks();

        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
                quit = true;
        }

        nes->prepareFrame();

        frameTicks = SDL_GetTicks() - startTime;

        if(frameTicks < SCREEN_TICKS_PER_FRAME)
            SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);

        updateScreen();
    }
}

GameWindow::~GameWindow()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GameWindow::updateScreen()
{
    SDL_BlitSurface(screenSurface, NULL, windowSurface, NULL);
    SDL_UpdateWindowSurface(window);
}