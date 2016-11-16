// Includes
#include <iwlib.h>              // Wireless Extension.
#include <linux/wireless.h>     // Linux wireless tools. Structs & constants.
#include <stdio.h>              // fprintf(), stdout, perror();
#include <stdlib.h>             // malloc();
#include <string.h>             // strcpy();
#include <string>               // std::string
#include <iostream>             // std::cout
#include <vector>               // std::vector

// https://www.tutorialspoint.com/sqlite/sqlite_installation.htm
#include <sqlite3.h>            // sqlite3

#include "WifiScanner.cpp"
#include "AccessPoint.cpp"

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

// Information elements: (WHERE ARE THEY???)
http://forums.cabling-design.com/wireless/what-does-the-information-element-with-elementid-221-represe-29948-.htm

<------------------------------< NOTES TO SELF >------------------------------>
You can set to scan all by using flag IW_SCAN_ALL_ESSID,
    and choose scan type with flag IW_SCAN_TYPE_ACTIVE or IW_SCAN_TYPE_PASSIVE.

ioctl() is a protocol for using UNIX sockets as ways to system-call the kernel.
In this case, we're using it to call the kernel to request data from IO devices.
    Namely, the wifi driver.
*/


//-[main entry point]--------------------------------------------------------//
int main(int argc, char** argv) {
    std::string interface = "mlan0";
    WifiScanner wifi;
    AccessPointBuilder ap_builder;
    std::vector<AccessPoint> ap_list;
    
    wifi.scan(interface.c_str());
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