// Includes
//#include <sys/types.h>
#include <sys/socket.h>         // UNIX sockets.
#include <linux/wireless.h>     // Linu wireless tools.
//#include <sys/ioctl.h>          // ioctl(); -- not needed. using iwlib, which wraps ioctl calls.
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();
//#include <net/if.h>             // ifreq struct;
#include <iwlib.h>              // Wireless Extension.




/*
http://stackoverflow.com/questions/400240/how-can-i-get-a-list-of-available-wireless-networks-on-linux
https://android.googlesource.com/platform/external/kernel-headers/+/donut-release/original/linux/wireless.h
http://wireless-tools.sourcearchive.com/documentation/30~pre9-3ubuntu6/iwlib_8h_source.html

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
    
     /* Init timeout value -> 250ms*/
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    
    // Params are inputs, data are outputs?
    req.u.param.flags = IW_SCAN_DEFAULT;
    req.u.param.value = 0;
    
    int sockfd = iw_sockets_open();
    if (iw_set_ext(sockfd, "mlan0", SIOCSIWSCAN, &req) < 0) {
        perror("iw_set_ext()");
        if (errno == EPERM)
            fprintf(stderr, "Launch the application using sudo.\n");
        else
            fprintf(stderr, "Interface does not support scanning!\n");
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
                // Stream isn't done. Parse it.
                switch(iwe.cmd) {
                    case SIOCGIWESSID:
                        print_scanning_token(&iwe);
                        break;
                }
            }
            
            
        } while (ret > 0);
    }
    
    
    

}



/*
int getSignalInfo(signalInfo *sigInfo, char *iwname){
    iwreq req;
    strcpy(req.ifr_name, iwname);

    iw_statistics *stats;

    //have to use a socket for ioctl
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    //make room for the iw_statistics object
    req.u.data.pointer = (iw_statistics *)malloc(sizeof(iw_statistics));
    req.u.data.length = sizeof(iw_statistics);

    //this will gather the signal strength
    if(ioctl(sockfd, SIOCGIWSTATS, &req) == -1){
        //die with error, invalid interface
        fprintf(stderr, "Invalid interface.\n");
        return(-1);
    }
    else if(((iw_statistics *)req.u.data.pointer)->qual.updated & IW_QUAL_DBM){
        //signal is measured in dBm and is valid for us to use
        sigInfo->level=((iw_statistics *)req.u.data.pointer)->qual.level - 256;
    }

    //SIOCGIWESSID for ssid
    char buffer[32];
    memset(buffer, 0, 32);
    req.u.essid.pointer = buffer;
    req.u.essid.length = 32;
    //this will gather the SSID of the connected network
    if(ioctl(sockfd, SIOCGIWESSID, &req) == -1){
        //die with error, invalid interface
        return(-1);
    }
    else{
        memcpy(&sigInfo->ssid, req.u.essid.pointer, req.u.essid.length);
        memset(&sigInfo->ssid[req.u.essid.length],0,1);
    }

    //SIOCGIWRATE for bits/sec (convert to mbit)
    int bitrate=-1;
    //this will get the bitrate of the link
    if(ioctl(sockfd, SIOCGIWRATE, &req) == -1){
        fprintf(stderr, "bitratefail");
        return(-1);
    }else{
        memcpy(&bitrate, &req.u.bitrate, sizeof(int));
        sigInfo->bitrate=bitrate/1000000;
    }


    //SIOCGIFHWADDR for mac addr
    ifreq req2;
    strcpy(req2.ifr_name, iwname);
    //this will get the mac address of the interface
    if(ioctl(sockfd, SIOCGIFHWADDR, &req2) == -1){
        fprintf(stderr, "mac error");
        return(-1);
    }
    else{
        sprintf(sigInfo->mac, "%.2X", (unsigned char)req2.ifr_hwaddr.sa_data[0]);
        for(int s=1; s<6; s++){
            sprintf(sigInfo->mac+strlen(sigInfo->mac), ":%.2X", (unsigned char)req2.ifr_hwaddr.sa_data[s]);
        }
    }
    close(sockfd);
}
*/






int main(int argc, char** argv) {
    fprintf(stdout, "Hello world!\n");
    scan();
}