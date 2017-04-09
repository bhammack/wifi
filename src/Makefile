CFLAGS=-Wall

all: scan locate

scan: scan.cpp
	g++ $(CFLAGS) -o scan.o scan.cpp -lgps -liw -lsqlite3

clean:
	rm -f *.o

locate: locate.cpp
	g++ $(CFLAGS) -o locate.o locate.cpp -lsqlite3
