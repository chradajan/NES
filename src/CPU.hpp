#ifndef CPU_H
#define CPU_H
#include <cstdint>

class CPU
{	
public:
	CPU(const char* file);
	void tick();
	~CPU();

	void CPU_TESTING();

private:

	short unsigned int cycles;

	struct CPU_Registers
	{
		uint8_t A;		//Accumulator
		uint8_t X;		//X
		uint8_t Y;		//Y
		uint16_t PC;	//Program Counter
		uint8_t S;		//Stack Pointer
		uint8_t P;		//Status
	};

	CPU_Registers* registers;
	uint8_t memory[0xFFFF];

	void loadROM(const char* file);
	uint8_t convertAscii(uint8_t c);

	//Used to read/set status register bits
	bool if_carry();
	bool if_overflow();
	bool if_sign();
	bool if_zero();
	void set_sign(uint16_t value);
	void set_zero(uint16_t value);
	void set_carry(bool condition);
	void set_overflow(bool condition);
	void set_interrupt(bool condition);
	void set_break(bool condition);

	void executeInstruction();

	//Instructions
	void ADC(uint16_t operand);
};

#endif