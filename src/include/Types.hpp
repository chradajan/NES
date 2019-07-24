#ifndef TYPES_H
#define TYPES_H

#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <iomanip>

enum AddressingMode
{
	ACCUMULATOR,
	IMMEDIATE,
	IMPLIED,
	ABSOLUTE,
	ZEROPAGE,
	ABSOLUTEINDEXED,
	ZEROPAGEINDEXED,
	INDIRECT,
	PREINDEXEDINDIRECT,
	POSTINDEXEDINDIRECT,
	RELATIVE,
	ABSOLUTEJMP
};

struct DebugInfo
{
	std::string OPCode;
	AddressingMode mode;
	uint8_t OPCodeHex, AC, X, Y, SP, P, memoryValue;
	uint16_t PC, address, firstByte, secondByte;
	int ppuCycle, ppuScanline, cpuCycle;
	std::string indexString;

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
		log << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (uint)PC << "  ";
		log << std::setw(2) << (uint)OPCodeHex << " ";

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

		log << readableInstruction();
		log << "A:" << std::setw(2) << (uint)AC << " ";
		log << "X:" << std::setw(2) << (uint)X << " ";
		log << "Y:" << std::setw(2) << (uint)Y << " ";
		log << "P:" << std::setw(2) << (uint)P << " ";
		log << "SP:" << std::setw(2) << (uint)SP << " ";
		log << "PPU:" << std::setfill(' ') << std::setw(3) << std::dec << ppuCycle << ",";
		log << std::setw(3) << ppuScanline << " ";
		log << "CYC:" << cpuCycle << std::endl;
	}
private:
	bool writeFirstByte = true;
	std::string readableInstruction()
	{
		std::stringstream ss;
		ss << OPCode << std::hex << std::uppercase << std::setfill('0');
		switch(mode)
		{
			case ACCUMULATOR:
				ss << " A";
				break;
			case IMMEDIATE:
				ss << " #$" << std::setw(2) << (uint)firstByte;
				break;
			case IMPLIED:
				break;
			case ABSOLUTE:
				ss << " $" << std::setw(4) << (uint)address << " = " << std::setw(2) << (uint)memoryValue;
				break;
			case ZEROPAGE:
				ss << " $" << std::setw(2) << (uint)address << " = " << std::setw(2) << (uint)memoryValue;
				break;
			case ZEROPAGEINDEXED:
			case ABSOLUTEINDEXED:
				ss << " $" << std::setw(2) << (uint)secondByte << std::setw(2) << (uint)firstByte << "," << indexString << " @ ";
				ss << (uint)(address) << " = " << std::setw(2) << (uint)memoryValue;
				break;
			case ABSOLUTEJMP:
				ss << " $" << std::setw(2) << (uint)secondByte << std::setw(2) << (uint)firstByte;
				break;
			case PREINDEXEDINDIRECT:
				ss << " ($" << std::setw(2) << (uint)firstByte << ",X) @ " << std::setw(2) << (uint)((firstByte + X) & 0xFF);
				ss << " = " << std::setw(4) << (uint)address << " = " << std::setw(2) << (uint)memoryValue;
				break;
			case POSTINDEXEDINDIRECT:
				break;
			case RELATIVE:
				ss << " $" << std::setw(4) << (uint)address;
				break;
			default:
				break;
		}

		int remainingSpaces = ss.str().length();

		for(int i = 0; i < 32 - remainingSpaces; ++i)
			ss << " ";

		return ss.str();
	}
};

struct HeaderData
{
	uint8_t PRG_ROM_SIZE;
	uint8_t CHR_ROM_SIZE;
	uint8_t Flags6, Flags7, Flags8, Flags9, Flags10;
};

struct PPU_Registers
{
	uint8_t PPUCTRL;
	uint8_t PPUMASK;
	uint8_t PPUSTATUS;
	uint8_t OAMADDR;
	uint8_t OAMDATA;
	uint8_t PPUSTROLL;
	uint8_t PPUADDR;
	uint8_t PPUDATA;

	//For cpu debugging
	int cycle;
	int scanline;

	uint8_t read(uint16_t address) const
	{
		address = (address < 0x2008 ? address : address - ((address / 0x0008) * 0x0008) + 0x2000); 
		switch(address)
		{
			case 0x2000:
				return PPUCTRL;
			case 0x2001:
				return PPUMASK;
			case 0x2002:
				return PPUSTATUS;
			case 0x2003:
				return OAMADDR;
			case 0x2004:
				return OAMDATA;
			case 0x2005:
				return PPUSTROLL;
			case 0x2006:
				return PPUADDR;
			case 0x2007:
				return PPUDATA;
		}
		//This shouldn't happen
		return 0x00;
	}

	void write(uint16_t address, uint8_t data)
	{
		address = (address < 0x2008 ? address : address - ((address / 0x0008) * 0x0008) + 0x2000);
		switch(address)
		{
			case 0x2000:
				PPUCTRL = data;
				break;
			case 0x2001:
				PPUMASK = data;
				break;
			case 0x2002:
				PPUSTATUS = data;
				break;
			case 0x2003:
				OAMADDR = data;
				break;
			case 0x2004:
				OAMDATA = data;
				++OAMADDR;
				break;
			case 0x2005:
				PPUSTROLL = data;
				break;
			case 0x2006:
				PPUADDR = data;
				break;
			case 0x2007:
				PPUDATA = data;
				break;
		}
	}
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

#endif