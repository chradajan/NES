#include "include/CPU.hpp"

CPU::CPU(Mapper* map, PPU_Registers& ppu_reg, APU_IO_Registers& apu_io_reg) 
: mapper(map), ppu_registers(ppu_reg), apu_io_registers(apu_io_reg)
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
	//TODO: set noise channel

	dataBus = 0x00;
	addressBus = 0x0000;

	//Testing
	write(0x0669, 0xED);
	cpu_registers.PC = 0x8000;
	cpu_registers.AC = 0x7B;
	cpu_registers.X = 0x0F;
	cycleCount = -1;
	oddCycle = true;
	//End Testing

	// cpu_registers.PC = Reset_Vector();
	// //Probably do something else here eventually, but this works for now
	// currentOP = readROM();
	// cycleCount = -1;
	// oddCycle = true;
}

void CPU::reset()
{

}

void CPU::tick()
{
	oddCycle = !oddCycle;

	++cycleCount;

	if(cycleCount == 0)
		currentOP = readROM();
	else if(cycleCount == 1)
		decodeOP();
	else
		addressingMode();
}

CPU::~CPU()
{

}

uint8_t CPU::read(uint16_t address) const
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
		return mapper->readPRG(address);
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
		apu_io_registers.write(address, data);
	else if(address < 0x4020) //Disabled APU and I/O Functionality
		throw Unsupported("CPU Test Mode Disabled");
	else //Cartridge Space
		mapper->writePRG(address, data);
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

uint8_t CPU::readROM()
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
		cpu_registers.SR |= 0x1 << 4;
	else
		cpu_registers.SR &= ~(0x1 << 4);
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

uint16_t CPU::NMI_Vector()
{
	uint8_t lowByte = read(0xFFFA);
	uint8_t highByte = read(0xFFFB);
	uint16_t vector = (highByte << 8) + lowByte;
	return vector;
}

uint16_t CPU::Reset_Vector()
{
	uint8_t lowByte = read(0xFFFC);
	uint8_t highByte = read(0xFFFD);
	uint16_t vector = (highByte << 8) + lowByte;
	return vector;
}

uint16_t CPU::IRQ_BRK_Vector()
{
	uint8_t lowByte = read(0xFFFE);
	uint8_t highByte = read(0xFFFF);
	uint16_t vector = (highByte << 8) + lowByte;
	return vector;
}

void CPU::immediate(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			dataBus = readROM();
			break;
		case 2:
			executeInstruction();
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::zeroPage(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			addressBus = readROM();
			break;
		case 2:
			dataBus = read(addressBus);
			break;
		case 3:
			executeInstruction();
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::zeroPageX(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			addressBus = dataBus = readROM();
			break;
		case 2:
			read(addressBus); //Dummy read
			dataBus += cpu_registers.X;
			addressBus = dataBus;
			break;
		case 3:
			dataBus = read(addressBus);
			break;
		case 4:
			executeInstruction();
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::absolute(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			addressBus = dataBus = readROM();
			break;
		case 2:
			dataBus = readROM();
			break;
		case 3:
			addressBus = (addressBus << 8) + dataBus;
			dataBus = read(addressBus);
			break;
		case 4:
			executeInstruction();
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::absoluteIndexed(std::function<void()> executeInstruction, uint8_t& index)
{
	switch(cycleCount)
	{
		case 1:
			dataBus = readROM();
			break;
		case 2:
			addressBus = dataBus + index;
			dataBus = readROM();
			break;
		case 3:
			if(addressBus <= 0xFF)
			{
				addressBus = (dataBus << 8) + addressBus;
				dataBus = read(addressBus);
				++cycleCount; //Skip next step since page boundary is not crossed
			}
			else
				read((dataBus << 8) + (addressBus & 0xFF)); //Dummy read since page boundary crossed
			break;
		case 4:
			addressBus = (dataBus << 8) + addressBus;
			dataBus = read(addressBus);
			break;
		case 5:
			executeInstruction();
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::indirectX(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			addressBus = dataBus = readROM();
			break;
		case 2:
			read(addressBus); //Dummy read
			dataBus += cpu_registers.X;
			addressBus = dataBus;
			break;
		case 3:
			dataBus = read(addressBus);
			break;
		case 4:
			addressBus = (read(addressBus + 1) << 8) + dataBus;
			break;
		case 5:
			dataBus = read(addressBus);
			break;
		case 6:
			executeInstruction();
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::indirectY(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			addressBus = readROM();
			break;
		case 2:
			dataBus = read(addressBus);
			break;
		case 3:
			addressBus = (read(addressBus + 1) << 8) + dataBus;
			break;
		case 4:
			if(dataBus + cpu_registers.Y <= 0xFF)
			{
				addressBus += cpu_registers.Y;
				dataBus = read(addressBus);
				++cycleCount; //Skip next step since page boundary is not crossed
			}
			else
			{
				read((addressBus & 0xFF00) + ((addressBus + cpu_registers.Y) & 0xFF)); //Dummy read since page boundary crossed
			}
			break;
		case 5:
			addressBus += cpu_registers.Y;
			dataBus = read(addressBus);
			break;
		case 6:
			executeInstruction();
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::ADC()
{
	uint16_t temp = dataBus + cpu_registers.AC + (if_carry() ? 1 : 0);
	set_zero(temp & 0xFF);
	set_sign(temp);
	set_overflow(!((cpu_registers.AC ^ dataBus) & 0x80) && ((cpu_registers.AC ^ temp) & 0x80));
	set_carry(temp > 0xFF);
	cpu_registers.AC = temp & 0xFF;
}

void CPU::AND()
{
	dataBus &= cpu_registers.AC;
	set_sign(dataBus);
	set_zero(dataBus);
	cpu_registers.AC = dataBus;
}

void CPU::decodeOP()
{
	std::function<void()> executeInstruction;
	switch(currentOP)
	{
		case 0x69:	//Immediate ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			addressingMode();
			break;
		case 0x65: //Zero Page ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			addressingMode();
			break;
		case 0x75: //Zero Page,X ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::zeroPageX, this, executeInstruction);
			addressingMode();
			break;
		case 0x6D: //Absolute ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			addressingMode();
			break;
		case 0x7D: //Absolute,X ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			addressingMode();
			break;
		case 0x79: //Absolute,Y ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			addressingMode();
			break;
		case 0x61: //Indirect,X ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			addressingMode();
			break;
		case 0x71: //Indirect,Y ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			addressingMode();
			break;
		case 0x29: //Immediate AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			addressingMode();
			break;
		case 0x25: //Zero Page AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			addressingMode();
			break;
		case 0x35: //Zero Page,X AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::zeroPageX, this, executeInstruction);
			addressingMode();
			break;
		case 0x2D: //Absolute AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			addressingMode();
			break;
		case 0x3D: //Absolute,X AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			addressingMode();
			break;
		case 0x39: //Absolute,Y AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			addressingMode();
			break;
		case 0x21: //Indirect,X AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			addressingMode();
			break;
		case 0x31: //Indirect,Y AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			addressingMode();
			break;

		default:
			break;
	}
}

void CPU::printRegisters()
{
	std::cout << std::hex << "AC: " << (uint)cpu_registers.AC << std::endl;
	std::cout << "X:  " << (uint)cpu_registers.X << std::endl;
	std::cout << "Y:  " << (uint)cpu_registers.Y << std::endl;
	std::cout << "PC: " << (uint)cpu_registers.PC << std::endl;
	std::cout << "SP: " << (uint)cpu_registers.SP << std::endl;
	std::cout << "SR: " << (uint)cpu_registers.SR << std::endl;
	std::cout << "Address: " << (uint)addressBus << std::endl;
	std::cout << "Data: " << (uint)dataBus << std::endl;
	std::cout << std::dec << "Cycle Count: " << cycleCount << "\n\n" << std::endl;

}