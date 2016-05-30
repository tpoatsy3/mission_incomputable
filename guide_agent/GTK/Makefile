# Makefile for 'gtkforGuideAgent' data type
# 
# Topaz, May 2016

PROG = gagui
OBJS = gagui.o

CC = gcc
CFLAGS = -Wall -pedantic -std=c99 -ggdb
GFLAGS =`pkg-config --cflags --libs gtk+-3.0`
MAKE = make

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(GFLAGS) -o $@

guideAgentGTK.o:

clean: 
	rm -f *~
	rm -f *.o
	rm -f $(PROG)
