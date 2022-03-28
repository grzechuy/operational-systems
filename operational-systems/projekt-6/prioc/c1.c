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

#define czytanie 0
#define pisanie 1
#define LICZBA_SEM 2

void read_unit();

int main(int argc, char *argv[]) {
    char *end;
    int liczba_miejsc = strtol(argv[1], &end, 10);
    if (errno == ERANGE) {
        perror("Blad - nieprawidlowy zakres\n");
        exit(EXIT_FAILURE);
    }
    else if (end == argv[1]) {
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
        printf("Blad - problem z utworzeniem pamieci dzielonej.\n");
    }
    else {
        //printf("[CZYTELNIK] Pamiec dzielona zostala utworzona.\n");
    }

    // dolaczam segment pamieci wspoldzielonej do procesu
    void *readcount = shmat(id_readcount, 0, 0);
    if (*(int*)readcount == -1) {
        printf("Blad - problem z przydzieleniem adresu.\n");
    }
    else {
        //printf("[CZYTELNIK] Przestrzen adresowa zostala przyznana.\n");
    }

    // kod glowny programu

    int wolne;
    sleep(3);

    while(1) {
        semafor_p(semID, czytanie);
        (*(int*)(readcount))++;
        if ((*(int*)(readcount)) == 1){
            semafor_p(semID, pisanie);
        }
        usleep(300);
        if ((*(int*)(readcount)) > liczba_miejsc) {
            (*(int*)(readcount))--;
            semafor_v(semID, czytanie);
            continue;
        }
        printf("[CZYTELNIK] Wchodze jako: %d czytelnik.\n", (*(int*)(readcount)));
        semafor_v(semID, czytanie);

        read_unit();

        sleep(1);
        semafor_p(semID, czytanie);
        (*(int*)(readcount))--;
        wolne = liczba_miejsc - (*(int*)(readcount));
        printf("[CZYTELNIK] Wychodze. Miejsc w czytelni: %d.\n", wolne);
        if ((*(int*)(readcount)) == 0) {
            semafor_v(semID, pisanie); // zwolnilo sie miejsce dla pisarza
        }
        semafor_v(semID, czytanie);
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
    printf("%s\n", text);

    fclose(fp);
}