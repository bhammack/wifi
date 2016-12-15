#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gps.h>



int main(int argc, char** argv) {
	printf("main()\n");
	gps_enable_debug(1, stdout);
	// WHY IS SEGFAULT OCCURING? I HAVEN'T DONE ANYTHING WRONG?!?!?!
	struct gps_data_t gpsdata;

	//sudo gpsd /dev/ttyUSB0 -F /var/run/gpsd.sock
	int rc;
	if ((rc = gps_open("localhost", "2947", &gpsdata)) == -1) {
    	printf("code: %d, reason: %s\n", rc, gps_errstr(rc));
		return 1;
	}
	
	printf("done!\n");
	return 0;
}


/*
pi@raspberrypi:~/wifi/gps $ gdb gps.OCCURING
Reading symbols from gps.o...(no debugging symbols found)...done.
(gdb) run
Starting program: /home/pi/wifi/gps/gps.o
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/arm-linux-gnueabihf/libthread_db.so.1".
Program received signal SIGSEGV, Segmentation fault.
0x76fa0a28 in gps_sock_open () from /usr/lib/arm-linux-gnueabihf/libgps.so.21
(gdb)
*/

/*
libgps: gps_sock_open(localhost, 2947)
libgps: netlib_connectsock() returns socket on fd 3
Segmentation fault
*/



