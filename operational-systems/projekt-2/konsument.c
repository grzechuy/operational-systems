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

    FILE *plik_output;
    if ((plik_output = fopen("output.txt", "w")) == NULL) {
        perror("\tKONSUMENT:\tBLAD FOPEN\n");
        exit(1);   
    }

    utworz_nowy_semafor(2);

    upd();
    upa();

    while(1) {
        semafor_p(1);
        if (!(*adres)) {
            break;
        }

        printf("\tKONSUMENT:\tTRWA KONSUMPCJA...\n");
        losoweCzekanie = ((double)rand()) / RAND_MAX;
        printf("\tKONSUMENT:\tCZAS TRWANIA KONSUMPCJI: %lfs\n", losoweCzekanie);
        sleep(losoweCzekanie);
        fprintf(plik_output, "%c", *adres);
        printf("\tKONSUMENT:\tSKONSUMOWANO: %c\n", *adres);
        semafor_v(0);
    }

    printf("\tKONSUMENT:\tKONIEC KONSUMPCJI.\n");

    odlacz_pamiec();
    semafor_v(0);

    return 0;
}

static void utworz_nowy_semafor(int ile) {
    semafor = semget(ftok("konsument.out", klucz), ile, 0600 | IPC_CREAT);
    if (semafor == - 1) {
        printf("\tKONSUMENT:\tNIE MOGLEM UTWORZYC NOWEGO SEMAFORA.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tKONSUMENT:\tSEMAFOR ZOSTAL UTWORZONY: %d\n.", semafor);
    }
}

static void semafor_p(int numer) {
    struct sembuf bufor_sem;
    bufor_sem.sem_num = numer;
    bufor_sem.sem_op = -1;
    bufor_sem.sem_flg = 0;

    if (semop(semafor, &bufor_sem, 1) == -1) {
        if (errno = EINTR) {
            printf("\tKONSUMENT:\tWZNAWIAM PROCES\n");
            semafor_p(numer);
        }
        else {
            printf("\tKONSUMENT:\tNIE MOGLEM ZABLOKOWAC SEKCJI KRYTYCZNEJ NR %d\n", numer);
            exit(EXIT_FAILURE);
        }
    }
    else {
        printf("\tKONSUMENT:\tSEKCJA KRYTYCZNA NR %d ZABLOKOWANA.\n", numer);
    }
}

static void semafor_v(int numer) {
    struct sembuf bufor_sem;
    bufor_sem.sem_num = numer;
    bufor_sem.sem_op = 1;
    bufor_sem.sem_flg = 0;

    if (semop(semafor, &bufor_sem, 1) == -1) {
        if (errno == EINTR) {
            printf("\tKONSUMENT:\tWZNAWIAM PROCES\n");
            semafor_v(numer);
        }

        printf("\tKONSUMENT:\tNIE MOGLEM ZABLOKOWAC SEKCJI KRYTYCZNEJ NR %d\n", numer);
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tKONSUMENT:\tSEKCJA KRYTYCZNA NR %d ODBLOKOWANA.\n", numer);
    }
}

void upd() {
    pamiec = shmget(ftok("konsument.out", klucz), rozmiar, 0600 | IPC_CREAT);
    if (pamiec == -1) {
        printf("\tKONSUMENT:\tBLAD PRZY TWORZENIU PAMIECI DZIELONEJ.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tKONSUMENT:\tPAMIEC DZIELONA UTWORZONA: %d\n", pamiec);
    }
}

void upa() {
    adres = shmat(pamiec, 0, 0);
    if (*adres == -1) {
        printf("\tKONSUMENT:\tBLAD PRZY PRZYDZIELANIU ADRESU.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tKONSUMENT:\tPRZESTRZEN ADRESOWA PRZYZNANA: %s\n", adres);
    }
}

void odlacz_pamiec() {
    odlaczenie_1 = shmctl(pamiec, IPC_RMID, 0);
    odlaczenie_2 = shmdt(adres);

    if (odlaczenie_1 == -1 || odlaczenie_2 == -1) {
        printf("\tKONSUMENT:\tBLAD PRZY ODLACZANIU PAMIECI DZIELONEJ.\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("\tKONSUMENT:\tPAMIEC DZIELONA ODLACZONA.\n");
    }
}