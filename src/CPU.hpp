#ifndef CPU_H
#define CPU_H

class CPU
{	
public:
	CPU(const char* file);
	void CPU_TESTING();

private:
	struct CPU_Registers
	{
		char A;
		char X;	
		char Y;
		short int PC;
		char S;
		char P;
	};

	CPU_Registers* registers;
	char* memory;

	void LoadROM(const char* file);
};

#endif