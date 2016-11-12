// Includes
#include <sys/socket.h>         // UNIX sockets.
#include <linux/wireless.h>     // Linu wireless tools.
//#include <sys/ioctl.h>          // ioctl(); -- not needed. using iwlib, which wraps ioctl calls.
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();
#include <iwlib.h>              // Wireless Extension.
//#include <getopt.h>             // argument parsing

#define KILO 1000
#define MEGA 1000000
#define GIGA 1000000000

int scan(char* iface);
void handle_event(struct iw_event* event, struct iw_range* range, int has_range);

void fail(char* reason) {
    perror(reason);
    exit(1);
}


/*
// Graphical view of iwevent structure. Handy!
https://heim.ifi.uio.no/naeemk/madwifi/structiw__event.html

// <iwlib.h> source -- linux wireless tools, version 29. (using 30).
https://github.com/HewlettPackard/wireless-tools/blob/master/wireless_tools/iwlib.c







// Measuring signal strength:
https://support.metageek.com/hc/en-us/articles/201955754-Understanding-WiFi-Signal-Strength


ioctl() is a protocol for using UNIX sockets as ways to system-call the kernel.
In this case specifically, it is to call the kernel to request data from IO devices.
*/

int ap_num; // global var, for now.


/*
    Data I need to collect:
    1. ap mac address
    2. ap essid
    3. ap quality & signal & noise
    4. geographic position
    5. channel & frequency
    6. encryption metadata
    


*/

void handle_event(struct iw_event* event, struct iw_range* range, int has_range) {
    char buffer[128];
    switch(event->cmd) {
        case SIOCGIWAP:
            sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
                event->u.ap_addr.sa_data[0], event->u.ap_addr.sa_data[1],
                event->u.ap_addr.sa_data[2], event->u.ap_addr.sa_data[3],
                event->u.ap_addr.sa_data[4], event->u.ap_addr.sa_data[5]);
            printf("\n[==========[ Cell %02d ]==========]\n", ap_num);
            printf("Address: %s\n", buffer);
            ap_num++;
            break;
        
        case SIOCGIWNWID:
            if (event->u.nwid.disabled)
                printf("NWID: off/any\n");
            else
                printf("NWID: %X\n", event->u.nwid.value);
            break;
        
        case SIOCGIWFREQ: {
            // Either the frequency or the channel.
            // TODO: There are some other operations I can include here to...
                // 1. Convert freq to channel number.
                // 2. Output whether freq is in MHz, GHz, etc.
            // View the source of iwlib.c for more.
            double freq = (double) event->u.freq.m;
            int channel = -1;
            
            // Converting frequency to floating point value.
            for (int i = 0; i < event->u.freq.e; i++) { freq *= 10; }
            if (freq < KILO) {
                // Freq is actually a channel number and not a freq at all.
                channel = (int)freq;
                sprintf(buffer, "Channel: %d", channel);
            }
            else {
                if (freq >= GIGA) {
                    sprintf(buffer, "Frequency: %g GHz", freq/GIGA);
                } else {
                    if (freq >= MEGA) {
                        sprintf(buffer, "Frequency: %g MHz", freq/MEGA);
                    } else {
                        sprintf(buffer, "Frequency: %g KHz", freq/KILO);
                    }
                }
            }
            printf("%s\n", buffer);
            } break;
        /*
        case SIOCGIWMODE:
            printf("Mode:%s\n", iw_operation_mode[event->u.mode]);
            break;

        case SIOCGIWNAME:
            printf("Protocol:%-1.16s\n", event->u.name);
            break;
        */
        case SIOCGIWESSID: {
            if (event->u.essid.length == 1) return;
            char essid[IW_ESSID_MAX_SIZE+1];
    	    if((event->u.essid.pointer) && (event->u.essid.length))
    	        memcpy(essid, event->u.essid.pointer, event->u.essid.length);
    	    essid[event->u.essid.length] = '\0';
    	    if (event->u.essid.flags) {
    	        printf("Essid: %s\n", essid);
    	    } else {
    	        printf("Essid: off/any\n");
    	    }
        } break;
        
        case SIOCGIWENCODE: {
            // Cut. There's more processing that can happen here.
            unsigned char key[IW_ENCODING_TOKEN_MAX];
            if (event->u.data.pointer)
                memcpy(key, event->u.data.pointer, event->u.data.length);
            else
                event->u.data.flags |= IW_ENCODE_NOKEY;
            printf("Encryption key: ");
            if (event->u.data.flags & IW_ENCODE_DISABLED)
                printf("none\n");
            else
                printf("on\n");
        } break;
        /*
        case SIOCGIWRATE:
            iw_print_bitrate(buffer, event->u.bitrate.value);
            printf("                    Bit Rate:%s\n", buffer);
            break;
        case SIOCGIWMODUL:
            break;
        */
        case IWEVQUAL: {
        	event->u.qual.updated = 0x0; // Not that reliable, disabled.
        	// It also seems that noise is not an output'd factor. Investigate...
        	
        	//iw_print_stats(buffer, &event->u.qual, range, has_range);
        	if (has_range && (event->u.qual.level != 0)) {
        	    if (event->u.qual.level > range->max_qual.level) {
        	        // The statistics are in dBm.
                    sprintf(buffer, "Quality: %d/%d Signal: %d dBm Noise: %d dBm%s",
                        event->u.qual.qual, range->max_qual.qual, // qual per range
                        event->u.qual.level - 0x100, event->u.qual.noise - 0x100, // sig & noise.
                        // 0x100 is 256 decimal. Not sure why this subtraction occurs...
                        (event->u.qual.updated & 0x7) ? " (updated)" : "");
        	    } else {
        	        // The statistics are relative levels.
        	        sprintf(buffer, "Quality: %d/%d Signal: %d/%d dBm Noise: %d/%d%s",
                        event->u.qual.qual, range->max_qual.qual, // qual per range
                        event->u.qual.level, range->max_qual.level, // sig per range
                        event->u.qual.noise, range->max_qual.noise, // noise per range
                        (event->u.qual.updated & 0x7) ? " (updated)" : "");
        	    }
        	} else {
        	    // The range could not be read. Statistics are unknown.
        	    sprintf(buffer, "Quality: %d Signal: %d dBm Noise: %d%s",
                    event->u.qual.qual, event->u.qual.level,
                    event->u.qual.noise,
                    (event->u.qual.updated & 0x7) ? " (updated)" : "");
        	}
        	printf("%s\n", buffer);
        } break;
        
        case IWEVGENIE: {
            // Information elements. Only looking for ones that tell us about encryption mode.
            //iw_print_gen_ie(event->u.data.pointer, event->u.data.length);
            int offset = 0;
            /*
            while (offset <= (event->u.data.length -2)) {
                switch((unsigned char*)event->u.data.pointer[offset]) {
                    case 0xDD:
                    case 0x30:
                        //iw_print_ie_wpa(buffer + offset, buflen);
                        break;
                }
                offset += (unsigned char*)event->u.data.pointer[offset+1]+2;
            }
            */
        } break;
    }
    
    
    
}



