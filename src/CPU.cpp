#include "include/CPU.hpp"

//TODO: Implement IRQ and NMI interrupts. Implement OMA transfer

CPU::CPU(Mapper* map, PPU_Registers& ppu_reg, APU_IO_Registers& apu_io_reg, std::fstream& cpuLog) 
: mapper(map), ppu_registers(ppu_reg), apu_io_registers(apu_io_reg), log(cpuLog)
{
	//cpu_registers.SR = 0x34;
	cpu_registers.SR = 0x24; //For nestest
	cpu_registers.AC = 0;
	cpu_registers.X = 0;
	cpu_registers.Y = 0;
	cpu_registers.SP = 0xFD;

	write(0x4017, 0x00);
	write(0x4015, 0x00);
	for(uint16_t i = 0x4000; i < 0x4010; ++i)
		write(i, 0x00);
	for(uint16_t i = 0x0000; i < 0x2000; ++i)
		write(i, 0x00);
	//TODO: set noise channel

	cycleCount = 0;
	totalCycles = 4;
	oddCycle = true;
	debugEnabled = true;
}

void CPU::reset()
{
	//TODO: add reset conditions
}

void CPU::tick()
{
	oddCycle = !oddCycle;

	++cycleCount;
	++totalCycles;

	if(dmaTransfer)
		executeDMATransfer();
	else if(totalCycles < 8)
		Reset_Vector();
	else if(cycleCount == 1)
		decodeOP();
	else
		addressingMode();
}

CPU::~CPU() {}

void CPU::executeDMATransfer()
{
	if(dmaTransferCycles == 514)
		read(cpu_registers.PC); //Dummy read
	else if(oddCycle && dmaTransferCycles == 513)
		return;
	else if(dmaTransferCycles == 513)
		read(cpu_registers.PC); //Dummy read
	else if(dmaTransferCycles % 2 == 0)
		dmaData = read(dmaPage + dmaLowByte++);
	else if(dmaTransferCycles % 2 == 1)
		write(0x2004, dmaData);

	--dmaTransferCycles;
	if(dmaTransferCycles == 0)
		dmaTransfer = false;
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
		dmaTransfer = true;
		dmaPage = (data << 8);
		dmaLowByte = 0x00;
		dmaTransferCycles = 513 + (oddCycle ? 1 : 0);
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
	return read(0x0100 + cpu_registers.SP);
}

void CPU::push(uint8_t data)
{
	write(0x0100 + cpu_registers.SP, data);
	--cpu_registers.SP;
}

void CPU::readOPCode()
{
	if(debugEnabled)
	{
		debug();
		debugInfo.PC = cpu_registers.PC;
		currentOP = read(cpu_registers.PC++);
		debugInfo.OPCodeHex = currentOP;
		debugInfo.AC = cpu_registers.AC;
		debugInfo.X = cpu_registers.X;
		debugInfo.Y = cpu_registers.Y;
		debugInfo.SP = cpu_registers.SP;
		debugInfo.P = cpu_registers.SR;
		debugInfo.ppuCycle = ppu_registers.cycle;
		debugInfo.ppuScanline = ppu_registers.scanline;
		debugInfo.cpuCycle = totalCycles;
		debugInfo.resetBytes();
	}
	else
		currentOP = read(cpu_registers.PC++);
	cycleCount = 0;
}

uint8_t CPU::readROM()
{
	uint8_t temp = read(cpu_registers.PC++);
	if(debugEnabled)
	{
		debugInfo.add(temp);
	}
	return temp;
}

bool CPU::if_carry()
{
	return cpu_registers.SR & 0x01;
}

bool CPU::if_overflow()
{
	return (cpu_registers.SR >> 6) & 0x01;
}

bool CPU::if_sign()
{
	return (cpu_registers.SR >> 7) & 0x01;
}

bool CPU::if_zero()
{
	return (cpu_registers.SR >> 1) & 0x01;
}

void CPU::set_carry(bool condition)
{
	if(condition)
		cpu_registers.SR |= 0x01;
	else
		cpu_registers.SR &= ~(0x01);
}

void CPU::set_zero(uint8_t value)
{
	if(value == 0)
		cpu_registers.SR |= 0x01 << 1;
	else
		cpu_registers.SR &= ~(0x01 << 1);
}

void CPU::set_interrupt(bool condition)
{
	if(condition)
		cpu_registers.SR |= 0x01 << 2;
	else
		cpu_registers.SR &= ~(0x01 << 2);
}

void CPU::set_decimal(bool condition)
{
	if(condition)
		cpu_registers.SR |= 0x01 << 3;
	else
		cpu_registers.SR &= ~(0x01 << 3);
}

void CPU::set_break(bool condition)
{
	if(condition)
		cpu_registers.SR |= 0x01 << 4;
	else
		cpu_registers.SR &= ~(0x01 << 4);
}

void CPU::set_overflow(bool condition)
{
	if(condition)
		cpu_registers.SR |= 0x01 << 6;
	else
		cpu_registers.SR &= ~(0x01 << 6);
}

void CPU::set_sign(uint8_t value)
{
	if(value >= 0x80)
		cpu_registers.SR |= 0x01 << 7;
	else
		cpu_registers.SR &= ~(0x01 << 7);
}

void CPU::NMI_Vector()
{
	switch(cycleCount)
	{
		case 1:
			addressBus = read(0xFFFA);
			break;
		case 2:
			addressBus = (read(0xFFFB) << 8) + addressBus;
			break;
		case 3:
			cpu_registers.PC = addressBus;
			readOPCode();
	}
}

