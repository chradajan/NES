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
	--registers.SP;
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

uint16_t CPU::fetchAbsoluteXAddress(bool addCycle = true)
{
	uint8_t lowByte = readROM();
	uint8_t highByte = readROM();
	uint16_t temp = lowByte + registers.X;
	if(temp > 0xFF && addCycle)
		++cycles;
	uint16_t address = (highByte << 8) + temp;
	return address;
}

uint16_t CPU::fetchAbsoluteYAddress(bool addCycle = true)
{
	uint8_t lowByte = readROM();
	uint8_t highByte = readROM();
	uint16_t temp = lowByte + registers.Y;
	if(temp > 0xFF && addCycle)
		++cycles;
	uint16_t address = (highByte << 8) + temp;
	return address;
}

uint16_t CPU::fetchIndirectAddress()
{
	uint8_t highByte = readROM();
	uint8_t lowByte = readROM();
	uint16_t firstAddress = (highByte << 8) + lowByte;
	uint16_t address = (readMEMORY(firstAddress + 1) << 8) + readMEMORY(firstAddress);
	return address;
}

uint16_t CPU::fetchIndirectXAddress()
{
	uint8_t firstAddress = readROM() + registers.X;
	uint16_t address = (readMEMORY(firstAddress + 1) << 8) + readMEMORY(firstAddress);
	return address;
}

