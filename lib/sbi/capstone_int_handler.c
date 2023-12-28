#define __domasync __attribute__((domasync))
#define __domentry __attribute__((domentry))

static unsigned handle_hw_int(unsigned cause) {
    unsigned to_inject = 0;
    // clear cause
    __asm__ volatile ("csrc cis, %0" :: "r"(1 << cause));
    to_inject = 1 << cause;
    return to_inject;
}
