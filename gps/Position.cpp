#include <gps.h>
#include <time.h>
// #include date time or whatever.
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>


// Pretty much a straight C++ copy of the gps_fix_t struct, with extra bells and whistles.
class Position {
	public:
		// Metadata
		time_t timestamp;
		int satellites_used;
		int satellites_known;
		// Datapoints
		double latitude;		// latitude (degrees)
		double longitude;		// longiude (degrees)
		double altitude;		// altitude (meters)
		double heading;			// heading (degrees)
		double speed;			// speed (meters/second)
		double climb;			// vertical speed (meters/second)
		// Deviations
		double latitude_dev;
		double longitude_dev;
		double altitude_dev;
		double heading_dev;
		double speed_dev;
		double climb_dev; 
};

class GPSReceiver {
	private:
		struct gps_data_t gpsdata;
		void populate(Position* pos);
		int rv;
	public:
		int fix(Position* pos);
}

int GPSReceiver::fix(Position* pos) {
	memset(&gpsdata, 0, sizeof(gpsdata));
	int scanning = 1;
	if ((rv = gps_open("localhost", "2947", &gpsdata)) < 0) {
		fprintf(stderr, "gps_open(): Error\n");
		return -1;
	}
	gps_stream(&gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);
	
	while (scanning) {
		if (gps_waiting(&gpsdata, 2000000)) {
			if ((rv  = gps_read(&gpsdata)) =< 0) {
				fprintf(stderr, "gps_read(): Data not available...\n");
			} 
			else {
				if ((gpsdata.status == STATUS_FIX) && 
					(gpsdata.fix.mode == MODE_2D || 
					gpsdata.fix.mode == MODE_3D)) {
						scanning = 0;
						populate(pos);
						printf("Got a fix on our position!\n");
				} else {
					printf("Waiting for a location lock...\n");
				}
			}
		} else {
			// Timeout occured. This shouldn't happen!
			return -2;
		}
	}
	gps_stream(&gpsdata, WATCH_DISABLE, NULL);
	gps_close(&gpsdata);
	return 0;
}


void GPSReceiver::populate(Position* pos) {
	// Populate pos with the current values found in gps_data_t.
	pos->time = (time_t) gpsdata.fix.time
	pos->satellites_used = gpsdata.satellites_used;
	pos->satellites_known = gpsdata.satellites_visible;
	//
	pos->latitude = gpsdata.fix.latitude;
	pos->longitude = gpsdata.fix.longitude;
	pos->speed = gpsdata.fix.speed;
	pos->heading = gpsdata.fix.track;
	pos->altitude = gpsdata.fix.altitude;
	pos->climb = gpsdata.fix.climb;
	//
	pos->latitude_dev = gpsdata.fix.epy;
	pos->longitude_dev = gpsdata.fix.epx;
	pos->speed_dev = gpsdata.fix.eps;
	pos->heading_dev = gpsdata.fix.epd;
	pos->altitude_dev = gpsdata.fix.epv;
	pos->climb_dev = gpsdata.fix.epc;
}