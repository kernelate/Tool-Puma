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
#include <fcntl.h>
#include <dirent.h>

#include "hardware_checker.h"

#define LOGI(...)

int led_check(void)
{
	int fd;
	
	fd = open("/dev/DoorTalk_Drivers", O_RDWR);
	if (fd < 0)
	{
		printf("cannot open doortalk_drivers!\n");
		close(fd);
		return -1;
	}

	if (ioctl(fd, SLOW) < 0) {
		LOGI("ioctl() Failed\n");
		return -1;
	}
	
	close(fd);
	return 0;
}

int e_canceller(void)
{
	int fd;
	
	//printf("%s\n",__func__);
	
	fd = open("/dev/echocanceller_dev", O_RDWR);
	if (fd < 0)
	{
		printf("cannot open doortalk_drivers!\n");
		close(fd);
		return -1;
	}

	if (ioctl(fd, EC_ON) < 0) {
		LOGI("ioctl() Failed\n");
		return -1;
	}
		
	close(fd);
	return 0;
}

int camera_check(void)
{
	char camera[100] = "am start -a android.intent.action.MAIN -n com.android.camera/.Camera";
	char key[100] = "input keyevent 27";
	
	
	if (system(camera) != 0)
	{
		puts(camera);
		LOGI("can't run camera app. \n");
		return -1;
	}
	sleep(1);
	if (system(key) != 0)
	{
		puts(key);
		LOGI("can't take a picure. \n");
		return -1;
	}
	
	return 0;
}

char* check_for_pictures(void)
{
    DIR *d;
    struct dirent *dir;
    int x;
    char* filename;
    const char* MyDir = "/mnt/sdcard/DCIM/Camera/";
    
//    printf("%s\n",__func__);
    
    filename = malloc(sizeof(char *));
    
    d = opendir(MyDir);
    if (d == NULL)
    	return "none";
    
    while ((dir = readdir(d)) != NULL)
    {
//      	printf("image: %s\n", dir->d_name);
      	strcpy(filename, dir->d_name);
      	if( dir == NULL ){ 
      		x = closedir(d);
      		printf("%d",x);
      	}
    }
//    printf("get_image: %s\n", filename);
	return filename;
}



