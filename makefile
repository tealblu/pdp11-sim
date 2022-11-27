SRCS = pdp11-sim.c
TARFILES = makefile README.md $(SRCS)
CC = gcc
CFLAGS = -g -Wall

default:
	$(CC) $(CFLAGS) $(SRCS) -lm

test: a.out test.txt
	./a.out < test.txt

trace:
	./a.out -t < test.txt

verbose:
	./a.out -v < test.txt

tar:
	tar -czvf chkarts_project1.tar.gz $(TARFILES)

clean:
	rm -f a.out
	rm -f chkarts_project1.tar.gz
	clear