/*
 ============================================================================
 Name        : test_mem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : memcpytest
 ============================================================================
 */
 
 
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
 
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define __USE_GNU
#include <sched.h>
#include <pthread.h>
 
#define COUNTNUMBER 1000
#define SENDCORE 1
 
static void timespec_sub(struct timespec *t1, const struct timespec *t2)
{
  assert(t1->tv_nsec >= 0);
  assert(t1->tv_nsec < 1000000000);
  assert(t2->tv_nsec >= 0);
  assert(t2->tv_nsec < 1000000000);
  t1->tv_sec -= t2->tv_sec;
  t1->tv_nsec -= t2->tv_nsec;
  if (t1->tv_nsec >= 1000000000)
  {
    t1->tv_sec++;
    t1->tv_nsec -= 1000000000;
  }
  else if (t1->tv_nsec < 0)
  {
    t1->tv_sec--;
    t1->tv_nsec += 1000000000;
  }
}
 
int main(int argc, char *argv[]) {
 
	int rc;
	int i;
	int j;
	char *src = NULL;
	char *dst = NULL;
	struct timespec ts_start, ts_end;
	int size=1920*1080*1.5*10;
	unsigned long long timec=0;
 	int corenum=-1;
	if (argc>2)
	{
		corenum=atoi(argv[1]);
		printf("band cpu core %d\n",corenum);
		cpu_set_t cpu_set;
		CPU_ZERO(&cpu_set);
		CPU_SET(corenum,&cpu_set);
		if(-1==sched_setaffinity(0,sizeof(cpu_set_t),&cpu_set))
		{
			perror("send process band core error.");
			return -1;
		}
	
 	
	
	posix_memalign((void **)&src, 4096/*alignment*/, 0x60000000);
 
	posix_memalign((void **)&dst, 4096/*alignment*/, 0x60000000);
	int pagesize = 0;
	int pageindex = 0;
	char *lsrc,*ldst;
	int loop_count=atoi(argv[2]);
	for(j=0;j<loop_count;j++)
	{
		pagesize = 0x60000000/size;
		pageindex = 0;
		lsrc = src;
		ldst = dst;
 
		rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
 
		for(i=0;i<COUNTNUMBER;i++)
		{
			memcpy(ldst,lsrc,size);
			if(i>= pagesize)
			{
				pageindex = 0;
			}
			ldst = dst+pageindex*size;
			lsrc = src+pageindex*size;
		}
 
		rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);
 
 
		timespec_sub(&ts_end, &ts_start);
		  /* display passed time, a bit less accurate but side-effects are accounted for */
		timec=ts_end.tv_sec*1000000+ts_end.tv_nsec/1000;
		//printf("CLOCK_MONOTONIC reports %ld.%09ld seconds (total) for copy %d 1000 times\n", ts_end.tv_sec, ts_end.tv_nsec,size);
		printf("CLOCK_MONOTONIC core%d reports %.2fus for copy %dtimes*%dKB\n", corenum, (1.0*timec)/COUNTNUMBER, COUNTNUMBER,size / 1024);
		//size=size*2;
	}
 	}
	else
	{
		printf("must input 2 params!\n");
		printf("such as\n:./memcpy 0 100\n");
		printf("program will band cpu core 0 and memcpy data 100 times.");
	}
	return EXIT_SUCCESS;
}

