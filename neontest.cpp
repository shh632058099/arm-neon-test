/*
** Copyright 2019, BigFish Semiconductor
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**       http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <arm_neon.h>
#include <sys/time.h>
#include <time.h>

#define N 100000
#define M 1000

inline double timingExec(struct timeval start, struct timeval end)
{
    double timeuse = 1000.0 * (end.tv_sec - start.tv_sec) +
                     (end.tv_usec - start.tv_usec) / 1000.0;

    return timeuse;
}

static float calc_c(const float* a, const float* b, int size)
{
    double sum = 0.f;
    int i;

    for (i = 0; i < N; i++) {
        sum += a[i] * b[i];
    }

    return (float)sum;
}

static float calc_neon(const float* a, const float *b, int size)
{
    double sum = 0.f;
    float32x4_t sum_vec = vdupq_n_f32(0);

    for (int i = 0; i < size / 4; ++i) {
        float32x4_t tmp_vec_a = vld1q_f32 (a + 4*i);
        float32x4_t tmp_vec_b = vld1q_f32 (b + 4*i);
        float32x4_t tmp_mul = vmulq_f32(tmp_vec_a, tmp_vec_b);
        sum_vec = vaddq_f32(sum_vec, tmp_mul);
    }

    sum += vgetq_lane_f32(sum_vec, 0);
    sum += vgetq_lane_f32(sum_vec, 1);
    sum += vgetq_lane_f32(sum_vec, 2);
    sum += vgetq_lane_f32(sum_vec, 3);

    return (float)sum;
}

static void matrix_mul_c(const int *a, const int *b, int *c, int MM, int NN, int KK)
{
    for (int i = 0; i < MM; i++) {
        for (int j = 0; j < KK; j++) {
            c[i * KK + j] = 0;
            for (int l = 0; l < NN; l++) {
                c[i * KK + j] += a[i * NN + l] * b[l * KK + j];
            }
        }
    }
}

static void test_matrix(void)
{
    struct timeval start, end;

    int MM = 4, NN = 8, KK = 8;
    int *m_a = (int*)malloc(MM * NN * sizeof(int));
    int *m_b = (int*)malloc(NN * KK * sizeof(int));
    int *m_c = (int*)malloc(MM * KK * sizeof(int));

    for (int i = 0; i < MM * NN; i++) {
        m_a[i] = rand() % 10;
    }

    for (int i = 0; i < KK * NN; i++) {
        m_b[i] = rand() % 10;
    }

    matrix_mul_c(m_a, m_b, m_c, MM, NN, KK);

    for (int i = 0; i < MM; i++) {
        for (int j = 0; j < KK; j++) {
            printf("%d ", m_c[i * KK + j]);
        }
        printf("\n");
    }

    free(m_a);
    free(m_b);
    free(m_c);
}

int main(int argc, char **argv)
{
    struct timeval start, end;
    float time, time_neon;
    float *c, *c_neon;
    float *a, *b;

    a = (float *)malloc(sizeof(float) * N);
    b = (float *)malloc(sizeof(float) * N);
    c = (float *)malloc(sizeof(float) * M);
    c_neon = (float *)malloc(sizeof(float) * M);

    for(int i = 0; i < N; ++i) {
        a[i] = rand() % M;
        b[i] = rand() % M;
    }

    gettimeofday(&start, NULL);
    for(int t = 0; t < M; ++t) {
        c[t] = calc_c(a, b, N);
    }
    gettimeofday(&end, NULL);
    printf("end = %ld, %ld, start = %ld, %ld, c[0] = %f\n",
            end.tv_sec, end.tv_usec, start.tv_sec, start.tv_usec, c[0]);

    time = timingExec(start, end);

    gettimeofday(&start, NULL);
    for(int t = 0; t < M; ++t) {
        c_neon[t] = calc_neon(a, b, N);
    }
    gettimeofday(&end, NULL);
    printf("end = %ld, %ld, start = %ld, %ld, c_neon[0] = %f\n",
            end.tv_sec, end.tv_usec, start.tv_sec, start.tv_usec, c_neon[0]);

    time_neon = timingExec(start, end);

    float diff = 0.f;
    for(int i = 0; i < M; ++i) {
        diff += fabs(c[i] - c_neon[i]);
    }

    printf("diff %f\n", diff);
    printf("test 1 time : %f time_neon : %f speed_up %f\n", time, time_neon, time / time_neon);

    free(a);
    free(b);
    free(c);
    free(c_neon);

    test_matrix();

    return 0;
}
