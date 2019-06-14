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

#define ENABLE_SLEEP 0

//1 - running in debug mode
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



void* Barber(void* arg)
{
	if (debug)
		printf("Created barber\n");
    while(1)
    {
    	//Obsłużenie jednego klienta
    	while(1)
    	{
            if (debug)
                printf("Barber: Checking queue...!\n");
			//Sprawdz raz kolejke
    		pthread_mutex_lock(&accessWaitingQueue);
    		//Jest ktoś
    		if(current_queue_size(&waitingQueue) > 0)
    		{
                pthread_mutex_unlock(&accessWaitingQueue);
                if (debug)
                    printf("Barber: Queue not empty! Preparing chair...\n");
                //barber przygotowuje miejsce do strzyzenia
                //TO DO: sleep
                //barber woła następnego klienta
                if (debug)
                    printf("Barber: Next client!\n");
                pthread_cond_broadcast(&chairAvailable);
                //barber czeka aż pojawi się klient
                pthread_cond_wait(&customerReady, &barber_mutex);
                break;
    		}
    		//Nie ma - ucinamy drzemke
    		else
    		{
    			pthread_mutex_unlock(&accessWaitingQueue);
                if (debug)
                    printf("Barber: Queue empty! Going to sleep...\n");
    			pthread_mutex_lock(&barber_napping_mutex);
				pthread_cond_wait(&customerShowedUp, &barber_napping_mutex);
    			pthread_mutex_unlock(&barber_napping_mutex);
    			printf("Barber: waking up\n");
    		}
    	}
    pthread_mutex_lock(&accessWaitingQueue);
    printf("Barber: haircutting customer, cliend id: %d\n", front(&waitingQueue));
    pop(&waitingQueue);
    pthread_mutex_unlock(&accessWaitingQueue);
    //barber skończył strzyc klienta
	printf("Barber: haircut done\n");
    pthread_cond_broadcast(&haircutDone);
    }
    return 0;
}

void* Client(void* numer) {
    int id;
    id = *((int *) numer);
	if(debug)
		printf("\t(debug) Start thread #%d\n", id);
    if(debug)
        printf("Client %d checks if chairs are available\n", id);
	//sprawdzamy kolejkę
	pthread_mutex_lock(&accessWaitingQueue);
	//Nie ma wolnych krzeseł - rezygnujemy
	if(current_queue_size(&waitingQueue) >= totalChairs)
	{
        pthread_mutex_unlock(&accessWaitingQueue);
		//Add client that resigned to resigned queue
		pthread_mutex_lock(&accessResignedQueue);
        push(&resignedQueue, id);
        if(debug)
			printf("Resigned count: %d\n", current_queue_size(&resignedQueue));
		pthread_mutex_unlock(&accessResignedQueue);
		if(debug)
			printf("Client %d resigned\n", id);
    }
    else
    {
        if(debug)
            printf("Client joining queue, clent id: %d\n", id);
        push(&waitingQueue, id);
        if(debug)
        	printf("Waiting clients count: %d\n", current_queue_size(&waitingQueue));
        pthread_mutex_unlock(&accessWaitingQueue);
        //klient wchodzi do sklepu, uruchamia się pozytywka przy drzwiach,
        // ktora budzi fryzjera
        pthread_cond_broadcast(&customerShowedUp);
        //Something changed -> print full message here
        // pthread_mutex_lock(&accessWaitingQueue);
        //klient staje w kolejce i czeka na zawolanie

		pthread_mutex_lock(&customer_waiting_mutex);
		do {
            printf("\twątek #%d oczekuje na sygnał...\n", id);
            pthread_cond_wait(&chairAvailable, &customer_waiting_mutex);
            printf("\t... wątek #%d otrzymał sygnał!\n", id);
			pthread_mutex_lock(&accessWaitingQueue);
			if (id==front(&waitingQueue) && front(&waitingQueue) != -1 )
            {
                if(debug)
        			printf("Client leaving queue, clent id: %d\n", id);
                if(debug)
        			printf("Queue front before: %d\n", front(&waitingQueue));
                if(debug)
        			printf("Waiting clients count before: %d\n", current_queue_size(&waitingQueue));
                pthread_mutex_unlock(&accessWaitingQueue);
				pthread_cond_broadcast(&customerReady);
                break;
            }
			pthread_mutex_unlock(&accessWaitingQueue);
		} while (1);
        pthread_mutex_unlock(&customer_waiting_mutex);
		printf("Client is having a haircut, client id: %d\n", id);
        pthread_mutex_lock(&customer_haircut_mutex);
        pthread_cond_wait(&haircutDone, &customer_haircut_mutex);
        pthread_mutex_lock(&customer_haircut_mutex);
        printf("Client leaving barbershop, client id: %d\n", id);
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
    //Enable debug
    if(argc == 4 && strcmp(argv[3],"-debug")==0)
    {
        debug = 1;
    }
    if(debug)
        printf("Debug mode enabled\n");
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

    if(debug)
        printf("(debug) N chairs = %d\n",totalChairs);

    //Init queues
    if(debug)
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
    exit(EXIT_SUCCESS);
}
