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

#define ENABLE_SLEEP 0
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
int N;
//total clients count
int totalClientsCount;

//If hairdresser chair is taken (1) or not (0)
//??
//error check mutex
pthread_mutexattr_t attribute;

pthread_mutex_t hairdressersChairTaken;
int currentlyCutId = -1;
sem_t customerReadyToBeCut;
//If there is any customer ready to be cut
// pthread_mutex_t customerReadyToBeCut = PTHREAD_MUTEX_INITIALIZER;
//Resigned queue (1 - can access, 0 - can not access)
pthread_mutex_t accessResignedQueue;
struct Queue resignedQueue;
//Waiting queue (1 - can access, 0 - can not access)
pthread_mutex_t accessWaitingQueue;
struct Queue waitingQueue;

/*
void show_message()
{
    printf("Res:%d WRomm: %d/%ld [in: %d]",
    clientsResignationCount,
    current_queue_size(&waitingRoom),
    N,
    currentlyCutClientId);
}
*/
//Waits <1;sec> seconds (random number)
void waiting(int sec)
{
    int zzz = (((rand()%sec)+1)*1000000);
		if(ENABLE_SLEEP)
   		usleep(zzz);
}

// TO DO: rename to client
void* Client(void* arg)
{
  int id;
  id = *((int *) arg);
	if(debug)
		printf("Created client with id: %d\n", id);
	//sprawdzamy kolejkę
	pthread_mutex_lock(&accessWaitingQueue);

	//Nie ma wolnych krzeseł - rezygnujemy
	if(current_queue_size(&waitingQueue) >= N)
	{
		pthread_mutex_unlock(&accessWaitingQueue);
		//Add client that resigned to resigned queue
		int result = pthread_mutex_lock(&accessResignedQueue);
		if(result)
		{
			fprintf(stderr, "(121)accessResignedQueue could not be locked");
			exit(EXIT_FAILURE);
		}
		push(&resignedQueue, id);
		//Something changed -> print full message here
		if(debug)
			printf("Resigned count: %d\n", current_queue_size(&resignedQueue));
		pthread_mutex_unlock(&accessResignedQueue);
		if(debug)
			printf("Client %d resigned\n", id);
		return 0;
	}
	//Kolejka jest pusta
	else if(current_queue_size(&waitingQueue) == 0)
	{
		//Jeśli barber jest wolny - siadamy do kolejki i tak

		//Barber jest zajęty - dołączamy do kolejki czekających
		//Something changed -> print full message here
		if(debug)
			printf("Queue is empty, joining client id: %d\n", id);
		push(&waitingQueue, id);
		pthread_mutex_unlock(&accessWaitingQueue);
    sem_post(&customerReadyToBeCut);
	}
	//Kolejka ma wolne miejsca - dołączamy do kolejki
	else
	{
		if(debug)
			printf("Client joining queue, clent id: %d\n", id);
		//Something changed -> print full message here
		pthread_mutex_lock(&accessResignedQueue);
		pthread_mutex_lock(&hairdressersChairTaken);
		printf("Res: %d WRoom %d/%d [in: %d]\n", current_queue_size(&resignedQueue), current_queue_size(&waitingQueue), N, currentlyCutId);
		pthread_mutex_unlock(&accessResignedQueue);
		pthread_mutex_unlock(&hairdressersChairTaken);
		push(&waitingQueue, id);
		pthread_mutex_unlock(&accessWaitingQueue);
    sem_post(&customerReadyToBeCut);
	}
  return 0;
}

void* Barber()
{
	if(debug)
		printf("Created barber\n");
	while(1)
	{
		//Sprawdz kolejke
		pthread_mutex_lock(&accessWaitingQueue);

		//Ktoś jest w kolejce
		if(current_queue_size(&waitingQueue) > 0)
		{
			pthread_mutex_lock(&hairdressersChairTaken);
			//Kolejka jest zablokowana - front() zwroci pierwszego klienta
			currentlyCutId = front(&waitingQueue);
			pop(&waitingQueue);
			//Strzyzenie
			if(debug)
				printf("Barber: cutting %d\n", currentlyCutId);
			waiting(9);
			if(debug)
				printf("Barber: finished cutting %d\n", currentlyCutId);
			pthread_mutex_unlock(&hairdressersChairTaken);
		}
        else
        {
            pthread_mutex_unlock(&accessWaitingQueue);
            sem_wait(&customerReadyToBeCut);
        }


	}
  return 0;
}

//Parameters: total chairs, total clients, optional: -debug
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
		N = atoi(argv[1]);
    if(N <= 0)
    {
      fprintf(stderr, "Invalid total chairs count\n");
      return 2;
    }
    //Set totalClientsCount
		totalClientsCount = atoi(argv[2]);
    if(totalClientsCount <= 0)
    {
      fprintf(stderr, "Invalid total clients count\n");
      return 3;
    }
		int mutexErr;
		//Init attribute (do error check)
		pthread_mutexattr_init(&attribute);
		pthread_mutexattr_settype(&attribute, PTHREAD_MUTEX_ERRORCHECK);
		//Init mutexes
		pthread_mutex_init(&hairdressersChairTaken, &attribute);
		pthread_mutex_init(&accessResignedQueue, &attribute);
		pthread_mutex_init(&accessWaitingQueue, &attribute);
		
		//Init semaphores
    if(debug)
        printf("Semaphores initialization\n");

		int errCheck = sem_init(&customerReadyToBeCut, 0, 0);
		if(errCheck != 0)
		{
			fprintf(stderr, "Semaphore customerReadyToBeCut could not init\n");
			return 4;
		}
        /*
		int errCheck2 = sem_init(&accesswaitingQueue, 0, 0);
		if(errCheck2 != 0)
		{
			fprintf(stderr, "Semaphore accesswaitingQueue could not init\n");
			return 5;
		}
    int errCheck3 = sem_init(&hairdressersChairTaken, 0, 0);
		if(errCheck3 != 0)
		{
			fprintf(stderr, "Semaphore hairdressersChairTaken could not init\n");
			return 6;
		}*/
    if(debug)
        printf("(debug) N chairs = %d\n",N);
		//Init queues
    if(debug)
        printf("Initialize queues\n");
    init(&waitingQueue);
		init(&resignedQueue);
		//Create barber thread
		pthread_t barberID;
		pthread_create(&barberID, NULL, &Barber, NULL);
    //Create clients threads
    pthread_t* clientID;
    clientID = (pthread_t*)malloc(sizeof(pthread_t)*totalClientsCount);
		if(clientID == NULL)
		{
			fprintf(stderr, "Could not allocate memory\n");
			return 9;
		}
    int* threadID = (int*)malloc(sizeof(int)*totalClientsCount);
		if(threadID == NULL)
		{
			fprintf(stderr, "Could not allocate memory\n");
			return 10;
		}
    srand((unsigned)time(NULL));
    for(int j=0;j<totalClientsCount;j++)
    {
        //clients show up to the hairdresser
        waiting(3);
        threadID[j]=j;
				int errCode = pthread_create (&clientID[j], NULL, &Client, (void*)&threadID[j]);
        if(errCode!=0)
			{
				fprintf(stderr, "pthread_create returned value: %d, could not create thread number %d\n", errCode, j);
				fprintf(stderr, "Make sure you did not try to create too many threads\n");
				return 11;
			}
    }
    for(int j=0;j<totalClientsCount;j++)
    {
      pthread_join(clientID[j], NULL);
    }
    exit(EXIT_SUCCESS);
}
