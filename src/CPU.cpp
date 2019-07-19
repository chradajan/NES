#include "include/CPU.hpp"

CPU::CPU(Mapper& map, PPU_Registers& ppu, APU_IO_Registers& apu_io) 
: mapper(map), ppu_registers(ppu), apu_io_registers(apu_io)
{
	cpu_registers.SR = 0x34;
	cpu_registers.AC = 0;
	cpu_registers.X = 0;
	cpu_registers.Y = 0;
	cpu_registers.SP = 0xFD;

	write(0x4017, 0x00);
	write(0x4015, 0x00);
	for(uint16_t i = 0x4000; i < 0x4010; ++i)
		write(i, 0x00);
}

void CPU::reset()
{

}

void CPU::tick()
{

}

CPU::~CPU()
{

}

uint8_t CPU::read(uint16_t address)
{
	if(address < 0x2000) //Internal RAM
		return RAM[address % 0x0800];
	else if(address < 0x4000) //PPU registers
		return ppu_registers.read(address);
	else if(address < 0x4018) //APU or I/O Registers
		return apu_io_registers.read(address);
	else if(address < 0x4020) //Disabled APU and I/O Functionality
		throw Unsupported("CPU Test Mode Disabled");
	else //Cartridge Space
		return mapper.readPRG(address);
}

void CPU::write(uint16_t address, uint8_t data)
{
	if(address < 0x2000) //Internal RAM
		RAM[address % 0x0800] = data;
	else if(address < 0x4000) //PPU registers
		ppu_registers.write(address, data);
	else if(address == 0x4014) //Trigger DMA Transfer
	{
		DMA_Transfer = true;
		apu_io_registers.write(address, data);
	}
	else if(address < 0x4018) //APU or I/O Registers
		apu_io_registers.write(address);
	else if(address < 0x4020) //Disabled APU and I/O Functionality
		throw Unsupported("CPU Test Mode Disabled");
	else //Cartridge Space
		mapper.writePRG(address);
}

uint8_t CPU::pop()
{
	++cpu_registers.SP;
	return read(cpu_registers.SP);
}

void CPU::push(uint8_t data)
{
	write(cpu_registers.SP, data);
	--cpu_registers.SP;
}

uint8_t CPU::getByte()
{
	return read(cpu_registers.PC++);
}

bool CPU::if_carry()
{
	return cpu_registers.SR & 0x1;
}

bool CPU::if_overflow()
{
	return (cpu_registers.SR >> 6) & 0x1;
}

bool CPU::if_sign()
{
	return (cpu_registers.SR >> 7) & 0x1;
}

bool CPU::if_zero()
{
	return (cpu_registers.SR >> 1) & 0x1;
}

void CPU::set_carry(bool condition)
{
	if(condition)
		cpu_registers.SR |= 0x1;
	else
		cpu_registers.SR &= ~(0x1);
}

void CPU::set_zero(uint16_t value)
{
	if(value == 0)
		cpu_registers.SR |= 0x1 << 1;
	else
		cpu_registers.SR &= ~(0x1 << 1);
}

void CPU::set_interrupt(bool condition)
{
	if(condition)
		cpu_registers.SR |= 0x1 << 2;
	else
		cpu_registers.SR &= ~(0x1 << 2);
}

void CPU::set_decimal(bool condition)
{
	if(condition)
		cpu_registers.SR |= 0x1 << 3;
	else
		cpu_registers.SR &= ~(0x1 << 3);
}

void CPU::set_break(bool condition)
{
	if(condition)
		registers.SR |= 0x1 << 4;
	else
		registers.SR &= ~(0x1 << 4);
}

void CPU::set_overflow(bool condition)
{
	if(condition)
		cpu_registers.SR |= 0x1 << 6;
	else
		cpu_registers.SR &= ~(0x1 << 6);
}

void CPU::set_sign(uint16_t value)
{
	cpu_registers.SR |= ((value >> 7) & 0x1) << 7;
}