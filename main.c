#include "supermercato.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[2]) {
  supermercato_t *s = create_supermercato(15, 15);
  sleep(5);
  close_supermercato(s);
  exit(EXIT_SUCCESS);
}
