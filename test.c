#include <mvar-internal.h>
#include <stdio.h>

int main ()
{
	MVar mvar;
	void *p;

	printf ("init\n");
	mvar_init (&mvar, NULL);

	printf ("put\n");
	mvar_put (&mvar, (void *)0x1234);

	printf ("read\n");
	p = mvar_read (&mvar);
	printf ("\t%p\n", p);

	printf ("take\n");
	p = mvar_take (&mvar);
	printf ("\t%p\n", p);

	printf ("empty? %s\n", mvar_empty (&mvar) ? "true" : "false");

	printf ("destroy\n");
	mvar_destroy (&mvar);
}
