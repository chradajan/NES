#include <iostream>
#include "CPU.hpp"

int main()
{
	//CPU cpu("DonkeyKong.txt");
	CPU cpu("test.txt");
	cpu.tick();
	cpu.CPU_TESTING();

	return 0;
}