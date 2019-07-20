#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <iostream>
#include <exception>
#include <string>
#include <cstdint>

class BadRom : virtual public std::exception
{
private:
	char unrecognizedChar;
	std::string errorMessage;
public:
	explicit BadRom(char c);
	virtual const char* what() const throw()
	{
		return errorMessage.c_str();
	}
};

BadRom::BadRom(char c) : unrecognizedChar(c)
{
	errorMessage = "";
	errorMessage += c;
}

class Unsupported : virtual public std::exception
{
private:
	std::string errorMessage;
public:
	explicit Unsupported(std::string message) : errorMessage(message) {}
	virtual const char* what() const throw()
	{
		return errorMessage.c_str();
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