#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

static int utworz_nowy_semafor(int klucz, int liczba_elementow);
static void ustaw_semafor(int semid, int numer_semafora, int binarna_wartosc);
static void semafor_p(int semid, int numer_semafora);
static void semafor_v(int semid, int numer_semafora);
static void usun_semafor(int semid);
static int zwroc_wartosc(int semid, int numer_semafora);

static int utworz_nowy_semafor(int klucz, int liczba_elementow) {
    int id_semafora = semget(klucz, liczba_elementow, IPC_CREAT | 0600);

    if (id_semafora == -1) {
        perror("Nie moglem utworzyc semafora.\n");
        exit(EXIT_FAILURE);
    }
    //else printf("Semafor został utworzony.\n");
}

static void ustaw_semafor(int id_semafora, int numer_semafora, int binarna_wartosc) {
    int ustaw_sem;
    ustaw_sem = semctl(id_semafora, numer_semafora, SETVAL, binarna_wartosc);
    if (ustaw_sem == -1) {
        printf("Nie mozna ustawic semafora.\n");
        exit(EXIT_FAILURE);
    }
    else {
        //printf("Semafor zostal ustawiony.\n");
    }
}

static int zwroc_wartosc(int id_semafora, int numer_semafora) {
    int zmien_sem;
    struct sembuf bufor_sem;
    bufor_sem.sem_num = numer_semafora;
    return bufor_sem.sem_op;
}

static void semafor_p(int id_semafora, int numer_semafora) {
    int zmien_sem;
    struct sembuf bufor_sem;
    bufor_sem.sem_num = numer_semafora;
    bufor_sem.sem_op = -1;
    bufor_sem.sem_flg = 0;
    zmien_sem = semop(id_semafora, &bufor_sem, 1);
    if (zmien_sem == -1) {
        if (errno == EINTR) {
            semafor_p(id_semafora, numer_semafora);
        }
        else if (errno == EIDRM) {
            printf("Zestaw semaforow zostal już usunięty, błąd EIDRM\n");
        }
        else {
            printf("Nie moglem zamknac semafora.\n");
            exit(EXIT_FAILURE);
        }
  }
  else {
    // printf("Semafor zostal zamkniety.\n");
  }
}

static void semafor_v(int id_semafora, int numer_semafora) {
    int zmien_sem;
    struct sembuf bufor_sem;
    bufor_sem.sem_num = numer_semafora;
    bufor_sem.sem_op = 1;
    bufor_sem.sem_flg = 0;
    zmien_sem = semop(id_semafora, &bufor_sem, 1);
    if (zmien_sem == -1) {
        if (errno == EIDRM) {
            printf("Zestaw semaforow zostal już usunięty, błąd EIDRM\n");
        }
        else {
            printf("Nie moglem otworzyc semafora.\n");
            exit(EXIT_FAILURE);
        }
    }
    else {
        //printf("Semafor zostal otwarty.\n");
    }
}

static void usun_semafor(int id_semafora) {
    int sem;
    sem = semctl(id_semafora, 0, IPC_RMID);
    if (sem == -1) {
        printf("Nie mozna usunac semafora.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("Semafor zostal usuniety.\n");
    }
}