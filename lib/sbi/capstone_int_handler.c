#include "capstone-sbi/sbi_capstone.h"

#define __domasync __attribute__((domasync))
#define __domentry __attribute__((domentry))
#define __dom __attribute__((dom))


#define MAX_SCHED_DOMAIN_N 64

// TODO: these need to be shared with library domains
static void* __dom domains[MAX_SCHED_DOMAIN_N];
static unsigned domain_n;

static unsigned handle_hw_int(unsigned cause) {
    unsigned to_inject = 0;
    // clear pending bit
    __asm__ volatile ("csrc cis, %0" :: "r"(1 << cause));

    switch(cause) {
        case IRQ_M_TIMER:
            // mtimer should be reset here rather than in the M mode of
            // individual domains
            to_inject = 1 << cause;
            break;
        default:;
    }

    return to_inject;
}
