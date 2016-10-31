// Includes
//#include <sys/types.h>
#include <sys/socket.h>         // UNIX sockets.
#include <linux/wireless.h>     // Linu wireless tools.
#include <sys/ioctl.h>          // ioctl(); -- not needed. using iwlib, which wraps ioctl calls.
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();
//#include <net/if.h>             // ifreq struct;
#include <iwlib.h>              // Wireless Extension.




/*
http://stackoverflow.com/questions/400240/how-can-i-get-a-list-of-available-wireless-networks-on-linux
http://www.mit.edu/afs.new/sipb/project/merakidev/src/openwrt-meraki/openwrt/build_mips/wireless_tools.28/iwlib.h

I can either wrap my own blunt of scan, or use a built in function in the iwlib, iw_scan(), that scans for me.

ioctl() is a protocol for using UNIX sockets as ways to system-call the kernel.
In this case specifically, it is to call the kernel to request data from IO devices.

*/


int scan() {
    /*
        SIOCSIWSCAN starts the scan
        SIOCGIWSCAN gets the results of the scan
    */
    
    
    unsigned char buffer[256];
    struct iwreq req;
    struct timeval tv;
    int timeout = 5000000; // 5 seconds.
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    
    // Params are inputs, data are outputs?
    req.u.param.flags = IW_SCAN_DEFAULT;
    req.u.param.value = 0;
    
    // What the hell man.
    req.u.data.pointer = NULL;
    req.u.data.flags = 0;
    req.u.data.length = 0;
    
    // http://man7.org/linux/man-pages/man3/errno.3.html
    /*
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ioctl(sockfd, SIOCSIWSCAN, &req) < 0) {
        perror("ioctl");
    }
    */
    
    
    int sockfd = iw_sockets_open();
    if (iw_set_ext(sockfd, "mlan0", SIOCSIWSCAN, &req) < 0) {
        perror("iw_set_ext()");
        if (errno == EPERM)
            fprintf(stderr, "Launch the application using sudo.\n");
        return(1);
    }
    
    
    
    timeout -= tv.tv_usec;
    // Scan started...
    
    
    // Apparently we need an infinite loop here. Good idea for a thread.
    while(1) {
        fd_set rfds;
        int last_fd;
        int ret;
        
        FD_ZERO(&rfds);
        last_fd = -1;
        ret = sleep(3);
        
        if (ret > 0) {
            
            
            if (errno == EAGAIN || errno == EINTR) {
                perror("what?");
                continue;
            } else {
                perror("unknown");
                return(1);
            }
            
            
        } else if (ret == 0) {
            // TODO: explain this.
            req.u.data.pointer = (char*) buffer;
            req.u.data.flags = 0;
            req.u.data.length = sizeof(buffer);
            if (iw_get_ext(sockfd, "mlan0", SIOCGIWSCAN, &req) < 0) {
                perror("iw_get_ext()");
                return(1);
            }
            break;
        }
    }
    
    // Got scan results. Set up a parsing stream to read them out, I guess.
    
    if (req.u.data.length) { // I guess this executes if it's not zero. Can't imagine length == 1.
        struct iw_event iwe;
        struct stream_descr stream;
        int ret;
        iw_init_event_stream(&stream, (char*)buffer, req.u.data.length);
        do {
            // Extract events from the event stream and do something with them.
            ret = iw_extract_event_stream(&stream, &iwe, WE_VERSION);
            if (ret > 0) {
                // Stream isn't done. Parse the event.
                fprintf(stdout, "event!\n");
                
                
                
                
                
                
            }
            
            
        } while (ret > 0);
    }
}




int main(int argc, char** argv) {
    fprintf(stdout, "Hello world!\n");
    //scan();
    
    wireless_scan_head head;
    wireless_scan* result;
    iwrange range;
    int socket;
    
    socket = iw_sockets_open();
    // Get some metadata to use for scanning
    if (iw_get_range_info(socket, "mlan0", &range) < 0) {
        printf("Error during iw_get_range_info. Aborting.\n");
        exit(2);
    }

    // Perform the scan
    if (iw_scan(socket, "mlan0", range.we_version_compiled, &head) < 0) {
        printf("Error during iw_scan. Aborting.\n");
        exit(2);
    }

    // Traverse the results
    result = head.result;
    while (NULL != result) {
        printf("%s\n", result->b.essid);
        result = result->next;
    }
    
}