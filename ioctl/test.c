// Includes
#include <sys/socket.h>         // UNIX sockets.
#include <linux/wireless.h>     // Linu wireless tools.
#include <sys/ioctl.h>          // ioctl(); -- not needed. using iwlib, which wraps ioctl calls.
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();
#include <iwlib.h>              // Wireless Extension.
#include <getopt.h>             // argument parsing


/*
http://stackoverflow.com/questions/400240/how-can-i-get-a-list-of-available-wireless-networks-on-linux
http://www.mit.edu/afs.new/sipb/project/merakidev/src/openwrt-meraki/openwrt/build_mips/wireless_tools.28/iwlib.h

I can either wrap my own blunt of scan, or use a built in function in the iwlib, iw_scan(), that scans for me.

ioctl() is a protocol for using UNIX sockets as ways to system-call the kernel.
In this case specifically, it is to call the kernel to request data from IO devices.

*/


int scan(char* iface) {
    /*
        SIOCSIWSCAN starts the scan
        SIOCGIWSCAN gets the results of the scan
    */
    unsigned char buffer[IW_SCAN_MAX_DATA*10]; // 4096*10 bytes
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
    fprintf(stdout, "Results: %d\n", req.u.data.length);
    if (req.u.data.length == 0) {
        fprintf(stderr, "Error: Scan returned no results?\n");
        return(2);
    }
    
    // Got scan results. Set up a parsing stream to read them out, I guess.
    struct iw_event iwe;
    struct stream_descr stream;
    int count = 0;
    iw_init_event_stream(&stream, (char*)buffer, req.u.data.length);
    while(iw_extract_event_stream(&stream, &iwe, WE_VERSION)) {
        // iwe is our temporary variable. Process the event.
        count++;
    }
    fprintf(stdout, "%d events in stream\n", count);
    
    
    iw_sockets_close(sockfd);
}




int main(int argc, char** argv) {
    char iface[] = "mlan0";
    scan(iface);
}