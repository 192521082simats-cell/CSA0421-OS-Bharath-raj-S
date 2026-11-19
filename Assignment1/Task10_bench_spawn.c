#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static void*tfn(void*a){(void)a;return NULL;}
int main(void){
    const int N=1000; double t0,t1;
    t0=now(); for(int i=0;i<N;i++){pid_t p=fork(); if(p==0)_exit(0); waitpid(p,NULL,0);} t1=now();
    printf("%-34s %9.2f ms  %7.1f us each\n","1000 x fork()+wait",(t1-t0)*1e3,(t1-t0)*1e6/N);
    t0=now(); for(int i=0;i<N;i++){pthread_t t; pthread_create(&t,NULL,tfn,NULL); pthread_join(t,NULL);} t1=now();
    printf("%-34s %9.2f ms  %7.1f us each\n","1000 x pthread_create()+join",(t1-t0)*1e3,(t1-t0)*1e6/N);
    t0=now(); for(int i=0;i<N;i++){pid_t p=fork(); if(p==0){execlp("true","true",(char*)NULL);_exit(127);} waitpid(p,NULL,0);} t1=now();
    printf("%-34s %9.2f ms  %7.1f us each\n","1000 x fork()+exec(true)",(t1-t0)*1e3,(t1-t0)*1e6/N);
    t0=now(); for(int i=0;i<N;i++){pid_t p; char*av[]={"true",NULL}; posix_spawnp(&p,"true",NULL,NULL,av,environ); waitpid(p,NULL,0);} t1=now();
    printf("%-34s %9.2f ms  %7.1f us each\n","1000 x posix_spawn(true)",(t1-t0)*1e3,(t1-t0)*1e6/N);
    pthread_t *ts=malloc(N*sizeof *ts); int ok=0; t0=now();
    for(int i=0;i<N;i++) if(pthread_create(&ts[i],NULL,tfn,NULL)==0)ok++;
    for(int i=0;i<ok;i++) pthread_join(ts[i],NULL); t1=now();
    printf("%-34s %9.2f ms  (created %d)\n","1000 threads, then join all",(t1-t0)*1e3,ok);
    pid_t *ps=malloc(N*sizeof *ps); t0=now();
    for(int i=0;i<N;i++){ps[i]=fork(); if(ps[i]==0)_exit(0);} for(int i=0;i<N;i++)waitpid(ps[i],NULL,0); t1=now();
    printf("%-34s %9.2f ms\n","1000 processes, then wait all",(t1-t0)*1e3);
    return 0;}
