#include <mvar-internal.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include <stdio.h>

#define IMPLIES(a, b) ((a) ? (b) : 1)

#define KNOWN_ERRORS_1(e, e1) do { \
		assert (IMPLIES ((e), (e) == (e1))); \
	} while (0)

#define KNOWN_ERRORS_2(e, e1, e2) do { \
		assert (IMPLIES ((e), (e) == (e1) || (e) == (e2))); \
	} while (0)

#define KNOWN_ERRORS_3(e, e1, e2, e3) do { \
		assert (IMPLIES ((e), (e) == (e1) || (e) == (e2) || (e) == (e3))); \
	} while (0)

#define RECOVERABLE_1(e, target, e1) do { \
		if ((e) == (e1)) \
			goto target; \
	} while (0)

#define RECOVERABLE_2(e, target, e1, e2) do { \
		if ((e) == (e1) || (e) == (e2)) \
			goto target; \
	} while (0)

#define UNRECOVERABLE(e) do { \
		assert (!(e)); \
	} while (0)

#define COND_INIT(e, target) do { \
		KNOWN_ERRORS_3 (e, EAGAIN, EINVAL, ENOMEM); \
		RECOVERABLE_2 (e, target, EAGAIN, ENOMEM); \
		UNRECOVERABLE (e); \
	} while (0)

#define MUTEX_INIT(e, target) do { \
		KNOWN_ERRORS_3 (e, EAGAIN, EINVAL, ENOMEM); \
		RECOVERABLE_2 (e, target, EAGAIN, ENOMEM); \
		UNRECOVERABLE (e); \
	} while (0)

#define MUTEX_DESTROY(e) do { \
		KNOWN_ERRORS_2 (e, EBUSY, EINVAL); \
		UNRECOVERABLE (e); \
	} while (0)

#define COND_DESTROY(e) do { \
		KNOWN_ERRORS_2 (e, EBUSY, EINVAL); \
		UNRECOVERABLE (e); \
	} while (0)

void
mvar_destroy (MVar *const mvar)
{
	int e = 0;
	assert (mvar);

	e = pthread_mutex_destroy (&mvar->lock);
	MUTEX_DESTROY (e);

	e = pthread_cond_destroy (&mvar->put);
	COND_DESTROY (e);

	e = pthread_cond_destroy (&mvar->read);
	COND_DESTROY (e);

	e = pthread_cond_destroy (&mvar->take);
	COND_DESTROY (e);

	mvar->value = NULL;
}

int
mvar_empty (MVar *const mvar)
{
	assert (mvar);
	return !mvar->value;
}

void
mvar_free (MVar *const mvar)
{
	assert (mvar);
	mvar_destroy (mvar);
	free (mvar);
}

int
mvar_init (MVar *const mvar, void *const value)
{
	int e = 0;
	assert (mvar);

	e = pthread_mutex_init (&mvar->lock, NULL);
	MUTEX_INIT (e, e_lock);

	e = pthread_cond_init (&mvar->put, NULL);
	COND_INIT (e, e_put);

	e = pthread_cond_init (&mvar->read, NULL);
	COND_INIT (e, e_read);

	e = pthread_cond_init (&mvar->take, NULL);
	COND_INIT (e, e_take);

	mvar->value = NULL;

	if (value)
		e = mvar_put (mvar, value);

	goto end;

	e = pthread_cond_destroy (&mvar->take);
	UNRECOVERABLE (e);
e_take:

	e = pthread_cond_destroy (&mvar->read);
	UNRECOVERABLE (e);
e_read:

	e = pthread_cond_destroy (&mvar->put);
	UNRECOVERABLE (e);
e_put:

	e = pthread_mutex_destroy (&mvar->lock);
	UNRECOVERABLE (e);
e_lock:

end:
	return e;
}

int
mvar_lock (MVar *mvar)
{
	int e = 0;
	assert (mvar);

	e = pthread_mutex_lock (&mvar->lock);
	KNOWN_ERRORS_2 (e, EDEADLK, EINVAL);
	RECOVERABLE_1 (e, e_lock, EDEADLK);
	UNRECOVERABLE (e);

	return 0;

e_lock:
	return e;
}

MVar *
mvar_new (void *const value)
{
	int e = 0;
	MVar *mvar = malloc (sizeof (MVar));
	if (!mvar) {
		e = errno;
		KNOWN_ERRORS_1 (e, ENOMEM);
		RECOVERABLE_1 (e, e_alloc, ENOMEM);
	}

	e = mvar_init (mvar, value);
	KNOWN_ERRORS_2 (e, EAGAIN, ENOMEM);
	RECOVERABLE_2 (e, e_init, EAGAIN, ENOMEM);

	return mvar;

e_init:

	free (mvar);
e_alloc:

	return NULL;
}

