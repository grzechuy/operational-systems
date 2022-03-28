#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){

    printf("PID: %d\t", getpid());
    printf("PPID: %d\t", getppid());
    printf("UID: %d\t", getuid());
    printf("GID: %d\n", getgid());

    return 0;
}