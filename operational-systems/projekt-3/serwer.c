#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
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
void sigintHandler(int sygnal);

void utworzKolejkeKomunikatow();
void usunKolejkeKomunikatow();

void wyswietlKomunikat(struct Komunikat* komunikat);
int odbierzKomunikat(struct Komunikat* komunikat);
void wyslijKomunikat(struct Komunikat * komunikat);

int main() {
    char c;
    struct Komunikat komunikat;

    funkcjaInicjalizacyjna();
    utworzKolejkeKomunikatow();

    while(1) {
        printf("SERWER:\tOCZEKIWANIE NA KOMUNIKAT...\n");
        
        if (odbierzKomunikat(&komunikat) == 1) {
            continue;
        }
        printf("SERWER:\tODEBRANO KOMUNIKAT\n\n");

        wyswietlKomunikat(&komunikat);

        sleep(20);
        printf("SERWER:\tWYSYLANIE ODPOWIEDZI...\n");
        wyslijKomunikat(&komunikat);
        printf("SERWER:\tWYSLANO ODPOWIEDZ\n\n\n");
    }

    printf("WPROWADZ DOWOLNY PRZYCISK ABY ZAKONCZYC...\n");
    scanf(" %c", &c);
    usunKolejkeKomunikatow();

    return 0;
}

void funkcjaInicjalizacyjna() {
    signal(SIGINT, sigintHandler);
    rozmiarKomunikatu = MESSAGE_SIZE * sizeof(char) + sizeof(long int);
}

void sigintHandler(int sygnal) {
    printf("\nSERWER:\tUZYTO CTRL+C - PRZERYWAM DZIALANIE SERWERA\n");
    usunKolejkeKomunikatow();
    exit(1);
}

void utworzKolejkeKomunikatow() {
    kolejkaID = msgget(klucz, 0600 | IPC_CREAT | IPC_EXCL);

    if (kolejkaID == -1) {
        printf("SERWER:\tBLAD PODCZAS TWORZENIA KOLEJKI - MOZE JUZ ISTNIEC KOLEJKA O KLUCZU: %d\n", klucz);
        exit(1);
    }

    printf("SERWER:\tUTWORZONO KOLEJKE O ID: %d\n", kolejkaID);
}

void usunKolejkeKomunikatow() {
    if (msgctl(kolejkaID, IPC_RMID, NULL) == -1) {
        printf("SERWER:\tBLAD PODCZAS USUWANIA KOLEJKI O ID: %d\n", kolejkaID);
        exit(1);
    }

    printf("SERWER:\tUSUNIETO KOLEJKE O ID: %d\n", kolejkaID);
}

void wyswietlKomunikat(struct Komunikat* komunikat) {
    int i;

    printf("\tNADAWCA:\t%ld\n", komunikat->nadawca);
    printf("\tODBIORCA:\t%ld\n", komunikat->odbiorca);
    printf("\tTRESC:\t");

    for (i=0; i<MESSAGE_SIZE; i++) {
        if (komunikat->zawartosc[i] == '\0') {
            break;
        }
        printf("%c", komunikat->zawartosc[i]);
    }

    printf("\n\n");
}

int odbierzKomunikat(struct Komunikat* komunikat) {
    /*
    if (msgrcv(kolejkaID, komunikat, rozmiarKomunikatu, 1, 0) == -1) {
        if (errno == EINTR) {
            return odbierzKomunikat(komunikat);
        }
        else if (errno == EIDRM) {
            printf("SERWER:\tBLAD PODCZAS ODBIERANIA KOMUNIKATU, KOLEJKA SKASOWANA\n");
            exit(EXIT_FAILURE);
        }
        else if (errno == E2BIG) {
            printf("SERWER:\tBLAD PODCZAS ODBIERANIA KOMUNIKATU, ROZMIAR KOMUNIKATU PRZEKRACZA OCZEKIWANA WARTOSC\n");
            exit(EXIT_FAILURE);
        }
        else {
            perror("SERWER:\tBLAD PODCZAS ODBIERANIA KOMUNUKATU\n");
            exit(EXIT_FAILURE);
        }
        return 1;
    }
    */
    
    if (msgrcv(kolejkaID, komunikat, rozmiarKomunikatu, 1, 0) == -1) {
        if (errno == EIDRM) {
            printf("SERWER:\tBLAD PODCZAS ODBIERANIA KOMUNIKATU, KOLEJKA SKASOWANA\n");
            exit(1);
        }
        if (errno == EINTR) {
            printf("SERWER:\tBLAD PODCZAS ODBIERANIA KOMUNIKATU, WZNOWIENIE PROCESU\n");
            return odbierzKomunikat(komunikat);
        }

        if (errno == EAGAIN) {
            printf("SERWER:\tBLAD PODCZAS ODBIERANIA KOMUNIKATU, KOLEJKA PELNA\n");
            return 1;
        }

        if (errno == E2BIG) {
            printf("SERWER:\tBLAD PODCZAS ODBIERANIA KOMUNIKATU, ROZMIAR WIADOMOSCI MOZE PRZEKRACZAC OCZEKIWANA WARTOSC\n");
            exit(EXIT_FAILURE);
        }

        printf ("SERWER:\tBLAD PODCZAS ODBIERANIA KOMUNIKATU\n");
        return 1;
    }

    return 0;
}

void wyslijKomunikat(struct Komunikat* komunikat) {
    int i;

    komunikat->odbiorca = komunikat->nadawca;
    komunikat->nadawca = 1;

    for (i=0; i<strlen(komunikat->zawartosc); i++) {
        komunikat->zawartosc[i] = toupper(komunikat->zawartosc[i]);
    }

    /*
    if (msgsnd(kolejkaID, komunikat, rozmiarKomunikatu, IPC_NOWAIT) == -1) {
        if (errno == EIDRM) {
            printf("SERWER:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU, KOLEJKA SKASOWANA\n");
            exit(EXIT_FAILURE);
        }
        else if (errno == EAGAIN) {
            printf("SERWER:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU, KOLEJKA PELNA\n");
        }
        else if (errno == EINVAL) {
            printf("SERWER:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU, MOZLIWE ZE ROZMIAR KOMUNIKATU PRZEKRACZA DOPUSZCZALNY LIMIT\n");
        }
        else {
            perror("SERWER:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU\n");
            exit(EXIT_FAILURE);
        }
    }
    */

    
    if (msgsnd(kolejkaID, komunikat, rozmiarKomunikatu, IPC_NOWAIT) == -1) {
        if (errno == EIDRM) {
            printf("SERWER:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU, KOLEJKA SKASOWANA\n");
            exit(EXIT_FAILURE);
        }

        if (errno == EINTR) {
            printf("SERWER:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU, WZNAWIAM PROCES\n");
            komunikat->nadawca = komunikat->odbiorca;
            return wyslijKomunikat(komunikat);
        }

        if (errno == EAGAIN) {
            printf("SERWER:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU, KOLEJKA PELNA\n");
            return;
        }

        if (errno == EINVAL) {
            printf("SERWER:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU, WIADOMOSC MOZE PRZEKRACZAC DOPUSZCZALNY LIMIT\n");
            exit(EXIT_FAILURE);
        }

        printf("SERWER:\tBLAD PODCZAS WYSYLANIA KOMUNIKATU\n");
        exit(1);
    }
    
}