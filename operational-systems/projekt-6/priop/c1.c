#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include "semafory.h"

#define LICZBA_SEM 5
#define w1 0
#define w2 1
#define w3 2
#define sp 3
#define sc 4

void read_unit();

int main(int argc, char *argv[]) {
    char *end;
    int liczba_miejsc = strtol(argv[1], &end, 10);

    if (errno == ERANGE) {
        perror("Blad - nieprawidlowy zakres\n");
        exit(EXIT_FAILURE);
    }
    else if(end == argv[1]) {
        printf("Blad - bledny format\n");
        exit(EXIT_FAILURE);
    }

    // przylaczanie semaforow
    key_t key_semafor = ftok(".", 'S');
    int semID = utworz_nowy_semafor(key_semafor, LICZBA_SEM);

    // tworze licznik -> pamiec dzielona
    key_t key_readcount = ftok(".", 'R');
    int id_readcount = shmget(key_readcount, sizeof(int), IPC_CREAT | 0600);
    if (id_readcount == -1) {
        printf("Blad - problem z utworzeniem pamieci dzielonej\n");
    }
    else {
        //printf("Pamiec dzielona zostala utworzona\n");
    }

    // dolaczam segment pamieci wspoldzielonej do procesu
    void *readcount = shmat(id_readcount, 0, 0);
    if (*(int*)readcount == -1) {
        printf("Blad - problem z przydzieleniem adresu\n");
    }
    else {
        //printf("Przestrzen adresowa zostala przyznana\n");
    }

    // kod glowny programu

    int wolne;
    
    while(1) {
        semafor_p(semID, w3);
        semafor_p(semID, sc);
        semafor_p(semID, w1);
        (*(int*)(readcount))++;
        if ((*(int*)(readcount)) == 1) {
            semafor_p(semID, sp);
        }
        if ((*(int*)(readcount)) > liczba_miejsc) {
            (*(int*)(readcount))--;
            semafor_v(semID, w1);
            semafor_v(semID, sc);
            semafor_v(semID, w3);
            continue;
        }
        semafor_v(semID, w1);
        semafor_v(semID, sc);
        semafor_v(semID, w3);

        read_unit();

        semafor_p(semID, w1);
        (*(int*)(readcount))--;
        wolne = liczba_miejsc - (*(int*)(readcount));
        printf("[CZYTELNIK] Wychodze. Miejsc w czytelni: %d.\n", wolne);
        if ((*(int*)(readcount)) == 0) {
            semafor_v(semID, sp);
        }
        semafor_v(semID, w1);
    }
}

void read_unit() {
    printf("[CZYTELNIK] Czytam: ");
    FILE *fp = fopen("czytelnia.txt", "r");
    if (!fp) {
        printf("Blad podczas otwierania pliku z danymi\n");
        exit(EXIT_FAILURE);
    }

    char text[100];
    fscanf(fp, "%s", text);
    printf("%s\n, text");

    fclose(fp);
}