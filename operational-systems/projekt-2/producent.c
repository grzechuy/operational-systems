#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>

static void utworz_nowy_semafor(int);
static void ustaw_semafor(int);
static void usun_semafor(void);
static void semafor_p(int);
static void semafor_v(int);
void upd();
void upa();
void odlacz_pamiec();

char *adres;
int klucz = 10;
int rozmiar = sizeof(char);
int pamiec, odlaczenie_1, odlaczenie_2, semafor;

int main() {
    double losoweCzekanie;
    srand(time(NULL));

    FILE *plik_input;
    if ((plik_input = fopen("input.txt", "r")) == NULL) {
        perror("\tPRODUCENT:\tBLAD FOPEN\n");
        exit(1);   
    }

    utworz_nowy_semafor(2);
    ustaw_semafor(0);
    ustaw_semafor(1);

    semafor_p(1);

    upd();
    upa();

    while(1) {
        semafor_p(0);
        fscanf(plik_input, "%c", adres);
        if (feof(plik_input)) {
            break;
        }

        printf("\tPRODUCENT:\tTRWA PRODUKCJA...\n");
        losoweCzekanie = ((double)rand()) / RAND_MAX;
        printf("\tPRODUCENT:\tCZAS TRWANIA PRODUKCJI: %lfs\n", losoweCzekanie);
        sleep(losoweCzekanie);
        printf("\tPRODUCENT:\tWYPRODUKOWANO: %c\n", *adres);
        semafor_v(1);
    }

    *adres = 0;
    printf("\tPRODUCENT:\tKONIEC PRODUKCJI.\n");
    semafor_v(1);
    semafor_p(0);

    fclose(plik_input);
    odlacz_pamiec();
    usun_semafor();
    exit(EXIT_SUCCESS);

    return 0;
}

static void utworz_nowy_semafor(int ile) {
    semafor = semget(ftok("konsument.out", klucz), ile, 0600 | IPC_CREAT);
    if (semafor == - 1) {
        printf("\tPRODUCENT:\tNIE MOGLEM UTWORZYC NOWEGO SEMAFORA.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tPRODUCENT:\tSEMAFOR ZOSTAL UTWORZONY: %d\n.", semafor);
    }
}

static void ustaw_semafor(int numer) {
    int ustawSemafor;
    ustawSemafor = semctl(semafor, numer, SETVAL, 1);
    if (ustawSemafor == -1) {
        printf("\tPRODUCENT:\tNIE MOGLEM USTAWIC SEMAFORA.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tPRODUCENT:\tSEMAFOR ZOSTAL USTAWIONY.\n");
    }
}

static void usun_semafor() {
    int sem;
    sem = semctl(semafor, 0, IPC_RMID);
    if (sem == -1) {
        printf("\tPRODUCENT:\tNIE MOGLEM USUNAC SEMAFORA.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tPRODUCENT:\tSEMAFOR ZOSTAL USUNIETY: %d\n", sem);
    }
}

static void semafor_p(int numer) {
    struct sembuf bufor_sem;
    bufor_sem.sem_num = numer;
    bufor_sem.sem_op = -1;
    bufor_sem.sem_flg = 0;

    if (semop(semafor, &bufor_sem, 1) == -1) {
        if (errno = EINTR) {
            printf("\tPRODUCENT:\tWZNAWIAM PROCES\n");
            semafor_p(numer);
        }
        else {
            printf("\tPRODUCENT:\tNIE MOGLEM ZABLOKOWAC SEKCJI KRYTYCZNEJ NR %d\n", numer);
            exit(EXIT_FAILURE);
        }
    }
    else {
        printf("\tPRODUCENT:\tSEKCJA KRYTYCZNA NR %d ZABLOKOWANA.\n", numer);
    }
}

static void semafor_v(int numer) {
    struct sembuf bufor_sem;
    bufor_sem.sem_num = numer;
    bufor_sem.sem_op = 1;
    bufor_sem.sem_flg = 0;

    if (semop(semafor, &bufor_sem, 1) == -1) {
        if (errno = EINTR) {
            printf("\tPRODUCENT:\tWZNAWIAM PROCES\n");
            semafor_v(numer);
        }

        printf("\tPRODUCENT:\tNIE MOGLEM ZABLOKOWAC SEKCJI KRYTYCZNEJ NR %d\n", numer);
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tPRODUCENT:\tSEKCJA KRYTYCZNA NR %d ODBLOKOWANA.\n", numer);
    }
}

void upd() {
    pamiec = shmget(ftok("konsument.out", klucz), rozmiar, 0600 | IPC_CREAT);
    if (pamiec == -1) {
        printf("\tPRODUCENT:\tBLAD PRZY TWORZENIU PAMIECI DZIELONEJ.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tPRODUCENT:\tPAMIEC DZIELONA UTWORZONA: %d\n", pamiec);
    }
}

void upa() {
    adres = shmat(pamiec, 0, 0);
    if (*adres == -1) {
        printf("\tPRODUCENT:\tBLAD PRZY PRZYDZIELANIU ADRESU.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tPRODUCENT:\tPRZESTRZEN ADRESOWA PRZYZNANA: %s\n", adres);
    }
}

void odlacz_pamiec() {
    odlaczenie_1 = shmctl(pamiec, IPC_RMID, 0);
    odlaczenie_2 = shmdt(adres);

    if (odlaczenie_1 == -1 || odlaczenie_2 == -1) {
        printf("\tPRODUCENT:\tBLAD PRZY ODLACZANIU PAMIECI DZIELONEJ.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tPRODUCENT:\tPAMIEC DZIELONA ODLACZONA.\n");
    }
}