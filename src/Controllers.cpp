#include "include/Controllers.hpp"

Controllers::Controllers()
{

}

uint8_t Controllers::read(uint16_t address)
{
    uint8_t controllerBit = 0x00;
    if(address == 0x4016) //For now, only read from controller one. TODO: use address to determine which controller to read from
    {
        if(S)
        {
            const uint8_t* currentKeyStates = SDL_GetKeyboardState(NULL);
            controllerBit = (currentKeyStates[SDL_SCANCODE_L] ? 0x01 : 0x00); //Check A key
        }
        else
        {
            controllerBit |= (JOY1 & 0x01);
            JOY1 >>= 1;
            JOY1 |= 0x80;
        }        
    }   
    return controllerBit; 
}

void Controllers::write(uint8_t data)
{
    if(data & 0x01)
    {
        S = true;
        JOY1 = 0x00;
    }
    else
    {
        S = false;
        getKeyPresses();
    }
}

void Controllers::getKeyPresses()
{
    const uint8_t* currentKeyStates = SDL_GetKeyboardState(NULL);

    if(currentKeyStates[SDL_SCANCODE_L]) //A
        JOY1 |= 0b00000001;
    else
        JOY1 &= 0b11111110;

    if(currentKeyStates[SDL_SCANCODE_K]) //B
        JOY1 |= 0b00000010;
    else
        JOY1 &= 0b11111101;  

    if(currentKeyStates[SDL_SCANCODE_O]) //Select
        JOY1 |= 0b00000100;
    else
        JOY1 &= 0b11111011;  

    if(currentKeyStates[SDL_SCANCODE_P]) //START
        JOY1 |= 0b00001000;
    else
        JOY1 &= 0b11110111;

    if(currentKeyStates[SDL_SCANCODE_W]) //UP
        JOY1 |= 0b00010000;
    else
        JOY1 &= 0b11101111; 

    if(currentKeyStates[SDL_SCANCODE_S]) //DOWN
        JOY1 |= 0b00100000;
    else
        JOY1 &= 0b11011111;  

    if(currentKeyStates[SDL_SCANCODE_A]) //LEFT
        JOY1 |= 0b01000000;
    else
        JOY1 &= 0b10111111;   

    if(currentKeyStates[SDL_SCANCODE_D]) //RIGHT
        JOY1 |= 0b10000000;
    else
        JOY1 &= 0b01111111;  
}

Controllers::~Controllers()
{

}