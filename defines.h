#ifndef DEFINES_H
#define DEFINES_H
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

#endif
