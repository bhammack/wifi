#include <sstream>
#include <iostream>
#include <fstream>

#include <cmath>
#define PI 3.14159265358979323846

double deg2rad(double deg) { return (deg * PI / 180); };
double rad2deg(double rad) { return (rad * 180 / PI); };


class KmlDrawer {
	private:
		std::ofstream kml;
	public:
		void open(const char* filename);
		void close();
		void draw_circle(std::string name, std::string desc, double latitude, double longitude, double radius);
		void draw_point(std::string name, std::string desc, double latitude, double longitude);
		void style();
};

// Closes the kml file. Writing is complete.
void KmlDrawer::close() {
	kml << "</Document>\n</kml>";
	kml.close();
}

// Opens the kml file for writing.
void KmlDrawer::open(const char* filename) {
	kml.open(filename, std::ios::out | std::ios::trunc);
	kml.precision(9);
	kml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	kml << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n";
	kml << "<Document>\n";
	style();
}

// Draws a circle around a specified coordinate.
// https://knackforge.com/blog/narendran/kml-google-map-circle-generator-script-php
void KmlDrawer::draw_circle(std::string name, std::string desc, double latitude, double longitude, double radius) {
	kml << "<Placemark>\n<name>" << name << "</name>\n<description>" << desc << "</description>\n";
	kml << "<styleUrl>#style0</styleUrl>\n";
	kml << "<Polygon>\n<outerBoundaryIs>\n<LinearRing>\n<coordinates>\n";
	double lat1 = deg2rad(latitude);
	double long1 = deg2rad(longitude);
	double d_rad = radius/6378137;
	for (int i = 0; i <= 360; i+=3) {
		double radial = deg2rad(i);
		double lat_rad = asin(sin(lat1)*cos(d_rad) + cos(lat1)*sin(d_rad)*cos(radial));
		double dlon_rad = atan2(sin(radial)*sin(d_rad)*cos(lat1), cos(d_rad)-sin(lat1)*sin(lat_rad));
		double lon_rad = fmod((long1+dlon_rad+PI), 2*PI) - PI;
		kml << rad2deg(lon_rad) << "," << rad2deg(lat_rad) << ",0\n";
	}
	kml << "</coordinates>\n</LinearRing>\n</outerBoundaryIs>\n</Polygon>\n</Placemark>\n";
}

// Draws a place marker on the map.
void KmlDrawer::draw_point(std::string name, std::string desc, double latitude, double longitude) {
	kml << "<Placemark>\n";
	kml << "<name>" << name << "</name>\n";
	kml << "<description>" << desc << "</description>\n";
	kml << "<Point>\n";
	kml << "<coordinates>" << longitude << "," << latitude << "</coordinates>\n";
	kml << "</Point>\n</Placemark>\n";
}

// Sets the document styles. This is where I can add custom colors later.
void KmlDrawer::style() {
	kml << "<Style id=\"style0\"><PolyStyle><fill>0</fill><outline>1</outline></PolyStyle><LineStyle><color>ff0000ff</color><width>1</width></LineStyle></Style>\n";
}