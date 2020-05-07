#ifndef NES_HPP
#define NES_HPP

#include "CPU.hpp"
#include "APU.hpp"
#include "PPU.hpp"
#include "Cartridge.hpp"
#include "Exceptions.hpp"
#include <cstdint>
#include <fstream>
#include <memory>

class NES
{
public:
	NES(const char* file, std::shared_ptr<char[]> frameBuffer);
	void prepareFrame();

private:
	std::shared_ptr<Cartridge> cart;
	std::shared_ptr<PPU> ppu;
	std::shared_ptr<APU> apu;
	std::shared_ptr<Controllers> controllers;
	std::unique_ptr<CPU> cpu;

	bool frameReady = false;

	//ROM Loading
	void initCart(const std::string& romPath);
};

#endif