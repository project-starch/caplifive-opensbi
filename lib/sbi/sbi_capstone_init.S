#include <sbi/riscv_encoding.h>
#include <sbi/sbi_capstone.h>

//#define CAPSTONE_ENTER_C

.section .text

.global sbi_capstone_init
sbi_capstone_init:
    # disable interrupts
    csrr s0, CSR_MSTATUS
    li s1, -1
    xori s1, s1, MSTATUS_MIE
    and s0, s0, s1
    csrw CSR_MSTATUS, s0

    # enter C mode
    # TODO: this is supposed to invalidate all CSR setups
#ifdef CAPSTONE_ENTER_C
    li s0, 1
    SETCAPMEM(s0)
#endif

    j sbi_capstone_init_cap

# code that is capability aware
.section .cap_text

sbi_capstone_init_cap:
    # TODO: this should in the end come from the genesis capability
#ifdef CAPSTONE_ENTER_C
    li s0, 0
    li s1, 0x100000000000
    GENCAP(a2, s0, s1)

    # a3 = trap vector PC cap
    lla s0, _cap_text_start
    lla s1, _cap_text_end
    GENCAP(a3, s0, s1)
    lla s0, _cap_trap_entry
    SCC(a3, a3, s0)

    # a4 = seal region
    lla s0, int_handler_seal_region
    lla s1, int_handler_seal_region_end
    GENCAP(a4, s0, s1)

    STC(a3, a4, 0)
    SEAL(a4, a4)

    CCSRRW(x0, CCSR_CIH, a4)
#endif

    # re-enable status; TODO: we should use a different interface for this?
    csrr s0, CSR_MSTATUS
    ori s0, s0, MSTATUS_MIE
    csrw CSR_MSTATUS, s0

    mret


_cap_trap_entry:
    PRINT(a0)
1:
    j 1b


.section .data
.align 4
int_handler_seal_region:
.zero 16 * 40
.align 4
int_handler_seal_region_end: