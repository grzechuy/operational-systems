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
#include <signal.h>
#include "semafory.h"

#define LICZBA_SEM 5
#define BUF_SIZE 256

int liczba_pisarzy, liczba_czytelnikow, liczba_miejsc;
void spr_argumentow(int argc, char *argv[]);
int spr_limit();
void wait_all();
void handler(int sig);
void clear();

//licznik ile aktualnie czytelnikow / pisarzy w czytelni -> jako pamięć dzielona
key_t key_readcount, key_writecount;
int id_readcount, id_writecount;
void *readcount;
void *writecount;

void upd_read();
void upa_read();

void upd_write();
void upa_write();

key_t key_semafor;

int main(int argc, char *argv[]) {

    spr_argumentow(argc, argv);

    // generuje klucz dla semaforow
    key_semafor = ftok(".", 'S');
    int semID = utworz_nowy_semafor(key_semafor, LICZBA_SEM);
    for (int k = 0; k < 5; k++) {
        ustaw_semafor(semID, k, 1);
    }

    // tworze licznik -> pamięć dzielona
    upd_read();
    upa_read();
    *(int *)readcount = 0;

    upd_write();
    upa_write();
    *(int *)writecount = 0;

    signal(SIGINT, handler);

    for (int i = 0; i < liczba_pisarzy; i++) {
        switch (fork()) {
        case -1:
            perror("Fork error");
            break;
        case 0:
            if (execl("./w1", "w1", NULL, NULL) == -1) {
                perror("Blad execl.\n");
                exit(0);
            }
            exit(0);
        }
    }

    for (int j = 0; j < liczba_czytelnikow; j++) {
        switch (fork()) {
        case -1:
            perror("Fork error");
            break;
        case 0: //potomek
            if (execl("./c1", "c1", argv[3], NULL) == -1) {
                perror("Blad execl.\n");
                exit(0);
            }
            exit(0);
        }
    }

    wait_all();

    // sprzatam semafory + pamiec dzielona
    clear();
    sleep(2);
}

void upd_read() {
    key_readcount = ftok(".", 'R');

    id_readcount = shmget(key_readcount, sizeof(int), IPC_CREAT | 0600);
    if (id_readcount == -1) {
        perror("Blad - problem z utworzeniem pamieci dzielonej.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("Pamiec dzielona zostala utworzona.\n");
    }    
}

void upd_write() {
    key_writecount = ftok(".", 'W');

    id_writecount = shmget(key_writecount, sizeof(int), IPC_CREAT | 0600);
    if (id_writecount == -1) {
        perror("Blad - problem z utworzeniem pamieci dzielonej.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("Pamiec dzielona zostala utworzona.\n");
    }
}

void upa_read() {
    readcount = shmat(id_readcount, NULL, 0);
    if (*(int *)readcount == -1) {
        perror("Blad - nie można przydzielić adresu.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("Przestrzen adresowa zostala przyznana\n");
    }    
}

void upa_write() {
    writecount = shmat(id_writecount, NULL, 0);
    if (*(int *)writecount == -1) {
        perror("Blad - nie można przydzielić adresu.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("Przestrzen adresowa zostala przyznana\n");
    }
}

void wait_all() {
    int cpid, status;

    for (int i = 0; i < liczba_pisarzy + liczba_czytelnikow; i++) {
        cpid = wait(&status);
        if (cpid == -1) {
            printf("Blad potomka.\n");
        }
        else {
            printf("Potomek zakonczyl sie poprawnie, id: %d\n", cpid);
        }
    }
}

void spr_argumentow(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Blad - nieprawidłowa liczba argumentow wywolania\n");
        exit(EXIT_FAILURE);
    }

    char *end;
    liczba_pisarzy = strtol(argv[1], &end, 10);
    if (errno == ERANGE) {
        perror("Blad - nieprawidlowy zakres\n");
        exit(EXIT_FAILURE);
    }
    else if (end == argv[1]) {
        printf("Blad - bledny format\n");
        exit(EXIT_FAILURE);
    }

    liczba_czytelnikow = strtol(argv[2], &end, 10);
    if (errno == ERANGE) {
        perror("Blad - nieprawidlowy zakres\n");
        exit(EXIT_FAILURE);
    }
    else if (end == argv[2]) {
        printf("Blad - bledny format\n");
        exit(EXIT_FAILURE);
    }

    liczba_miejsc = strtol(argv[3], &end, 10);
    if (errno == ERANGE) {
        perror("Blad - nieprawidlowy zakres\n");
        exit(EXIT_FAILURE);
    }
    else if (end == argv[3]) {
        printf("Blad - bledny format\n");
        exit(EXIT_FAILURE);
    }

    if (liczba_czytelnikow < 1 || liczba_pisarzy < 1) {
        printf("Blad - zbyt malo %s.\n", liczba_pisarzy < 1 ? (liczba_czytelnikow < 1 ? "producentow i znakow" : "producentow") : "znakow");
        exit(EXIT_FAILURE);
    }

    if (liczba_miejsc <= 0) {
        printf("UWAGA! Wybrano 0 miejsc lub mniej w czytelni, nikt nie moze do niej wejsc\n");
    }

    if (liczba_pisarzy + liczba_czytelnikow > spr_limit()) {
        printf("Blad - proszę podac mniejsza ilosc procesow");
        exit(EXIT_FAILURE);
    }
}

/*
int spr_limit() {

    FILE *fp_in, *fp_in2;

    fp_in = popen("ps u | grep ^4640 | wc -l", "r");

    char tmp[50];
    char *end;
    int suma1;

    if (fp_in != NULL) {
        if (fgets(tmp, sizeof(tmp), fp_in)) {
            suma1 = strtol(tmp, &end, 10);
        }

        if (errno == ERANGE) {
            perror("Blad - nieprawidlowy zakres\n");
            exit(EXIT_FAILURE);
        }

        else if (end == tmp) {
            printf("Blad - bledny format\n");
            exit(EXIT_FAILURE);
        }
    }

    pclose(fp_in);


    fp_in2 = popen("ulimit -u ", "r");

    char tmp2[50];
    char *end2;
    int suma2;

    if (fp_in2 != NULL) {
        if (fgets(tmp2, sizeof(tmp2), fp_in2)) {
            suma2 = strtol(tmp2, &end2, 10);
        }

        if (errno == ERANGE) {
            perror("Blad - nieprawidlowy zakres\n");
            exit(EXIT_FAILURE);
        }
        else if (end2 == tmp2) {
            printf("Blad - bledny format\n");
            exit(EXIT_FAILURE);
        }
    }

    pclose(fp_in2);
    int limit = suma2 - suma1;
    printf("Limit = %d\n", limit);
    return limit;
*/

int spr_limit() {
  char buf[BUF_SIZE];
  int limit;
  int procesy;


  FILE *cmd;

  cmd = popen("ulimit -a | grep process | grep -E '[0-9]+' -o", "r");
  if (cmd == NULL) {
    perror("Błąd popen()");
    exit(EXIT_FAILURE);
  }

  if (fgets(buf, BUF_SIZE, cmd) == 0 || (limit = atoi(buf)) == 0) {
    printf("Pobranie limitu procesów nie powiodło się!\n");
    exit(EXIT_FAILURE);
  }

  if (pclose(cmd) == -1) {
    perror("Błąd pclose()");
    exit(EXIT_FAILURE);
  }


  sprintf(buf, "ps -u %d | wc -l", geteuid());
  cmd = popen(buf, "r");
  if (cmd == NULL) {
    perror("Błąd popen()");
    exit(EXIT_FAILURE);
  }

  if (fgets(buf, BUF_SIZE, cmd) == 0 || (procesy = atoi(buf)) == 0) {
    printf("Pobranie ilości uruchomionych procesów nie powiodło się!\n");
    exit(EXIT_FAILURE);
  }

  if (pclose(cmd) == -1) {
    perror("Błąd pclose()");
    exit(EXIT_FAILURE);
  }


  return limit - procesy;
}

void handler(int sig) {
    printf("Trwa konczenie programu...\n");
    sleep(1);
    clear();
    sleep(1);

    printf("Koniec programu.\n");
    kill(getpid(), SIGINT);
    exit(0);
}

void clear() {
    // czysci semafory z pamieci podlacza sie pod IPC_CREAT
    key_semafor = ftok(".", 'S');
    int semID = utworz_nowy_semafor(key_semafor, LICZBA_SEM);
    usun_semafor(semID);

    // zwalnia pamiec dzielona dla readcount ktory ten program stworzyl

    if (shmdt(readcount) == -1) {
        printf("Blad - nie udalo sie zwolnic pamieci\n");
        exit(EXIT_FAILURE);
    }
    if (shmctl(id_readcount, IPC_RMID, 0) == -1) {
        printf("Blad - nie udalo sie usunac pamieci\n");
        exit(EXIT_FAILURE);
    }
    printf("Pamiec dzielona zostala zwolniona.\n");

    if (shmdt(writecount) == -1) {
        printf("Blad - nie udalo sie zwolnic pamieci\n");
        exit(EXIT_FAILURE);
    }
    if (shmctl(id_writecount, IPC_RMID, 0) == -1) {
        printf("Blad - nie udalo sie usunac pamieci\n");
        exit(EXIT_FAILURE);
    }
    printf("Pamiec dzielona zostala zwolniona.\n");
}