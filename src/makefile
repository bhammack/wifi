CFLAGS=-Wall

all: scan

scan: scan.cpp
	g++ $(CFLAGS) -o scan.o scan.cpp -lgps -liw

clean:
	rm -f *.o
