#include <sqlite3.h>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <cmath>

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
	std::string select_data = "SELECT scans.id, scans.latitude, scans.longitude, \
	scans.latitude_error, scans.longitude_error, data.signal, data.noise, \
	data.quality FROM scans, data WHERE scans.id = data.scan_id AND data.mac = '" + mac + "';";
	
	std::vector< std::vector<std::string> > table;
	rv = sqlite3_exec(db, select_data.c_str(), callback2, &table, &errmsg);
	if (rv != SQLITE_OK) {
		fprintf(stderr, "sqlite3_exec(): %s\n", errmsg);
		sqlite3_free(errmsg);
		return;
	}
	
	double latitude;
	double longitude;
	double radius;
	
	for (int i = 0; i < table.size(); i++) {
		std::vector<std::string>* row = &table.at(i);
		for (int j = 0; j < row->size(); j++) {
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











