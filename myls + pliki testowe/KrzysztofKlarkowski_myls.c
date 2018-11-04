#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

typedef struct Pliki
{
    /*do obu trybów*/
    char uprawnienia[10];
    long long rozmiar;
    char nazwa[255];

    /*do pierwszego trybu*/
    char typ;
    int liczba_dowiazan;
    char wlasciciel[32];
    char grupa[31];
    int data_m_dzien;
    char data_m_miesiac[3];
    int data_m_minuta;
    int data_m_godzina;

    /*do drugiego trybu*/
    char typ_slownie[20];
    char sciezka[255];
    char sciezka_l[255]; /*sciezka do pliku na ktory wskazuje link*/
    char ost_uzywany[100];
    char ost_modyfikowany[100];
    char ost_zm_stan[100];
}Plik;

int main(int argc, char** argv)
{
    if(argc>2) /*podano dwa lub wiecej argumentow przy uruchamianiu programu*/
    {
        fprintf(stderr,"Podano zbyt wiele argumentow, podaj jeden argument bedacy nazwa pliku\n");
        return 0;
    }
    if(argc==1) /*nie podano parametru => program dziala w trybie pierwszym*/
    {
        DIR* folder_stream_pointer;
        struct dirent* folder_struktura_pointer;
        folder_stream_pointer = opendir("."); /*otwieramy directory stream (zwraca wskaznik na pierwsze entry w folderze (jak parametr kest "." bo chcemy otworzyc folder o biezacej sciezce*/

        while(1)
        {
            Plik biezacy_plik;
            folder_struktura_pointer = readdir(folder_stream_pointer);
            if(folder_struktura_pointer == NULL) break;

            /*pobieranie nazwy pliku*/
            strcpy(biezacy_plik.nazwa,folder_struktura_pointer->d_name); /*funkcja pozwalajaca na przypisanie nazwy do zmiennej w strukturze (zwykle przypisanie nie dziala dla tablicy charow)*/

            /*pobieranie informacji o plikach*/
            struct stat  plik_info;
            lstat(biezacy_plik.nazwa, &plik_info);

                /*pobranie typu*/
                if((plik_info.st_mode & S_IFMT)==S_IFDIR)
                {
                    biezacy_plik.typ='d';
                }
                else if((plik_info.st_mode & S_IFMT)==S_IFLNK)
                {
                    biezacy_plik.typ='l';
                    /*gdy jest to link symboliczny wyswietlana nazwa musi zawierac rowniez nazwe plik na ktory link wskazuje*/
                    char pom[255];
                    int spr = readlink(biezacy_plik.nazwa, pom,255); /*funkcja readlink zwraca rozmiar lancucha lub -1 jesli nie udalo sie pobrac sciezki*/
                    if(spr == -1)
                    {
                        fprintf(stderr,"nie udalo sie pobrac sciezki\n");
                    }
                    else
                    {
                        pom[spr]='\0'; /*funkcja readlink nie umieszcza \0 na koncu napisu, nalezy zrobi to recznie*/
                        sprintf(biezacy_plik.nazwa,"%s -> %s",biezacy_plik.nazwa, pom);
                    }
                }
                else if((plik_info.st_mode & S_IFMT)==S_IFBLK)
                {
                    biezacy_plik.typ='b';
                }
                else if((plik_info.st_mode & S_IFMT)==S_IFSOCK)
                {
                    biezacy_plik.typ='s';
                }
                else if((plik_info.st_mode & S_IFMT)==S_IFIFO)
                {
                    biezacy_plik.typ='p';
                }
                else if((plik_info.st_mode & S_IFMT)==S_IFCHR)
                {
                    biezacy_plik.typ='c';
                }
                else
                {
                    biezacy_plik.typ='-';
                }

                /*pobranie uprawnien*/
                biezacy_plik.uprawnienia[0] = (plik_info.st_mode & S_IRUSR) ? 'r' : '-'; /*sprawdza czy wlasciciel ma prawa r*/
                biezacy_plik.uprawnienia[1] = (plik_info.st_mode & S_IWUSR) ? 'w' : '-'; /*sprawdza czy wlasciciel ma prawa w*/
                biezacy_plik.uprawnienia[2] = (plik_info.st_mode & S_IXUSR) ? 'x' : '-'; /*sprawdza czy wlasciciel ma prawa x*/
                biezacy_plik.uprawnienia[3] = (plik_info.st_mode & S_IRGRP) ? 'r' : '-'; /*sprawdza czy grupa ma prawa r*/
                biezacy_plik.uprawnienia[4] = (plik_info.st_mode & S_IWGRP) ? 'w' : '-'; /*sprawdza czy grupa ma prawa w*/
                biezacy_plik.uprawnienia[5] = (plik_info.st_mode & S_IXGRP) ? 'x' : '-'; /*sprawdza czy grupa ma prawa x*/
                biezacy_plik.uprawnienia[6] = (plik_info.st_mode & S_IROTH) ? 'r' : '-'; /*sprawdza czy inni maja prawa r*/
                biezacy_plik.uprawnienia[7] = (plik_info.st_mode & S_IWOTH) ? 'w' : '-'; /*sprawdza czy inni maja prawa w*/
                biezacy_plik.uprawnienia[8] = (plik_info.st_mode & S_IXOTH) ? 'x' : '-'; /*sprawdza czy inni maja prawa x*/
                biezacy_plik.uprawnienia[9] = '\0'; /*znak konca napisu*/

                /*pobranie liczby dowiazan*/
                biezacy_plik.liczba_dowiazan=(int) plik_info.st_nlink;

                /*pobranie nazwy wlasciciela*/
                if(getpwuid(plik_info.st_uid)!=NULL)
                {
                    struct passwd* pw = getpwuid(plik_info.st_uid); /*struktura passwd dla uid wlasciciela biezacego pliku ze struktury stat*/
                    strcpy(biezacy_plik.wlasciciel,pw->pw_name);
                }
                else
                {
                    sprintf(biezacy_plik.nazwa,"%d",plik_info.st_uid);
                }
                /*pobranie nazwy grupy do ktorej nalezy wlasciciel*/
                if(getgrgid(plik_info.st_gid)!=NULL)
                {
                    struct group* gr = getgrgid(plik_info.st_gid);
                    strcpy(biezacy_plik.grupa,gr->gr_name);
                }
                else
                {
                    sprintf(biezacy_plik.grupa,"%d",plik_info.st_gid);
                }
                /*pobranie rozmiaru*/
                biezacy_plik.rozmiar=(long long) plik_info.st_size;

                /*pobranie daty*/
                struct tm* czas_s; /*struktura tm zawierajaca czas kalendarzowy rozbity na poszczegolne komponenty*/
                czas_s=localtime(&(plik_info.st_mtime));
                    /*wziecie pod uwage miesiaca*/
                    if(czas_s!=NULL)
                    {
                        biezacy_plik.data_m_dzien=czas_s->tm_mday;
                        biezacy_plik.data_m_godzina=czas_s->tm_hour;
                        biezacy_plik.data_m_minuta=czas_s->tm_min;
                        switch(czas_s->tm_mon)
                        {
                            case 0:
                                strcpy(biezacy_plik.data_m_miesiac,"sty");
                            break;
                            case 1:
                                strcpy(biezacy_plik.data_m_miesiac,"lut");
                            break;
                            case 2:
                                strcpy(biezacy_plik.data_m_miesiac,"mar");
                            break;
                            case 3:
                                strcpy(biezacy_plik.data_m_miesiac,"kwi");
                            break;
                            case 4:
                                strcpy(biezacy_plik.data_m_miesiac,"maj");
                            break;
                            case 5:
                                strcpy(biezacy_plik.data_m_miesiac,"cze");
                            break;
                            case 6:
                                strcpy(biezacy_plik.data_m_miesiac,"lip");
                            break;
                            case 7:
                                strcpy(biezacy_plik.data_m_miesiac,"sie");
                            break;
                            case 8:
                                strcpy(biezacy_plik.data_m_miesiac,"wrz");
                            break;
                            case 9:
                                strcpy(biezacy_plik.data_m_miesiac,"paz");
                            break;
                            case 10:
                                strcpy(biezacy_plik.data_m_miesiac,"lis");
                            break;
                            case 11:
                                strcpy(biezacy_plik.data_m_miesiac,"gru");
                            break;
                        }
                    }
                    else
                    {
                        fprintf(stderr,"Nie udalo sie wywolac funkcji localtime");
                    }
            /*wypisywanie*/
            printf("%c%s %5d %32s %21s %10lld %2d %s %02d:%02d %s\n",biezacy_plik.typ, biezacy_plik.uprawnienia, biezacy_plik.liczba_dowiazan, biezacy_plik.wlasciciel, biezacy_plik.grupa, biezacy_plik.rozmiar, biezacy_plik.data_m_dzien, biezacy_plik.data_m_miesiac, biezacy_plik.data_m_godzina, biezacy_plik.data_m_minuta, biezacy_plik.nazwa);
        }
        /*zamkniecie directory stream*/
        closedir(folder_stream_pointer);
	}
	else /*podano parametr, program dziala w trybie drugim*/
    {
        Plik podany_plik;
        strcpy(podany_plik.nazwa,argv[1]); /*przypisanie nazwy na podstawie podanego argumentu*/
        printf("Informacje o %s:\n", podany_plik.nazwa);
        struct stat plik_info;
        lstat(podany_plik.nazwa, &plik_info);
                /*wypisanie typu*/
                if((plik_info.st_mode & S_IFMT)==S_IFDIR)
                {
                    sprintf(podany_plik.typ_slownie,"katalog");
                }
                else if((plik_info.st_mode & S_IFMT)==S_IFLNK)
                {
                    sprintf(podany_plik.typ_slownie,"link symboliczny");
                    podany_plik.typ='l';
                }
                else if((plik_info.st_mode & S_IFMT)==S_IFBLK)
                {
                   sprintf(podany_plik.typ_slownie,"urzadzenie blokowe");
                }
                else if((plik_info.st_mode & S_IFMT)==S_IFSOCK)
                {
                    sprintf(podany_plik.typ_slownie,"gniazdo");
                }
                else if((plik_info.st_mode & S_IFMT)==S_IFIFO)
                {
                    sprintf(podany_plik.typ_slownie,"FIFO");
                }
                else if((plik_info.st_mode & S_IFMT)==S_IFCHR)
                {
                    sprintf(podany_plik.typ_slownie,"urzadzenie znakowe");
                }
                else
                {
                    sprintf(podany_plik.typ_slownie,"zwykly plik");
                    podany_plik.typ='-';
                }
                printf("Typ pliku: %s\n",podany_plik.typ_slownie);

                /*wypisanie sciezki*/

                    /*jesli plik jest linkiem symbolicznym*/
                    if(podany_plik.typ=='l')
                    {
                        /*wypisanie sciezki linku*/
                        if (getcwd(podany_plik.sciezka, sizeof(podany_plik.sciezka)) == NULL)
                        {
                            fprintf(stderr,"nie udalo sie pobrac biezacego katalogu\n");
                        }
                        else
                        {
                             printf("Sciezka: %s/%s\n",podany_plik.sciezka, podany_plik.nazwa);
                        }
                        /*wypisanie na jaki plik wskazuje link*/
                        char pom[255];
                        int spr = readlink(podany_plik.nazwa, pom,255); /*funkcja readlink zwraca rozmiar lancucha lub -1 jesli nie udalo sie pobrac sciezki*/
                        if(spr == -1)
                        {
                            fprintf(stderr,"nie udalo sie pobrac sciezki\n");
                        }
                        else
                        {
                            pom[spr]='\0'; /*funkcja readlink nie umieszcza \0 na koncu napisu, nalezy zrobi to recznie*/
                            char *res2 = realpath(pom, podany_plik.sciezka_l); /*dzieki funkcji readlink otrzymalismy nazwe pliku na ktory wskazuje link, trzeba jeszcze znalezc jego pelna sciezke*/
                            if(!res2)
                            {
                                fprintf(stderr,"nie udalo sie pobrac sciezkix\n");
                            }
                            else
                            {
                                printf("Wskazuje na: %s\n", podany_plik.sciezka_l);
                            }
                        }
                    }
                    else /*jesli nie jest linkkiem symbolicznym*/
                    {
                        char *res = realpath(podany_plik.nazwa, podany_plik.sciezka);
                        if(!res)
                        {
                            fprintf(stderr,"nie udalo sie pobrac sciezki\n");
                        }
                        printf("Sciezka: %s\n",podany_plik.sciezka);
                    }

                /*wypisanie rozmiaru*/
                podany_plik.rozmiar=(long long) plik_info.st_size;
                if(podany_plik.rozmiar%10==2 || podany_plik.rozmiar%10==3 || podany_plik.rozmiar%10==4)
                {
                    if(podany_plik.rozmiar==12 || podany_plik.rozmiar==13 || podany_plik.rozmiar==14)
                    {
                        printf("Rozmiar: %lld bajtow\n",podany_plik.rozmiar);
                    }
                    else
                    {
                        printf("Rozmiar: %lld bajty\n",podany_plik.rozmiar);
                    }
                }
                else if(podany_plik.rozmiar==1)
                {
                    printf("Rozmiar: %lld bajt\n",podany_plik.rozmiar);
                }
                else
                {
                    printf("Rozmiar: %lld bajtow\n",podany_plik.rozmiar);
                }

                /*wypisanie uprawnien*/
                podany_plik.uprawnienia[0] = (plik_info.st_mode & S_IRUSR) ? 'r' : '-'; /*sprawdza czy wlasciciel ma prawa r*/
                podany_plik.uprawnienia[1] = (plik_info.st_mode & S_IWUSR) ? 'w' : '-'; /*sprawdza czy wlasciciel ma prawa w*/
                podany_plik.uprawnienia[2] = (plik_info.st_mode & S_IXUSR) ? 'x' : '-'; /*sprawdza czy wlasciciel ma prawa x*/
                podany_plik.uprawnienia[3] = (plik_info.st_mode & S_IRGRP) ? 'r' : '-'; /*sprawdza czy grupa ma prawa r*/
                podany_plik.uprawnienia[4] = (plik_info.st_mode & S_IWGRP) ? 'w' : '-'; /*sprawdza czy grupa ma prawa w*/
                podany_plik.uprawnienia[5] = (plik_info.st_mode & S_IXGRP) ? 'x' : '-'; /*sprawdza czy grupa ma prawa x*/
                podany_plik.uprawnienia[6] = (plik_info.st_mode & S_IROTH) ? 'r' : '-'; /*sprawdza czy inni maja prawa r*/
                podany_plik.uprawnienia[7] = (plik_info.st_mode & S_IWOTH) ? 'w' : '-'; /*sprawdza czy inni maja prawa w*/
                podany_plik.uprawnienia[8] = (plik_info.st_mode & S_IXOTH) ? 'x' : '-'; /*sprawdza czy inni maja prawa x*/
                podany_plik.uprawnienia[9] = '\0'; /*znak konca napisu*/
                printf("Uprawnienia: %s\n", podany_plik.uprawnienia);

                /*wypisanie dat*/
                struct tm* czas_s; /*struktura tm zawierajaca czas kalendarzowy rozbity na poszczegolne komponenty*/
                char miesiac[12][13] = {"stycznia", "lutego", "marca", "kwietnia", "maja", "czerwca", "lipca", "sierpnia", "wrzesnia", "pazdziernika", "listopada", "grudnia"};

                    /*ostatniego uzycia*/
                    czas_s=localtime(&(plik_info.st_atime));
                    if(czas_s!=NULL)
                    {
                        sprintf(podany_plik.ost_uzywany,"%d %s %d roku o %02d:%02d:%02d", czas_s->tm_mday, miesiac[czas_s->tm_mon], (czas_s->tm_year)+1900, czas_s->tm_hour, czas_s->tm_min, czas_s->tm_sec);
                        printf("Ostatnio uzywany        %s\n", podany_plik.ost_uzywany);
                    }
                    else
                    {
                        fprintf(stderr,"Nie udalo sie wywolac funckji localtime");
                    }

                    /*ostatniej modyfikacji*/
                    czas_s=localtime(&(plik_info.st_mtime));
                    if(czas_s!=NULL)
                    {
                        sprintf(podany_plik.ost_modyfikowany,"%d %s %d roku o %02d:%02d:%02d", czas_s->tm_mday, miesiac[czas_s->tm_mon], (czas_s->tm_year)+1900, czas_s->tm_hour, czas_s->tm_min, czas_s->tm_sec);
                        printf("Ostatnio modyfikowany   %s\n", podany_plik.ost_modyfikowany);
                    }
                    else
                    {
                        fprintf(stderr,"Nie udalo sie wywolac funckji localtime");
                    }

                    /*ostatniej zmiany stanu*/
                    czas_s=localtime(&(plik_info.st_ctime));
                    if(czas_s!=NULL)
                    {
                        sprintf(podany_plik.ost_zm_stan,"%d %s %d roku o %02d:%02d:%02d", czas_s->tm_mday, miesiac[czas_s->tm_mon], (czas_s->tm_year)+1900, czas_s->tm_hour, czas_s->tm_min, czas_s->tm_sec);
                        printf("Ostatnio zmieniany stan %s\n", podany_plik.ost_zm_stan);
                    }
                    else
                    {
                        fprintf(stderr,"Nie udalo sie wywolac funckji localtime");
                    }
                /*wypisanie zawartosci pliku jesli jest on plikiem tekstowym*/
                if(podany_plik.typ=='-' && podany_plik.uprawnienia[2]=='-' && podany_plik.uprawnienia[5]=='-' && podany_plik.uprawnienia[8]=='-')
                {
                    char zawartosc[81];
                    int cat=open(podany_plik.sciezka, O_RDONLY);
                    if(cat==-1)
                    {
                        fprintf(stderr,"Nie udalo sie odczytac zawartosci pliku");
                    }
                    else
                    {
                        read(cat, zawartosc, 80);
                        zawartosc[80]='\0';
                        printf("Poczatek zawartosci:\n%s", zawartosc);
                    }
                }
    }
return 0;
}
