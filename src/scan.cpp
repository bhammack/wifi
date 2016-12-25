#include "WifiScanner.cpp"
#include "AccessPoint.cpp"
//#include "GpsScanner.cpp"
#include <vector>

int main(int argc, char** argv) {
	WifiScanner wifi;
	//GpsScanner gps;
	AccessPointBuilder builder;
	std::vector<AccessPoint> ap_list;
	
	
	wifi.scan("wlan0");
	builder.set_range(wifi.get_range());
	iw_event* iwe;
	while ((iwe = wifi.get_event()) != NULL) {
		builder.handle(iwe);
		if (builder.is_built()) {
			ap_list.push_back(builder.get_ap());
			builder.clear();
		}
	}
	
	for (unsigned int i = 0; i < ap_list.size(); i++) {
		ap_list[i].toString();
	}
	
	return 0;
}