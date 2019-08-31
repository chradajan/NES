#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <SDL2/SDL.h>

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;
const int channels = 3;

class GameWindow
{
public:
    GameWindow(char* frameBuffer);
    ~GameWindow();
    void update();
private:
    SDL_Window* window = nullptr;
    SDL_Surface* screenSurface = nullptr;
};

#endif