void CPU::Reset_Vector() //TODO:fix this
{
	switch(cycleCount)
	{
		case 1:
			addressBus = read(0xFFFC);
			break;
		case 2:
			addressBus = (read(0xFFFD) << 8) + addressBus;
			break;
		case 3:
			cpu_registers.PC = addressBus - 4;
			ppu_registers.cycle = 0;
			ppu_registers.scanline = 0;
			readOPCode();
	}
}

void CPU::IRQ_BRK_Vector()
{
	switch(cycleCount)
	{
		case 1:
			addressBus = read(0xFFFE);
			break;
		case 2:
			addressBus = (read(0xFFFF) << 8) + addressBus;
			break;
		case 3:
			cpu_registers.PC = addressBus;
			readOPCode();
	}
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
			readOPCode();
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
			readOPCode();
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
			readOPCode();
	}
}

void CPU::zeroPageIndexed(std::function<void()> executeInstruction, const uint8_t& index)
{
	switch(cycleCount)
	{
		case 1:
			dataBus = readROM();
			break;
		case 2:
			read(dataBus); //Dummy read
			dataBus += index;
			addressBus = dataBus;
			break;
		case 3:
			dataBus = read(addressBus);
			break;
		case 4:
			executeInstruction();
			readOPCode();
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
			readOPCode();
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
			readOPCode();
	}
}

void CPU::indirectX(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			dataBus = readROM();
			break;
		case 2:
			read(dataBus); //Dummy read
			dataBus += cpu_registers.X;
			break;
		case 3:
			addressBus = read(dataBus);
			++dataBus;
			break;
		case 4:
			addressBus = (read(dataBus) << 8) + addressBus;
			break;
		case 5:
			dataBus = read(addressBus);
			break;
		case 6:
			executeInstruction();
			readOPCode();
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
			readOPCode();
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
			if(debugEnabled) debugInfo.address = addressBus;
			break;
		case 2:
			if(!condition)
				readOPCode();
			else
				read((cpu_registers.PC & 0xFF00) + (addressBus & 0xFF)); //Dummy read
			break;
		case 3:
			if((cpu_registers.PC & 0xFF00) != (addressBus & 0xFF00))
				read(addressBus); //Dummy read
			else
			{
				cpu_registers.PC = addressBus;
				readOPCode();
			}
			break;
		case 4:
			cpu_registers.PC = addressBus;
			readOPCode();
	}
}

void CPU::zeroPage_Store(const uint8_t& regValue)
{
	switch(cycleCount)
	{
		case 1:
			addressBus = readROM();
			break;
		case 2:
			if(debugEnabled) { debugInfo.memoryValue = read(addressBus); debugInfo.address = addressBus; };
			write(addressBus, regValue);
			break;
		case 3:
			readOPCode();
	}
}

void CPU::zeroPageIndexed_Store(const uint8_t& regValue, const uint8_t& index)
{
	switch(cycleCount)
	{
		case 1:
			addressBus = dataBus = readROM();
			break;
		case 2:
			read(addressBus); //Dummy read
			dataBus += index;
			addressBus = dataBus;
			break;
		case 3:
			if(debugEnabled) { debugInfo.memoryValue = read(addressBus); debugInfo.address = addressBus; };
			write(addressBus, regValue);
			break;
		case 4:
			readOPCode();
	}
}

void CPU::absolute_Store(const uint8_t& regValue)
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
			if(debugEnabled) { debugInfo.memoryValue = read(addressBus); debugInfo.address = addressBus; };
			write(addressBus, regValue);
			break;
		case 4:
			readOPCode();
	}
}

void CPU::absoluteIndexed_Store(const uint8_t& regValue, const uint8_t& index)
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
			read((addressBus & 0xFF00) + ((addressBus + index) & 0xFF)); //Dummy read
			addressBus += index;
			break;
		case 4:
			if(debugEnabled) { debugInfo.memoryValue = read(addressBus); debugInfo.address = addressBus; };
			write(addressBus, regValue);
			break;
		case 5:
			readOPCode();
	}
}

void CPU::indirectX_Store(const uint8_t& regValue)
{
	switch(cycleCount)
	{
		case 1:
			dataBus = readROM();
			break;
		case 2:
			read(dataBus); //Dummy read
			dataBus += cpu_registers.X;
			break;
		case 3:
			addressBus = read(dataBus);
			++dataBus;
			break;
		case 4:
			addressBus = (read(dataBus) << 8) + addressBus;
			break;
		case 5:
			if(debugEnabled) { debugInfo.memoryValue = read(addressBus); debugInfo.address = addressBus; };
			write(addressBus, regValue);
			break;
		case 6:
			readOPCode();
	}
}

void CPU::indirectY_Store(const uint8_t& regValue)
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
			read((addressBus & 0xFF00) + ((addressBus + cpu_registers.Y) & 0xFF)); //Dummy read
			addressBus += cpu_registers.Y;
			break;
		case 5:
			if(debugEnabled) { debugInfo.memoryValue = read(addressBus); debugInfo.address = addressBus; };
			write(addressBus, regValue);
			break;
		case 6:
			readOPCode();
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
		case 2:
			executeInstruction();
			cpu_registers.AC = dataBus;
			readOPCode();
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
			write(addressBus, 0xFF); //Write is performed here while data is being modified
			break;
		case 4:
			executeInstruction();
			write(addressBus, dataBus);
			break;
		case 5:
			readOPCode();
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
			write(addressBus, 0xFF); //Write is performed here while data is being modified
			break;
		case 5:
			executeInstruction();
			write(addressBus, dataBus);
			break;
		case 6:
			readOPCode();
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
			addressBus = (readROM() << 8) + addressBus;
			break;
		case 3:
			dataBus = read(addressBus);
			break;
		case 4:
			write(addressBus, 0xFF); //Write is performed here while data is being modified
			break;
		case 5:
			executeInstruction();
			write(addressBus, dataBus);
			break;
		case 6:
			readOPCode();
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
			write(addressBus, 0xFF); //Write is performed here while data is being modified
			break;
		case 6:
			executeInstruction();
			write(addressBus, dataBus);
			break;
		case 7:
			readOPCode();
	}
}