int scan(char* iface) {
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
    if (iw_set_ext(sockfd, iface, SIOCSIWSCAN, &request) < 0) {
        fail("iw_set_ext()");
        // If this throws "Operation not permitted", run as sudo or su.
    }
    
    
    fprintf(stdout, "Scan initiated...\n");

    // TODO: refine this algo.
    // dynamically allocate the buffer size.
    // Keep making ioctl requests for the scan results until they're returned.
    
    
    // Enter the while loop that attempts to collect scanning results.
    request.u.data.pointer = (char*) buffer;
    request.u.data.flags = 0;
    request.u.data.length = sizeof(buffer);
    int scanning = 1;
    while(scanning) {
        sleep(3); // normal delay until device available.
        // TODO: remove sleep(); replace with polling?
        // It doesn't seem to be the case that the more sleep time it has,
        // the more results that are collected.
        if (iw_get_ext(sockfd, iface, SIOCGIWSCAN, &request) < 0) {
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
    
    /*
    if (req.u.data.length == 0) {
        fprintf(stderr, "Error: Scan returned no results!\n");
        fail("req.u.data.length == 0");
    }
    */
    
    // Scan results have been returned. Create an event stream to read.
    struct iw_event iwe;
    struct stream_descr stream;
    int count = 0;
    
    // Determine the range at which the scan took place.
    struct iw_range range;
    int has_range = (iw_get_range_info(sockfd, iface, &range) >= 0);
    
    // Create an event stream. Push each event to the event handler function.
    iw_init_event_stream(&stream, (char*)buffer, request.u.data.length);
    while(iw_extract_event_stream(&stream, &iwe, range.we_version_compiled)) {
        handle_event(&iwe, &range, has_range);
        count++;
    }
    fprintf(stdout, "%d events in stream\n", count);
    iw_sockets_close(sockfd);
}





int main(int argc, char** argv) {
    ap_num = 1;
    char iface[] = "mlan0";
    scan(iface);
}
