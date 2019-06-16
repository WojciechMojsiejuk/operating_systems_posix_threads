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
pthread_mutex_t accessTotalClientsCount;

//mutex attribute
pthread_mutexattr_t attribute;

pthread_mutex_t hairdressersChairTaken;
int currentlyCutId = -1;
//If there is any customer ready to be cut
sem_t customerReadyToBeCut;

//Resigned queue (1 - can access, 0 - can not access)
pthread_mutex_t accessResignedQueue;
struct Queue resignedQueue;

//Waiting queue (1 - can access, 0 - can not access)
pthread_mutex_t accessWaitingQueue;
struct Queue waitingQueue;

//Waits <1;sec> seconds (random number)
void waiting(int sec)
{
    int zzz = (((rand()%sec)+1)*1000000);
	if(ENABLE_SLEEP)
   		usleep(zzz);
}

void* Client(void* arg)
{
	int id;
	id = *((int *) arg);
	if(debug >= 2)
		printf("Created client with id: %d\n", id);
	//Error variable check
	int result;
	//Check waiting queue
	result = pthread_mutex_lock(&accessWaitingQueue);
	if(result)
	{
		fprintf(stderr, "accessWaitingQueue could not be locked\n");
		exit(EXIT_FAILURE);
	}
	//No empty chair - waiting queue full
	if(current_queue_size(&waitingQueue) >= N)
	{
		//Add client that resigned to resigned queue
		result = pthread_mutex_lock(&accessResignedQueue);
		if(result)
		{
			fprintf(stderr, "accessResignedQueue could not be locked");
			exit(EXIT_FAILURE);
		}
		push(&resignedQueue, id);

		result = pthread_mutex_lock(&accessTotalClientsCount);
		if(result)
		{
			fprintf(stderr, "accessTotalClientsCount could not be locked");
			exit(EXIT_FAILURE);
		}
		--totalClientsCount;
		pthread_mutex_unlock(&accessTotalClientsCount);
		//Something changed -> print full message here	
		result = pthread_mutex_lock(&hairdressersChairTaken);
		if(result)
		{
			fprintf(stderr, "hairdressersChairTaken could not be locked");
			exit(EXIT_FAILURE);
		}
		if(debug)
		{
			printf("Resigned: ");
			print_queue(&resignedQueue);
			printf("\n");
			printf("In queue: ");
			print_queue(&waitingQueue);
			printf("\n");
		}
		printf("Res: %d WRoom %d/%d [in: %d]\n", current_queue_size(&resignedQueue), current_queue_size(&waitingQueue), N, currentlyCutId);
		if(debug >= 2)
			printf("Resigned count: %d\n", current_queue_size(&resignedQueue));
		pthread_mutex_unlock(&accessWaitingQueue);
		pthread_mutex_unlock(&hairdressersChairTaken);
		pthread_mutex_unlock(&accessResignedQueue);
		if(debug >= 2)
			printf("Client %d resigned\n", id);
		return 0;
	}
	//Waiting queue not full - we can join
	else
	{
		if(debug >= 2)
			printf("Client joining queue, clent id: %d\n", id);
		//Something changed -> print full message here
		result = pthread_mutex_lock(&accessResignedQueue);
		if(result)
		{
			fprintf(stderr, "accessResignedQueue could not be locked");
			exit(EXIT_FAILURE);
		}
		result = pthread_mutex_lock(&hairdressersChairTaken);
		if(result)
		{
			fprintf(stderr, "hairdressersChairTaken could not be locked");
			exit(EXIT_FAILURE);
		}
		if(debug)
		{
			printf("Resigned: ");
			print_queue(&resignedQueue);
			printf("\n");
			printf("In queue: ");
			print_queue(&waitingQueue);
			printf("\n");
		}
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
	if(debug >= 2)
		printf("Created barber\n");
	//variable containing latest err code
	int result = 1;
	while(1)
	{
		//Check waiting room once
		pthread_mutex_lock(&accessWaitingQueue);

		//There is someone waiting
		if(current_queue_size(&waitingQueue) > 0)
		{
			result = pthread_mutex_lock(&hairdressersChairTaken);
			if(result)
			{
				fprintf(stderr, "hairdressersChairTaken could not be locked");
				exit(EXIT_FAILURE);
			}
			//Get first client
			currentlyCutId = front(&waitingQueue);
			pop(&waitingQueue);
			//Something changed -> print full message here
			//Cutting...
			if(debug >= 2)
				printf("Barber: cutting %d\n", currentlyCutId);
			result = pthread_mutex_lock(&accessResignedQueue);
			if(result)
			{
				fprintf(stderr, "accessResignedQueue could not be locked");
				exit(EXIT_FAILURE);
			}
			if(debug)
			{
				printf("Resigned: ");
				print_queue(&resignedQueue);
				printf("\n");
				printf("In queue: ");
				print_queue(&waitingQueue);
				printf("\n");
			}
			printf("Res: %d WRoom %d/%d [in: %d]\n", current_queue_size(&resignedQueue), current_queue_size(&waitingQueue), N, currentlyCutId);
			pthread_mutex_unlock(&accessResignedQueue);
			pthread_mutex_unlock(&hairdressersChairTaken);
			waiting(9);
			if(debug >= 2)
				printf("Barber: finished cutting %d\n", currentlyCutId);
			//Cutting done
			//Decrease totalClientsCount
			result = pthread_mutex_lock(&accessTotalClientsCount);
			if(result)
			{
				fprintf(stderr, "accessTotalClientsCount could not be locked");
				exit(EXIT_FAILURE);
			}
			--totalClientsCount;
			pthread_mutex_unlock(&accessTotalClientsCount);
			//Nobody is on chair - set currentlyCutId to -1
			result = pthread_mutex_lock(&hairdressersChairTaken);
			if(result)
			{
				fprintf(stderr, "accessTotalClientsCount could not be locked");
				exit(EXIT_FAILURE);
			}
			currentlyCutId = -1;
			pthread_mutex_unlock(&hairdressersChairTaken);
		}
		//Go to sleep
		else
		{
		    pthread_mutex_unlock(&accessWaitingQueue);
		    sem_wait(&customerReadyToBeCut);
		}
		//Can I go home
		result = pthread_mutex_lock(&accessTotalClientsCount);
		if(result)
		{
			fprintf(stderr, "accessTotalClientsCount could not be locked");
			exit(EXIT_FAILURE);
		}
		if(totalClientsCount == 0)
		{
			printf("Barber is going home\n");
			pthread_mutex_unlock(&accessTotalClientsCount);
			break;
		}
		pthread_mutex_unlock(&accessTotalClientsCount);
	}
  return 0;
}

//Parameters: total chairs, total clients, optional: -debug/-debug2
int main(int argc, char* argv[])
{
    //Invalid parameter count - end program
    if(argc < 3 || argc > 4)
    {
        fprintf(stderr, "Invalid parameter count\n");
        return 1;
    }
    //Enable debug level 1
    if(argc == 4 && strcmp(argv[3],"-debug")==0)
    {
        debug = 1;
    }
	//Or debug level 2
	else if(argc == 4 && strcmp(argv[3],"-debug2") ==0)
	{
		debug = 2;
	}
    if(debug == 1)
		printf("Debug level 1 enabled\n");
	else if(debug == 2)
		printf("Debug level 2 enabled\n");
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
	//Init attribute
	mutexErr = pthread_mutexattr_init(&attribute);
	if(mutexErr)
	{
		fprintf(stderr, "Could not init mutex attribute\n");
		return 4;
	}
	mutexErr = pthread_mutexattr_settype(&attribute, PTHREAD_MUTEX_ERRORCHECK);
	if(mutexErr)
	{
		fprintf(stderr, "Could not set mutex attribute\n");
		return 5;
	}
	//Init mutexes
	mutexErr = pthread_mutex_init(&hairdressersChairTaken, &attribute);
	if(mutexErr)
	{
		fprintf(stderr, "Could not init mutex hairdressersChairTaken\n");
		return 6;
	}
	mutexErr = pthread_mutex_init(&accessResignedQueue, &attribute);
	if(mutexErr)
	{
		fprintf(stderr, "Could not init mutex accessResignedQueue\n");
		return 7;
	}
	mutexErr = pthread_mutex_init(&accessWaitingQueue, &attribute);
	if(mutexErr)
	{
		fprintf(stderr, "Could not init mutex accessWaitingQueue\n");
		return 8;
	}
	mutexErr = pthread_mutex_init(&accessTotalClientsCount, &attribute);
	if(mutexErr)
	{
		fprintf(stderr, "Could not init mutex accessTotalClientsCount\n");
		return 9;
	}

	//Init semaphores
    if(debug >= 2)
		printf("Semaphores initialization\n");
	int errCheck = sem_init(&customerReadyToBeCut, 0, 0);
	if(errCheck != 0)
	{
		fprintf(stderr, "Semaphore customerReadyToBeCut could not init\n");
		return 10;
	}
    if(debug >= 2)
        printf("(debug) N chairs = %d\n",N);
	//Init queues
    if(debug >= 2)
        printf("Initialize queues\n");
    init(&waitingQueue);
	init(&resignedQueue);
	//Create barber thread
	pthread_t barberID;
	int barberResult = pthread_create(&barberID, NULL, &Barber, NULL);
	if(barberResult)
	{
		fprintf(stderr, "Could not create barber thread\n");
		return 11;
	}
    //Create clients threads
    pthread_t* clientID;
    clientID = (pthread_t*)malloc(sizeof(pthread_t)*totalClientsCount);
	if(clientID == NULL)
	{
		fprintf(stderr, "Could not allocate memory\n");
		return 12;
	}
    int* threadID = (int*)malloc(sizeof(int)*totalClientsCount);
	if(threadID == NULL)
	{
		fprintf(stderr, "Could not allocate memory\n");
		return 13;
	}
    srand((unsigned)time(NULL));
	int mainClientCount = totalClientsCount;
    for(int j=0;j<mainClientCount;j++)
    {
        //clients show up to the hairdresser
        waiting(3);
        threadID[j]=j;
		int errCode = pthread_create (&clientID[j], NULL, &Client, (void*)&threadID[j]);
        if(errCode!=0)
		{
			fprintf(stderr, "pthread_create returned value: %d, could not create thread number %d\n", errCode, j);
			fprintf(stderr, "Make sure you did not try to create too many threads\n");
			return 14;
		}
    }
    for(int j=0;j<mainClientCount;j++)
    {
		pthread_join(clientID[j], NULL);
    }
	pthread_join(barberID,NULL);
    exit(EXIT_SUCCESS);
}
