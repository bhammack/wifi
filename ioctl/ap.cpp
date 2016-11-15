// Includes
#include <iwlib.h>              // Wireless Extension.
#include <linux/wireless.h>     // Linux wireless tools. Structs & constants.
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();
#include <string>               // std::string
#include <iostream>             // std::cout
#include <vector>               // std::vector

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


/*
<--------------------< USEFUL LINKS >----------------------------------------->
// Graphical view of iwevent structure. Handy!
https://heim.ifi.uio.no/naeemk/madwifi/structiw__event.html

// <iwlib.h> source -- linux wireless tools, version 29. (using 30).
https://github.com/HewlettPackard/wireless-tools/blob/master/wireless_tools/iwlib.c

// Good commented explanations of all the structs in the linux wireless tools.
http://w1.fi/hostapd/devel/wireless__copy_8h_source.html

// Measuring signal strength:
https://support.metageek.com/hc/en-us/articles/201955754-Understanding-WiFi-Signal-Strength

<------------------------------< NOTES TO SELF >------------------------------>
You can set to scan all by using flag IW_SCAN_ALL_ESSID,
    and choose scan type with flag IW_SCAN_TYPE_ACTIVE or IW_SCAN_TYPE_PASSIVE.

ioctl() is a protocol for using UNIX sockets as ways to system-call the kernel.
In this case, we're using it to call the kernel to request data from IO devices.
    Namely, the wifi driver.
*/



//-[AccessPoint]-------------------------------------------------------------//
class AccessPoint {
    public:
        std::string essid;  // 32 chars
        std::string mac;    // 17 chars
        float frequency;
        int channel;
        int signal;
        int noise;
        float quality;      // (event->u.qual.qual / range->max_qual.qual)
        bool encrypted;
};


//-[AccessPointBuilder]------------------------------------------------------//
class AccessPointBuilder {
    private:
        // Data members.
        AccessPoint _ap;
        struct iw_range* _range;
        char buffer[128];
        char _progress; // measures build progress
        
        // Building functions. Each one sets a bit of _progress.
        void add_mac(iw_event* event);
        void add_essid(iw_event* event);
        void add_freq(iw_event* event);
        void add_quality(iw_event* event);
        void add_encrypted(iw_event* event);
        void add_info(iw_event* event);
        // can make
        // two more
        
    public:
        AccessPointBuilder();
        AccessPoint get_ap();
        bool is_built();
        void clear();
        void handle(iw_event* event);
        void set_range(iw_range* range);
};

// Constructor
AccessPointBuilder::AccessPointBuilder() {
    _progress = 0x00;
    _range = NULL;
}


// Get the result from the builder.
AccessPoint AccessPointBuilder::get_ap() {
    return _ap;
}


// If the bitmask of all adds is the correct value, return 1.
bool AccessPointBuilder::is_built() {
    return(_progress == 0xff);
}


// Clear the current AP from memory and start fresh.
void AccessPointBuilder::clear() {
    _ap = AccessPoint();
}