void CPU::ADC()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	uint16_t temp = dataBus + cpu_registers.AC + (if_carry() ? 1 : 0);
	set_zero(temp & 0xFF);
	set_sign(temp);
	set_overflow(!((cpu_registers.AC ^ dataBus) & 0x80) && ((cpu_registers.AC ^ temp) & 0x80));
	set_carry(temp > 0xFF);
	cpu_registers.AC = temp & 0xFF;
}

void CPU::AND()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	dataBus &= cpu_registers.AC;
	set_sign(dataBus);
	set_zero(dataBus);
	cpu_registers.AC = dataBus;
}

void CPU::ASL()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	set_carry(dataBus & 0x80);
	dataBus <<= 1;
	set_sign(dataBus);
	set_zero(dataBus);
}

void CPU::BIT()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
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
		{
			uint8_t temp = cpu_registers.SR; //Set bits 5 and 4 to distinguish how value was pushed
			temp |= 0x1 << 5;
			temp |= 0x1 << 4;
			push(temp);
			break;
		}
		case 5:
			addressBus = read(0xFFFE);
			break;
		case 6:
			addressBus = (read(0xFFFF) << 8) + addressBus;
			break;
		case 7:
			cpu_registers.PC = addressBus;
			readOPCode();
	}
}

void CPU::CMP(const uint8_t& regValue)
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	uint16_t temp = regValue - dataBus;
	set_carry(temp < 0x100);
	set_sign(temp);
	set_zero(temp & 0xFF);
}

void CPU::DEC()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
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
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	cpu_registers.AC ^= dataBus;
	set_sign(cpu_registers.AC);
	set_zero(cpu_registers.AC);
}

void CPU::INC()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
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
			readOPCode();
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
			addressBus = read((addressBus & 0xFF00) + ((addressBus + 1) & 0xFF)) + dataBus; //Recreate hardware bug that prevents indirect JMP from crossing page
			break;
		case 5:
			if(debugEnabled) debugInfo.address = addressBus;
			cpu_registers.PC = addressBus;
			readOPCode();
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
			read(0x0100 + cpu_registers.SP); //Dummy read
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
			readOPCode();
	}
}

void CPU::LDA()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	set_sign(dataBus);
	set_zero(dataBus);
	cpu_registers.AC = dataBus;
}
void CPU::LDX()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	set_sign(dataBus);
	set_zero(dataBus);
	cpu_registers.X = dataBus;
}

void CPU::LDY()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	set_sign(dataBus);
	set_zero(dataBus);
	cpu_registers.Y = dataBus;
}

void CPU::LSR()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	set_carry(dataBus & 0x01);
	dataBus >>= 1;
	set_sign(dataBus);
	set_zero(dataBus);
}

void CPU::ORA()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	cpu_registers.AC |= dataBus;
	set_sign(cpu_registers.AC);
	set_zero(cpu_registers.AC);
}

void CPU::PHA()
{
	switch(cycleCount)
	{
		case 1:
			read(cpu_registers.PC); //Dummy read
			break;
		case 2: 
			push(cpu_registers.AC);
			break;
		case 3:
			readOPCode();
	}
}

void CPU::PHP()
{
	switch(cycleCount)
	{
		case 1:
			read(cpu_registers.PC); //Dummy read
			break;
		case 2:
		{
			uint8_t temp = cpu_registers.SR; //Set bits 5 and 4 to distinguish how value was pushed
			temp |= 0x1 << 5;
			temp |= 0x1 << 4;
			push(temp);
			break;
		}
		case 3:
			readOPCode();
	}
}

void CPU::PLA()
{
	switch(cycleCount)
	{
		case 1:
			read(cpu_registers.PC); //Dummy read
			break;
		case 2:
			read(0x0100 + cpu_registers.SP); //Dummy read
			break;
		case 3:
			cpu_registers.AC = pop();
			set_sign(cpu_registers.AC);
			set_zero(cpu_registers.AC);
			break;
		case 4:
			readOPCode();
	}
}

void CPU::PLP()
{
	switch(cycleCount)
	{
		case 1:
			read(cpu_registers.PC); //Dummy read
			break;
		case 2:
			read(0x0100 + cpu_registers.SP); //Dummy read
			break;
		case 3:
		{
			uint8_t temp = pop();
			temp |= 0x1 << 5;
			temp &= ~(0x1 << 4);
			cpu_registers.SR = temp;
			break;
		}
		case 4:
			readOPCode();
	}
}

void CPU::ROL()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	uint16_t temp = dataBus;
	temp <<= 1;
	if(if_carry())
		temp |= 0x01;
	set_carry(temp > 0xFF);
	dataBus = temp;
	set_sign(dataBus);
	set_zero(dataBus);
}

void CPU::ROR()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	uint16_t temp = dataBus;
	if(if_carry())
		temp |= 0x100;
	set_carry(temp & 0x01);
	temp >>= 1;
	dataBus = temp;
	set_sign(dataBus);
	set_zero(dataBus);
}

void CPU::RTI()
{
	switch(cycleCount)
	{
		case 1:
			read(cpu_registers.PC); //Dummy read
			break;
		case 2:
			read(0x0100 + cpu_registers.SP); //Dummy read
			break;
		case 3:
		{
			uint8_t temp = pop();
			temp |= 0x1 << 5;
			temp &= ~(0x1 << 4);
			cpu_registers.SR = temp;
			break;
		}
		case 4:
			addressBus = pop();
			break;
		case 5:
			addressBus = (pop() << 8) + addressBus;
			break;
		case 6:
			cpu_registers.PC = addressBus;
			readOPCode();
	}
}

