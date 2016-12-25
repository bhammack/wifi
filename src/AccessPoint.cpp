#include <iwlib.h>              // Wireless Extension.
#include <linux/wireless.h>     // Linux wireless tools. Structs & constants.
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();

#define KILO 1000
#define MEGA 1000000
#define GIGA 1000000000

#define ESSID_SIZE 33           // technically 32, but +1 for null terminator.
#define MAC_SIZE 17             // 17 characters, including colons.

// From iwlib.h, so I don't have to type 'struct' everywhere.
typedef struct iw_event iw_event;
typedef struct iw_range iw_range;


//-[AccessPoint]-------------------------------------------------------------//
class AccessPoint {
    public:
        char essid[ESSID_SIZE];
        char mac[MAC_SIZE];
        float frequency;    // in GHz
        int channel;        // standard channel numbers
        int signal;         // dBm
        int noise;          // dBm
        float quality;      // (event->u.qual.qual / range->max_qual.qual)
        int encrypted;      // 1/0
        void toString();     // print the ap to stdout.
};

// Prints the ap. Assumes all fields have been filled.
void AccessPoint::toString() {
    printf("===================================\n");
    printf("Mac:\t\t%s\n", mac);
    printf("Essid:\t\t%s\n", essid);
    printf("Freq:\t\t%f\n", frequency);
    printf("Channel:\t%d\n", channel);
    printf("Signal:\t\t%d\n", signal);
    printf("Noise:\t\t%d\n", noise);
    printf("Quality:\t%1.3f\n", quality); // TODO: figure out how to format.
    printf("Encrypted:\t%d\n", encrypted);
}


//-[AccessPointBuilder]------------------------------------------------------//
class AccessPointBuilder {
    private:
        // Data members.
        AccessPoint _ap;
        struct iw_range* _range;
        bool _is_built; // measures build progress
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
    _is_built = false;
    _range = NULL;
}


// Get the result from the builder.
AccessPoint AccessPointBuilder::get_ap() {
    return _ap;
}


// If the bitmask of all adds is the correct value, return 1.
bool AccessPointBuilder::is_built() {
    return _is_built;
}


// Clear the current AP from memory and start fresh.
void AccessPointBuilder::clear() {
    _ap = AccessPoint();
    memset(_ap.essid, 0, ESSID_SIZE);
    memset(_ap.mac, 0, MAC_SIZE);
}


// Set the scan's range data. Used in other adding functions.
void AccessPointBuilder::set_range(iw_range* range) {
    _range = range;
}


// Add the mac address. This is also the first function called.
void AccessPointBuilder::add_mac(iw_event* event) {
    sprintf(_ap.mac, "%02X:%02X:%02X:%02X:%02X:%02X",
        event->u.ap_addr.sa_data[0], event->u.ap_addr.sa_data[1],
        event->u.ap_addr.sa_data[2], event->u.ap_addr.sa_data[3],
        event->u.ap_addr.sa_data[4], event->u.ap_addr.sa_data[5]);
}


