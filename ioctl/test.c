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
    unsigned char buffer[IW_SCAN_MAX_DATA*10]; // 4096 bytes
    struct iwreq req;
    struct timeval tv;
    int timeout = 5000000; // 5 seconds.
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    
    // Params are inputs, data are outputs? These params dont seem to be crucial...
    //req.u.param.flags = IW_SCAN_DEFAULT;
    //req.u.param.value = 0;
    
    // Set the initial data pointers to null. Might not need to do this.
    req.u.data.pointer = NULL;
    req.u.data.flags = 0;
    req.u.data.length = 0;
    
    // Make an ioctl request to initiate scanning.
    int sockfd = iw_sockets_open();
    if (iw_set_ext(sockfd, "mlan0", SIOCSIWSCAN, &req) < 0) {
        perror("iw_set_ext()");
        if (errno == EPERM)
            fprintf(stderr, "Launch the application using sudo.\n");
        return(1);
    }
    fprintf(stdout, "Scan initiated...\n");
    timeout -= tv.tv_usec;

    // TODO: refine this algo.
    // dynamically allocate the buffer size.
    // dont use sleep(3), use select.
    
    // Keep making ioctl requests for the scan results until they're returned.
    while(1) {
        fd_set rfds;
        int last_fd;
        int ret;
        
        FD_ZERO(&rfds); // ?
        last_fd = -1; // ?
        ret = sleep(3);
        
        if (ret > 0) {
            if (errno == EAGAIN || errno == EINTR) {
                perror("what?");
                continue;
            } else {
                perror("unknown");
                return(1);
            }
        }
        
        if (ret == 0) {
            // TODO: explain this.
            req.u.data.pointer = (char*) buffer;
            req.u.data.flags = 0;
            req.u.data.length = sizeof(buffer);
            if (iw_get_ext(sockfd, "mlan0", SIOCGIWSCAN, &req) < 0) {
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
                    return(1);
                }
            }
            // Got the results. Break from the infinite loop.
            break;
        }
    }
    
    fprintf(stdout, "Scan completed!\n");
    // Got scan results. Set up a parsing stream to read them out, I guess.
    if (req.u.data.length) { // I guess this executes if it's not zero. Can't imagine length == 1.
        struct iw_event iwe;
        struct stream_descr stream;
        int stream_valid; // Either 1 (if the stream isn't done) or 0 (stream complete).
        iw_init_event_stream(&stream, (char*)buffer, req.u.data.length);
        do {
            // Extract events from the event stream and do something with them.
            stream_valid = iw_extract_event_stream(&stream, &iwe, WE_VERSION);
            if (stream_valid > 0) {
                // Stream isn't done. Parse the event.
            }
        } while (stream_valid > 0);
    }
}




int main(int argc, char** argv) {
    //fprintf(stdout, "Hello world!\n");
    scan();
}