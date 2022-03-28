#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

#define STR2(x) #x
#define STR(X) STR2(X)

#define MESSAGE_SIZE 4

struct Komunikat {
    long int odbiorca;
    long int nadawca;
    char zawartosc[MESSAGE_SIZE];
};

key_t klucz = 0x2000;

int kolejkaID;
int rozmiarKomunikatu;

void funkcjaInicjalizacyjna();
void dolaczDoKolejki();

void wyswietlKomunikat(struct Komunikat* komunikat);
void odbierzKomunikat(struct Komunikat* komunikat);
void wyslijKomunikat(char *komunikat);

void watekUtworz(pthread_t *watekID, void *funkcjaWatek);
void watekOczekiwanie(pthread_t watekID);
void *watekNadawcy();
void *watekOdbiorcy();

int main() {
    pthread_t idWatkuNadawcy;
    pthread_t idWatkuOdbiorcy;

    funkcjaInicjalizacyjna();
    dolaczDoKolejki();

    watekUtworz(&idWatkuNadawcy, &watekNadawcy);
    watekUtworz(&idWatkuOdbiorcy, &watekOdbiorcy);

    watekOczekiwanie(idWatkuNadawcy);
    watekOczekiwanie(idWatkuOdbiorcy);
    return 0;
}

void funkcjaInicjalizacyjna(){
    rozmiarKomunikatu = MESSAGE_SIZE * sizeof(char) + sizeof(long int);
}

void dolaczDoKolejki(){
    kolejkaID = msgget(klucz, 0600 | IPC_EXCL);

    if (kolejkaID == -1) {
        if (errno == ENOENT) {
            printf("KLIENT:\tBLAD PODCZAS DOLACZANIA DO KOLEJKI, MOZLIWE ZE SERWER NIE JEST URUCHOMIONY\n");
            exit(EXIT_FAILURE);
        }
        else {
            printf("KLIENT:\tBLAD PODCZAS DOLACZANIA DO KOLEJKI\n");
            exit(EXIT_FAILURE);
        }
    }
    else {
        printf("KLIENT:\tPOMYSLNIE DOLACZONO DO KOLEJKI O ID: %d\n", kolejkaID);
    }
}

void wyswietlKomunikat(struct Komunikat* komunikat) {
    printf("KLIENT:\tODEBRANO KOMUNIKAT\n");
    printf("\tNADAWCA: %ld\n", komunikat->nadawca);
    printf("\tODBIORCA: %ld\n", komunikat->odbiorca);
    printf("\tTRESC: %s\n\n", komunikat->zawartosc);
}

void odbierzKomunikat(struct Komunikat* komunikat) {
    /*
    if (msgrcv(kolejkaID, komunikat, rozmiarKomunikatu, (long int) getpid(), 0) == -1){
       if (errno == EINTR) {
            return odbierzKomunikat(komunikat);
        }
        else if (errno == EIDRM) {
            printf("KLIENT:\tBLAD PODCZAS ODBIERANIA KOMUNIKATU, KOLEJKA SKASOWANA\n");
            exit(EXIT_FAILURE);
        }
        else if (errno == E2BIG) {
            printf("KLIENT:\tBLAD PODCZAS ODBIERANIA KOMUNIKATU, ROZMIAR KOMUNIKATU PRZEKRACZA OCZEKIWANA WARTOSC\n");
            exit(EXIT_FAILURE);
        }
        else {
            perror("KLIENT:\tBLAD PODCZAS ODBIERANIA KOMUNUKATU\n");
            exit(EXIT_FAILURE);
        }
        return -1;
    }
    return 0;
    */
    
    if (msgrcv(kolejkaID, komunikat, rozmiarKomunikatu, (long int) getpid(), 0) == -1){
        if (errno == EIDRM){
            printf("KLIENT:\tBLAD PRZY ODBIORZE KOMUNIKATU, KOLEJKA ZOSTALA SKASOWANA\n");
            exit(EXIT_FAILURE);
        }

        if (errno == EINTR){
            printf("KLIENT:\tBLAD PRZY ODBIORZE KOMUNIKATU, WZNAWIAM PROCES\n");
            return odbierzKomunikat(komunikat);
        }

        if (errno == EAGAIN){
            printf("KLIENT:\tBLAD PRZY ODBIORZE KOMUNIKATU, KOLEJKA PELNA\n");
            exit(EXIT_FAILURE);
        }

        if (errno == E2BIG) {
            printf("KLIENT:\tBLAD PODCZAS ODBIERANIA KOMUNIKATU, ROZMIAR WIADOMOSCI MOZE PRZEKRACZAC OCZEKIWANA WARTOSC\n");
            exit(EXIT_FAILURE);
        }

        printf("KLIENT:\tBLAD PRZY ODBIORZE KOMUNIKATU\n");
        exit(EXIT_FAILURE);
    }
    
}

