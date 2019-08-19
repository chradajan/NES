#include "include/CPU.hpp"

CPU::CPU(Cartridge* cart, PPU& ppu, APU_IO_Registers& apu_io_reg, std::fstream& cpuLog) 
: cart(*cart), ppu(ppu), apu_io_registers(apu_io_reg), log(cpuLog)
{
	reg.SR = 0x34;
	reg.AC = 0;
	reg.X = 0;
	reg.Y = 0;
	reg.SP = 0xFD;

	write(0x4017, 0x00);
	write(0x4015, 0x00);
	for(uint16_t i = 0x4000; i < 0x4010; ++i)
		write(i, 0x00);
	for(uint16_t i = 0x0000; i < 0x2000; ++i)
		write(i, 0x00);
	//TODO: set noise channel

	cycleCount = 0;
	totalCycles = 0;
	oddCycle = false;

	totalCycles = 0;

	debugEnabled = false;

	Reset_Vector();

	readOPCode();
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
	else if(cycleCount == 1)
		decodeOP();
	else
		tickFunction();
}

CPU::~CPU() {}

void CPU::executeDMATransfer()
{
	if(dmaTransferCycles == 514)
		read(reg.PC); //Dummy read
	else if(oddCycle && dmaTransferCycles == 513)
		return;
	else if(dmaTransferCycles == 513)
		read(reg.PC); //Dummy read
	else if(dmaTransferCycles % 2 == 0)
		dmaData = read(dmaPage + dmaLowByte++);
	else if(dmaTransferCycles % 2 == 1)
		write(0x2004, dmaData);

	--dmaTransferCycles;
	if(dmaTransferCycles == 0)
	{
		dmaTransfer = false;
		cycleCount = cycleCountReturn;
	}
}

void CPU::NMI()
{
	switch(cycleCount)
	{
		case 1:
			read(reg.PC); //Dummy read
			break;
		case 2:
			read(reg.PC); //Dummy read
			break;
		case 3:
			push(reg.PC >> 8);
			break;
		case 4:
			push(reg.PC & 0xFF);
			break;
		case 5:
		{
			uint8_t temp = reg.SR;
			temp |= 0x20;
			temp &= 0xEF;
			push(temp);
			break;
		}
		case 6:
			addressBus = read(0xFFFA);
			break;
		case 7:
			addressBus = (read(0xFFFB) << 8) + addressBus;
			break;
		case 8:
			reg.PC = addressBus;
			readOPCode();
	}
}

uint8_t CPU::read(uint16_t address) const
{
	if(address < 0x2000) //Internal RAM
		return RAM[address % 0x0800];
	else if(address < 0x4000) //PPU registers
		return ppu.readMemMappedReg(address);
	else if(address < 0x4018) //APU or I/O Registers
		return apu_io_registers.read(address);
	else if(address < 0x4020) //Disabled APU and I/O Functionality
		throw Unsupported("CPU Test Mode Disabled");
	else //Cartridge Space
		return cart.readPRG(address);
}

void CPU::write(uint16_t address, uint8_t data)
{
	if(address < 0x2000) //Internal RAM
		RAM[address % 0x0800] = data;
	else if(address < 0x4000) //PPU registers
		ppu.writeMemMappedReg(address, data);
	else if(address == 0x4014) //Trigger DMA Transfer
	{
		dmaTransfer = true;
		dmaPage = (data << 8);
		dmaLowByte = 0x00;
		dmaTransferCycles = 513 + (oddCycle ? 1 : 0);
		cycleCountReturn = cycleCount;
		apu_io_registers.write(address, data);
	}
	else if(address < 0x4018) //APU or I/O Registers
		apu_io_registers.write(address, data);
	else if(address < 0x4020) //Disabled APU and I/O Functionality
		throw Unsupported("CPU Test Mode Disabled");
	else //Cartridge Space
		cart.writePRG(address, data);
}

uint8_t CPU::pop()
{
	++reg.SP;
	return read(0x0100 + reg.SP);
}

void CPU::push(uint8_t data)
{
	write(0x0100 + reg.SP, data);
	--reg.SP;
}

