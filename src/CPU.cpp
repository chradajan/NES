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
	registers->PC = 0x8000; //This might need to be changed based on mapper
	
	memory[0x4017] = 0x00;
	memory[0x4015] = 0x00;

	memory[0x0069] = 0x69;

	for(int i = 0x4000; i <= 0x400F; ++i)
		memory[i] = 0x00;
	//TODO: Set Noise Channel to 0x0000

	loadROM(file);
}

void CPU::tick()
{
	executeInstruction();
}

CPU::~CPU()
{
	delete registers;
}

void CPU::loadROM(const char* file)
{
	uint16_t loc = 0x8000;
	std::ifstream rom(file);
	uint8_t temp1, temp2;
	uint8_t value;

	while(rom >> std::hex >> temp1)
	{
		rom >> std::hex >> temp2;
		temp1 = convertAscii(temp1);
		temp2 = convertAscii(temp2);

		value = (temp1 << 4) + temp2;

		memory[loc] = value;
		++loc;
	}

	if(loc <= 0xBFFF)
	{
		uint16_t mirrorLoc = 0xC000;
		for(uint16_t i = 0x8000; i <= loc; ++i)
		{
			memory[mirrorLoc] = memory[i];
			++mirrorLoc;
		}
	}
	
	rom.close();
}

uint8_t CPU::convertAscii(uint8_t c)
{
	if(c >= 48 && c <= 57)
			return c - 48;
		else if(c >= 65 && c <= 70)
			return c - 55;
		else if(c >= 97 && c <= 102)
			return c - 87;
		else
			throw BadRom{};
}

uint8_t CPU::readByte()
{
	return memory[registers->PC++];
}

bool CPU::if_carry()
{
	return registers->P & 0x1;
}

bool CPU::if_overflow()
{
	return (registers->P >> 6) & 0x1;
}

bool CPU::if_sign()
{
	return (registers->P >> 7) & 0x1;
}

bool CPU::if_zero()
{
	return (registers->P >> 1) & 0x1;
}

void CPU::set_sign(uint16_t value)
{
	registers->P |= ((value >> 7) & 0x1) << 7;
}

void CPU::set_zero(uint16_t value)
{
	if(value == 0)
		registers->P |= 0x1 << 1;
	else
		registers->P &= ~(0x1 << 1);
}

void CPU::set_carry(bool condition)
{
	if(condition)
		registers->P |= 0x1;
	else
		registers->P &= ~(0x1);
}

void CPU::set_overflow(bool condition)
{
	if(condition)
		registers->P |= 0x1 << 6;
	else
		registers->P &= ~(0x1 << 6);
}

void CPU::set_interrupt(bool condition)
{
	if(condition)
		registers->P |= 0x1 << 2;
	else
		registers->P &= ~(0x1 << 2);
}

void CPU::set_break(bool condition)
{
	if(condition)
		registers->P |= 0x1 << 4;
	else
		registers->P &= ~(0x1 << 4);
}

uint8_t CPU::fetchImmediate()
{
	return readByte();
}

uint8_t CPU::fetchZeroPage()
{
	return memory[readByte()];
}

uint8_t CPU::fetchZeroPageX()
{
	uint8_t address = readByte();
	address += registers->X;
	return memory[address];
}

uint8_t CPU::fetchZeroPageY()
{
	uint8_t address = readByte();
	address += registers->Y;
	return memory[address];
}

uint8_t CPU::fetchAbsolute()
{
	uint8_t lowByte = readByte();
	uint8_t highByte = readByte();
	return memory[(highByte << 8) + lowByte];
}

uint8_t CPU::fetchAbsoluteX()
{
	uint8_t lowByte = readByte();
	uint8_t highByte = readByte();
	uint16_t temp = lowByte + registers->X;
	if(temp > 0xFF)
		++cycles;
	uint16_t address = (highByte << 8) + temp;
	return memory[address];
}

uint8_t CPU::fetchAbsoluteY()
{
	uint8_t lowByte = readByte();
	uint8_t highByte = readByte();
	uint16_t temp = lowByte + registers->Y;
	if(temp > 0xFF)
		++cycles;
	uint16_t address = (highByte << 8) + temp;
	return memory[address];
}

uint8_t CPU::fetchIndirectX()
{
	uint8_t firstAddress = readByte() + registers->X;
	uint16_t operandAddress = (memory[firstAddress + 1] << 8) + memory[firstAddress];
	return memory[operandAddress];
}

uint8_t CPU::fetchIndirectY()
{
	uint8_t firstAddress = readByte();
	uint16_t address = memory[firstAddress] + registers->Y;
	if(address > 0xFF)
		++cycles;
	address = (memory[firstAddress+1] << 8) + address;
	return memory[address];
}

void CPU::executeInstruction()
{
	switch(memory[registers->PC++])
	{
		case 0x69: //Immediate ADC
		{
			cycles = 2;
			ADC(fetchImmediate());
			break;
		}
		case 0x65: //Zero Page ADC
		{
			cycles = 3;
			ADC(fetchZeroPage());
			break;
		}
		case 0x75: //Zero Page,X ADC
		{
			cycles = 4;
			ADC(fetchZeroPageX());
			break;
		}
		case 0x60: //Absolute ADC
		{
			cycles = 4;
			ADC(fetchAbsolute());
			break;
		}
		case 0x70: //Absolute,X ADC
		{
			cycles = 4;
			ADC(fetchAbsoluteX());
			break;
		}
		case 0x79: //Absolute,Y ADC
		{
			cycles = 4;
			ADC(fetchAbsoluteY());
			break;
		}
		case 0x61: //Indirect,X ADC
		{
			cycles = 6;
			ADC(fetchIndirectX());
			break;
		}
		case 0x71: //Indirect,Y ADC
		{
			cycles = 5;
			ADC(fetchIndirectY());
			break;
		}
	}
}

void CPU::ADC(uint8_t operand)
{
	uint16_t temp = operand + registers->A + (if_carry() ? 1 : 0);
	set_zero(temp & 0xFF);
	set_sign(temp);
	set_overflow(!((registers->A ^ operand) & 0x80) && ((registers->A ^ temp) & 0x80));
	set_carry(temp > 0xFF);
	registers->A = temp;
}

void CPU::CPU_TESTING()
{
	// registers->P = 0b00000000;
	// std::cout << std::hex << (unsigned int)registers->P << std::endl;
	// uint16_t test = 0x1;
	// set_zero(test);
	// std::cout << std::hex << (unsigned int)registers->P << std::endl;
}