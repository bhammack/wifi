#include <iwlib.h>              // Wireless Extension.
#include <linux/wireless.h>     // Linux wireless tools. Structs & constants.
#include <stdio.h>              // fprintf(), stdout, perror();
//#include <stdlib.h>             // malloc();

// TODO: keep malloc stuff and add conditional fprintf stuff.

/*
	sudo apt-get install libiw-dev
	TODO: The difference between bssid and essid is that bssid usually corresponds
	with the MAC address of the router. Essid is the name of a signal broadcasting from an AP.
	An AP can broadcast multiple SSID's (UF's network is ESSID, mine is BSSID).
	TODO: Investigate RSSI -- measure of signal strength.
*/


//-[WifiScanner]-------------------------------------------------------------//
class WifiScanner {
    private:
        struct stream_descr stream;
        iw_range range;
        int has_range;
        iw_event iwe;
    public:
        int scan(const char* iface);
        iw_event* get_event();
        iw_range* get_range();
		void get_iface_addr(const char* iface);
};

// Gets the hardware MAC address associated with the network interface.
void WifiScanner::get_iface_addr(const char* iface) {
	int s;
	struct ifreq buffer;
	s = socket(PF_INET, SOCK_DGRAM, 0);
	memset(&buffer, 0x00, sizeof(buffer));
	strcpy(buffer.ifr_name, "eth0");
	ioctl(s, SIOCGIFHWADDR, &buffer);
	close(s);
	for(int i = 0; i < 6; i++ )
		printf("%.2X ", (unsigned char)buffer.ifr_hwaddr.sa_data[i]);
	printf("\n");
}



// Pop an event reference from the event stream.
iw_event* WifiScanner::get_event() {
    if (iw_extract_event_stream(&stream, &iwe, range.we_version_compiled))
        return &iwe;
    else
        return NULL;
}


// Return a reference to the latest scan's range.
iw_range* WifiScanner::get_range() {
    if (has_range)
        return &range;
    else
        return NULL;
}

// TODO: dynamic buffer size. Reallocate if the buffer was too small.
int WifiScanner::scan(const char* iface) {
    /*
        Scanning flags:
        http://w1.fi/hostapd/devel/wireless__copy_8h_source.html
        line 00528
    */
    unsigned char buffer[IW_SCAN_MAX_DATA*15]; // 4096*15 bytes
    struct iwreq request;
    // Clear initial data pointers.
    request.u.data.pointer = NULL;
    request.u.data.flags = 0;
    request.u.data.length = 0;
    // Request a socket to communicate with the kernel via ioctl().
    int sockfd = iw_sockets_open();
    // Send the SIOCSIWSCAN to the new socket to initiate scan call.
    if (iw_set_ext(sockfd, iface, SIOCSIWSCAN, &request) < 0) {
        perror("iw_set_ext()");
        exit(1);
        // If this throws "Operation not permitted", run as sudo or su.
    }
    fprintf(stdout, "[WifiScanner]: Scan initiated...\n");
    request.u.data.pointer = (char*) buffer;
    request.u.data.flags = 0;
    request.u.data.length = sizeof(buffer);
    int scanning = 1;
    while(scanning) {
        sleep(3);
        // Keep sending SIOCGIWSCAN to the socket to request scan results.
        if (iw_get_ext(sockfd, iface, SIOCGIWSCAN, &request) < 0) {
            if (errno == EAGAIN) {
                fprintf(stderr, "[WifiScanner]: Interface %s in use...\n", iface);
                continue;
            } else if (errno == E2BIG) {
                fprintf(stderr, "buffer too small!\n");
                perror("iw_get_ext()");
                exit(1);
            } else {
                perror("iw_get_ext()");
                exit(1);
            }
        }
        // Got the results. Terminate the loop.
        scanning = 0;
    }
    // Assumption: For the scan to be 'successful', it must have range.
    has_range = (iw_get_range_info(sockfd, iface, &range) >= 0);
    if (!has_range) {
        fprintf(stderr, "scan(): Range could not be determined!\n");
        exit(1);
    }
    // Scanning done. Results are present in the event stream.
    iw_init_event_stream(&stream, (char*)buffer, request.u.data.length);
    iw_sockets_close(sockfd);
    return 0;
}
