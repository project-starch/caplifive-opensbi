#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (C) 2019 FORTH-ICS/CARV
#		Panagiotis Peristerakis <perister@ics.forth.gr>
#

# Compiler flags
# This platform IS the FPGA target of the Capstone monitor: the monitor sources (lib/sbi/capstone-sbi,
# included by sbi_capstone_init.S and compiled by capstone-c for the generated .c.S) select their
# per-target constants on this define (capstone_target.h). The buildroot Makefile passes the same
# define to the capstone-c regeneration through CAPSTONE_EXTRA_DEFS.
# CAPSTONE_PLATFORM_DEFS (make variable, empty by default): the monitor build's defines for OpenSBI's own C and
# assembly, e.g. -DCAPSTONE_SUPERVISED_CALL, which sizes dom_stack in sbi_capstone_init.S. capstone-c gets the same
# defines through CAPSTONE_EXTRA_DEFS; without this the .S never sees them (2026-10-03, the dom_stack gate caught it).
platform-cppflags-y = -DCAPSTONE_TARGET_FPGA $(CAPSTONE_PLATFORM_DEFS)
platform-cflags-y = -DCAPSTONE_TARGET_FPGA $(CAPSTONE_PLATFORM_DEFS)
platform-asflags-y = -DCAPSTONE_TARGET_FPGA $(CAPSTONE_PLATFORM_DEFS)
platform-ldflags-y =

# Object to build
platform-objs-y += platform.o

PLATFORM_RISCV_XLEN = 64

# Blobs to build
FW_TEXT_START=0x80000000
FW_DYNAMIC=y
FW_JUMP=y
LINUX_PAYLOAD ?=
ifeq ($(LINUX_PAYLOAD),1)
  FW_FDT_PATH=../../images/caplifive.dtb
  FW_PAYLOAD_PATH=../../images/Image
  FW_PAYLOAD_FDT_PATH=../../images/caplifive.dtb
endif
ifeq ($(PLATFORM_RISCV_XLEN), 32)
 # This needs to be 4MB aligned for 32-bit support
 FW_JUMP_ADDR=0x80400000
 else
 # This needs to be 2MB aligned for 64-bit support
 FW_JUMP_ADDR=0x80200000
 endif
FW_JUMP_FDT_ADDR=0x82200000

# Firmware with payload configuration.
FW_PAYLOAD=y

ifeq ($(PLATFORM_RISCV_XLEN), 32)
# This needs to be 4MB aligned for 32-bit support
  FW_PAYLOAD_OFFSET=0x400000
else
# This needs to be 2MB aligned for 64-bit support
  FW_PAYLOAD_OFFSET=0x200000
endif
FW_PAYLOAD_FDT_ADDR=0x82200000
# FW_PAYLOAD_ALIGN=0x1000

FW_FDT_PAYLOAD_OFFSET=0x2200000