int
mvar_put (MVar *const mvar, void *const value)
{
	int e = 0;
	assert (mvar);

	/* Putting nothing in does nothing. */
	if (!value) {
		e = EINVAL;
		RECOVERABLE_1 (e, e_arg, EINVAL);
	}

	e = mvar_lock (mvar);
	KNOWN_ERRORS_1 (e, EINVAL);
	RECOVERABLE_1 (e, e_lock, EDEADLK);
	UNRECOVERABLE (e);

	/* Block until empty. Loop to handle spurious wakeups. */
	while (mvar->value) {
		e = pthread_cond_wait (&mvar->put, &mvar->lock);
		KNOWN_ERRORS_1 (e, EINVAL);
		UNRECOVERABLE (e);
	}

	/* Perform the actual store. */
	mvar->value = value;

	/* Notify everyone waiting to read. */
	e = pthread_cond_broadcast (&mvar->read);
	KNOWN_ERRORS_1 (e, EINVAL);
	UNRECOVERABLE (e);

	/* Notify someone waiting to take. Assumes fairness! */
	e = pthread_cond_signal (&mvar->take);
	KNOWN_ERRORS_1 (e, EINVAL);
	UNRECOVERABLE (e);

	mvar_unlock (mvar);

e_lock:
e_arg:

	return e;
}

/* Returns NULL and sets errno to EDEADLK if a deadlock would occur. */
void *
mvar_read (MVar *const mvar)
{
	int e = 0;
	void *value;
	assert (mvar);

	e = mvar_lock (mvar);
	KNOWN_ERRORS_1 (e, EDEADLK);
	RECOVERABLE_1 (e, e_lock, EDEADLK);

	/* Block until non-empty. Loop to handle spurious wakeups. */
	while (!mvar->value) {
		e = pthread_cond_wait (&mvar->read, &mvar->lock);
		KNOWN_ERRORS_1 (e, EINVAL);
		UNRECOVERABLE (e);
	}

	/* Perform actual load. */
	value = mvar->value;

	mvar_unlock (mvar);

	/* Return new value. */
	return value;

e_lock:
	errno = e;
	return NULL;

}

void *
mvar_take (MVar *const mvar)
{
	int e = 0;
	void *value;
	assert (mvar);

	mvar_lock (mvar);
	KNOWN_ERRORS_1 (e, EDEADLK);
	RECOVERABLE_1 (e, e_lock, EDEADLK);

	/* Block until non-empty. Loop to handle spurious wakeups. */
	while (!mvar->value) {
		e = pthread_cond_wait (&mvar->take, &mvar->lock);
		KNOWN_ERRORS_1 (e, EINVAL);
		UNRECOVERABLE (e);
	}

	/* Exchange value for NULL. */
	value = mvar->value;
	mvar->value = NULL;

	/* Notify someone waiting to put. */
	e = pthread_cond_signal (&mvar->put);
	KNOWN_ERRORS_1 (e, EINVAL);
	UNRECOVERABLE (e);

	mvar_unlock (mvar);

	/* Return old value. */
	return value;

e_lock:
	errno = e;
	return NULL;

}

int
mvar_try_put (MVar *const mvar, void *const value)
{
	int e = 0;
	assert (mvar);

	/* Putting nothing in does nothing. */
	if (!value) {
		e = EINVAL;
		RECOVERABLE_1 (e, e_arg, EINVAL);
	}

	e = mvar_lock (mvar);
	KNOWN_ERRORS_1 (e, EDEADLK);
	RECOVERABLE_1 (e, e_lock, EDEADLK);

	/* Don't block, just test. */
	if (!mvar->value) {

		mvar->value = value;

		/* Notify everyone waiting to read. */
		e = pthread_cond_broadcast (&mvar->read);
		KNOWN_ERRORS_1 (e, EINVAL);
		UNRECOVERABLE (e);

		/* Notify someone waiting to take. Assumes fairness! */
		e = pthread_cond_signal (&mvar->take);
		KNOWN_ERRORS_1 (e, EINVAL);
		UNRECOVERABLE (e);

	}

	mvar_unlock (mvar);
e_lock:
e_arg:

	return e;

}

void *
mvar_try_read (MVar *const mvar)
{
	int e = 0;
	void *value = NULL;
	assert (mvar);

	e = mvar_lock (mvar);
	KNOWN_ERRORS_1 (e, EDEADLK);
	RECOVERABLE_1 (e, e_lock, EDEADLK);

	value = mvar->value;

	mvar_unlock (mvar);
e_lock:

	return value;
}

void *
mvar_try_take (MVar *const mvar)
{
	int e = 0;
	void *value = NULL;
	assert (mvar);

	e = mvar_lock (mvar);
	KNOWN_ERRORS_1 (e, EDEADLK);
	RECOVERABLE_1 (e, e_lock, EDEADLK);

	/* Don't block, just test. */
	if (mvar->value) {

		/* Exchange value for NULL. */
		value = mvar->value;
		mvar->value = NULL;

		/* Notify someone waiting to put. */
		e = pthread_cond_signal (&mvar->put);
		KNOWN_ERRORS_1 (e, EINVAL);
		UNRECOVERABLE (e);

	}

	mvar_unlock (mvar);
e_lock:

	/* Return old value. */
	return value;
}

void
mvar_unlock (MVar *mvar)
{
	int e = 0;
	assert (mvar);
	e = pthread_mutex_unlock (&mvar->lock);
	KNOWN_ERRORS_2 (e, EINVAL, EPERM);
	UNRECOVERABLE (e);
}
