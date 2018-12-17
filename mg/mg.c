#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <asm/msr.h>

#define IA32_DS_AREA              0x600
#define PEBS_LD_LAT_THRESHOLD     0x3f6
#define IA32_PEBS_ENABLE          0x3f1
#define IA32_PERF_GLOBAL_OVF_CTRL 0x390
#define IA32_PERF_GLOBAL_STATUS   0x38e
#define IA32_PERF_GLOBAL_CTRL     0x38f
#define IA32_PERF_CAPABILITIES    0x345
#define IA32_PERFEVTSEL(x)        (0x186 + x)
#define IA32_PMC(x)               (0x0c1 + x)

#define PEBS_BUFFER_SIZE   (64 * 1024)
#define SAMPLE_FREQ 100003

#define LLC_REF   0x4f2e
#define LLC_MISS  0x412e

#define MEM_LOAD_RETIRED_L2_MISS 0X10d1
#define MEM_TRANS_RETIRED_PRECISE_STORE 0x2cd

#define EVTSEL_USER BIT(16)
#define EVTSEL_OS   0
#define EVTSEL_INT  BIT(20)
#define EVTSEL_EN   BIT(22)

struct pebs_rec_v1 {
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
};

struct pebs_rec_v2 {
    struct pebs_rec_v1 pebs_v1;
    uint64_t eventing_rip;
    uint64_t reserved;
};

struct pebs_rec_v3 {
    struct pebs_rec_v2 pebs_v2;
    uint64_t tsc;                     // 0xc0H
};

static int pebs_rec_size = sizeof(struct pebs_rec_v1);
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
    uint64_t bts_interrupt_threshold;  // 0x18H
    uint64_t pebs_buffer_base;         // 0x20H
    uint64_t pebs_index;               // 0x28H
    uint64_t pebs_absolute_maximum;    // 0x30H
    uint64_t pebs_interrupt_threshold; // 0x38H
    uint64_t pebs_counter0_reset;      // 0x40H
    uint64_t pebs_counter1_reset;      // 0x48H
    uint64_t pebs_counter2_reset;      // 0x50H
    uint64_t pebs_counter3_reset;      // 0x58H
    uint64_t reserved;                 // 0x60H
};

static DEFINE_PER_CPU(struct ds_area *, cpu_ds);

static int alloc_ds(void)
{
    uint64_t cap;
    unsigned pebs_num;
    struct ds_area *ds;

    /* check perf capability */
    rdmsrl(IA32_PERF_CAPABILITIES, cap);
    switch ((cap >> 8) & 0xf) {
        case 1:
            pebs_rec_size = sizeof(struct pebs_rec_v1);
            break;
        case 2:
            pebs_rec_size = sizeof(struct pebs_rec_v2);
            break;
        case 3:
            pebs_rec_size = sizeof(struct pebs_rec_v3);
            break;
        default:
            printk(KERN_INFO "Unsupported PEBS format\n");
            return false;
    }

    ds = kmalloc(sizeof(struct ds_area), GFP_KERNEL);
    memset(ds, 0, sizeof(struct ds_area));
    if (!ds) {
        return -1;
    }

    ds->pebs_buffer_base = (unsigned long)kmalloc(PEBS_BUFFER_SIZE, GFP_KERNEL);
    if (!ds->pebs_buffer_base) {
        kfree(ds);
        return -1;
    }
    memset((void *)ds->pebs_buffer_base, 0, PEBS_BUFFER_SIZE);
    pebs_num = PEBS_BUFFER_SIZE / pebs_rec_size;
    ds->pebs_index = ds->pebs_buffer_base;
    ds->pebs_absolute_maximum = ds->pebs_buffer_base + (pebs_num - 1) * pebs_rec_size;
    ds->pebs_interrupt_threshold = ds->pebs_buffer_base + (pebs_num - pebs_num / 10) * pebs_rec_size;
    ds->pebs_counter0_reset = -(long long)SAMPLE_FREQ;
    ds->pebs_counter1_reset = -(long long)SAMPLE_FREQ;
    ds->pebs_counter2_reset = -(long long)SAMPLE_FREQ;
    ds->pebs_counter3_reset = -(long long)SAMPLE_FREQ;

    __this_cpu_write(cpu_ds, ds);

    return 0;
}