// Set the scan's range data. Used in other adding functions.
void AccessPointBuilder::set_range(iw_range* range) {
    _range = range;
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
    /*
    http://www.speedguide.net/faq/how-does-rssi-dbm-relate-to-signal-quality-percent-439
    event->u.qual has four members.
        qual        rssi
        noise       noise (dBm?)
        level       dBm?
        updated     bitmask
    */
    
    // The conditionals below are testing flags via the & operation.
    if (!(event->u.qual.updated & IW_QUAL_QUAL_INVALID)) {
        // Set the quality. It is always a relative value.
        _ap.quality = ((float)event->u.qual.qual) / _range->max_qual.qual;
    }
    
    if (event->u.qual.updated & IW_QUAL_RCPI) {
        // The signal data is in RCPI (IEEE 802.11k) format.
        // Signal
        if (!(event->u.qual.updated & IW_QUAL_LEVEL_INVALID)) {
            double rcpilevel = (event->u.qual.level / 2.0) - 110.0;
            _ap.signal = rcpilevel;
        }
        // The noise level should be in dBm.
        // Noise
        if (!(qual->updated & IW_QUAL_NOISE_INVALID)) {
            double rcpinoise = (qual->noise / 2.0) - 110.0;
            _ap.noise = rcpinoise;
        }
    } else if ((event->u.qual.updated & IW_QUAL_DBM)
        || (event->u.qual.level > _range->max_qual.level)) {
        // The signal data is in units of dBm. Handy!
        // Signal
        if (!(event->u.qual.updated & IW_QUAL_LEVEL_INVALID)) {
            int dblevel = event->u.qual.level;
            // Implement a range for dBm [-192; 63]
            if(dblevel >= 64)
                dblevel -= 0x100;
            _ap.signal = dblevel;
        }
        // Noise
        if (!(event->u.qual.updated & IW_QUAL_NOISE_INVALID)) {
            int dbnoise = event->u.qual.noise;
            // Implement a range for dBm [-192; 63]
            if(dbnoise >= 64)
                dbnoise -= 0x100;
            _ap.noise = dbnoise;
		}
    } else {
        // Level is a relative value to the range.
        // We do not want this case at all. Fail if we hit it.
        fail("relative values in signal and noise!");
        if (!(event->u.qual.updated & IW_QUAL_LEVEL_INVALID)) {}
        if (!(event->u.qual.updated & IW_QUAL_NOISE_INVALID)) {}
    }
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


// Add an information element. We're looking for specific ones.
void AccessPointBuilder::add_info(iw_event* event) {
    // Each IE is at least two bytes.
    // 0xDD (221) & 0x30 (48) map to WPA1 and WPA2 information.
    /*
    Table 4-7. Information Elements
    https://www.safaribooksonline.com/library/view/80211-wireless-networks/0596100523/ch04.html
    */
    
}

void AccessPointBuilder::handle(iw_event* event) {
    // Read each event code like this:
    // [SIOC][G][IW][data_element]
    switch(event->cmd) {
        case SIOCGIWAP:
            add_mac(event);
            break;
        /*
        case SIOCGIWNWID:
            // network-id used in pre 802.11 systems. replaced by essid.
            // Doesn't ever seem to occur.
            if (event->u.nwid.disabled)
                printf("NWID: off/any\n");
            else
                printf("NWID: %X\n", event->u.nwid.value);
            break;
        */
        case SIOCGIWFREQ:
            add_freq(event);
            break;
        /*
        case SIOCGIWMODE:
            // Appears to always be 'Master'.
            //printf("Mode:%s\n", iw_operation_mode[event->u.mode]);
            break;
        *//*
        case SIOCGIWNAME:
            //printf("Protocol:%-1.16s\n", event->u.name);
            break;
        */
        case SIOCGIWESSID:
            add_essid(event);
            break;
            
        case SIOCGIWENCODE:
            add_encrypted(event);
            break;
            
        case SIOCGIWRATE:
            // Always occurs. Should I capture this though?
            /*iw_print_bitrate(buffer, event->u.bitrate.value);
            printf("                    Bit Rate:%s\n", buffer);*/
            break;
        /*
        case SIOCGIWMODUL:
            // Doesn't ever seem to occur.
            break;
        */
        case IWEVQUAL:
            add_quality(event);
            break;
            
        case IWEVGENIE:
            // Information events. TODO: research how to parse these!
            add_info(event);
            break;
        
        case IWEVCUSTOM:
            // Extra data the router broadcasts back to us. Last beacon time.
            /*char custom[IW_CUSTOM_MAX+1];
	        if((event->u.data.pointer) && (event->u.data.length))
        	    memcpy(custom, event->u.data.pointer, event->u.data.length);
        	custom[event->u.data.length] = '\0';
        	printf("Extra:%s\n", custom);*/
            break;
            
        default:
            printf("(Unknown Wireless Token 0x%04X)\n",event->cmd);
            fail("handle()");
            break;
    }
}

//-[WifiScanner]-------------------------------------------------------------//
class WifiScanner {
    private:
        struct stream_descr stream;
        iw_range range;
        int has_range;
        iw_event iwe;
    public:
        int scan(std::string& iface);
        iw_event* get_event();
        iw_range* get_range();
};

iw_event* WifiScanner::get_event() {
    if (iw_extract_event_stream(&stream, &iwe, range.we_version_compiled))
        return &iwe;
    else
        return NULL;
}

iw_range* WifiScanner::get_range() {
    if (has_range)
        return &range;
    else
        return NULL;
}

// TODO: dynamic buffer size. Reallocate if the buffer was too small.
int WifiScanner::scan(std::string& iface) {
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
        // Keep sending SIOCGIWSCAN to the socket to request scan results.
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
    // Assumption: For the scan to be 'successful', it must have range.
    has_range = (iw_get_range_info(sockfd, iface.c_str(), &range) >= 0);
    if (!has_range)
        fail("scan() has no range!");
    // Scanning done. Results are present in the event stream.
    iw_init_event_stream(&stream, (char*)buffer, request.u.data.length);
    iw_sockets_close(sockfd);
    return 0;
}






//-[main entry point]--------------------------------------------------------//
int main(int argc, char** argv) {
    std::string interface = "mlan0";
    WifiScanner wifi;
    AccessPointBuilder ap_builder;
    std::vector<AccessPoint> ap_list;
    
    wifi.scan(interface);
    ap_builder.set_range(wifi.get_range());
    iw_event* iwe;
    while ((iwe=wifi.get_event()) != NULL) {
        ap_builder.handle(iwe);
        if (ap_builder.is_built()) {
            ap_list.push_back(ap_builder.get_ap());
            ap_builder.clear();
        }
    }
    return 0;
}