void CPU::readOPCode()
{
	if(ppu.NMI())
	{
		if(debugEnabled)
			debug();
		cycleCount = 1;
		tickFunction = std::bind(&CPU::NMI, this);
		tickFunction();
	}
	else
	{
		currentOP = read(reg.PC++);
		if(debugEnabled)
			debug();
		cycleCount = 0;
	}
}

uint8_t CPU::readROM()
{
	uint8_t operand = read(reg.PC++);
	if(debugEnabled)
		debugInfo.add(operand);
	return operand;
}

bool CPU::if_carry()
{
	return reg.SR & 0x01;
}

bool CPU::if_overflow()
{
	return (reg.SR >> 6) & 0x01;
}

bool CPU::if_sign()
{
	return (reg.SR >> 7) & 0x01;
}

bool CPU::if_zero()
{
	return (reg.SR >> 1) & 0x01;
}

void CPU::set_carry(bool condition)
{
	if(condition)
		reg.SR |= 0x01;
	else
		reg.SR &= ~(0x01);
}

void CPU::set_zero(uint8_t value)
{
	if(value == 0)
		reg.SR |= 0x01 << 1;
	else
		reg.SR &= ~(0x01 << 1);
}

void CPU::set_interrupt(bool condition)
{
	if(condition)
		reg.SR |= 0x01 << 2;
	else
		reg.SR &= ~(0x01 << 2);
}

void CPU::set_decimal(bool condition)
{
	if(condition)
		reg.SR |= 0x01 << 3;
	else
		reg.SR &= ~(0x01 << 3);
}

void CPU::set_break(bool condition)
{
	if(condition)
		reg.SR |= 0x01 << 4;
	else
		reg.SR &= ~(0x01 << 4);
}

void CPU::set_overflow(bool condition)
{
	if(condition)
		reg.SR |= 0x01 << 6;
	else
		reg.SR &= ~(0x01 << 6);
}

void CPU::set_sign(uint8_t value)
{
	if(value >= 0x80)
		reg.SR |= 0x01 << 7;
	else
		reg.SR &= ~(0x01 << 7);
}

void CPU::Reset_Vector()
{
		addressBus = read(0xFFFC);
		addressBus = (read(0xFFFD) << 8) + addressBus;
		reg.PC = addressBus;
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
			reg.PC = addressBus;
			readOPCode();
	}
}

void CPU::implied(std::function<void()> executeInstruction)
{
	switch(cycleCount)
	{
		case 1:
			read(reg.PC); //Dummy read
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
			dataBus += reg.X;
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
			dataBus = readROM();
			break;
		case 2:
			addressBus = read(dataBus);
			++dataBus;
			break;
		case 3:
			addressBus = (read(dataBus) << 8) + addressBus;
			break;
		case 4:
			if(((addressBus & 0xFF) + reg.Y) <= 0xFF)
			{
				addressBus += reg.Y;
				dataBus = read(addressBus);
				++cycleCount; //Skip next step since page boundary is not crossed
			}
			else
				read((addressBus & 0xFF00) + ((addressBus + reg.Y) & 0xFF)); //Dummy read since page boundary crossed
			break;
		case 5:
			addressBus += reg.Y;
			dataBus = read(addressBus);
			break;
		case 6:
			executeInstruction();
			readOPCode();
	}
}

uint16_t CPU::relativeAddress(uint8_t offset)
{
	// bool isNegative = (offset & 0x80) >> 7;
	// if(isNegative)
	// {
	// 	offset &= 0x7F;
	// 	return reg.PC - offset;
	// }
	// return reg.PC + offset;
	int8_t signedOffset = offset;
	return reg.PC + signedOffset;
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
				readOPCode();
			else
				read((reg.PC & 0xFF00) + (addressBus & 0xFF)); //Dummy read
			break;
		case 3:
			if((reg.PC & 0xFF00) != (addressBus & 0xFF00))
				read(addressBus); //Dummy read
			else
			{
				reg.PC = addressBus;
				readOPCode();
			}
			break;
		case 4:
			reg.PC = addressBus;
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
			dataBus += reg.X;
			break;
		case 3:
			addressBus = read(dataBus);
			++dataBus;
			break;
		case 4:
			addressBus = (read(dataBus) << 8) + addressBus;
			break;
		case 5:
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
			read((addressBus & 0xFF00) + ((addressBus + reg.Y) & 0xFF)); //Dummy read
			addressBus += reg.Y;
			break;
		case 5:
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
			read(reg.PC); //Dummy read
			dataBus = reg.AC;
			break;
		case 2:
			executeInstruction();
			reg.AC = dataBus;
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
			dataBus += reg.X;
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
			read((addressBus << 8) + ((dataBus + reg.X) & 0xFF)); //Dummy read
			addressBus = (addressBus << 8) + dataBus;
			break;
		case 4:
			addressBus += reg.X; 
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
	uint16_t temp = dataBus + reg.AC + (if_carry() ? 1 : 0);
	set_zero(temp & 0xFF);
	set_sign(temp);
	set_overflow(!((reg.AC ^ dataBus) & 0x80) && ((reg.AC ^ temp) & 0x80));
	set_carry(temp > 0xFF);
	reg.AC = temp & 0xFF;
}

