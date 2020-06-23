#ifndef STOPWATCH_H
#define STOPWATCH_H

#define STOPWATCH_STOPPED 0
#define STOPWATCH_STARTING 1

typedef struct stopwatch stopwatch_t;

struct stopwatch *stopwatch_create(int start);
void stopwatch_start(struct stopwatch *stopwatch);
int stopwatch_end(struct stopwatch *stopwatch);
void stopwatch_free(struct stopwatch *stopwatch);

#endif
