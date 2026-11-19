#include <stdio.h>
/* Task 5 alternative: copy with fopen/fread/fwrite (libc buffers 4096 B) */
int main(void){
    FILE*f=fopen("big.dat","w"); for(int i=0;i<100000;i++)fputc('A',f); fclose(f);
    f=fopen("big.dat","r"); FILE*o=fopen("big2.dat","w"); char b[100]; size_t n;
    while((n=fread(b,1,sizeof b,f))>0) fwrite(b,1,n,o);
    fclose(f);fclose(o);remove("big.dat");remove("big2.dat");return 0;}
