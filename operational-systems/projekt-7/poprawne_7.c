#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

static void dolaczZbiorSemaforow(key_t);
static void usunZbiorSemaforow();
static void ustawSemafory();
static void semafor_v(int);
static void semafor_p(int);
void utworzFilozofow(pid_t);
void myslenie(int);
void jedzenie(int);
void handler(int);
key_t returnKey();

pid_t pID;
int semID;

int main(int argc, char **argv) {
    srand(time(NULL));
    int numerPID, numer;
    pID = getpid();
    signal(SIGINT, handler);
    if (pID == getpid()) {
        utworzFilozofow(pID);
    }
    numerPID = getpid();
    numer = numerPID % 5;
    printf("Numer %d:\n", numer);
    while (1) {
        myslenie(numer);
        semafor_p(5);
        semafor_p(numer);
        semafor_p((numer + 1) % 5);
        jedzenie(numer);
        semafor_v(numer);
        semafor_v((numer + 1) % 5);
        semafor_v(5);
    }

    exit(0);
}

static void dolaczZbiorSemaforow(key_t key) {
    semID = semget(key, 6, 0666 | IPC_CREAT);
    if (semID == -1) {
        exit(EXIT_FAILURE);
    }
    else {
        printf("Dolaczono zbior semaforow %d do pID %d\n", semID, getpid());
    }
}

static void usunZbiorSemaforow() {
    int usunSem;
    usunSem = semctl(semID, 0, IPC_RMID);
    if (usunSem == -1 && errno != EINVAL) {
        perror("Blad - nie mozna usunac zbioru\n");
        exit(EXIT_FAILURE);
    }
    else if (usunSem == -1 && errno == EINVAL) {
        printf("Blad - zbior jest juz usuniety\n");
    }
    else {
        printf("Zbior zostal usuniety %d\n", usunSem);
    }
}

static void ustawSemafory() {
    int ustawSem;
    for (int i = 0; i < 6; i++) {
        if (i < 5) {
            ustawSem = semctl(semID, i, SETVAL, 1);
        }
        else if (i == 5) {
            ustawSem = semctl(semID, i, SETVAL, 4);
        }
        if (ustawSem == -1) {
            perror("Blad - nie mozna ustawic semafora\n");
            exit(EXIT_FAILURE);
        }
        else {
            printf("Semafor %d zostal ustawiony\n", i);
        }
    }
}

static void semafor_v(int numer) {
    int operSem;
    struct sembuf buforSem;
    buforSem.sem_num = numer;
    buforSem.sem_op = 1;
    operSem = semop(semID, &buforSem, 1);
    if (operSem == -1 && errno == EINTR) {
        printf("Otwieranie semafora po zatrzymaniu procesu %d\n", numer);
        semafor_v(numer);
    }
    else if (operSem == -1 && errno == EAGAIN) {
        printf("Blad - semafor %d nie jest jeszcze otwarty\n", numer);
        semctl(semID, numer, SETVAL, 1);
        semafor_v(numer);
    }
    else if (operSem == -1) {
        printf("Blad - semafor %d nie jest jeszcze otwarty\n", numer);
        exit(EXIT_FAILURE);
    }
}

static void semafor_p(int numer) {
    int operSem;
    struct sembuf buforSem;
    buforSem.sem_num = numer;
    buforSem.sem_op = -1;
    operSem = semop(semID, &buforSem, 1);
    if (operSem == -1 && errno == EINTR) {
        printf("Zamykanie semafora po zatrzymaniu procesu %d\n", numer);
        semafor_p(numer);
    }
    else if (operSem == -1 && errno == EAGAIN) {
        printf("Blad - semafor %d nie zostal zamkniety\n", numer);
        semctl(semID, numer, SETVAL, 1);
        semafor_p(numer);
    }
    else if (operSem == -1) {
        printf("Blad - semafor %d nie zostal zamkniety\n", numer);
        exit(EXIT_FAILURE);
    }
}

void utworzFilozofow(pid_t pID) {
    key_t key = returnKey();
    dolaczZbiorSemaforow(key);
    ustawSemafory();
    for (int i = 0; i < 4; i++) {
        if (getpid() == pID) {
            switch (fork()) {
                case -1:
                    perror("Blad przy tworzeniu procesu potomnego\n");
                    exit(EXIT_FAILURE);
                case 0:
                    printf("Utworzono filozofa o pID %d\n", getpid());
                    dolaczZbiorSemaforow(key);
                    break;
                default:
                    break;
            }
        }
    }
}

void myslenie(int numer) {
    sleep(rand() % 2 + 1);
    printf("Myslenie - filozof %d\n", numer);
}

void jedzenie(int numer) {
    sleep(rand() % 2 + 1);
    printf("Jedzenie - filozof %d\n", numer);
}

void handler(int sig) {
    int status, w;
    printf("Sygnal SIGINT od %d\n", getpid());
    if (pID == getpid()) {
        usunZbiorSemaforow();
        for (int i = 0; i < 4; i++) {
            w = wait(&status);
            if (w == -1) {
                perror("Blad error\n");
            }
            else {
                printf("Proces potomny pID %d zakonczyl sie z kodem %d\n", w, status);
            }
        }
    }
    exit(0);
}

key_t returnKey() {
    key_t key;
    key = ftok(".", 'B');
    return key;
}

