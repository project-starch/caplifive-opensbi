#include "capstone-sbi/sbi_capstone.c"

#define C_WRITE_CSR(csr_name, v) __asm__("csrw " #csr_name ", %0":: "r"(v))
#define __dominit __attribute__((dominit))

__dominit void dom_init() { }

void cap_env_init(__linear void *cap0, __linear void *cap1, __linear void *cap2) {
    int i;
#ifdef CAPSTONE_TARGET_FPGA
    unsigned k;
    unsigned boot_n;
    __linear unsigned *cap_above;
    /* R-55 (regions): the genesis regions' NOMINAL bounds (sbi_capstone_init.S: CAPENTER's a0, the boot stack's two
       SPLITs) and every other slot's "not known", set before any carve reads them; never rely on zero-init. The
       full-scheme encoder rounds region 1's decoded base below _cap_text_end (it is the lower piece of a SPLIT of a1
       = [_cap_text_end, 2^40)); the cursorless one does not. That is what turns exact_regions on: on a cursorless
       bitstream (0568f93a9) the table behaves exactly as before. */
    exact_regions = 0;
    if(cap_base(cap1) != _cap_text_end) {
        exact_regions = 1;
    }
    for(k = 0; k < CAPSTONE_MAX_REGION_N; k += 1) {
        region_nbase[k] = 0;
        region_nend[k] = 0;
    }
    region_nbase[0] = 0;
    region_nend[0] = _cap_text_start;
    region_nbase[1] = _cap_text_end;
    region_nend[1] = dom_stack;
    region_nbase[2] = dom_stack_end;
    region_nend[2] = 0x10000000000;
#endif

    // regions
    region_n = 3;
    C_WRITE_CCSR(cpmp(0), cap0);
    C_WRITE_CCSR(cpmp(1), cap1);
    C_WRITE_CCSR(cpmp(2), cap2);
    cpmp_region[0] = 0;
    cpmp_region[1] = 1;
    cpmp_region[2] = 2;
    region_cpmp[0] = 0;
    region_cpmp[1] = 1;
    region_cpmp[2] = 2;
    for(i = 3; i < CPMP_COUNT; i += 1) {
        cpmp_region[i] = -1;
    }
    for(i = 3; i < CAPSTONE_MAX_REGION_N; i += 1) {
        region_cpmp[i] = -1;
    }
    /* the three genesis regions are CPMP-resident and never appended: mark them live by hand
       and every other slot dead, exactly as the -1 loops above do (never rely on zero-init) */
    region_live[0] = 1;
    region_live[1] = 1;
    region_live[2] = 1;
    for(i = 3; i < CAPSTONE_MAX_REGION_N; i += 1) {
        region_live[i] = 0;
    }

    // // trap vector
    C_WRITE_CCSR(ctvec, _cap_trap_entry);

#ifdef CAPSTONE_TARGET_FPGA
    // UART capability, for making monitor errors visible on the console (I-4).
    // Minted exactly like the timer capabilities below: a non-linear cap split out
    // of the genesis region. CAPENTER hands a0 = [0, _cap_text_start) and the
    // firmware links at 0x8xxxxxxx, so 0x10000000 (ARIANE_UART_ADDR) falls inside
    // region 0 -- the same region 0x200bff8 (mtime) comes out of. Done FIRST so that
    // a failure in one of the splits below can itself be reported.
    /* The fence.i that preceded this split was dropped 2026-09-09 with the three CCSRRW-adjacent ones in
     * sbi_capstone.S (R-26: the RTL flushes after every CCSRRW since 9d8797560); booted clean as sw47. */
    unsigned *cap_uart = split_out_cap(CAPSTONE_UART_BASE, CAPSTONE_UART_LEN, 0);
    /* R-55: set, not assumed -- see the mtime carve below. */
    cap_uart = __setcursor(cap_uart, CAPSTONE_UART_BASE);
    capstone_uart = cap_uart;
    capstone_uart_ready = 1;
#endif
#ifdef CAPSTONE_BOOT_TRACE
    capstone_report(0x42543030, 0);   /* "BT00": cap_env_init reached, UART capability minted */
#endif

    /* The fence.i that used to follow each split_out_cap/write below was removed 2026-09-08
       (Phase B item 8): neither a fetch-side capability cache nor icache/data non-coherence
       explains them (rtl-oracle, R-26 notes), and a firmware-only board boot without them ran the
       control and six BEEBS rungs at their oracles. The one in the UART mint above stays: never
       tested for removal. */
    // timer capabilities
    unsigned *cap = split_out_cap(SBI_MTIME_ADDR, 8, 0);
#ifdef CAPSTONE_TARGET_FPGA
    /* R-55 (2026-10-15): SPLIT gives the piece split_out_cap returns its parent's DECODED start as cursor. Under a
       full-scheme bounds encoder (the FP bitstreams' option (b), the Sublet format) that start rounds below the
       carve point: mtime came out as [0x2000000, 0x200c000) with its cursor on the CLINT's msip, so
       handle_exception's `*mtime` emulated every S-mode rdtime as 0 and Linux hung in start_kernel. A carve
       that is dereferenced at its cursor sets the cursor itself. FPGA only: QEMU's bases stay exact, its cursor
       is already here, and its monitor stays byte-identical. */
    cap = __setcursor(cap, SBI_MTIME_ADDR);
#endif
    mtime = cap;
    cap = split_out_cap(SBI_MTIMECMP_ADDR, 8, 0);
#ifdef CAPSTONE_TARGET_FPGA
    cap = __setcursor(cap, SBI_MTIMECMP_ADDR);   /* R-55: exact under today's layout; set, not assumed */
#endif
    mtimecmp = cap;

    // int handler domain
    __linear unsigned *cap_int_stack  = split_out_cap(int_handler_stack,
        int_handler_stack_end - int_handler_stack, 1);
    cap_int_stack = __setcursor(cap_int_stack, int_handler_stack_end);
    __linear unsigned *cap_int_seal = split_out_cap(int_handler_seal_region,
        int_handler_seal_region_end - int_handler_seal_region, 1);
    unsigned *cap_int_code = split_out_cap(_cap_int_handler_text_start,
        _cap_int_handler_text_end - _cap_int_handler_text_start, 0);
    cap_int_code = __setcursor(cap_int_code, __int_handler_entry_entry);
#ifdef CAPSTONE_TARGET_FPGA
    /* R-55 (regions): end Linux's region at the DRAM window. The part above it is kept as its own slot, so nothing
       S-mode could reach before is lost, and every remainder carved from Linux's region later is at most 1 GiB, whose
       granule is at most 1 MiB, CMA's alignment cap. Then cut every slot the boot left into exact pieces. Both only
       when the encoder rounds. */
    if(exact_regions != 0) {
        cap_above = split_out_cap(CAPSTONE_DRAM_TOP, 0x10000000000 - CAPSTONE_DRAM_TOP, 1);
        region_append(cap_above, CAPSTONE_DRAM_TOP, 0x10000000000);
        exact_regions = 2;   /* from here on every carve's remainders are cut exact */
        boot_n = region_n;
        for(k = 0; k < boot_n; k += 1) {
            region_make_exact(k);
        }
    }
#endif

    __linear unsigned **cap_int_seal_cap_writer = cap_int_seal;
    *cap_int_seal_cap_writer = cap_int_code; cap_int_seal_cap_writer += 16;
    *cap_int_seal_cap_writer = 0; cap_int_seal_cap_writer += 16;
    *cap_int_seal_cap_writer = cap_int_stack; cap_int_seal_cap_writer += 16;
    cap_int_seal = cap_int_seal_cap_writer;

    unsigned csr_val;
    C_READ_CSR(mstatus, csr_val);
    *cap_int_seal = csr_val | (3 << 38); cap_int_seal += 8;
#define SAVE_CSR(csr_name) do { C_READ_CSR(csr_name, csr_val); *cap_int_seal = csr_val; cap_int_seal += 8; } while(0)
    SAVE_CSR(mideleg);
    SAVE_CSR(medeleg);
    SAVE_CSR(mip);
    SAVE_CSR(mie);
#undef SAVE_CSR

    __dom void *dom_ih = __seal(cap_int_seal);
    C_WRITE_CCSR(cih, dom_ih);

    // interrupt delegation
    C_WRITE_CSR(cid, 0);

    // S-mode entry point
    unsigned mepc_val;
    C_READ_CSR(mepc, mepc_val);
    C_WRITE_CCSR(cepc, mepc_val);

#ifdef CAPSTONE_BOOT_TRACE
    capstone_report(0x42543031, 1);   /* "BT01": before the save-area carve */
#endif
#ifdef CAPSTONE_SUPERVISOR_CSR_EVENTS
    // supervised CALL on silicon: the monitor's private save area (capstone-sbi supervised_invoke)
    sup_save_area = split_out_cap(sup_save_region, sup_save_region_end - sup_save_region, 1);
#endif
#ifdef CAPSTONE_BOOT_TRACE
    capstone_report(0x42543032, 2);   /* "BT02": after the save-area carve */
#endif

    // re-enable interrupts
    // __asm__("csrs mstatus, %0" :: "r"(MSTATUS_MIE));
#ifdef CAPSTONE_BOOT_TRACE
    capstone_report(0x42543033, 3);   /* "BT03": cap_env_init done */
#endif
#ifdef CAPSTONE_TARGET_QEMU
    __asm__("csrs mstatus, %0" :: "r"(MSTATUS_MIE));
#endif
}
