#
# Makefile for chat
#
# LIBS	= -lsocket -lnsl
LIBS	= -pthread
CFLAGS	= -g -ggdb -std=gnu99

all:	threads

threads:	chatServer3 chatClient3 directoryServer3

client3.o server3.o directory3.o: inet.h

chatServer3: server3.o
	gcc $(CFLAGS) -o $@ chatServer3.c $(LIBS)

chatClient3: client3.o
	gcc $(CFLAGS) -o $@ chatClient3.c $(LIBS)

directoryServer3: directory3.o
	gcc $(CFLAGS) -o $@ directoryServer3.c $(LIBS)

#
# Clean up the mess we made
#
clean:
	rm *.o \
	chatServer3 chatClient3 directoryServer3 2>/dev/null
