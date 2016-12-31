#include <sqlite3.h>
#include <sstream>

/* Database schema:
-------------------------------------------------------------------------------
scans(
	scan_id 	INTEGER PRIMARY KEY AUTOINCREMENT, 
	time 		INTEGER, 
	latitude 	REAL, 
	longitude 	REAL
);

routers(
	mac 		CHARACTER(17) PRIMARY KEY, 
	bssid 		VARCHAR(32)
); 
// also encryption information once I can get it.
// also latitude and longitude estimates.

data(
	scan_id 	INTEGER, 
	mac 		CHARACTER(17), 
	signal 		INTEGER, 
	noise 		INTEGER,
	FOREIGN KEY(scan_id) REFERENCES scans(scan_id)
);
-------------------------------------------------------------------------------
*/




// Class written to encapsualte dumping out data to a sqlite database instance.
class SqlWriter {
	public:
		int open(const char* filename);
		void close();
		int write(Position* pos, std::vector<AccessPoint>* ap_list);
	private:
		const char* filename;
		sqlite3* db;
	
	
	
};

int SqlWriter::open(const char* fname){
	filename = fname;
	int rv = sqlite3_open(filename, &db);
	if (rv) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return 1;
	}
	// database must have opened successfully.
	
	const char* setup_schema = "CREATE TABLE IF NOT EXISTS scans(			\
		time 		INTEGER PRIMARY KEY NOT NULL, 							\
		latitude 	REAL NOT NULL, 											\
		longitude 	REAL NOT NULL											\
	); CREATE TABLE IF NOT EXISTS routers(									\
		mac 		CHARACTER(17) PRIMARY KEY UNIQUE, 						\
		bssid 		VARCHAR(32),											\
		latitude 	REAL,													\
		longitude 	REAL													\
	); CREATE TABLE IF NOT EXISTS data(										\
		time 		INTEGER, 												\
		mac 		CHARACTER(17), 											\
		signal 		INTEGER, 												\
		noise 		INTEGER,												\
		FOREIGN KEY(time) REFERENCES scans(time)							\
	);";
	rv = sqlite3_exec(db, setup_schema, NULL, 0, NULL);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "Creation of schema failed: %s\n", sqlite3_errmsg(db));
		return 1;
	}
	fprintf(stdout, "[SqlWriter]: %s is ready for use!\n", filename);
	return 0;
};


void SqlWriter::close() {
	sqlite3_close(db);
	fprintf(stdout, "[SqlWriter]: %s has been closed.\n", this->filename);
}


int SqlWriter::write(Position* pos, std::vector<AccessPoint>* ap_list) {
	// INSERT INTO scans VALUES ()
	std::ostringstream ss;
	ss << "INSERT INTO scans VALUES (" << pos->time << ", ";
	ss << pos->latitude << ", " << pos->longitude << ");";
	
	
	
}