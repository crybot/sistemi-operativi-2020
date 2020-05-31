#ifndef _CLIENTE_H
#define _CLIENTE_H

#define _MIN_CLIENT_TIME 10
#define _MIN_CLIENT_PURCHASE 0
#include <pthread.h>

typedef struct cliente {
  pthread_t* _thread; // thread di lavoro del cliente
}cliente_t;

cliente_t* create_cliente(int lifetime, int products);


#endif

