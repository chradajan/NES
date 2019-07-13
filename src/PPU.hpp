#ifndef PPU_H
#define PPU_H
#include <cstdint>

class PPU
{
public:
	PPU();
	void tick();
	~PPU();

	void PPU_TESTING();
private:
	uint8_t memory[0x4000];

	//Read/Write
	uint8_t readMEMORY(uint16_t address);
	void writeMEMORY(uint16_t address, uint8_t data);
};

#endif