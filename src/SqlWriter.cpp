#include <sqlite3.h>
#include <sstream>
#include <vector>
#include <stdlib.h>

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
		char* errmsg;
		sqlite3* db;
		int rv;
	
	
	
};

int SqlWriter::open(const char* fname){
	filename = fname;
	rv = sqlite3_open(filename, &db);
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
/*
static int callback(void *not_used, int argc, char** argv, char** col_name) {
	for (int i = 0; i < argc; i++)
		printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
	// col_name seems to be the query we used, not the actual column.
	return 0;
}
*/
static int exists_callback(void* mac_address, int argc, char** argv, char** col_name) {
	int* x = (int*)mac_address;
	*x = atoi(argv[0]);
	return 0;
}


int SqlWriter::write(Position* pos, std::vector<AccessPoint>* ap_list) {
	fprintf(stdout, "Attempting to write to the database...\n");
	
	// Writing to the database will be doine with only one sqlite3_exec() command.
	std::ostringstream q; // queries. All executed at once.

	// INSERT INTO scans VALUES ()
	// First create the query to insert into scans.
	q << "INSERT INTO scans VALUES(";
	q << pos->time << "," << pos->latitude << "," << pos->longitude << ");";	
	
	// To build the one query we'll execute, iterate over all ap's found.
	std::string ap_query;
	for (unsigned int i = 0; i < ap_list->size(); i++) {
		AccessPoint ap = ap_list->at(i);
		ap_query = "SELECT EXISTS(SELECT 1 FROM routers WHERE mac=\"" + std::string(ap.mac) + "\" LIMIT 1);";
		int mac_exists = 0;
		rv = sqlite3_exec(db, ap_query.c_str(), exists_callback, &mac_exists, &errmsg);
		if (rv != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", errmsg);
			sqlite3_free(errmsg);
			return -1;
		}
		
		// We've got a router on the hook. If it's new, insert it.
		if (mac_exists) {
			printf("[SqlWriter]: Old MAC %s already exists...\n", ap.mac);
		} else {
			printf("[SqlWriter]: New MAC %s found! Adding...\n", ap.mac);
			// MAC doesn't exist in the database. Add it.
			q << "INSERT INTO routers VALUES(";
			q << "\"" << ap.mac << "\", \"" << ap.essid << "\",";
			q << 0 << "," << 0 << ");"; // lat and lng estimiates init to 0's.
		}
		
		// Insert the new data entries into the data table.
		q << "INSERT INTO data VALUES(";
		q << pos->time << "," << "\"" << ap.mac << "\"" << ",";
		q << ap.signal << "," << ap.noise << ");";
	}
	
	// Execute the query formed from the stringstream.
	rv = sqlite3_exec(db, q.str().c_str(), NULL, 0, &errmsg);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "sqlite3_exec(): %s\n", errmsg);
		sqlite3_free(errmsg);
		return -1;
	}
	
	// TODO: Return something different?
	return ap_list->size();
}

/*
	TODO: Question > Is there an iterative triangulation algorithm?
	Like, is there an algorithm that I can continuiously feed data 
	(instead of store all of that data and average it or whatever),
	as this would totally negate the use of a database.
	Which, nullifies a bit of my project, but makes it more elegant.
*/





















