#define _DEFAULT_SOURCE
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include "queue.h"
/*
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
//total chairs
long int N;
//If hairdresser is busy (1) or not (0)
sem_t hairdressersChairTaken;
//If waiting room queue is empty(1) or not (0)
sem_t queueEmpty;
//total clients count
long int totalClientsCount;
// blocks clientsResignationCount value
pthread_mutex_t readingResigned = PTHREAD_MUTEX_INITIALIZER;
//how many clients resigned
int clientsResignationCount = 0;

pthread_mutex_t hairdressersChair = PTHREAD_MUTEX_INITIALIZER;
//currently cut client
int currentlyCutClientId=-1;
//blocks current_queue_size value
pthread_mutex_t readingQueue = PTHREAD_MUTEX_INITIALIZER;
//clients queue: waiting in waiting room
struct Queue waitingRoom;
//clients

//pthread_mutex_t readingQueue, readingResigned , hairdressersChair
void show_message()
{
    printf("Res:%d WRomm: %d/%ld [in: %d]",
    clientsResignationCount,
    current_queue_size(&waitingRoom),
    N,
    currentlyCutClientId);
}
void waiting(int sec)
{
    int zzz = (((rand()%sec)+1)*1000000);
    usleep(zzz);
}
// TO DO: rename to client
void* haircut(void* arg)
{
  int id;
  id = *((int *) arg);
  pthread_mutex_lock(&hairdressersChair);
  hairdressersChairTaken = 1;
  currentlyCutClientId=id;
  if(debug)
    printf("Haircutting... (client id: %d)\n",id);
  waiting(5);
  hairdressersChairTaken = 0;
  pthread_mutex_lock(&readingQueue);
  pop(&waitingRoom);
  if(debug)
    printf("Leaving hairdresser... (client id: %d)\n",id );
  pthread_mutex_unlock(&readingQueue);
  pthread_mutex_unlock(&hairdressersChair);
  return 0;
}

void* barberCut(struct Client* client)
{
  int cuttingTime = 1;
  //cuttingTime = rand();
  pthread_mutex_lock(&hairdressersChair);
  printf("Haircutting... (in: %d)\n", client->id);
  sleep(cuttingTime);
  pthread_mutex_unlock(&hairdressersChair);
  return 0;
}

//Parameters: total chairs, total client, optional: -debug
int main(int argc, char* argv[])
{
    //Invalid parameter count - end program
    if(argc < 3 || argc > 4)
    {
        fprintf(stderr, "Invalid parameter count\n");
        return 1;
    }
    //Enable debug
    if(argc == 4 && strcmp(argv[3],"-debug")==0)
    {
        debug = 1;
    }
    if(debug)
        printf("Debug mode enabled\n");
    //Set all chairs count
    char* endptr;
    N = strtol(argv[1], &endptr,10);
    if(*endptr != '\0' || N < 0 || N == LONG_MIN || N == LONG_MAX)
    {
      fprintf(stderr, "Invalid total chairs count\n");
      return 2;
    }
    //Set totalClientsCount
    char* endptr2;
    totalClientsCount = strtol(argv[2], &endptr2, 10);
    if(*endptr2 != '\0' ||  totalClientsCount< 0 ||  totalClientsCount== LONG_MIN ||  totalClientsCount== LONG_MAX)
    {
      fprintf(stderr, "Invalid total clients count\n");
      return 3;
    }
    if(debug)
        printf("Semaphores initialization\n");
    sem_init(&queueEmpty,0,1);
    sem_init(&hairdressersChairTaken,0,0);
    if(debug)
        printf("(debug) N chairs = %ld\n",N);
    if(debug)
        printf("Initialize queue\n");
    init(&waitingRoom);
    // for(int i=0;i<totalClientsCount;i++)
    // {
    //     push(&waitingRoom,i);
    // }
    //int x;
    //Create threads
    pthread_t* clientID;
    clientID = (pthread_t*)malloc(sizeof(pthread_t)*totalClientsCount);
    int* threadID = (int*)malloc(sizeof(int)*totalClientsCount);
    srand((unsigned)time(NULL));
    for(int j=0;j<totalClientsCount;j++)
    {
        //clients show up to the hairdresser
        waiting(3);
        threadID[j]=j;
        //checking if queue is full
        pthread_mutex_lock(&readingQueue);
        if(current_queue_size(&waitingRoom)<N)
        {
            push(&waitingRoom,j);
            pthread_mutex_unlock(&readingQueue);
            pthread_create (&clientID[j], NULL, &haircut, (void*)&threadID[j]);
        }
        else
        {
            if(debug)
                printf("Queue is full. (client id: %d) resigned \n",j);
            pthread_mutex_lock(&readingResigned);
            clientsResignationCount++;
            pthread_mutex_unlock(&readingResigned);
            pthread_mutex_unlock(&readingQueue);
        }

    }
    for(int j=0;j<totalClientsCount;j++)
    {
      pthread_join(clientID[j], NULL);
    }
    exit(EXIT_SUCCESS);
}
