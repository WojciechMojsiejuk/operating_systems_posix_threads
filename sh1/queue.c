#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

void init(struct Queue *q) {
	q->front = NULL;
	q->last = NULL;
	q->size = 0;
}

char* front(struct Queue *q) {
  if(q->front != NULL)
  {
  	return q->front->id;
  }
  return NULL;
}

char* last(struct Queue *q)
{
  if(q->last != NULL)
  {
  	return q->last->id;
  }
  return NULL;
}

void pop(struct Queue *q) {
	q->size--;

	struct Node *tmp = q->front;
	q->front = q->front->next;
	free(tmp);
}

void push(struct Queue *q, int id) {
	q->size++;

	if (q->front == NULL) {
		q->front = (struct Client *) malloc(sizeof(struct Client));
		q->front->id = id;
		q->front->next = NULL;
		q->last = q->front;
	} else {
		q->last->next = (struct Client *) malloc(sizeof(struct Client));
		q->last->next->id = id;
		q->last->next->next = NULL;
		q->last = q->last->next;
	}
}

// void print_queue(struct Queue *q)
// {
//   struct Client *temp = q->front;
//   while(temp !=NULL)
//   {
//       printf("%s\n",temp->id);
//       temp=temp->next;
//   }
// }

int current_queue_size(struct Queue *q)
{
  return q->size;
}
