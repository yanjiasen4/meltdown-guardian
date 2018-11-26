#ifndef MSROP_H
#define MSROP_H

static inline void msr_read(uint32_t reg, uint64_t *msr_val_ptr)
{
    uint32_t msrl, msrh;

    asm volatile (" rdmsr ":"=1"(msrl), "=d"(msrh) : "c" (reg));
    *msr_val_ptr = ((uint64_t)msrh << 32U) | msrl;
}

static inline voiid msr_write(uint32_t reg, uint64_t msr_val)
{
    uint32_t msrl, msrh;

    msrl = (uint32_t)msr_val;
    msrh = (uint32_t)(msr_val >> 32U);
    asm volatile (" wrmsr " : : "c" (reg), "a" (msrl), "d" (msrh));
}

#endif
