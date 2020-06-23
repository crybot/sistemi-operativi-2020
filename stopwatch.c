#include "stopwatch.h"
#include "defines.h"
#include <time.h>
#include <stdlib.h>

typedef struct stopwatch {
  struct timespec start;
  struct timespec end;
  int started;
}stopwatch_t;

/*
 * Calcola la differenza in millisecondi tra le due misurazioni di uno stopwatch.
 */
static int diff_ms(stopwatch_t *stopwatch) {
  return (stopwatch->end.tv_sec - stopwatch->start.tv_sec)*1000 +
    (stopwatch->end.tv_nsec - stopwatch->start.tv_nsec)/(1000*1000);
}

/*
 * Crea e restituisce il puntatore a un nuovo stopwatch.
 * - se start == STOPWATCH_STOPPED, lo stopwatch è inizialmente inattivo;
 * - se start == STOPWATCH_STARTING, lo stopwatch è inizialmente attivo.
 * - in tutti gli altri casi lo stopwatch è inattivo di default.
 */
stopwatch_t *stopwatch_create(int start) {
  stopwatch_t *stopwatch = (stopwatch_t*)malloc(sizeof(stopwatch_t));
  if (stopwatch == NULL) {
    handle_error("stopwatch_create: malloc");
  }
  if (start == STOPWATCH_STARTING) {
    stopwatch->started = 1;
    clock_gettime(CLOCK_REALTIME, &stopwatch->start); /* Inizializza lo stopwatch */
  }
  else {
    stopwatch->started = 0;
  }

  return stopwatch;
}

/*
 * Fa partire lo stopwatch il quale inizia a misurare il tempo reale (wall time)
 * trascorso a partire dal momento della chiamata.
 * Se lo stopwatch era già stato avviato, la vecchia misurazione viene sovrascritta.
 */
void stopwatch_start(stopwatch_t *stopwatch) {
  if (stopwatch == NULL) {
    handle_error("stopwatch_start: NULL stopwatch");
  }
  stopwatch->started = 1;
  clock_gettime(CLOCK_REALTIME, &stopwatch->start);
}

/*
 * Ferma lo stopwatch e restituisce il tempo trascorso in millisecondi dal momento
 * in cui è stato avviato.
 * Se lo stopwatch non era già stato avviato restituisce 0.
 */
int stopwatch_end(stopwatch_t *stopwatch) {
  if (stopwatch == NULL) {
    handle_error("stopwatch_end: NULL stopwatch");
  }
  if (!stopwatch->started) {
    return 0;
  }
  stopwatch->started = 0;
  clock_gettime(CLOCK_REALTIME, &stopwatch->end);
  return diff_ms(stopwatch);
}

/*
 * Libera le risorse allocate dallo stopwatch.
 */
void stopwatch_free(stopwatch_t *stopwatch) {
  free(stopwatch);
}
