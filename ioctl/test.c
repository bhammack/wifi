// Includes
#include <sys/socket.h>         // UNIX sockets.
#include <linux/wireless.h>     // Linu wireless tools.
#include <sys/ioctl.h>          // ioctl(); -- not needed. using iwlib, which wraps ioctl calls.
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();
#include <iwlib.h>              // Wireless Extension.
//#include <getopt.h>             // argument parsing

#define KILO 1000
#define MEGA 1000000
#define GIGA 1000000000

int scan(char* iface);
void handle(struct iw_event* event, struct iw_range* range, int has_range);

/*
http://stackoverflow.com/questions/400240/how-can-i-get-a-list-of-available-wireless-networks-on-linux
http://www.mit.edu/afs.new/sipb/project/merakidev/src/openwrt-meraki/openwrt/build_mips/wireless_tools.28/iwlib.h
http://lxr.free-electrons.com/source/include/uapi/linux/wireless.h#L895
https://heim.ifi.uio.no/naeemk/madwifi/structiw__event.html


COPY THIS -----
https://github.com/cobyism/edimax-br-6528n/blob/master/AP/wireless_tools.25/iwlist.c
---------------

I can either wrap my own blunt of scan, or use a built in function in the iwlib, iw_scan(), that scans for me.

ioctl() is a protocol for using UNIX sockets as ways to system-call the kernel.
In this case specifically, it is to call the kernel to request data from IO devices.

*/


void print_hex(char* array, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02X", array[i]);
        if (i != len-1)
            printf(":");
    }
    printf("\n");
}


int ap_num;
void handle(struct iw_event* event, struct iw_range* range, int has_range) {
    // Here's all the codes:
    //http://lxr.free-electrons.com/source/include/linux/wireless.h?v=2.6.31#L231
    //int cmd = event->cmd;
    /*
    if (cmd == SIOCGIWESSID) {
        if (event->u.essid.length == 1) return;
        char essid[IW_ESSID_MAX_SIZE+1];
	    if((event->u.essid.pointer) && (event->u.essid.length))
	        memcpy(essid, event->u.essid.pointer, event->u.essid.length);
	    essid[event->u.essid.length] = '\0';
	    printf("%s", essid);
	    printf(" - ");
	    print_hex(event->u.ap_addr.sa_data, 14);
    }
    */
    char buffer[128];
    
    switch(event->cmd) {
        case SIOCGIWAP:
            sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
                event->u.ap_addr.sa_data[0], event->u.ap_addr.sa_data[1],
                event->u.ap_addr.sa_data[2], event->u.ap_addr.sa_data[3],
                event->u.ap_addr.sa_data[4], event->u.ap_addr.sa_data[5]
            );
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
            for (int i = 0; i < event->u.freq.e; i++) { freq *= 10; }
            if (freq < KILO) {
                sprintf(buffer, "Channel: %g", freq);
            } else {
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
        /*
        case SIOCGIWRATE:
            iw_print_bitrate(buffer, event->u.bitrate.value);
            printf("                    Bit Rate:%s\n", buffer);
            break;
        */
        case IWEVQUAL: {
        	event->u.qual.updated = 0x0; // Not that reliable, disabled.
        	//iw_print_stats(buffer, &event->u.qual, range, has_range);
        	if (has_range && (event->u.qual.level != 0)) {
        	    if (event->u.qual.level > range->max_qual.level) {
        	        // The statistics are in dBm.

        	        
        	        
        	        
        	        
        	    } else {
        	        // The statistics are relative levels.
        	        
        	        
        	    }
        	    
        	    
        	}
        	
        	
        	
        	
        	
        	
        	
        	
        	
        	
        	printf("%s\n", buffer);
        } break;
        
        
        
    }
    
    
    
}

int scan(char* iface) {
    /*
        SIOCSIWSCAN starts the scan
        SIOCGIWSCAN gets the results of the scan
    */
    unsigned char buffer[IW_SCAN_MAX_DATA*15]; // 4096*12 bytes
    struct iwreq req;
    
    // Params are inputs, data are outputs? These params dont seem to be crucial...
    //req.u.param.flags = IW_SCAN_DEFAULT;
    //req.u.param.value = 0;
    
    // Set the initial data pointers to null. Might not need to do this.
    req.u.data.pointer = NULL;
    req.u.data.flags = 0;
    req.u.data.length = 0;
    
    // Make an ioctl request to initiate scanning.
    int sockfd = iw_sockets_open();
    if (iw_set_ext(sockfd, iface, SIOCSIWSCAN, &req) < 0) {
        perror("iw_set_ext()");
        if (errno == EPERM)
            fprintf(stderr, "Launch the application using sudo.\n");
        return(1);
    }
    fprintf(stdout, "Scan initiated...\n");

    // TODO: refine this algo.
    // dynamically allocate the buffer size.
    // Keep making ioctl requests for the scan results until they're returned.
    while(1) {
        sleep(3); // normal delay until device available.
        // TODO: remove sleep(); replace with polling?
        int ret = 0;

            // TODO: explain this.
            // TODO: don't need to assign this in the loop.
            req.u.data.pointer = (char*) buffer;
            req.u.data.flags = 0;
            req.u.data.length = sizeof(buffer);
            if (iw_get_ext(sockfd, iface, SIOCGIWSCAN, &req) < 0) {
                if (errno == E2BIG) {
                    // buffer was too small; resizing...
                    // TODO: this shouldn't happen.
                    fprintf(stderr, "buffer too small...\n");
                    exit(1);
                }
                if (errno == EAGAIN) { // resource in use. try again.
                    fprintf(stderr, "resource in use...\n");
                    continue;
                }
                else {
                    perror("iw_get_ext()");
                    exit(1);
                }
            }
            // Got the results. Break from the infinite loop.
            break;
    }
    
    fprintf(stdout, "Scan completed!\n");
    //fprintf(stdout, "Results: %d\n", req.u.data.length);
    if (req.u.data.length == 0) {
        fprintf(stderr, "Error: Scan returned no results?\n");
        return(2);
    }
    
    // Got scan results. Set up a parsing stream to read them out, I guess.
    struct iw_event iwe;
    struct stream_descr stream;
    int count = 0;
    
    struct iw_range range;
    
    int has_range = (iw_get_range_info(sockfd, iface, &range) >= 0);
    iw_init_event_stream(&stream, (char*)buffer, req.u.data.length);
    while(iw_extract_event_stream(&stream, &iwe, WE_VERSION)) {
        handle(&iwe, &range, has_range);
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