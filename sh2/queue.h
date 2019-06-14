#ifndef QUEUE_H
	#define QUEUE_H
	struct Client
	{
	int id;
	struct Client *next;
	};

	struct Queue
	{
	struct Client *front;
	struct Client *last;
	unsigned int size;
	};

	void init(struct Queue *q);

	int front(struct Queue *q);

	char* last(struct Queue *q);

	void pop(struct Queue *q);

	void push(struct Queue *q, int id);

	void print_queue(struct Queue *q);

	int current_queue_size(struct Queue *q);

#endif
