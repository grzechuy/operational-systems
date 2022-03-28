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
static void semafor_v(int, int);
static void semafor_p(int, int);
void utworzFilozofow(pid_t);
void myslenie(int);
void myslenieGlodzenie(int);
void jedzenieGlodzenie(int);
void jedzenie(int);
void handler(int);
key_t returnKey();

pid_t pID;
int semID;
int wait_licz = 0;
int licz = 0;

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
    while(1) {
        myslenieGlodzenie(numer);
        semafor_p(numer, (numer + 1) % 5);
        jedzenieGlodzenie(numer);
        semafor_v(numer, (numer + 1) % 5);
    }
    exit(0);
}

static void dolaczZbiorSemaforow(key_t key) {
    semID = semget(key, 5, 0666 | IPC_CREAT);
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
    for (int i = 0; i < 5; i++) {
        ustawSem = semctl(semID, i, SETVAL, 1);
        if (ustawSem == -1) {
            perror("Blad - nie mozna ustawic semafora\n");
            exit(EXIT_FAILURE);
        }
        else {
            printf("Semafor %d zostal ustawiony\n", i);
        }
    }
}

static void semafor_v(int numer1, int numer2) {
    int operSem;
    struct sembuf buforSem[2];
    buforSem[0].sem_num = numer1;
    buforSem[0].sem_op = 1;
    buforSem[1].sem_num = numer2;
    buforSem[1].sem_op = 1;
    operSem = semop(semID, buforSem, 2);
    if (operSem == -1 && errno == EINTR) {
        printf("Otwieranie semafora po zatrzymaniu procesu\n");
        semafor_v(numer1, numer2);
    }
    else if (operSem == -1 && errno == EAGAIN) {
        printf("Blad - semafor nie jest jeszcze otwarty\n");
        semctl(semID, numer1, SETVAL, 1);
        semctl(semID, numer2, SETVAL, 1);
        semafor_v(numer1, numer2);
    }
    else if (operSem == -1) {
        printf("Blad - semafor nie jest jeszcze otwarty\n");
        exit(EXIT_FAILURE);
    }
}

static void semafor_p(int numer1, int numer2) {
    int operSem;
    struct sembuf buforSem[2];
    buforSem[0].sem_num = numer1;
    buforSem[0].sem_op = -1;
    buforSem[1].sem_num = numer2;
    buforSem[1].sem_op = -1;
    operSem = semop(semID, buforSem, 2);
    if (operSem == -1 && errno == EINTR) {
        printf("Zamykanie semafora po zatrzymaniu procesu\n");
        semafor_p(numer1, numer2);
    }
    else if (operSem == -1 && errno == EAGAIN) {
        printf("Blad - semafor nie zostal zamkniety\n");
        semctl(semID, numer1, SETVAL, 1);
        semctl(semID, numer2, SETVAL, 1);
        semafor_p(numer1, numer2);
    }
    else if (operSem == -1) {
        printf("Blad - semafor nie zostal zamkniety\n");
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
    sleep(rand() % 2 + numer);
    printf("Myslenie - filozof %d\n", numer);
}

void myslenieGlodzenie(int numer) {
  
    if (numer == 0) {
        printf("Myslenie - filozof %d\n", numer);
    }
    else if (numer == 1) {
        wait_licz++;
        printf("Myslenie - filozof %d\n", numer);
    }
    else if (numer == 2) {
        printf("Myslenie - filozof %d\n", numer);
    }
    else if (numer == 3) {
        printf("Myslenie - filozof %d\n", numer);
    }
    else if (numer == 4) {
        printf("Myslenie - filozof %d\n", numer);
    }
}

void jedzenie(int numer) {
    sleep(rand() % 2 + 1 + numer);
    printf("Jedzenie - filozof %d\n", numer);
}

void jedzenieGlodzenie(int numer) {
    licz++;
    if (numer == 0) {
        printf("Poczatek jedzenia - filozof %d\n", numer);
        printf("Koniec jedzenia - filozof %d\n", numer);
    }
    else if (numer == 1) {
        printf("Poczatek jedzenia - filozof %d\n", numer);
        sleep(rand() % 3);
        printf("Koniec jedzenia - filozof %d\n", numer);
    }
    else if (numer == 2) {
        printf("Poczatek jedzenia - filozof %d\n", numer);
        //sleep(rand()%3);
        printf("Koniec jedzenia - filozof %d\n", numer);
    }
    else if (numer == 3) {
        printf("Poczatek jedzenia - filozof %d\n", numer);
        sleep(rand() % 3);
        printf("Koniec jedzenia - filozof %d\n", numer);
    }
    else if (numer == 1) {
        printf("Poczatek jedzenia - filozof %d\n", numer);
        printf("Koniec jedzenia - filozof %d\n", numer);
    }
    printf("Licznik filozofa %d = %d\n", numer, licz);
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