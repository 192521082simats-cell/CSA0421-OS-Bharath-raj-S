#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
/* Task 5: a tiny program whose system calls are easy to see under strace */
int main(void){
    const char *msg = "patient 101: heart rate 72\n";
    int fd = open("patient.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644);   /* file management  */
    write(fd, msg, strlen(msg));                                    /* file management  */
    close(fd);
    char buf[64];
    fd = open("patient.txt", O_RDONLY);
    ssize_t n = read(fd, buf, sizeof buf);                          /* read()           */
    close(fd);
    write(STDOUT_FILENO, buf, (size_t)n);                           /* to the screen    */
    printf("pid=%d\n", getpid());                                   /* information maint*/
    unlink("patient.txt");
    return 0;}
