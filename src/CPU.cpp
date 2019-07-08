#include "CPU.hpp"
#include <iostream>

CPU::CPU()
{
	registers = new CPU_Registers();
	memory = new char[0xFFFF];

	registers->P = 0x34;
	registers->A = 0;
	registers->X = 0;
	registers->Y = 0;
	registers->S = 0xFD;
	
	memory[0x4017] = 0x00;
	memory[0x4015] = 0x00;
	for(int i = 0x4000; i <= 0x400F; ++i)
		memory[i] = 0x00;
	//TODO: Set Noise Channel to 0x0000
}

void CPU::CPU_TESTING()
{

}