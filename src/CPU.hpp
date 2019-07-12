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

	CPU_Registers registers;
	uint8_t memory[0xFFFF];

	void loadROM(const char* file);
	uint8_t convertAscii(uint8_t c);

	//Mapper
	uint16_t mapPC();

	//Read/Write
	uint8_t readMEMORY(uint16_t address);
	void writeMEMORY(uint16_t address, uint8_t data);
	uint8_t readROM();

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

	//Fetch operand address
	uint16_t fetchImmediateAddress();
	uint16_t fetchZeroPageAddress();
	uint16_t fetchZeroPageXAddress();
	uint16_t fetchZeroPageYAddress();
	uint16_t fetchAbsoluteAddress();
	uint16_t fetchAbsoluteXAddress();
	uint16_t fetchAbsoluteYAddress();
	uint16_t fetchIndirectXAddress();
	uint16_t fetchIndirectYAddress();

	void executeInstruction();

	//Utility
	uint16_t relativeAddress(int8_t )

	//Instructions
	void ADC(uint8_t operand);
	void ADC(uint16_t operandAddress);
	void AND(uint16_t operandAddress);
	void ASL(uint16_t operandAddress);
	void BCC(int8_t operand);
};

#endif