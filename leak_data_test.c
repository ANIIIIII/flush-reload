#include <stdio.h>
#include <x86intrin.h>
#define MAXN (int)0x100

int probe_index = 0;
char data[MAXN];

void flush(char *adrs) {
    asm __volatile__(
        "    clflush 0(%0)      \n"
        :
        : "c" (adrs)
        :
    );
}

unsigned long probe_timing(char *adrs) {
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
        "    clflush 0(%1)      \n"
        : "=a" (time)
        : "c" (adrs)
        : "%esi", "%edx"
    );
    return time;
}

void spy(){
	unsigned long time;
	time = probe_timing(&data[probe_index]);
	printf("%lu\n", time);
}

void victim(){
    	int index = 0;
    	data[index] = 100;
    	printf("%c\n", data[index]); // bypass compiler optimization
}

void init(){
    	for(int i = 0; i < MAXN; i++){
		flush(&data[i]);
    	}
}

int main(void){
	printf("probe_index : ");
	scanf("%d", &probe_index); 
	init();
	victim();
	spy();
	return 0;
}

