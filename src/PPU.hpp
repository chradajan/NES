#ifndef PPU_H
#define PPU_H
#include <cstdint>
#include <fstream>

class PPU
{
public:
	PPU(std::ifstream& rom);
	void tick();
	~PPU();

	void PPU_TESTING();
private:
	uint8_t memory[0x4000];

	//ROM Loading
	void loadROM(std::ifstream& rom);
	void loadNROM(std::ifstream& rom);

	//Read/Write
	uint8_t readMEMORY(uint16_t address);
	void writeMEMORY(uint16_t address, uint8_t data);
};

#endif