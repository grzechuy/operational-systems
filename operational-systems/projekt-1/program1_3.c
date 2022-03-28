#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>


int main(){

    int i, potomek, pid, status;
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
                sleep(i+1);
                printf("PROCES POTOMNY %d:\t", i);
                int execValue = execl("./program1_1", "program1_1", NULL);

                if (execValue == -1){
                    perror("BLAD EXECL");
                    exit(1);
                }
                break;
        }
    }

    for (i=1;i<4;i++){
        potomek = wait(&status);
        
        switch(status){
            case 0:
            printf("DZIALANIE PROCESU ZAKONCZONE POMYSLNIE\n");
            break;

            default:
            printf("DZIALANIE PROCESU ZAKONCZONE BLEDEM\n");
            break;
        }

        printf("PID POTOMKA: %d\t STATUS: %d\n", potomek, status/256);

        switch (potomek){
            case -1:
            perror("BLAD WAIT\n");
            exit(1);
        }
    }

    

    return 0;
}