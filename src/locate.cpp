#include <sqlite3.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	sqlite3* db;
	int rv = sqlite3_open("test.db", &db);
	/* Triangulation methods:
	1. Weighted average of coordinates. Use the quality as the
	   weighted percentage and multiply it by both lat and lng.
	2. Triangulation via the earthquake method. Overlapping circles.
	3. By other means. Not sure what these would be.

	*/
}