int sprawdzWysylanie(int kolejka, int rozmiar) {
    struct msqid_ds status;
    int ctl = msgctl(kolejka, IPC_STAT, &status);

    if (ctl == -1) {
        perror("KLIENT:\tBLAD PODCZAS POBIERANIA INFORMACJI O KOLEJCE\n");
        exit(EXIT_FAILURE);
    }
    else {
        return !(rozmiar + status.__msg_cbytes > status.msg_qbytes);
    }
}

void wyslijKomunikat(char* kom){
    struct Komunikat komunikat;

    komunikat.odbiorca = 1;
    komunikat.nadawca = getpid();
    strcpy(komunikat.zawartosc, kom);

    /*
    if (msgsnd(kolejkaID, &komunikat, rozmiarKomunikatu, IPC_NOWAIT) == -1){
        if (errno == EIDRM) {
            printf("KLIENT:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU, KOLEJKA SKASOWANA\n");
            exit(EXIT_FAILURE);
        }
        else if (errno == EAGAIN) {
            printf("KLIENT:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU, KOLEJKA PELNA\n");
        }
        else if (errno == EINVAL) {
            printf("KLIENT:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU, MOZLIWE ZE ROZMIAR KOMUNIKATU PRZEKRACZA DOPUSZCZALNY LIMIT\n");
        }
        else {
            perror("KLIENT:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU\n");
            exit(EXIT_FAILURE);
        }
    }
    */

    if (sprawdzWysylanie(kolejkaID, MESSAGE_SIZE + sizeof(int) + rozmiarKomunikatu) == 0) {
        printf("KLIENT:\tBLAD PRZY WYSYLANIU, BRAK MIEJSCA W KOLEJCE\n");
        return;
    }

    if (msgsnd(kolejkaID, &komunikat, rozmiarKomunikatu, IPC_NOWAIT) == -1){
        if (errno == EIDRM){
            printf("KLIENT:\tBLAD PRZY WYSYLANIU KOMUNIKATU, KOLEJKA ZOSTALA SKASOWANA\n");
            exit(1);
        }

        if (errno == EINTR){
            printf("KLIENT:\tBLAD PRZY WYSYLANIU KOMUNIKATU, WZNAWIAM PROCES\n");
            return wyslijKomunikat(kom);
        }

        if (errno == EAGAIN){
            printf("KLIENT:\tBLAD PRZY WYSYLANIU KOMUNIKATU, KOLEJKA PELNA\n");
            return;
        }

        if (errno == EINVAL){
            printf("KLIENT:\tBLAD PRZY WYSYLANIU KOMUNIKATU, WIADOMOSC MOZE PRZEKRACZAC DOPUSZCZALNY LIMIT\n");
            exit(EXIT_FAILURE);
        }

        printf("KLIENT:\tBLAD PRZY WYSYLANIU KOMUNIKATU\n");
        exit(1);
    }
    
}

void *watekNadawcy(){
    int i;
    char* komunikat = malloc(MESSAGE_SIZE*sizeof(char));
    i = 1;

    while(1){
        printf("\nKLIENT:\tWYSYLAM KOMUNIKAT NR %d\n", i++);
        printf("KLIENT:\tWPROWADZ KOMUNIKAT\n");
        scanf("%" STR(MESSAGE_SIZE) "s", komunikat);
        //scanf("%10s", komunikat);
        printf("\n");
        wyslijKomunikat(komunikat);
        printf("KLIENT\tWYSLANO KOMUNIKAT\n\n");
    }

    free(komunikat);
    pthread_exit(0);
}

void *watekOdbiorcy(){
    int i;
    struct Komunikat komunikat;

    while(1){
        odbierzKomunikat(&komunikat);
        wyswietlKomunikat(&komunikat);
        sleep(1);
    }

    pthread_exit(0);
}

void watekUtworz(pthread_t *watekID, void *funkcjaWatek){
    int watekError;
    watekError = pthread_create(watekID, NULL, funkcjaWatek, NULL);
    if (watekError != 0){
        printf("\nKLIENT:\tBLAD PRZY TWORZENIU WATKU: %s \n", strerror(watekError));
    }
    else {
        printf("\nKLIENT:\tPOMYSLNIE UTWORZONO WATEK\n");
    }
}

void watekOczekiwanie(pthread_t watekID){
    int watekError;
    int **watekStatus;

    watekStatus = malloc(sizeof(*watekStatus));
    *watekStatus = malloc(sizeof(**watekStatus));
    watekError = pthread_join(watekID, (void**) watekStatus);

    if (watekError != 0){
        printf("\nKLIENT:\tBLAD PODCZAS DOLACZANIA WATKU: %s", strerror(watekError));
        printf("\nKLIENT:\tBLAD PODCZAS DOLACZANIA WATKU: %d", **(int**)watekStatus);
    }
    else {
        printf("\nKLIENT:\tPOMYSLNIE DOLACZONO WATEK\n");
    }

    free(*watekStatus);
    free(watekStatus);
}
