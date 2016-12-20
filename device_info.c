#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/stat.h>

#include "device_info.h"

#define LOGR(...) 

char* getModel(void)
{
	FILE *fp;
	int check;
	char getprop[100] = "getprop ro.product.model > /cache/product";
	char buff[100];

	if (system(getprop) != 0)
	{
		puts((const char*) getprop);
		LOGR("can't create /cache/product. \n");
		return -1;
	}

	fp = fopen("/cache/product", "r");

	if (fp == NULL)
	{
		LOGR("can't open /cache/product. \n");
		return -1;
	}

	fgets(buff, 100, (FILE*) fp);
	strtok(buff, "\n");
	strcpy(project,strtok(buff," "));
//	printf("project = %s\n",project);
//	LOGR("2: %s\n", ver );

	fclose(fp);

	if (system("rm /cache/product") != 0)
	{
		LOGR("cant delete /cache/product . \n");
	}

	return project;
}

int getVersion(void)
{
	FILE *fp;
	int check;
	char getprop[100] = "getprop ro.product.version > /cache/product";
	char buff[100];

	if (system(getprop) != 0)
	{
		puts((const char*) getprop);
		LOGR("can't create /cache/product. \n");
		return -1;
	}

	fp = fopen("/cache/product", "r");

	if (fp == NULL)
	{
		LOGR("can't open /cache/product. \n");
		return -1;
	}

	fgets(buff, 100, (FILE*) fp);
//	LOGR("str = %s",strtok(buff, "\n"));
	strtok(buff,"\n");
	strcpy(verinfo, strtok(buff, "\n"));
	LOGR("2: %s\n", verinfo );

	fclose(fp);

	if (system("rm /cache/product") != 0)
	{
		LOGR("cant delete /cache/product . \n");
	}

	return 0;
}

int getIP(void)
{
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ - 1);

	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
		return -1;

	close(fd);

	if (strlen((const char*) inet_ntoa(&ifr.ifr_addr)) <= 0)
		return -1;

	strcpy(ip,
			(char*) inet_ntoa(
					((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr));

	return 0;
}

int getMAC(void)
{
	int fd;
	struct ifreq ifr;

	char buffer[100];

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ - 1);

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
		return -1;

	close(fd);

	sprintf(buffer, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
			(unsigned char) ifr.ifr_hwaddr.sa_data[0],
			(unsigned char) ifr.ifr_hwaddr.sa_data[1],
			(unsigned char) ifr.ifr_hwaddr.sa_data[2],
			(unsigned char) ifr.ifr_hwaddr.sa_data[3],
			(unsigned char) ifr.ifr_hwaddr.sa_data[4],
			(unsigned char) ifr.ifr_hwaddr.sa_data[5]);

	strcpy(mac, buffer);
//	LOGR("mac = %s\n", mac);
	return 0;
}
