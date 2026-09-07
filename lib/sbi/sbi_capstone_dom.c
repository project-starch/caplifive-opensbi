#include "capstone-sbi/sbi_capstone.c"

#define C_WRITE_CSR(csr_name, v) __asm__("csrw " #csr_name ", %0":: "r"(v))
#define __dominit __attribute__((dominit))

__dominit void dom_init() { }

void cap_env_init(__linear void *cap0, __linear void *cap1, __linear void *cap2) {
    int i;

    // regions
    region_n = 3;
    C_WRITE_CCSR(cpmp(0), cap0);
    C_WRITE_CCSR(cpmp(1), cap1);
    C_WRITE_CCSR(cpmp(2), cap2);
    cpmp_region[0] = 0;
    cpmp_region[1] = 1;
    cpmp_region[2] = 2;
    region_cpmp[0] = 0;
    region_cpmp[1] = 1;
    region_cpmp[2] = 2;
    for(i = 3; i < CPMP_COUNT; i += 1) {
        cpmp_region[i] = -1;
    }
    for(i = 3; i < CAPSTONE_MAX_REGION_N; i += 1) {
        region_cpmp[i] = -1;
    }
    /* the three genesis regions are CPMP-resident and never appended: mark them live by hand
       and every other slot dead, exactly as the -1 loops above do (never rely on zero-init) */
    region_live[0] = 1;
    region_live[1] = 1;
    region_live[2] = 1;
    for(i = 3; i < CAPSTONE_MAX_REGION_N; i += 1) {
        region_live[i] = 0;
    }

    // // trap vector
    C_WRITE_CCSR(ctvec, _cap_trap_entry);

#ifdef CAPSTONE_TARGET_FPGA
    // UART capability, for making monitor errors visible on the console (I-4).
    // Minted exactly like the timer capabilities below: a non-linear cap split out
    // of the genesis region. CAPENTER hands a0 = [0, _cap_text_start) and the
    // firmware links at 0x8xxxxxxx, so 0x10000000 (ARIANE_UART_ADDR) falls inside
    // region 0 -- the same region 0x200bff8 (mtime) comes out of. Done FIRST so that
    // a failure in one of the splits below can itself be reported.
    __asm__ ("fence.i");
    unsigned *cap_uart = split_out_cap(CAPSTONE_UART_BASE, CAPSTONE_UART_LEN, 0);
    capstone_uart = cap_uart;
    capstone_uart_ready = 1;
#endif

    // timer capabilities
#ifdef CAPSTONE_TARGET_FPGA
    __asm__ ("fence.i");
#endif
    unsigned *cap = split_out_cap(SBI_MTIME_ADDR, 8, 0);
    mtime = cap;
#ifdef CAPSTONE_TARGET_FPGA
    __asm__ ("fence.i");
#endif
    cap = split_out_cap(SBI_MTIMECMP_ADDR, 8, 0);
    mtimecmp = cap;

    // int handler domain
#ifdef CAPSTONE_TARGET_FPGA
    __asm__ ("fence.i");
#endif
    __linear unsigned *cap_int_stack  = split_out_cap(int_handler_stack,
        int_handler_stack_end - int_handler_stack, 1);
#ifdef CAPSTONE_TARGET_FPGA
    __asm__ ("fence.i");
#endif
    cap_int_stack = __setcursor(cap_int_stack, int_handler_stack_end);
    __linear unsigned *cap_int_seal = split_out_cap(int_handler_seal_region,
        int_handler_seal_region_end - int_handler_seal_region, 1);
#ifdef CAPSTONE_TARGET_FPGA
    __asm__ ("fence.i");
#endif
    unsigned *cap_int_code = split_out_cap(_cap_int_handler_text_start,
        _cap_int_handler_text_end - _cap_int_handler_text_start, 0);
    cap_int_code = __setcursor(cap_int_code, __int_handler_entry_entry);

    __linear unsigned **cap_int_seal_cap_writer = cap_int_seal;
    *cap_int_seal_cap_writer = cap_int_code; cap_int_seal_cap_writer += 16;
    *cap_int_seal_cap_writer = 0; cap_int_seal_cap_writer += 16;
    *cap_int_seal_cap_writer = cap_int_stack; cap_int_seal_cap_writer += 16;
    cap_int_seal = cap_int_seal_cap_writer;

    unsigned csr_val;
    C_READ_CSR(mstatus, csr_val);
    *cap_int_seal = csr_val | (3 << 38); cap_int_seal += 8;
#define SAVE_CSR(csr_name) do { C_READ_CSR(csr_name, csr_val); *cap_int_seal = csr_val; cap_int_seal += 8; } while(0)
    SAVE_CSR(mideleg);
    SAVE_CSR(medeleg);
    SAVE_CSR(mip);
    SAVE_CSR(mie);
#undef SAVE_CSR

    __dom void *dom_ih = __seal(cap_int_seal);
    C_WRITE_CCSR(cih, dom_ih);

    // interrupt delegation
    C_WRITE_CSR(cid, 0);

    // S-mode entry point
    unsigned mepc_val;
    C_READ_CSR(mepc, mepc_val);
    C_WRITE_CCSR(cepc, mepc_val);

    // re-enable interrupts
    // __asm__("csrs mstatus, %0" :: "r"(MSTATUS_MIE));
#ifdef CAPSTONE_TARGET_QEMU
    __asm__("csrs mstatus, %0" :: "r"(MSTATUS_MIE));
#endif
}
