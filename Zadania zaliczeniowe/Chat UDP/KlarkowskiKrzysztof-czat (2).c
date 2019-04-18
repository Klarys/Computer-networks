#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h>

int pid_child;
int sockfd=0;

typedef struct
{
    char name[20];
    char text[255];
    int first_msg; /*0 - pierwsza wiadomosc*/

}my_msg;

void end_program(int sygnal)
{
    if(sockfd != 0)
        close(sockfd);
    exit(0);
}

int main(int argc, char* argv[])
{
    signal(SIGINT, end_program);

    /* warunki poczatkowe */
    if(argc<2)
    {
        printf("Nie podano adresu odbiorcy, konczenie pracy programu\n");
        return 0;
    }
    if(argc>3)
    {
        printf("Podano zbyt duzo argumentow, konczenie pracy programu\n");
	return 0;
    }

    /* deklaracje zmiennych */

    struct sockaddr_in receiver_addr, my_addr;
    int bind_result;
    int sent_bytes;
    int rcved_bytes;
    int port = 18378;
    my_msg msg_s, msg_r;
    msg_s.first_msg = 1;
    msg_r.first_msg = 1;
    struct addrinfo *host_info;
    struct sockaddr_in *tmp;


    /* ustawianie nicku */
    if(argc == 3)
    {
        strcpy(msg_s.name, argv[2]);
    }
    else
    {
        strcpy(msg_s.name, "NN");
    }

    /* ustawianie adresu odbiorcy z uzyciem getaddrinfo aby mozliwe bylo rowniez podanie adresu w postaci nazwy mnemonicznej */
    if(getaddrinfo(argv[1], "18378", NULL, &host_info) != 0)
    {
        printf("Problem z getaddrinfo\n");
        return 0;
    }
    tmp = (struct sockaddr_in*)host_info->ai_addr;
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr = tmp->sin_addr;
    receiver_addr.sin_port = htons(port);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
    {
        printf("Problem z tworzeniem gniazda\n");
        return 0;
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);

    bind_result = bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    if(bind_result == -1)
    {
        printf("Problem z bind\n");
        return 0;
    }

    /* wysylanie komunikatu nawiazania polaczenia */
    msg_s.first_msg = 0;
    sent_bytes = sendto(sockfd, &msg_s, sizeof(msg_s), 0, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr));
    if(sent_bytes< 0)
    {
        printf("Problem z wyslaniem komunikatu dolaczenia do czatu.");
        close(sockfd);

    }
    msg_s.first_msg = 1;

    /*fork - potomek odbiera, a rodzic wysyla wiadomosci */
    if((pid_child = fork()) == -1)
    {
        printf("Blad fork\n");
        close(sockfd);
        return 0;
    }
    else if(pid_child != 0) /* potomek - nasluchuje wiadomosci od drugiej osoby */
    {
        printf("Rozpoczynam chat z %s. Napisz <koniec> aby zakonczyc chat.\n", inet_ntoa(receiver_addr.sin_addr));
        while(1)
        {
           /*rcved_bytes = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *) &receiver_addr, &receiver_struct_size);*/
            rcved_bytes = recvfrom(sockfd, &msg_r, sizeof(msg_r), 0, 0, 0);
            if (rcved_bytes == -1)
            {
                printf("Problem z pobraniem wiadomosci od drugiego uczestnika chatu");
                close(sockfd);
                exit(0);
            }
            if(msg_r.first_msg == 0) /* komunikat dolaczenia drugiej osoby do chatu do chatu */
            {
                printf("[%s (%s) dolaczyl do chatu]\n", msg_r.name, inet_ntoa(receiver_addr.sin_addr));
            }
            else if(strcmp(msg_r.text, "<koniec>") == 0) /* druga osoba opuszcza chat*/
            {
                printf("[%s (%s) zakonczyl rozmowe]\n", msg_r.name, inet_ntoa(receiver_addr.sin_addr));
            }
            else
            {
                printf("\n[%s (%s) ]> %s\n", msg_r.name, inet_ntoa(receiver_addr.sin_addr), msg_r.text);
            }
            printf("[%s]> ", msg_s.name);
            fflush(stdout);
        }
    }
    else  /* rodzic - pisanie i wysylanie wiadomosci */
    {
        while(1)
        {
            printf("[%s]> ", msg_s.name);
            fgets(msg_s.text, 255, stdin);
            msg_s.text[strlen(msg_s.text)-1] = '\0';
            if(strcmp(msg_s.text, "<koniec>") == 0) /* uzytkownik zakonczyl dzialanie swojego klienta */
            {
                sent_bytes = sendto(sockfd, &msg_s, sizeof(msg_s), 0 ,(struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
                if(sent_bytes == -1)
                {
                    printf("Problem z wysylaniem komuniaktu zakonczenia rozmowy");
                    close(sockfd);
                    return 0;
                }
                kill(pid_child, SIGINT); /*wyslanie SIGINT do potomka*/
                close(sockfd);
                return 0;
            }

            sent_bytes = sendto(sockfd, &msg_s, sizeof(msg_s), 0 ,(struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
            if(sent_bytes == -1)
            {
                printf("Problem z wysylaniem");
                close(sockfd);
                return 0;
            }
        }
    }
return 0;
}





/*
int msg_end(char * text)
{
    return (
            text[0] == '<' &&
            text[1] == 'k' &&
            text[2] == 'o' &&
            text[3] == 'n' &&
            text[4] == 'i' &&
            text[5] == 'e' &&
            text[6] == 'c' &&
            text[7] == '>' &&
            text[8] == '\n'
    );
}
*/