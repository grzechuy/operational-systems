#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(){

    int i, potomek, pid;
    char drzewo[256];

    pid = getpid();

    printf("PROCES MACIERZYSTY:\t");
    printf("PID: %d\t", getpid());
    printf("PPID: %d\t", getppid());
    printf("UID: %d\t", getuid());
    printf("GID: %d\n", getgid());
    for (i = 1; i < 4; i++){
        potomek=fork();
        switch(potomek){
            case -1:
                perror("FORK ERROR\n");
                exit(1);
                break;
            case 0:
                printf("PROCES POTOMNY %d:\t", i);
                printf("PID: %d\t", getpid());
                printf("PPID: %d\t", getppid());
                printf("UID: %d\t", getuid());
                printf("GID: %d\n", getgid());
                break;
        }
    }

    
    if (getpid()==pid){
        sprintf(drzewo, "%d", pid);
        sleep(1);
        execl("/bin/pstree", "pstree", "-p", drzewo, NULL);
    }
    else {
        sleep(2);
    }

    return 0;
}