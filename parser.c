#include "parser.h"
#include "defines.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* Nomi dei parametri di configurazione */
static const char* params_names[N_PARAMS] = {
  "C", "E", "K", "I", "T", "P", "TP", "AI", "S1", "S2"
};

/*
 * Parsa il file di configurazione la cui path è passata in input e imposta
 * le variabili globali
 */
void parse_config(const char *path, config_t *config) {
  assert(path != NULL && config != NULL);

  FILE *file = fopen(path, "r");

  if (file == NULL) {
    handle_error("parse_config fopen");
  }

  char line[80 + 1];
  char *key, *value;

  /* inizializza i parametri di configurazione con valori di default */
  for (int i=0; i<N_PARAMS; i++) {
    config->params[i] = UNDEFINED_PARAM;
  }

  while(fgets(line, 80, file) != NULL) {
    // printf("%s", line);
    key = strtok(line, "=");
    if (key != NULL) { /* splitta la linea */
      // printf("%s", key);
      value = strtok(NULL, "=");

      /* itera i valori dell'enum config_params */
      for (int i=0; i<N_PARAMS; i++) {
        /* confronta la chiave con il nome del parametro */
        if (!strcmp(key, params_names[i])) {
          config->params[i] = atoi(value); /* converte la stringa in intero */
        }
      }
    }
  }

  /* debug checking */
  for (int i=0; i<N_PARAMS; i++) {
    assert(config->params[i] != UNDEFINED_PARAM);
  }

  fclose(file);
}