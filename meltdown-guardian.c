#include "meltdown-guardian.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h> // mmap
#include <sys/types.h>

struct ds_area *pds_area;
int npages = 0;

void
pebs_init(int nRecords, uint64_t *counter, uint64_t *reset_val)
{
   /*
    * Setting up the DS Save Area
    * To save branch records with the BTS buffer, the DS save area must be
    * set up in memory. See Section 18.4.4.1 for detail.
    *
    * 1. Create the DS buffer management information area in memory.
    * 2. Write the base linear address of the DS buffer management area into
    * the IA32_DS_AREA MSR.
    * 3. Set up the performance counter entry in the xAPIC LVT for fixed
    * delivery and edge sensitive. (See Section 10.5.1)
    * 4. Establish an interrupt handler in the IDT for the vector associated
    * with the performance counter entry in the xAPIC LVT.
    * 5. Write an interrupt service routine to handle the interrupt.
    */
    int pagesize = getpagesize();
    npages = 2 + (sizeof(struct pebs_rec)*nRecords) / pagesize;

    pds_area = mmap(NULL, npages * pagesize,
                PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_LOCKED | MAP_PRIVATE, -1, 0);

    if (pds_area == (void *)-1) {
        perror("mmap for pds_area failed");
        assert(0);
    }

    struct pebs_rec *ppebs = (struct pebs_rec *) ((uint64_t)pds_area + pagesize);

    // Setting up precise event buffering utilities
    pds_area->bts_buffer_base = 0;
    pds_area->bts_index       = 0;
    pds_area->bts_absolute_maximum = 0;
    pds_area->bts_interrupt_threshold = 0;
    pds_area->pebs_buffer_base = ppebs;
    pds_area->pebs_index       = ppebs;
    pds_area->pebs_absolute_maximum = ppebs + (nRecords - 1) * sizeof(struct pebs_rec);
    pds_area->pebs_interrupt_threshold = ppebs + (nRecords - 1) * sizeof(struct pebs_rec);
    pds_area->pebs_counter0_reset = reset_val[0];
    pds_area->pebs_counter1_reset = reset_val[1];
    pds_area->pebs_counter2_reset = reset_val[2];
    pds_area->pebs_counter3_reset = reset_val[3];
    pds_area->reserved = 0;

    // Requirements to program PEBS (see Table 18-11)
    msr_write(0, PERF_GLOBAL_CTRL, 0x0);
    msr_write(0, IA32_PEBS_ENABLE, 0x0);
    msr_write(0, IA32_DS_AREA, (uint64_t)pds_area);

    msr_write(0, IA32_PEBS_ENABLE, 0xf | ((uint64_t)0xf << 32));

    msr_write(0, IA32_PMC(0), reset_val[0]);
    msr_write(0, IA32_PMC(1), reset_val[1]);
    msr_write(0, IA32_PMC(2), reset_val[2]);
    msr_write(0, IA32_PMC(3), reset_val[3]);

    msr_write(0, IA32_PERFEVTSEL(0), 0x410000 | counter[0]);
    msr_write(0, IA32_PERFEVTSEL(1), 0x410000 | counter[1]);
    msr_write(0, IA32_PERFEVTSEL(2), 0x410000 | counter[2]);
    msr_write(0, IA32_PERFEVTSEL(3), 0x410000 | counter[3]);

    msr_write(0, PERF_GLOBAL_CTRL, 0xf);
}

void pebs_dump()
{
    static int initialized = 0;
	static int fd = 0;
	if(!initialized){
		initialized = 1;
		fd = open("./pebs.out", O_WRONLY | O_APPEND | O_CREAT );
		if(fd == -1){
			fprintf(stderr, "%s::%d file error.\n", __FILE__, __LINE__ );
			perror("Bye!\n");
			exit(-1);
		}
	}
	struct pebs_rec *p = pds_area->pebs_buffer_base;
	while(p != pds_area->pebs_index){
		write( fd, p, sizeof(struct pebs_rec) );
	}
	close(fd);
}

void pebs_clear()
{
    int ret;
    int pagesize = getpagesize();

    msr_write(0, PERF_GLOBAL_CTRL, 0x0);
    msr_write(0, IA32_PEBS_ENABLE, 0x0);
    msr_write(0, IA32_DS_AREA, 0x0);

    msr_write(0, IA32_PERFEVTSEL(0), 0x0);
    msr_write(0, IA32_PERFEVTSEL(1), 0x0);
    msr_write(0, IA32_PERFEVTSEL(2), 0x0);
    msr_write(0, IA32_PERFEVTSEL(3), 0x0);

    msr_write(0, IA32_PMC(0), 0x0);
    msr_write(0, IA32_PMC(1), 0x0);
    msr_write(0, IA32_PMC(2), 0x0);
    msr_write(0, IA32_PMC(3), 0x0);

    munmap(pds_area, npages * pagesize);
    msr_finalize();
}

void
pebs_print(){
    uint64_t pmc_val[4];
    msr_read(0, IA32_PMC(0), pmc_val);
    msr_read(0, IA32_PMC(1), pmc_val + 1);
    msr_read(0, IA32_PMC(2), pmc_val + 2);
    msr_read(0, IA32_PMC(3), pmc_val + 3);

    printf("%lu, %lu, %lu, %lu\n", pmc_val[0], pmc_val[1], pmc_val[2], pmc_val[3]);
    struct pebs_rec *p = pds_area->pebs_buffer_base;
	while(p != pds_area->pebs_index){
	    printf("[-----------------------------------------]\n\
                rflags: %lu, rip: %lu, rax: %lu, rbx: %lu\n\
                rcx: %lu, rdx: %lu, rsi: %lu, rdi: %lu\n\
                rdi: %lu, rbp: %lu, rsp: %lu\n\
                ac: %lu, dla: %lu, dse: %lu, lv: %lu\n\
                er: %lu, tsc: %lu",
                p->rflags, p->rip, p->rax, p->rbx,
                p->rcx, p->rdx, p->rsi, p->rdi,
                p->rdi, p->rbp, p->rsp,
                p->applicable_counters, p->data_linear_address, p->data_source_encoding,
                p->latency_value, p->eventing_rip, p->tsc);
    }
}

int
main()
{
    uint64_t events[4] = {
        PE(MEM_LOAD_UOPS_RETIRED, L2_MISS), 0, 0, 0
    };
    uint64_t reset_values[4] = {
        SAMPLE_FREQ(1000), 0, 0, 0
    };
    int i = 0;
    msr_init();
    printf("msr init finished\n");
    pebs_init(16, events, reset_values);

    printf("pebs init finished\n");
    printf("sleeping...\n");

    for (i = 0; i < 100; i++) {
        pebs_print();
        sleep(1);
    }

    sleep(3);
    pebs_dump();
    printf("pebs dump finished\n");
    pebs_clear();
    printf("pebs clear finished\n");
    return 0;
}
