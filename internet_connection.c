#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internet_connection.h"


int wpa_supplicant(void)
{
	FILE *fp;
	int check;
	char wpa[] = "/system/bin/wpa_supplicant.sh platform aaaa1112 WPA-PSK 0";
	char wifiON[] = "svc wifi enable";
	char wifiOFF[] = "svc wifi disable";
	char insmodWifi[] = "insmod /system/wifi/wlan.ko";


	printf("wpa_supplicant() \n");

	if (system(wpa) != 0)
	{
		puts(wpa);
		printf("error wpa_supplicant.sh \n");
		return -1;
	}

	
	if (installed==0){
		if(system(insmodWifi)!=0)
		{
			puts(insmodWifi);
			printf("wlan.ko is already installed!\n");
			installed = 1;
			if (system(wifiON) != 0)
			{
				puts(wifiON);
				printf("can't turn ON wifi \n");
				return -1;
			}
			else
				wifi_state = 1;
		
		}
	
	}
	else
	{
		if(wifi_state==0)
			if (system(wifiON) != 0)
			{
				puts(wifiON);
				printf("can't turn ON wifi \n");
				return -1;
			}
	}

	
	return 0;

}
