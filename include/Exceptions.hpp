#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <iostream>
#include <sstream>
#include <cstdint>
#include <iomanip>
#include <exception>

class Unsupported : virtual public std::exception
{
private:
	std::string errorMessage;
public:
	explicit Unsupported(std::string message) : errorMessage(message) {}
	virtual const char* what() const throw()
	{
		return errorMessage.c_str();
	}
};

class UnkownOPCode : virtual public std::exception
{
private:
	std::string errorMessage;
public:
	explicit UnkownOPCode(uint8_t code, int cycle, uint16_t pc)
	{
		std::stringstream ss;
		ss << "Bad OPCode: " << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << (uint)code << " @ PC = " << std::setw(4) << (uint)pc << std::endl;
		ss << "Cycle count: " << cycle << std::endl;
		errorMessage = ss.str();
	}
	virtual const char* what() const throw()
	{
		return errorMessage.c_str();
	}
};

class IllegalROMRead : virtual public std::exception
{
private:
	std::string errorMessage;
public:
	explicit IllegalROMRead(std::string romMessage, uint16_t address)
	{
		std::stringstream ss;
		ss << romMessage << std::endl;
		ss << "Attempted read address: " << std::hex << std::setw(4) << std::setfill('0') << (uint)address << std::endl;
		errorMessage = ss.str();
	}
	virtual const char* what() const throw()
	{
		return errorMessage.c_str();
	}
};

class IllegalROMWrite : virtual public std::exception
{
private:
	std::string errorMessage;
public:
	explicit IllegalROMWrite(std::string romMessage, uint16_t address, uint8_t data)
	{
		std::stringstream ss;
		ss << romMessage << std::endl;
		ss << "Attempted to write " << std::hex << std::setw(2) << std::setfill('0') << (uint)data << " to address " << std::setw(4) << (uint)address << std::endl;
		errorMessage = ss.str();
	}
	virtual const char* what() const throw()
	{
		return errorMessage.c_str();
	}
};

#endif