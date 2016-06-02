CC = gcc
CFLAGS = -Wall -pedantic -std=c11 -ggdb



PROG = counterstest
OBJS = counterstest.o counters.o
LIBS =

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

counterstest.o: counters.h
counters.o: counters.h

clean:
	rm -f *~
	rm -f *.o
	rm -f $(PROG)
	
#end