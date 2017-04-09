#include <sqlite3.h>

#include <sstream>
#include <vector>
#include <utility>

#include <stdlib.h>
#include <cmath>

#include <sstream>
#include <iostream>
#include <fstream>

#include <string>
#include <algorithm>
#include "KmlDrawer.cpp"

#define PI 3.14159265358979323846
#define EARTH_RADIUS_KM 6371.0
#define NO_WRITE 1

// Degrees to radians.
//double deg2rad(double deg) { return (deg * PI / 180); };
//double rad2deg(double rad) { return (rad * 180 / PI); };


// String split functions. Might not need them now
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
// Universal callback. Returns the row as a CSV string. SERIALIZATION BOIIII.
static int serialize(void* io, int argc, char** argv, char** col_name) {
	std::vector<std::string>* row = (std::vector<std::string>*)io;
	std::ostringstream buffer;
	for (int i = 0; i < argc; i++) {
		buffer << argv[i];
		if (i != (argc - 1)) {
			buffer << "|";
		}
	}
	row->push_back(buffer.str());
	return 0;
}
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
	public:
		Point(double lat, double lng) : latitude(lat), longitude(lng) {}
		
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
			return 2000.0 * EARTH_RADIUS_KM * asin(sqrt(  u*u + cos(lat1r)*cos(lat2r)*v*v  ));
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
		double weight; // rssi (quality)
		int time;
		int id;
	public:
		Circle(int scan_id, int timestamp, double lat, double lng, double rad, double rssi)
		: center(lat,lng), radius(rad*(1.0/rssi)), weight(rssi), time(timestamp), id(scan_id) {}
		
		bool does_intersect(const Circle& other) {
			double d = center.distance_to(other.center);
			if (d > (radius + other.radius)) {
				// no solutions. circles don't touch.
				//double delta = d - (radius + other.radius);
				//fprintf(stderr, "intersects(): circles are %f meters from intersection\n", delta);
				//fprintf(stderr, "intersects(): circles do not intersect\n");
				return false;
			}
			if (d < abs(radius - other.radius)) {
				// no solutions. circles coincide.
				//fprintf(stderr, "intersects(): circles coincide\n");
				return false;
			}
			if (d == 0) {
				// infinite solutions.
				//fprintf(stderr, "intersects(): infinite solutions\n");
				return false;
			}
			if (center.latitude == other.center.latitude && center.longitude == other.center.longitude) {
				return false;
			}
			return true;
		}
		
		// Assumes two circles intersect. Use does_intersect to check this.
		std::pair<Point,Point> intersects(const Circle& other) {
			double d = center.distance_to(other.center);
			double a = ( pow(radius,2) - pow(other.radius,2) + pow(d,2)) / (2 * d);
			// h = nan.
			//double h = sqrt(pow(radius,2)-pow(a,2));
			// wow I'm dumb...
			double h = sqrt(abs(pow(radius,2)-pow(a,2)));
			double p2lat = center.latitude + a*(other.center.latitude - center.latitude)/d;
			double p2lng = center.longitude + a*(other.center.longitude - center.longitude)/d;
			
			double p3latA = p2lat - h*(other.center.longitude - center.longitude)/d;
			double p3lngA = p2lng - h*(other.center.latitude - center.latitude)/d;
			
			double p3latB = p2lat + h*(other.center.longitude - center.longitude)/d;
			double p3lngB = p2lng + h*(other.center.latitude - center.latitude)/d;
			//printf("d=%f, a=%f, h=%f\n, radius=%f, other.radius=%f\n", d, a, h, radius, other.radius);
			Point left(p3latA, p3lngA);
			Point right(p3latB, p3lngB);
			return std::make_pair(left, right);
		};
		
		bool contains(const Point& p) {
			if (p.distance_to(center) > radius) {
				return false;
			} else {
				return true;
			}
		}
		
};


