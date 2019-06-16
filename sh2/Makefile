CFLAGS = -Wall --std=c99
OBJ= main.o queue.o

sleepinghairdresser1: $(OBJ)
	gcc $(OBJ) -pthread -o sleepinghairdresser2 $(CFLAGS)

.PHONY : clean
clean:
	rm *.o sleepinghairdresser2
