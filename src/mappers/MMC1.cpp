#include "MMC1.hpp"

MMC1::MMC1(HeaderData& header, std::ifstream& rom)
{
    mirroringIsVertical = header.Flags6 & 0x01;
    PRG_Bank_Count = header.PRG_ROM_SIZE;
    CHR_Bank_Count = header.CHR_ROM_SIZE;
    PRG_Banks.resize(PRG_Bank_Count);
    loadROM(rom);
}

uint8_t MMC1::readPRG(uint16_t address)
{
    if(address < 0x8000)
        throw IllegalROMRead("Attempted to read PRG ROM", address);

    address -= 0x8000;

    if(address < 0x4000) //First bank
        return PRG_Banks[PRG_Bank_1][address];
    else //Second bank
        return PRG_Banks[PRG_Bank_2][address - 0x4000];
}

void MMC1::writePRG(uint16_t address, uint8_t data)
{
    if(address < 0x8000)
        throw IllegalROMWrite("Attempted to write PRG ROM", address);

    if(data & 0x80)
    {
        PRG_Bank_2 = PRG_Bank_Count - 1;    //Fix 0xC000 - 0xFFFF to last PRG bank
        shiftRegister = 0x00;               //Reset register
        writeCounter = 0;                   //Prepare for first of 5 writes
    }
    else
    {
        shiftRegister |= ((data & 0x01) << writeCounter);

        if(writeCounter == 4)
        {
            writeCounter = 0;

            if(address < 0xA000) //Control Register
            {

            }
            else if(address < 0xC000) //CHR Bank 0
            {

            }
            else if(address < 0xE000) //CHR Bank 1
            {

            }
            else() //PRG Bank
            {

            }

            shiftRegister = 0x00;
        }      
    }
}

uint8_t MMC1::readCHR(uint16_t address)
{
    if(address > 0x1FFF)
        throw IllegalROMRead("Attempted to read CHR ROM", address);

    if(address < 0x1000) //First bank
        return CHR_Banks[CHR_Bank_2][address];
    else //Second bank
        return CHR_Banks[CHR_Bank_2][address - 0x1000];
}

void MMC1::writeCHR(uint16_t address, uint8_t data)
{

}

mirroringType MMC1::verticalMirroring() const
{
    return mirroringType;
}

void MMC1::loadROM(std::ifstream& rom)
{
    for(int i = 0; i < PRG_Bank_Count; ++i)
    {
        PRG_Banks[i].resize(0x4000); //Each bank is 16KB
        for(uint16_t j = 0x0000; j < 0x4000; ++j)
        {
            rom >> std::noskipws >> std::hex >> PRG_Banks[i][j];
        }
    }

    PRG_Bank_2 = PRG_Bank_Count - 1;

    for(int i = 0; i < CHR_Bank_Count * 2; ++i) //INES gives bank count as multiple of 8KB, but each mmc1 bank is half that
    {
        CHR_Banks[i].resize(0x1000); //Each bank is 4KB
        for(uint16_t j = 0x0000; j < 0x1000; ++j)
        {
            rom >> std::noskipws >> std::hex >> CHR_Banks[i][j];
        }
    }
}

MMC1::~MMC1()
{

}