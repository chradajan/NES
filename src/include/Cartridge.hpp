#ifndef CARTRIDGE_H
#define CARTRIDGE_H
#include <cstdint>
#include <fstream>

class Cartridge
{
public:
	virtual uint8_t readPRG(uint16_t address) const = 0;
	virtual void writePRG(uint16_t address, uint8_t data) = 0;
	virtual uint8_t readCHR(uint16_t address) const = 0;
	virtual void writeCHR(uint16_t address, uint8_t data) = 0;
	virtual bool verticalMirroring() const = 0;
	virtual ~Cartridge() {}
protected:
	virtual void loadROM(std::ifstream& rom) = 0;
	bool mirroringIsVertical;
};

#endif