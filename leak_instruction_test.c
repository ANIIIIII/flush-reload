#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <assert.h>
#include <x86intrin.h>


void flush(char *adrs) {
    asm __volatile__(
        "    clflush 0(%0)      \n"
        :
        : "c" (adrs)
        :
    );
}

unsigned long probe(char *adrs) {
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


int main(void){
	// mmap binary
	int fd = open("./test", O_RDONLY);
    	struct stat file_stats;
    	fstat(fd, &file_stats);
    	int size = file_stats.st_size;
    	void *address = mmap(NULL, size, PROT_READ | PROT_EXEC | PROT_WRITE , MAP_PRIVATE, fd, 0); // MAP_PRIVATE
    	close(fd);

	char *probe_address = address + (0x1206 & ~0x3f);
	for(int i = 0; i < 100000000; i++){
		int probe_timing = probe(probe_address);
		if(probe_timing < 100){
			printf("get\n");
			break;
		}
	}
	
	return 0;
}

