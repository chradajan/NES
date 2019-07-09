#ifndef CPU_H
#define CPU_H
#include <cstdint>

class CPU
{	
public:
	CPU(const char* file);
	void CPU_TESTING();

private:
	struct CPU_Registers
	{
		uint8_t A;
		uint8_t X;	
		uint8_t Y;
		uint16_t PC;
		uint8_t S;
		uint8_t P;
	};

	CPU_Registers* registers;
	uint8_t memory[0xFFFF];

	void loadROM(const char* file);
	uint8_t convertAscii(uint8_t c);
};

#endif