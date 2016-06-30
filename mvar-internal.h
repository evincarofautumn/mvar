#ifndef MVAR_INTERNAL_H_2222195A_A36B_4D94_9066_E8645E3AA5D9
#define MVAR_INTERNAL_H_2222195A_A36B_4D94_9066_E8645E3AA5D9

#include <mvar.h>
#include <pthread.h>

struct MVar {
	pthread_mutex_t mutex;
	pthread_rwlock_t rw_mutex;
	pthread_cond_t put_cond, read_cond, take_cond;
	void *volatile value;
};

/* Deinitialize an MVar in place. */
void mvar_destroy (MVar *mvar);

/* Initialize an MVar with a value, or NULL for no value. */
void mvar_init (MVar *mvar, void *value);

#endif /* MVAR_INTERNAL_H_2222195A_A36B_4D94_9066_E8645E3AA5D9 */
