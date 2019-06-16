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

//Waiting for X seconds in cond_timedwait will result in error
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

pthread_mutex_t	mBarberWokenUp = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cBarberWokenUp = PTHREAD_COND_INITIALIZER;
int barberWokenUp = 0;

/*
pthread_mutex_t* mInvitedToCut;
pthread_cond_t* cInvitedToCut;
int* invitedToCut;
*/

pthread_mutex_t* mFinishedCutting;
pthread_cond_t* cFinishedCutting;
int* finishedCutting;

pthread_mutex_t hairdressersChairTaken;
int currentlyCutId = -1;

//Waits <1;sec> seconds (random number)
void waiting(int sec)
{
    int zzz = (((rand()%sec)+1)*1000000);
		if(ENABLE_SLEEP)
   		usleep(zzz);
}

void* Barber(void* arg)
{
	int result;
	while(1)
	{
		//Sprawdz raz kolejke
		pthread_mutex_lock(&accessWaitingQueue);

		//Ktoś jest w kolejce
		if(current_queue_size(&waitingQueue) > 0)
		{
			/*
			//wyslij sygnal do klienta, ze bedzie go teraz strzygl
			pthread_mutex_lock(&mInvitedToCut[currentlyCutId]);
			invitedToCut[currentlyCutId] = 1;
			pthread_cond_broadcast(&cInvitedToCut[currentlyCutId]);
			pthread_mutex_unlock(&mInvitedToCut[currentlyCutId]);
			*/
			pthread_mutex_lock(&hairdressersChairTaken);
			//Wez klienta
			currentlyCutId = front(&waitingQueue);
			pop(&waitingQueue);
			//Something changed -> print full message here
			//Strzyzenie
			if(debug >=2)
				printf("Barber: cutting %d\n", currentlyCutId);
			pthread_mutex_lock(&accessResignedQueue);
			if(debug)
			{
				printf("Resigned: ");
				print_queue(&resignedQueue);
				printf("\n");
				printf("In queue: ");
				print_queue(&waitingQueue);
				printf("\n");
			}
			printf("Res: %d WRoom %d/%d [in: %d]\n", current_queue_size(&resignedQueue), current_queue_size(&waitingQueue), totalChairs, currentlyCutId);
			pthread_mutex_unlock(&accessWaitingQueue);
			pthread_mutex_unlock(&accessResignedQueue);
			pthread_mutex_unlock(&hairdressersChairTaken);
			waiting(9);
			if(debug >= 2)
				printf("Barber: finished cutting %d\n", currentlyCutId);
			//Skonczono strzyzenie
			////Zmniejsz liczbe klientow
			//pthread_mutex_lock(&accessTotalClientsCount);
			//--totalClientsCount;
			//pthread_mutex_unlock(&accessTotalClientsCount);

			//Wyslij sygnal klientowi ze skonczyl go strzyc
			pthread_mutex_lock(&mFinishedCutting[currentlyCutId]);
			finishedCutting[currentlyCutId] = 1;
			pthread_cond_broadcast(&cFinishedCutting[currentlyCutId]);
			pthread_mutex_unlock(&mFinishedCutting[currentlyCutId]);
			//Nikt nie jest teraz strzyzony - ustaw id na -1
			pthread_mutex_lock(&hairdressersChairTaken);
			currentlyCutId = -1;
			pthread_mutex_unlock(&hairdressersChairTaken);
		}
		//Spij
		else
		{
		    pthread_mutex_unlock(&accessWaitingQueue);
			pthread_mutex_lock(&mBarberWokenUp);
			barberWokenUp = 0;
			while(barberWokenUp == 0)
			{
				pthread_cond_wait(&cBarberWokenUp, &mBarberWokenUp);
			}
			pthread_mutex_unlock(&mBarberWokenUp);
		}
	}
}

