// Includes
//#include <sys/types.h>
#include <sys/socket.h>         // UNIX sockets.
#include <linux/wireless.h>     // Linu wireless tools.
#include <sys/ioctl.h>          // ioctl();
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();
//#include <net/if.h>             // ifreq struct;
//#include <iwlib.h>              // wireless-tools. if not found sudo apt-get install iwlib-dev?

// struct to hold wireless data
/*
struct datapoint {
    char mac[18]; // 12 for chars, 5 for colons. No need to have 18.
    char ssid[33]; // why 33?
    int bitrate; // what are these?
    int level;
};
https://android.googlesource.com/platform/external/kernel-headers/+/donut-release/original/linux/wireless.h
*/


int scan() {
    
    struct iwreq req; // fuck me I had it all along...
    memset(&req, 0, sizeof(req)); // not sure why this is required...
    strcpy(req.ifr_name, "mlan0");

    //struct ifreq freq; // fuck me double.
    //strcpy(req.ifr_name, "mlan0");
    //char buffer[1024];
    //memset(buffer, 0, 1024);
    
    req.u.data.pointer = NULL;
    req.u.data.flags = 0;
    req.u.data.length = 0;
    //strcpy(freq.ifr_name, "mlan0");
    
    //memset(&freq, 0, sizeof(freq));

    // IOctl uses sockets. This is because IOctl is a system call for IO devices.
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    /*
    SIOCGIWSTATS -> iw_statistics object.
    SIOCGIWESSID -> ssid as char*.
    SIOCS80211SCAN
    */
    //char buffer[32];
    //memset(buffer, 0, 32); // set the buffer to just 0's.
    
    //wreq.u.essid.pointer = buffer;
    //wreq.u.essid.length = 32;
    
    //this will gather the SSID of the connected network
    //if(ioctl(sockfd, SIOCGIWESSID, &wreq) == -1){
    
    if(ioctl(sockfd, SIOCSIWSCAN, &req) == -1){
        perror("scan()");
        return(1);
    } else {
        //char response[wreq.u.essid.length];
        //memcpy(response, wreq.u.essid.pointer, wreq.u.essid.length);
        //fprintf(stdout, "%s", response);
        fprintf(stdout, "%s", buffer);
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