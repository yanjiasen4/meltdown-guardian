#ifndef MELTDOWN_GUARDIAN_H
#define MELTDOWN_GUARDIAN_H

#include <sys/types.h>

#define LLC_REF  0x4f2e
#define LLC_MISS 0x412e
#define BRANCH_INS_RETIRED  0x00c4
#define BRANCH_MISS_RETIRED 0x00c5

#define IA32_DS_AREA       0x600
#define IA32_PEBS_ENABLE   0x3f1
#define PERF_GLOBAL_CTRL   0x38f
#define IA32_PMC(x)        (0x0c1 + x)
#define IA32_PERFEVTSEL(x) (0x186 + x)

/*
 * Debug_Store Save Area
 * 64 bit format of the DS Save Area
 * When DTES64 = 1 (CPUID.1.EXC[2] = 1), the structure of the DS save area
 * is as the following. Get more detail at Intel SDM Document Vol. 3B
 * Chapter 17.49
 */
struct ds_area {
    uint64_t bts_buffer_base;          // 0x00H
    uint64_t bts_index;                // 0x08H
    uint64_t bts_absolute_maximum;     // 0x10H
    uint32_t interrupt_threshold;      // 0x18H
    uint64_t pebs_buffer_base;         // 0x20H
    uint64_t pebs_index;               // 0x28H
    uint64_t pebs_absolute_maximum;    // 0x30H
    uint32_t pebs_interrupt_threshold; // 0x38H
    uint64_t pebs_counter0_reset;      // 0x40H
    uint64_t pebs_counter1_reset;      // 0x48H
    uint64_t pebs_counter2_reset;      // 0x50H
    uint64_t pebs_counter3_reset;      // 0x58H
    uint64_t reserved;                 // 0x60H
};

void pebs_init(uint64_t *counter, uinte64_t *reset_val);
void pebs_dump();

#endif
