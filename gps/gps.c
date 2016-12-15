#include <gps.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
	struct gps_data_t gpsdata;
	int scanning = 1;
	int rv;
	if (gps_open("localhost", "2947", &gpsdata) < 0) {
		perror("gps_open()");
		return 1;
	}
	gps_stream(&gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);

	while (scanning) {
		if (gps_waiting(&gpsdata, 2000000)) {
			rv = gps_read(&gpsdata);
			if (rv < 0) {
				perror("gps_read()");
			} else if (rv == 0) {
				printf("no data in socket...\n");
			} else {
				if ((gpsdata.status == STATUS_FIX) && 
					(gpsdata.fix.mode == MODE_2D || gpsdata.fix.mode == MODE_3D)) {
					// We've got a valid fix on our location!
					printf("lat: %f | lng: %f | sats_used: %d | stas_vis: %d\n", 
					gpsdata.fix.latitude, gpsdata.fix.longitude,
					gpsdata.satellites_used, gpsdata.satellites_visible);
				} else {
					printf("Getting a fix on the position...\n");
				}
			}
		} else {
			scanning = 0;
		}
	}

	gps_stream(&gpsdata, WATCH_DISABLE, NULL);
	gps_close(&gpsdata);

	return 0;
}