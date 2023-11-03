#include <sbi/riscv_encoding.h>
#include <sbi/sbi_capstone.h>

#define CAPSTONE_ENTER_C

.section .text

.global sbi_capstone_init
sbi_capstone_init:
    # disable interrupts
    csrr s0, CSR_MSTATUS
    li s1, -1
    xori s1, s1, MSTATUS_MIE
    and s0, s0, s1
    csrr s2, mepc
    csrw CSR_MSTATUS, s0

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
    li s0, 0
    li s1, 0x100000000000
    GENCAP(a2, s0, s1)

    # a3 = trap vector PC cap
    lla s0, _cap_text_start
    lla s1, _cap_text_end
    GENCAP(a3, s0, s1)
    lla s0, _cap_int_entry
    SCC(a3, a3, s0)

    PRINT(s0)

    # a4 = seal region
    lla s0, int_handler_seal_region
    lla s1, int_handler_seal_region_end
    GENCAP(a4, s0, s1)

    STC(a3, a4, 0)
    SEAL(a4, a4)

    PRINT(a4)

    CCSRRW(x0, CCSR_CIH, a4)

    # re-enable status; TODO: we should use a different interface for this?
    csrr s0, CSR_MSTATUS
    ori s0, s0, MSTATUS_MIE
    csrw CSR_MSTATUS, s0

    // FIXME: we play with the C mode first
    PRINT(a3)
    PRINT(a4)

    
    // we just call into a4 instead of setting it up as a handler
    # CALL(a4, a4)

    # CALL(a4, a4)

    lla s0, _cap_text_start
    lla s1, _cap_text_end
    GENCAP(a3, s0, s1)
    lla s0, _cap_trap_entry
    SCC(a3, a3, s0)
    CCSRRW(x0, CCSR_CTVEC, a3)

    lla s0, scratch_region
    lla s1, scratch_region_end
    GENCAP(a3, s0, s1)
    CCSRRW(x0, CCSR_CSCRATCH, a3)

#endif

    mret

_cap_trap_entry:
    CCSRRW(s0, CCSR_CSCRATCH, s0)
    # now s0 is the scratch region
    # save context. TODO: assuming that there is no GPR containing a capability
    SAVE_REG(ra, s0)
    SAVE_REG(sp, s0)
    SAVE_REG(gp, s0)
    SAVE_REG(tp, s0)
    SAVE_REG(t0, s0)
    SAVE_REG(t1, s0)
    SAVE_REG(t2, s0)
    SAVE_REG(s1, s0)
    SAVE_REG(a0, s0)
    SAVE_REG(a1, s0)
    SAVE_REG(a2, s0)
    SAVE_REG(a3, s0)
    SAVE_REG(a4, s0)
    SAVE_REG(a5, s0)
    SAVE_REG(a6, s0)
    SAVE_REG(a7, s0)
    SAVE_REG(s2, s0)
    SAVE_REG(s3, s0)
    SAVE_REG(s4, s0)
    SAVE_REG(s5, s0)
    SAVE_REG(s6, s0)
    SAVE_REG(s7, s0)
    SAVE_REG(s8, s0)
    SAVE_REG(s9, s0)
    SAVE_REG(s10, s0)
    SAVE_REG(s11, s0)
    SAVE_REG(t3, s0)
    SAVE_REG(t4, s0)
    SAVE_REG(t5, s0)
    SAVE_REG(t6, s0)

    csrr t0, mcause

    li t1, CAUSE_SUPERVISOR_ECALL
    beq t0, t1, handle_trap_ecall

    j handle_trap_fail

handle_trap_ecall:

    li t0, SBI_EXT_BASE
    beq a7, t0, handle_base_ext

    j handle_trap_fail


handle_base_ext:
# very bruteforce implementation
    li t0, SBI_EXT_BASE_GET_SPEC_VERSION
    beq a6, t0, handle_base_version

    li t0, SBI_EXT_BASE_GET_IMP_ID
    beq a6, t0, handle_base_imp_id

    li t0, SBI_EXT_BASE_GET_IMP_VERSION
    beq a6, t0, handle_base_imp_version

    li t0, SBI_EXT_BASE_PROBE_EXT
    beq a6, t0, handle_base_probe_ext

    li t0, SBI_EXT_BASE_GET_MVENDORID
    beq a6, t0, handle_base_mvendorid

    li t0, SBI_EXT_BASE_GET_MARCHID
    beq a6, t0, handle_base_marchid

    li t0, SBI_EXT_BASE_GET_MIMPID
    beq a6, t0, handle_base_mimpid

    j handle_trap_fail

