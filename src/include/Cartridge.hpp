#ifndef CARTRIDGE_H
#define CARTRIDGE_H
#include <cstdint>
#include <fstream>
#include "Types.hpp"

class Cartridge
{
public:
	virtual uint8_t readPRG(uint16_t address) = 0;
	virtual void writePRG(uint16_t address, uint8_t data) = 0;
	virtual uint8_t readCHR(uint16_t address) = 0;
	virtual void writeCHR(uint16_t address, uint8_t data) = 0;
	virtual Mirroring nametableMirroring() const = 0;
	virtual ~Cartridge() {}
protected:
	Mirroring mirroringType;
	virtual void loadROM(std::ifstream& rom) = 0;
};

#endif