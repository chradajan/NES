#include "../include/APU.hpp"

APU::APU()
{

}

uint8_t APU::readMemMappedReg(uint16_t address) const
{
    switch(address)
    {
        case 0x4000:
            return reg.SQ1_VOL;
        case 0x4001:
            return reg.SQ1_SWEEP;
        case 0x4002:
            return reg.SQ1_LO;
        case 0x4003:
            return reg.SQ1_HI;
        case 0x4004:
            return reg.SQ2_VOL;
        case 0x4005:
            return reg.SQ2_SWEEP;
        case 0x4006:
            return reg.SQ2_LO;
        case 0x4007:
            return reg.SQ2_HI;
        case 0x4008:
            return reg.TRI_LINEAR;
        case 0x4009:
            return reg.UNUSED1;
        case 0x400A:
            return reg.TRI_LO;
        case 0x400B:
            return reg.TRI_HI;
        case 0x400C:
            return reg.NOISE_VOL;
        case 0x400D:
            return reg.UNUSED2;
        case 0x400E:
            return reg.NOISE_LO;
        case 0x400F:
            return reg.NOISE_HI;
        case 0x4010:
            return reg.DMC_FREQ;
        case 0x4011:
            return reg.DMC_RAW;
        case 0x4012:
            return reg.DMC_START;
        case 0x4013:
            return reg.DMC_LEN;
        case 0x4015:
            return reg.SND_CHN;
        case 0x4017:
            return reg.JOY2;
    }
    return 0x00;
}

void APU::writeMemMappedReg(uint16_t address, uint8_t data)
{
    switch(address)
    {
        case 0x4000:
            reg.SQ1_VOL = data;
            break;
        case 0x4001:
            reg.SQ1_SWEEP = data;
            break;
        case 0x4002:
            reg.SQ1_LO = data;
            break;
        case 0x4003:
            reg.SQ1_HI = data;
            break;
        case 0x4004:
            reg.SQ2_VOL = data;
            break;
        case 0x4005:
            reg.SQ2_SWEEP = data;
            break;
        case 0x4006:
            reg.SQ2_LO = data;
            break;
        case 0x4007:
            reg.SQ2_HI = data;
            break;
        case 0x4008:
            reg.TRI_LINEAR = data;
            break;
        case 0x4009:
            reg.UNUSED1 = data;
            break;
        case 0x400A:
            reg.TRI_LO = data;
            break;
        case 0x400B:
            reg.TRI_HI = data;
            break;
        case 0x400C:
            reg.NOISE_VOL = data;
            break;
        case 0x400D:
            reg.UNUSED2 = data;
            break;
        case 0x400E:
            reg.NOISE_LO = data;
            break;
        case 0x400F:
            reg.NOISE_HI = data;
            break;
        case 0x4010:
            reg.DMC_FREQ = data;
            break;
        case 0x4011:
            reg.DMC_RAW = data;
            break;
        case 0x4012:
            reg.DMC_START = data;
            break;
        case 0x4013:
            reg.DMC_LEN = data;
            break;
        case 0x4015:
            reg.SND_CHN = data;
            break;
        case 0x4017:
            reg.JOY2 = data;
            break;
    }
}

APU::~APU()
{

}