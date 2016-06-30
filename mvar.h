#ifndef MVAR_H_479A0E6B_CE77_44EE_9CE9_90F54D2A79F8
#define MVAR_H_479A0E6B_CE77_44EE_9CE9_90F54D2A79F8

typedef struct MVar MVar;

/**** Creating and destroying MVars ****/

/*
 * Allocate and initialize an MVar with a value, or NULL for
 * no value.
 */
MVar *mvar_new (void *value);

/*
 * Deinitialize an MVar and free() it.
 */
void mvar_free (MVar *mvar);

/**** Reading and writing MVars (blocking) ****/

/*
 * Put a new value into an MVar, blocking until the MVar is
 * empty.
 */
void mvar_put (MVar *mvar, void *value);

/*
 * Read the value of an MVar, blocking until the MVar is
 * non-empty.
 */
void *mvar_read (MVar *mvar);

/*
 * Take the value of an MVar, blocking until the MVar is
 * non-empty, and replacing the old value with NULL.
 */
void *mvar_take (MVar *mvar);

/**** Reading and writing MVars (non-blocking) ****/

/*
 * Test whether an MVar is currently empty. If you expect
 * other threads to be concurrently modifying the MVar, you
 * should use mvar_try_read() instead.
 */
int mvar_empty (MVar *mvar);

/*
 * Try to put a new value into an MVar, returning whether
 * the store was successful.
 */
int mvar_try_put (MVar *mvar, void *value);

/*
 * Try to read the value of an MVar, returning its value or
 * NULL if no value was present.
 */
void *mvar_try_read (MVar *mvar);

/*
 * Try to take the value of an MVar, returning its value or
 * NULL if no value was present.
 */
void *mvar_try_take (MVar *mvar);

#endif /* MVAR_H */
