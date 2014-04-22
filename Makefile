# automatically generated makefile by ./create-Makefile.pl

CFLAGS=-Wall -g -O2 -std=gnu99 -I./include -L./lib 

LIBS=lib/genlib.a 

SEMFLAGS=-lpthread -lrt


all: fclient fserver

clean:
	rm -f lib/genlib.a  fclient fserver lib/tcp-client.o lib/tcp-server.o lib/clientlib.o lib/genlib.o

fclient: fclient.c lib/genlib.a include/genlib.h
	gcc $(CFLAGS) fclient.c $(LIBS) -o fclient $(SEMFLAGS)

fserver: fserver.c lib/genlib.a include/genlib.h
	gcc $(CFLAGS) fserver.c $(LIBS) -o fserver $(SEMFLAGS)

lib/tcp-client.o: lib/tcp-client.c
	gcc -c $(CFLAGS) lib/tcp-client.c -o lib/tcp-client.o $(SEMFLAGS)

lib/tcp-server.o: lib/tcp-server.c
	gcc -c $(CFLAGS) lib/tcp-server.c -o lib/tcp-server.o $(SEMFLAGS)

lib/clientlib.o: lib/clientlib.c
	gcc -c $(CFLAGS) lib/clientlib.c -o lib/clientlib.o $(SEMFLAGS)

lib/genlib.o: lib/genlib.c
	gcc -c $(CFLAGS) lib/genlib.c -o lib/genlib.o $(SEMFLAGS)



lib/genlib.a: lib/tcp-client.o lib/tcp-server.o lib/clientlib.o lib/genlib.o
	ar crs lib/genlib.a lib/tcp-client.o lib/tcp-server.o lib/clientlib.o lib/genlib.o
