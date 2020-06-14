#ifndef _DIRETTORE_H
#define _DIRETTORE_H

/*
 * Implementa le funzionalità del thread direttore
 *
 */

struct cassiere;
struct supermercato;

extern void init_direttore(struct supermercato *supermercato, int s1, int s2);
extern void comunica_numero_clienti(const struct cassiere *cassiere, int n);
extern void terminate_direttore();



#endif
