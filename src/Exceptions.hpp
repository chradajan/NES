#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <iostream>

class BadRom
{
public:
	BadRom()
	{
		std::cout << "Unexpected char in rom file" << std::endl;
	}
};

#endif