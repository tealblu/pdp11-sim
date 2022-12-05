SRCS = pdp11-sim.c cache.c
TARFILES = makefile README.md $(SRCS)
CC = gcc
CFLAGS = -g -Wall

default:
	$(CC) $(CFLAGS) $(SRCS) -lm

test: default
	./a.out < test.txt

trace: default
	./a.out -t < test.txt

verbose: default
	./a.out -v < test.txt

tar: clean
	tar -czvf chkarts_project1.tar.gz $(TARFILES)

clean:
	rm -f a.out
	rm -f chkarts_project1.tar.gz
	clear