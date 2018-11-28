#include "meltdown-guardian.h"

void
pebs_init(uint64_t *counter, uint64_t *reset_val)
{
}

void pebs_dump()
{
}

int
main()
{
    msr_init();
    msr_write(0, IA32_PERFEVTSEL(0), LLC_MISS);
    return 0;
}
