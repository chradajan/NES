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
	cpu_registers.X = 0x00;
	cpu_registers.PC = 0x8000;
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

	debug();
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

void CPU::implied(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			read(cpu_registers.PC); //Dummy read
			break;
		case 2:
			executeInstruction();
			currentOP = readROM();
			cycleCount = 0;
	}
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
			addressBus = readROM();
			break;
		case 2:
			addressBus = (readROM() << 8) + addressBus;
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

void CPU::absoluteIndexed(std::function<void()> executeInstruction, const uint8_t& index)
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

uint16_t CPU::relativeAddress(uint8_t offset)
{
	bool isNegative = (offset & 0x80) >> 7;
	if(isNegative)
	{
		offset &= 0x7F;
		return cpu_registers.PC - offset;
	}
	return cpu_registers.PC + offset;
}

void CPU::relative(bool condition)
{
	switch(cycleCount)
	{
		case 1:
			dataBus = readROM();
			addressBus = relativeAddress(dataBus);
			break;
		case 2:
			if(!condition)
			{
				currentOP = readROM();
				cycleCount = 0;
			}
			else if((cpu_registers.PC & 0xFF00) == (addressBus & 0xFF00)) //Same page
			{
				cpu_registers.PC = addressBus;
				currentOP = readROM();
				cycleCount = 0;
			}
			else
				read((cpu_registers.PC & 0xFF00) + (addressBus & 0xFF)); //Dummy read
			break;
		case 3:
			cpu_registers.PC = addressBus;
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::accumulator(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			read(cpu_registers.PC); //Dummy read
			dataBus = cpu_registers.AC;
			break;
		case 2:	//TODO: test timing since ac might be modified 1 cycle later than it is here
			executeInstruction();
			cpu_registers.AC = dataBus;
			currentOP = readROM();
			break;
	}
}

void CPU::zeroPage_RMW(std::function<void()> executeInstruction)
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
			write(addressBus, 0x00); //Write is performed here while data is being modified
			break;
		case 4:
			executeInstruction();
			write(addressBus, dataBus);
			break;
		case 5:
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::zeroPageX_RMW(std::function<void()> executeInstruction)
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
			write(addressBus, 0x00); //Write is performed here while data is being modified
			break;
		case 5:
			executeInstruction();
			write(addressBus, dataBus);
			break;
		case 6:
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::absolute_RMW(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			addressBus = readROM();
			break;
		case 2:
			dataBus = readROM();
			break;
		case 3:
			addressBus = (addressBus << 8) + dataBus;
			dataBus = read(addressBus);
			break;
		case 4:
			write(addressBus, 0x00); //Write is performed here while data is being modified
			break;
		case 5:
			executeInstruction();
			write(addressBus, dataBus);
			break;
		case 6:
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::absoluteX_RMW(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			dataBus = readROM();
			break;
		case 2:
			addressBus = readROM();
			break;
		case 3:
			read((addressBus << 8) + ((dataBus + cpu_registers.X) & 0xFF)); //Dummy read
			addressBus = (addressBus << 8) + dataBus;
			break;
		case 4:
			addressBus += cpu_registers.X; 
			dataBus = read(addressBus);
			break;
		case 5:
			write(addressBus, 0x00); //Write is performed here while data is being modified
			break;
		case 6:
			executeInstruction();
			write(addressBus, dataBus);
			break;
		case 7:
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

void CPU::ASL()
{
	set_carry(dataBus & 0x80);
	dataBus <<= 1;
	set_sign(dataBus);
	set_zero(dataBus);
}

void CPU::BIT()
{
	set_sign(dataBus);
	set_overflow(0x40 & dataBus);
	set_zero(dataBus & cpu_registers.AC);
}

void CPU::BRK()
{
	switch(cycleCount)
	{
		case 1:
			readROM(); //Absorb padding byte after BRK
			break;
		case 2:
			push(cpu_registers.PC >> 8);
			break;
		case 3:
			push(cpu_registers.PC & 0xFF);
			break;
		case 4:
			push(cpu_registers.SR);
			break;
		case 5:
			addressBus = read(0xFFFE);
			break;
		case 6:
			addressBus = (read(0xFFFF) << 8) + addressBus;
			break;
		case 7:
			cpu_registers.PC = addressBus;
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::CMP(const uint8_t& regValue)
{
	uint16_t temp = regValue - dataBus;
	set_carry(temp < 0x100);
	set_sign(temp);
	set_zero(temp & 0xFF);
}

void CPU::DEC()
{
	uint16_t temp = (dataBus - 1) & 0xFF;
	dataBus = temp;
	set_sign(dataBus);
	set_zero(dataBus);
}

void CPU::DEX()
{
	uint16_t temp = (cpu_registers.X - 1) & 0xFF;
	set_sign(temp);
	set_zero(temp);
	cpu_registers.X = temp;
}

void CPU::DEY()
{
	uint16_t temp = (cpu_registers.Y - 1) & 0xFF;
	set_sign(temp);
	set_zero(temp);
	cpu_registers.Y = temp;
}

void CPU::EOR()
{
	cpu_registers.AC ^= dataBus;
	set_sign(cpu_registers.AC);
	set_zero(cpu_registers.AC);
}

void CPU::INC()
{
	uint16_t temp = (dataBus + 1) & 0xFF;
	dataBus = temp;
	set_sign(dataBus);
	set_zero(dataBus);
}

void CPU::INX()
{
	uint16_t temp = (cpu_registers.X + 1) & 0xFF;
	set_sign(temp);
	set_zero(temp);
	cpu_registers.X = temp;
}

void CPU::INY()
{
	uint16_t temp = (cpu_registers.Y + 1) & 0xFF;
	set_sign(temp);
	set_zero(temp);
	cpu_registers.Y = temp;
}

void CPU::absoluteJMP()
{
	switch(cycleCount)
	{
		case 1:
			addressBus = readROM();
			break;
		case 2:
			addressBus = (readROM() << 8) + addressBus;
			break;
		case 3:
			cpu_registers.PC = addressBus;
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::indirectJMP()
{
	switch(cycleCount)
	{
		case 1:
			addressBus = readROM();
			break;
		case 2:
			addressBus = (readROM() << 8) + addressBus;
			break;
		case 3:
			dataBus = read(addressBus);
			break;
		case 4:
			addressBus = read(addressBus + 1) + dataBus;
			break;
		case 5:
			cpu_registers.PC = addressBus;
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::JSR()
{
	switch(cycleCount)
	{
		case 1:
			addressBus = readROM();
			break;
		case 2:
			read(cpu_registers.SP); //Dummy read
			break;
		case 3:
			push(cpu_registers.PC >> 8);
			break;
		case 4:
			push(cpu_registers.PC & 0xFF);
			break;
		case 5:
			addressBus = (readROM() << 8) + addressBus;
			break;
		case 6:
			cpu_registers.PC = addressBus;
			currentOP = readROM();
			cycleCount = 0;
	}
}

void CPU::decodeOP()
{
	std::function<void()> executeInstruction;
	switch(currentOP)
	{
		case 0x69:	//Immediate ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x65: //Zero Page ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x75: //Zero Page,X ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::zeroPageX, this, executeInstruction);
			break;
		case 0x6D: //Absolute ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x7D: //Absolute,X ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x79: //Absolute,Y ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0x61: //Indirect,X ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x71: //Indirect,Y ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0x29: //Immediate AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x25: //Zero Page AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x35: //Zero Page,X AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::zeroPageX, this, executeInstruction);
			break;
		case 0x2D: //Absolute AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x3D: //Absolute,X AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x39: //Absolute,Y AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0x21: //Indirect,X AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x31: //Indirect,Y AND
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;

		case 0x0A: //Accumulator ASL
			executeInstruction = std::bind(&CPU::ASL, this);
			addressingMode = std::bind(&CPU::accumulator, this, executeInstruction);
			break;
		case 0x06: //Zero Page ASL
			executeInstruction = std::bind(&CPU::ASL, this);
			addressingMode = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0x16: //Zero Page,X ASL
			executeInstruction = std::bind(&CPU::ASL, this);
			addressingMode = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0x0E: //Absolute ASL
			executeInstruction = std::bind(&CPU::ASL, this);
			addressingMode = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0x1E: //Absolute,X ASL
			executeInstruction = std::bind(&CPU::ASL, this);
			addressingMode = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0x90: //Relative BCC
			addressingMode = std::bind(&CPU::relative, this, !if_carry());
			break;
		case 0xB0: //Relative BCS
			addressingMode = std::bind(&CPU::relative, this, if_carry());
			break;
		case 0xF0: //Relative BEQ
			addressingMode = std::bind(&CPU::relative, this, if_zero());
			break;
		case 0x24: //Zero Page BIT
			executeInstruction = std::bind(&CPU::BIT, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x2C: //Absolute BIT
			executeInstruction = std::bind(&CPU::BIT, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x30: //Relative BMI
			addressingMode = std::bind(&CPU::relative, this, if_sign());
			break;
		case 0xD0: //Relative BNE
			addressingMode = std::bind(&CPU::relative, this, !if_zero());
			break;
		case 0x10: //Relative BPL
			addressingMode = std::bind(&CPU::relative, this, !if_sign());
			break;
		case 0x00: //Implied BRK
			addressingMode = std::bind(&CPU::BRK, this);
			break;
		case 0x50: //Relative BVC
			addressingMode = std::bind(&CPU::relative, this, !if_overflow());
			break;
		case 0x70: //Relative BVS
			addressingMode = std::bind(&CPU::relative, this, if_overflow());
			break;
		case 0x18: //Implied CLC
			executeInstruction = std::bind(&CPU::set_carry, this, 0);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xD8: //Implied CLD
			executeInstruction = std::bind(&CPU::set_decimal, this, 0);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x58: //Implied CLI
			executeInstruction = std::bind(&CPU::set_interrupt, this, 0);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xB8: //Implied CLV
			executeInstruction = std::bind(&CPU::set_overflow, this, 0);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xC9: //Immediate CMP
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xC5: //Zero Page CMP
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xD5: //Zero Page,X CMP
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::zeroPageX, this, executeInstruction);
			break;
		case 0xCD: //Absolute CMP
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xDD: //Absolute,X CMP
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0xD9: //Absolute,Y CMP
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0xC1: //Indirect,X CMP
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0xD1: //Indirect,Y CMP
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0xE0: //Immediate CPX
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.X);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xE4: //Zero Page CPX
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.X);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xEC: //Absolute CPX
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.X);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xC0: //Immediate CPY
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.Y);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xC4: //Zero Page CPY
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.Y);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xCC: //Absolute CPY
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.Y);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xC6: //Zero Page DEC
			executeInstruction = std::bind(&CPU::DEC, this);
			addressingMode = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0xD6: //Zero Page, X DEC
			executeInstruction = std::bind(&CPU::DEC, this);
			addressingMode = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0xCE: //Absolute DEC
			executeInstruction = std::bind(&CPU::DEC, this);
			addressingMode = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0xDE: //Absolute,X DEC
			executeInstruction = std::bind(&CPU::DEC, this);
			addressingMode = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0xCA: //Implied DEX
			executeInstruction = std::bind(&CPU::DEX, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x88: //Implied DEY
			executeInstruction = std::bind(&CPU::DEY, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x49: //Immediate EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x45: //Zero Page EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x55: //Zero Page,X EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::zeroPageX, this, executeInstruction);
			break;
		case 0x4D: //Absolute EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x5D: //Absolute,X EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x59: //Absolute,Y EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0x41: //Indirect,X EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x51: //Indrect,Y EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0xE6: //Zero Page INC
			executeInstruction = std::bind(&CPU::INC, this);
			addressingMode = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0xF6: //Zero Page, X INC
			executeInstruction = std::bind(&CPU::INC, this);
			addressingMode = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0xEE: //Absolute INC
			executeInstruction = std::bind(&CPU::INC, this);
			addressingMode = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0xFE: //Absolute,X INC
			executeInstruction = std::bind(&CPU::INC, this);
			addressingMode = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0xE8: //Implied INX
			executeInstruction = std::bind(&CPU::INX, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xC8: //Implied INY
			executeInstruction = std::bind(&CPU::INY, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x4C: //Absolute JMP
			addressingMode = std::bind(&CPU::absoluteJMP, this);
			break;
		case 0x6C: //Indirect JMP
			addressingMode = std::bind(&CPU::indirectJMP, this);
			break;
		case 0x20: //Absolute JSR
			addressingMode = std::bind(&CPU::JSR, this);
			break;
		case 0xA9: //Immediate LDA
		case 0xA5: //Zero Page LDA
		case 0xB5: //Zero Page,X LDA
		case 0xAD: //Absolute LDA
		case 0xBD: //Absolute,X LDA
		case 0xB9: //Absolute,Y LDA
		case 0xA1: //Indirect,X LDA
		case 0xB1: //Indirect,Y LDA
		case 0xA2: //Immediate LDX
		case 0xA6: //Zero Page LDX
		case 0xB6: //Zero Page,Y LDX
		case 0xAE: //Absolute LDX
		case 0xBE: //Absolute,Y LDX
		case 0xA0: //Immediate LDY
		case 0xA4: //Zero Page LDY
		case 0xB4: //Zero Page,X LDY
		case 0xAC: //Absolute LDY
		case 0xBC: //Absolute,X LDY
		case 0x4A: //Accumulator LSR
		case 0x46: //Zero Page LSR
		case 0x56: //Zero Page,X LSR
		case 0x4E: //Absolute LSR
		case 0x5E: //Absolute,X LSR
		case 0xEA: //Implied NOP
		case 0x09: //Immediate ORA
		case 0x05: //Zero Page ORA
		case 0x15: //Zero Page,X ORA
		case 0x0D: //Absolute ORA
		case 0x1D: //Absolute,X ORA
		case 0x19: //Absolute,Y ORA
		case 0x01: //Indirect,X ORA
		case 0x11: //Indirect,Y ORA
		case 0x48: //Implied PHA
		case 0x08: //Implied PHP
		case 0x68: //Implied PLA
		case 0x28: //Implied PLP
		case 0x2A: //Accumulator ROL
		case 0x26: //Zero Page ROL
		case 0x36: //Zero Page,X ROL
		case 0x2E: //Absolute ROL
		case 0x3E: //Absolute,X ROL
		case 0x6A: //Accumulator ROR
		case 0x66: //Zero Page ROR
		case 0x76: //Zero Page,X ROR
		case 0x6E: //Absolute ROR
		case 0x7E: //Absolute,X ROR
		case 0x40: //Implied RTI
		case 0x60: //Implied RTS
		case 0xE9: //Immediate SBC
		case 0xE5: //Zero Page SBC
		case 0xF5: //Zero Page,X SBC
		case 0xED: //Absolute SBC
		case 0xFD: //Absolute,X SBC
		case 0xF9: //Absolute,Y SBC
		case 0xE1: //Indirect,X SBC
		case 0xF1: //Indirect,Y SBC
		case 0x38: //Implied SEC
		case 0xF8: //Implied SED
		case 0x78: //Implied SEI
		case 0x85: //Zero Page STA
		case 0x95: //Zero Page,X STA
		case 0x80: //Absolute STA
		case 0x9D: //Absolute,X STA
		case 0x99: //Absolute,Y STA
		case 0x81: //Indirect,X STA
		case 0x91: //Indirect,Y STA
		case 0x86: //Zero Page STX
		case 0x96: //Zero Page,Y STX
		case 0x8E: //Absolute STX
		case 0x84: //Zero Page STY
		case 0x94: //Zero Page,Y STY
		case 0x8C: //Absolute STY
		case 0xAA: //Implied TAX
		case 0xA8: //Implied TAY
		case 0xBA: //Implied TSX
		case 0x8A: //Implied TXA
		case 0x9A: //Implied TXS
		case 0x98: //Implied TYA
			break;
	}
	addressingMode();
}

void CPU::debug()
{
	std::cout << "PC: $" << std::hex << std::setfill('0') << std::setw(4) << (uint)cpu_registers.PC << "   ";
	std::cout << "OP_Code: $" << std::setw(2) << std::setfill('0') << (uint)currentOP << "   ";
	std::cout << "Cycle: " << cycleCount << "   ";
	std::cout << "AC: $" << std::setw(2) << std::setfill('0') << (uint)cpu_registers.AC << "   ";
	std::cout << "X: $" << std::setw(2) << std::setfill('0') << (uint)cpu_registers.X << "   ";
	std::cout << "Y: $" << std::setw(2) << std::setfill('0') << (uint)cpu_registers.Y << "   ";
	std::cout << "SP: $" << std::setw(2) << std::setfill('0') << (uint)cpu_registers.SP << "   ";
	std::cout << "SR: $" << std::setw(2) << std::setfill('0') << (uint)cpu_registers.SR << std::endl;
}