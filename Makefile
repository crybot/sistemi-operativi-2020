CC = gcc
CFLAGS = -Wall -pedantic -pthread
OBJECTS = main.o supermercato.o cliente.o cassiere.o direttore.o queue.o
TEST = test
TESTS = $(wildcard $(TEST)/*.c)
TEST_BINS = $(patsubst $(TEST)/%.c, $(TEST)/%.test, $(TESTS))

.PHONY: clean test test_queue

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o main

main.o: main.c

supermercato.o: supermercato.c supermercato.h cassiere.h defines.h

cliente.o: cliente.c cliente.h defines.h

cassiere.o: cassiere.c cassiere.h defines.h

direttore.o: direttore.c direttore.h defines.h

queue.o: queue.c queue.h

test: $(TEST_BINS)
	@echo "Test eseguiti con successo!"

%.test: %.c queue.o 
	$(CC) $(CFLAGS) $^ -o $@
	@$@ $$(cat $*.input) || echo "Test fallito!"

clean:
	-rm -f *.o *.gch main
	-rm -f $(TEST)/*.test
