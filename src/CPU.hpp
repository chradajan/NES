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
		uint8_t AC;		//Accumulator
		uint8_t X;		//X
		uint8_t Y;		//Y
		uint16_t PC;	//Program Counter
		uint8_t SP;		//Stack Pointer
		uint8_t SR;		//Status
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
	uint8_t pop();
	void push(uint8_t);

	//Used to read/set status register bits
	bool if_carry();
	bool if_overflow();
	bool if_sign();
	bool if_zero();
	void set_carry(bool condition);
	void set_zero(uint16_t value);
	void set_interrupt(bool condition);
	void set_decimal(bool condition);
	void set_break(bool condition);
	void set_overflow(bool condition);
	void set_sign(uint16_t value);

	//Fetch operand address
	uint16_t fetchImmediateAddress();
	uint16_t fetchZeroPageAddress();
	uint16_t fetchZeroPageXAddress();
	uint16_t fetchZeroPageYAddress();
	uint16_t fetchAbsoluteAddress();
	uint16_t fetchAbsoluteXAddress(bool addCycle);
	uint16_t fetchAbsoluteYAddress(bool addCycle);
	uint16_t fetchIndirectAddress();
	uint16_t fetchIndirectXAddress();
	uint16_t fetchIndirectYAddress(bool addCycle);

	void executeInstruction();

	//Utility
	uint16_t relativeAddress(uint8_t offset);

	//Instructions
	void ADC(uint8_t operand);
	void ADC(uint16_t operandAddress);
	void AND(uint8_t operand);
	void AND(uint16_t operandAddress);
	void ASL(uint8_t operand);
	void ASL(uint16_t operandAddress);
	void BIT(uint16_t operandAddress);
	void BRANCH(bool condition);
	void CMP(uint8_t operand, uint8_t regValue);
	void CMP(uint16_t operandAddress, uint8_t regValue);
	void DEC(uint16_t operandAddress);
	void EOR(uint8_t operand);
	void EOR(uint16_t operandAddress);
	void INC(uint16_t operandAddress);
	void LOAD(uint8_t operand, uint8_t& reg);
	void LOAD(uint16_t operandAddress, uint8_t& reg);
	void LSR(uint8_t operand);
	void LSR(uint16_t operandAddress);
	void ORA(uint8_t operand);
	void ORA(uint16_t operandAddress);
	void ROL(uint8_t operand);
	void ROL(uint16_t operandAddress);
	void ROR(uint8_t operand);
	void ROR(uint16_t operandAddress);
	void SBC(uint8_t operand);
	void SBC(uint16_t operandAddress);
	void STORE(uint16_t operandAddress, uint8_t regValue);
};

#endif