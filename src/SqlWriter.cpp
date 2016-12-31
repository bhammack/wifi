#include <sqlite3.h>

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
	
	
	
	return sqlite3_open(filename, &db);
};