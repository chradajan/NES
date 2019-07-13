#include "CPU.hpp"
#include "Exceptions.hpp"
#include <iostream>
#include <fstream>

CPU::CPU(const char* file)
{
	registers.SR = 0x34;
	registers.AC = 0;
	registers.X = 0;
	registers.Y = 0;
	registers.SP = 0xFD;
	registers.PC = 0; //This might need to be changed based on mapper
	
	memory[0x4017] = 0x00;
	memory[0x4015] = 0x00;

	memory[0x0069] = 0x69; //For testing purposes

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

uint16_t CPU::mapPC()
{
	return registers.PC + 0x8000;
}

uint8_t CPU::readMEMORY(uint16_t address)
{
	//TODO: catch invalid reads
	if(address <= 0x07FF)
	{
		return memory[address];
	}
	else if(address <= 0x1FFF)
	{
		return memory[address - (address / 0x0800) * 0x0800];
	}
	else if(address <= 0x2007)
	{
		return memory[address];
	}
	else if(address <= 0x3FFF)
	{
		return memory[address - ((address / 0x0008) * 0x0008) + 0x2000];
	}
	else if(address <= 0x4017)
	{
		return memory[address];
	}
	else if(address <= 0x401F)
	{
		std::cout << "TEST MODE" << std::endl;
		//return memory[address];
	}
	else
	{
		//TODO: implement mapping
		return memory[address];
	}
	return 0;
}

void CPU::writeMEMORY(uint16_t address, uint8_t data)
{
	//TODO: catch invalid writes
	memory[address] = data;
}

uint8_t CPU::readROM()
{
	uint16_t mappedPC = mapPC();
	++registers.PC;
	return memory[mappedPC];
}

uint8_t CPU::pop()
{
	++registers.SP;
	return readMEMORY(registers.SP);
}
void CPU::push(uint8_t data)
{
	writeMEMORY(registers.SP, data);
	++registers.SP;
}

bool CPU::if_carry()
{
	return registers.SR & 0x1;
}

bool CPU::if_overflow()
{
	return (registers.SR >> 6) & 0x1;
}

bool CPU::if_sign()
{
	return (registers.SR >> 7) & 0x1;
}

bool CPU::if_zero()
{
	return (registers.SR >> 1) & 0x1;
}

void CPU::set_carry(bool condition)
{
	if(condition)
		registers.SR |= 0x1;
	else
		registers.SR &= ~(0x1);
}

void CPU::set_zero(uint16_t value)
{
	if(value == 0)
		registers.SR |= 0x1 << 1;
	else
		registers.SR &= ~(0x1 << 1);
}

void CPU::set_interrupt(bool condition)
{
	if(condition)
		registers.SR |= 0x1 << 2;
	else
		registers.SR &= ~(0x1 << 2);
}

void CPU::set_decimal(bool condition)
{
	if(condition)
		registers.SR |= 0x1 << 3;
	else
		registers.SR &= ~(0x1 << 3);
}

void CPU::set_break(bool condition)
{
	if(condition)
		registers.SR |= 0x1 << 4;
	else
		registers.SR &= ~(0x1 << 4);
}

void CPU::set_overflow(bool condition)
{
	if(condition)
		registers.SR |= 0x1 << 6;
	else
		registers.SR &= ~(0x1 << 6);
}

void CPU::set_sign(uint16_t value)
{
	registers.SR |= ((value >> 7) & 0x1) << 7;
}

uint16_t CPU::fetchZeroPageAddress()
{
	return readROM();
}

uint16_t CPU::fetchZeroPageXAddress()
{
	uint8_t address = readROM();
	address += registers.X;
	return address;
}

uint16_t CPU::fetchZeroPageYAddress()
{
	uint8_t address = readROM();
	address += registers.Y;
	return address;
}

uint16_t CPU::fetchAbsoluteAddress()
{
	uint8_t lowByte = readROM();
	uint8_t highByte = readROM();
	uint16_t address = (highByte << 8) + lowByte;
	return address;
}

uint16_t CPU::fetchAbsoluteXAddress()
{
	uint8_t lowByte = readROM();
	uint8_t highByte = readROM();
	uint16_t temp = lowByte + registers.X;
	if(temp > 0xFF)
		++cycles;
	uint16_t address = (highByte << 8) + temp;
	return address;
}

uint16_t CPU::fetchAbsoluteYAddress()
{
	uint8_t lowByte = readROM();
	uint8_t highByte = readROM();
	uint16_t temp = lowByte + registers.Y;
	if(temp > 0xFF)
		++cycles;
	uint16_t address = (highByte << 8) + temp;
	return address;
}

uint16_t CPU::fetchIndirectXAddress()
{
	uint8_t firstAddress = readROM() + registers.X;
	uint16_t address = (memory[firstAddress + 1] << 8) + memory[firstAddress];
	return address;
}

uint16_t CPU::fetchIndirectYAddress()
{
	uint8_t firstAddress = readROM();
	uint16_t address = memory[firstAddress] + registers.Y;
	if(address > 0xFF)
		++cycles;
	address = (memory[firstAddress+1] << 8) + address;
	return address;
}

void CPU::executeInstruction()
{
	uint8_t opcode = readROM();
	switch(opcode)
	{
		case 0x69: //Immediate ADC
		{
			cycles = 2;
			ADC(readROM());
			break;
		}
		case 0x65: //Zero Page ADC
		{
			cycles = 3;
			ADC(fetchZeroPageAddress());
			break;
		}
		case 0x75: //Zero Page,X ADC
		{
			cycles = 4;
			ADC(fetchZeroPageXAddress());
			break;
		}
		case 0x6D: //Absolute ADC
		{
			cycles = 4;
			ADC(fetchAbsoluteAddress());
			break;
		}
		case 0x7D: //Absolute,X ADC
		{
			cycles = 4;
			ADC(fetchAbsoluteXAddress());
			break;
		}
		case 0x79: //Absolute,Y ADC
		{
			cycles = 4;
			ADC(fetchAbsoluteYAddress());
			break;
		}
		case 0x61: //Indirect,X ADC
		{
			cycles = 6;
			ADC(fetchIndirectXAddress());
			break;
		}
		case 0x71: //Indirect,Y ADC
		{
			cycles = 5;
			ADC(fetchIndirectYAddress());
			break;
		}
		case 0x29: //Immediate AND
		{
			cycles = 2;
			AND(readROM());
			break;
		}
		case 0x25: //Zero Page AND
		{
			cycles = 3;
			AND(fetchZeroPageAddress());
			break;
		}
		case 0x35: //Zero Page,X AND
		{
			cycles = 4;
			AND(fetchZeroPageXAddress());
			break;
		}
		case 0x2D: //Absolute AND
		{
			cycles = 4;
			AND(fetchAbsoluteAddress());
			break;
		}
		case 0x3D: //Absolute,X AND
		{
			cycles = 4;
			AND(fetchAbsoluteXAddress());
			break;
		}
		case 0x39: //Absolute,Y AND
		{
			cycles = 4;
			AND(fetchAbsoluteYAddress());
			break;
		}
		case 0x21: //Indirect,X AND
		{
			cycles = 6;
			AND(fetchIndirectXAddress());
			break;
		}
		case 0x31: //Indirect,Y AND
		{
			cycles = 5;
			AND(fetchIndirectYAddress());
			break;
		}
		case 0x0A: //Accumulator ASL
		{
			cycles = 2;
			ASL(registers.AC);
			break;
		}
		case 0x06: //Zero Page ASL
		{
			cycles = 5;
			ASL(fetchZeroPageAddress());
			break;
		}
		case 0x16: //Zero Page,X ASL
		{
			cycles = 6;
			ASL(fetchZeroPageXAddress());
			break;
		}
		case 0x0E: //Absolute ASL
		{
			cycles = 6;
			ASL(fetchAbsoluteAddress());
			break;
		}
		case 0x1E: //Absolute,X ASL
		{
			cycles = 2;
			ASL(fetchAbsoluteXAddress());
			break;
		}
		case 0x90: //Relative BCC
		{
			BRANCH(!if_carry());
			break;
		}
		case 0xB0: //Relative BCS
		{
			BRANCH(if_carry());
			break;
		}
		case 0xF0: //Relative BEQ
		{
			BRANCH(if_zero());
			break;
		}
		case 0x24: //Zero Page BIT
		{
			cycles = 3;
			BIT(fetchZeroPageAddress());
			break;
		}
		case 0x2C: //Absolute BIT
		{
			cycles = 4;
			BIT(fetchAbsoluteAddress());
			break;
		}
		case 0x30: //Relative BMI
		{
			BRANCH(if_sign());
			break;
		}
		case 0xD0: //Relative BNE
		{
			BRANCH(!if_zero());
			break;
		}
		case 0x10: //Relative BPL
		{
			BRANCH(!if_sign());
			break;
		}
		case 0x00: //Implied BRK
		{
			cycles = 7;
			++registers.PC;
			push((registers.PC >> 8) & 0xFF);
			push(registers.PC & 0xFF);
			set_break(1);
			push(registers.SR);
			set_interrupt(1);
			uint16_t newPC = readMEMORY(0xFFFF);
			newPC <<= 8;
			newPC += readMEMORY(0xFFFE);
			registers.PC = newPC;
			break;
		}
		case 0x50: //Relative BVC
		{
			BRANCH(!if_overflow());
			break;
		}
		case 0x70: //Relative BVS
		{
			BRANCH(if_overflow());
			break;
		}
		case 0x18: //Implied CLC
		{
			cycles = 2;
			set_carry(0);
			break;
		}
		case 0xD8: //Implied CLD
		{
			cycles = 2;
			set_decimal(0);
			break;
		}
		case 0x58: //Implied CLI
		{
			cycles = 2;
			set_interrupt(0);
			break;
		}
		case 0xB8: //Implied CLV
		{
			cycles = 2;
			set_overflow(0);
			break;
		}
		case 0xC9: //Immediate CMP
		{
			cycles = 2;
			CMP(readROM());
			break;
		}
		case 0xC5: //Zero Page CMP
		{
			cycles = 3;
			CMP(fetchZeroPageAddress());
			break;
		}
		case 0xD5: //Zero Page,X CMP
		{
			cycles = 4;
			CMP(fetchZeroPageXAddress());
			break;
		}
		case 0xCD: //Absolute CMP
		{
			cycles = 4;
			CMP(fetchAbsoluteAddress());
			break;
		}
		case 0xDD: //Absolute,X CMP
		{
			cycles = 4;
			CMP(fetchAbsoluteXAddress());
			break;
		}
		case 0xD9: //Absolute,Y CMP
		{
			cycles = 4;
			CMP(fetchAbsoluteYAddress());
			break;
		}
		case 0xC1: //Indirect,X CMP
		{
			cycles = 6;
			CMP(fetchIndirectXAddress());
			break;
		}
		case 0xD1: //Indirect,Y CMP
		{
			cycles = 5;
			CMP(fetchIndirectYAddress());
			break;
		}
	}
}

uint16_t CPU::relativeAddress(uint8_t offset)
{
	bool isNegative = (offset & 0x80) >> 7;
	if(isNegative)
	{
		int8_t signedOffset = offset & 0x7F;
		signedOffset *= -1;
		return registers.PC + signedOffset;
	}
	return registers.PC + offset;
}

void CPU::ADC(uint8_t operand)
{
	uint16_t temp = operand + registers.AC + (if_carry() ? 1 : 0);
	set_zero(temp & 0xFF);
	set_sign(temp);
	set_overflow(!((registers.AC ^ operand) & 0x80) && ((registers.AC ^ temp) & 0x80));
	set_carry(temp > 0xFF);
	registers.AC = temp;
}

void CPU::ADC(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	uint16_t temp = operand + registers.AC + (if_carry() ? 1 : 0);
	set_zero(temp & 0xFF);
	set_sign(temp);
	set_overflow(!((registers.AC ^ operand) & 0x80) && ((registers.AC ^ temp) & 0x80));
	set_carry(temp > 0xFF);
	registers.AC = temp;
}

void CPU::AND(uint8_t operand)
{
	operand &= registers.AC;
	set_sign(operand);
	set_zero(operand);
	registers.AC = operand;
}

void CPU::AND(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	operand &= registers.AC;
	set_sign(operand);
	set_zero(operand);
	registers.AC = operand;
}

void CPU::ASL(uint8_t operand)
{
	set_carry(operand & 0x80);
	operand <<= 1;
	set_sign(operand);
	set_zero(operand);
	registers.AC = operand;
}

void CPU::ASL(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	set_carry(operand & 0x80);
	operand <<= 1;
	set_sign(operand);
	set_zero(operand);
	writeMEMORY(operandAddress, operand);
}

void CPU::BIT(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	set_sign(operand);
	set_overflow(0x40 & operand);
	set_zero(operand & registers.AC);
}

void CPU::BRANCH(bool condition)
{
	cycles = 2;
	uint8_t operand = readROM();
	if(condition)
	{
		uint16_t newPC = relativeAddress(operand);
		cycles += ((registers.PC & 0xFF00) != (newPC & 0xFF00) ? 2 : 1);
		registers.PC = newPC;
	}
}

void CPU::CMP(uint8_t operand)
{
	uint16_t result = registers.AC - operand;
	set_carry(result < 0x100);
	set_sign(result);
	set_zero(result & 0xFF);
}

void CPU::CMP(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	uint16_t result = registers.AC - operand;
	set_carry(result < 0x100);
	set_sign(result);
	set_zero(result & 0xFF);
}

void CPU::CPU_TESTING()
{
}