class Locator {
	private:
		KmlDrawer kml;
		const char* filename;
		char* errmsg;
		sqlite3* db;
		int rv;
	public:
		std::vector<Circle> get_scans(std::string mac, int qual);
		std::pair<double,double> trilaterate(std::string mac, bool draw_scans);
		int open(const char* fname);
		void close();
		void write_kml();
		void write_meta();
		void trilaterate_all();
};

int Locator::open(const char* fname) {
	filename = fname;
	rv = sqlite3_open(filename, &db);
	if (rv) {
		fprintf(stderr, "open(): can't open database: %s\n", sqlite3_errmsg(db));
		return 1;
	}
	fprintf(stdout, "[Locator]: Opening database file <%s>\n", filename);
	kml.open(std::string(std::string(fname) + ".kml").c_str());
	return 0;
};

void Locator::close() {
	sqlite3_close(db);
	kml.close();
	fprintf(stdout, "[Locator]: Closing database file <%s>\n", filename);
}

void Locator::write_kml() {
	/*
	std::string select_routers = "SELECT * FROM routers;";
	std::vector<std::string> routers;
	rv = sqlite3_exec(db, select_routers.c_str(), serialize, &routers, &errmsg);
	for (unsigned int i = 0; i < routers.size(); i++) {
		std::vector<std::string> data = split(routers.at(i), '|');
		std::string mac = data.at(0);
		std::string ssid = data.at(1);
		std::string freq = data.at(2);
		std::string channel = data.at(3);
		std::pair<double,double> pos = trilaterate(mac);
		// draw point
	}
	*/
}



// Enum format for the row that I'm returning.
enum {id, ts, lat, lng, dbm, freq, rssi};
static int cb_scans(void* io, int argc, char** argv, char** col_name) {
	std::vector<Circle>* scans = (std::vector<Circle>*)io;
	double latitude = atof(argv[lat]);
	double longitude = atof(argv[lng]);
	double frequency = atof(argv[freq]);
	double decibels = atof(argv[dbm]);
	double quality = atof(argv[rssi]);
	int time = atoi(argv[ts]);
	int scan_id = atoi(argv[id]);
	//
	Circle scan(scan_id, time, latitude, longitude, fspl_distance(decibels, frequency), quality);
	scans->push_back(scan);
	return 0;
}


std::vector<Circle> Locator::get_scans(std::string mac, int qual_cutoff=70) {
	std::vector<Circle> result;
	//std::string cutoff = std::string(qual_cutoff);
	std::stringstream ss;
	ss << qual_cutoff;
	
	std::string select_data = "SELECT scans.id, scans.time, scans.latitude, scans.longitude, data.signal, routers.frequency, data.quality FROM scans, data, routers WHERE scans.id = data.scan_id AND data.quality*100 >= " + ss.str() + " AND routers.mac = data.mac AND data.mac = '" + mac + "';";
	rv = sqlite3_exec(db, select_data.c_str(), cb_scans, &result, &errmsg);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "sqlite3_exec(): %s\n", errmsg);
		fprintf(stderr, "query: %s\n", select_data.c_str());
		sqlite3_free(errmsg);
	}
	return result;
}

// Achievement unlocked: --computer science graduate--
	// You wrote an O(n^3) algorithm and justified using it!
	
