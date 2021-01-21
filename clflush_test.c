#include <stdio.h>
#include <x86intrin.h>

void flush(int *adrs) {
    asm __volatile__(
        "    clflush 0(%0)      \n"
        :
        : "c" (adrs)
        :
    );
}

unsigned long probe_timing(int *adrs) {
    volatile unsigned long time;
    asm __volatile__(
        "    mfence             \n"
        "    lfence             \n"
        "    rdtsc              \n"
        "    lfence             \n"
        "    movl %%eax, %%esi  \n"
        "    movl (%1), %%eax   \n"
        "    lfence             \n"
        "    rdtsc              \n"
        "    subl %%esi, %%eax  \n"
        : "=a" (time)
        : "c" (adrs)
        : "%esi", "%edx"
    );
    return time;
}

int main(void){
    unsigned long time = 0;
    int target;
#ifdef FLUSH
    flush(&target);
#endif
    time = probe_timing(&target);
    printf("%lu\n", time);
    return 0;
}
