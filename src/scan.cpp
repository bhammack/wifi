#include "WifiScanner.cpp"
#include "AccessPoint.cpp"
#include "GpsScanner.cpp"
#include "SqlWriter.cpp"
//#include "Locator.cpp"
#include <vector>

// sudo gpsd /dev/ttyUSB0 -F /var/run/gpsd.sock
int scan(const char* iface, const char* filename) {
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
	sql.open(filename);
	sql.write(hw_addr, &pos, &ap_list);
	sql.close();
	
	// Trilaterate. 
	// TODO: Merge this in with the SqlWriter.
		// Only one class should access the DB for encapsulation purposes.
	//Locator l;
	//l.open(filename);
	//l.locate();
	//l.close();

	return 0;
}

int scanloop(const char* iface, const char* filepath) {
	int rv = scan(iface, filepath);
	while (rv == 0)
		rv = scan(iface, filepath);
	return 0;
}




int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage:\tscan.o <interface> <filepath.db>\n\n");
	}
	char* iface = argv[1];
	char* filepath = argv[2];
	printf("Scanning on interface %s. Writing to file %s\n", iface, filepath);
	scanloop(iface, filepath);
	return 0;
}
