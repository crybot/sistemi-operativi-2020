#include "logger.h"
#include "defines.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <stdarg.h>

static FILE *file;
static pthread_mutex_t mtx;

/*
 * Apre il file su cui verranno scritti i dati delle successive chiamate a
 * log_write.
 */
void log_setfile(const char *filename) {
  assert(filename != NULL);
  file = fopen(filename, "a+");

  if (file == NULL) {
    handle_error("log_setfile: fopen");
  }

  pthread_mutex_init_ec(&mtx, NULL);
}

/*
 * Scrittura thread-safe su un file di log
 */
void log_write(const char *format, ...) {
  va_list args;
  va_start(args, format);

  pthread_mutex_lock_safe(&mtx);
  if (vfprintf(file, format, args) < 1) {
    handle_error("log_write: vfprintf");
  }
  //TODO: forse è necessario fflush(file)
  pthread_mutex_unlock_safe(&mtx);
  va_end(args);
}

