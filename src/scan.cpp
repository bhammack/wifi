#include "WifiScanner.cpp"
#include "AccessPoint.cpp"
#include "GpsScanner.cpp"
#include "SqlWriter.cpp"
#include "Locator.cpp"
#include <vector>


// sudo gpsd /dev/ttyUSB0 -F /var/run/gpsd.sock
int scan(const char* iface) {
	WifiScanner wifi;
	GpsScanner gps;
	Position pos;
	AccessPointBuilder builder;
	std::vector<AccessPoint> ap_list;

	// Scanning for Wifi hotspots.
	char* hw_addr = wifi.get_iface_addr(iface);
	wifi.scan(iface);
	builder.set_range(wifi.get_range());
	iw_event* iwe;
	while ((iwe = wifi.get_event()) != NULL) {
		builder.handle(iwe);
		if (builder.is_built()) {
			ap_list.push_back(builder.get_ap());
			builder.clear();
		}
	}
	// Poll gpsd for the current position.
	int rv;
	if ((rv = gps.scan(&pos)) < 0) {
		// error gps scan didn't work. NOT GOOD!
		return 1;
	}

	// Create the database writer to output collected data.
	SqlWriter sql;
	sql.open("test.db");
	sql.write(hw_addr, &pos, &ap_list);
	sql.close();

	return 0;
}

int scanloop() {
	int rv = scan("wlan1");
	while (rv == 0)
		rv = scan("wlan1");
	return 0;
}

int main(int argc, char** argv) {
	//printf("scan() returned %d\n", scan("wlan1"));
	//return scanloop();
	Locator l;
	l.open("test.db");
	l.locate();
	return 0;
}
