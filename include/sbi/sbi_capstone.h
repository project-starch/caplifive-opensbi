#ifndef _SBI_CAPSTONE_H_
#define _SBI_CAPSTONE_H_

#define SBI_SPEC_VERSION (1 << 24)

/* Capstone-specific definitions */

#define CCSR_CTVEC    0
#define CCSR_CIH    1
#define CCSR_CEPC  2
#define CCSR_CMMU    3
#define CCSR_CSCRATCH   4

#define SETCAPMEM(reg) .insn r 0x5b, 0x1, 0x41, x0, reg, x0
#define OFFCAPMEM      .insn r 0x5b, 0x1, 0x41, x0, x0, x0
#define GENCAP(rd, rs1, rs2) .insn r 0x5b, 0x1, 0x40, rd, rs1, rs2
#define LDC(rd, rs1, imm) .insn i 0x5b, 0x3, rd, imm(rs1)
#define STC(rs2, rs1, imm) .insn s 0x5b, 0x4, rs2, imm(rs1)
#define CLEARCMMAP     .insn r 0x5b, 0x1, 0x42, x0, x0, x0
#define PRINT(reg)     .insn r 0x5b, 0x1, 0x43, x0, reg, x0
#define SEAL(rd, rs1)  .insn r 0x5b, 0x1, 0x7, rd, rs1, x0
#define CCSRRW(rd, ccsr, rs1) .insn i 0x5b, 0x7, rd, ccsr(rs1)
#define SCC(rd, rs1, rs2) .insn r 0x5b, 0x1, 0x5, rd, rs1, rs2
#define CALL(rd, rs1) .insn r 0x5b, 0x1, 0x20, rd, rs1, x0
#define RETURN(rd, rs1, rs2) .insn r 0x5b, 0x1, 0x21, rd, rs1, rs2
#define CINCOFFSETIMM(rd, rs1, imm) .insn i 0x5b, 0x2, rd, imm(rs1)
#define CINCOFFSET(rd, rs1, rs2) .insn r 0x5b, 0x1, 0xc, rd, rs1, rs2
#define DELIN(rd)     .insn r 0x5b, 0x1, 0x3, rd, x0, x0

#define CSR_CIS          0x800
#define CSR_CID			 0x801
#define CSR_CIC          0x802
#define CSR_OFFSETMMU	 0x803

/* Capstone SBI */

#define SBI_EXT_CAPSTONE 0x12345678

#define SBI_EXT_CAPSTONE_DOM_CREATE 0x0

/** Index of zero member in sbi_trap_regs */
#define SBI_TRAP_REGS_zero			0
/** Index of ra member in sbi_trap_regs */
#define SBI_TRAP_REGS_ra			1
/** Index of sp member in sbi_trap_regs */
#define SBI_TRAP_REGS_sp			2
/** Index of gp member in sbi_trap_regs */
#define SBI_TRAP_REGS_gp			3
/** Index of tp member in sbi_trap_regs */
#define SBI_TRAP_REGS_tp			4
/** Index of t0 member in sbi_trap_regs */
#define SBI_TRAP_REGS_t0			5
/** Index of t1 member in sbi_trap_regs */
#define SBI_TRAP_REGS_t1			6
/** Index of t2 member in sbi_trap_regs */
#define SBI_TRAP_REGS_t2			7
/** Index of s0 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s0			8
/** Index of s1 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s1			9
/** Index of a0 member in sbi_trap_regs */
#define SBI_TRAP_REGS_a0			10
/** Index of a1 member in sbi_trap_regs */
#define SBI_TRAP_REGS_a1			11
/** Index of a2 member in sbi_trap_regs */
#define SBI_TRAP_REGS_a2			12
/** Index of a3 member in sbi_trap_regs */
#define SBI_TRAP_REGS_a3			13
/** Index of a4 member in sbi_trap_regs */
#define SBI_TRAP_REGS_a4			14
/** Index of a5 member in sbi_trap_regs */
#define SBI_TRAP_REGS_a5			15
/** Index of a6 member in sbi_trap_regs */
#define SBI_TRAP_REGS_a6			16
/** Index of a7 member in sbi_trap_regs */
#define SBI_TRAP_REGS_a7			17
/** Index of s2 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s2			18
/** Index of s3 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s3			19
/** Index of s4 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s4			20
/** Index of s5 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s5			21
/** Index of s6 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s6			22
/** Index of s7 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s7			23
/** Index of s8 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s8			24
/** Index of s9 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s9			25
/** Index of s10 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s10			26
/** Index of s11 member in sbi_trap_regs */
#define SBI_TRAP_REGS_s11			27
/** Index of t3 member in sbi_trap_regs */
#define SBI_TRAP_REGS_t3			28
/** Index of t4 member in sbi_trap_regs */
#define SBI_TRAP_REGS_t4			29
/** Index of t5 member in sbi_trap_regs */
#define SBI_TRAP_REGS_t5			30
/** Index of t6 member in sbi_trap_regs */
#define SBI_TRAP_REGS_t6			31

// #define SAVE_REG(reg, creg) SDC(reg, creg, SBI_TRAP_REGS_ ## reg*8)
// #define RESTORE_REG(reg, creg) LDC(reg, creg, SBI_TRAP_REGS_ ## reg*8)

#define SAVE_REG(reg, creg) sd reg, SBI_TRAP_REGS_ ## reg*8(creg)
#define RESTORE_REG(reg, creg) ld reg, SBI_TRAP_REGS_ ## reg*8(creg)


