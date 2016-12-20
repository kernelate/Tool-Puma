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
#include <pthread.h>

#include <signal.h>
//#include "cutils/log.h"

#include "device_info.h"
#include "devlist_checker_API.h"
#include "hardware_checker.h"
#include "reply.h"

//#define LOGR(...) ((void)__android_log_print(ANDROID_LOG_INFO, "TEST_FARM", __VA_ARGS__))
#define LOGR(...) 
//#define TARGET_ADDR     "127.0.0.1"
#define TARGET_ADDR     "192.168.3.23"
#define TARGET_PORT     5001
#define SDCARD_CAMERA_DIR  	"/mnt/sdcard/DCIM/Camera/"

#define CMD_CONNECT             "TESTFARM:CMD_CONNECT"
#define CMD_DEVICE_INFO         "TESTFARM:CMD_DEVICE_INFO"
#define CMD_SEND_FILE			"TESTFARM:CMD_SEND_FILE"
#define END                     "END"

#define MAX_BUFF                255
#define MAX_MSG      		    255
#define MAX_MSG_CMD     		50
#define MAX_EXPECTED_PARAMS     100
#define MAX_REPLY               15
#define MAX_SIZE			10
//TODO: limit global variables, use them as func params
char header[] = "TESTFARM";
char command[MAX_BUFF];
int socketfd;
int run_socket = 0;
int status = 1; //status reflects available commands
struct sockaddr_in serv_addr;
char rcvBuff[MAX_BUFF];
char msg_param[MAX_EXPECTED_PARAMS][MAX_MSG];
char update_stat[255] = "online";
char *temp;
//char* dev_check;
char* projName;

int replyFlag=0;
int cameraExist = 0;
int watch_timer = 0; //30secs
int fileSize;
int start_download = 0; //download flag
char **update_file;

char* available_commands[] = 
{
		CMD_CONNECT, /*send */
		CMD_DEVICE_INFO, /*send peripheral info*/
		CMD_SEND_FILE, /*send file */
};

char* replies[] = { "reboot", "shutdown", "factory reset", "switch mode",
		"update", "find me", "device info", "adb mode", "clear", "transfer", };

void error(const char *msg) 
{
	perror(msg);
	exit(0);
}

void reset_watchdog_timer(){
	watch_timer=0;
//	printf("SOCKET SERVICE: watchdog cleared\n");
}
//**************************************************
//----------------process reply--------------------
//**************************************************

/* some command only works upon reboot
 * */
void execute_reply(char **reply_msg) 
{
	printf("%s %s\n", __func__, reply_msg[0]);
	int i, mode, ret, count;
	
	reset_watchdog_timer();
	
	if (strlen(reply_msg[0]) < 1)
		return;

	for (i = 0; replies[i] != NULL; i++) 
	{
		if (strcmp(replies[i], reply_msg[0]) == 0){
			printf("%s\n",reply_msg[0]);
			break;
		}
			
	}
	
	switch (i) 
	{
		case 0:
			restart_dev();
			strcpy(update_stat, "entering recovery");
			break;
		case 1:
          shutdown_dev();
			break;
		case 2:
//          factory_reset();
			break;
		case 3: //switch mode
			break;
		case 4:

#if 0
			strcpy(update_stat, "downloading");
			if ((ret = update_firmware(reply_msg)) < 0)
				strcpy(update_stat, "error");
			else {
				sprintf(update_stat, "%s complete", reply_msg[1]);
			}
#endif            
            update_file = malloc(sizeof(char *) * MAX_MSG);
            memset(update_file, 0, MAX_MSG);
            if(strlen(reply_msg[1]) > 0){ 
                for(i = 0; i < 4; i++){
                    update_file[i] = reply_msg[i];
                
                } 
//                printf("****************** %s\n", update_file[1]);
                start_download = 1;
            }
            status = 1;
			break;
		case 5: //find me
			printf("findme\n");
			findme(projName);
			break;
		case 6: //hardware info 
			//initiate self test
#if 0			
			start_socket_connection();
			dev_check = malloc(sizeof(char *) * MAX_BUFF);
			dev_check = get_hw_info();
			sprintf(command, "%s:%s:%s", available_commands[1], dev_check, END);
			printf("command: %s\n",command);
			send_socket_message(command);
			stop_socket_connection();
			free(dev_check);
#endif
			status = 2;
			break;
		case 7: //adb mode
			if(strlen(reply_msg[1]) > 0)
			{ 
				mode = strcmp(reply_msg[1], "0");
				adb_mode(mode);
			}   
			break;
		case 8:
            clear_cmd();
			break;
		
		case 9:
			printf("File Transfer\n");
            if(!strcmp(reply_msg[1], "complete"))
    			status = 1;
            else
                status = 3;
                            
			break;
				
		default:
     	       printf("%s INVALID reply\n", __func__);
			return;
	}
}

void clear_params(void)
{
	memset(msg_param, 0,
			sizeof(msg_param[0][0]) * MAX_EXPECTED_PARAMS * MAX_MSG);
}

int countParams(char* s, char c) 
{
	return *s == '\0' ? 0 : countParams(s + 1, c) + (*s == c);
}

int msg_parser(char* msgbuff) 
{
	/*parse string msg with dynamic parameters
	 *see ktabmidid for ex. in aml
	 * */
	int i, ret, count;
	i = count = ret = 0;
	char *buff;
	char **reply;
	clear_params();
	count = countParams(msgbuff, ':');

	buff = strtok(msgbuff, ":");
	
	if (strncmp(header, buff, 8) != 0) 
	{
		goto error1;
	}
	
//	memset(*reply, 0,sizeof(*reply));

	//parse string message
	for (i = 0; i < count; i++) 
	{
		buff = strtok(NULL, ":");
		if (strncmp("END", buff, 3) == 0) 
		{
			i = count;
			break;
		}
		if (i == 0 && (strncmp("ACK", buff, 3) != 0))
			break; //msg not valid
		else
			strcpy(msg_param[i], buff);
	}
	reply = malloc(sizeof(char *) * MAX_REPLY);
	for (i = 1; i < count; i++)
		reply[i - 1] = msg_param[i];

	execute_reply(reply);
	free(reply);
	return 0;

	error1:
	//DPRINTF("Invalid msg\n");
//	printf("INVALID\n");
	return -1;
}

//**************************************************
//--------------------------socket handling--------------------------------
//**************************************************
int start_socket_connection() 
{

	//prepare socket 
	memset(rcvBuff, 0, MAX_BUFF);
	if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		printf("\n Error : Could not create socket \n");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(TARGET_PORT);       //port no

	if (inet_pton(AF_INET, TARGET_ADDR, &serv_addr.sin_addr) <= 0) 
	{
		printf("\n inet_pton error occured\n");
		return -1;
	}

	// start socket connection
	if (connect(socketfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))< 0) 
	{
		printf("\n Error : Connect Failed \n");
		return -1;
	}
	
	printf("connected\n");

	return socketfd;
}

void send_socket_message(char *msg) 
{
//    printf("%s %s\n", __func__, msg);
	printf("send: %s\n", msg);
	write(socketfd, msg, strlen(msg));

}

void read_socket_message(void) {
    int ret;
	printf("=======read_socket_message======\n");
	while ((read(socketfd, rcvBuff, sizeof(rcvBuff) - 1)) > 0) 
	{
		if(strcmp(rcvBuff,"TESTFARM:ACK:received:file:END")==0){
//			replyFlag = 1;
            ret = send_file(temp,fileSize);
            if(ret<0){
                printf("error in sending file.\n");
           //     replyFlag = 0;
                status = 1;
            }
			printf("RECEIVED\n");
		}
		if (fputs(rcvBuff, stdout) == EOF) 
		{
			printf("\n Error : Fputs error\n");
		}
	}

//    printf("\n%s\n", rcvBuff);
	printf("\n");
}

void stop_socket_connection(void) 
{
	printf("stop_socket\n");
	close(socketfd);
}

void* socket_handler(void* arg) 
{
	int i;
	while (1) 
	{
		if (run_socket) 
		{
	        printf("%s %s\n", __func__, command);
//			sleep(1); // add delay while filling up command buffer
			start_socket_connection();
			send_socket_message(command);
			read_socket_message();
			stop_socket_connection();
			msg_parser(rcvBuff);
			run_socket = 0;
		}
		sleep(1); // add delay while filling up command buffer
	}
}

void* update_handler(void* arg)
{
    int ret = 0;
    while(1){
//        printf("******** %d\n\n",start_download );
        if(start_download){
            printf("************%s**********\n", __func__);
			strcpy(update_stat, "downloading");
			if ((ret = update_firmware(update_file)) < 0)
				strcpy(update_stat, "error");
			else 
				strcpy(update_stat, "complete");
			
            start_download = 0; 
        }
        sleep(1);
    }//end while(1)
}

void* watchdog(int pid)
{
//	printf("%s\n", __func__);
	
	while(1)
	{
		while(watch_timer <= 30){
			watch_timer++;
			sleep(1);
		}
		kill(pid,SIGTERM);
	}
	
}

//**************************************************
//-----------sending file to the server-------------
//**************************************************
//TODO:
int send_file(char * filename, int sz)
{
	FILE * file;
	int ch, transfd;
	char filepath[100];
	int x;
	char toSEND[1];
	
	
	memset(filepath,0,sizeof(filepath));
	
	//-----------------opening a file-------------------
	sprintf(filepath, "%s%s",SDCARD_CAMERA_DIR,filename);
	printf("send_file: %s\n", filepath);
	
	file = fopen (filepath,"r");
	if(file < 1) 
	{
		printf("Error opening file\n");
		return -1;
	} 
	//*********************OPEN_END***********************
	
	//--------------------sending file--------------------
	x=0;
	printf("size: %d\n",sz);
	printf("sending...\n");
	while(x<sz)
	{
		ch=getc(file);
		toSEND[0] = ch;
		write(socketfd, toSEND, 1);
		printf("x:%d|--%x--\n",x,toSEND[0]);
		x++;
	}

	//*********************SEND_END***********************
	printf("file closing...\n");
	fclose(file);
	return 0;
}
char* get_hardware_status(void)
{
	char **hardware;
	char *camera_status;
	char *led_status;
	char *ecanceller_status;
	char *codec;
	char *stat;
	
	int counter;
	int ret = 0;
	
	//=============== memory allocation =============
	camera_status = malloc(sizeof(char *)* MAX_BUFF);
	ecanceller_status = malloc(sizeof(char *)* MAX_BUFF);
	codec = malloc(sizeof(char *)* MAX_BUFF);
	stat = malloc(sizeof(char *) * MAX_BUFF);
	hardware = malloc(sizeof(char *)* MAX_SIZE);
	led_status = malloc(sizeof(char *)* MAX_BUFF);
	
	//================== memset =====================
	memset(camera_status, 0, MAX_BUFF);
	memset(ecanceller_status, 0, MAX_BUFF);
	memset(codec, 0, MAX_BUFF);
	memset(led_status, 0, MAX_BUFF);
	memset(stat, 0, MAX_BUFF);
	memset(hardware, 0, MAX_SIZE);
	
	//============== getting hardware list ============= //charles's api
	hardware = search_for_hardware();
	if(strcmp(hardware,"none")==0)
	{
		stat = "none";
		free(camera_status);
		free(ecanceller_status);
		free(codec);
		free(hardware);
		return stat;
		free(stat);
	}
				
	//============= hardware checking ==============
	printf("checking for hardware!\n");
	for (counter = 0; counter < sizeof(hardware) + 1; counter++) 
	{
		printf("hardware[%d]: %s\n",counter, hardware[counter]);
		if (strcmp(hardware[counter], "END") == 0) 
			break;
		
		if (strcmp(hardware[counter], "echocanceller") == 0) {
			printf("checking echo canceller: %d\n", counter);
			ret = e_canceller();
			if (ret != 0)
				ecanceller_status = "echocanceller:error";
			else
				ecanceller_status = "echocanceller:ok";
		}
					
		else if (strcmp(hardware[counter], "camera") == 0)
		{
			cameraExist = 1;
//			printf("checking camera: %d\n", counter);
			ret = camera_check();
			if (ret != 0)
				camera_status = "camera:error";
			else
				camera_status = "camera:ok";
            printf("camera check done\n");
		}
		
		else if (strcmp(hardware[counter],"led")==0)
		{
//			printf("checking led: %d\n", counter);
			ret = led_check();
			if (ret != 0)
				led_status = "led:error";
			else
				led_status = "led:ok";
            printf("led check done\n");
		}
		
		else if (strcmp(hardware[counter], "codec") == 0){
//			printf("checking codec: %d\n", counter);
			codec = "codec:ok";
			printf("checking codec: %s\n", codec);
		}
	}
	memset(stat, 0, MAX_BUFF);
	sprintf(stat, "%s:%s:%s:%s",camera_status, ecanceller_status,codec, led_status);
//	free(camera_status);
//	free(ecanceller_status);
//	free(codec);
//	free(hardware);
	return stat;
}

char* get_picture(void)
{
//	printf("get_picture\n");
	char *picture;
	
	picture = malloc(sizeof(char *) * MAX_BUFF);
	
	//============== get picture from SD Card ==============
	memset(picture, 0, MAX_BUFF);
	picture = check_for_pictures();
	if(strcmp(picture,"none")==0)
	{
		picture = "NOFILE";
		return picture;
	}
		
	printf("picture: %s\n",picture);
	return picture;
}

int get_FileSize(char * file)
{
	char filepath[100];
	int fSize = 0;
	
	FILE * input_file;
	
	memset(filepath,0,sizeof(filepath));
	//============ read File ================
	sprintf(filepath, "%s%s",SDCARD_CAMERA_DIR,file);
	input_file = fopen(filepath,"r");
	if(input_file < 1) 
	{
		fSize = -1;
		return fSize;
	} 
	//========= get Size ===========
	fseek(input_file, 0L, SEEK_END);
	fSize = ftell(input_file);
	fseek(input_file, 0L, SEEK_SET);
	fclose(input_file);
	
	return fSize;
}

void get_data(int cmd) 
{
	char *device_stat; //
	char *file_to_send;
	char *msg;
	int fsize =0;
	int counter;
	int ret = 0;

	while(run_socket)
	{
		//do nothing
		usleep(150000);
	}
	//clear previous commands
	memset(command, 0, MAX_BUFF);

	switch (cmd) 
	{
		case 1:
			printf("=======getting mac and ip address======\n");
			
			projName = malloc(sizeof(char *) * MAX_BUFF);
			memset(projName,0,MAX_BUFF);
			
			projName = getModel();
#if 1
			if (getMAC() < 0)
				printf("error in getting MAC address.\n");

			if (getVersion() < 0)
				printf("error in getting version.\n");

			if (getIP() < 0)
				printf("Please check internet connection.\n");
#endif
			//TODO get device index reply msg and send this for easy search, initial index is 0
		
			sprintf(command, "%s:%s:%s:%s:%s:%s", available_commands[cmd - 1], mac,ip, update_stat, verinfo, END);
//			sprintf(command, "%s:%s:%s:%s:%s:%s", available_commands[cmd - 1], "08:D8:33:92:E7:60","192.168.2.1", update_stat, "1.9t", END);
		//	send_socket_message(command);
			
			break;
		case 2:
			//TODO send device health
			printf("=======getting device health======\n");
			cameraExist = 0;
			
//			msg = malloc(sizeof(char *)* MAX_BUFF);
//			memset(msg, 0, MAX_BUFF);
			
			device_stat = get_hardware_status();
			if(strcmp(device_stat,"none")==0)
			{
				sprintf(command, "%s:cant' open build.prop:%s", available_commands[1], END);
			//	send_socket_message(msg);
				printf("%s\n",command);
				status = 1;
				break;
			}
			
			if (!cameraExist)
			{
				sprintf(command, "%s:%s:%s", available_commands[cmd - 1],device_stat,END);
//				send_socket_message(msg);
				printf("%s\n",command);
				status = 1;
				break;
			}
			sprintf(command, "%s:%s:%s", available_commands[cmd - 1],device_stat,END);
			
			sleep(1);
			status = 3;
			break;
			
		case 3:
			printf("send file here\n");
			
			msg = malloc(sizeof(char *)* MAX_BUFF);
			file_to_send = malloc(sizeof(char *) * MAX_BUFF);
			
			memset(msg, 0, MAX_BUFF);
			fsize = 0;
			
			file_to_send = get_picture();
           		temp = file_to_send;//will be used in recv_socket
            		printf("=== %s %s\n", temp, file_to_send);
			if(strcmp(file_to_send,"NOFILE")==0)
			{
				printf("There is no file.\n");
				sprintf(command, "%s:camera:error:%s", available_commands[1],END);
//				send_socket_message(msg);
				status = 1;
				break;
			}
			
			fsize = get_FileSize(file_to_send);
            		fileSize = fsize; //will be used in recv_socket
			if(fsize <=0)
			{
				printf("Error opening file\n");
				sprintf(command, "%s:camera:error:%s", available_commands[cmd - 1],END);
//				send_socket_message(msg);
				status = 1;
				break;
			}
			
			sprintf(command, "%s:%s:%d:%s", available_commands[cmd - 1], mac, fsize, END);
			break;
		
		case 4:
			//TODO: checking internet connection
			if (system("ping -c1 192.168.2.1 ") != 0)
				if (strcmp(ip, "") == 0)
					if (wpa_supplicant() < 0)
						printf("No internet connection\n");

			break;

		default:
			printf("invalid cmd\n");
			break;
		}
}

int main(int argc, char *argv[]) 
{
	int pid;

	pthread_t pid_socket, pid_watchdog, pid_update;
	
	pid = getpid();
	
	printf("pid: %d\n",pid);
	
	pthread_create(&pid_socket, NULL, &socket_handler, NULL);
	pthread_create(&pid_update, NULL, &update_handler, NULL);
	pthread_create(&pid_watchdog, NULL, &watchdog, pid);
	int ret;

	while (status) 
	{       
		get_data(status);
		run_socket = 1;
		sleep(1);
	}

	pthread_join(pid_socket, NULL);
	pthread_join(pid_watchdog, NULL);
	pthread_join(pid_update, NULL);
	return 0;
}
