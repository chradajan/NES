#ifndef MAPPER_H
#define MAPPER_H
#include <cstdint>

class Mapper
{
public:
	virtual void writePRG(uint16_t address, uint8_t data) = 0;
	virtual uint8_t readPRG(uint16_t address) = 0;
	virtual void writeCHR(uint16_t address, uint8_t data) = 0;
	virtual uint8_t readCHR(uint16_t address) = 0;
	~Mapper() {}
};

#endif