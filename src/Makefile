CFLAGS=-Wall

all: scan

scan: scan.cpp
	g++ $(CFLAGS) -o scan.o scan.cpp -lgps -liw -lsqlite3

clean:
	rm -f *.o

debug: debug.cpp
	g++ $(CFLAGS) -o debug.o debug.cpp -lgps -liw -lsqlite3