void* Client(void* numer) {
    int id;
    id = *((int *) numer);
	if(debug >= 2)
		printf("Created client %d\n", id);
	int result;
	while(1)
	{
		result = pthread_mutex_lock(&accessWaitingQueue);
		if(result)
		{
			fprintf(stderr, "accessWaitingQueue could not be locked\n");
			exit(EXIT_FAILURE);
		}
		//Nie ma wolnych krzeseł - rezygnujemy
		if(current_queue_size(&waitingQueue) >= totalChairs)
		{
			//pthread_mutex_unlock(&accessWaitingQueue);
			//Add client that resigned to resigned queue
			result = pthread_mutex_lock(&accessResignedQueue);
			if(result)
			{
				fprintf(stderr, "accessResignedQueue could not be locked\n");
				exit(EXIT_FAILURE);
			}
			push(&resignedQueue, id);

			////pthread_mutex_lock(&accessTotalClientsCount);
			////--totalClientsCount;
			////pthread_mutex_unlock(&accessTotalClientsCount);
			//Something changed -> print full message here
			//pthread_mutex_lock(&accessWaitingQueue);	
			pthread_mutex_lock(&hairdressersChairTaken);
			if(debug)
			{
				printf("Resigned: ");
				print_queue(&resignedQueue);
				printf("\n");
				printf("In queue: ");
				print_queue(&waitingQueue);
				printf("\n");
			}
			printf("Res: %d WRoom %d/%d [in: %d]\n", current_queue_size(&resignedQueue), current_queue_size(&waitingQueue), totalChairs, currentlyCutId);
			if(debug >= 2)
				printf("Resigned count: %d\n", current_queue_size(&resignedQueue));
			pthread_mutex_unlock(&accessWaitingQueue);
			pthread_mutex_unlock(&hairdressersChairTaken);
			pthread_mutex_unlock(&accessResignedQueue);
			if(debug >= 2)
				printf("Client %d resigned\n", id);
			return 0;
		}
		//Kolejka ma wolne miejsca - dołączamy do kolejki
		else
		{
			if(debug >= 2)
				printf("Client joining queue, clent id: %d\n", id);
			//Something changed -> print full message here
			pthread_mutex_lock(&accessResignedQueue);
			pthread_mutex_lock(&hairdressersChairTaken);
			if(debug)
			{
				printf("Resigned: ");
				print_queue(&resignedQueue);
				printf("\n");
				printf("In queue: ");
				print_queue(&waitingQueue);
				printf("\n");
			}
			printf("Res: %d WRoom %d/%d [in: %d]\n", current_queue_size(&resignedQueue), current_queue_size(&waitingQueue), totalChairs, currentlyCutId);
			pthread_mutex_unlock(&accessResignedQueue);
			pthread_mutex_unlock(&hairdressersChairTaken);
			push(&waitingQueue, id);
			pthread_mutex_unlock(&accessWaitingQueue);

			//wyslij sygnal ze ktos jest gotowy
			pthread_mutex_lock(&mBarberWokenUp);
			barberWokenUp = 1;
			pthread_cond_broadcast(&cBarberWokenUp);
			pthread_mutex_unlock(&mBarberWokenUp);
			/*
			//czekaj az zaprosi go fryzjer
			pthread_mutex_lock(&mInvitedToCut[id]);
			while(invitedToCut[id] == 0)
			{
				pthread_cond_wait(&cInvitedToCut[id], &mInvitedToCut[id]);
			}
			pthread_mutex_unlock(&mInvitedToCut[id]);
			*/
			//czekaj az fryzjer zasygnalizuje, ze skonczyl go strzyc
			pthread_mutex_lock(&mFinishedCutting[id]);
			while(finishedCutting[id] == 0)
			{
				pthread_cond_wait(&cFinishedCutting[id], &mFinishedCutting[id]);
			}
			pthread_mutex_unlock(&mFinishedCutting[id]);
			//idz do domu
			break;
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
		/*
		//alloc and init invitedToCut
		mInvitedToCut = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*totalClientsCount);
		if(mInvitedToCut == NULL)
		{
			fprintf(stderr, "Could not allocate memory\n");
			return 9;
		}
		cInvitedToCut = (pthread_cond_t*)malloc(sizeof(pthread_cond_t)*totalClientsCount);
		if(cInvitedToCut == NULL)
		{
			fprintf(stderr, "Could not allocate memory\n");
			return 9;
		}
		invitedToCut = (int*)malloc(sizeof(int)*totalClientsCount);
		if(invitedToCut == NULL)
		{
			fprintf(stderr, "Could not allocate memory\n");
			return 9;
		}
		int i;
		for(i=0;i<totalClientsCount;i++)
		{
			int mutexResult = pthread_mutex_init(&mInvitedToCut[i], NULL);
			if(mutexResult)
			{
				fprintf(stderr, "Could not init mInvitedToCut[%d]\n", i);
				return 10;
			}
			int condResult = pthread_cond_init(&cInvitedToCut[i], NULL);
			if(condResult)
			{
				fprintf(stderr, "Could not init cInvitedToCut[%d]\n", i);
				return 11;
			}
			//Client not invited at beginning
			invitedToCut[i] = 0;
		}
		*/
		//alloc and init finishedCutting
				mFinishedCutting = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*totalClientsCount);
		if(mFinishedCutting == NULL)
		{
			fprintf(stderr, "Could not allocate memory\n");
			return 9;
		}
		cFinishedCutting = (pthread_cond_t*)malloc(sizeof(pthread_cond_t)*totalClientsCount);
		if(cFinishedCutting == NULL)
		{
			fprintf(stderr, "Could not allocate memory\n");
			return 9;
		}
		finishedCutting = (int*)malloc(sizeof(int)*totalClientsCount);
		if(finishedCutting == NULL)
		{
			fprintf(stderr, "Could not allocate memory\n");
			return 9;
		}
		for(int i=0;i<totalClientsCount;i++)
		{
			int mutexResult = pthread_mutex_init(&mFinishedCutting[i], NULL);
			if(mutexResult)
			{
				fprintf(stderr, "Could not init mFinishedCutting[%d]\n", i);
				return 12;
			}
			int condResult = pthread_cond_init(&cFinishedCutting[i], NULL);
			if(condResult)
			{
				fprintf(stderr, "Could not init cFinishedCutting[%d]\n", i);
				return 13;
			}
			finishedCutting[i] = 0;
		}
		//Print chairs count
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
			return 9;
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
