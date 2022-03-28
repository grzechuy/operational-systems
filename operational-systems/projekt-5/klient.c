#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUF_SIZE 256
#define DANE_SIZE 10
#define NAZWA_FIFO_SERWER "/tmp/G1"
#define NAZWA_FIFO_KLIENT "/tmp/fifo_%d"

struct dane_do_przekazania {
    pid_t pid_klienta;
    char dane[DANE_SIZE];
};

void piperr(int);
void utworzFIFO(char*);
void usunPlik(char *);
void stop(int);
void wyslijDane(int, int, int);
void stworzKlientow(int, int);
int znajdzLimitProcesow();
int pisz(int, struct dane_do_przekazania *, int);
int czytaj(int, struct dane_do_przekazania *, int);
int otworz(char*, int);
void zamknij(int);
char klient_nazwa_fifo[BUF_SIZE];

int main(int argc, char *argv[]){
    int limit, klienci, dlugoscWiadomosci, i;

    if (sizeof(struct dane_do_przekazania) > PIPE_BUF) {
        printf("[KLIENT] Rozmiar danych jest wiekszy niz systemowy rozmiar bufora\n");
        exit(EXIT_FAILURE);
    }

    if (argc < 3) {
        printf("[KLIENT] Wprowadzono bledna ilosc argumentow. Oczekiwano: (ile klientow) (ile znakow)\n");
        exit(EXIT_FAILURE);
    }

    if ((klienci = atoi(argv[1])) <= 0) {
        printf("[KLIENT] Liczba klientow ma byc wieksza od 0\n");
        exit(EXIT_FAILURE);
    }

    if ((dlugoscWiadomosci = atoi(argv[2])) <= 0 || dlugoscWiadomosci > DANE_SIZE - 1) {
        printf("Liczba znakow ma zawierac sie w przedziale od 1 do %d!\n", DANE_SIZE - 1);
        exit(EXIT_FAILURE);
    }

    //if ((dlugoscWiadomosci = atoi(argv[2])) <= 0 || dlugoscWiadomosci > DANE_SIZE) {
    //     printf("Liczba znakow ma zawierac sie w przedziale od 1 do %d!\n", DANE_SIZE);
    //     exit(EXIT_FAILURE);
    //}

    limit = znajdzLimitProcesow();
    //printf("Limit procesow: %d\n", limit);
    printf("[KLIENT] Klientow: %d, znakow: %d\n", klienci, dlugoscWiadomosci);

    if (klienci > limit) {
        printf("[KLIENT] Blad - przekroczono dopuszczalna ilosc procesow\n");
        exit(EXIT_FAILURE);
    }

    stworzKlientow(klienci, dlugoscWiadomosci);

    int status;
    pid_t pid;

    for (i = 0; i < klienci; i++) {
        pid = wait(&status);
        if (status == 0) {
            //printf("[KLIENT] Pomyslnie zakonczono proces <%d>", i);
        } else {
            printf("[KLIENT] Blad procesu: %d, %d\n", pid, status);
        }

        if (pid == -1) {
            if (errno == ECHILD) {
                printf("[KLIENT] Brak procesow potomnych do obsluzenia\n");
                exit(EXIT_FAILURE);
            } else {
                perror ("[KLIENT] Blad podczas oczekiwania na proces potomny\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    return 0;
}

void pipeerr(int signum) {
    printf("Nieoczekiwane zamkniecie drugiego konca potoku\n");
    exit(EXIT_FAILURE);
}

int znajdzLimitProcesow() {
    char buf[BUF_SIZE];
    int limit, procesy;
    FILE *cmd;

    cmd = popen("ulimit -a | grep process | grep -E '[0-9]+' -o", "r");
    if (cmd == NULL) {
        perror("Blad popen\n");
        exit(EXIT_FAILURE);
    }
    if (fgets(buf, BUF_SIZE, cmd) == 0 || (limit = atoi(buf)) == 0) {
        printf("Blad podczas pobierania limitow procesow\n");
        exit(EXIT_FAILURE);
    }
    if (pclose(cmd) == -1) {
        perror("Blad pclose\n");
        exit(EXIT_FAILURE);
    }

    sprintf(buf, "ps -u %d | wc -l", geteuid());
    cmd = popen(buf, "r");

    if (cmd == NULL) {
         perror("Blad popen\n");
        exit(EXIT_FAILURE);
    }

    if (fgets(buf, BUF_SIZE, cmd) == 0 || (procesy = atoi(buf)) == 0) {
        printf("Blad podczas pobierania ilosci uruchomionych procesow\n");
        exit(EXIT_FAILURE);
    }

    if (pclose(cmd) == -1) {
        perror("Blad pclose\n");
        exit(EXIT_FAILURE);
    }

    return limit - procesy;
}

int pisz(int fd, struct dane_do_przekazania *pakiet, int rozmiar) {
    int dlugosc = write(fd, pakiet, rozmiar);

    if (dlugosc == -1) {
        perror("Blad podczas pisania do FIFO\n");
        kill(getpid(), SIGINT);
    } else if (dlugosc != rozmiar) {
        pisz(fd, pakiet, rozmiar);
    } else {
        return dlugosc;
    }
}

int czytaj(int fd, struct dane_do_przekazania *pakiet, int rozmiar) {
    int dlugosc = read(fd, pakiet, rozmiar);

    if (dlugosc == -1) {
        if (errno == EINTR) {
            czytaj(fd, pakiet, rozmiar);
        } else {
            perror("Blad podczas czytania z FIFO\n");
            kill(getpid(), SIGINT);
        }
    } else {
        return dlugosc;
    }
}

int otworz(char* plik, int flag) {
    int fd = open(plik, flag);

    if (fd == -1) {
        if (errno == EINTR) {
            otworz(plik, flag);
        } else {
            printf("Mozliwe, ze serwer nie jest uruchomiony\n");
            perror("Blad podczas otwierania pliku\n");
            kill(getpid(), SIGINT);
        }
    } else {
        return fd;
    }
}

void zamknij(int fd) {
    int i = close(fd);

    if (i == -1) {
        perror("Blad podczas zamykania pliku\n");
        kill(getpid(), SIGINT);
    }
}

void utworzFIFO(char *fifo_nazwa) {
    int i = mkfifo(fifo_nazwa, 0660);

    if (i == -1) {
        perror("Blad podczas tworzenia fifo\n");
        exit(EXIT_FAILURE);
    }
}

void usunPlik(char *plik_nazwa) {
    int i = unlink(plik_nazwa);
    if (i == -1) {
        perror("Blad podczas usuwania pliku\n");
        exit(EXIT_FAILURE);
    }
}

void stworzKlientow(int ile, int dlWiadomosci) {
    pid_t pid;
    int i;

    for (i = 0; i < ile; i++) {
        pid = fork();

        switch(pid) {
            case -1:
            if (errno == EAGAIN) {
                printf("[KLIENT] Osiagnieto limit uruchomionych procesow\n");
                exit(EXIT_FAILURE);
            } else {
                perror("[KLIENT] Blad forkowania\n");
                exit(EXIT_FAILURE);
            }
            break;

            case 0:
            wyslijDane(i, dlWiadomosci, 1);
            break;
        }
    }
}

void wyslijDane(int nrKlienta, int dlWiadomosci, int ile) {
    int i, j, kfd, l;
    char c;
    sprintf(klient_nazwa_fifo, NAZWA_FIFO_KLIENT, getpid());
    srand(getpid()*getppid());

    struct dane_do_przekazania pakiet;
    pakiet.pid_klienta = getpid();

    signal(SIGPIPE, pipeerr);
    signal(SIGINT, stop);

    int fsfd = otworz(NAZWA_FIFO_SERWER, O_WRONLY);

    utworzFIFO(klient_nazwa_fifo);
    
    for (i = 0; i < ile; i++) {
        for (j = 0; j < dlWiadomosci; j++) {
            c = (char)((rand()%26)+97);
            pakiet.dane[j] = c;
        }
        pakiet.dane[dlWiadomosci] = '\0';

        printf("[KLIENT] <%d> Wysylam %s\n", nrKlienta, pakiet.dane);

        int dl = pisz(fsfd, &pakiet, sizeof(pakiet));
        //printf("[KLIENT] <%d> Wyslalem pakiet, wielkosc: %d\n", nrKlienta, dl);
        printf("[KLIENT] <%d> Wyslalem pakiet\n", nrKlienta);

        kfd = otworz(klient_nazwa_fifo, O_RDONLY);
        czytaj(kfd, &pakiet, sizeof(pakiet));
        printf("[KLIENT] <%d> Odebrano %s\n", nrKlienta, pakiet.dane);
    }
    zamknij(fsfd);
    zamknij(kfd);
    usunPlik(klient_nazwa_fifo);
    exit(EXIT_SUCCESS);
}

void stop(int signum) {
    usunPlik(klient_nazwa_fifo);
    exit(2);
}