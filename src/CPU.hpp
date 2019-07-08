#ifndef CPU_H
#define CPU_H

class CPU
{	
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

public:
	CPU();
	void CPU_TESTING();

};

#endif