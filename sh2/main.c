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

pthread_mutex_t	mutex	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	chairAvailable	= PTHREAD_COND_INITIALIZER;

char warunek = 0;

void* Client(void* numer) {
    int id;
    id = *((int *) numer);
	printf("\turuchomiono wątek #%d\n", id);
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
    	while (1) {
    		pthread_mutex_lock(&mutex);
    		do {
    			if (warunek)
    				break;
    			else {
                    if(debug)
            			printf("Client joining queue, clent id: %d\n", id);
            		//Something changed -> print full message here
            		pthread_mutex_lock(&accessWaitingQueue);
            		push(&waitingQueue, id);
            		pthread_mutex_unlock(&accessWaitingQueue);
    				printf("\twątek #%d oczekuje na sygnał...\n", id);
    				pthread_cond_wait(&chairAvailable, &mutex);
    				printf("\t... wątek #%d otrzymał sygnał!\n", id);
    			}
    		} while (1);
    		pthread_mutex_unlock(&mutex);
    		/* ... */
    	}
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
        printf("Initialize queues\n");
    init(&waitingQueue);
	init(&resignedQueue);
	//Create barber thread
	pthread_t barberID;
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




	puts("początek programu");
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

	/* wysyłanie sygnałów */

	sleep(1);
	puts("pthread_cond_signal - sygnalizacja");
	pthread_cond_signal(&chairAvailable);

	sleep(1);
	puts("pthread_cond_broadcast - rozgłaszanie");
	pthread_cond_broadcast(&chairAvailable);

	sleep(1);

	/* kończymy proces, bez oglądania się na wątki */
	puts("koniec programu");
	return EXIT_SUCCESS;
}
