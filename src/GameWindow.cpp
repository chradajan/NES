#include "include/GameWindow.hpp"

GameWindow::GameWindow()
{
    frameBuffer = new char[SCREEN_WIDTH * SCREEN_HEIGHT * channels];
    screenSurface = SDL_CreateRGBSurfaceFrom((void*)frameBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, channels * 8, SCREEN_WIDTH * channels, 0x0000FF, 0x00FF00, 0xFF0000, 0);
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    nes = new NES("C:/Users/Chris/Desktop/NES/roms/DonkeyKong.nes", frameBuffer);
}

void GameWindow::run()
{
    bool quit = false;

    SDL_Event e;

    while(!quit)
    {
        while(SDL_PollEvent(&e) != 0)
        {
            if(e.type == SDL_QUIT)
                quit = true;
        }

        nes->prepareFrame();
        update();
    }
}

GameWindow::~GameWindow()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GameWindow::update()
{
    SDL_BlitSurface(screenSurface, 0, SDL_GetWindowSurface(window), 0);
    SDL_UpdateWindowSurface(window);
}