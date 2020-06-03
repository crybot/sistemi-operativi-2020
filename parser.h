#ifndef PARSER_H
#define PARSER_H

#define UNDEFINED_PARAM (-1)

enum config_params {
  C, /* max numero clienti */
  E, /* numero di clienti da far entrare per volta */
  K, /* numero di casse totali */
  I, /* numero iniziale di casse */
  T, /* tempo massimo per gli acquisti per i clienti */
  P, /* numero massimo di prodotti acquistabili per i clienti */
  TP, /* tempo di gestione singolo prodotto da parte di un cassiere */
  AI, /* ampiezza intervallo di comunicazione tra cassiere e direttore */
  S1, /* soglia chiusura cassa: numero di casse con al più un cliente */
  S2, /* soglia apertura cassa: numero di clienti in coda in una cassa */
  N_PARAMS /* numero di parametri configurabili */
};

/*
 * Parametri di configurazione della simulazione.
 * Le lettere maiuscole per i campi sono utilizzate per congruenza con il testo
 * del progetto.
 */
typedef struct config {
  int params[N_PARAMS]; /* parametri configurabili */
  char *LOG; /* nome del file di log */
}config_t;

extern void parse_config(const char *path, config_t *config);

#endif