void CPU::RTS()
{
	switch(cycleCount)
	{
		case 1:
			read(cpu_registers.PC); //Dummy read
			break;
		case 2:
			read(0x0100 + cpu_registers.SP); //Dummy read
			break;
		case 3:
			addressBus = pop();
			break;
		case 4:
			addressBus = (pop() << 8) + addressBus;
			break;
		case 5:
			read(addressBus); //Dummy read
			break;
		case 6:
			cpu_registers.PC = addressBus + 1;
			readOPCode();
	}
}

void CPU::SBC()
{
	if(debugEnabled) { debugInfo.memoryValue = dataBus; debugInfo.address = addressBus; }
	uint16_t temp = cpu_registers.AC - dataBus - (if_carry() ? 0 : 1);
	set_sign(temp);
	set_zero(temp & 0xFF);
	set_overflow(((cpu_registers.AC ^ temp) & 0x80) && ((cpu_registers.AC ^ dataBus) & 0x80));
	set_carry(temp < 0x100);
	cpu_registers.AC = (temp & 0xFF);
}

void CPU::TAX()
{
	cpu_registers.X = cpu_registers.AC;
	set_sign(cpu_registers.X);
	set_zero(cpu_registers.X);
}

void CPU::TAY()
{
	cpu_registers.Y = cpu_registers.AC;
	set_sign(cpu_registers.Y);
	set_zero(cpu_registers.Y);
}

void CPU::TSX()
{
	cpu_registers.X = cpu_registers.SP;
	set_sign(cpu_registers.X);
	set_zero(cpu_registers.X);
}

void CPU::TXA()
{
	cpu_registers.AC = cpu_registers.X;
	set_sign(cpu_registers.AC);
	set_zero(cpu_registers.AC);
}

void CPU::TXS()
{
	cpu_registers.SP = cpu_registers.X;
}

void CPU::TYA()
{
	cpu_registers.AC = cpu_registers.Y;
	set_sign(cpu_registers.AC);
	set_zero(cpu_registers.AC);
}