// This here is what makes the whole project chooch. TRILATERATION.
std::pair<double,double> Locator::trilaterate(std::string mac, bool draw_scans = false) {
	printf("[Locator]: Using MAC address: %s\n", mac.c_str());
	
	// Open the KML file to write to.
	std::string macname = std::string(mac);
	std::replace(macname.begin(), macname.end(), ':', '-');
	std::string kmlname = macname + ".kml";
	//kml.open(kmlname.c_str());

	
	std::pair<double,double> pos = std::make_pair(0.0,0.0);
	
	// Gets all scans with quality greater than or equal to 70
	std::vector<Circle> scans = get_scans(mac, 70);
	if (scans.size() == 0) {
		printf("[Locator]: Not enough data points!\n");
		return pos;
	}
	// Use the area of the intersection of all circles as the error/certainty?
		// Yes, but how to calculate?
	
	
	// Draw every circle on the map...
	if (draw_scans) {
		for (unsigned int x = 0; x < scans.size(); x++) {
			Circle c = scans.at(x);
			std::ostringstream temp;
			temp << x;
			std::string name = temp.str();
			temp.str(""); temp.clear();
			temp << "radius: " << c.radius << " quality: " << c.weight;
			std::string desc = temp.str();
			kml.draw_circle(name, desc, c.center.latitude, c.center.longitude, c.radius);
		}
	}
	
	
	double latitude = 0.0;
	double quals = 0;
	double longitude = 0.0;
	double radius = 0;
	int radius_count = 0;
	
	int valid_intersections = 0;
	
	// For every pair of circles (that intersect)...
	std::vector< std::pair<Point,Point> > vertex_pairs;
	for (unsigned int i = 0; i < (scans.size()-1); i++) {
		Circle a = scans.at(i);
		for (unsigned int j = i+1; j < scans.size(); j++) {
			Circle b = scans.at(j);
			if (!a.does_intersect(b)) continue;
			// The circles intersect.
			valid_intersections++;
			
			//std::pair<Point,Point> intersection = a.intersects(b);
			quals += a.weight + b.weight;
			latitude += (a.weight*a.center.latitude) + (b.weight*b.center.latitude);
			longitude += (a.weight*a.center.longitude) + (b.weight*b.center.longitude);
			radius += (a.radius + b.radius);
			radius_count += 2;
			
			
			
			
			
			//vertex_pairs.push_back(a.intersects(b));
		}
	}
	
	
	/*
	
	// For every pair of verticies...
		// Keep the vertex that's within more circles than the other.
	std::vector<Point> polygon;
	for (unsigned int i = 0; i < vertex_pairs.size(); i++) {
		Point alpha = vertex_pairs.at(i).first;
		Point beta = vertex_pairs.at(i).second;
		unsigned int alpha_count = 0;
		unsigned int beta_count = 0;
		for (unsigned int j = 0; j < scans.size(); j++) {
			if (scans.at(j).contains(alpha))
				alpha_count += 1;
			if (scans.at(j).contains(beta))
				beta_count += 1;
		}
		if (alpha_count > beta_count)
			polygon.push_back(alpha);
		else
			polygon.push_back(beta);
	}
	
	// Return the average intersection of all the polygon's points.
	double latitude = 0.0;
	double longitude = 0.0;
	for (unsigned int i = 0; i < polygon.size(); i++) {
		//printf("%lf, %lf\n", polygon.at(i).longitude, polygon.at(i).latitude);
		latitude += polygon.at(i).latitude;
		longitude += polygon.at(i).longitude;
	}
	latitude /= polygon.size();
	longitude /= polygon.size();
	pos = std::make_pair(latitude, longitude);
	*/
	
	latitude /= quals;
	longitude /= quals;
	radius /= radius_count;
	
	printf("mac: %s is near %lf, %lf, radius of %lf\n", mac.c_str(), latitude, longitude, radius);
	kml.draw_point(mac, "trilateration point", latitude, longitude);
	//kml.close();
	return pos;
}

void Locator::trilaterate_all() {
    std::vector<std::string> rows;
    std::string select_data = "SELECT routers.mac FROM routers;";
	rv = sqlite3_exec(db, select_data.c_str(), serialize, &rows, &errmsg);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "sqlite3_exec(): %s\n", errmsg);
		fprintf(stderr, "query: %s\n", select_data.c_str());
		sqlite3_free(errmsg);
	}
	for (unsigned int i = 0; i < rows.size(); i++) {
		std::vector<std::string> values = split(rows.at(i), '|');
		trilaterate(values.at(0), false);
	}
    
}

