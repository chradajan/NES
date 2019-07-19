#ifndef NROM_H
#define NROM_H
#include <cstdint>
#include "Mapper.hpp"

class NROM : public Mapper
{
public:
	NROM(uint8_t PRG_ROM_MULTIPLIER);
	void writePRG(uint16_t address, uint8_t data);
	uint8_t readPRG(uint16_t address);
	void writeCHR(uint16_t address, uint8_t data);
	uint8_t readCHR(uint16_t address);
	~NROM();
private:
	uint8_t* PRG_ROM;
	uint8_t* CHR_ROM;
	bool PRG_Mirroring;
};

#endif