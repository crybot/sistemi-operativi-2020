SHELL = /bin/bash
CC = gcc
CFLAGS = -g -Wall -Wpedantic -pthread -Warray-bounds -Wextra -Wwrite-strings -Wno-parentheses
OBJECTS = supermercato.o cliente.o cassiere.o direttore.o queue.o parser.o threadpool.o logger.o stopwatch.o
SRC = src
TEST = test
TESTS = $(wildcard $(TEST)/*.c)
TEST_BINS = $(patsubst $(TEST)/%.c, $(TEST)/%.test, $(TESTS))
MAIN = simulazione
CONFIG_TEST = test.txt
LOG_TEST = test.log
ANALYSIS = analisi.sh

.PHONY: clean test test2

all: $(MAIN).o $(OBJECTS)
	$(CC) $(CFLAGS) $< $(OBJECTS) -o $(MAIN)

$(MAIN).o: $(MAIN).c supermercato.h cliente.h cassiere.h parser.h direttore.h logger.h

supermercato.o: supermercato.c supermercato.h cassiere.h defines.h logger.h

cliente.o: cliente.c cliente.h supermercato.h defines.h utils.h stopwatch.h logger.h

cassiere.o: cassiere.c cassiere.h cliente.h defines.h utils.h stopwatch.h logger.h

direttore.o: direttore.c direttore.h cassiere.h supermercato.h defines.h

queue.o: queue.c queue.h defines.h

parser.o: parser.c parser.h defines.h

threadpool.o: threadpool.c threadpool.h defines.h

logger.o: logger.c logger.h defines.h

stopwatch.o: stopwatch.c stopwatch.h defines.h

test2: all
	-rm -f $(LOG_TEST)
	-rm -f $(CONFIG_TEST)
	@echo "K=6" >> $(CONFIG_TEST)
	@echo "C=50" >> $(CONFIG_TEST)
	@echo "E=3" >> $(CONFIG_TEST)
	@echo "T=200" >> $(CONFIG_TEST)
	@echo "P=100" >> $(CONFIG_TEST)
	@echo "S=20" >> $(CONFIG_TEST)
	@echo "S1=2" >> $(CONFIG_TEST)
	@echo "S2=10" >> $(CONFIG_TEST)
	@echo "I=2" >> $(CONFIG_TEST)
	@echo "TP=10" >> $(CONFIG_TEST)
	@echo "LOG=$(LOG_TEST)" >> $(CONFIG_TEST)
	@echo Esecuzione simulazione
	@./$(MAIN) -c $(CONFIG_TEST) & PID=$$!; \
		sleep 25 && kill -SIGQUIT $$PID \
		&& wait $$PID
	@./$(ANALYSIS) ./$(LOG_TEST)

test: $(TEST_BINS)
	$(MAKE) test2
	@echo "Test eseguiti con successo!"

%.test: %.c $(OBJECTS)
	@echo Esecuzione test $*
	$(CC) $(CFLAGS) $^ -o $@
	@$@ $$(cat $*.input) > $*.output || (echo "Test fallito: asserzioni fallite" && exit 1)
	@diff -q --new-file $*.output $*.expected || (echo "Test fallito: output non corretto" && exit 1)

clean:
	-rm -f *.o *.gch $(MAIN)
	-rm -f $(TEST)/*.test $(TEST)/*.output
	-rm -f *.log
	-rm -f $(CONFIG_TEST)
