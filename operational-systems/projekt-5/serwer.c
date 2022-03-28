#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
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

void stop(int);
void piperr(int);
void utworzFIFO(char*);
void usunPlik(char *);
int znajdzLimitProcesow();
int pisz(int, struct dane_do_przekazania *, int);
int czytaj(int, struct dane_do_przekazania *, int);
int otworz(char*, int);
void zamknij(int);

int main(int argc, char *argv[]) {
    char klient_nazwa_fifo[BUF_SIZE];
    struct dane_do_przekazania pakiet;
    int sfd, kfd;

    if (sizeof(struct dane_do_przekazania) > PIPE_BUF) {
        printf("Rozmiar danych jest wiekszy niz systemowy rozmiar bufora\n");
        exit(EXIT_FAILURE);
    }

    utworzFIFO(NAZWA_FIFO_SERWER);
    signal(SIGINT, stop);

    printf("[SERWER] Oczekiwanie na pierwszego klienta\n");
    sfd = otworz(NAZWA_FIFO_SERWER, O_RDONLY);
    printf("[SERWER] Wznawianie pracy serwera\n");
    sleep(5);

    while(read(sfd, &pakiet, sizeof(pakiet))) {
        printf("[SERWER] <%d> Odebrano %s\n", pakiet.pid_klienta, pakiet.dane);
        for (int i=0;i<strlen(pakiet.dane); i++) {
            pakiet.dane[i] = toupper(pakiet.dane[i]);
        }

        sprintf(klient_nazwa_fifo, NAZWA_FIFO_KLIENT, pakiet.pid_klienta);
        kfd = otworz(klient_nazwa_fifo, O_WRONLY);

        printf("[SERWER] <%d> Wysylam %s\n", pakiet.pid_klienta, pakiet.dane);
        pisz(kfd, &pakiet, sizeof(pakiet));

        zamknij(kfd);
    }

    printf("[SERWER] Odlaczono ostatniego klienta\n");

    zamknij(sfd);
    usunPlik(NAZWA_FIFO_SERWER);

    return 0;
}

void stop(int signum) {
    usunPlik(NAZWA_FIFO_SERWER);
    exit(EXIT_FAILURE);
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

