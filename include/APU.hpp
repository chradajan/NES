#ifndef APU_HPP
#define APU_HPP

#include <cstdint>

class APU
{
public:
    APU();
    uint8_t readMemMappedReg(uint16_t address) const;
    void writeMemMappedReg(uint16_t address, uint8_t data);
    ~APU();

private:
    struct APU_Registers
    {
        uint8_t SQ1_VOL = 0x00;
        uint8_t SQ1_SWEEP = 0x00;
        uint8_t SQ1_LO = 0x00;
        uint8_t SQ1_HI = 0x00;
        uint8_t SQ2_VOL = 0x00;
        uint8_t SQ2_SWEEP = 0x00;
        uint8_t SQ2_LO = 0x00;
        uint8_t SQ2_HI = 0x00;
        uint8_t TRI_LINEAR = 0x00;
        uint8_t UNUSED1 = 0x00;
        uint8_t TRI_LO = 0x00;
        uint8_t TRI_HI = 0x00;
        uint8_t NOISE_VOL = 0x00;
        uint8_t UNUSED2 = 0x00;
        uint8_t NOISE_LO = 0x00;
        uint8_t NOISE_HI = 0x00;
        uint8_t DMC_FREQ = 0x00;
        uint8_t DMC_RAW = 0x00;
        uint8_t DMC_START = 0x00;
        uint8_t DMC_LEN = 0x00;
        uint8_t SND_CHN = 0x00;
        uint8_t JOY2 = 0x00;
    };
    APU_Registers reg;
};

#endif