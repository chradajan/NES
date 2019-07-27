#ifndef TYPES_H
#define TYPES_H

#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <iomanip>

#define uint unsigned int //Mingw doesn't recognize uintg

struct HeaderData
{
	uint8_t PRG_ROM_SIZE;
	uint8_t CHR_ROM_SIZE;
	uint8_t Flags6, Flags7, Flags8, Flags9, Flags10;
};

struct CPU_Registers
{
	uint8_t AC;		//Accumulator
	uint8_t X;		//X
	uint8_t Y;		//Y
	uint16_t PC;	//Program Counter
	uint8_t SP;		//Stack Pointer
	uint8_t SR;		//Status
};

struct APU_IO_Registers
{
	uint8_t SQ1_VOL;
	uint8_t SQ1_SWEEP;
	uint8_t SQ1_LO;
	uint8_t SQ1_HI;
	uint8_t SQ2_VOL;
	uint8_t SQ2_SWEEP;
	uint8_t SQ2_LO;
	uint8_t SQ2_HI;
	uint8_t TRI_LINEAR;
	uint8_t UNUSED1;
	uint8_t TRI_LO;
	uint8_t TRI_HI;
	uint8_t NOISE_VOL;
	uint8_t UNUSED2;
	uint8_t NOISE_LO;
	uint8_t NOISE_HI;
	uint8_t DMC_FREQ;
	uint8_t DMC_RAW;
	uint8_t DMC_START;
	uint8_t DMC_LEN;
	uint8_t OAMDMA;
	uint8_t SND_CHN;
	uint8_t JOY1;
	uint8_t JOY2;

	uint8_t read(uint16_t address) const
	{
		switch(address)
		{
			case 0x4000:
				return SQ1_VOL;
			case 0x4001:
				return SQ1_SWEEP;
			case 0x4002:
				return SQ1_LO;
			case 0x4003:
				return SQ1_HI;
			case 0x4004:
				return SQ2_VOL;
			case 0x4005:
				return SQ2_SWEEP;
			case 0x4006:
				return SQ2_LO;
			case 0x4007:
				return SQ2_HI;
			case 0x4008:
				return TRI_LINEAR;
			case 0x4009:
				return UNUSED1;
			case 0x400A:
				return TRI_LO;
			case 0x400B:
				return TRI_HI;
			case 0x400C:
				return NOISE_VOL;
			case 0x400D:
				return UNUSED2;
			case 0x400E:
				return NOISE_LO;
			case 0x400F:
				return NOISE_HI;
			case 0x4010:
				return DMC_FREQ;
			case 0x4011:
				return DMC_RAW;
			case 0x4012:
				return DMC_START;
			case 0x4013:
				return DMC_LEN;
			case 0x4014:
				return OAMDMA;
			case 0x4015:
				return SND_CHN;
			case 0x4016:
				return JOY1;
			case 0x4017:
				return JOY2;
		}
		//This shouldn't happen
		return 0x00;
	}

	void write(uint16_t address, uint8_t data)
	{
		switch(address)
		{
			case 0x4000:
				SQ1_VOL = data;
				break;
			case 0x4001:
				SQ1_SWEEP = data;
				break;
			case 0x4002:
				SQ1_LO = data;
				break;
			case 0x4003:
				SQ1_HI = data;
				break;
			case 0x4004:
				SQ2_VOL = data;
				break;
			case 0x4005:
				SQ2_SWEEP = data;
				break;
			case 0x4006:
				SQ2_LO = data;
				break;
			case 0x4007:
				SQ2_HI = data;
				break;
			case 0x4008:
				TRI_LINEAR = data;
				break;
			case 0x4009:
				UNUSED1 = data;
				break;
			case 0x400A:
				TRI_LO = data;
				break;
			case 0x400B:
				TRI_HI = data;
				break;
			case 0x400C:
				NOISE_VOL = data;
				break;
			case 0x400D:
				UNUSED2 = data;
				break;
			case 0x400E:
				NOISE_LO = data;
				break;
			case 0x400F:
				NOISE_HI = data;
				break;
			case 0x4010:
				DMC_FREQ = data;
				break;
			case 0x4011:
				DMC_RAW = data;
				break;
			case 0x4012:
				DMC_START = data;
				break;
			case 0x4013:
				DMC_LEN = data;
				break;
			case 0x4014:
				OAMDMA = data;
				break;
			case 0x4015:
				SND_CHN = data;
				break;
			case 0x4016:
				JOY1 = data;
				break;
			case 0x4017:
				JOY2 = data;
				break;
		}
	}
};

struct DebugInfo
{
	void setInfo(uint8_t opcode, const CPU_Registers& cpu_reg, int cycles)
	{
		OPCode = opcode;
		PC = cpu_reg.PC - 1;
		AC = cpu_reg.AC;
		X = cpu_reg.X;
		Y = cpu_reg.Y;
		SP = cpu_reg.SP;
		P = cpu_reg.SR;
		cycle = cycles;
		firstByte = secondByte = 0xFFFF;
		writeFirstByte = true;
	}
	void add(uint8_t data)
	{
		if(writeFirstByte)
		{
			firstByte = data;
			writeFirstByte = false;
		}
		else
		{
			secondByte = data;
			writeFirstByte = true;
		}
	}
	void resetBytes()
	{
		writeFirstByte = true;
		firstByte = 0xFFFF;
		secondByte = 0xFFFF;
	}
	void print(std::fstream& log)
	{
		// if(cycle == 0)
		// 	return;

		log << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (uint)PC << "  ";
		log << std::setw(2) << (uint)OPCode << " ";

		if(firstByte <= 0xFF)
		{
			log << std::setw(2) << (uint)(firstByte & 0xFF) << " ";
			if(secondByte <= 0xFF)
				log << std::setw(2) << (uint)secondByte << "  ";
			else
				log << "    ";
		}
		else
		{
			log << "       ";
		}

		log << "A:" << std::setw(2) << (uint)AC << " ";
		log << "X:" << std::setw(2) << (uint)X << " ";
		log << "Y:" << std::setw(2) << (uint)Y << " ";
		log << "P:" << std::setw(2) << (uint)P << " ";
		log << "SP:" << std::setw(2) << (uint)SP << " ";
		log << "CYC:" << std::dec << cycle << std::endl;
	}
private:
	bool writeFirstByte;
	uint8_t OPCode, AC, X, Y, SP, P;
	uint16_t PC, firstByte, secondByte;
	int cycle;
};

#endif