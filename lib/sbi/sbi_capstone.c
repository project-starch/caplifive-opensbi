/**
 * NOTE: This file is to be compiled with Capstone-CC to generate sbi_capstone.S.
*/

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_capstone.h>

#define READ_CSR(csr_name, v) __asm__("csrr %0, " #csr_name : "=r"(v))
#define PRINT(v) __asm__ volatile(".insn r 0x5b, 0x1, 0x43, x0, %0, x0" :: "r"(v))

unsigned mtime;
unsigned mtimecmp;

// SBI implementation
unsigned handle_trap_ecall(unsigned arg0, unsigned arg1,
                     unsigned ext_code, unsigned func_code) {
    // PRINT(ext_code);
    // PRINT(func_code);
    unsigned res = 0, err = 0;
    switch(ext_code) {
        case SBI_EXT_BASE:
            switch(func_code) {
                case SBI_EXT_BASE_GET_SPEC_VERSION:
                    res = SBI_SPEC_VERSION;
                    break;
                case SBI_EXT_BASE_GET_IMP_ID:
                case SBI_EXT_BASE_GET_IMP_VERSION:
                    res = 0;
                    break;
                case SBI_EXT_BASE_PROBE_EXT:
                    // we only have time extension
                    res = arg0 == SBI_EXT_TIME;
                    break;
                case SBI_EXT_BASE_GET_MVENDORID:
                    READ_CSR(mvendorid, res);
                    break;
                case SBI_EXT_BASE_GET_MARCHID:
                    READ_CSR(marchid, res);
                    break;
                case SBI_EXT_BASE_GET_MIMPID:
                    READ_CSR(mimpid, res);
                    break;
                default:
                    err = 1;
            }
            break;
        case SBI_EXT_TIME:
            if (func_code == SBI_EXT_TIME_SET_TIMER) {
                __asm__ volatile ("csrc mip, %0" :: "r"(MIP_STIP | MIP_MTIP));
                mtimecmp = arg0;
                __asm__ volatile ("csrs mie, %0" :: "r"(MIP_MTIP));
            } else {
                err = 1;
            }
            break;
        default:
            err = 1;
    }
    return res;
}

void handle_interrupt(unsigned int_code) {
    switch(int_code) {
        case IRQ_M_TIMER:
            __asm__ volatile ("csrc mie, %0" :: "r"(MIP_MTIP));
            __asm__ volatile ("csrs mip, %0" :: "r"(MIP_STIP));
            break;        
    }
}