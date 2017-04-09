#include <stdio.h>
#include "Locator.cpp"

int main(int argc, char** argv) {
	if (argc < 4) {
		fprintf(stderr, "Usage:\tlocate.o <interface> <filepath.db> <mac address>\n\n");
	}
	char* iface = argv[1];
	char* filepath = argv[2];
	char* mac = argv[3];
	printf("Locating from scans on interface %s. Writing to file %s\n", iface, filepath);
	
	int do_all = 0;
	std::string do_all_str(mac);
	if (do_all_str.compare(std::string("all")) == 0) {
		do_all = 1;
	}
		
	Locator l;
	l.open(filepath);
	
	
	if (do_all) {
		printf("Trilaterating all points! This make take a while...\n");
		l.trilaterate_all();
	} else {
		l.trilaterate(std::string(mac, true));
	}
	//l.write_kml();
	//l.trilaterate("A0:63:91:87:A0:37"); // free virus
	//l.trilaterate("10:DA:43:8C:25:B5"); // casa del fupa
	l.close();
	return 0;
}
