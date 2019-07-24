#ifndef NROM_H
#define NROM_H
#include <cstdint>
#include "../include/Cartridge.hpp"
#include "../include/Types.hpp"
#include "../include/Exceptions.hpp"

class NROM : public Cartridge
{
public:
	NROM(HeaderData& header, std::ifstream& rom);
	uint8_t readPRG(uint16_t address) const;
	void writePRG(uint16_t address, uint8_t data);
	uint8_t readCHR(uint16_t address) const;
	void writeCHR(uint16_t address, uint8_t data);
	bool verticalMirroring() const;
	~NROM();
private:
	uint8_t* PRG_ROM;
	uint8_t* CHR_ROM;
	bool PRG_Mirroring;
	void loadROM(std::ifstream& rom);
};

#endif