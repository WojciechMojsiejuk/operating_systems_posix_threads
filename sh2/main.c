#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "queue.h"

#define MAX_TIMED_WAIT 10
#define ENABLE_SLEEP 0

//1 - debug mode level 1
//2 - debug mode level 2
int debug = 0;
//total chairs
int totalChairs;
//total clients count
int totalClientsCount;

//Resigned queue (1 - can access, 0 - can not access)
pthread_mutex_t accessResignedQueue = PTHREAD_MUTEX_INITIALIZER;
struct Queue resignedQueue;

//Waiting queue (1 - can access, 0 - can not access)
pthread_mutex_t accessWaitingQueue = PTHREAD_MUTEX_INITIALIZER;
struct Queue waitingQueue;


pthread_mutex_t	customer_waiting_mutex	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	chairAvailable	= PTHREAD_COND_INITIALIZER;

pthread_mutex_t	customer_haircut_mutex	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	haircutDone	= PTHREAD_COND_INITIALIZER;

pthread_mutex_t	barber_mutex	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	customerReady	= PTHREAD_COND_INITIALIZER;

pthread_mutex_t	barber_napping_mutex	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	customerShowedUp	= PTHREAD_COND_INITIALIZER;

pthread_mutex_t hairdressersChairTaken;
int currentlyCutId = -1;


void* Barber(void* arg)
{
	if (debug >= 2)
		printf("Created barber\n");
	int mutexCode;
    while(1)
    {
    	//Obsłużenie jednego klienta
    	while(1)
    	{
			if (debug >= 2)
		        printf("Barber: Checking queue...!\n");
			//Sprawdz raz kolejke
			mutexCode = pthread_mutex_lock(&accessWaitingQueue);
			if(mutexCode)
			{
				fprintf(stderr, "Barber: accessWaitingQueue [1] could not be locked\n");
				exit(EXIT_FAILURE);
			}
    		//Jest ktoś
    		if(current_queue_size(&waitingQueue) > 0)
    		{

                if (debug >= 2)
                    printf("Barber: Queue not empty! Preparing chair...\n");
				pthread_mutex_unlock(&accessWaitingQueue);
                //barber przygotowuje miejsce do strzyzenia
                //TO DO: sleep
                //barber woła następnego klienta
                if (debug >= 2)
                    printf("Barber: Next client!\n");

				//Przed broadcast powinien byc lock wg mnie
                pthread_cond_broadcast(&chairAvailable);
                //barber czeka aż pojawi się klient

				mutexCode = pthread_mutex_lock(&barber_mutex);
				if(mutexCode)
				{
					fprintf(stderr, "Barber: barber_mutex [1] could not be locked\n");
					exit(EXIT_FAILURE);
				}
				struct timespec maxWait = {0, 0};
				const int getTime = clock_gettime(CLOCK_REALTIME, &maxWait);
				if (getTime)
				{
					fprintf(stderr, "Could not get system time\n");
					exit(EXIT_FAILURE);
				}
				maxWait.tv_sec += MAX_TIMED_WAIT;
				int timedWait = pthread_cond_timedwait(&customerReady, &barber_mutex, &maxWait);
				if(timedWait)
				{
					fprintf(stderr, "Barber: customerReady cond waiting too long\n");
					exit(EXIT_FAILURE);
				}
                break;
    		}
    		//Nie ma - ucinamy drzemke
    		else
    		{
				mutexCode = pthread_mutex_lock(&barber_napping_mutex);
				if(mutexCode)
				{
					fprintf(stderr, "Barber: barber_napping_mutex [1] could not be locked\n");
					exit(EXIT_FAILURE);
				}
                if (debug >= 2)
                    printf("Barber: Queue empty! Going to sleep...\n");
				pthread_mutex_unlock(&accessWaitingQueue);

				struct timespec maxWait = {0, 0};
				const int getTime = clock_gettime(CLOCK_REALTIME, &maxWait);
				if (getTime)
				{
					fprintf(stderr, "Could not get system time\n");
					exit(EXIT_FAILURE);
				}
				maxWait.tv_sec += MAX_TIMED_WAIT;
				int timedWait = pthread_cond_timedwait(&customerShowedUp, &barber_napping_mutex, &maxWait);
				if(timedWait)
				{
					fprintf(stderr, "Barber: customerShowedUp cond waiting too long\n");
					exit(EXIT_FAILURE);
				}
				if (debug >= 2)
    				printf("Barber: waking up\n");
				pthread_mutex_unlock(&barber_napping_mutex);
    		}
    	}
    mutexCode = pthread_mutex_lock(&accessWaitingQueue);
	if(mutexCode)
	{
		fprintf(stderr, "Barber: accessWaitingQueue [2] could not be locked\n");
		exit(EXIT_FAILURE);
	}
	//Fryzjer zaprosil kogos z kolejki [pop(&waitingQueue)]
	//Wypisac pelny komunikat
	if (debug >=2)
    	printf("Barber: haircutting customer, cliend id: %d\n", front(&waitingQueue));
	pthread_mutex_unlock(&hairdressersChairTaken);
	currentlyCutId = front(&waitingQueue);
	pthread_mutex_unlock(&hairdressersChairTaken);
    pop(&waitingQueue);
	if(debug)
	{
		printf("Resigned: ");
		print_queue(&resignedQueue);
		printf("\n");
		printf("In queue: ");
		print_queue(&waitingQueue);
		printf("\n");
	}
	pthread_mutex_unlock(&accessWaitingQueue);
    //barber skończył strzyc klienta
	//Skonczyl strzyc -> zmieniamy currentlyCutId na -1?
	//I wypisujemy pelny komunikat
	if (debug >= 2)
		printf("Barber: haircut done\n");
	pthread_mutex_lock(&hairdressersChairTaken);
	while(currentlyCutId!=-1)
	{
		pthread_mutex_unlock(&hairdressersChairTaken);
		pthread_cond_broadcast(&haircutDone);
	}

	pthread_mutex_unlock(&barber_mutex);
	//Przed broadcast lock imo



    }
    return 0;
}

