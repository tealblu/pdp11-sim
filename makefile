SRCS = pdp11-sim.c
TARFILES = makefile README $(SRCS)
CC = gcc
CFLAGS = -g -Wall

default:
	$(CC) $(CFLAGS) $(SRCS) -lm

tar:
	tar -czvf $(TARFILES) chkarts_project1.tar.gz