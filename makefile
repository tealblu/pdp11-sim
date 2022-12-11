SRCS = pdp11-sim.c cache.c
TARFILES = makefile README.md $(SRCS) a.out
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

tar:
	tar -czvf ckharts_project2.tar.gz $(TARFILES)

clean:
	rm -f a.out
	rm -f ckharts_project2.tar.gz
	clear