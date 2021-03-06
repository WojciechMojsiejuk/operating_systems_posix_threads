#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
/*
TO DO:
ZAPYTAĆ JAK MAJĄ BYĆ TWORZENI KLIENCI

Salon fryzjerski składa się z gabinetu z jednym
fotelem (zasób wymagający synchronizacji)
oraz z poczekalni zawierającej n krzeseł.
W danym momencie w gabinecie może być strzyżony
tylko jeden klient (wątek), reszta czeka na
wolnych krzesłach w poczekalni. Fryzjer po skończeniu
strzyżenia prosi do gabinetu kolejnego klienta,
lub ucina sobie drzemkę, jeśli poczekalnia jest pusta.
Nowy klient budzi fryzjera jeśli ten śpi, lub siada
na wolne miejsce w poczekalni jeśli fryzjer jest zajęty.
Jeśli poczekalnia jest pełna, to klient nie
wchodzi do niej i rezygnuje z wizyty.

Napisz program koordynujący pracę gabinetu.
Zsynchronizuj wątki klientów i fryzjera:

bez wykorzystania zmiennych warunkowych
(tylko mutexy/semafory) [17 p]
wykorzystując zmienne warunkowe
(condition variables) [17 p]
Aby móc obserwować działanie programu,
każdemu klientowi (klientowi) przydziel numer.
Program powinien wypisywać komunikaty według poniższego przykładu:

Res:2 WRomm: 5/10 [in: 4]
Oznacza to, że dwóch klientów zrezygnowało z
powodu braku miejsc (Res), w poczekalni (WRoom)
zajętych jest 5 z 10 krzeseł, a w gabinecie obsługiwany
jest klient o numerze 4. Po uruchomieniu programu z parametrem
-debug należy wypisywać cała kolejka klientów czekających,
a także lista klientów, którzy nie dostali się do gabinetu.
Komunikat należy wypisywać w momencie zmiany którejkolwiek z tych wartości.

Uwagi dot. sprawozdania:

Rozwiązania wykorzystujące zmienne warunkowe muszą posiadać
kolejkę FIFO dla czekających wątków.
Proszę koniecznie zaznaczyć wybraną wersję projektu
tym samym oczekiwaną ilość punktów).
Zamieścić w sprawozdaniu tą część kod programu, wyróżniając
(np. pogrubioną czcionką) fragmenty korzystające z
mechanizmów synchronizacji.
Opisać konkretne przeznaczenie i sposób wykorzystania
każdego mechanizmu synchronizacji
(semafora, mutexu, zmiennej warunkowej).
Można to zrobić w formie komentarzy do kodu umieszczonych
w miejscach, gdzie używany jest któryś z tych mechanizmów).
Za każdy przypadek potencjalnego wyścigu -3p.
*/

//1 - running in debug mode
int debug = 0;
//chairs
long int N;
//waiting rooms available chairs
long int chairsAvailable;
//how many clients resigned
long int clientsResignationCount = 0;
//
pthread_mutex_t hairdressersChair = PTHREAD_MUTEX_INITIALIZER;



void* haircut(void* arg)
{
  pthread_mutex_lock(&hairdressersChair);
  long *id;
  id = (long*)arg;
  printf("Haircutting... (in: %ld)\n", id);
  usleep(250*1000);
  pthread_mutex_unlock(&hairdressersChair);
}

int main(int argc, char* argv[])
{
    //Invalid parameter cunt - end program
    if(argc < 2 || argc > 3)
    {
        fprintf(stderr, "Invalid parameter count\n");
        return 1;
    }
    //Enable debug
    if(argc == 3 && strcmp(argv[2],"-debug")==0)
    {
        debug = 1;
    }
    if(debug)
        printf("Debug mode enabled\n");
    char* endptr;
    N = strtol(argv[1], &endptr,10);
    if(*endptr != '\0' || N < 0 || N == LONG_MIN || N == LONG_MAX)
    {
      fprintf(stderr, "Invalid N parameter\n");
      return 2;
    }
    //Set chairs available quantity to N
    chairsAvailable = N;
    if(debug)
        printf("(debug) N chairs = %ld\n",N);
    pthread_t* id_array;
    id_array = (pthread_t*)malloc(sizeof(pthread_t)*N);
    for(long j=0;j<N;j++)
    {
      id_array[j]=j;
      pthread_create (&id_array[j], NULL, &haircut,(void*)&id_array[j]);
    }
    for(long j=0;j<N;j++)
    {
      pthread_join(id_array[j], NULL);
    }
    exit(EXIT_SUCCESS);
}

/*
int main(int argc, char *argv[])
{
    if(argc==3)
    {
        int N = atoi(argv[1]);
        printf("%d\n",N);
        ITER = atoi(argv[2]);
        printf("%d\n",ITER);
        pthread_t  *id_array;
        id_array = (pthread_t*)malloc(sizeof(pthread_t)*N);
        for(long j=0;j<N;j++)
        {
          pthread_create (&id_array[j], NULL, &function,(void*)j);
        }
        for(long k=0;k<N;k++)
        {
            pthread_join(id_array[k], NULL);
        }
        printf("\n\n %d \n\n", counter);
        exit(EXIT_SUCCESS);
    }
*/
