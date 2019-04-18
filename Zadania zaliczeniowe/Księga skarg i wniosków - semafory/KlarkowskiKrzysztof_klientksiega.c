#define _WITH_GETLINE
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#define MY_MSG_SIZE 64

key_t key;
int   shmid;
int   semid;
struct my_data
{
    int  typ;
    char txt[MY_MSG_SIZE];
    int rozmiar;
    char username[20];
} *ksiega;

struct sembuf operacja; /*konieczne do wykonywania operacji na semaforze*/
char   *buf = NULL;
size_t bufsize = MY_MSG_SIZE;
bool czy_uzykano_dostep=false;

void onexit(int signal)
{
    if(czy_uzykano_dostep == true) /*koniecznosc odblokowania semfora*/
    {
        operacja.sem_num = 0;
        operacja.sem_op = -1;
        operacja.sem_flg = 0;
        printf("\n[Klient]: konczenie pracy programu...\n");
        printf(" (odlaczenie pamieci: %s, odblokowywanie semafora: %s.)\n",
               (shmdt(ksiega) == 0) ?"OK":"blad przy odlaczaniu pamieci dzielonej",
               (semop(semid, &operacja, 1) == -1) ?"blad odblokowywnia semafora":"odblokowano semafor");
        printf("\nDziekujemy za skorzystanie z ksiegi skarg i wnioskow.\n");
    }
    else /*brak koniecznosci odblokowania semfora*/
    {
        printf("\n[Klient]: konczenie pracy programu...\n");
        printf(" (odlaczenie pamieci: %s)\n", (shmdt(ksiega) == 0) ?"OK":"blad przy odlaczaniu pamieci dzielonej");
        printf("\nDziekujemy za skorzystanie z ksiegi skarg i wnioskow.\n");
    }
exit(0);
}

int main(int argc, char * argv[])
{
    int i;

    /*obsluga sygnalow*/
    signal(SIGINT, onexit);

    /*komunikat poczatkowy*/
    printf("\nWitamy w kliencie Ksiegi skarg i wnioskow.\n");

    /*tworzenie klucza*/
	key = ftok(argv[1], 1);
    if(key == -1)
    {
        printf("[Klient]: nie udalo sie utworzyc klucza, blad ftok.\n");
        exit(0);
    }


    /*podlaczenie segmentu pamieci dzielonej*/
	if( (shmid = shmget(key, 0, 0)) == -1 )
	{
		printf("[Klient]: blad shmget\n");
		exit(0);
	}
	ksiega = (struct my_data *) shmat(shmid, (void *)0, 0);
	if(ksiega == (struct my_data *)-1)
	{
		printf("[Klient]: blad shmat!\n");
		exit(0);
	}

	/*dostep do semaforow */
	if((semid = semget(key, 1, 0)) == -1)
	{
		printf("[Klient]: blad uzyskiwania dostepu do semaforow.\n");
        fprintf(stderr, "Value of errno: %d\n", errno);

	}

    /* sprawdzanie czy semafor umozliwia obecnie zapisywanie, jesli nie czekamy */
   while(1)
   {
       if (semctl(semid, 0, GETVAL) == -1) /*blad semctl*/
       {
           printf("[Klient]: blad semctl - sprawdzaniu wartosci semafora.\n");
           exit(0);
       }
       else if(semctl(semid, 0, GETVAL) == 0)/*można zapisywać*/
       {
           /*blokowanie mozliwosci zapisu innym klientom*/
           czy_uzykano_dostep=true;
           operacja.sem_num = 0;
           operacja.sem_op = 1; /*operacja blokujaca*/
           operacja.sem_flg = SEM_UNDO;

           if (semop(semid, &operacja, 1) == -1)
           {
               fprintf(stderr, "[Klient]: blad blokowania semafora\\n");
               exit(1);
           }
           break;
       }
       else
       {
           printf("\nSerwer jest zajety, prosze czekac na mozliwosc dokonania wpisu do ksiegi...\n(Jesli nie chcesz czekac na zwolnienie serwera i zakonczyc dzialanie programu bez dokonywania wpisu nacisnij Ctrl^C)\n");
           sleep(10);
       }
   }

    /* sprawdzanie czy ksiega ma wolne sloty */
    int pierw_wolny_slot= -1;
    for(i=0; i<ksiega->rozmiar; i++)
    {
        if(ksiega[i].typ != 1)
        {
            pierw_wolny_slot = i;
            printf("[Klient]: Ilosc wolnych rekordow w ksiedze: %d (na %d).\n", (ksiega[i].rozmiar - i), ksiega->rozmiar);
            break;
        }
    }

	/*pobranie wiadomosci od uzytkownika i  wpisywanie do pierwszego wolnego slota lub komunikat o braku wolnych slotow*/
	if(pierw_wolny_slot>-1)
	{
	    do
        {
            printf("[Klient]: podaj komunikat ktory chcesz wpisac do ksiegi skarg i wnioskow:\n");
            getline(&buf, &bufsize, stdin);
        }
	    while(strlen(buf) <= 1);

        ksiega[pierw_wolny_slot].typ = 1; /*zmiana typu na jeden oznacza ze ten slot jest zajety*/
        buf[strlen(buf) - 1] = '\0'; /* usuniecie konca linii*/
        strcpy(ksiega[pierw_wolny_slot].txt, buf);
        strcpy(ksiega[pierw_wolny_slot].username, getenv("USER"));
        printf("[Klient]: Dziekuje za dokonanie wpisu do ksiegi.\n");
    }
	else
    {
	    printf("[Klient]: Niestety w ksiedze brakuje wolnych rekordow. Sprobuj ponownie pozniej.\n");
    }

	/*wyslanie sygnalu aby wywolac obsluge sygnalu i zakonczyc program - odblokowanie semafora i odlaczenie pamieci*/
    kill(getpid(),SIGINT);

	return 0;
}