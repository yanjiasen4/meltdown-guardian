#ifndef MSROP_H
#define MSROP_H

#include <stdint.h>    // uintxx_t
#include <sys/types.h> // off_t

#define CPU_CORES 4

void msr_init();
void msr_finalize();
void msr_read(int core, off_t msr, uint64_t *val);
void msr_write(int core, off_t msr, uint64_t val);

static inline void msr_kread(uint32_t reg, uint64_t *msr_val_ptr)
{
    uint32_t msrl, msrh;

    asm volatile (" rdmsr ":"=a"(msrl), "=d"(msrh) : "c" (reg));
    *msr_val_ptr = ((uint64_t)msrh << 32U) | msrl;
}

static inline void msr_kwrite(uint32_t reg, uint64_t msr_val)
{
    uint32_t msrl, msrh;

    msrl = (uint32_t)msr_val;
    msrh = (uint32_t)(msr_val >> 32U);
    asm volatile (" wrmsr " : : "c" (reg), "a" (msrl), "d" (msrh));
}

#endif