// Add the essid.
void AccessPointBuilder::add_essid(iw_event* event) {
    if (event->u.essid.length == 1) {
        _ap.essid[0] = 0x00;
        return;
    }
    if((event->u.essid.pointer) && (event->u.essid.length))
        memcpy(_ap.essid, event->u.essid.pointer, event->u.essid.length);
    _ap.essid[event->u.essid.length] = '\0';
    /*if (event->u.essid.flags)
        _ap.essid = std::string(essid);
    else
        _ap.essid = "";*/
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
        if (!(event->u.qual.updated & IW_QUAL_NOISE_INVALID)) {
            double rcpinoise = (event->u.qual.noise / 2.0) - 110.0;
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
        fprintf(stderr, "add_quality(): relative values detected!\n");
        exit(1);
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
        _ap.encrypted = 0;
    else
        _ap.encrypted = 1;
}


// Add an information element. We're looking for specific ones.
void AccessPointBuilder::add_info(iw_event* event) {
    /*
        Since the order of the stream shows only one IE received,
            they all must be packaged in the data field and parseable.
        Each IE is at least two bytes.
        0xDD (221) & 0x30 (48) map to WPA1 and WPA2 information.
        Table 4-7. Information Elements
    https://www.safaribooksonline.com/library/view/80211-wireless-networks/0596100523/ch04.html
    
    http://forums.cabling-design.com/wireless/what-does-the-information-element-with-elementid-221-represe-29948-.htm
    */
}

void AccessPointBuilder::handle(iw_event* event) {
    /*
    // Read each event code like this:
    // [SIOC][G][IW][data_element]
    Apparent order of cmd events:
    -------------------
        SIOCGIWAP
        SIOCGIWFREQ // one of these is channel...
        SIOCGIWFREQ
        IWEVQUAL
        SIOCGIWENCODE
        SIOCGIWESSID
        SIOCGIWRATE
        SIOCGIWRATE
        SIOCGIWRATE
        SIOCGIWRATE
        SIOCGIWRATE
        SIOCGIWRATE
        SIOCGIWRATE
        SIOCGIWRATE
        SIOCGIWMODE
        IWEVCUSTOM
        IWEVCUSTOM
        IWEVGENIE
    -------------------
    */
    
    switch(event->cmd) {
        case SIOCGIWAP:
            #ifdef DEBUG
            printf("SIOCGIWAP\n");
            #endif
            add_mac(event);
            break;
        
        case SIOCGIWNWID:
            // network-id used in pre 802.11 systems. replaced by essid.
            // Doesn't ever seem to occur.
            #ifdef DEBUG
            printf("SIOCGIWNWID\n");
            #endif
            /*if (event->u.nwid.disabled)
                printf("NWID: off/any\n");
            else
                printf("NWID: %X\n", event->u.nwid.value);*/
            break;
        
        case SIOCGIWFREQ:
            #ifdef DEBUG
            printf("SIOCGIWFREQ\n");
            #endif
            add_freq(event);
            break;
        
        case SIOCGIWMODE:
            // Appears to always be 'Master'.
            #ifdef DEBUG
            printf("SIOCGIWMODE\n");
            #endif
            //printf("Mode:%s\n", iw_operation_mode[event->u.mode]);
            break;
        
        case SIOCGIWNAME:
            #ifdef DEBUG
            printf("SIOCGIWNAME\n");
            #endif
            //printf("Protocol:%-1.16s\n", event->u.name);
            break;
        
        case IWEVQUAL:
            #ifdef DEBUG
            printf("IWEVQUAL\n");
            #endif
            add_quality(event);
            break;
            
        case SIOCGIWENCODE:
            #ifdef DEBUG
            printf("SIOCGIWENCODE\n");
            #endif
            add_encrypted(event);
            break;
            
        case SIOCGIWESSID:
            #ifdef DEBUG
            printf("SIOCGIWESSID\n");
            #endif
            add_essid(event);
            break;
            
        case SIOCGIWRATE:
            // Always occurs. Should I capture this though?
            #ifdef DEBUG
            printf("SIOCGIWRATE\n");
            #endif
            /*iw_print_bitrate(buffer, event->u.bitrate.value);
            printf("Bit Rate:%s\n", buffer);*/
            break;
        
        case SIOCGIWMODUL:
            // Doesn't ever seem to occur.
            #ifdef DEBUG
            printf("SIOCGIWMODUL\n");
            #endif
            break;
            
        case IWEVCUSTOM:
            // Extra data the router broadcasts back to us. Last beacon time.
            /*char custom[IW_CUSTOM_MAX+1];
	        if((event->u.data.pointer) && (event->u.data.length))
        	    memcpy(custom, event->u.data.pointer, event->u.data.length);
        	custom[event->u.data.length] = '\0';
        	printf("Extra:%s\n", custom);*/
        	#ifdef DEBUG
        	printf("IWEVCUSTOM\n");
        	#endif
            break;
            
        case IWEVGENIE: {
            // Information events. TODO: research how to parse these!
            #ifdef DEBUG
            printf("IWEVGENIE\n");
            #endif
            add_info(event);
            
            // Temp
            //AccessPoint ap = get_ap();
            //ap.toString();
            //clear();
			_is_built = true;
        } break;
            
        default:
            printf("UNKNOWN 0x%04X\n",event->cmd);
            break;
    }
}
