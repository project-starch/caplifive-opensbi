#include "capstone-sbi/sbi_capstone.h"

#define CAPSTONE_ENTER_C

.section .text

.global sbi_capstone_init
sbi_capstone_init:
    # disable interrupts
    csrr s0, mstatus
    li s1, -1
    xori s1, s1, MSTATUS_MIE
    and s0, s0, s1
    csrr s2, mepc
    csrw mstatus, s0

    # enter C mode
    # TODO: this is supposed to invalidate all CSR setups
#ifdef CAPSTONE_ENTER_C
    li s0, 1
    SETCAPMEM(s0)
#endif

    CCSRRW(x0, CCSR_CEPC, s2)

    j sbi_capstone_init_cap

# code that is capability aware
.section .cap_text

sbi_capstone_init_cap:
    # TODO: this should in the end come from the genesis capability
#ifdef CAPSTONE_ENTER_C
    # a3 = trap vector PC cap
    lla s0, _cap_int_handler_text_start
    lla s1, _cap_int_handler_text_end
    GENCAP(a3, s0, s1)
    lla s0, _handle_hw_int_entry
    SCC(a3, a3, s0)

    # a4 = seal region
    lla s0, int_handler_seal_region
    lla s1, int_handler_seal_region_end
    GENCAP(a4, s0, s1)

    # store PC
    STC(a3, a4, 0)
    CINCOFFSETIMM(a4, a4, 16)

#define SAVE_REG_16(x) STC(x, a4, 0); CINCOFFSETIMM(a4, a4, 16)
#define SAVE_REG_8(x) sd x, 0(a4); CINCOFFSETIMM(a4, a4, 8)
#define SAVE_CSR(x) csrr t0, x; sd t0, 0(a4); CINCOFFSETIMM(a4, a4, 8)

    SAVE_REG_16(x0) // ra
    lla s0, int_handler_stack
    lla s1, int_handler_stack_end
    GENCAP(t0, s0, s1)
    SCC(t0, t0, s1)
    DELIN(t0)
    SAVE_REG_16(t0) // sp

    # GPRs x (31 - 2) + CCSRs x 4
    li t1, (31 - 2) + 4
1:
    addi t1, t1, -1
    SAVE_REG_16(x0)
    bne t1, x0, 1b

    # CSRs
    csrr t0, mstatus
    li t1, 3
    slli t1, t1, 38
    or t0, t0, t1
    SAVE_REG_8(t0)

    SAVE_CSR(mideleg)
    SAVE_CSR(medeleg)
    SAVE_CSR(mip)
    SAVE_CSR(mie)
    SAVE_CSR(mcause)
    SAVE_CSR(mtval)
    SAVE_REG_8(x0) # mtval2 is not available?
    SAVE_REG_8(x0) # mtinst is not available?
    SAVE_CSR(stvec)
    SAVE_CSR(scause)
    SAVE_CSR(stval)
    SAVE_CSR(sepc)
    SAVE_CSR(sscratch)
    SAVE_CSR(satp)

    SAVE_CSR(CSR_OFFSETMMU)

#undef SAVE_CSR
#undef SAVE_REG_8
#undef SAVE_REG_16

    SEAL(a4, a4)
    CCSRRW(x0, CCSR_CIH, a4)

    # delegate most of S- and U- mode H-interrupts
    li t0, MIP_SSIP | MIP_STIP | MIP_SEIP # | MIP_MTIP
    csrw CSR_CID, t0

    # re-enable status; TODO: we should use a different interface for this?
    csrr s0, mstatus
    ori s0, s0, MSTATUS_MIE
    csrw mstatus, s0

    lla s0, _cap_text_start
    lla s1, _cap_text_end
    GENCAP(a3, s0, s1)
    lla s0, _cap_trap_entry
    SCC(a3, a3, s0)
    CCSRRW(x0, CCSR_CTVEC, a3)

    # set up the global capability table
    lla s0, global_cap_table
    lla s1, global_cap_table_end
    GENCAP(a5, s0, s1)
    SCC(a5, a5, s0)

    li s0, SBI_MTIME_ADDR
    addi s1, s0, 8
    GENCAP(a4, s0, s1)
    SCC(a4, a4, s0)
    STC(a4, a5, 0)

    li s0, SBI_MTIMECMP_ADDR
    addi s1, s0, 8
    GENCAP(a4, s0, s1)
    SCC(a4, a4, s0)
    STC(a4, a5, 16)

#define SETUP_CAPTAB_ENTRY(name, idx) \
    lla s0, name; \
    lla s1, name ## _end; \
    GENCAP(a4, s0, s1); \
    SCC(a4, a4, s0); \
    STC(a4, a5, 16 * idx)

    SETUP_CAPTAB_ENTRY(domains, 2)
    SETUP_CAPTAB_ENTRY(regions, 3)
    SETUP_CAPTAB_ENTRY(dom_n, 4)
    SETUP_CAPTAB_ENTRY(region_n, 5)
    SETUP_CAPTAB_ENTRY(caller_dom, 6)
    SETUP_CAPTAB_ENTRY(caller_buf, 7)
    SETUP_CAPTAB_ENTRY(smode_initialised, 8)
    SETUP_CAPTAB_ENTRY(smode_saved_context, 9)

#undef SETUP_CAPTAB_ENTRY

    lla s0, dom_stack
    lla s1, dom_stack_end
    GENCAP(a3, s0, s1)
    SCC(a3, a3, s1)
    DELIN(a3) # conforming to the ABI
    STC(a5, a3, -32 * 8 -16) # store the cap to the global cap table at the bottom of the stack
    
    CCSRRW(x0, CCSR_CSCRATCH, a3)
    li a3, 0

    li s0, 0
    li s1, 0x10000000000
    GENCAP(a3, s0, s1)
    CCSRRW(x0, CCSR_CMMU, a3)
    li a3, 0

#endif

    mret


#include "capstone-sbi/sbi_capstone.S"

# hardware interrupt

.section .cap_int_handler_text

_handle_hw_int_entry:
    # save ra
    CINCOFFSETIMM(sp, sp, -16)
    STC(ra, sp, 0)

    # check CIC
    csrr a0, CSR_CIC
    slli a0, a0, 1
    srli a0, a0, 1
    call handle_hw_int

    # cause
    # ignore the external interrupts for now

# handle_int_ret:
    # inject V-interrupt
    lla t0, _handle_hw_int_entry

    LDC(ra, sp, 0)
    CINCOFFSETIMM(sp, sp, 16)
    RETURN(ra, t0, a0)

.section .data
.align 4
int_handler_seal_region:
.zero 16 * 64
.align 4
int_handler_seal_region_end:

.align 4
dom_stack:
.zero 4096 * 8
.align 4
dom_stack_end:


#define DEF_REGION(name, size) \
.align 4; \
name:; \
.zero size; \
name ## _end:

DEF_REGION(domains, 16 * CAPSTONE_MAX_DOM_N)
DEF_REGION(regions, 16 * CAPSTONE_MAX_REGION_N)
DEF_REGION(dom_n, 8)
DEF_REGION(region_n, 8)
DEF_REGION(caller_dom, 16)
DEF_REGION(caller_buf, 16)
DEF_REGION(smode_initialised, 8)
DEF_REGION(smode_saved_context, 16)
DEF_REGION(global_cap_table, 16 * 10)

#undef DEF_REGION

.section .cap_text
#include "capstone-sbi/sbi_capstone.c.S"

.section .cap_int_handler_text
#include "capstone_int_handler.c.S"

.section .data
.align 4
int_handler_stack:
.zero 4096 * 4
.align 4
int_handler_stack_end:
