#ifndef MELTDOWN_GUARDIAN_H
#define MELTDOWN_GUARDIAN_H

#include <stdint.h>
#include "msrop.h"

#define LLC_REF  0x4f2e
#define LLC_MISS 0x412e
#define BRANCH_INS_RETIRED  0x00c4
#define BRANCH_MISS_RETIRED 0x00c5

#define MEM_LOAD_UOPS_RETIRED 0xd1
#define L1_HIT                0x01
#define L2_HIT                0x02
#define L1_MISS               0x08
#define L2_MISS               0x10
#define HITM                  0x20
#define WCB_HIT               0x40
#define DRAM_HIT              0x80

#define PE(event_sel, umask) ((event_sel << 8) | umask)

#define IA32_DS_AREA       0x600
#define IA32_PEBS_ENABLE   0x3f1
#define PERF_GLOBAL_CTRL   0x38f
#define IA32_PMC(x)        (0x0c1 + x)
#define IA32_PERFEVTSEL(x) (0x186 + x)

#define PMC_MAX (~0x0UL >> 16)
#define SAMPLE_FREQ(x) (PMC_MAX + 1 - x)

struct pebs_rec {
    uint64_t rflags;                  // 0x00H
    uint64_t rip;                     // 0x08H
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;                     // 0x88H
    /*
     *  Goldmont Performance Monitoring Unit Programming
     */
    uint64_t applicable_counters;     // 0x90H
    uint64_t data_linear_address;     // 0x98H
    uint64_t data_source_encoding;    // 0xa0H
    uint64_t latency_value;           // 0xa8H
    uint64_t eventing_rip;            // 0xb0H
    uint64_t reserved;                // 0xb8H
    uint64_t tsc;                     // 0xc0H
};

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
    uint32_t bts_interrupt_threshold;      // 0x18H
    struct pebs_rec *pebs_buffer_base;         // 0x20H
    struct pebs_rec *pebs_index;               // 0x28H
    struct pebs_rec *pebs_absolute_maximum;    // 0x30H
    struct pebs_rec *pebs_interrupt_threshold; // 0x38H
    uint64_t pebs_counter0_reset;      // 0x40H
    uint64_t pebs_counter1_reset;      // 0x48H
    uint64_t pebs_counter2_reset;      // 0x50H
    uint64_t pebs_counter3_reset;      // 0x58H
    uint64_t reserved;                 // 0x60H
};

void pebs_init(int nRecords, uint64_t *counter, uint64_t *reset_val);
void pebs_dump();
void pebs_clear();
void pebs_print();

#endif
