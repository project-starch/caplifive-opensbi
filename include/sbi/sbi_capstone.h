#ifndef _SBI_CAPSTONE_H_
#define _SBI_CAPSTONE_H_

#define CCSR_CEH    0x0
#define CCSR_CIH    0x1
#define CCSR_CINIT  0x2
#define CCSR_EPC    0x3
#define CCSR_CMMU   0x4

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

#endif
