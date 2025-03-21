#include "capstone-sbi/sbi_capstone.h"

#define CAPSTONE_ENTER_C

.section .cap_text

#define SCRUB(rg) li rg, 0
#define LOOP 1: j 1b


test_direct_cap_access:
    csrr a0, CSR_CDCB
    PRINT(a0)
    CCSRRW(a0, CCSR_CDC, x0)
    PRINT(a0)
    lla a1, dom_stack
    PRINT(a1)
    SCC(a0, a0, a1)
    PRINT(a0)

    # shrink to [dom_stack, dom_stack + 32)
    addi a2, a1, 32
    SHRINK(a0, a1, a2)
    PRINT(a0)

    # store and load at [dom_stack, dom_stack + 8)
    li a1, 42
    sd a1, 0(a0)
    PRINT(a1)
    ld a2, 0(a0)
    PRINT(a2)
    SCRUB(a2)

    # this should not be affected by cdcb
    li a3, 0x1000
    csrw CSR_CDCB, a3

    ld a2, 0(a0)
    PRINT(a2) # should still read 42
    SCRUB(a2)

    # store and load at [dom_stack + 8, dom_stack + 16)
    # with offset
    sd a1, 8(a0)
    ld a2, 8(a0)
    PRINT(a2)
    SCRUB(a2)

    # now try going out of bound, should cause exception
    # sd a1, -8(a0)
    # ld a2, -8(a0)
    # PRINT(a2)

    LOOP

test_cdc_access:
    # current state:
    # PC is using a capability
    # CDC is set with cursor = 0, CDCB = 0
    # so can access through a raw pointer that is a physical address

    lla a0, dom_stack
    PRINT(a0)

    li a1, 42
    sd a1, 0(a0)
    ld a2, 0(a0)
    PRINT(a2)
    SCRUB(a2)

    # we can modify this by setting cdcb
    li a3, 0x1000
    csrw CSR_CDCB, a3

    # sfence.vma # this is done implicitly when setting cdcb

    PRINT(a0)
    # now this should access a different location
    ld a2, 0(a0)
    PRINT(a2)
    SCRUB(a2)

    sub a0, a0, a3
    PRINT(a0)
    # and this should access the same location
    ld a2, 0(a0)
    PRINT(a2)
    SCRUB(a2)

    LOOP

test_vm_access:
    # set up the capability-based page table

    # we store a test value
    lla a2, vm_test_data
    li a1, 42
    sd a1, 0(a2)

    # we don't need to use CDC cap. Take it out to
    # obtain the capabilities for building the page table

    CCSRRW(a0, CCSR_CDC, x0)
    PRINT(a0)

    # shrink first
    # currently this address must be page-aligned due to TLB limitation
    lla a2, vm_test_data # base address
    li a1, 16 * 4096
    add a3, a2, a1 # end address
    SHRINK(a0, a2, a3)
    PRINT(a0)

    # split in half, the first half for data
    # the second half for page table
    li a1, 8 * 4096
    add a3, a2, a1
    SPLIT(a4, a0, a3)
    PRINT(a4) # higher: for page table
    PRINT(a0) # lower: for data

    # a2 is still the target address

    li a1, 2 * 4096
    add a3, a3, a1
    # new split point
    SPLIT(a5, a4, a3)
    PRINT(a4) # lower
    PRINT(a5) # remaining

    li a2, 0x1000

    PRINT(a2)

    # map in a4
    srli t0, a2, 12 # t0 = a2 >> 12
    andi t0, t0, 511 # t0 = t0 & 511
    slli t0, t0, 4 # t0 = t0 << 4 (16 bytes per entry)
    PRINT(t0)
    CINCOFFSET(a4, a4, t0)
    PRINT(a4)
    PRINT(a2)
    PRINT(a0)
    STC(a0, a4, 0)

    # test load store of capabilities
    # li a5, 0
    # PRINT(a5)
    # LDC(a5, a4, 0)
    # PRINT(a5)
    # LOOP

    PRINT(a2)
    neg t0, t0
    CINCOFFSET(a4, a4, t0) # set cursor back to base

    PRINT(a4)

    PRINT(a2)
    # a0: moved
    # a4: leaf entry
    # a5: remaining

    add a3, a3, a1
    SPLIT(a0, a5, a3)
    PRINT(a0) # remaining
    PRINT(a5) # lower

    srli t0, a2, 21
    andi t0, t0, 511 # t0 = t0 & 511
    slli t0, t0, 4 # t0 = t0 << 4 (16 bytes per entry)
    CINCOFFSET(a5, a5, t0)
    STC(a4, a5, 0)
    neg t0, t0
    CINCOFFSET(a5, a5, t0)

    PRINT(a2)
    # a0: remaining
    # a4: moved
    # a5: 2nd level entry

    srli t0, a2, 30
    andi t0, t0, 511 # t0 = t0 & 511
    slli t0, t0, 4 # t0 = t0 << 4 (16 bytes per entry)
    CINCOFFSET(a0, a0, t0)
    STC(a5, a0, 0)
    neg t0, t0
    CINCOFFSET(a0, a0, t0)

    # a0: root page table

    CCSRRW(x0, CCSR_CATP, a0)

    # now loading from 0x1000 should give the test value
    ld a1, 0(a2)
    PRINT(a1)

    LOOP


.global cap_page_table_test
cap_page_table_test:
    # j test_direct_cap_access
    # j test_cdc_access
    j test_vm_access

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
.align 12
vm_test_data:
.zero 4096 * 16

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