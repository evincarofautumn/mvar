debug_flags := -O0 -g
warning_flags := -Wall

test : test.c libmvar.a Makefile
	$(CC) $< -I. -L. -lmvar -o $@ $(debug_flags) $(warning_flags)

.PHONY : clean
clean :
	-rm -rf mvar.o libmvar.a test

libmvar.a : mvar.o Makefile
	ar qc $@ $<

mvar.o : mvar.c Makefile
	$(CC) -c $< -I. -o $@ $(debug_flags) $(warning_flags)

mvar.c : mvar.h mvar-internal.h
