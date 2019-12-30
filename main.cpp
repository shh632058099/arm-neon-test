#include <arm_neon.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <future>
#include <pthread.h>

static const uint64_t buffer_size = 1920U*1080U*3U/2U;

void memcpy1(uint8_t* dst, const uint8_t* src, uint64_t size)
{
    auto n = size/8/3;
    while (n-- >0)
    {
        vst3_u8(dst, vld3_u8(src));
        src += 24;
        dst += 24;
    } 
}

void memcpy2(uint8_t* dst, const uint8_t* src, uint64_t size)
{
    auto n = size/8;
    while (n-- >0)
    {
        vst1_u8(dst, vld1_u8(src));
        src += 8;
        dst += 8;
    } 
}

void *memcpy3(uint8_t* dst, const uint8_t* src, uint64_t count)
{
        int i;
        unsigned long *s = (unsigned long *)src;
        unsigned long *d = (unsigned long *)dst;
        for (i = 0; i < count / 64; i++) {
                vst1q_u64(&d[0], vld1q_u64(&s[0])); 
                vst1q_u64(&d[2], vld1q_u64(&s[2])); 
                vst1q_u64(&d[4], vld1q_u64(&s[4])); 
                vst1q_u64(&d[6], vld1q_u64(&s[6]));
                d += 8; s += 8;
        }
        return dst;
}

std::atomic_bool g_flag;
void time_waste()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(2, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
        perror("pthread_setaffinity_np");
    }

    uint8_t* src = new uint8_t[buffer_size];
    uint8_t* dst = new uint8_t[buffer_size];
    while(g_flag)
    {
        memcpy(dst,src,buffer_size);
    }
    delete[] src;
    delete[] dst;
}

int main ()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(2, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
        perror("pthread_setaffinity_np");
    }

    g_flag = true;
    uint8_t* src = new uint8_t[buffer_size];
    uint8_t* dst = new uint8_t[buffer_size];

    sprintf((char*)src,"Hello World!");

    // auto fut = std::async(time_waste);

    auto t1 = std::chrono::steady_clock::now();
    for (int i = 100; i > 0 ; --i)
        memcpy(dst,src,buffer_size);
    auto t2 = std::chrono::steady_clock::now();
    printf("memcpy: %dms\n",std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count());

    t1 = std::chrono::steady_clock::now();
    for (int i = 100; i > 0 ; --i)
        memcpy1(dst,src,buffer_size);
    t2 = std::chrono::steady_clock::now();
    printf("memcpy1:%dms\n",std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count());

    t1 = std::chrono::steady_clock::now();
    for (int i = 100; i > 0 ; --i)
        memcpy2(dst,src,buffer_size);
    t2 = std::chrono::steady_clock::now();
    printf("memcpy2:%dms\n",std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count());

    t1 = std::chrono::steady_clock::now();
    for (int i = 100; i > 0 ; --i)
        memcpy3(dst,src,buffer_size);
    t2 = std::chrono::steady_clock::now();
    printf("memcpy3:%dms\n",std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count());

    delete[] src;
    delete[] dst;

    g_flag = false;
    // fut.get();

    return 0;
}