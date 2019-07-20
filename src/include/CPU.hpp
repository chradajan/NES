#ifndef CPU_H
#define CPU_H
#include <cstdint>
#include "../mappers/Mapper.hpp"
#include "Types.hpp"

class CPU
{
public:
	CPU(Mapper* map, PPU_Registers& ppu_reg, APU_IO_Registers& apu_io_reg);
	void reset();
	void tick();
	~CPU();

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

	Mapper* mapper;
	CPU_Registers cpu_registers;
	PPU_Registers& ppu_registers;
	APU_IO_Registers& apu_io_registers;
	uint8_t RAM[0x0800];
	bool oddCycle;

	//Buses
	uint8_t dataBus;
	uint16_t addressBus;

	//Read/Write
	uint8_t read(uint16_t address) const;
	void write(uint16_t address, uint8_t data);
	uint8_t pop();
	void push(uint8_t data);
	bool DMA_Transfer;

	//Status Register
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

	//Addressing
	void immediate();
};

#endif