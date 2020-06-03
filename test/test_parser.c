#include "../parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  assert(argc == 2); /* filename + path file di configurazione */

  assert(access(argv[1], R_OK) != -1); /* file esiste e si può leggere */

  config_t config;
  parse_config(argv[1], &config);

  /* controlla che tutti i parametri di configurazione siano definiti:
   * se il file non contiene un parametro, il test fallisce. */
  for (int i=0; i<N_PARAMS; i++) {
    assert(config.params[i] != UNDEFINED_PARAM);
    assert(config.params[i] >= 0);
  }

  exit(EXIT_SUCCESS);
}
