#!/usr/bin/env bash
# The firmware's Capstone contract on the reference QEMU (capstone-qemu 9289114e06 or later):
# a trap into M keeps the tags of S's registers (holder.S). Builds this tree twice, with and
# without FW_CAPSTONE_KEEP_TAGS, and runs every case against both.
#   run.sh <qemu-system-riscv64> <work dir>
# Expected: keep-tags 0/0 for CASE 0 and 1; stock 152 for both (CDERIVE of a reference that came
# back from M untagged raises 24, redirected to S); keep-tags CASE 0 -DSTRIP_S5 53 (the checker's
# control). Takes the shared QEMU lock when CAPSTONE_QEMU_LOCK is set.
set -euo pipefail
qemu=${1:?qemu} work=${2:?work dir}
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
src=$(cd -- "$here/../.." && pwd)
cpu="rv64,sstc=false,x-capstone-test-csrs=true,x-capstone-supervisor-refs=true,x-capstone-test-s-ldc=false,x-capstone-trap-route=true"
mkdir -p "$work"
for v in y n; do
    make -s -C "$src" -j12 PLATFORM=generic CROSS_COMPILE=${CROSS_COMPILE:-riscv64-unknown-elf-} \
        FW_CAPSTONE_KEEP_TAGS=$v ${FW_EXTRA:-} O="$work/fw-$v" > "$work/fw-$v.log" 2>&1
done
build() { riscv64-unknown-elf-gcc -march=rv64gc -mabi=lp64d -nostdlib -nostartfiles "$@"; }
build -DCASE=0 -Wl,-T,"$here/link.ld" "$here/holder.S" -o "$work/case0.elf"
build -DCASE=1 -Wl,-T,"$here/link.ld" "$here/holder.S" -o "$work/case1.elf"
build -DCASE=0 -DSTRIP_S5 -Wl,-T,"$here/link.ld" "$here/holder.S" -o "$work/case0-strip.elf"
boot() { # fw elf log -> rc
    local rc=0
    if [[ -n ${CAPSTONE_QEMU_LOCK:-} ]]; then
        flock -w 3600 "$CAPSTONE_QEMU_LOCK" timeout 60s "$qemu" -M virt -cpu "$cpu" -smp 1 -m 256M \
            -nographic -bios "$1" -kernel "$2" > "$3" 2>&1 || rc=$?
    else
        timeout 60s "$qemu" -M virt -cpu "$cpu" -smp 1 -m 256M -nographic -bios "$1" -kernel "$2" \
            > "$3" 2>&1 || rc=$?
    fi
    echo "$rc"
}
failed=0
check() { # name fw elf expected
    local rc; rc=$(boot "$work/fw-$2/platform/generic/firmware/fw_jump.elf" "$work/$3.elf" "$work/$1.log")
    if [[ $rc == "$4" ]]; then echo "PASS $1 (rc $rc)"; else echo "FAIL $1 (rc $rc, expected $4)"; failed=1; fi
}
check keep-timer y case0 0
check keep-call y case1 0
check stock-timer n case0 152
check stock-call n case1 152
check keep-strip y case0-strip 53
grep -q 'Capstone: register tags kept across M traps' "$work/keep-timer.log" || { echo "FAIL banner"; failed=1; }
exit $failed