void* Client(void* numer) {
    int id;
    id = *((int *) numer);
	if(debug >= 2)
		printf("\tStarted client thread #%d\n", id);
	//Error code return from pthread_mutex_lock
	int mutexCode;
	mutexCode = pthread_mutex_lock(&accessWaitingQueue);
	if(mutexCode)
	{
		fprintf(stderr, "Client: accessWaitingQueue [1] could not be locked\n");
		exit(EXIT_FAILURE);
	}
  	if(debug >= 2)
	  	printf("Client %d checks if chairs are available\n", id);
	//sprawdzamy kolejkę

	//Nie ma wolnych krzeseł - rezygnujemy
	if(current_queue_size(&waitingQueue) >= totalChairs)
	{

		//Add client that resigned to resigned queue
		//Something changed - we should print message here
		pthread_mutex_unlock(&accessWaitingQueue);
		pthread_mutex_lock(&accessResignedQueue);
        push(&resignedQueue, id);
		if(debug)
		{
			printf("Resigned: ");
			print_queue(&resignedQueue);
			printf("\n");
			printf("In queue: ");
			print_queue(&waitingQueue);
			printf("\n");
		}
        if(debug >= 2)
			printf("Resigned count: %d\n", current_queue_size(&resignedQueue));
		if(debug >= 2)
			printf("Client %d resigned\n", id);
		pthread_mutex_unlock(&accessResignedQueue);
    }
    else
    {
        if(debug >= 2)
            printf("Client joining queue, clent id: %d\n", id);
		//Something changed, we should print message here
        push(&waitingQueue, id);
		if(debug)
		{
			printf("Resigned: ");
			print_queue(&resignedQueue);
			printf("\n");
			printf("In queue: ");
			print_queue(&waitingQueue);
			printf("\n");
		}
        if(debug >= 2)
        	printf("Waiting clients count: %d\n", current_queue_size(&waitingQueue));
        //klient wchodzi do sklepu, uruchamia się pozytywka przy drzwiach,
        // ktora budzi fryzjera
		pthread_mutex_unlock(&accessWaitingQueue);
        pthread_cond_broadcast(&customerShowedUp);
        //Something changed -> print full message here

        //klient staje w kolejce i czeka na zawolanie
		mutexCode = pthread_mutex_lock(&customer_waiting_mutex);
		if(mutexCode)
		{
			fprintf(stderr, "Client: customer_waiting_mutex [1] could not be locked\n");
			exit(EXIT_FAILURE);
		}
		do {
			if(debug >= 2)
            	printf("\twątek #%d oczekuje na sygnał...\n", id);
			pthread_mutex_unlock(&accessWaitingQueue);
			struct timespec maxWait = {0, 0};
			const int getTime = clock_gettime(CLOCK_REALTIME, &maxWait);
			if (getTime)
			{
				fprintf(stderr, "Could not get system time\n");
				exit(EXIT_FAILURE);
			}
			maxWait.tv_sec += MAX_TIMED_WAIT;
            const int timedWait = pthread_cond_timedwait(&chairAvailable, &customer_waiting_mutex, &maxWait);
			if(timedWait)
			{
				fprintf(stderr, "Client: chairAvailable cond waiting too long\n");
				exit(EXIT_FAILURE);
			}
			if (debug >= 2)
            	printf("\t... wątek #%d otrzymał sygnał!\n", id);
			mutexCode = pthread_mutex_lock(&accessWaitingQueue);
			if(mutexCode)
			{
				fprintf(stderr, "Client: accessWaitingQueue [2] could not be locked\n");
				exit(EXIT_FAILURE);
			}
			if (id==front(&waitingQueue) && front(&waitingQueue) != -1 )
            {
                if(debug >= 2)
        			printf("Client leaving queue, clent id: %d\n", id);
                if(debug >= 2)
        			printf("Queue front before: %d\n", front(&waitingQueue));
                if(debug >= 2)
        			printf("Waiting clients count before: %d\n", current_queue_size(&waitingQueue));
                pthread_mutex_unlock(&accessWaitingQueue);
				pthread_mutex_lock(&hairdressersChairTaken);
				while(currentlyCutId!=id)
				{
					pthread_mutex_unlock(&hairdressersChairTaken);
					pthread_cond_broadcast(&customerReady);
				}
                break;
            }
			pthread_mutex_unlock(&accessWaitingQueue);
		} while (1);
		mutexCode = pthread_mutex_lock(&customer_haircut_mutex);
		if(mutexCode)
		{
			fprintf(stderr, "Client: customer_haircut_mutex [1] could not be locked\n");
			exit(EXIT_FAILURE);
		}
		if (debug >= 2)
			printf("Client is having a haircut, client id: %d\n", id);
		struct timespec maxWait = {0, 0};
		const int getTime = clock_gettime(CLOCK_REALTIME, &maxWait);
		if (getTime)
		{
			fprintf(stderr, "Could not get system time\n");
			exit(EXIT_FAILURE);
		}
		maxWait.tv_sec += MAX_TIMED_WAIT;
		const int timedWait = pthread_cond_timedwait(&haircutDone, &customer_haircut_mutex, &maxWait);
		if(timedWait)
		{
			fprintf(stderr, "Client: haircutDone cond waiting too long\n");
			exit(EXIT_FAILURE);
		}
		if (debug >= 2)
        	printf("Client leaving barbershop, client id: %d\n", id);
		pthread_mutex_lock(&hairdressersChairTaken);
		currentlyCutId=-1;
		pthread_mutex_unlock(&hairdressersChairTaken);
		pthread_mutex_unlock(&customer_haircut_mutex);
		pthread_mutex_unlock(&customer_waiting_mutex);
    }

	return NULL;
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
    //Enable debug level 1
    if(argc == 4 && strcmp(argv[3],"-debug")==0)
    {
        debug = 1;
    }
		//Or debug level 2
		else if(argc == 4 && strcmp(argv[3],"-debug2")==0)
		{
			debug = 2;
		}
    if(debug == 1)
      printf("Debug level 1 enabled\n");
		else if(debug == 2)
			printf("Debug level 2 enabled\n");
    //Set all chairs count
        totalChairs = atoi(argv[1]);
    if(totalChairs <= 0)
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

    if(debug >= 2)
        printf("(debug) N chairs = %d\n",totalChairs);

    //Init queues
    if(debug >= 2)
        printf("(debug) Initialize queues\n");
    init(&waitingQueue);
	init(&resignedQueue);
		//Create barber thread
		pthread_t barberID;
		int barberCode = pthread_create(&barberID, NULL, &Barber, NULL);
		if(barberCode != 0)
		{
			fprintf(stderr, "Could not create barber thread\n");
		}
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
		if(debug >= 2)
			printf("(debug) Program begins\n");
    int j;
	/* utworzenie wątków */
	for (j=0; j < totalClientsCount; j++) {
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
	printf("Barber finished work for today.\n");
    exit(EXIT_SUCCESS);
}
