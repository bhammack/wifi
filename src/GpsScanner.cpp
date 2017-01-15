#include <gps.h>
#include <time.h>
#include <cmath>
#define TIMEOUT 3000000 // microseconds

// Pretty much a straight C++ copy of the gps_fix_t struct, with extra bells and whistles.
class Position {
	public:
		// Metadata
		time_t time;
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
		void print();
};

void Position::print() {
	printf("===================================\n");
	printf("Timestamp:\t\t%ld\n", time);
	printf("Satellites:\t\t%d/%d\n", satellites_used, satellites_known);
	printf("Latitude:\t\t%f\n", latitude);
	printf("Longitude:\t\t%f\n", longitude);
}




class GpsScanner {
	private:
		struct gps_data_t gpsdata;
		void populate(Position* pos);
		int rv;
	public:
		int scan(Position* pos);
};

int GpsScanner::scan(Position* pos) {
	memset(&gpsdata, 0, sizeof(gpsdata));
	int scanning = 1;
	if ((rv = gps_open("localhost", "2947", &gpsdata)) < 0) {
		fprintf(stderr, "gps_open(): Error\n");
		fprintf(stderr, "hint: sudo gpsd /dev/ttyUSB0 -F /var/run/gpsd.sock\n");
		return -1;
	}
	gps_stream(&gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);
	char status = 'n';
	while (scanning) {
		if (gps_waiting(&gpsdata, TIMEOUT)) {
			if ((rv  = gps_read(&gpsdata)) <= 0) {
				fprintf(stderr, "gps_read(): Data not available...\n");
				fprintf(stderr, "try moving somewhere outdoors?\n");
			} else {
				// We've got gpsdata. Verify that it's a fix on our location.
				if (
				(gpsdata.status == STATUS_FIX) && 
				(gpsdata.fix.mode == MODE_2D || gpsdata.fix.mode == MODE_3D) &&
				(!std::isnan(gpsdata.fix.latitude) && !std::isnan(gpsdata.fix.longitude))
					) {
						scanning = 0;
						populate(pos);
				} else {
					// The gpsdata doesn't contain anything meaningful, yet.
					if (status == 'n') {
						printf("[GpsScanner]: Getting a fix on the current position...\n");
						status = 'u';
					}
				}
			}
		} else {
			// Timeout occured. This shouldn't happen!
			fprintf(stderr, "gps_read(): Timeout occured\n");
			return -2;
		}
	}
	gps_stream(&gpsdata, WATCH_DISABLE, NULL);
	gps_close(&gpsdata);
	return 0;
}


void GpsScanner::populate(Position* pos) {
	// Populate pos with the current values found in gps_data_t.
	pos->time = (time_t) gpsdata.fix.time;
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
