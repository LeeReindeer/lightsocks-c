CFLAGS=-Wall -g
EVENTLIB?=-levent
BINDIR?=./bin
SERVERDIR?=./server
CLIENTDIR?=./client
LIB?=./lib

$(shell mkdir -p $(BINDIR))


all: lib lightclient lightserver

lib: base64 log password securesocket parson

lightclient:
		gcc -o $(BINDIR)/client $(CLIENTDIR)/client.c $(BINDIR)/base64.o $(BINDIR)/log.o $(BINDIR)/password.o $(BINDIR)/securesocket.o $(BINDIR)/parson.o $(EVENTLIB)

lightserver:
		gcc -o $(BINDIR)/server $(SERVERDIR)/server.c $(BINDIR)/base64.o $(BINDIR)/log.o $(BINDIR)/password.o $(BINDIR)/securesocket.o $(BINDIR)/parson.o $(EVENTLIB)

# libs
base64:
		gcc -c $(LIB)/base64.c -o $(BINDIR)/base64.o

log: 
		gcc -c $(LIB)/log.c -o $(BINDIR)/log.o -DLOG_USE_COLOR

password:
		gcc -c $(LIB)/password.c -o $(BINDIR)/password.o

securesocket:
		gcc -c $(LIB)/securesocket.c -o $(BINDIR)/securesocket.o

parson:
		gcc -c $(LIB)/parson.c -o $(BINDIR)/parson.o

clean:
		rm -rf bin