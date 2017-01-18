#include <sqlite3.h>
#include <sstream>
#include <vector>
#include <stdlib.h>

/* Database schema:
-------------------------------------------------------------------------------
	see below...
-------------------------------------------------------------------------------
*/

// TODO: Make a querybuilder class. It modifies from ostringstream, but is
// customized to produce SQLite3 queries. 
// for instance, q << "\"" << some_string << "\"" << "," is tedious.


// Class written to encapsualte dumping out data to a sqlite database instance.
class SqlWriter {
	public:
		int open(const char* filename);
		void close();
		int write(char* hw_addr, Position* pos, std::vector<AccessPoint>* ap_list);
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
		fprintf(stderr, "open(): can't open database: %s\n", sqlite3_errmsg(db));
		return 1;
	}
	// database must have opened successfully.

	const char* schema = "\
	CREATE TABLE IF NOT EXISTS scans(\
		id					INTEGER PRIMARY KEY,\
		time 				INTEGER NOT NULL,\
		iface				CHARACTER(17) NOT NULL,\
		latitude 			REAL NOT NULL,\
		longitude 			REAL NOT NULL,\
		latitude_error 		REAL NOT NULL,\
		longitude_error 	REAL NOT NULL\
	);\
	CREATE TABLE IF NOT EXISTS routers(\
		mac 				CHARACTER(17) PRIMARY KEY UNIQUE,\
		bssid 				VARCHAR(32),\
		frequency			REAL NOT NULL,\
		channel				INTEGER NOT NULL,\
		latitude 			REAL,\
		longitude 			REAL,\
		radius				REAL\
	);\
	CREATE TABLE IF NOT EXISTS data(\
		scan_id				INTEGER NOT NULL,\
		mac 				CHARACTER(17),\
		signal 				INTEGER,\
		noise 				INTEGER,\
		quality				REAL,\
		FOREIGN KEY(scan_id) REFERENCES scans(id)\
	);";
	// get the row id from scans.
	// make a column in data for the row id of scans as a foreign key.
	
	
	
	
	
	rv = sqlite3_exec(db, schema, NULL, 0, NULL);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "Creation of schema failed: %s\n", sqlite3_errmsg(db));
		return 1;
	}
	fprintf(stdout, "[SqlWriter]: Opening database file <%s>\n", filename);
	return 0;
};


void SqlWriter::close() {
	sqlite3_close(db);
	fprintf(stdout, "[SqlWriter]: Closing database file <%s>\n", this->filename);
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


int SqlWriter::write(char* hw_addr, Position* pos, std::vector<AccessPoint>* ap_list) {	
	// If no access points were found in the scan, do not do an add.
	if (ap_list->size() == 0) {
		printf("[SqlWriter]: ap_list was empty. No access points to update!\n");
		return 0;
	}

	// Writing to the database will be doine with only one sqlite3_exec() command.
	std::string iface_addr = std::string(hw_addr); 
	std::ostringstream q; // queries. All executed at once.

	// INSERT INTO scans VALUES ()
	// First create the query to insert into scans.
	// TODO: This is the "bad" way to insert values. 
		// You should be referencing the row name.
	q << "INSERT INTO scans VALUES(";
	q << "NULL" << ",";
	q << pos->time << ",";
	q << "\"" << iface_addr << "\"" << ",";
	q << pos->latitude << ",";
	q << pos->longitude << ",";
	q << pos->latitude_dev << ",";
	q << pos->longitude_dev << "";
	q << ");";
	
	// Execute the query formed from the stringstream.
	rv = sqlite3_exec(db, q.str().c_str(), NULL, 0, &errmsg);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "sqlite3_exec(): %s\n", errmsg);
		sqlite3_free(errmsg);
		return -1;
	}
	
	// Clear out the stringstream.
	q.str("");
	q.clear();
	
	// Execute the next query to retreive the new id.
	std::string rid_query;
	rid_query = "SELECT last_insert_rowid();";
	int scan_id = 0;
	rv = sqlite3_exec(db, rid_query.c_str(), exists_callback, &scan_id, &errmsg);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
		return -1;
	}
	printf("[SqlWriter]: New scan row %d inserted!\n", scan_id);
	
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
			//printf("[SqlWriter]: Old MAC %s already exists...\n", ap.mac);
		} else {
			printf("[SqlWriter]: New router <%s> discovered!\n", ap.essid);
			// MAC doesn't exist in the database. Add it.
			q << "INSERT INTO routers VALUES(";
			q << "\"" << ap.mac << "\"" << ",";
			q << "\"" << ap.essid << "\"" << ",";
			q << ap.frequency << ",";
			q << ap.channel << ",";
			q << 0 << ",";
			q << 0 << ",";
			q << 0 << "";
			q << ");"; // lat and lng estimiates init to 0's.
		}
		
		// Insert the new data entries into the data table.
		q << "INSERT INTO data VALUES(";
		q << scan_id << ",";
		//q << pos->time << ",";
		//q << "\"" << iface_addr << "\"" << ",";
		q << "\"" << ap.mac << "\"" << ",";
		q << ap.signal << ","; 
		q << ap.noise << ",";
		q << ap.quality << "";
		q << ");";
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





















