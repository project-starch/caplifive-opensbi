#include "capstone-sbi/sbi_capstone.h"

#define CAPSTONE_ENTER_C

.section .cap_text

.global sbi_capstone_init
sbi_capstone_init:
    # disable interrupts
    csrr s0, mstatus
    li s1, -1
    xori s1, s1, MSTATUS_MIE
    and s0, s0, s1
    csrr s2, mepc
    csrw mstatus, s0

    # to avoid clobbering during capenter
    mv a4, a0
    mv a5, a1

    # enter C mode
    # TODO: this is supposed to invalidate all CSR setups
#ifdef CAPSTONE_ENTER_C
    lla s0, _cap_text_start
    lla s1, _cap_text_end
    CAPENTER(s0, s1)
#endif

    j sbi_capstone_init_cap

# code that is capability aware
.section .cap_text

sbi_capstone_init_cap:
#ifdef CAPSTONE_ENTER_C
    # initialise the stack region
    lla s0, dom_stack
    lla s1, dom_stack_end
    SPLIT(a2, a1, s0)
    SPLIT(a3, a2, s1)

    # store args to callee-saved regs
    mv s0, a4
    mv s1, a5

    # capabilities: a0, pc, a1, a2 (stack), a3
    CCSRRW(x0, CCSR_CSCRATCH, a2)
    call dom_init

    MOVC(a2, a3)
    call cap_env_init

    STC(gp, sp, -32 * 8 - 16)
    CCSRRW(x0, CCSR_CSCRATCH, sp)


#endif
    # #### hack to speed up ######
    # li a0, 0
    # li a1, 1
    # slli a1, a1, 63
    # GENCAP(a3, a0, a1)
    # CCSRRW(x0, CCSR_CPMP(0), a3)

    mv a0, s0
    mv a1, s1

    mret

.section .cap_text
#include "capstone-sbi/sbi_capstone.S"

# hardware interrupt

.section .cap_text
#include "sbi_capstone_dom.c.S"

.section .cap_int_handler_text
.weak _start
#include "capstone_int_handler.c.S"

.section .data
.align 4
dom_stack:
.zero 4096 * 16
.align 4
dom_stack_end:

.align 4
int_handler_stack:
.zero 4096 * 8
.align 4
int_handler_stack_end:

.align 4
int_handler_seal_region:
.zero 16 * 64
.align 4
int_handler_seal_region_end: