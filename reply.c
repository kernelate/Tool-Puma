#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>

#include "reply.h"
#include "device_info.h"
#include "hardware_checker.h"

#include "devlist_checker_API.h"
#include "hardware_checker.h"

#define LOGR(...)

#define MAX_BUFFER  255
#define RUN_APP				"com.ntek.texttospeach/com.ntek.texttospeach.MainActivity" //app to be run by am command
#define TARGET_URL  "http://192.168.3.23/NTEK_UPDATES/" //TODO change IP, 
//server needs apache2 and all update files are inside /var/www/NTEK_UPDATES/
//run test farm server as root



char temp[MAX_BUFFER];
static int adb_wifi = 0;

void restart_dev(void){
    system("reboot"); 
}

void shutdown_dev(void){
    system("reboot -p");
}

void factory_reset(void){
    system("echo 2 > /cache/recovery/command");
    //and reboot
}

void switch_mode(int mode){
    printf("%s\n", __func__);
//no definite task yet
}

void download_progress(char *path, unsigned long target_size){
    unsigned long current_size;
    unsigned long temp;
    
    current_size = f_size(path);
    temp = current_size;
    int i = 0;
#if 0 //for testing only
    while(i<10){
        sleep(1);
        i++;
    }
    return;
#endif
    while(current_size < target_size){
    	printf("size of file %lu %lu\n", current_size, target_size); 
        sleep(1);
        current_size = f_size(path);
        if(temp == current_size) 
            i++;
        else i = 0; 
        temp = current_size; 
        if(i == 30)  break; //connection time out
    }
}

int update_firmware(char **msg){   
    char *fsize = msg[2];
    char *checksum = msg[3];
    char *update, *file_loc;
    unsigned long size = strtoul(fsize,NULL,10);
  
    char *dest[] = {
        "/data/", /*not enough storage*/
        "/mnt/sdcard/", //3
        "/mnt/usb/",    //5
        "/mnt/usb2/",   //6
    };
    
    char command[MAX_BUFFER];
    char path[MAX_BUFFER];
    char temp[MAX_BUFFER];
    int ret, flag = 0;
    
    
    memset(temp, 0, MAX_BUFF);
    strcpy(temp, checksum);

    if(!strcmp("cancel", msg[1])){
        sprintf(command, "busybox rm %s", temp);
        system(command);
        clear_cmd();    
    }

    if (strlen(msg[1]) > strlen("cancel")){ 
        //get filename only
        file_loc = strtok(msg[1], "/");
        while(file_loc != NULL){
            update = strdup(file_loc);
            file_loc = strtok(NULL, "/");
        }

    printf("*****%s %s\n", __func__, checksum);
        sprintf(path, "%s%s", dest[1], update);
        //if file exists, return
        if((ret = checker(path, temp)) == 0)
            return 0;

        sprintf(command, "busybox rm %s", path);
        system(command);
        sprintf(command,"busybox wget %s%s -P %s", TARGET_URL, update, dest[1]);
        system(command);

        printf("%d %s <%d>  \n", __LINE__,temp, flag);
        download_progress(path, size);
        if((ret = checker(path, temp)) < 0)
            return ret;
    //    system("busybox echo 3 > /cache/recovery/command");
        printf("\n\n====Download complete====\n\n");
        strcpy(temp, path);
    }
    return 0;
}

void findme(char* project){
	printf("findme\n");
	char am[100];
	char echo[100];
	int fd;
	
	if(strcmp(project,"KiddieJam")==0){
		fd = open("/dev/recovery_led", O_RDWR);
		if (ioctl(fd, KJAM_WIPE_CACHE) < 0){
			printf("Error in recovery_led");
		}
		close(fd);
		return;
	}
	
	else
	{
		printf("text2speech!\n");
		sprintf(am,"am start -a android.intent.action.MAIN -n %s",RUN_APP);
		if (system(am) != 0)
			puts(am);
	
		sprintf(echo, "echo \"I'm here\" > /cache/updateresult.txt");
		if (system(echo) != 0) {
			puts(echo);
			LOGR("cannot remount /cache/ directory. \n");
		}
		
		fd = open("/dev/DoorTalk_Drivers", O_RDWR);
		if (ioctl(fd, FAST) < 0) {
			printf("ioctl() Failed\n");
			return;
		}
		close(fd);
		return;
	}
}

//health check
char* get_hw_info(void)
{
	printf("=======getting device health======\n");
	char *device_stat; //
#if 0
	char *camera_status;
	char *ecanceller_status;
	int ret = 0;
	int counter;
		
	camera_status = malloc(sizeof(char *));
	ecanceller_status = malloc(sizeof(char *));
	filetosend = malloc(sizeof(char *) * MAX_BUFF);
	device_stat = malloc(sizeof(char *) * MAX_BUFF);
			
	ret = search_for_hardware();
	if (ret < 0){
		device_stat = "cant' open build.prop";
//		sprintf(device_stat[0],"cant' open build.prop.");
		return device_stat;
	}
	
	printf("checking hardware!\n");
	for (counter = 1; counter <= MAX_SIZE; counter++) 
	{
		if (strcmp(hardware[counter], "echocanceller") == 0) {
			printf("checking echo canceller: %d\n", counter);
			ret = e_canceller();
			if (ret != 0)
				ecanceller_status = "echocanceller_error";
			else
				ecanceller_status = "echocanceller_ok";
		}
		
		if (strcmp(hardware[counter], "camera") == 0)
		{
			printf("checking camera: %d\n", counter);
			ret = camera_check();
			if (ret != 0){
				camera_status = "camera_error";
//				printf("camera_status = %s\n",camera_status);
//				device_stat[counter] = strdup(camera_status);
			}else{
				camera_status = "camera_ok";
//				printf("camera_status = %s\n",camera_status);
//				device_stat[counter] = strdup(camera_status);
//				printf("device_stat: %s\n",device_stat[counter]);
			}
			
			filetosend = check_for_pictures();
			if(strcmp(filetosend,"none")==0)
				printf("There is no file.\n");
			printf("filetosend: %s\n",filetosend);
//			return device_stat;
		}
	}
	sprintf(device_stat, "%s:%s",camera_status, ecanceller_status);
	printf("device_stat: %s\n",device_stat);
#endif
	return device_stat;
}

//option for adb over wifi (mode = 1) or 
//adb thru cable (mode = 0)
void adb_mode(int mode){
	if(mode)
	{
		system("setprop service.adb.tcp.port 5555");
		system("stop adbd");
		system("start adbd");
		adb_wifi = 1;
	}
	else if(!mode & adb_wifi)
	{
		system("setprop service.adb.tcp.port -1");
		system("stop adbd");
		system("start adbd");
		adb_wifi = 0;
	}

}

//clear any recovery command
//stop download
void clear_cmd(void){
    system("rm /cache/recovery/command");
}

