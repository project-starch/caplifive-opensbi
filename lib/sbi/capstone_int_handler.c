#include "capstone-sbi/sbi_capstone.h"

#define __domasync __attribute__((domasync))
#define __domentry __attribute__((domentry))
#define __domreentry __attribute__((domreentry))
#define __dom __attribute__((dom))
#define __domret __attribute__((domret))

#define C_READ_CSR(csr_name, v) __asm__("csrr %0, " #csr_name : "=r"(v))
#define C_READ_CCSR(ccsr_name, v) __asm__("ccsrrw(%0, " #ccsr_name ", x0)" : "=r"(v))
#define C_WRITE_CCSR(ccsr_name, v) __asm__("ccsrrw(x0, " #ccsr_name ", %0)" :: "r"(v))
#define C_SET_CURSOR(dest, cap, cursor) __asm__("scc(%0, %1, %2)" : "=r"(dest) : "r"(cap), "r"(cursor))
#define C_PRINT(v) __asm__ volatile(".insn r 0x5b, 0x1, 0x43, x0, %0, x0" :: "r"(v))
#define C_GEN_CAP(dest, base, end) __asm__(".insn r 0x5b, 0x1, 0x40, %0, %1, %2" : "=r"(dest) : "r"(base), "r"(end));
#define capstone_error(err_code) do { C_PRINT(CAPSTONE_ERR_STARTER); C_PRINT(err_code); while(1); } while(0)
#define cap_base(cap) __capfield((cap), 3)
#define cap_end(cap) __capfield((cap), 4)

static __domret void *main_thread;
/* is the currently running thread a non-main thread? */
static unsigned nonmain_running;


static void handle_hw_int(__domasync void *ra) {
    unsigned cause;
    __asm__ volatile ("csrr %0, cic" : "=r"(cause));
    cause = (cause << 1) >> 1;
    // clear pending bit
    __asm__ volatile ("csrc cis, %0" :: "r"(1 << cause));

    if(nonmain_running) {
        // return to main thread
        nonmain_running = 0;
        // TODo: need to pass ra back
        __domreturn(main_thread, __int_handler_entry_reentry, 1 << cause);
    } else {
        __domreturn(ra, __int_handler_entry_reentry, 1 << cause);
    }
}

/* handler domain has been called synchronously */
/* handle the request */
static void handle_ih_call(__domret void *ra, unsigned request_no, __dom void *arg) {
    // only the main thread can call the interrupt handler
    if(nonmain_running) {
        __domreturn(ra, __int_handler_entry_reentry, 0);
    }
    switch(request_no) {
        case CAPSTONE_IHI_THREAD_SPAWN:
            // save ra somewhere
            main_thread = ra;
            nonmain_running = 1;
            __domreturn(arg, __int_handler_entry_reentry, 0);
            while(1); /* should not reach here */
        case CAPSTONE_IHI_THREAD_YIELD:
            while(1);
        default:
            while(1);
    }
}

static __domentry __domreentry void int_handler_entry(__domret void *ra, unsigned request_no, __dom void *arg) {
    if(__capfield(ra, 6) == 0) {
        // synchronous
        handle_ih_call(ra, request_no, arg);
    } else {
        handle_hw_int(ra);
    }
}