static void pebs_cpu_init(void *arg)
{
    if (__this_cpu_read(cpu_ds) == NULL) {
        if (alloc_ds() < 0) {
            return;
        }
    }

    wrmsrl(IA32_PERF_GLOBAL_CTRL, 0x0);
    wrmsrl(IA32_PEBS_ENABLE, 0x0);
    wrmsrl(IA32_DS_AREA, __this_cpu_read(cpu_ds));

    /* Enable perf count 3 */
    wrmsrl(IA32_PEBS_ENABLE, (0x1 << 3) | (0x1L << 63));

    wrmsrl(IA32_PMC(3), -(long long)SAMPLE_FREQ);
    wrmsrl(IA32_PERFEVTSEL(3), MEM_TRANS_RETIRED_PRECISE_STORE | EVTSEL_EN | EVTSEL_USER | EVTSEL_OS);

    wrmsrl(IA32_PERF_GLOBAL_CTRL, 0xf);
}

static void pebs_cpu_print(void *arg)
{
    struct ds_area *ds = __this_cpu_read(cpu_ds);
    struct pebs_rec_v1 *pebs, *end;
    uint64_t ip, ac, dla, dse, lv;
    int count = 0;
    printk(KERN_INFO "pebs_base: %llx, pebs_index: %llx, pebs_max: %llx", ds->pebs_buffer_base, ds->pebs_index, ds->pebs_absolute_maximum);

    end = (struct pebs_rec_v1 *)ds->pebs_index;
    for (pebs = (struct pebs_rec_v1 *)ds->pebs_buffer_base; pebs < end;
            pebs = (struct pebs_rec_v1 *)((char *)pebs + pebs_rec_size)) {
        ip = pebs->rip;
        ac = pebs->applicable_counters;
        dse = pebs->data_source_encoding;
        dla = pebs->data_linear_address;
        lv = pebs->latency_value;
        printk(KERN_INFO "%d: ip: %llx, ac: %llx, dse: %llx, dla: %llx, lv: %llx\n",
                count++, ip, ac, dse, dla, lv);
    }
}

static void pebs_cpu_reset(void *arg)
{
    struct ds_area *ds = __this_cpu_read(cpu_ds);
    if (ds != NULL) {
        kfree(ds);
    }

    wrmsrl(IA32_PERFEVTSEL(3), 0x0);
    wrmsrl(IA32_PMC(3), 0x0);
}

static struct task_struct *tsk;

static int fr_disruptor_thread(void *data)
{
    int time_count = 0;
    do {
        printk(KERN_INFO "fr_disruptor running %d", ++time_count);
        on_each_cpu(pebs_cpu_print, NULL, 1);
        msleep(1000);
    } while (!kthread_should_stop());

    return time_count;
}

static int pebs_init(void)
{
    get_online_cpus();
    on_each_cpu(pebs_cpu_init, NULL, 1);
    put_online_cpus();
    return 0;
}

static int pebs_reset(void)
{
    get_online_cpus();
    on_each_cpu(pebs_cpu_reset, NULL, 1);
    put_online_cpus();
    return 0;
}

static int __init mg_init(void)
{
    pebs_init();

    tsk = kthread_run(fr_disruptor_thread, NULL, "fr_disruptor");
    if (IS_ERR(tsk)) {
        printk(KERN_INFO "create kthread failed!\n");
    } else {
        printk(KERN_INFO "create kthread success!\n");
    }
    printk(KERN_INFO "mg_init init\n");

    return 0;
}

static void __exit mg_exit(void)
{
    pebs_reset();

    printk(KERN_INFO "mg_exit exit\n");
}

module_init(mg_init);
module_exit(mg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yang Ming");
MODULE_DESCRIPTION("module test");
