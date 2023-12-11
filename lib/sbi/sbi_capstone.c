/**
 * NOTE: This file is to be compiled with Capstone-CC to generate sbi_capstone.S.
*/

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_capstone.h>

/* Capstone-C defs */
#define __linear __attribute__((linear))
#define __dom __attribute__((dom))
#define __rev __attribute__((rev))
#define __domret __attribute__((domret))
#define __domasync __attribute__((domasync))
#define __domentry __attribute__((domentry))

#define C_READ_CSR(csr_name, v) __asm__("csrr %0, " #csr_name : "=r"(v))
#define C_READ_CCSR(ccsr_name, v) __asm__("ccsrrw(%0, " #ccsr_name ", x0)" : "=r"(v))
#define C_WRITE_CCSR(ccsr_name, v) __asm__("ccsrrw(x0, " #ccsr_name ", %0)" :: "r"(v))
#define C_SET_CURSOR(dest, cap, cursor) __asm__("scc(%0, %1, %2)" : "=r"(dest) : "r"(cap), "r"(cursor))
#define C_PRINT(v) __asm__ volatile(".insn r 0x5b, 0x1, 0x43, x0, %0, x0" :: "r"(v))
#define C_GEN_CAP(dest, base, end) __asm__(".insn r 0x5b, 0x1, 0x40, %0, %1, %2" : "=r"(dest) : "r"(base), "r"(end));

unsigned mtime;
unsigned mtimecmp;

static unsigned create_domain(unsigned base_addr, unsigned code_size,
                          unsigned tot_size, unsigned entry_offset)
{
    // alignment requirement
    code_size = (((code_size - 1) >> 4) + 1) << 4;
    __linear void *mem_l, *dom_code, *dom_data, *mem_r;
    __linear void **dom_seal;
    // READ_CCSR(cmmu, mem_l);

    // FIXME: cmmu is not working properly; we cheat for now
    // mint a capability for the memory region
    C_GEN_CAP(dom_code, base_addr, base_addr + tot_size);

    // dom_code = __split(mem_l, base_addr);
    // mem_r = __split(dom_code, base_addr + tot_size);

    dom_seal = __split(dom_code, base_addr + code_size);
    dom_data = __split(dom_seal, base_addr + code_size + (16 * 64));

    int i;
    for(i = 0; i < 64; i += 1) {
        dom_seal[i] = 0;
    }

    C_SET_CURSOR(dom_code, dom_code, base_addr + entry_offset);

    // construct the sealed region of the domain
    dom_seal[0] = dom_code;
    dom_seal[2] = dom_data;
    dom_seal[3] = 3 << 34;

    __dom void *dom = __seal(dom_seal);

    // PRINT(dom);

    unsigned res;
    __domcall(dom, &res);

    // WRITE_CCSR(cmmu, mem_l);


    return res;
}

// SBI implementation
unsigned handle_trap_ecall(unsigned arg0, unsigned arg1,
                           unsigned arg2, unsigned arg3,
                           unsigned arg4, unsigned arg5,
                           unsigned func_code, unsigned ext_code) {
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
                    C_READ_CSR(mvendorid, res);
                    break;
                case SBI_EXT_BASE_GET_MARCHID:
                    C_READ_CSR(marchid, res);
                    break;
                case SBI_EXT_BASE_GET_MIMPID:
                    C_READ_CSR(mimpid, res);
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
        case SBI_EXT_CAPSTONE:
            switch(func_code) {
                case SBI_EXT_CAPSTONE_DOM_CREATE:
                    res = create_domain(arg0, arg1, arg2, arg3);
                    break;
                default:
                    err = 1;
            }
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
