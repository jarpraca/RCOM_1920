objects= receiver sender
all:$(objects)
CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -D_DEFAULT_SOURCE 
DEPS = 

%.objects: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

receiver: receiver.c
	$(CC) -o receiver receiver.c protocol.c
sender: sender.c
	$(CC) -o sender sender.c protocol.c


.PHONY: all clean

clean:
	rm -f $(TARGETS) *.o *.d *.i *.s