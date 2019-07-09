#include "CPU.hpp"
#include "Exceptions.hpp"
#include <iostream>
#include <fstream>

CPU::CPU(const char* file)
{
	registers = new CPU_Registers();

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

	LoadROM(file);
}

void CPU::LoadROM(const char* file)
{
	uint16_t loc = 0x8000;
	std::ifstream rom(file);
	char temp1, temp2;
	uint8_t value;
	//while(!rom.eof())
	for(int j = 0; j < 16; ++j)
	{
		rom >> std::hex >> temp1;
		std::cout << temp1 << std::endl;
		rom >> std::hex >> temp2;
		
		if(temp1 >= 48 && temp1 <= 57)
			temp1 -= 48;
		else if(temp1 >= 65 && temp1 <= 70)
			temp1 -= 55;
		else if(temp1 >= 97 && temp1 <= 102)
			temp1 -= 87;
		else
			throw BadRom{};

		if(temp2 >= 48 && temp2 <= 57)
			temp2 -= 48;
		else if(temp2 >= 65 && temp2 <= 70)
			temp2 -= 55;
		else if(temp2 >= 97 && temp2 <= 102)
			temp2 -= 87;
		else
			throw BadRom{};

		value = temp1*16 + temp2;
		std::cout << temp1 << std::endl;
	}

	std::cout << std::endl;
	rom.close();
}


void CPU::CPU_TESTING()
{

}