void CPU::AND()
{
	dataBus &= reg.AC;
	set_sign(dataBus);
	set_zero(dataBus);
	reg.AC = dataBus;
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
	set_zero(dataBus & reg.AC);
}

void CPU::BRK()
{
	switch(cycleCount)
	{
		case 1:
			readROM(); //Absorb padding byte after BRK
			break;
		case 2:
			push(reg.PC >> 8);
			break;
		case 3:
			push(reg.PC & 0xFF);
			break;
		case 4:
		{
			uint8_t temp = reg.SR; //Set bits 5 and 4 to distinguish how value was pushed
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
			reg.PC = addressBus;
			readOPCode();
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
	uint16_t temp = (reg.X - 1) & 0xFF;
	set_sign(temp);
	set_zero(temp);
	reg.X = temp;
}

void CPU::DEY()
{
	uint16_t temp = (reg.Y - 1) & 0xFF;
	set_sign(temp);
	set_zero(temp);
	reg.Y = temp;
}

void CPU::EOR()
{
	reg.AC ^= dataBus;
	set_sign(reg.AC);
	set_zero(reg.AC);
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
	uint16_t temp = (reg.X + 1) & 0xFF;
	set_sign(temp);
	set_zero(temp);
	reg.X = temp;
}

void CPU::INY()
{
	uint16_t temp = (reg.Y + 1) & 0xFF;
	set_sign(temp);
	set_zero(temp);
	reg.Y = temp;
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
			reg.PC = addressBus;
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
			addressBus = ((addressBus & 0xFF00) + ((addressBus + 1) & 0xFF)); //Recreate hardware bug that prevents indirect JMP from crossing page
			break;
		case 4:
			addressBus = (read(addressBus) << 8) + dataBus;
			break;
		case 5:
			reg.PC = addressBus;
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
			read(0x0100 + reg.SP); //Dummy read
			break;
		case 3:
			push(reg.PC >> 8);
			break;
		case 4:
			push(reg.PC & 0xFF);
			break;
		case 5:
			addressBus = (readROM() << 8) + addressBus;
			break;
		case 6:
			reg.PC = addressBus;
			readOPCode();
	}
}

void CPU::LDA()
{
	set_sign(dataBus);
	set_zero(dataBus);
	reg.AC = dataBus;
}
void CPU::LDX()
{
	set_sign(dataBus);
	set_zero(dataBus);
	reg.X = dataBus;
}

void CPU::LDY()
{
	set_sign(dataBus);
	set_zero(dataBus);
	reg.Y = dataBus;
}

void CPU::LSR()
{
	set_carry(dataBus & 0x01);
	dataBus >>= 1;
	set_sign(dataBus);
	set_zero(dataBus);
}

void CPU::ORA()
{
	reg.AC |= dataBus;
	set_sign(reg.AC);
	set_zero(reg.AC);
}

void CPU::PHA()
{
	switch(cycleCount)
	{
		case 1:
			read(reg.PC); //Dummy read
			break;
		case 2: 
			push(reg.AC);
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
			read(reg.PC); //Dummy read
			break;
		case 2:
		{
			uint8_t temp = reg.SR; //Set bits 5 and 4 to distinguish how value was pushed
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
			read(reg.PC); //Dummy read
			break;
		case 2:
			read(0x0100 + reg.SP); //Dummy read
			break;
		case 3:
			reg.AC = pop();
			set_sign(reg.AC);
			set_zero(reg.AC);
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
			read(reg.PC); //Dummy read
			break;
		case 2:
			read(0x0100 + reg.SP); //Dummy read
			break;
		case 3:
		{
			uint8_t temp = pop();
			temp |= 0x1 << 5;
			temp &= ~(0x1 << 4);
			reg.SR = temp;
			break;
		}
		case 4:
			readOPCode();
	}
}

void CPU::ROL()
{
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
			read(reg.PC); //Dummy read
			break;
		case 2:
			read(0x0100 + reg.SP); //Dummy read
			break;
		case 3:
		{
			uint8_t temp = pop();
			temp |= 0x1 << 5;
			temp &= ~(0x1 << 4);
			reg.SR = temp;
			break;
		}
		case 4:
			addressBus = pop();
			break;
		case 5:
			addressBus = (pop() << 8) + addressBus;
			break;
		case 6:
			reg.PC = addressBus;
			readOPCode();
	}
}

void CPU::RTS()
{
	switch(cycleCount)
	{
		case 1:
			read(reg.PC); //Dummy read
			break;
		case 2:
			read(0x0100 + reg.SP); //Dummy read
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
			reg.PC = addressBus + 1;
			readOPCode();
	}
}

void CPU::SBC()
{
	uint16_t temp = reg.AC - dataBus - (if_carry() ? 0 : 1);
	set_sign(temp);
	set_zero(temp & 0xFF);
	set_overflow(((reg.AC ^ temp) & 0x80) && ((reg.AC ^ dataBus) & 0x80));
	set_carry(temp < 0x100);
	reg.AC = (temp & 0xFF);
}

void CPU::TAX()
{
	reg.X = reg.AC;
	set_sign(reg.X);
	set_zero(reg.X);
}

void CPU::TAY()
{
	reg.Y = reg.AC;
	set_sign(reg.Y);
	set_zero(reg.Y);
}

void CPU::TSX()
{
	reg.X = reg.SP;
	set_sign(reg.X);
	set_zero(reg.X);
}

void CPU::TXA()
{
	reg.AC = reg.X;
	set_sign(reg.AC);
	set_zero(reg.AC);
}

void CPU::TXS()
{
	reg.SP = reg.X;
}

void CPU::TYA()
{
	reg.AC = reg.Y;
	set_sign(reg.AC);
	set_zero(reg.AC);
}

void CPU::decodeOP()
{
	std::function<void()> executeInstruction;
	switch(currentOP)
	{
		case 0x69:	//Immediate ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x65: //Zero Page ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x75: //Zero Page,X ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			tickFunction = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, reg.X);
			break;
		case 0x6D: //Absolute ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x7D: //Absolute,X ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.X);
			break;
		case 0x79: //Absolute,Y ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.Y);
			break;
		case 0x61: //Indirect,X ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			tickFunction = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x71: //Indirect,Y ADC
			executeInstruction = std::bind(&CPU::ADC, this);
			tickFunction = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0x29: //Immediate AND
			executeInstruction = std::bind(&CPU::AND, this);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x25: //Zero Page AND
			executeInstruction = std::bind(&CPU::AND, this);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x35: //Zero Page,X AND
			executeInstruction = std::bind(&CPU::AND, this);
			tickFunction = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, reg.X);
			break;
		case 0x2D: //Absolute AND
			executeInstruction = std::bind(&CPU::AND, this);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x3D: //Absolute,X AND
			executeInstruction = std::bind(&CPU::AND, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.X);
			break;
		case 0x39: //Absolute,Y AND
			executeInstruction = std::bind(&CPU::AND, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.Y);
			break;
		case 0x21: //Indirect,X AND
			executeInstruction = std::bind(&CPU::AND, this);
			tickFunction = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x31: //Indirect,Y AND
			executeInstruction = std::bind(&CPU::AND, this);
			tickFunction = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0x0A: //Accumulator ASL
			executeInstruction = std::bind(&CPU::ASL, this);
			tickFunction = std::bind(&CPU::accumulator, this, executeInstruction);
			break;
		case 0x06: //Zero Page ASL
			executeInstruction = std::bind(&CPU::ASL, this);
			tickFunction = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0x16: //Zero Page,X ASL
			executeInstruction = std::bind(&CPU::ASL, this);
			tickFunction = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0x0E: //Absolute ASL
			executeInstruction = std::bind(&CPU::ASL, this);
			tickFunction = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0x1E: //Absolute,X ASL
			executeInstruction = std::bind(&CPU::ASL, this);
			tickFunction = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0x90: //Relative BCC
			tickFunction = std::bind(&CPU::relative, this, !if_carry());
			break;
		case 0xB0: //Relative BCS
			tickFunction = std::bind(&CPU::relative, this, if_carry());
			break;
		case 0xF0: //Relative BEQ
			tickFunction = std::bind(&CPU::relative, this, if_zero());
			break;
		case 0x24: //Zero Page BIT
			executeInstruction = std::bind(&CPU::BIT, this);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x2C: //Absolute BIT
			executeInstruction = std::bind(&CPU::BIT, this);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x30: //Relative BMI
			tickFunction = std::bind(&CPU::relative, this, if_sign());
			break;
		case 0xD0: //Relative BNE
			tickFunction = std::bind(&CPU::relative, this, !if_zero());
			break;
		case 0x10: //Relative BPL
			tickFunction = std::bind(&CPU::relative, this, !if_sign());
			break;
		case 0x00: //Implied BRK
			tickFunction = std::bind(&CPU::BRK, this);
			break;
		case 0x50: //Relative BVC
			tickFunction = std::bind(&CPU::relative, this, !if_overflow());
			break;
		case 0x70: //Relative BVS
			tickFunction = std::bind(&CPU::relative, this, if_overflow());
			break;
		case 0x18: //Implied CLC
			executeInstruction = std::bind(&CPU::set_carry, this, 0);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xD8: //Implied CLD
			executeInstruction = std::bind(&CPU::set_decimal, this, 0);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x58: //Implied CLI
			executeInstruction = std::bind(&CPU::set_interrupt, this, 0);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xB8: //Implied CLV
			executeInstruction = std::bind(&CPU::set_overflow, this, 0);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xC9: //Immediate CMP
			executeInstruction = std::bind(&CPU::CMP, this, reg.AC);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xC5: //Zero Page CMP
			executeInstruction = std::bind(&CPU::CMP, this, reg.AC);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xD5: //Zero Page,X CMP
			executeInstruction = std::bind(&CPU::CMP, this, reg.AC);
			tickFunction = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, reg.X);
			break;
		case 0xCD: //Absolute CMP
			executeInstruction = std::bind(&CPU::CMP, this, reg.AC);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xDD: //Absolute,X CMP
			executeInstruction = std::bind(&CPU::CMP, this, reg.AC);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.X);
			break;
		case 0xD9: //Absolute,Y CMP
			executeInstruction = std::bind(&CPU::CMP, this, reg.AC);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.Y);
			break;
		case 0xC1: //Indirect,X CMP
			executeInstruction = std::bind(&CPU::CMP, this, reg.AC);
			tickFunction = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0xD1: //Indirect,Y CMP
			executeInstruction = std::bind(&CPU::CMP, this, reg.AC);
			tickFunction = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0xE0: //Immediate CPX
			executeInstruction = std::bind(&CPU::CMP, this, reg.X);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xE4: //Zero Page CPX
			executeInstruction = std::bind(&CPU::CMP, this, reg.X);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xEC: //Absolute CPX
			executeInstruction = std::bind(&CPU::CMP, this, reg.X);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xC0: //Immediate CPY
			executeInstruction = std::bind(&CPU::CMP, this, reg.Y);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xC4: //Zero Page CPY
			executeInstruction = std::bind(&CPU::CMP, this, reg.Y);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xCC: //Absolute CPY
			executeInstruction = std::bind(&CPU::CMP, this, reg.Y);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xC6: //Zero Page DEC
			executeInstruction = std::bind(&CPU::DEC, this);
			tickFunction = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0xD6: //Zero Page, X DEC
			executeInstruction = std::bind(&CPU::DEC, this);
			tickFunction = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0xCE: //Absolute DEC
			executeInstruction = std::bind(&CPU::DEC, this);
			tickFunction = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0xDE: //Absolute,X DEC
			executeInstruction = std::bind(&CPU::DEC, this);
			tickFunction = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0xCA: //Implied DEX
			executeInstruction = std::bind(&CPU::DEX, this);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x88: //Implied DEY
			executeInstruction = std::bind(&CPU::DEY, this);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x49: //Immediate EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x45: //Zero Page EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x55: //Zero Page,X EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			tickFunction = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, reg.X);
			break;
		case 0x4D: //Absolute EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x5D: //Absolute,X EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.X);
			break;
		case 0x59: //Absolute,Y EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.Y);
			break;
		case 0x41: //Indirect,X EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			tickFunction = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x51: //Indrect,Y EOR
			executeInstruction = std::bind(&CPU::EOR, this);
			tickFunction = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0xE6: //Zero Page INC
			executeInstruction = std::bind(&CPU::INC, this);
			tickFunction = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0xF6: //Zero Page, X INC
			executeInstruction = std::bind(&CPU::INC, this);
			tickFunction = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0xEE: //Absolute INC
			executeInstruction = std::bind(&CPU::INC, this);
			tickFunction = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0xFE: //Absolute,X INC
			executeInstruction = std::bind(&CPU::INC, this);
			tickFunction = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0xE8: //Implied INX
			executeInstruction = std::bind(&CPU::INX, this);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xC8: //Implied INY
			executeInstruction = std::bind(&CPU::INY, this);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x4C: //Absolute JMP
			tickFunction = std::bind(&CPU::absoluteJMP, this);
			break;
		case 0x6C: //Indirect JMP
			tickFunction = std::bind(&CPU::indirectJMP, this);
			break;
		case 0x20: //Absolute JSR
			tickFunction = std::bind(&CPU::JSR, this);
			break;
		case 0xA9: //Immediate LDA
			executeInstruction = std::bind(&CPU::LDA, this);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xA5: //Zero Page LDA
			executeInstruction = std::bind(&CPU::LDA, this);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xB5: //Zero Page,X LDA
			executeInstruction = std::bind(&CPU::LDA, this);
			tickFunction = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, reg.X);
			break;
		case 0xAD: //Absolute LDA
			executeInstruction = std::bind(&CPU::LDA, this);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xBD: //Absolute,X LDA
			executeInstruction = std::bind(&CPU::LDA, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.X);
			break;
		case 0xB9: //Absolute,Y LDA
			executeInstruction = std::bind(&CPU::LDA, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.Y);
			break;
		case 0xA1: //Indirect,X LDA
			executeInstruction = std::bind(&CPU::LDA, this);
			tickFunction = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0xB1: //Indirect,Y LDA
			executeInstruction = std::bind(&CPU::LDA, this);
			tickFunction = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0xA2: //Immediate LDX
			executeInstruction = std::bind(&CPU::LDX, this);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xA6: //Zero Page LDX
			executeInstruction = std::bind(&CPU::LDX, this);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xB6: //Zero Page,Y LDX
			executeInstruction = std::bind(&CPU::LDX, this);
			tickFunction = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, reg.Y);
			break;
		case 0xAE: //Absolute LDX
			executeInstruction = std::bind(&CPU::LDX, this);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xBE: //Absolute,Y LDX
			executeInstruction = std::bind(&CPU::LDX, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.Y);
			break;
		case 0xA0: //Immediate LDY
			executeInstruction = std::bind(&CPU::LDY, this);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xA4: //Zero Page LDY
			executeInstruction = std::bind(&CPU::LDY, this);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xB4: //Zero Page,X LDY
			executeInstruction = std::bind(&CPU::LDY, this);
			tickFunction = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, reg.X);
			break;
		case 0xAC: //Absolute LDY
			executeInstruction = std::bind(&CPU::LDY, this);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xBC: //Absolute,X LDY
			executeInstruction = std::bind(&CPU::LDY, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.X);
			break;
		case 0x4A: //Accumulator LSR
			executeInstruction = std::bind(&CPU::LSR, this);
			tickFunction = std::bind(&CPU::accumulator, this, executeInstruction);
			break;
		case 0x46: //Zero Page LSR
			executeInstruction = std::bind(&CPU::LSR, this);
			tickFunction = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0x56: //Zero Page,X LSR
			executeInstruction = std::bind(&CPU::LSR, this);
			tickFunction = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0x4E: //Absolute LSR
			executeInstruction = std::bind(&CPU::LSR, this);
			tickFunction = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0x5E: //Absolute,X LSR
			executeInstruction = std::bind(&CPU::LSR, this);
			tickFunction = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0xEA: //Implied NOP
			executeInstruction = [](){};
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x09: //Immediate ORA
			executeInstruction = std::bind(&CPU::ORA, this);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0x05: //Zero Page ORA
			executeInstruction = std::bind(&CPU::ORA, this);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0x15: //Zero Page,X ORA
			executeInstruction = std::bind(&CPU::ORA, this);
			tickFunction = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, reg.X);
			break;
		case 0x0D: //Absolute ORA
			executeInstruction = std::bind(&CPU::ORA, this);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0x1D: //Absolute,X ORA
			executeInstruction = std::bind(&CPU::ORA, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.X);
			break;
		case 0x19: //Absolute,Y ORA
			executeInstruction = std::bind(&CPU::ORA, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.Y);
			break;
		case 0x01: //Indirect,X ORA
			executeInstruction = std::bind(&CPU::ORA, this);
			tickFunction = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0x11: //Indirect,Y ORA
			executeInstruction = std::bind(&CPU::ORA, this);
			tickFunction = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0x48: //Implied PHA
			tickFunction = std::bind(&CPU::PHA, this);
			break;
		case 0x08: //Implied PHP
			tickFunction = std::bind(&CPU::PHP, this);
			break;
		case 0x68: //Implied PLA
			tickFunction = std::bind(&CPU::PLA, this);
			break;
		case 0x28: //Implied PLP
			tickFunction = std::bind(&CPU::PLP, this);
			break;
		case 0x2A: //Accumulator ROL
			executeInstruction = std::bind(&CPU::ROL, this);
			tickFunction = std::bind(&CPU::accumulator, this, executeInstruction);
			break;
		case 0x26: //Zero Page ROL
			executeInstruction = std::bind(&CPU::ROL, this);
			tickFunction = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0x36: //Zero Page,X ROL
			executeInstruction = std::bind(&CPU::ROL, this);
			tickFunction = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0x2E: //Absolute ROL
			executeInstruction = std::bind(&CPU::ROL, this);
			tickFunction = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0x3E: //Absolute,X ROL
			executeInstruction = std::bind(&CPU::ROL, this);
			tickFunction = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0x6A: //Accumulator ROR
			executeInstruction = std::bind(&CPU::ROR, this);
			tickFunction = std::bind(&CPU::accumulator, this, executeInstruction);
			break;
		case 0x66: //Zero Page ROR
			executeInstruction = std::bind(&CPU::ROR, this);
			tickFunction = std::bind(&CPU::zeroPage_RMW, this, executeInstruction);
			break;
		case 0x76: //Zero Page,X ROR
			executeInstruction = std::bind(&CPU::ROR, this);
			tickFunction = std::bind(&CPU::zeroPageX_RMW, this, executeInstruction);
			break;
		case 0x6E: //Absolute ROR
			executeInstruction = std::bind(&CPU::ROR, this);
			tickFunction = std::bind(&CPU::absolute_RMW, this, executeInstruction);
			break;
		case 0x7E: //Absolute,X ROR
			executeInstruction = std::bind(&CPU::ROR, this);
			tickFunction = std::bind(&CPU::absoluteX_RMW, this, executeInstruction);
			break;
		case 0x40: //Implied RTI
			tickFunction = std::bind(&CPU::RTI, this);
			break;
		case 0x60: //Implied RTS
			tickFunction = std::bind(&CPU::RTS, this);
			break;
		case 0xE9: //Immediate SBC
			executeInstruction = std::bind(&CPU::SBC, this);
			tickFunction = std::bind(&CPU::immediate, this, executeInstruction);
			break;
		case 0xE5: //Zero Page SBC
			executeInstruction = std::bind(&CPU::SBC, this);
			tickFunction = std::bind(&CPU::zeroPage, this, executeInstruction);
			break;
		case 0xF5: //Zero Page,X SBC
			executeInstruction = std::bind(&CPU::SBC, this);
			tickFunction = std::bind(&CPU::zeroPageIndexed, this, executeInstruction, reg.X);
			break;
		case 0xED: //Absolute SBC
			executeInstruction = std::bind(&CPU::SBC, this);
			tickFunction = std::bind(&CPU::absolute, this, executeInstruction);
			break;
		case 0xFD: //Absolute,X SBC
			executeInstruction = std::bind(&CPU::SBC, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.X);
			break;
		case 0xF9: //Absolute,Y SBC
			executeInstruction = std::bind(&CPU::SBC, this);
			tickFunction = std::bind(&CPU::absoluteIndexed, this, executeInstruction, reg.Y);
			break;
		case 0xE1: //Indirect,X SBC
			executeInstruction = std::bind(&CPU::SBC, this);
			tickFunction = std::bind(&CPU::indirectX, this, executeInstruction);
			break;
		case 0xF1: //Indirect,Y SBC
			executeInstruction = std::bind(&CPU::SBC, this);
			tickFunction = std::bind(&CPU::indirectY, this, executeInstruction);
			break;
		case 0x38: //Implied SEC
			executeInstruction = std::bind(&CPU::set_carry, this, 1);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xF8: //Implied SED
			executeInstruction = std::bind(&CPU::set_decimal, this, 1);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x78: //Implied SEI
			executeInstruction = std::bind(&CPU::set_interrupt, this, 1);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x85: //Zero Page STA
			tickFunction = std::bind(&CPU::zeroPage_Store, this, reg.AC);
			break;
		case 0x95: //Zero Page,X STA
			tickFunction = std::bind(&CPU::zeroPageIndexed_Store, this, reg.AC, reg.X);
			break;
		case 0x8D: //Absolute STA
			tickFunction = std::bind(&CPU::absolute_Store, this, reg.AC);
			break;
		case 0x9D: //Absolute,X STA
			tickFunction = std::bind(&CPU::absoluteIndexed_Store, this, reg.AC, reg.X);
			break;
		case 0x99: //Absolute,Y STA
			tickFunction = std::bind(&CPU::absoluteIndexed_Store, this, reg.AC, reg.Y);
			break;
		case 0x81: //Indirect,X STA
			tickFunction = std::bind(&CPU::indirectX_Store, this, reg.AC);
			break;
		case 0x91: //Indirect,Y STA
			tickFunction = std::bind(&CPU::indirectY_Store, this, reg.AC);
			break;
		case 0x86: //Zero Page STX
			tickFunction = std::bind(&CPU::zeroPage_Store, this, reg.X);
			break;
		case 0x96: //Zero Page,Y STX
			tickFunction = std::bind(&CPU::zeroPageIndexed_Store, this, reg.X, reg.Y);
			break;
		case 0x8E: //Absolute STX
			tickFunction = std::bind(&CPU::absolute_Store, this, reg.X);
			break;
		case 0x84: //Zero Page STY
			tickFunction = std::bind(&CPU::zeroPage_Store, this, reg.Y);
			break;
		case 0x94: //Zero Page,X STY
			tickFunction = std::bind(&CPU::zeroPageIndexed_Store, this, reg.Y, reg.X);
			break;
		case 0x8C: //Absolute STY
			tickFunction = std::bind(&CPU::absolute_Store, this, reg.Y);
			break;
		case 0xAA: //Implied TAX
			executeInstruction = std::bind(&CPU::TAX, this);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xA8: //Implied TAY
			executeInstruction = std::bind(&CPU::TAY, this);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0xBA: //Implied TSX
			executeInstruction = std::bind(&CPU::TSX, this);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x8A: //Implied TXA
			executeInstruction = std::bind(&CPU::TXA, this);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x9A: //Implied TXS
			executeInstruction = std::bind(&CPU::TXS, this);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		case 0x98: //Implied TYA
			executeInstruction = std::bind(&CPU::TYA, this);
			tickFunction = std::bind(&CPU::implied, this, executeInstruction);
			break;
		default:
			throw UnkownOPCode(currentOP, cycleCount, totalCycles, reg.PC);
	}
	tickFunction();
}

void CPU::debug()
{
	if(totalCycles > 0)
		debugInfo.print(log);
	// debugInfo.setInfo(currentOP, reg, totalCycles, ppu.dot, ppu.scanline);
	debugInfo.setInfo(currentOP, reg, totalCycles, 0, 0);
}