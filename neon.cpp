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

void neon_cpy(uint8_t* dst, uint8_t* src, int size)
{
    /*
    int64_t t ;
    uint8_t* aa = src;
    uint8_t* bb = dst;
    uint8x16x4_t s;
    uint64_t array_bytes = size;

        int step = 48;
        for (t = array_bytes; t >=  step; t -= step, aa +=step, bb += step)
        {
            vst4q_u8(bb, vld4q_u8(aa)); //read 
        }

        if(t) {
            memcpy(bb, aa, t);
        }
    */
    asm volatile (
        // "prfm pldl1keep, [%[src]]\n"
        "1: \n"
        "ld1 {v0.16b-v3.16b}, [%[src]], #64  \n"
        "st1 {v0.16b-v3.16b}, [%[dst]], #64 \n"
        "subs %[size], %[size], #64 \n"
        "bgt 1b \n"
        : [dst] "+r" (dst)
        : [src] "r" (src),  [size] "r" (size)
        : "memory", "v0", "v1"
        );
}

const int buff_len = 1920;
int main(int argc, char* argv)
{
    printf("SIZE:%d\n", buff_len);
    unsigned  char *src = (unsigned char*)malloc(buff_len);
    unsigned char *dst = (unsigned char*)malloc(buff_len);
    for (int i = 0; i < buff_len; i++)
        src[i] = i % 26 + 'a';

    neon_cpy((uint8_t*)dst, (uint8_t*)src, buff_len);
    printf("res:%s copysize:%d\n", dst, buff_len);

    return 0;
}