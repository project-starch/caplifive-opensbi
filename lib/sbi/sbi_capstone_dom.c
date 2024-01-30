#include "capstone-sbi/sbi_capstone.c"

#define C_WRITE_CSR(csr_name, v) __asm__("csrw " #csr_name ", %0":: "r"(v))
#define __dominit __attribute__((dominit))

__dominit void dom_init() { }

void cap_env_init(__linear void *cap0, __linear void *cap1, __linear void *cap2) {
    // regions
    region_n = 3;
    regions[1] = cap1;
    regions[2] = cap2;
    C_WRITE_CCSR(CCSR_CMMU, cap0);
    cmmu_region_id = 0; 

    // trap vector
    C_WRITE_CCSR(CCSR_CTVEC, _cap_trap_entry);

    // timer capabilities
    unsigned *cap = split_out_cap(SBI_MTIME_ADDR, 8, 0);
    mtime = cap;
    cap = split_out_cap(SBI_MTIMECMP_ADDR, 8, 0);
    mtimecmp = cap;

    // int handler domain
    __linear unsigned *cap_int_stack  = split_out_cap(int_handler_stack,
        int_handler_stack_end - int_handler_stack, 1);
    cap_int_stack = __setcursor(cap_int_stack, int_handler_stack_end);
    __linear unsigned *cap_int_seal = split_out_cap(int_handler_seal_region, 
        int_handler_seal_region_end - int_handler_seal_region, 1);
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
    C_WRITE_CCSR(CCSR_CIH, dom_ih);

    // interrupt delegation
    C_WRITE_CSR(CSR_CID, 0);

    // S-mode entry point
    unsigned mepc_val;
    C_READ_CSR(mepc, mepc_val);
    C_WRITE_CCSR(CCSR_CEPC, mepc_val);

    // re-enable interrupts
    __asm__("csrs mstatus, %0" :: "r"(MSTATUS_MIE));
}
