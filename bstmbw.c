/*
 * vim: ai ts=4 sts=4 sw=4 cinoptions=>4 expandtab
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>



#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sched.h>
#include <pthread.h>

#define ENABLE_ENON 1

#ifdef ENABLE_ENON
#include <arm_neon.h>
#endif

/* how many runs to average by default */
#define DEFAULT_NR_LOOPS 10


/* default block size for test 2, in bytes */
#define DEFAULT_BLOCK_SIZE 262144 //256k


// /* we have 3 tests at the moment */
// #define MAX_TESTS 3
// /* test types */
// #define TEST_MEMCPY 0
// #define TEST_DUMB 1
// #define TEST_MCBLOCK 2
// #define TEST_NEON 3

enum {
    TEST_MEMCPY = 0,
    TEST_MEMCPY_KERNEL,
    TEST_DUMB,
    TEST_MCBLOCK,
    TEST_NEON,
    TEST_NEON_ASM,
    MAX_TESTS
};

/* version number */
#define VERSION "1.4"

/*
 * MBW memory bandwidth benchmark
 *
 * 2006, 2012 Andras.Horvath@gmail.com
 * 2013 j.m.slocum@gmail.com
 * (Special thanks to Stephen Pasich)
 *
 * http://github.com/raas/mbw
 *
 * compile with:
 *			gcc -O -o mbw mbw.c
 *
 * run with eg.:
 *
 *			./mbw 300
 *
 * or './mbw -h' for help
 *
 * watch out for swap usage (or turn off swap)
 */

void usage()
{
    printf("mbw memory benchmark v%s, https://github.com/raas/mbw\n", VERSION);
    printf("Usage: mbw [options] array_size_in_MiB\n");
    printf("Options:\n");
    printf("	-n: number of runs per test (0 to run forever)\n");
    printf("	-a: Don't display average\n");
    printf("	-l: allocate memory use posix_memalign\n");
    printf("	-t%d: memcpy test\n", TEST_MEMCPY);
    printf("	-t%d: dumb (b[i]=a[i] style) test\n", TEST_DUMB);
    printf("	-t%d: memcpy test with fixed block size\n", TEST_MCBLOCK);
    printf("	-t%d: memcpy with neon instructions\n", TEST_NEON);
    printf("	-t%d: memcpy with neon instructions asm version\n", TEST_NEON_ASM);
    printf("	-b <size>: block size in bytes for -t2 (default: %d)\n", DEFAULT_BLOCK_SIZE);
	printf("	-c: no. of cpu core band (0 to cpu number),(default: not band cpu)\n");
    printf("	-q: quiet (print statistics only)\n");
    printf("(will then use two arrays, watch out for swapping)\n");
    printf("'Bandwidth' is amount of data copied over the time this operation took.\n");
    printf("\nThe default is to run all tests available.\n");
}

/* ------------------------------------------------------ */

/* allocate a test array and fill it with data
 * so as to force Linux to _really_ allocate it */
long *make_array_alignment(unsigned long long asize)
{
    unsigned long long t;
    unsigned int long_size=sizeof(long);
    long *a;

	if (posix_memalign((void **) &a, 4096/*alignment*/, 0x60000000) != 0)
    {
        printf("posix_memalign fail.\n");
        exit(1);
    }
	else
    {
        printf("posix_memalign success.\n");
    }
    return a;
}

/* ------------------------------------------------------ */

/* allocate a test array and fill it with data
 * so as to force Linux to _really_ allocate it */
long *make_array(unsigned long long asize)
{
    unsigned long long t;
    unsigned int long_size=sizeof(long long);
    long *a;

    a=calloc(asize, long_size);

    if(NULL==a) {
        perror("Error allocating memory");
        exit(1);
    }
    else
    {
        printf("allocating memory success.\n");
    }
    /* make sure both arrays are allocated, fill with pattern */
    for(t=0; t<asize; t++) {
        a[t]=0xaa;
    }
    return a;
}

extern void * __memcpy(void *, void *, size_t);
/* actual benchmark */
/* asize: number of type 'long' elements in test arrays
 * long_size: sizeof(long) cached
 * type: 0=use memcpy, 1=use dumb copy loop (whatever GCC thinks best)
 *
 * return value: elapsed time in seconds
 */