/* SBI Extension IDs */
#define SBI_EXT_0_1_SET_TIMER			0x0
#define SBI_EXT_0_1_CONSOLE_PUTCHAR		0x1
#define SBI_EXT_0_1_CONSOLE_GETCHAR		0x2
#define SBI_EXT_0_1_CLEAR_IPI			0x3
#define SBI_EXT_0_1_SEND_IPI			0x4
#define SBI_EXT_0_1_REMOTE_FENCE_I		0x5
#define SBI_EXT_0_1_REMOTE_SFENCE_VMA		0x6
#define SBI_EXT_0_1_REMOTE_SFENCE_VMA_ASID	0x7
#define SBI_EXT_0_1_SHUTDOWN			0x8
#define SBI_EXT_BASE				0x10
#define SBI_EXT_TIME				0x54494D45
#define SBI_EXT_IPI				0x735049
#define SBI_EXT_RFENCE				0x52464E43
#define SBI_EXT_HSM				0x48534D
#define SBI_EXT_SRST				0x53525354
#define SBI_EXT_PMU				0x504D55
#define SBI_EXT_DBCN				0x4442434E
#define SBI_EXT_SUSP				0x53555350
#define SBI_EXT_CPPC				0x43505043

/* SBI function IDs for BASE extension*/
#define SBI_EXT_BASE_GET_SPEC_VERSION		0x0
#define SBI_EXT_BASE_GET_IMP_ID			0x1
#define SBI_EXT_BASE_GET_IMP_VERSION		0x2
#define SBI_EXT_BASE_PROBE_EXT			0x3
#define SBI_EXT_BASE_GET_MVENDORID		0x4
#define SBI_EXT_BASE_GET_MARCHID		0x5
#define SBI_EXT_BASE_GET_MIMPID			0x6

/* SBI function IDs for TIME extension*/
#define SBI_EXT_TIME_SET_TIMER			0x0

/* SBI function IDs for IPI extension*/
#define SBI_EXT_IPI_SEND_IPI			0x0

/* SBI function IDs for RFENCE extension*/
#define SBI_EXT_RFENCE_REMOTE_FENCE_I		0x0
#define SBI_EXT_RFENCE_REMOTE_SFENCE_VMA	0x1
#define SBI_EXT_RFENCE_REMOTE_SFENCE_VMA_ASID	0x2
#define SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA_VMID	0x3
#define SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA	0x4
#define SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA_ASID	0x5
#define SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA	0x6

/* SBI function IDs for HSM extension */
#define SBI_EXT_HSM_HART_START			0x0
#define SBI_EXT_HSM_HART_STOP			0x1
#define SBI_EXT_HSM_HART_GET_STATUS		0x2
#define SBI_EXT_HSM_HART_SUSPEND		0x3

#define SBI_HSM_STATE_STARTED			0x0
#define SBI_HSM_STATE_STOPPED			0x1
#define SBI_HSM_STATE_START_PENDING		0x2
#define SBI_HSM_STATE_STOP_PENDING		0x3
#define SBI_HSM_STATE_SUSPENDED			0x4
#define SBI_HSM_STATE_SUSPEND_PENDING		0x5
#define SBI_HSM_STATE_RESUME_PENDING		0x6

#define SBI_HSM_SUSP_BASE_MASK			0x7fffffff
#define SBI_HSM_SUSP_NON_RET_BIT		0x80000000
#define SBI_HSM_SUSP_PLAT_BASE			0x10000000

#define SBI_HSM_SUSPEND_RET_DEFAULT		0x00000000
#define SBI_HSM_SUSPEND_RET_PLATFORM		SBI_HSM_SUSP_PLAT_BASE
#define SBI_HSM_SUSPEND_RET_LAST		SBI_HSM_SUSP_BASE_MASK
#define SBI_HSM_SUSPEND_NON_RET_DEFAULT		SBI_HSM_SUSP_NON_RET_BIT
#define SBI_HSM_SUSPEND_NON_RET_PLATFORM	(SBI_HSM_SUSP_NON_RET_BIT | \
						 SBI_HSM_SUSP_PLAT_BASE)
#define SBI_HSM_SUSPEND_NON_RET_LAST		(SBI_HSM_SUSP_NON_RET_BIT | \
						 SBI_HSM_SUSP_BASE_MASK)

/* SBI function IDs for SRST extension */
#define SBI_EXT_SRST_RESET			0x0

#define SBI_SRST_RESET_TYPE_SHUTDOWN		0x0
#define SBI_SRST_RESET_TYPE_COLD_REBOOT	0x1
#define SBI_SRST_RESET_TYPE_WARM_REBOOT	0x2
#define SBI_SRST_RESET_TYPE_LAST	SBI_SRST_RESET_TYPE_WARM_REBOOT

#define SBI_SRST_RESET_REASON_NONE	0x0
#define SBI_SRST_RESET_REASON_SYSFAIL	0x1

/* SBI function IDs for PMU extension */
#define SBI_EXT_PMU_NUM_COUNTERS	0x0
#define SBI_EXT_PMU_COUNTER_GET_INFO	0x1
#define SBI_EXT_PMU_COUNTER_CFG_MATCH	0x2
#define SBI_EXT_PMU_COUNTER_START	0x3
#define SBI_EXT_PMU_COUNTER_STOP	0x4
#define SBI_EXT_PMU_COUNTER_FW_READ	0x5
#define SBI_EXT_PMU_COUNTER_FW_READ_HI	0x6

/* Assuming the following locations for the timer */
#define SBI_MTIME_ADDR    0x200bff8
#define SBI_MTIMECMP_ADDR 0x2004000

#endif
