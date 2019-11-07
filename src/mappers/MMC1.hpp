#ifndef MMC1_H
#define MMC1_h

#include <cstdint>
#include <vector>
#include "../include/Cartridge.hpp"
#include "../include/Exceptions.hpp"

class MMC1 : public Cartridge
{
public:
    MMC1(HeaderData& header, std::ifstream& rom);
    uint8_t readPRG(uint16_t address);
    void writePRG(uint16_t address, uint8_t data);
    uint8_t readCHR(uint16_t address);
    void writeCHR(uint16_t address, uint8_t data);
    bool verticalMirroring() const;
    ~MMC1();
private:
    void loadROM(std::ifstream& rom);
    bool containsRAM;
    int writeCounter = 0;
    int PRG_Bank_Count, CHR_Bank_Count;
    int PRG_Bank_1 = 0, PRG_Bank_2;
    int CHR_Bank_1, CHR_Bank_2;
    int RAM_Bank;
    uint8_t shiftRegister = 0x00;
    std::vector<std::vector<uint8_t>> PRG_Banks; 
    std::vector<std::vector<uint8_t>> CHR_Banks;
    std::vector<std::vector<uint8_t>> RAM_Banks;
};

#endif