double worker(unsigned long long asize, long *a, long *b, int type, unsigned long long block_size)
{
    unsigned long long t;
    struct timeval starttime, endtime;
    double te;
    unsigned int long_size=sizeof(long);
    /* array size in bytes */
    unsigned long long array_bytes=asize*long_size;

    if(type==TEST_MEMCPY) { /* memcpy test */
        /* timer starts */
        gettimeofday(&starttime, NULL);
        memcpy(b, a, array_bytes);
        /* timer stops */
        gettimeofday(&endtime, NULL);
    } else if(type==TEST_MEMCPY_KERNEL) { /* memcpy test */
        /* timer starts */
        gettimeofday(&starttime, NULL);
        // memcpy(b, a, array_bytes);
        __memcpy(b, a, array_bytes);
        /* timer stops */
        gettimeofday(&endtime, NULL);
    } else if(type==TEST_MCBLOCK) { /* memcpy block test */
        char* aa = (char*)a;
        char* bb = (char*)b;
        gettimeofday(&starttime, NULL);
        for (t=array_bytes; t >= block_size; t-=block_size, aa+=block_size){
            bb=mempcpy(bb, aa, block_size);
        }
        if(t) {
            bb=mempcpy(bb, aa, t);
        }
        gettimeofday(&endtime, NULL);
    } else if(type==TEST_DUMB) { /* dumb test */
        gettimeofday(&starttime, NULL);
        for(t=0; t<asize; t++) {
            b[t]=a[t];
        }
        gettimeofday(&endtime, NULL);
    } else if (type == TEST_NEON_ASM) {/* neno test */
        
        uint8_t* aa = a;
        uint8_t* bb = b;
        gettimeofday(&starttime, NULL);

        asm volatile (
        "prfm pldl1keep, [%[aa]]\n"
        "1: \n"
        "ld1 {v0.16b-v3.16b}, [%[aa]], #64 \n"
        "st1 {v0.16b-v3.16b}, [%[bb]], #64 \n"
        "subs %[array_bytes], %[array_bytes], #64 \n"
        "bgt 1b \n"
        : [bb] "+r" (bb)
        : [aa] "r" (aa),  [array_bytes] "r" (array_bytes)
        : "memory", "v0", "v1"
        );

        gettimeofday(&endtime, NULL);
    } else if (type == TEST_NEON) {
        uint8_t* aa = a;
        uint8_t* bb = b;
        gettimeofday(&starttime, NULL);
        int step = 64;
        //transfer 64 Byte every times
        for (t = array_bytes; t >=  step; t -= step, aa +=step, bb += step)
        {
                vst4q_u8(bb, vld4q_u8(aa)); //read 
        }

        //transfor bytes left
        if(t) {
            memcpy(bb, aa, t);
        }
        gettimeofday(&endtime, NULL);
    }

    te=((double)(endtime.tv_sec*1000000-starttime.tv_sec*1000000+endtime.tv_usec-starttime.tv_usec))/1000000;

    return te;
}

/* ------------------------------------------------------ */

/* pretty print worker's output in human-readable terms */
/* te: elapsed time in seconds
 * mt: amount of transferred data in MiB
 * type: see 'worker' above
 *
 * return value: -
 */
void printout(double te, double mt, int type)
{
    switch(type) {
        case TEST_MEMCPY:
            printf("Method: MEMCPY\t");
            break;
        case TEST_MEMCPY_KERNEL:
            printf("Method: MEMCPY KERNEL\t");
            break;
        case TEST_DUMB:
            printf("Method: DUMB\t");
            break;
        case TEST_MCBLOCK:
            printf("Method: MCBLOCK\t");
            break;
        case TEST_NEON:
            printf("Method: NEON\t");
            break;
        case TEST_NEON_ASM:
            printf("Method: NEON ASM\t");
            break;
    }
    printf("Elapsed: %.5f\t", te);
    printf("MiB: %.5f\t", mt);
    printf("Copy: %.3f MiB/s\n", mt/te);
    return;
}

/* ------------------------------------------------------ */

