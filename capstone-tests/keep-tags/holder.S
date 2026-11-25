/* holder.S: an S payload for OpenSBI's fw_jump (QEMU virt, the Capstone Sublet format with the trap
 * route) that holds a reference in every register across traps into M, and checks the firmware's
 * contract: a trap into M keeps the tags of S's registers (capstone-qemu tests/machine-trap-tags,
 * the same protocol from S under the firmware itself).
 *
 * S writes a paged Sublet table, selects it with srevroot, mints one NONLIN reference to page_a
 * with CSMINT and copies it into every register but the ones a case needs. Then:
 *   CASE 0 (timer): S arms the SBI timer with its own interrupts off and spins. The machine timer
 *     traps into M, which clears MTIE and sets STIP; the spin ends once sip.STIP reads 1 (bounded).
 *     Every register still holds the reference: CDERIVE from s2 must succeed (it refuses an
 *     untagged parent with 24), and LCC type reads NONLIN in each.
 *   CASE 1 (call): S calls sbi_get_spec_version (a7 = 0x10, a6 = 0). a0 and a1 come back as the
 *     call's integers (a0 = 0, LCC type SCALAR) and every other register keeps its reference.
 * Exit through QEMU's sifive_test: 0 for a pass; 32 + n when xn lost its reference; 2 when no
 * machine timer was seen; 3 when a0 is wrong; 128 + scause for a trap into S; 1 for any other failure.
 * -DSTRIP_S5 (CASE 0): the checker's control, s5 is made an integer after the timer: exit 53. */
#ifndef CASE
#error CASE required
#endif
#define CSR_SREVROOT 0x5c1
#define CSMINT(rd, slot, desc) .insn r 0x5b, 1, 0x50, rd, slot, desc
#define LDC(rd, base, off) .insn i 0x5b, 3, rd, base, off
#define MOVC(rd, src) .insn r 0x5b, 1, 0x0a, rd, src, zero
#define LCC(rd, src, field) .insn r 0x5b, 1, 4, rd, src, field
#define DERIVE(rd, parent, size) .insn r 0x5b, 1, 0x51, rd, parent, size
#define LCC_TYPE x1
#define TYPE_NONLIN 1
#define TYPE_SCALAR 7
#define TEST_DEV 0x100000
#define SPIN_MAX (1 << 26)
#define TICK 100000
#define SUBLET_TABLE_FLAGS 3
#define SUBLET_TABLE_MAGIC 0x3245474150444f4e
#define SUBLET_RECORDS 256

.macro EXIT_CODE reg
    li t6, TEST_DEV
    slli \reg, \reg, 16
    li t4, 0x3333
    or \reg, \reg, t4
    sw \reg, 0(t6)
9:  j 9b
.endm

/* Every listed register still holds a NONLIN reference, or exit 32 + n. */
.macro CHECK_NONLIN regs:vararg
    .irp n, \regs
    LCC(t6, x\n, LCC_TYPE)
    addi t6, t6, -TYPE_NONLIN
    beqz t6, 1f
    li t5, 32 + \n
    EXIT_CODE t5
1:
    .endr
.endm

    .option norvc
    .section .text
    .globl _start
_start:
    lla t0, s_trap
    csrw stvec, t0
    csrw sie, zero
    csrci sstatus, 2
    /* the table: root page node_table, two directory pages, one record page */
    lla t0, node_table
    li t1, SUBLET_TABLE_FLAGS
    slli t1, t1, 62
    li t2, SUBLET_RECORDS
    or t1, t1, t2
    sd t1, 0(t0)
    li t1, 2
    sd t1, 8(t0)
    li t1, SUBLET_TABLE_MAGIC
    sd t1, 32(t0)
    lla t1, dir1
    sd t1, 64(t0)
    lla t0, dir1
    lla t1, dir2
    sd t1, 0(t0)
    lla t0, dir2
    lla t1, records
    sd t1, 0(t0)
    lla t0, node_table
    csrw CSR_SREVROOT, t0
    /* mint [page_a, page_a + 4096) with descriptor 0x10f */
    lla t3, desc
    lla t4, page_a
    sd t4, 0(t3)
    li t1, 4096
    add t4, t4, t1
    sd t4, 8(t3)
    li t4, 0x10f
    sd t4, 16(t3)
    ld t4, 0(t3)
    sd t4, 24(t3)
    sd x0, 32(t3)
    sd x0, 40(t3)
    lla t5, slot
    CSMINT(t6, t5, t3)
    LDC(t0, t5, 0)
#if CASE == 0
    /* arm the SBI timer TICK from now, before the registers are filled */
    rdtime a0
    li t1, TICK
    add a0, a0, t1
    li a7, 0x54494d45
    li a6, 0
    ecall
    bnez a0, fail
    .irp n, 1,2,3,4,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30
    MOVC(x\n, t0)
    .endr
    li t6, SPIN_MAX
1:  addi t6, t6, -1
    beqz t6, no_timer
    csrr t5, sip
    andi t5, t5, 0x20
    beqz t5, 1b
    /* a few more iterations after the tick, then the use */
    li t6, 1000
2:  addi t6, t6, -1
    bnez t6, 2b
#ifdef STRIP_S5
    addi s5, s5, 0                /* the checker's control: s5 (x21) becomes an integer */
#endif
    li t6, 64
    DERIVE(t5, s2, t6)
    CHECK_NONLIN 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29
#elif CASE == 1
    .irp n, 1,2,3,4,6,7,8,9,10,11,12,13,14,15,18,19,20,21,22,23,24,25,26,27,28,29,30
    MOVC(x\n, t0)
    .endr
    li a7, 0x10
    li a6, 0
    ecall
    bnez a0, bad_a0
    LCC(t6, a0, LCC_TYPE)
    addi t6, t6, -TYPE_SCALAR
    bnez t6, bad_a0
    LCC(t6, a1, LCC_TYPE)
    addi t6, t6, -TYPE_SCALAR
    bnez t6, bad_a0
    li t6, 64
    DERIVE(t5, s2, t6)
    CHECK_NONLIN 1,2,3,4,5,6,7,8,9,12,13,14,15,18,19,20,21,22,23,24,25,26,27,28,29
#else
#error unknown CASE
#endif
    li t6, TEST_DEV
    li t5, 0x5555
    sw t5, 0(t6)
3:  j 3b
no_timer:
    li t5, 2
    EXIT_CODE t5
bad_a0:
    li t5, 3
    EXIT_CODE t5
fail:
    li t5, 1
    EXIT_CODE t5
    .balign 4
s_trap:
    csrr t5, scause
    li t4, 128
    add t5, t5, t4
    EXIT_CODE t5

    .section .data
    .balign 16
slot: .space 16
desc: .space 48
    .balign 4096
page_a: .space 4096
    .section .bss
    .balign 4096
node_table: .space 4096
dir1: .space 4096
dir2: .space 4096
records: .space 4096
