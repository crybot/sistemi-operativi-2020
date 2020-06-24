#!/bin/bash

if [ $# -ne 1 ]; then # script name + logfile path
  echo Uso: $0 log_file >> /dev/stderr
  exit 1
fi

LOG=$1

if [ ! -f $LOG ]; then
  echo File $LOG non esistente
fi

# Client lines sorted by ID in ascending order
CLIENT_LINES=$(grep 'CLIENTE' $LOG | sort -t " " -k 2 -g)
CLIENT_IDS=$(echo "$CLIENT_LINES" | cut -d " " -f 2 | cut -d ":" -f 1 | uniq);
CLIENT_PRODS=$(echo "$CLIENT_LINES" | grep "prodotti acquistati" | sed 's/.*=\s//')
CLIENT_TOT_TS=$(echo "$CLIENT_LINES" | grep "tempo totale" | sed 's/.*=\s//')
CLIENT_QUEUE_TS=$(echo "$CLIENT_LINES" | grep "tempo in coda" | sed 's/.*=\s//')
CLIENT_CHANGES=$(echo "$CLIENT_LINES" | grep "cambi di coda" | sed 's/.*=\s//')
CLIENTS=$(echo "$CLIENT_IDS" | tail -1)

# Get number of clients
# echo -n "Numero di clienti: "
# echo $CLIENTS

echo "$CLIENT_IDS" > .ids
echo "$CLIENT_PRODS" > .prods
echo "$CLIENT_TOT_TS" > .tots
echo "$CLIENT_QUEUE_TS" > .qts
echo "$CLIENT_CHANGES" > .changes

# echo RIASSUNTO CLIENTI
# echo -e "ID\tPROD.\tT.TOT\tT.CODA\tN.CODE"
paste .ids .prods .tots .qts .changes > analisi.log

# Imposta il locale in modo da intepretare i numeri in virgola mobile
# correttamente 
export LC_ALL=C

# # Fallisce se trova numeri negativi
[ $(grep -q -s -- '-[1-9]*' analisi.log) ] &&
  { echo "Sono presenti valori negativi" >> /dev/stderr; exit 1; }

# Fallisce se un cliente acquista troppi prodotti
[ -z "$(awk '$1 > 100' .prods)" ] || 
  { echo "errore prodotti acquistati dai clienti" >> /dev/stderr; exit 1; }

# Fallisce se un cliente impiega troppo tempo nel supermercato
[ -z "$(awk '$1 > 20' .tots)" ] || 
  { echo "errore tempo totale cliente" >> /dev/stderr; exit 1; }

# Fallisce se un cliente impiega troppo tempo in coda
[ -z "$(awk '$1 > 20' .qts)" ] || 
  { echo "errore tempo in coda cliente" >> /dev/stderr; exit 1; }

# Fallisce se un cliente cambia coda troppe volte
[ -z "$(awk '$1 > 6' .changes)" ] || 
  { echo "errore numero cambio code" >> /dev/stderr; exit 1; }

# Rimuove i file temporanei
rm -f .ids .prods .tots .qts .changes

# Cashier lines sorted by ID in ascending order
CASH_LINES=$(grep 'CASSA' $LOG | sort -t " " -k 2 -g)
CASH_IDS=$(echo "$CASH_LINES" | cut -d " " -f 2 | cut -d ":" -f 1 | uniq);
CASH_PRODS=$(echo "$CASH_LINES" | grep "prodotti venduti" | sed 's/.*=\s//')
CASH_CLIENTS=$(echo "$CASH_LINES" | grep "clienti serviti" | sed 's/.*=\s//')
CASH_TOT_TS=$(echo "$CASH_LINES" | grep "tempo totale" | sed 's/.*=\s//')
CASH_AVG_TS=$(echo "$CASH_LINES" | grep "tempo medio servizio" | sed 's/.*=\s//')
CASH_CLOSE=$(echo "$CASH_LINES" | grep "numero chiusure" | sed 's/.*=\s//')

echo "$CASH_IDS" > .ids
echo "$CASH_PRODS" > .prods
echo "$CASH_CLIENTS" > .clients
echo "$CASH_TOT_TS" > .tots
echo "$CASH_AVG_TS" > .avgs
echo "$CASH_CLOSE" > .close

# echo
# echo RIASSUNTO CASSIERI
# echo -e "ID\tPROD.\tCLIENTI\tT.TOT\tT.MEDIO\tN.CHIUS."
paste .ids .prods .clients .tots .avgs .close > analisi.log

# # Fallisce se trova numeri negativi
[ $(grep -q -s -- '-[1-9]*' analisi.log) ] &&
  { echo "Sono presenti valori negativi" >> /dev/stderr; exit 1; }

# Fallisce se sono stati serviti troppi clienti
[ -z "$(awk '$1 > $CLIENTS' .clients)" ] || { echo "errore clienti serviti" >> /dev/stderr; exit 1; }

# Fallisce se le casse sono state aperte troppo tempo
[ -z "$(awk '$1 > 50' .tots)" ] || { echo "errore tempo totale cassiere" >> /dev/stderr; exit 1; }

# Fallisce se le casse hanno un tempo medio di servizio troppo alto
[ -z "$(awk '$1 > 2' .avgs)" ] || { echo "errore tempo medio di servizio" >> /dev/stderr; exit 1; }

# Fallisce se le casse sono state chiuse troppe volte
[ -z "$(awk '$1 > 3' .close)" ] || { echo "errore numero di chiusure casse" >> /dev/stderr; exit 1; }

rm -f .ids .prods .clients .tots .avgs .close

