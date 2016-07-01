#ifndef MVAR_INTERNAL_H_2222195A_A36B_4D94_9066_E8645E3AA5D9
#define MVAR_INTERNAL_H_2222195A_A36B_4D94_9066_E8645E3AA5D9

#include <mvar.h>

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>

struct MVar {
	pthread_mutex_t lock;
	pthread_cond_t put, read, take;
	void *volatile value;
};

/* Deinitialize an MVar in place. */
void mvar_destroy (MVar *mvar);

/* Initialize an MVar with a value, or NULL for no value. */
int mvar_init (MVar *mvar, void *value);

/* Take exclusive ownership of an MVar. */
int mvar_lock (MVar *mvar);

/* Release ownership of an MVar. */
void mvar_unlock (MVar *mvar);

#endif /* MVAR_INTERNAL_H_2222195A_A36B_4D94_9066_E8645E3AA5D9 */
