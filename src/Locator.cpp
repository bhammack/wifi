#include <sqlite3.h>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <cmath>

#define PI 3.14159265358979323846
#define EARTH_RADIUS_KM 6371.0

// String split functions. Might not need them now
/*
// http://stackoverflow.com/questions/236129/split-a-string-in-c
template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}
*/



// http://rvmiller.com/2013/05/part-1-wifi-based-trilateration-on-android/
// Based on the equation for free-space path loss; solved for distance in meters.
double radius(double decibels, double gigahertz) {
	double megahertz = gigahertz * 1000.0;
	const double c = 27.55; // magic constant... TODO: explain this...
	double exp = (c - (20*log10(megahertz)) + abs(decibels)) / 20.0;
	return pow(10.0, exp);
}

// http://math.stackexchange.com/questions/221881/the-intersection-of-n-disks-circles

// A class for a 2-Dimensional point on Earth surface. A coordinate position.
class Point {	
	private:
		// TODO: Add the def for PI and EARTH_RADIUS_KM here.
		double deg2rad(double deg) { return (deg * PI / 180); };
		double rad2deg(double rad) { return (rad * 180 / PI); };
	public:
		Point(double lat, double lng) {
			latitude = lat;
			longitude = lng;
		};
		double latitude;
		double longitude;
		// Haversine formula. Distance returned in meters.
		// http://stackoverflow.com/questions/10198985/calculating-the-distance-between-2-latitudes-and-longitudes-that-are-saved-in-a
		double distance_to(Point* other) {
			double lat1r, lon1r, lat2r, lon2r, u, v;
			lat1r = deg2rad(latitude);
			lon1r = deg2rad(longitude);
			lat2r = deg2rad(other->latitude);
			lon2r = deg2rad(other->longitude);
			u = sin((lat2r - lat1r)/2);
			v = sin((lon2r - lon1r)/2);
			return 2000.0 * EARTH_RADIUS_KM * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
		};
};

// A circle. Defined as a point, and a radius.
class Circle {
	private:
		Point center;
		double radius; // meters
	public:
		Circle(double lat, double lng, double rad)
		: center(lat,lng) {
			radius = rad;
		};
};









class Locator {
	private:
		const char* filename;
		char* errmsg;
		sqlite3* db;
		int rv;
		void locate_mac(std::string mac);
	public:
		int open(const char* fname);
		void locate();
};

int Locator::open(const char* fname) {
	filename = fname;
	rv = sqlite3_open(filename, &db);
	if (rv) {
		fprintf(stderr, "open(): can't open database: %s\n", sqlite3_errmsg(db));
		return 1;
	}
	return 0;
};


// Universal callback. Returns the row as a CSV string. SERIALIZATION BOIIII.
/*
static int callback(void* io, int argc, char** argv, char** col_name) {
	std::vector<std::string>* row = (std::vector<std::string>*)io;
	std::ostringstream buffer;
	for (int i = 0; i < argc; i++) {
		buffer << "\"" << argv[i] << "\"";
		if (i != (argc - 1)) {
			buffer << ",";
		}
	}
	row->push_back(buffer.str());
	return 0;
}
*/

// Another universal callback. Returns the query as a table of vector vector.
static int callback2(void* io, int argc, char** argv, char** col_name) {
	std::vector< std::vector<std::string> >* table = (std::vector< std::vector<std::string> >*)io;
	std::vector<std::string> row;
	for (int i = 0; i < argc; i++)
		row.push_back(std::string(argv[i]));
	table->push_back(row);
	return 0;
}




// This method is called for every row returned via a select statement. Huh...
// Can I chain these callbacks such that I keep calling back until I get what I need?
// Eh, I probably shouldn't do it like that.
static int cb_macs(void* macs_vector, int argc, char** argv, char** col_name) {
	std::vector<std::string>* macs = (std::vector<std::string>*)macs_vector;
	macs->push_back(std::string(argv[0]));
	return 0;
}




// This here is what makes the whole project chooch.
void Locator::locate_mac(std::string mac) {
	//std::string select_data = "SELECT scans.id, scans.latitude, scans.longitude, \
	scans.latitude_error, scans.longitude_error, data.signal, data.noise, \
	data.quality FROM scans, data WHERE scans.id = data.scan_id AND data.mac = '" + mac + "';";
	
	std::string select_data = "SELECT scans.id, scans.latitude, scans.longitude, scans.latitude_error, scans.longitude_error, data.signal, routers.frequency FROM scans, data, routers WHERE scans.id = data.scan_id AND routers.mac = data.mac AND data.mac = '" + mac + "';";
	
	
	std::vector< std::vector<std::string> > table;
	rv = sqlite3_exec(db, select_data.c_str(), callback2, &table, &errmsg);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "sqlite3_exec(): %s\n", errmsg);
		sqlite3_free(errmsg);
		return;
	}
	
	//double latitude;
	//double longitude;
	//double radius;
	
	for (unsigned int i = 0; i < table.size(); i++) {
		std::vector<std::string>* row = &table.at(i);
		for (unsigned int j = 0; j < row->size(); j++) {
			printf("%s\n", row->at(j).c_str());
			
			
		}
	}
	
	
	
	
	
}


void Locator::locate() {
	std::string select_macs = "SELECT mac FROM routers;";
	std::vector<std::string> macs;
	rv = sqlite3_exec(db, select_macs.c_str(), cb_macs, &macs, &errmsg);
	for (unsigned int i = 0; i < macs.size(); i++) {
		locate_mac(macs.at(i));
	}
}