# the following handlers assume that the a0 and a1 contain the original values

handle_base_version:
    li a1, SBI_SPEC_VERSION
    j handle_trap_ret_succ

handle_base_imp_id:
    li a1, 0
    j handle_trap_ret_succ

handle_base_imp_version:
    li a1, 0
    j handle_trap_ret_succ

handle_base_probe_ext:
    li a1, 0 # no extension supported
    j handle_trap_ret_succ

handle_base_mvendorid:
    # let's allow this for now
    csrr a1, mvendorid
    j handle_trap_ret_succ

handle_base_marchid:
    csrr a1, marchid
    j handle_trap_ret_succ

handle_base_mimpid:
    csrr a1, marchid
    j handle_trap_ret_succ

handle_trap_ret_succ:
    SAVE_REG(a1, s0)
    li a0, 0
    SAVE_REG(a0, s0)

    # increment EPC
    CCSRRW(t0, CCSR_CEPC, x0)
    addi t0, t0, 4
    CCSRRW(x0, CCSR_CEPC, t0)

handle_trap_ret:
    RESTORE_REG(ra, s0)
    RESTORE_REG(sp, s0)
    RESTORE_REG(gp, s0)
    RESTORE_REG(tp, s0)
    RESTORE_REG(t0, s0)
    RESTORE_REG(t1, s0)
    RESTORE_REG(t2, s0)
    RESTORE_REG(s1, s0)
    RESTORE_REG(a0, s0)
    RESTORE_REG(a1, s0)
    RESTORE_REG(a2, s0)
    RESTORE_REG(a3, s0)
    RESTORE_REG(a4, s0)
    RESTORE_REG(a5, s0)
    RESTORE_REG(a6, s0)
    RESTORE_REG(a7, s0)
    RESTORE_REG(s2, s0)
    RESTORE_REG(s3, s0)
    RESTORE_REG(s4, s0)
    RESTORE_REG(s5, s0)
    RESTORE_REG(s6, s0)
    RESTORE_REG(s7, s0)
    RESTORE_REG(s8, s0)
    RESTORE_REG(s9, s0)
    RESTORE_REG(s10, s0)
    RESTORE_REG(s11, s0)
    RESTORE_REG(t3, s0)
    RESTORE_REG(t4, s0)
    RESTORE_REG(t5, s0)
    RESTORE_REG(t6, s0)

    CCSRRW(s0, CCSR_CSCRATCH, s0)

    mret

handle_trap_fail:
    RESTORE_REG(a6, s0)
    RESTORE_REG(a7, s0)
    PRINT(a6)
    PRINT(a7)

    li t0, 42
    PRINT(t0)
1:
    j 1b

_cap_int_entry:
    # li a5, 42
    # PRINT(a5)

    //lla s0, _cap_int_entry
    //RETURN(ra, s0)
    # CCSRRW(a1, CCSR_CEPC, x0)
    # PRINT(a1)
    # CINCOFFSETIMM(a1, a1, 2)
    # PRINT(a1)
    # CCSRRW(x0, CCSR_CEPC, a1)
    csrr t0, mcause
    slli t0, t0, 1
    srli t0, t0, 1

    # ignore the external interrupts for now
    li t1, 8
    bge t0, t1, handle_int_external

handle_int_fail:
    PRINT(t0)

    li s0, 32
    PRINT(s0)

    lla s0, _cap_int_entry
    RETURN(ra, s0)
1:
    j 1b

handle_int_external:
    // just clear mip
    PRINT(t0)
    li t1, 1
    sll t1, t1, t0
    not t1, t1
    csrr t2, mip
    and t2, t2, t1
    csrw mip, t2

handle_int_ret:
    lla t0, _cap_int_entry
    RETURN(ra, t0)

.section .data
.align 4
int_handler_seal_region:
.zero 16 * 40
.align 4
int_handler_seal_region_end:

.align 4
scratch_region:
.zero 16 * 40
.align 4
scratch_region_end:

