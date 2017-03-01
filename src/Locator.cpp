#include <sqlite3.h>

#include <sstream>
#include <vector>
#include <utility>

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
double fspl_distance(double decibels, double gigahertz) {
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
		double deg2rad(double deg) const { return (deg * PI / 180); };
		double rad2deg(double rad) const { return (rad * 180 / PI); };
	public:
		Point(double lat, double lng) 
		: latitude(lat), longitude(lng) {}
		
		double latitude, longitude;
		// Haversine formula. Distance returned in meters.
		// http://stackoverflow.com/questions/10198985/calculating-the-distance-between-2-latitudes-and-longitudes-that-are-saved-in-a
		double distance_to(const Point& other) const {
			double lat1r, lon1r, lat2r, lon2r, u, v;
			lat1r = deg2rad(latitude);
			lon1r = deg2rad(longitude);
			lat2r = deg2rad(other.latitude);
			lon2r = deg2rad(other.longitude);
			u = sin((lat2r - lat1r)/2);
			v = sin((lon2r - lon1r)/2);
			return 2000.0 * EARTH_RADIUS_KM * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
		};
	public:
		static bool is_valid(double lat, double lng) {
			return (abs(lat) < 90.0 && abs(lng) < 180.0);
		};
};

// A circle. Defined as a point, and a radius.
class Circle {
	public:
		Point center;
		double radius; // meters
	public:
		Circle(double lat, double lng, double rad)
		: center(lat,lng), radius(rad) {}
		
		bool does_intersect(const Circle& other) {
			double d = center.distance_to(other.center);
			if (d > (radius + other.radius)) {
				// no solutions. circles don't touch.
				//double delta = d - (radius + other.radius);
				//fprintf(stderr, "intersects(): circles are %f meters from intersection\n", delta);
				fprintf(stderr, "intersects(): circles do not intersect\n");
				return false;
			}
			if (d < abs(radius - other.radius)) {
				// no solutions. circles coincide.
				fprintf(stderr, "intersects(): circles coincide\n");
				return false;
			}
			if (d == 0) {
				// infinite solutions.
				fprintf(stderr, "ntersects(): infinite solutions\n");
				return false;
			}
			return true;
		}
		
		
		// Assumes two circles intersect. Use does_intersect to check this.
		std::pair<Point,Point> intersects(const Circle& other) {
			double d = center.distance_to(other.center);
			double a = ( pow(radius,2) - pow(other.radius,2) + pow(d,2)) / (2 * d);
			double h = sqrt(pow(radius,2)-pow(a,2));
			double p2lat = center.latitude + a*(other.center.latitude - center.latitude)/d;
			double p2lng = center.longitude + a*(other.center.longitude - center.longitude)/d;
			
			double p3latA = p2lat - h*(other.center.longitude - center.longitude)/d;
			double p3lngA = p2lng - h*(other.center.latitude - center.latitude)/d;
			
			double p3latB = p2lat + h*(other.center.longitude - center.longitude)/d;
			double p3lngB = p2lng + h*(other.center.latitude - center.latitude)/d;
			
			Point left(p3latA, p3lngA);
			Point right(p3latB, p3lngB);
			return std::make_pair(left, right);
		};
		
};


class Locator {
	private:
		const char* filename;
		char* errmsg;
		sqlite3* db;
		int rv;
		void trilaterate(std::string mac);
	public:
		int open(const char* fname);
		void locate();
		void close();
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

void Locator::close() {
	sqlite3_close(db);
	fprintf(stdout, "[Locator]: Closing database file <%s>\n", filename);
}


// Universal callback. Returns the row as a CSV string. SERIALIZATION BOIIII.
/*
static int serialize(void* io, int argc, char** argv, char** col_name) {
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
/*
static int callback(void* io, int argc, char** argv, char** col_name) {
	std::vector< std::vector<std::string> >* table = (std::vector< std::vector<std::string> >*)io;
	std::vector<std::string> row;
	for (int i = 0; i < argc; i++)
		row.push_back(std::string(argv[i]));
	table->push_back(row);
	return 0;
}
*/

// Enum format for the row that I'm returning.
enum {id, lat, lng, lat_err, lng_err, dbm, freq};
static int cb_scans(void* io, int argc, char** argv, char** col_name) {
	std::vector<Circle>* scans = (std::vector<Circle>*)io;
	double latitude = atof(argv[lat]);
	double longitude = atof(argv[lng]);
	double frequency = atof(argv[freq]);
	double decibels = atof(argv[dbm]);
	//
	Circle scan(latitude, longitude, fspl_distance(decibels, frequency));
	scans->push_back(scan);
	return 0;
}


// This method is called for every row returned via a select statement.
static int cb_macs(void* io, int argc, char** argv, char** col_name) {
	std::vector<std::string>* macs = (std::vector<std::string>*)io;
	macs->push_back(std::string(argv[0]));
	return 0;
}




// This here is what makes the whole project chooch. TRILATERATION.
void Locator::trilaterate(std::string mac) {
	std::string select_data = "SELECT scans.id, scans.latitude, scans.longitude, scans.latitude_error, scans.longitude_error, data.signal, routers.frequency FROM scans, data, routers WHERE scans.id = data.scan_id AND routers.mac = data.mac AND data.mac = '" + mac + "';";
	
	std::vector<Circle> scans;
	rv = sqlite3_exec(db, select_data.c_str(), cb_scans, &scans, &errmsg);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "sqlite3_exec(): %s\n", errmsg);
		sqlite3_free(errmsg);
		return;
	}
	
	// Use the area of the intersection of all circles as the error/certainty?
		// Yes, but how to calculate?
	double latitude;
	double longitude;
	unsigned int size = scans.size();
	unsigned int valid = 0;
	
	for (unsigned int i = 0; i < (size-1); i++) {
		Circle a = scans.at(i);
		for (unsigned int j = i+1; j < size; j++) {
			Circle b = scans.at(j);
			if (!a.does_intersect(b)) {
				continue;
			}
			
			// The circles are guarenteed to intersect.
			valid += 1;
			// There's really no reason I should return the pairs of points, 
			// I'm going to end up averaging them anyway...
			std::pair<Point,Point> points = a.intersects(b);
			Point alpha = points.first;
			Point beta = points.second;
			Point midpoint ((alpha.latitude+beta.latitude)/2.0, (alpha.longitude+beta.longitude)/2.0);
			latitude += midpoint.latitude;
			longitude += midpoint.longitude;
		}
	}
	// Average them out, considering there are twice the number of latitudes than the size.
	// This is cause there's two points.
	fprintf(stderr, "[Locator]: Using %d valid intersections between %d scans.\n", valid, size);
	if (valid == 0) {
		fprintf(stderr, "[Locator]: No scans were valid! Trilateration impossible!\n");
		return;
	}
	double est_lat = latitude / valid;
	double est_lng = latitude / valid;

	// Check.
	if (Point::is_valid(est_lat, est_lng)) {
		printf("[Locator]: MAC %s is near lat: %f, lng: %f\n", mac.c_str(), est_lat, est_lng);
	} else {
		fprintf(stderr, "[Locator]: lat: %f, lng %f IS INVALID!!!\n", est_lat, est_lng);
		return;
	}
	
	// Insert the trilaterated value back into the database.
	std::ostringstream q;
	q << "UPDATE routers SET latitude=" << est_lat << ", longitude=" << est_lng;
	q << " WHERE mac='" << mac << "';";
	rv = sqlite3_exec(db, q.str().c_str(), NULL, NULL, &errmsg);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "sqlite3_exec(): %s\n", errmsg);
		fprintf(stderr, "query: %s\n", q.str().c_str());
		sqlite3_free(errmsg);
		return;
	}
}

// Trilateration will consist of averaging all of the intersecting points for a given MAC.
// In reality, I might be able to remove half of the points by only considering the
// midpoint between the two circles (which should result in the same result???)
void Locator::locate() {
	/*
	std::string select_macs = "SELECT mac FROM routers;";
	std::vector<std::string> macs;
	rv = sqlite3_exec(db, select_macs.c_str(), cb_macs, &macs, &errmsg);
	for (unsigned int i = 0; i < macs.size(); i++) {
		trilaterate(macs.at(i));
	}
	*/
	trilaterate("A0:63:91:87:A0:37");
}