uint16_t CPU::fetchIndirectYAddress(bool addCycle = true)
{
	uint8_t firstAddress = readROM();
	uint16_t address = readMEMORY(firstAddress) + registers.Y;
	if(address > 0xFF && addCycle)
		++cycles;
	address = (readMEMORY(firstAddress+1) << 8) + address;
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
			CMP(readROM(), registers.AC);
			break;
		}
		case 0xC5: //Zero Page CMP
		{
			cycles = 3;
			CMP(fetchZeroPageAddress(), registers.AC);
			break;
		}
		case 0xD5: //Zero Page,X CMP
		{
			cycles = 4;
			CMP(fetchZeroPageXAddress(), registers.AC);
			break;
		}
		case 0xCD: //Absolute CMP
		{
			cycles = 4;
			CMP(fetchAbsoluteAddress(), registers.AC);
			break;
		}
		case 0xDD: //Absolute,X CMP
		{
			cycles = 4;
			CMP(fetchAbsoluteXAddress(), registers.AC);
			break;
		}
		case 0xD9: //Absolute,Y CMP
		{
			cycles = 4;
			CMP(fetchAbsoluteYAddress(), registers.AC);
			break;
		}
		case 0xC1: //Indirect,X CMP
		{
			cycles = 6;
			CMP(fetchIndirectXAddress(), registers.AC);
			break;
		}
		case 0xD1: //Indirect,Y CMP
		{
			cycles = 5;
			CMP(fetchIndirectYAddress(), registers.AC);
			break;
		}
		case 0xE0: //Immediate CPX
		{
			cycles = 2;
			CMP(readROM(), registers.X);
			break;
		}
		case 0xE4: //Zero Page CPX
		{
			cycles = 3;
			CMP(fetchZeroPageAddress(), registers.X);
			break;
		}
		case 0xEC: //Absolute CPX
		{
			cycles = 4;
			CMP(fetchAbsoluteAddress(), registers.X);
			break;
		}
		case 0xC0: //Immediate CPY
		{
			cycles = 2;
			CMP(readROM(), registers.Y);
			break;
		}
		case 0xC4: //Zero Page CPY
		{
			cycles = 3;
			CMP(fetchZeroPageAddress(), registers.Y);
			break;
		}
		case 0xCC: //Absolute CPY
		{
			cycles = 4;
			CMP(fetchAbsoluteAddress(), registers.Y);
			break;
		}
		case 0xC6: //Zero Page DEC
		{
			cycles = 5;
			DEC(fetchZeroPageAddress());
			break;
		}
		case 0xD6: //Zero Page, X DEC
		{
			cycles = 6;
			DEC(fetchZeroPageXAddress());
			break;
		}
		case 0xCE: //Absolute DEC
		{
			cycles = 6;
			DEC(fetchAbsoluteAddress());
			break;
		}
		case 0xDE: //Absolute,X DEC
		{
			cycles = 7;
			DEC(fetchAbsoluteXAddress(false));
			break;
		}
		case 0xCA: //Implied DEX
		{
			cycles = 2;
			--registers.X;
			set_sign(registers.X);
			set_zero(registers.X);
			break;
		}
		case 0x88: //Implied DEY
		{
			cycles = 2;
			--registers.Y;
			set_sign(registers.Y);
			set_zero(registers.Y);
			break;
		}
		case 0x49: //Immediate EOR
		{
			cycles = 2;
			EOR(readROM());
			break;
		}
		case 0x45: //Zero Page EOR
		{
			cycles = 3;
			EOR(fetchZeroPageAddress());
			break;
		}
		case 0x55: //Zero Page,X EOR
		{
			cycles = 4;
			EOR(fetchZeroPageXAddress());
			break;
		}
		case 0x4D: //Absolute EOR
		{
			cycles = 4;
			EOR(fetchAbsoluteAddress());
			break;
		}
		case 0x5D: //Absolute,X EOR
		{
			cycles = 4;
			EOR(fetchAbsoluteXAddress());
			break;
		}
		case 0x59: //Absolute,Y EOR
		{
			cycles = 4;
			EOR(fetchAbsoluteYAddress());
			break;
		}
		case 0x41: //Indirect,X EOR
		{
			cycles = 6;
			EOR(fetchIndirectXAddress());
			break;
		}
		case 0x51: //Indrect,Y EOR
		{
			cycles = 5;
			EOR(fetchIndirectYAddress());
			break;
		}
		case 0xE6: //Zero Page INC
		{
			cycles = 5;
			INC(fetchZeroPageAddress());
			break;
		}
		case 0xF6: //Zero Page, X INC
		{
			cycles = 6;
			INC(fetchZeroPageXAddress());
			break;
		}
		case 0xEE: //Absolute INC
		{
			cycles = 6;
			INC(fetchAbsoluteAddress());
			break;
		}
		case 0xFE: //Absolute,X INC
		{
			cycles = 7;
			INC(fetchAbsoluteXAddress(false));
			break;
		}
		case 0xE8: //Implied INX
		{
			cycles = 2;
			++registers.X;
			set_sign(registers.X);
			set_zero(registers.X);
			break;
		}
		case 0xC8: //Implied INY
		{
			cycles = 2;
			++registers.Y;
			set_sign(registers.Y);
			set_zero(registers.Y);
			break;
		}
		case 0x4C: //Absolute JMP
		{
			cycles = 3;
			registers.PC = fetchAbsoluteAddress();
			break;
		}
		case 0x6C: //Indirect JMP
		{
			cycles = 5;
			registers.PC = fetchIndirectAddress();
			break;
		}
		case 0x20: //Absolute JSR
		{
			cycles = 6;
			uint16_t address = fetchAbsoluteAddress();
			--registers.PC;
			push((registers.PC >> 8) & 0xFF);
			push(registers.PC & 0xFF);
			registers.PC = address;
			break;
		}
		case 0xA9: //Immediate LDA
		{
			cycles = 2;
			LOAD(readROM(), registers.AC);
			break;
		}
		case 0xA5: //Zero Page LDA
		{
			cycles = 3;
			LOAD(fetchZeroPageAddress(), registers.AC);
			break;
		}
		case 0xB5: //Zero Page,X LDA
		{
			cycles = 4;
			LOAD(fetchZeroPageXAddress(), registers.AC);
			break;
		}
		case 0xAD: //Absolute LDA
		{
			cycles = 4;
			LOAD(fetchAbsoluteAddress(), registers.AC);
			break;
		}
		case 0xBD: //Absolute,X LDA
		{
			cycles = 4;
			LOAD(fetchAbsoluteXAddress(), registers.AC);
			break;
		}
		case 0xB9: //Absolute,Y LDA
		{
			cycles = 4;
			LOAD(fetchAbsoluteYAddress(), registers.AC);
			break;
		}
		case 0xA1: //Indirect,X LDA
		{
			cycles = 6;
			LOAD(fetchIndirectXAddress(), registers.AC);
			break;
		}
		case 0xB1: //Indirect,Y LDA
		{
			cycles = 5;
			LOAD(fetchIndirectYAddress(), registers.AC);
			break;
		}
		case 0xA2: //Immediate LDX
		{
			cycles = 2;
			LOAD(readROM(), registers.X);
			break;
		}
		case 0xA6: //Zero Page LDX
		{
			cycles = 3;
			LOAD(fetchZeroPageAddress(), registers.X);
			break;
		}
		case 0xB6: //Zero Page,Y LDX
		{
			cycles = 4;
			LOAD(fetchZeroPageYAddress(), registers.X);
			break;
		}
		case 0xAE: //Absolute LDX
		{
			cycles = 4;
			LOAD(fetchAbsoluteAddress(), registers.X);
			break;
		}
		case 0xBE: //Absolute,Y LDX
		{
			cycles = 4;
			LOAD(fetchAbsoluteYAddress(), registers.X);
			break;
		}
		case 0xA0: //Immediate LDY
		{
			cycles = 2;
			LOAD(readROM(), registers.Y);
			break;
		}
		case 0xA4: //Zero Page LDY
		{
			cycles = 3;
			LOAD(fetchZeroPageAddress(), registers.Y);
			break;
		}
		case 0xB4: //Zero Page,X LDY
		{
			cycles = 4;
			LOAD(fetchZeroPageYAddress(), registers.Y);
			break;
		}
		case 0xAC: //Absolute LDY
		{
			cycles = 4;
			LOAD(fetchAbsoluteAddress(), registers.Y);
			break;
		}
		case 0xBC: //Absolute,X LDY
		{
			cycles = 4;
			LOAD(fetchAbsoluteYAddress(), registers.Y);
			break;
		}
		case 0x4A: //Accumulator LSR
		{
			cycles = 2;
			LSR(registers.AC);
			break;
		}
		case 0x46: //Zero Page LSR
		{
			cycles = 5;
			LSR(fetchZeroPageAddress());
			break;
		}
		case 0x56: //Zero Page,X LSR
		{
			cycles = 6;
			LSR(fetchZeroPageXAddress());
			break;
		}
		case 0x4E: //Absolute LSR
		{
			cycles = 6;
			LSR(fetchAbsoluteAddress());
			break;
		}
		case 0x5E: //Absolute,X LSR
		{
			cycles = 7;
			LSR(fetchAbsoluteXAddress(false));
			break;
		}
		case 0xEA: //Implied NOP
		{
			cycles = 2;
			break;
		}
		case 0x09: //Immediate ORA
		{
			cycles = 2;
			ORA(readROM());
			break;
		}
		case 0x05: //Zero Page ORA
		{
			cycles = 3;
			ORA(fetchZeroPageAddress());
			break;
		}
		case 0x15: //Zero Page,X ORA
		{
			cycles = 4;
			ORA(fetchZeroPageXAddress());
			break;
		}
		case 0x0D: //Absolute ORA
		{
			cycles = 4;
			ORA(fetchAbsoluteAddress());
			break;
		}
		case 0x1D: //Absolute,X ORA
		{
			cycles = 4;
			ORA(fetchAbsoluteXAddress());
			break;
		}
		case 0x19: //Absolute,Y ORA
		{
			cycles = 4;
			ORA(fetchAbsoluteYAddress());
			break;
		}
		case 0x01: //Indirect,X ORA
		{
			cycles = 6;
			ORA(fetchIndirectXAddress());
			break;
		}
		case 0x11: //Indirect,Y ORA
		{
			cycles = 5;
			ORA(fetchIndirectYAddress(false));
			break;
		}
		case 0x48: //Implied PHA
		{
			cycles = 3;
			push(registers.AC);
			break;
		}
		case 0x08: //Implied PHP
		{
			cycles = 3;
			push(registers.SR);
			break;
		}
		case 0x68: //Implied PLA
		{
			cycles = 4;
			registers.AC = pop();
			set_sign(registers.AC);
			set_zero(registers.AC);
			break;
		}
		case 0x28: //Implied PLP
		{
			cycles = 4;
			registers.SR = pop();
			break;
		}
		case 0x2A: //Accumulator ROL
		{
			cycles = 2;
			ROL(registers.AC);
			break;
		}
		case 0x26: //Zero Page ROL
		{
			cycles = 5;
			ROL(fetchZeroPageAddress());
			break;
		}
		case 0x36: //Zero Page,X ROL
		{
			cycles = 6;
			ROL(fetchZeroPageXAddress());
			break;
		}
		case 0x2E: //Absolute ROL
		{
			cycles = 6;
			ROL(fetchAbsoluteAddress());
			break;
		}
		case 0x3E: //Absolute,X ROL
		{
			cycles = 7;
			ROL(fetchAbsoluteXAddress(false));
			break;
		}
		case 0x6A: //Accumulator ROR
		{
			cycles = 2;
			ROR(registers.AC);
			break;
		}
		case 0x66: //Zero Page ROR
		{
			cycles = 5;
			ROR(fetchZeroPageAddress());
			break;
		}
		case 0x76: //Zero Page,X ROR
		{
			cycles = 6;
			ROR(fetchZeroPageXAddress());
			break;
		}
		case 0x6E: //Absolute ROR
		{
			cycles = 6;
			ROR(fetchAbsoluteAddress());
			break;
		}
		case 0x7E: //Absolute,X ROR
		{
			cycles = 7;
			ROR(fetchAbsoluteXAddress(false));
			break;
		}
		case 0x40: //Implied RTI
		{
			cycles = 6;
			uint8_t sr = pop();
			registers.SR = sr;
			uint8_t lowByte = pop();
			uint8_t highByte = pop();
			uint16_t pc = (highByte << 8) + lowByte;
			registers.PC = pc;
			break;
		}
		case 0x60: //Implied RTS
		{
			cycles = 6;
			uint8_t lowByte = pop();
			uint8_t highByte = pop();
			uint16_t pc = (highByte << 8) + lowByte;
			++pc;
			registers.PC = pc;
			break;
		}
		case 0xE9: //Immediate SBC
		{
			cycles = 2;
			SBC(readROM());
			break;
		}
		case 0xE5: //Zero Page SBC
		{
			cycles = 3;
			SBC(fetchZeroPageAddress());
			break;
		}
		case 0xF5: //Zero Page,X SBC
		{
			cycles = 4;
			SBC(fetchZeroPageXAddress());
			break;
		}
		case 0xED: //Absolute SBC
		{
			cycles = 4;
			SBC(fetchAbsoluteAddress());
			break;
		}
		case 0xFD: //Absolute,X SBC
		{
			cycles = 4;
			SBC(fetchAbsoluteXAddress());
			break;
		}
		case 0xF9: //Absolute,Y SBC
		{
			cycles = 4;
			SBC(fetchAbsoluteYAddress());
			break;
		}
		case 0xE1: //Indirect,X SBC
		{
			cycles = 6;
			SBC(fetchIndirectXAddress());
			break;
		}
		case 0xF1: //Indirect,Y SBC
		{
			cycles = 5;
			SBC(fetchIndirectYAddress(false));
			break;
		}
		case 0x38: //Implied SEC
		{
			cycles = 2;
			set_carry(1);
			break;
		}
		case 0xF8: //Implied SED
		{
			cycles = 2;
			set_decimal(1);
			break;
		}
		case 0x78: //Implied SEI
		{
			cycles = 2;
			set_interrupt(1);
			break;
		}
		case 0x85: //Zero Page STA
		{
			cycles = 3;
			STORE(fetchZeroPageAddress(), registers.AC);
			break;
		}
		case 0x95: //Zero Page,X STA
		{
			cycles = 4;
			STORE(fetchZeroPageXAddress(), registers.AC);
			break;
		}
		case 0x80: //Absolute STA
		{
			cycles = 4;
			STORE(fetchAbsoluteAddress(), registers.AC);
			break;
		}
		case 0x9D: //Absolute,X STA
		{
			cycles = 5;
			STORE(fetchAbsoluteXAddress(false), registers.AC);
			break;
		}
		case 0x99: //Absolute,Y STA
		{
			cycles = 5;
			STORE(fetchAbsoluteYAddress(false), registers.AC);
			break;
		}
		case 0x81: //Indirect,X STA
		{
			cycles = 6;
			STORE(fetchIndirectXAddress(), registers.AC);
			break;
		}
		case 0x91: //Indirect,Y STA
		{
			cycles = 6;
			STORE(fetchIndirectYAddress(false), registers.AC);
			break;
		}
		case 0x86: //Zero Page STX
		{
			cycles = 3;
			STORE(fetchZeroPageAddress(), registers.X);
			break;
		}
		case 0x96: //Zero Page,Y STX
		{
			cycles = 4;
			STORE(fetchZeroPageYAddress(), registers.X);
			break;
		}
		case 0x8E: //Absolute STX
		{
			cycles = 4;
			STORE(fetchAbsoluteAddress(), registers.X);
			break;
		}
		case 0x84: //Zero Page STY
		{
			cycles = 3;
			STORE(fetchZeroPageAddress(), registers.Y);
			break;
		}
		case 0x94: //Zero Page,Y STY
		{
			cycles = 4;
			STORE(fetchZeroPageYAddress(), registers.Y);
			break;
		}
		case 0x8C: //Absolute STY
		{
			cycles = 4;
			STORE(fetchAbsoluteAddress(), registers.Y);
			break;
		}
		case 0xAA: //Implied TAX
		{
			cycles = 2;
			set_sign(registers.AC);
			set_zero(registers.AC);
			registers.X = registers.AC;
			break;
		}
		case 0xA8: //Implied TAY
		{
			cycles = 2;
			set_sign(registers.AC);
			set_zero(registers.AC);
			registers.Y = registers.AC;
			break;
		}
		case 0xBA: //Implied TSX
		{
			cycles = 2;
			set_sign(registers.SP);
			set_zero(registers.SP);
			registers.X = registers.SP;
			break;
		}
		case 0x8A: //Implied TXA
		{
			cycles = 2;
			set_sign(registers.X);
			set_zero(registers.X);
			registers.AC = registers.X;
			break;
		}
		case 0x9A: //Implied TXS
		{
			cycles = 2;
			registers.SP = registers.X;
			break;
		}
		case 0x98: //Implied TYA
		{
			cycles = 2;
			set_sign(registers.Y);
			set_zero(registers.Y);
			registers.AC = registers.Y;
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

void CPU::CMP(uint8_t operand, uint8_t regValue)
{
	uint16_t result = regValue - operand;
	set_carry(result < 0x100);
	set_sign(result);
	set_zero(result & 0xFF);
}

void CPU::CMP(uint16_t operandAddress, uint8_t regValue)
{
	uint8_t operand = readMEMORY(operandAddress);
	uint16_t result = regValue - operand;
	set_carry(result < 0x100);
	set_sign(result);
	set_zero(result & 0xFF);
}

void CPU::DEC(uint16_t operandAddress)
{
	//TODO: test without creating 16 bit operand
	int16_t operand = readMEMORY(operandAddress);
	--operand;
	uint8_t newValue = operand & 0xFF;
	set_sign(newValue);
	set_zero(newValue);
	writeMEMORY(operandAddress, newValue);
}

void CPU::EOR(uint8_t operand)
{
	operand ^= registers.AC;
	set_sign(operand);
	set_zero(operand);
	registers.AC = operand;
}

void CPU::EOR(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	operand ^= registers.AC;
	set_sign(operand);
	set_zero(operand);
	registers.AC = operand;
}

void CPU::INC(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	++operand;
	set_sign(operand);
	set_zero(operand);
	writeMEMORY(operandAddress, operand);
}

void CPU::LOAD(uint8_t operand, uint8_t& reg)
{
	set_sign(operand);
	set_zero(operand);
	reg = operand;
}

void CPU::LOAD(uint16_t operandAddress, uint8_t& reg)
{
	uint8_t operand = readMEMORY(operandAddress);
	set_sign(operand);
	set_zero(operand);
	reg = operand;
}

void CPU::LSR(uint8_t operand)
{
	set_carry(operand & 0x01);
	operand >>= 1;
	set_sign(operand);
	set_zero(operand);
	registers.AC = operand;
}

void CPU::LSR(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	set_carry(operand & 0x01);
	operand >>= 1;
	set_sign(operand);
	set_zero(operand);
	writeMEMORY(operandAddress, operand);
}

void CPU::ORA(uint8_t operand)
{
	operand |= registers.AC;
	set_sign(operand);
	set_zero(operand);
	registers.AC = operand;
}

void CPU::ORA(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	operand |= registers.AC;
	set_sign(operand);
	set_zero(operand);
	registers.AC = operand;
}

void CPU::ROL(uint8_t operand)
{
	uint16_t temp = operand;
	temp <<= 1;
	if(if_carry())
		temp |= 0x01;
	set_carry(temp > 0xFF);
	operand = temp & 0xFF;
	set_sign(operand);
	set_zero(operand);
	registers.AC = operand;
}

void CPU::ROL(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	uint16_t temp = operand;
	temp <<= 1;
	if(if_carry())
		temp |= 0x01;
	set_carry(temp > 0xFF);
	operand = temp & 0xFF;
	set_sign(operand);
	set_zero(operand);
	writeMEMORY(operandAddress, operand);
}

void CPU::ROR(uint8_t operand)
{
	uint16_t temp = operand;
	if(if_carry())
		temp |= 0x100;
	set_carry(temp & 0x01);
	temp >>= 1;
	operand = temp & 0xFF;
	set_sign(operand);
	set_zero(operand);
	registers.AC = operand;
}

void CPU::ROR(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	uint16_t temp = operand;
	if(if_carry())
		temp |= 0x100;
	set_carry(temp & 0x01);
	temp >>= 1;
	operand = temp & 0xFF;
	set_sign(operand);
	set_zero(operand);
	writeMEMORY(operandAddress, operand);
}

void CPU::SBC(uint8_t operand)
{
	uint16_t temp = registers.AC - operand - (if_carry() ? 0 : 1);
	set_sign(temp);
	set_zero(temp & 0xFF);
	set_overflow(((registers.AC ^ temp) & 0x80) && ((registers.AC ^ operand) & 0x80));
	set_carry(temp < 0x100);
	registers.AC = (temp & 0xFF);
}

void CPU::SBC(uint16_t operandAddress)
{
	uint8_t operand = readMEMORY(operandAddress);
	uint16_t temp = registers.AC - operand - (if_carry() ? 0 : 1);
	set_sign(temp);
	set_zero(temp & 0xFF);
	set_overflow(((registers.AC ^ temp) & 0x80) && ((registers.AC ^ operand) & 0x80));
	set_carry(temp < 0x100);
	registers.AC = (temp & 0xFF);
}

void CPU::STORE(uint16_t operandAddress, uint8_t regValue)
{
	writeMEMORY(operandAddress, regValue);
} 

void CPU::CPU_TESTING()
{
}