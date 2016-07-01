#include <mvar-internal.h>

#include <assert.h>
#include <stdio.h>

typedef struct {
	MVar *mvar;
	int value;
} IncrementorState;

static void *
incrementor (void *data)
{
	IncrementorState *state = (IncrementorState *)data;
	int i;
	for (i = 0; i < 100000; ++i) {
		(void)mvar_take (state->mvar);
		++state->value;
		mvar_put (state->mvar, (void *)1);
	}
	return NULL;
}

int main ()
{
	{
		MVar mvar;
		void *p;
		int e = 0;

		mvar_init (&mvar, NULL);
		assert (mvar_empty (&mvar));
		e = mvar_put (&mvar, (void *)0x1234);
		assert (e == 0);
		assert (!mvar_empty (&mvar));
		p = mvar_read (&mvar);
		assert (p);
		assert (!mvar_empty (&mvar));
		p = mvar_take (&mvar);
		assert (p);
		assert (mvar_empty (&mvar));
		mvar_destroy (&mvar);
	}

	{
		MVar *mvar = mvar_new ((void *)1);
		pthread_t thread1, thread2;
		IncrementorState state;
		state.mvar = mvar;
		state.value = 0;
		pthread_create (&thread1, NULL, incrementor, (void *)&state);
		pthread_create (&thread2, NULL, incrementor, (void *)&state);
		pthread_join (thread1, NULL);
		pthread_join (thread2, NULL);
		mvar_free (mvar);
		assert (state.value == 200000);
	}

	printf ("All tests passed. :)\n");
	return 0;
}