void CPU::decodeOP()
{
	std::function<void()> executeInstruction;
	switch(currentOP)
	{
		case 0x69:	//Immediate ADC
			if(debugEnabled) { debugInfo.OPCode = "ADC"; debugInfo.mode = IMMEDIATE; }
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x65: //Zero Page ADC
			if(debugEnabled) { debugInfo.OPCode = "ADC"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x75: //Zero Page,X ADC
			if(debugEnabled) { debugInfo.OPCode = "ADC"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x6D: //Absolute ADC
			if(debugEnabled) { debugInfo.OPCode = "ADC"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x7D: //Absolute,X ADC
			if(debugEnabled) { debugInfo.OPCode = "ADC"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x79: //Absolute,Y ADC
			if(debugEnabled) { debugInfo.OPCode = "ADC"; debugInfo.mode = ABSOLUTEINDEXED, debugInfo.indexString = "Y"; }
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0x61: //Indirect,X ADC
			if(debugEnabled) { debugInfo.OPCode = "ADC"; debugInfo.mode = PREINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x71: //Indirect,Y ADC
			if(debugEnabled) { debugInfo.OPCode = "ADC"; debugInfo.mode = POSTINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::ADC, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0x29: //Immediate AND
			if(debugEnabled) { debugInfo.OPCode = "AND"; debugInfo.mode = IMMEDIATE;}
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x25: //Zero Page AND
			if(debugEnabled) { debugInfo.OPCode = "AND"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x35: //Zero Page,X AND
			if(debugEnabled) { debugInfo.OPCode = "AND"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x2D: //Absolute AND
			if(debugEnabled) { debugInfo.OPCode = "AND"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x3D: //Absolute,X AND
			if(debugEnabled) { debugInfo.OPCode = "AND"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x39: //Absolute,Y AND
			if(debugEnabled) { debugInfo.OPCode = "AND"; debugInfo.mode = ABSOLUTEINDEXED, debugInfo.indexString = "Y"; }
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0x21: //Indirect,X AND
			if(debugEnabled) { debugInfo.OPCode = "AND"; debugInfo.mode = PREINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x31: //Indirect,Y AND
			if(debugEnabled) { debugInfo.OPCode = "AND"; debugInfo.mode = POSTINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::AND, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;

		case 0x0A: //Accumulator ASL
			if(debugEnabled) { debugInfo.OPCode = "ASL"; debugInfo.mode = ACCUMULATOR; }
			executeInstruction = std::bind(&CPU::ASL, this);
			addressingMode = std::bind(&CPU::accumulator, this, executeInstruction);
			break;
		case 0x06: //Zero Page ASL
			if(debugEnabled) { debugInfo.OPCode = "ASL"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::ASL, this);
			addressingMode = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0x16: //Zero Page,X ASL
			if(debugEnabled) { debugInfo.OPCode = "ASL"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::ASL, this);
			addressingMode = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0x0E: //Absolute ASL
			if(debugEnabled) { debugInfo.OPCode = "ASL"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::ASL, this);
			addressingMode = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0x1E: //Absolute,X ASL
			if(debugEnabled) { debugInfo.OPCode = "ASL"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::ASL, this);
			addressingMode = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0x90: //Relative BCC
			if(debugEnabled) { debugInfo.OPCode = "BCC"; debugInfo.mode = RELATIVE; }
			addressingMode = std::bind(&CPU::relative, this, !if_carry());
			break;
		case 0xB0: //Relative BCS
			if(debugEnabled) { debugInfo.OPCode = "BCS"; debugInfo.mode = RELATIVE; }
			addressingMode = std::bind(&CPU::relative, this, if_carry());
			break;
		case 0xF0: //Relative BEQ
			if(debugEnabled) { debugInfo.OPCode = "BEQ"; debugInfo.mode = RELATIVE; }
			addressingMode = std::bind(&CPU::relative, this, if_zero());
			break;
		case 0x24: //Zero Page BIT
			if(debugEnabled) { debugInfo.OPCode = "BIT"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::BIT, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x2C: //Absolute BIT
			if(debugEnabled) { debugInfo.OPCode = "BIT"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::BIT, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x30: //Relative BMI
			if(debugEnabled) { debugInfo.OPCode = "BMI"; debugInfo.mode = RELATIVE; }
			addressingMode = std::bind(&CPU::relative, this, if_sign());
			break;
		case 0xD0: //Relative BNE
			if(debugEnabled) { debugInfo.OPCode = "BNE"; debugInfo.mode = RELATIVE; }
			addressingMode = std::bind(&CPU::relative, this, !if_zero());
			break;
		case 0x10: //Relative BPL
			if(debugEnabled) { debugInfo.OPCode = "BPL"; debugInfo.mode = RELATIVE; }
			addressingMode = std::bind(&CPU::relative, this, !if_sign());
			break;
		case 0x00: //Implied BRK
			if(debugEnabled) { debugInfo.OPCode = "BRK"; debugInfo.mode = IMPLIED; }
			addressingMode = std::bind(&CPU::BRK, this);
			break;
		case 0x50: //Relative BVC
			if(debugEnabled) { debugInfo.OPCode = "BVC"; debugInfo.mode = RELATIVE; }
			addressingMode = std::bind(&CPU::relative, this, !if_overflow());
			break;
		case 0x70: //Relative BVS
			if(debugEnabled) { debugInfo.OPCode = "BVS"; debugInfo.mode = RELATIVE; }
			addressingMode = std::bind(&CPU::relative, this, if_overflow());
			break;
		case 0x18: //Implied CLC
			if(debugEnabled) { debugInfo.OPCode = "CLC"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::set_carry, this, 0);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xD8: //Implied CLD
			if(debugEnabled) { debugInfo.OPCode = "CLD"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::set_decimal, this, 0);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x58: //Implied CLI
			if(debugEnabled) { debugInfo.OPCode = "CLI"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::set_interrupt, this, 0);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xB8: //Implied CLV
			if(debugEnabled) { debugInfo.OPCode = "CLV"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::set_overflow, this, 0);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xC9: //Immediate CMP
			if(debugEnabled) { debugInfo.OPCode = "CMP"; debugInfo.mode = IMMEDIATE; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xC5: //Zero Page CMP
			if(debugEnabled) { debugInfo.OPCode = "CMP"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xD5: //Zero Page,X CMP
			if(debugEnabled) { debugInfo.OPCode = "CMP"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0xCD: //Absolute CMP
			if(debugEnabled) { debugInfo.OPCode = "CMP"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xDD: //Absolute,X CMP
			if(debugEnabled) { debugInfo.OPCode = "CMP"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0xD9: //Absolute,Y CMP
			if(debugEnabled) { debugInfo.OPCode = "CMP"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "Y"; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0xC1: //Indirect,X CMP
			if(debugEnabled) { debugInfo.OPCode = "CMP"; debugInfo.mode = PREINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0xD1: //Indirect,Y CMP
			if(debugEnabled) { debugInfo.OPCode = "CMP"; debugInfo.mode = POSTINDEXEDINDIRECT;  }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.AC);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0xE0: //Immediate CPX
			if(debugEnabled) { debugInfo.OPCode = "CPX"; debugInfo.mode = IMMEDIATE; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.X);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xE4: //Zero Page CPX
			if(debugEnabled) { debugInfo.OPCode = "CPX"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.X);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xEC: //Absolute CPX
			if(debugEnabled) { debugInfo.OPCode = "CPX"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.X);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xC0: //Immediate CPY
			if(debugEnabled) { debugInfo.OPCode = "CPY"; debugInfo.mode = IMMEDIATE; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.Y);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xC4: //Zero Page CPY
			if(debugEnabled) { debugInfo.OPCode = "CPY"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.Y);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xCC: //Absolute CPY
			if(debugEnabled) { debugInfo.OPCode = "CPY"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::CMP, this, cpu_registers.Y);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xC6: //Zero Page DEC
			if(debugEnabled) { debugInfo.OPCode = "DEC"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::DEC, this);
			addressingMode = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0xD6: //Zero Page, X DEC
			if(debugEnabled) { debugInfo.OPCode = "DEC"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::DEC, this);
			addressingMode = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0xCE: //Absolute DEC
			if(debugEnabled) { debugInfo.OPCode = "DEC"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::DEC, this);
			addressingMode = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0xDE: //Absolute,X DEC
			if(debugEnabled) { debugInfo.OPCode = "DEC"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::DEC, this);
			addressingMode = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0xCA: //Implied DEX
			if(debugEnabled) { debugInfo.OPCode = "DEX"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::DEX, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x88: //Implied DEY
			if(debugEnabled) { debugInfo.OPCode = "DEY"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::DEY, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x49: //Immediate EOR
			if(debugEnabled) { debugInfo.OPCode = "EOR"; debugInfo.mode = IMMEDIATE; }
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x45: //Zero Page EOR
			if(debugEnabled) { debugInfo.OPCode = "EOR"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x55: //Zero Page,X EOR
			if(debugEnabled) { debugInfo.OPCode = "EOR"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x4D: //Absolute EOR
			if(debugEnabled) { debugInfo.OPCode = "EOR"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x5D: //Absolute,X EOR
			if(debugEnabled) { debugInfo.OPCode = "EOR"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x59: //Absolute,Y EOR
			if(debugEnabled) { debugInfo.OPCode = "EOR"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "Y"; }
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0x41: //Indirect,X EOR
			if(debugEnabled) { debugInfo.OPCode = "EOR"; debugInfo.mode = PREINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x51: //Indrect,Y EOR
			if(debugEnabled) { debugInfo.OPCode = "EOR"; debugInfo.mode = POSTINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::EOR, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0xE6: //Zero Page INC
			if(debugEnabled) { debugInfo.OPCode = "INC"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::INC, this);
			addressingMode = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0xF6: //Zero Page, X INC
			if(debugEnabled) { debugInfo.OPCode = "INC"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::INC, this);
			addressingMode = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0xEE: //Absolute INC
			if(debugEnabled) { debugInfo.OPCode = "INC"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::INC, this);
			addressingMode = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0xFE: //Absolute,X INC
			if(debugEnabled) { debugInfo.OPCode = "INC"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::INC, this);
			addressingMode = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0xE8: //Implied INX
			if(debugEnabled) { debugInfo.OPCode = "INX"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::INX, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xC8: //Implied INY
			if(debugEnabled) { debugInfo.OPCode = "INY"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::INY, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x4C: //Absolute JMP
			if(debugEnabled) { debugInfo.OPCode = "JMP"; debugInfo.mode = ABSOLUTEJMP; }
			addressingMode = std::bind(&CPU::absoluteJMP, this);
			break;
		case 0x6C: //Indirect JMP
			if(debugEnabled) { debugInfo.OPCode = "JMP"; debugInfo.mode = INDIRECT; }
			addressingMode = std::bind(&CPU::indirectJMP, this);
			break;
		case 0x20: //Absolute JSR
			if(debugEnabled) { debugInfo.OPCode = "JSR"; debugInfo.mode = ABSOLUTEJMP; }
			addressingMode = std::bind(&CPU::JSR, this);
			break;
		case 0xA9: //Immediate LDA
			if(debugEnabled) { debugInfo.OPCode = "LDA"; debugInfo.mode = IMMEDIATE; }
			executeInstruction = std::bind(&CPU::LDA, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xA5: //Zero Page LDA
			if(debugEnabled) { debugInfo.OPCode = "LDA"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::LDA, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xB5: //Zero Page,X LDA
			if(debugEnabled) { debugInfo.OPCode = "LDA"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::LDA, this);
			addressingMode = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0xAD: //Absolute LDA
			if(debugEnabled) { debugInfo.OPCode = "LDA"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::LDA, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xBD: //Absolute,X LDA
			if(debugEnabled) { debugInfo.OPCode = "LDA"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::LDA, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0xB9: //Absolute,Y LDA
			if(debugEnabled) { debugInfo.OPCode = "LDA"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "Y"; }
			executeInstruction = std::bind(&CPU::LDA, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0xA1: //Indirect,X LDA
			if(debugEnabled) { debugInfo.OPCode = "LDA"; debugInfo.mode = PREINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::LDA, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0xB1: //Indirect,Y LDA
			if(debugEnabled) { debugInfo.OPCode = "LDA"; debugInfo.mode = POSTINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::LDA, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0xA2: //Immediate LDX
			if(debugEnabled) { debugInfo.OPCode = "LDX"; debugInfo.mode = IMMEDIATE; }
			executeInstruction = std::bind(&CPU::LDX, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xA6: //Zero Page LDX
			if(debugEnabled) { debugInfo.OPCode = "LDX"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::LDX, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xB6: //Zero Page,Y LDX
			if(debugEnabled) { debugInfo.OPCode = "LDX"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "Y"; }
			executeInstruction = std::bind(&CPU::LDX, this);
			addressingMode = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0xAE: //Absolute LDX
			if(debugEnabled) { debugInfo.OPCode = "LDX"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::LDX, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xBE: //Absolute,Y LDX
			if(debugEnabled) { debugInfo.OPCode = "LDX"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "Y"; }
			executeInstruction = std::bind(&CPU::LDX, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0xA0: //Immediate LDY
			if(debugEnabled) { debugInfo.OPCode = "LDY"; debugInfo.mode = IMMEDIATE; }
			executeInstruction = std::bind(&CPU::LDY, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xA4: //Zero Page LDY
			if(debugEnabled) { debugInfo.OPCode = "LDY"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::LDY, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xB4: //Zero Page,X LDY
			if(debugEnabled) { debugInfo.OPCode = "LDY"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::LDY, this);
			addressingMode = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0xAC: //Absolute LDY
			if(debugEnabled) { debugInfo.OPCode = "LDY"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::LDY, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xBC: //Absolute,X LDY
			if(debugEnabled) { debugInfo.OPCode = "LDY"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::LDY, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x4A: //Accumulator LSR
			if(debugEnabled) { debugInfo.OPCode = "LSR"; debugInfo.mode = ACCUMULATOR; }
			executeInstruction = std::bind(&CPU::LSR, this);
			addressingMode = std::bind(&CPU::accumulator, this, executeInstruction);
			break;
		case 0x46: //Zero Page LSR
			if(debugEnabled) { debugInfo.OPCode = "LSR"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::LSR, this);
			addressingMode = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0x56: //Zero Page,X LSR
			if(debugEnabled) { debugInfo.OPCode = "LSR"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::LSR, this);
			addressingMode = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0x4E: //Absolute LSR
			if(debugEnabled) { debugInfo.OPCode = "LSR"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::LSR, this);
			addressingMode = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0x5E: //Absolute,X LSR
			if(debugEnabled) { debugInfo.OPCode = "LSR"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::LSR, this);
			addressingMode = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0xEA: //Implied NOP
			if(debugEnabled) { debugInfo.OPCode = "NOP"; debugInfo.mode = IMPLIED; }
			executeInstruction = [](){};
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x09: //Immediate ORA
			if(debugEnabled) { debugInfo.OPCode = "ORA"; debugInfo.mode = IMMEDIATE; }
			executeInstruction = std::bind(&CPU::ORA, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x05: //Zero Page ORA
			if(debugEnabled) { debugInfo.OPCode = "ORA"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::ORA, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x15: //Zero Page,X ORA
			if(debugEnabled) { debugInfo.OPCode = "ORA"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::ORA, this);
			addressingMode = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x0D: //Absolute ORA
			if(debugEnabled) { debugInfo.OPCode = "ORA"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::ORA, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x1D: //Absolute,X ORA
			if(debugEnabled) { debugInfo.OPCode = "ORA"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::ORA, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0x19: //Absolute,Y ORA
			if(debugEnabled) { debugInfo.OPCode = "ORA"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "Y"; }
			executeInstruction = std::bind(&CPU::ORA, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0x01: //Indirect,X ORA
			if(debugEnabled) { debugInfo.OPCode = "ORA"; debugInfo.mode = PREINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::ORA, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x11: //Indirect,Y ORA
			if(debugEnabled) { debugInfo.OPCode = "ORA"; debugInfo.mode = POSTINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::ORA, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0x48: //Implied PHA
			if(debugEnabled) { debugInfo.OPCode = "PHA"; debugInfo.mode = IMPLIED; }
			addressingMode = std::bind(&CPU::PHA, this);
			break;
		case 0x08: //Implied PHP
			if(debugEnabled) { debugInfo.OPCode = "PHP"; debugInfo.mode = IMPLIED; }
			addressingMode = std::bind(&CPU::PHP, this);
			break;
		case 0x68: //Implied PLA
			if(debugEnabled) { debugInfo.OPCode = "PLA"; debugInfo.mode = IMPLIED; }
			addressingMode = std::bind(&CPU::PLA, this);
			break;
		case 0x28: //Implied PLP
			if(debugEnabled) { debugInfo.OPCode = "PLP"; debugInfo.mode = IMPLIED; }
			addressingMode = std::bind(&CPU::PLP, this);
			break;
		case 0x2A: //Accumulator ROL
			if(debugEnabled) { debugInfo.OPCode = "ROL"; debugInfo.mode = ACCUMULATOR; }
			executeInstruction = std::bind(&CPU::ROL, this);
			addressingMode = std::bind(&CPU::accumulator, this, executeInstruction);
			break;
		case 0x26: //Zero Page ROL
			if(debugEnabled) { debugInfo.OPCode = "ROL"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::ROL, this);
			addressingMode = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0x36: //Zero Page,X ROL
			if(debugEnabled) { debugInfo.OPCode = "ROL"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::ROL, this);
			addressingMode = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0x2E: //Absolute ROL
			if(debugEnabled) { debugInfo.OPCode = "ROL"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::ROL, this);
			addressingMode = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0x3E: //Absolute,X ROL
			if(debugEnabled) { debugInfo.OPCode = "ROL"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::ROL, this);
			addressingMode = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0x6A: //Accumulator ROR
			if(debugEnabled) { debugInfo.OPCode = "ROR"; debugInfo.mode = ACCUMULATOR; }
			executeInstruction = std::bind(&CPU::ROR, this);
			addressingMode = std::bind(&CPU::accumulator, this, executeInstruction);
			break;
		case 0x66: //Zero Page ROR
			if(debugEnabled) { debugInfo.OPCode = "ROR"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::ROR, this);
			addressingMode = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0x76: //Zero Page,X ROR
			if(debugEnabled) { debugInfo.OPCode = "ROR"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::ROR, this);
			addressingMode = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0x6E: //Absolute ROR
			if(debugEnabled) { debugInfo.OPCode = "ROR"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::ROR, this);
			addressingMode = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0x7E: //Absolute,X ROR
			if(debugEnabled) { debugInfo.OPCode = "ROR"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::ROR, this);
			addressingMode = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0x40: //Implied RTI
			if(debugEnabled) { debugInfo.OPCode = "RTI"; debugInfo.mode = IMPLIED; }
			addressingMode = std::bind(&CPU::RTI, this);
			break;
		case 0x60: //Implied RTS
			if(debugEnabled) { debugInfo.OPCode = "RTS"; debugInfo.mode = IMPLIED; }
			addressingMode = std::bind(&CPU::RTS, this);
			break;
		case 0xE9: //Immediate SBC
			if(debugEnabled) { debugInfo.OPCode = "SBC"; debugInfo.mode = IMMEDIATE; }
			executeInstruction = std::bind(&CPU::SBC, this);
			addressingMode = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xE5: //Zero Page SBC
			if(debugEnabled) { debugInfo.OPCode = "SBC"; debugInfo.mode = ZEROPAGE; }
			executeInstruction = std::bind(&CPU::SBC, this);
			addressingMode = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xF5: //Zero Page,X SBC
			if(debugEnabled) { debugInfo.OPCode = "SBC"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::SBC, this);
			addressingMode = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0xED: //Absolute SBC
			if(debugEnabled) { debugInfo.OPCode = "SBC"; debugInfo.mode = ABSOLUTE; }
			executeInstruction = std::bind(&CPU::SBC, this);
			addressingMode = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xFD: //Absolute,X SBC
			if(debugEnabled) { debugInfo.OPCode = "SBC"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			executeInstruction = std::bind(&CPU::SBC, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.X);
			break;
		case 0xF9: //Absolute,Y SBC
			if(debugEnabled) { debugInfo.OPCode = "SBC"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "Y"; }
			executeInstruction = std::bind(&CPU::SBC, this);
			addressingMode = std::bind(&CPU::absoluteIndexed, this, executeInstruction, cpu_registers.Y);
			break;
		case 0xE1: //Indirect,X SBC
			if(debugEnabled) { debugInfo.OPCode = "SBC"; debugInfo.mode = PREINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::SBC, this);
			addressingMode = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0xF1: //Indirect,Y SBC
			if(debugEnabled) { debugInfo.OPCode = "SBC"; debugInfo.mode = POSTINDEXEDINDIRECT; }
			executeInstruction = std::bind(&CPU::SBC, this);
			addressingMode = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0x38: //Implied SEC
			if(debugEnabled) { debugInfo.OPCode = "SEC"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::set_carry, this, 1);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xF8: //Implied SED
			if(debugEnabled) { debugInfo.OPCode = "SED"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::set_decimal, this, 1);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x78: //Implied SEI
			if(debugEnabled) { debugInfo.OPCode = "SEI"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::set_interrupt, this, 1);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x85: //Zero Page STA
			if(debugEnabled) { debugInfo.OPCode = "STA"; debugInfo.mode = ZEROPAGE; }
			addressingMode = std::bind(&CPU::zeroPage_Store, this, cpu_registers.AC);
			break;
		case 0x95: //Zero Page,X STA
			if(debugEnabled) { debugInfo.OPCode = "STA"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "X"; }
			addressingMode = std::bind(&CPU::zeroPageIndexed_Store, this, cpu_registers.AC, cpu_registers.X);
			break;
		case 0x8D: //Absolute STA
			if(debugEnabled) { debugInfo.OPCode = "STA"; debugInfo.mode = ABSOLUTE; }
			addressingMode = std::bind(&CPU::absolute_Store, this, cpu_registers.AC);
			break;
		case 0x9D: //Absolute,X STA
			if(debugEnabled) { debugInfo.OPCode = "STA"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "X"; }
			addressingMode = std::bind(&CPU::absoluteIndexed_Store, this, cpu_registers.AC, cpu_registers.X);
			break;
		case 0x99: //Absolute,Y STA
			if(debugEnabled) { debugInfo.OPCode = "STA"; debugInfo.mode = ABSOLUTEINDEXED; debugInfo.indexString = "Y"; }
			addressingMode = std::bind(&CPU::absoluteIndexed_Store, this, cpu_registers.AC, cpu_registers.Y);
			break;
		case 0x81: //Indirect,X STA
			if(debugEnabled) { debugInfo.OPCode = "STA"; debugInfo.mode = PREINDEXEDINDIRECT; }
			addressingMode = std::bind(&CPU::indirectX_Store, this, cpu_registers.AC);
			break;
		case 0x91: //Indirect,Y STA
			if(debugEnabled) { debugInfo.OPCode = "STA"; debugInfo.mode = POSTINDEXEDINDIRECT; }
			addressingMode = std::bind(&CPU::indirectY_Store, this, cpu_registers.AC);
			break;
		case 0x86: //Zero Page STX
			if(debugEnabled) { debugInfo.OPCode = "STX"; debugInfo.mode = ZEROPAGE; }
			addressingMode = std::bind(&CPU::zeroPage_Store, this, cpu_registers.X);
			break;
		case 0x96: //Zero Page,Y STX
			if(debugEnabled) { debugInfo.OPCode = "STX"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "Y"; }
			addressingMode = std::bind(&CPU::zeroPageIndexed_Store, this, cpu_registers.X, cpu_registers.Y);
			break;
		case 0x8E: //Absolute STX
			if(debugEnabled) { debugInfo.OPCode = "STX"; debugInfo.mode = ABSOLUTE; }
			addressingMode = std::bind(&CPU::absolute_Store, this, cpu_registers.X);
			break;
		case 0x84: //Zero Page STY
			if(debugEnabled) { debugInfo.OPCode = "STY"; debugInfo.mode = ZEROPAGE; }
			addressingMode = std::bind(&CPU::zeroPage_Store, this, cpu_registers.Y);
			break;
		case 0x94: //Zero Page,X STY
			if(debugEnabled) { debugInfo.OPCode = "STY"; debugInfo.mode = ZEROPAGEINDEXED; debugInfo.indexString = "x"; }
			addressingMode = std::bind(&CPU::zeroPageIndexed_Store, this, cpu_registers.Y, cpu_registers.X);
			break;
		case 0x8C: //Absolute STY
			if(debugEnabled) { debugInfo.OPCode = "STY"; debugInfo.mode = ABSOLUTE; }
			addressingMode = std::bind(&CPU::absolute_Store, this, cpu_registers.Y);
			break;
		case 0xAA: //Implied TAX
			if(debugEnabled) { debugInfo.OPCode = "TAX"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::TAX, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xA8: //Implied TAY
			if(debugEnabled) { debugInfo.OPCode = "TAY"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::TAY, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xBA: //Implied TSX
			if(debugEnabled) { debugInfo.OPCode = "TSX"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::TSX, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x8A: //Implied TXA
			if(debugEnabled) { debugInfo.OPCode = "TXA"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::TXA, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x9A: //Implied TXS
			if(debugEnabled) { debugInfo.OPCode = "TXS"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::TXS, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x98: //Implied TYA
			if(debugEnabled) { debugInfo.OPCode = "TYA"; debugInfo.mode = IMPLIED; }
			executeInstruction = std::bind(&CPU::TYA, this);
			addressingMode = std::bind(&CPU::implied, this, executeInstruction);
			break;
		default:
			throw UnkownOPCode(currentOP, cycleCount, totalCycles, cpu_registers.PC);
	}
	addressingMode();
}

void CPU::debug()
{
	if(totalCycles > 7)
	{
		debugInfo.print(log);
	}
}