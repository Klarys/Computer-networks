#include <sys/types.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#define MY_MSG_SIZE 64

int slots; /*do przechowywania mozliwych wpisow w ksiedze*/
key_t key;
int shmid;
int semid;
struct my_data
{
	int  typ;
	char txt[MY_MSG_SIZE];
	int rozmiar;
	char username[20];
} *ksiega;

#if defined(__linux) /*dla systemów linuxowych*/
union semun{
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
}ustaw_sem;
#else /*dla innych systemów, np. FreeBSD - jest juz zadeklarowane)*/
union semun ustaw_sem;
#endif


/*obsluga sygnalu crtl^C*/
void sgnhandle_ctrc(int signal)
{
	printf("\n[Serwer]: sygnal SIGINT => konczenie pracy programu...");
	printf(" (odlaczenie pamieci: %s, usuniecie bloku pamieci: %s, usuwanie zbioru semaforow: %s.)\n",
			(shmdt(ksiega) == 0) ?"OK":"blad shmdt - odlaczania pamieci dzielonej",
			(shmctl(shmid, IPC_RMID, 0) == 0) ?"OK":"blad shmctl - usuwania pamieci dzielonej",
            (semctl(semid, 0, IPC_RMID) == 0) ?"OK":"blad semctl - usuwania zbioru semaforow"); /*przy uzyciu IPCRMID drugi argument jest pomijany*/
	exit(0);
}

/*obsluga sygnalu crtl^Z*/
void sgnhandle_ctrz(int singal)
{
    int i, j;
    int ile_zapisanych=0;
	printf("\nObecny stan ksiegi: \n");
    for(i = 0; i < slots; i++)
    {
        if(ksiega[i].typ != 0)
        {
            ile_zapisanych++;
        }
    }
    if(ile_zapisanych==0)
    {
        printf("\nKsiega skarg i wnioskow jest jeszcze pusta, nacisnij Ctrl^Z ponownie aby odswiezyc...\n");
    }
    else
    {
        for (j = 0; j < ile_zapisanych; ++j)
        {
            printf("___________  Ksiega skarg i wnioskow | wpis %d  ___________\n\n", j + 1);
            printf("Autor: %s | Tresc wpisu: %s\n\n",ksiega[j].username, ksiega[j].txt);
        }
        printf("Ilosc wolnych slotow w ksiedze: %d. (nacisnij Ctrl^Z ponownie aby odswiezyc)\n", (slots-ile_zapisanych));
    }
}

void sprzatanie(bool get, bool att, bool sget)
{
    printf("\n[Serwer]: napotkano blad, konczenie pracy programu...");
    if(att) /*jesli program podlaczyl segment pamieci dzielonej*/
    {

        printf("odlaczenie pamieci: %s",(shmdt(ksiega) == 0) ?"OK":"blad shmdt - odlaczania pamieci dzielonej\n");
    }
    if(get)/*jesli program stworzyl segment pamieci dzielonej*/
    {
        printf("usuwanie pamieci dzielonej %s",(shmctl(shmid, IPC_RMID, 0) == 0) ?"OK":"blad shmctl - usuwania pamieci dzielonej\n");
    }
    if(sget)/*jesli program stworzyl zbior semaforow*/
    {
        printf("usuwanie zbioru semaforow %s",(semctl(semid, 0, IPC_RMID) == 0) ?"OK":"blad semctl - usuwania zbioru semaforow\n");
    }

exit(0);
}

int main(int argc, char * argv[])
{
    int i;
    bool stworzono_pamiec = false;
    bool podlaczono_pamiec = false;
    bool zbior_semaforow = false;

    slots = atoi(argv[2]);
    struct shmid_ds buf;

    /*obsluga sygnalow*/
    signal(SIGINT, sgnhandle_ctrc);
    signal(SIGTSTP, sgnhandle_ctrz);

    /*komunikat poczatkowy*/
    printf("\nKsiega skarg i wnioskow - WARIANT B\n");

    /*tworzenie klucza*/
    printf("[Serwer]: tworze klucz...");
    key = ftok(argv[1], 1);
    if(key == -1)
    {
        printf("[Serwer]: nie udalo sie utworzyc klucza, blad ftok.\n");
        sprzatanie(stworzono_pamiec, podlaczono_pamiec, zbior_semaforow);
    }
    else
    {
    #if defined(__linux)
        printf(" OK (klucz: %d)\n", key); /*dla systemów linuxowych*/
    #else
        printf(" OK (klucz: %ld)\n", key);/*dla innych systemów, np. Free BSD)*/
    #endif
    }


    /*tworzenie segmentu pamieci dzielonej*/
    printf("[Serwer]: tworze segment pamieci wspolnej...");
    if ((shmid = shmget(key, sizeof(struct my_data) * slots, 0600 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        printf("Serwer]: blad shmget!\n");
        sprzatanie(stworzono_pamiec, podlaczono_pamiec, zbior_semaforow);
    }
    stworzono_pamiec=true;
    /*podlaczenie segmentu pamieci dzielonej*/
    if(shmctl(shmid, IPC_STAT, &buf) == -1)/* kopiowanie dane z jadra do struktury buf*/
    {
        printf("[Serwer]: blad przy kopiowaniu danych z jadra do struktury buf - funkcja shmctl().\n");
        exit(0);
    }
    printf(" OK (id: %d, rozmiar: %zub)\n", shmid, buf.shm_segsz);
    printf("[Serwer]: dolaczam pamiec wspolna...");
    ksiega = (struct my_data *) shmat(shmid, (void *) 0, 0);
    if (ksiega == (struct my_data *) -1)
    {
        printf("[Serwer]: blad shmat.\n");
        sprzatanie(stworzono_pamiec, podlaczono_pamiec, zbior_semaforow);
    }
    podlaczono_pamiec=true;
    printf(" OK (adres: %lX)\n", (long int) ksiega);

    /*tworzenie zbioru semaforow, obecnie jednoelementowy*/
    if ((semid = semget(key, 1, IPC_CREAT | 0700)) == -1)
    {
        printf("[Serwer]: blad przy tworzeniu semafora funkcja semget().\n");
        sprzatanie(stworzono_pamiec, podlaczono_pamiec, zbior_semaforow);
    }
    zbior_semaforow=true;
    printf("[Serwer]: Poprawnie stworzono zbior semaforow.\n");

    /*inicjalizacja - ustawianie poczatkowej wartosci semafora na 0) */

    ustaw_sem.val = 0;
    if ((semctl(semid, 0, SETVAL, ustaw_sem)) == -1)
    {
        printf("[Serwer]: blad przy ustawianiu domyslej wartosci semafora.\n");
        sprzatanie(stworzono_pamiec, podlaczono_pamiec, zbior_semaforow);
    }
    printf("[Serwer]: poprawnie zainicjalizowano semafor na wartosc domyslna: %d.\n", semctl(semid, 0, GETVAL));

    /*ustawianie typow i wartosci pomocniczej z rozmiarem w ksiedze*/
    for (i = 0; i < slots; i++)
    {
        ksiega[i].typ=0;
        ksiega[i].rozmiar=slots;
    }

    /*wypisanie komunikatu dla uzytkownika*/
	printf("\n\n[Serwer]: nacisnij Crtl^Z by wyswietlic stan ksiegi...\n");

    /*utrzymanie serwera dzialajacego*/
	while(1){}

return 0;
}