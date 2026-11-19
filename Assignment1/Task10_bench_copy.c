#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/resource.h>

static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static double tv(struct timeval t){return t.tv_sec+t.tv_usec/1e6;}

static long copy_rw(const char*s,const char*d,size_t bs){
    char*buf=malloc(bs); int in=open(s,O_RDONLY),out=open(d,O_WRONLY|O_CREAT|O_TRUNC,0644);
    long calls=0; ssize_t n;
    while((n=read(in,buf,bs))>0){calls++; ssize_t o=0; while(o<n){ssize_t w=write(out,buf+o,n-o); o+=w;} }
    calls++; close(in);close(out);free(buf);return calls;}

static void copy_mmap(const char*s,const char*d){
    int in=open(s,O_RDONLY),out=open(d,O_WRONLY|O_CREAT|O_TRUNC,0644);
    struct stat st; fstat(in,&st);
    char*a=mmap(0,st.st_size,PROT_READ,MAP_PRIVATE,in,0);
    off_t o=0; while(o<st.st_size){ssize_t w=write(out,a+o,st.st_size-o); if(w<=0)break; o+=w;}
    munmap(a,st.st_size);close(in);close(out);}

static long copy_stdio(const char*s,const char*d,size_t bs){
    FILE*in=fopen(s,"r"),*out=fopen(d,"w"); char*buf=malloc(bs); size_t n; long calls=0;
    while((n=fread(buf,1,bs,in))>0){fwrite(buf,1,n,out);calls++;}
    fclose(in);fclose(out);free(buf);return calls;}

static int cmpd(const void*x,const void*y){double a=*(const double*)x,b=*(const double*)y;return (a>b)-(a<b);}
#define RUNS 5
/* run fn() RUNS times; print the run with the median real time */
#define MEASURE(label, calls_expr, stmt) do{ double R[RUNS],U[RUNS],S[RUNS]; long C=0; \
    for(int r=0;r<RUNS;r++){ struct rusage a,b; getrusage(RUSAGE_SELF,&a); double t0=now(); stmt; double t1=now(); getrusage(RUSAGE_SELF,&b); \
        R[r]=t1-t0; U[r]=tv(b.ru_utime)-tv(a.ru_utime); S[r]=tv(b.ru_stime)-tv(a.ru_stime); C=(calls_expr);} \
    qsort(R,RUNS,sizeof(double),cmpd); qsort(U,RUNS,sizeof(double),cmpd); qsort(S,RUNS,sizeof(double),cmpd); \
    printf("%-26s %9ld %9.4f %9.4f %9.4f\n",label,C,R[RUNS/2],U[RUNS/2],S[RUNS/2]); }while(0)

int main(int argc,char**argv){
    size_t MB = argc>1?(size_t)atoi(argv[1]):16;
    const char*src="bench_src.dat",*dst="bench_dst.dat";
    FILE*f=fopen(src,"w"); static char blk[65536]; for(int i=0;i<65536;i++)blk[i]='A'+i%26;
    for(size_t i=0;i<MB*16;i++){ fwrite(blk,1,65536,f); }
    fclose(f);
    printf("Source file: %zu MB   (median of %d runs)\n",MB,RUNS);
    printf("%-26s %9s %9s %9s %9s\n","method","reads","real(s)","user(s)","sys(s)");
    size_t sizes[]={64,512,4096,65536,1048576}; char name[64]; long c=0;
    for(int i=0;i<5;i++){ snprintf(name,sizeof name,"read/write buf=%zu",sizes[i]);
        MEASURE(name, c, c=copy_rw(src,dst,sizes[i])); }
    MEASURE("fread/fwrite buf=4096", c, c=copy_stdio(src,dst,4096));
    MEASURE("mmap + write(whole file)", 0, copy_mmap(src,dst));
    FILE*g=fopen("small.dat","w"); for(int k=0;k<1048576;k++){ fputc('A',g); } fclose(g);
    MEASURE("read/write buf=1 (1 MB)", c, c=copy_rw("small.dat",dst,1));
    unlink("small.dat");unlink(src);unlink(dst);return 0;}
