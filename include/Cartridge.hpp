#ifndef CARTRIDGE_H
#define CARTRIDGE_H
#include <cstdint>
#include <fstream>

enum class MirrorType {HORIZONTAL, VERTICAL, SINGLE, QUAD};

class Cartridge
{
public:
	virtual uint8_t readPRG(uint16_t address) = 0;
	virtual void writePRG(uint16_t address, uint8_t data) = 0;
	virtual uint8_t readCHR(uint16_t address) = 0;
	virtual void writeCHR(uint16_t address, uint8_t data) = 0;
	virtual uint16_t nametableAddress(uint16_t addr) = 0;
	virtual ~Cartridge() {}
protected:
	MirrorType mirroringType;
	virtual void loadROM(std::ifstream& rom) = 0;
};

#endif