#include <stdio.h>
#include "Locator.cpp"

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage:\tlocate.o <interface> <filepath.db>\n\n");
	}
	char* iface = argv[1];
	char* filepath = argv[2];
	printf("Locating from scans on interface %s. Writing to file %s\n", iface, filepath);
		
	Locator l;
	l.open(filepath);
	l.write_kml();
	//l.trilaterate(std::string(mac_addr));
	l.close();
	return 0;
}
