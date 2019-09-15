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

        //Actual commented out

        // const uint8_t* currentKeyStates = SDL_GetKeyboardState(NULL);

        // nes->buttonPress(0x4016, A, currentKeyStates[SDL_SCANCODE_L]);
        // nes->buttonPress(0x4016, B, currentKeyStates[SDL_SCANCODE_K]);  
        // nes->buttonPress(0x4016, SELECT, currentKeyStates[SDL_SCANCODE_O]);
        // nes->buttonPress(0x4016, START, currentKeyStates[SDL_SCANCODE_P]);
        // nes->buttonPress(0x4016, UP, currentKeyStates[SDL_SCANCODE_W]);
        // nes->buttonPress(0x4016, DOWN, currentKeyStates[SDL_SCANCODE_S]);
        // nes->buttonPress(0x4016, LEFT, currentKeyStates[SDL_SCANCODE_A]);
        // nes->buttonPress(0x4016, RIGHT, currentKeyStates[SDL_SCANCODE_D]);

        //End here

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