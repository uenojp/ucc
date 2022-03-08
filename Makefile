CC     = gcc
CFLAGS = -std=c11 -g -static -Wall

PROGRAM = ucc
SRCS    = $(wildcard *.c)
OBJS    = $(SRCS:.c=.o)

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $(OBJS)

%.o: %.c $(PROGRAM).h
	$(CC) $(CFLAGS) -o $@ -c $<

test: $(PROGRAM)
	./test.sh

clean:
	rm -f $(PROGRAM) $(OBJS) *~ tmp*

.PHONY: test clean
