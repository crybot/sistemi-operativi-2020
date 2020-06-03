CC = gcc
CFLAGS = -g -Wall -Wpedantic -pthread -Warray-bounds -Wextra -Wwrite-strings -Wno-parentheses
OBJECTS = supermercato.o cliente.o cassiere.o direttore.o queue.o parser.o
SRC = src
TEST = test
TESTS = $(wildcard $(TEST)/*.c)
TEST_BINS = $(patsubst $(TEST)/%.c, $(TEST)/%.test, $(TESTS))
MAIN = main

.PHONY: clean test

all: $(MAIN).o $(OBJECTS)
	$(CC) $(CFLAGS) $< $(OBJECTS) -o $(MAIN)

$(MAIN).o: $(MAIN).c supermercato.h cliente.h cassiere.h parser.h

supermercato.o: supermercato.c supermercato.h cassiere.h defines.h

cliente.o: cliente.c cliente.h supermercato.h defines.h utils.h

cassiere.o: cassiere.c cassiere.h cliente.h defines.h utils.h

direttore.o: direttore.c direttore.h defines.h

queue.o: queue.c queue.h

parser.o: parser.c parser.h

test: $(TEST_BINS)
	@echo "Test eseguiti con successo!"

%.test: %.c $(OBJECTS)
	@echo Esecuzione test $*
	$(CC) $(CFLAGS) $^ -o $@
	@$@ $$(cat $*.input) > $*.output || (echo "Test fallito: asserzioni fallite" && exit 1)
	@diff -q --new-file $*.output $*.expected || (echo "Test fallito: output non corretto" && exit 1)

clean:
	-rm -f *.o *.gch $(MAIN)
	-rm -f $(TEST)/*.test $(TEST)/*.output
