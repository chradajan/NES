#ifndef NROM_H
#define NROM_H
#include <cstdint>
#include "../../include/Cartridge.hpp"
#include "../../include/Exceptions.hpp"
#include <array>
#include <vector>

class NROM : public virtual Cartridge
{
public:
	NROM(std::ifstream& rom, const std::array<uint8_t, 16>& header);
	uint8_t readPRG(uint16_t addr) override;
	void writePRG(uint16_t addr, uint8_t data) override;
	uint8_t readCHR(uint16_t addr) override;
	void writeCHR(uint16_t addr, uint8_t data) override;
	uint16_t nametableAddress(uint16_t addr) override;
private:
	std::array<uint8_t, 0x2000> PRG_RAM;
	std::vector<uint8_t> PRG_ROM;
	std::array<uint8_t, 0x2000> CHR_ROM;
	bool PRG_Mirroring;
	void loadROM(std::ifstream& rom);
};

#endif