int main(int argc, char **argv)
{
    unsigned int long_size=0;
    double te, te_sum; /* time elapsed */
    unsigned long long asize=0; /* array size (elements in array) */
    int i;
    long *a, *b; /* the two arrays to be copied from/to */
    int o; /* getopt options */
    unsigned long testno;

    /* options */

    /* how many runs to average? */
    int nr_loops=DEFAULT_NR_LOOPS;
    /* fixed memcpy block size for -t2 */
    unsigned long long block_size=DEFAULT_BLOCK_SIZE;
    /* show average, -a */
    int showavg=1;
    /* what tests to run (-t x) */
    int tests[MAX_TESTS];
    double mt=0; /* MiBytes transferred == array size in MiB */
    int quiet=0; /* suppress extra messages */
	int coreno = -1; /* band cpu core number */
    int alignment=0; /* allocate memory use posix_memalign,0:not use,1:use*/
	
    tests[0]=0;
    tests[1]=0;
    tests[2]=0;
	
	
	
    while((o=getopt(argc, argv, "haqln:t:b:c:")) != EOF) {
        switch(o) {
            case 'h':
                usage();
                exit(1);
                break;
            case 'a': /* suppress printing average */
                showavg=0;
                break;
            case 'n': /* no. loops */
                nr_loops=strtoul(optarg, (char **)NULL, 10);
                break;
            case 't': /* test to run */
                testno=strtoul(optarg, (char **)NULL, 10);
                if(0>testno) {
                    printf("Error: test number must be between 0 and %d\n", MAX_TESTS);
                    exit(1);
                }
                tests[testno]=1;
                break;
            case 'b': /* block size in bytes*/
                block_size=strtoull(optarg, (char **)NULL, 10);
                if(0>=block_size) {
                    printf("Error: what block size do you mean?\n");
                    exit(1);
                }
                break;
			case 'c': /* band to cpu core */
				coreno=strtoul(optarg, (char **)NULL, 10);
				break;
            case 'l': /* allocate memory use posix_memalign */
				alignment=1;
				break;
            case 'q': /* quiet */
                quiet=1;
                break;
            default:
                break;
        }
    }
	
	if (coreno!=-1)
    {
        printf("band cpu core %d\n",coreno);
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(coreno,&cpu_set);
        if(-1==sched_setaffinity(0,sizeof(cpu_set_t),&cpu_set))
        {
                perror("send process band core error.");
                return -1;
        }
	}
	
    /* default is to run all tests if no specific tests were requested */
    int sum = 0;
    for (int i = 0; i < MAX_TESTS; i++)
    {
        sum += tests[i];
    }
    if (sum == 0)
    {
        for (int i = 0; i < MAX_TESTS; i++)
        {
            tests[i] = 0;
        }
    }

    if( nr_loops==0 && (sum != 1) ) {
        printf("Error: nr_loops can be zero if only one test selected!\n");
        exit(1);
    }

    if(optind<argc) {
        mt=strtoul(argv[optind++], (char **)NULL, 10);
    } else {
        printf("Error: no array size given!\n");
        exit(1);
    }

    if(0>=mt) {
        printf("Error: array size wrong!\n");
        exit(1);
    }

    /* ------------------------------------------------------ */

    long_size=sizeof(long); /* the size of long on this platform */
    asize=1024*1024/long_size*mt; /* how many longs then in one array? */

    if(asize*long_size < block_size) {
        printf("Error: array size larger than block size (%llu bytes)!\n", block_size);
        exit(1);
    }

    if(!quiet) {
        printf("Long uses %d bytes. ", long_size);
        printf("Allocating 2*%lld elements = %lld bytes of memory.\n", asize, 2*asize*long_size);
        if(tests[2]) {
            printf("Using %lld bytes as blocks for memcpy block copy test.\n", block_size);
        }
    }
    if(alignment==1) {
        a=make_array_alignment(asize);
        b=make_array_alignment(asize);
    }
    else {
        a=make_array(asize);
        b=make_array(asize);
    }
    /* ------------------------------------------------------ */
    if(!quiet) {
        printf("Getting down to business... Doing %d runs per test.\n", nr_loops);
    }

    /* run all tests requested, the proper number of times */
    for(testno=0; testno<MAX_TESTS; testno++) {
        te_sum=0;
        if(tests[testno]) {
            for (i=0; nr_loops==0 || i<nr_loops; i++) {
                te=worker(asize, a, b, testno, block_size);
                te_sum+=te;
                printf("%d\t", i);
                printout(te, mt, testno);
            }
            if(showavg) {
                printf("AVG\t");
                printout(te_sum/nr_loops, mt, testno);
            }
        }
    }

    free(a);
    free(b);
    return 0;
}