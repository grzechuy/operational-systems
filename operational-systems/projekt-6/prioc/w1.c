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
#include <time.h>
#include "semafory.h"

#define LICZBA_SEM 2
#define pisanie 1

void write_unit(int licznik);

int main(int argc, char *argv[]) {
    // tworze nowy set semaforow
    key_t key_semafor = ftok(".", 'S');
    int semID = utworz_nowy_semafor(key_semafor, 2);

    // kod glowny programu
    int licznik = 0;
    while(1) {
        licznik++; // ktory raz proces wchodzi

        semafor_p(semID, pisanie);
        write_unit(licznik);
        sleep(3);

        semafor_v(semID, pisanie);
    }
}

void write_unit(int licznik) {
    FILE *fp = fopen("czytelnia.txt", "w");
    if (!fp) {
        printf("Blad podczas otwierania pliku z danymi\n");
        exit(EXIT_FAILURE);
    }
    printf("[PISARZ] Pisze do pliku \"czytelnia.txt\"...\n");
    fprintf(fp, "[PISARZ]<getpid:%d>->Jestem-tu-%d-raz.\n", getpid(), licznik);
    fflush(fp);
    fclose(fp);
}