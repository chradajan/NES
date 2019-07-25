#ifndef CPU_H
#define CPU_H
#include <cstdint>
#include <functional>
#include <iostream>
#include <fstream>
#include <string>
#include "Cartridge.hpp"
#include "Types.hpp"
#include "Exceptions.hpp"
#include "PPU.hpp"

class CPU
{
public:
	CPU(Cartridge* cart, PPU_Registers& ppu_reg, APU_IO_Registers& apu_io_reg, std::fstream& cpuLog);
	void reset();
	void tick();
	~CPU();

	//Debug
	void debug();

private:
	struct CPU_Registers
	{
		uint8_t AC;		//Accumulator
		uint8_t X;		//X
		uint8_t Y;		//Y
		uint16_t PC;	//Program Counter
		uint8_t SP;		//Stack Pointer
		uint8_t SR;		//Status
	};

	Cartridge* cart;
	CPU_Registers cpu_registers;
	PPU_Registers& ppu_registers;
	APU_IO_Registers& apu_io_registers;
	uint8_t RAM[0x0800];

	//State
	bool oddCycle;
	uint8_t currentOP;
	int cycleCount = 0;
	uint8_t dataBus = 0x00;
	uint16_t addressBus = 0x0000;
	std::function<void()> addressingMode;
	
	//Debug
	DebugInfo debugInfo;
	bool debugEnabled;
	std::fstream& log;
	int totalCycles = 0;

	//DMA Transfer
	bool dmaTransfer = false;
	int dmaTransferCycles;
	uint16_t dmaPage;
	uint8_t dmaLowByte;
	uint8_t dmaData;
	void executeDMATransfer();

	//Read/Write
	uint8_t read(uint16_t address) const;
	void write(uint16_t address, uint8_t data);
	uint8_t pop();
	void push(uint8_t data);
	void readOPCode();
	uint8_t readROM();

	//Status Register
	bool if_carry();
	bool if_overflow();
	bool if_sign();
	bool if_zero();
	void set_carry(bool condition);
	void set_zero(uint8_t value);
	void set_interrupt(bool condition);
	void set_decimal(bool condition);
	void set_break(bool condition);
	void set_overflow(bool condition);
	void set_sign(uint8_t value);

	//Vectors
	void NMI_Vector();
	void Reset_Vector();
	void IRQ_BRK_Vector();

	void NMI();

	//Addressing
	void implied(std::function<void()> executeInstruction);
	void immediate(std::function<void()> executeInstruction);
	void zeroPage(std::function<void()> executeInstruction);
	void zeroPageIndexed(std::function<void()> executeInstruction, const uint8_t& index);
	void absolute(std::function<void()> executeInstruction);
	void absoluteIndexed(std::function<void()> executeInstruction, const uint8_t& index);
	void indirectX(std::function<void()> executeInstruction);
	void indirectY(std::function<void()> executeInstruction);
	uint16_t relativeAddress(uint8_t offset);
	void relative(bool condition);
	void zeroPage_Store(const uint8_t& regValue);
	void zeroPageIndexed_Store(const uint8_t& regValue, const uint8_t& index);
	void absolute_Store(const uint8_t& regValue);
	void absoluteIndexed_Store(const uint8_t& regValue, const uint8_t& index);
	void indirectX_Store(const uint8_t& regValue);
	void indirectY_Store(const uint8_t& regValue);

	//Read-Modify-Write Addressing
	void accumulator(std::function<void()> executeInstruction);
	void zeroPage_RMW(std::function<void()> executeInstruction);
	void zeroPageX_RMW(std::function<void()> executeInstruction);
	void absolute_RMW(std::function<void()> executeInstruction);
	void absoluteX_RMW(std::function<void()> executeInstruction);

	//Instructions
	void ADC();
	void AND();
	void ASL();
	void BIT();
	void BRK();
	void CLR();
	void CMP(const uint8_t& regValue);
	void DEC();
	void DEX();
	void DEY();
	void EOR();
	void INC();
	void INX();
	void INY();
	void absoluteJMP();
	void indirectJMP();
	void JSR();
	void LDA();
	void LDX();
	void LDY();
	void LSR();
	void ORA();
	void PHA();
	void PHP();
	void PLA();
	void PLP();
	void ROL();
	void ROR();
	void RTI();
	void RTS();
	void SBC();
	void TAX();
	void TAY();
	void TSX();
	void TXA();
	void TXS();
	void TYA();
	
	//Execution
	void decodeOP();
};

#endif