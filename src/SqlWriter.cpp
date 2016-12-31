#include <sqlite3.h>

/* Database schema:
scans(scan_id* INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER, REAL latitude, REAL longitude);
routers(mac TEXT, bssid TEXT, encryption TEXT, cypher TEXT, ...);
data(scan_id* INTEGER FOREIGN KEY)



*/




// Class written to encapsualte dumping out data to a sqlite database instance.
class SqlWriter {
	public:
		int open();
	private:
		sqlite3* db;
	
	
	
};

int SqlWriter::open(const char* filename) {
	int rv = sqlite3_open(filename, &db);
	if (rv) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	// database must have opened successfully.
	
	char* setup_schema = "";
	char* make_tables = "CREATE TABLE "
	
	
	
	return sqlite3_open(filename, &db);
};