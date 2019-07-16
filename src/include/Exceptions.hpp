#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <iostream>
#include <exception>
#include <string>

class BadRom : virtual public std::exception
{
private:
	char unrecognizedChar;
	std::string errorMessage;
public:
	explicit BadRom(char c);
	virtual const char* what() const throw()
	{
		return errorMessage.c_str();
	}
};

BadRom::BadRom(char c) : unrecognizedChar(c)
{
	errorMessage = "";
	errorMessage += c;
}

#endif