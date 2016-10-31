// Includes
//#include <sys/types.h>
#include <sys/socket.h>         // UNIX sockets.
#include <linux/wireless.h>     // Linu wireless tools.
#include <sys/ioctl.h>          // ioctl();
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();
//#include <net/if.h>             // ifreq struct;
#include <iwlib.h>              // Additional wireless tools?




/*
http://stackoverflow.com/questions/400240/how-can-i-get-a-list-of-available-wireless-networks-on-linux
https://android.googlesource.com/platform/external/kernel-headers/+/donut-release/original/linux/wireless.h
http://wireless-tools.sourcearchive.com/documentation/30~pre9-3ubuntu6/iwlib_8h_source.html
*/


int scan() {
    /*
        SIOCSIWSCAN starts the scan
        SIOCGIWSCAN gets the results of the scan
    */
    unsigned char buffer[256];
    struct iwreq req;
    
    memset(&req, 0, sizeof(req)); // not sure why this is required...
    //strcpy(req.ifr_name, "mlan0");
    
    // Params are inputs, data are outputs?
    req.u.param.flags = IW_SCAN_DEFAULT;
    req.u.param.value = 0;
    
    
    req.u.data.pointer = NULL;
    req.u.data.flags = 0;
    req.u.data.length = 0;
    
    // ioctl() uses sockets, since they are system calls to the kernel.
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (iw_set_ext(sockfd, "mlan0", SIOCSIWSCAN, &req) < 0) {
        perror("iw_set_ext()");
        if (errno == EPERM)
            fprintf(stderr, "Launch the application using sudo.\n");
        else
            fprintf(stderr, "Interface does not support scanning!\n");
        return(1);
    }
    // Successful!
    
    //req.u.data.pointer = (char*) buffer;
    //req.u.data.flags = 0; // why assign this?
    //req.u.data.length = sizeof(buffer); // why? this doesn't make sense!
    //fprintf(stdout, "%s", buffer);
    
    
    
    
    
    // Apparently we need an infinite loop here. Good idea for a thread.
    //while(1) {}

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