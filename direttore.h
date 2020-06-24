#ifndef _DIRETTORE_H
#define _DIRETTORE_H

/*
 * Definisce le funzionalità del thread direttore
 */

struct cassiere;
struct supermercato;

void init_direttore(struct supermercato *supermercato, int s1, int s2);
void comunica_numero_clienti(const struct cassiere *cassiere, int n);
void terminate_direttore(void);
void get_permesso(void);



#endif
