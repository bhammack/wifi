// Includes
#include <iwlib.h>              // Wireless Extension.
#include <linux/wireless.h>     // Linux wireless tools.
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();
#include <string>               // std::string
#include <iostream>             // std::cout

#define KILO 1000
//#define MEGA 1000000
#define GIGA 1000000000

// From iwlib.h, so I don't have to type 'struct' everywhere.
typedef struct iw_event iw_event;
typedef struct iw_range iw_range;

// Handy fail function.
void fail(char const * reason) {
    perror(reason);
    exit(1);
}





class AccessPoint {
    public:
        std::string essid;  // 32 chars
        std::string mac;    // 17 chars
        float frequency;
        int channel;
        int signal;
        float quality;
        bool encrypted;
};



class AccessPointBuilder {
    private:
        AccessPoint _ap;
        struct iw_range* _range;
        int _has_range;
        char buffer[128];
    public:
        AccessPoint get_ap();
        void clear();
        void set_range(iw_range* range, int has_range);
        void add_mac(iw_event* event);
        void add_essid(iw_event* event);
        void add_freq(iw_event* event);
        void add_quality(iw_event* event);
        void add_encrypted(iw_event* event);
};


// Get the result from the builder.
AccessPoint AccessPointBuilder::get_ap() {
    return _ap;
}


// Clear the current AP from memory and start fresh.
void AccessPointBuilder::clear() {
    _ap = AccessPoint();
}


// Set the scan's range data. Used in other adding functions.
void AccessPointBuilder::set_range(iw_range* range, int has_range) {
    _range = range;
    _has_range = has_range;
}


// Add the mac address. This is also the first function called.
void AccessPointBuilder::add_mac(iw_event* event) {
    sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
        event->u.ap_addr.sa_data[0], event->u.ap_addr.sa_data[1],
        event->u.ap_addr.sa_data[2], event->u.ap_addr.sa_data[3],
        event->u.ap_addr.sa_data[4], event->u.ap_addr.sa_data[5]);
    _ap.mac = std::string(buffer);
}


// Add the essid.
void AccessPointBuilder::add_essid(iw_event* event) {
    if (event->u.essid.length == 1) {
        _ap.essid = "";
        return;
    }
    char essid[IW_ESSID_MAX_SIZE+1];
    if((event->u.essid.pointer) && (event->u.essid.length))
        memcpy(essid, event->u.essid.pointer, event->u.essid.length);
    essid[event->u.essid.length] = '\0';
    if (event->u.essid.flags)
        _ap.essid = std::string(essid);
    else
        _ap.essid = "";
}


// Add the frequency of broadcast, or the channel.
void AccessPointBuilder::add_freq(iw_event* event) {
    double freq = (double) event->u.freq.m;
    // Expanding frequency to it's full size value.
    for (int i = 0; i < event->u.freq.e; i++) { freq *= 10; }
    if (freq < KILO) {
        _ap.channel = (int)freq;
    } else {
        if (freq >= GIGA) {
            _ap.frequency = freq/GIGA;
        } else {
            // This shouldn't ever happen. We're scanning wifi points...
            fprintf(stderr, "add_freq(): frequency less than GHz?\n");
            exit(1);
        }
    }
}


// Add the signal, quality, and noise. Needs to have iw_range already set.
void AccessPointBuilder::add_quality(iw_event* event) {
    
}



// Determine if the ap's encrypted. The encryption type will come later.
void AccessPointBuilder::add_encrypted(iw_event* event) {
    unsigned char key[IW_ENCODING_TOKEN_MAX];
    if (event->u.data.pointer)
        memcpy(key, event->u.data.pointer, event->u.data.length);
    else
        event->u.data.flags |= IW_ENCODE_NOKEY;
    if (event->u.data.flags & IW_ENCODE_DISABLED)
        _ap.encrypted = false;
    else
        _ap.encrypted = true;
}



class Scanner {
    private:
        AccessPointBuilder _builder;
        void handle(iw_event* event);
    public:
        int scan(std::string& iface);
};


// Preform the scan.
int Scanner::scan(std::string& iface) {
    /*
        SIOCSIWSCAN starts the scan
        SIOCGIWSCAN gets the results of the scan
    */
    unsigned char buffer[IW_SCAN_MAX_DATA*15]; // 4096*12 bytes
    struct iwreq request;
    
    // Clear initial data pointers.
    request.u.data.pointer = NULL;
    request.u.data.flags = 0;
    request.u.data.length = 0;
    
    // Request a socket to communicate with the kernel via ioctl().
    int sockfd = iw_sockets_open();
    
    // Send the initiate scan call to the new socket.
    if (iw_set_ext(sockfd, iface.c_str(), SIOCSIWSCAN, &request) < 0) {
        fail("iw_set_ext()");
        // If this throws "Operation not permitted", run as sudo or su.
    }
    fprintf(stdout, "Scan initiated...\n");
    
    request.u.data.pointer = (char*) buffer;
    request.u.data.flags = 0;
    request.u.data.length = sizeof(buffer);
    int scanning = 1;
    while(scanning) {
        sleep(3);
        if (iw_get_ext(sockfd, iface.c_str(), SIOCGIWSCAN, &request) < 0) {
            if (errno == EAGAIN) {
                fprintf(stderr, "resource in use...\n");
                continue;
            } else if (errno == E2BIG) {
                fprintf(stderr, "buffer too small!\n");
                fail("iw_get_ext()");
            } else {
                fail("iw_get_ext()");
            }
        }
        // Got the results. Terminate the loop.
        scanning = 0;
    }
    
    // Scan results have been returned. Create an event stream to read.
    struct iw_event iwe;
    struct stream_descr stream;
    int count = 0;
    
    // Determine the range at which the scan took place.
    struct iw_range range;
    int has_range = (iw_get_range_info(sockfd, iface.c_str(), &range) >= 0);
    
    // Give the range information to the builder.
    _builder.set_range(&range, has_range);
    
    // Create an event stream. Push each event to the event handler function.
    iw_init_event_stream(&stream, (char*)buffer, request.u.data.length);
    while(iw_extract_event_stream(&stream, &iwe, range.we_version_compiled)) {
        handle(&iwe);
        count++;
    }
    iw_sockets_close(sockfd);
    return count;
}


void Scanner::handle(iw_event* event) {
    switch(event->cmd) {
        case SIOCGIWAP:
            _builder.add_mac(event);
            break;
        
        
     
     
     
     
        
    }
}









int main(int argc, char** argv) {
    
    return 0;
}