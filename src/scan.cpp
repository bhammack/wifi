#include "WifiScanner.cpp"
#include "AccessPoint.cpp"
#include "GpsScanner.cpp"
#include <vector>

// sudo gpsd /dev/ttyUSB0 -F /var/run/gpsd.sock

int main(int argc, char** argv) {
	WifiScanner wifi;
	GpsScanner gps;
	Position pos;
	AccessPointBuilder builder;
	std::vector<AccessPoint> ap_list;
	
	// Scanning for Wifi hotspots.
	wifi.scan("wlan0");
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
		// error gps scan didn't work.
		return 1;
	}
	
	// Print the results to stdout.
	pos.print();
	for (unsigned int i = 0; i < ap_list.size(); i++)
		ap_list[i].print();
	
	
	
	return 0;
}