#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>

#define PRODUCENT 0
#define KONSUMENT 1

int pipeDescriptors[2];
long ileProducentow, ileKonsumentow, ileZnakow;

void sprawdzArgumenty(int argc, char *argv[]);
void utworz(int ile, int kogo);
void producent();
void konsument();
char jakiZnak();

int main(int argc, char *argv[]) {
    int cpid, status;

    sprawdzArgumenty(argc, argv);

    pipe(pipeDescriptors);

    utworz(ileProducentow, PRODUCENT);
    utworz(ileKonsumentow, KONSUMENT);

    close(pipeDescriptors[0]);
    close(pipeDescriptors[1]);

    for (int i = 0; i < ileProducentow + ileKonsumentow; i++) {
        cpid = wait(&status);
        if (cpid == -1) {
            printf("BLAD POTOMKA - KOD POWROTU POTOMKA: %d\n", status/256);
        }
        else {
            printf("POTOMEK ZAKONCZYL SIE POPRAWNIE, ID: %d\n", cpid);
        }
    }

    return 0;
}

int sprawdzLimit() {
    FILE *in, *in2;

    in = popen("ps ux | grep gzaprzala | wc -l", "r");

    char temp[50];
    char *koniec;
    int suma1;

    if (in != NULL) {
        if (fgets(temp, sizeof(temp), in)) {
            suma1 = strtol(temp, &koniec, 10);
        }

        if (errno == ERANGE) {
            perror("BLAD - NIEPRAWIDLOWY ZAKRES\n");
            exit(EXIT_FAILURE);
        }
        else if (koniec == temp) {
            printf("BLAD - NIEPRAWIDLOWY FORMAT\n");
            exit(EXIT_FAILURE);
        }
    }

    pclose(in);

    in2 = popen("ulimit -u", "r");

    char temp2[50];
    char *koniec2;
    int suma2;

    if (in2 != NULL) {
        if (fgets(temp2, sizeof(temp2), in2)) {
            suma2 = strtol(temp2, &koniec2, 10);
        }

        if (errno == ERANGE) {
            perror("BLAD - NIEPRAWIDLOWY ZAKRES\n");
            exit(EXIT_FAILURE);
        }
        else if(koniec == temp2) {
            printf("BLAD - NIEPRAWIDLOWY FORMAT\n");
            exit(EXIT_FAILURE);
        }
    }

    pclose(in2);

    int limit = suma2 - suma1;
    printf("LIMIT: %d\n", limit);

    return limit;
}

void sprawdzArgumenty(int argc, char *argv[]) {
    if (argc !=4) {
        printf("BLAD - NIEPRAWIDLOWA LICZBA ARGUMENTOW WYWOLANIA\n");
        exit(EXIT_FAILURE);
    }

    char *koniec;
    ileProducentow = strtol(argv[1], &koniec, 10);

    if (errno == ERANGE) {
        perror("BLAD - PRZEKROCZONO LIMIT PRODUCENTOW\n");
        exit(EXIT_FAILURE);
    }
    else if (koniec == argv[1]) {
        printf("BLAD - BLEDNY FORMAT\n");
        exit(EXIT_FAILURE);
    }

    ileKonsumentow = strtol(argv[2], &koniec, 10);

    if (errno == ERANGE) {
        perror("BLAD - PRZEKROCZONO LIMIT KONSUMENTOW\n");
        exit(EXIT_FAILURE);
    }
    else if (koniec == argv[2]) {
        printf("BLAD - BLEDNY FORMAT\n");
    }

    ileZnakow = strtol(argv[3], &koniec, 10);

    if (errno == ERANGE) {
        perror("BLAD - PRZEKROCZONO LIMIT ZNAKOW\n");
        exit(EXIT_FAILURE);
    }
    else if (koniec == argv[3]) {
        printf("BLAD - BLEDNY FORMAT\n");
        exit(EXIT_FAILURE);
    }

    if (ileProducentow < 1 || ileKonsumentow < 1) {
        printf("BLAD - ZA MALO %s.\n", ileProducentow < 1 ? (ileKonsumentow < 1 ? "PRODUCENTOW I KONSUMENTOW" : "PRODUCENTOW") : "KONSUMENTOW");
        exit(EXIT_FAILURE);
    }

    if (ileProducentow + ileKonsumentow > sprawdzLimit()) {
        printf("BLAD - PODAJ MNIEJSZA ILOSC PROCESOW\n");
        exit(EXIT_FAILURE);
    }
}

void utworz(int ile, int kogo) {
    for (int i = 0; i < ile; i++) {
        switch(fork()) {
            case -1:
            perror("BLAD - NIEPRAWIDLOWE WYWOLANIE fork()\n");
            exit(EXIT_FAILURE);

            case 0:
            if (kogo == PRODUCENT) {
                producent();
                printf("PRODUCENT\n");
            }
            
            else {
                konsument();
                printf("KONSUMENT\n");
            }
            
            exit(0);
        }
    }
}

void producent() {
    char plikInput[32];
    char znak;

    close(pipeDescriptors[0]);
    sprintf(plikInput, "we_%ld.txt", (long int)getpid());

    FILE *plik = fopen(plikInput, "w");
    if (!plik) {
        printf("BLAD PODCZAS OTWIERANIA PLIKU Z DANYMI\n");
        exit(1);
    }

    for (int i = 0; i < ileZnakow; i++) {
        znak = jakiZnak();
        fprintf(plik, "%c", znak);
        write(pipeDescriptors[1], &znak, sizeof(char));
    }

    fclose(plik);
    close(pipeDescriptors[1]);

    return;
}

void konsument() {
    char plikOutput[32];
    char znak;

    close(pipeDescriptors[1]);
    sprintf(plikOutput, "wy_%ld.txt", (long int)getpid());

    FILE *plik = fopen(plikOutput, "w");
    if (!plik) {
        perror("BLAD PODCZAS OTWIERANIA PLIKU WYNIKOWEGO\n");
        exit(EXIT_FAILURE);
    }

    while(read(pipeDescriptors[0], &znak, sizeof(char))) {
        putc(znak, plik);
    }

    close(pipeDescriptors[0]);
}

char jakiZnak() {
    char znak;
    srand((int)getpid());

    znak = rand()%(126-32)+32;

    return znak;
}