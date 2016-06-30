#include <mvar-internal.h>

#include <pthread.h>
#include <stdlib.h>

void
mvar_destroy (MVar *const mvar)
{
	pthread_mutex_destroy (&mvar->mutex);
	pthread_rwlock_destroy (&mvar->rw_mutex);
	pthread_cond_destroy (&mvar->put_cond);
	pthread_cond_destroy (&mvar->read_cond);
	pthread_cond_destroy (&mvar->take_cond);
	mvar->value = NULL;
}

int
mvar_empty (MVar *const mvar)
{
	return !mvar->value;
}

void
mvar_free (MVar *const mvar)
{
	mvar_destroy (mvar);
	free (mvar);
}

void
mvar_init (MVar *const mvar, void *const value)
{
	pthread_mutex_init (&mvar->mutex, NULL);
	pthread_rwlock_init (&mvar->rw_mutex, NULL);
	pthread_cond_init (&mvar->put_cond, NULL);
	pthread_cond_init (&mvar->read_cond, NULL);
	pthread_cond_init (&mvar->take_cond, NULL);
	mvar->value = NULL;
	mvar_put (mvar, value);
}

MVar *
mvar_new (void *const value)
{
	MVar *mvar = malloc (sizeof (MVar));
	mvar_init (mvar, value);
	return mvar;
}

void
mvar_put (MVar *const mvar, void *const value)
{
	/* Putting nothing in does nothing. */
	if (!value)
		return;
	pthread_mutex_lock (&mvar->mutex);
	/* Block until empty. Loop to handle spurious wakeups. */
	while (mvar->value)
		pthread_cond_wait (&mvar->put_cond, &mvar->mutex);
	mvar->value = value;
	/* Notify everyone waiting to read. */
	pthread_cond_broadcast (&mvar->read_cond);
	/* Notify someone waiting to take. Assumes fairness! */
	pthread_cond_signal (&mvar->take_cond);
	pthread_mutex_unlock (&mvar->mutex);
}

void *
mvar_read (MVar *const mvar)
{
	/* Lock for reading. */
	pthread_rwlock_rdlock (&mvar->rw_mutex);
	/* I don't know what this is for. */
	pthread_mutex_t lock;
	pthread_mutex_init (&lock, NULL);
	/* Block until non-empty. Loop to handle spurious wakeups. */
	while (!mvar->value)
		pthread_cond_wait (&mvar->read_cond, &lock);
	pthread_mutex_unlock (&lock);
	pthread_mutex_destroy (&lock);
	pthread_rwlock_unlock (&mvar->rw_mutex);
	/* Return new value. */
	return mvar->value;
}

void *
mvar_take (MVar *const mvar)
{
	void *value;
	pthread_mutex_lock (&mvar->mutex);
	/* Block until non-empty. Loop to handle spurious wakeups. */
	while (!mvar->value)
		pthread_cond_wait (&mvar->take_cond, &mvar->mutex);
	/* Lock for writing. */
	pthread_rwlock_wrlock (&mvar->rw_mutex);
	/* Exchange value for NULL. */
	value = mvar->value;
	mvar->value = NULL;
	/* Notify someone waiting to put. */
	pthread_cond_signal (&mvar->put_cond);
	pthread_rwlock_unlock (&mvar->rw_mutex);
	pthread_mutex_unlock (&mvar->mutex);
	/* Return old value. */
	return value;
}

int
mvar_try_put (MVar *const mvar, void *const value)
{
	int status = 0;
	/* Putting nothing in does nothing. */
	if (!value)
		return status;
	pthread_mutex_lock (&mvar->mutex);
	/* Don't block, just test. */
	if (!mvar->value) {
		mvar->value = value;
		/* Notify everyone waiting to read. */
		pthread_cond_broadcast (&mvar->read_cond);
		/* Notify someone waiting to take. Assumes fairness! */
		pthread_cond_signal (&mvar->take_cond);
		status = 1;
	}
	pthread_mutex_unlock (&mvar->mutex);
	return status;
}

void *
mvar_try_read (MVar *const mvar)
{
	return mvar->value;
}

void *
mvar_try_take (MVar *const mvar)
{
	void *value = NULL;
	pthread_mutex_lock (&mvar->mutex);
	/* Don't block, just test. */
	if (mvar->value) {
		/* Lock for writing. */
		pthread_rwlock_wrlock (&mvar->rw_mutex);
		/* Exchange value for NULL. */
		value = mvar->value;
		mvar->value = NULL;
		/* Notify someone waiting to put. */
		pthread_cond_signal (&mvar->put_cond);
		pthread_rwlock_unlock (&mvar->rw_mutex);
	}
	pthread_mutex_unlock (&mvar->mutex);
	/* Return old value. */
	return value;
}
