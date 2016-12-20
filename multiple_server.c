/* A simple server in the internet domain using TCP
 The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <sys/stat.h>
#include <time.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/ioctl.h>


#define DEBUG
#ifdef DEBUG
    #define DPRINTF(fmt, args...) printf(fmt, ##args);
#else
    #define DPRINTF(fmt, args...) 
#endif

//socket specific
#define MAX_MSG      		255
#define MAX_MSG_CMD     		50
#define MAX_EXPECTED_PARAMS     100
#define MAX_PARAM               50

#define TARGET_PORT         5001
#define MAX_DEVICE          100
#define MAX_DEV_LEN         20
#define MAX_DEV_STATLEN     100
#define UPDATE_RATE_SEC     1
#define MAX_DISPLAY_LINE    20
#define MAX_BUFFER          255
#define MAXLEN              50

#define DISP_MAIN           1
#define DISP_DETAILS        2
#define DISP_DIAGBOX        3

#define KEY_ESC             27
#define NTEK_UPDATE_DIR       "/var/www/html/NTEK_UPDATES/" 
#define UPDATE_LIST           "/tmp/update_list.txt"

struct device_info {
    int device_number;
    char ver[MAX_DEV_LEN];
    char mac[MAX_DEV_LEN]; 
    char ip[MAX_DEV_LEN];
    char stat[MAX_DEV_STATLEN];             //status of the device, long message
    char replycommand[MAX_MSG];
    char replycommand2[MAX_MSG];
    char devhealth[MAX_BUFFER];
    int timepassed;
};

unsigned int key_input = 0;
unsigned int screen_changes = 1;
unsigned int current_selected_device = 999999;      //do not select anything
unsigned int line_number = 0;
unsigned int page_number = 0;
unsigned int ui_level = 1;    // 1:dev_summary,2:devs_specs,3:dialogbox
unsigned int device_index;
static int device_count = 0;
struct device_info active_device[MAX_DEVICE];
char buffer[MAX_BUFFER];

int updateList = 0;
int completeFlag = 1;
int receivedFlag = 0;
int stopFlag = 0;
int cntHardware = 0;
char **hardware;
char **hw;
char id;
//====== update lists
char **filenames;
int fileCount;
//======
char * hwList[]={"camera","echocanceller","led","doortalkdrivers","codec",};

//socket specific
int f_runService = 1;
char header[9]="TESTFARM";
char msg_socket_buff[MAX_MSG];
char msg_cmd[MAX_MSG_CMD];
char msg_param[MAX_EXPECTED_PARAMS][MAX_MSG];

char* available_commands[] = {
	"CMD_CONNECT", /*send */
    "CMD_DEVICE_INFO", /*send peripheral info*/
    "CMD_SEND_FILE",
};

char* replies[] = {
    "reboot",
    "shutdown",
    "factory reset",
    "switch mode",
    "update",
    "find me",
    "device info",
    "adb mode",  
    "clear",
    "transfer",
};

char **update_file;

void error(const char *msg) {
	perror(msg);
	exit(1);
}

void start_timer(int ctr)
{
    {
        active_device[ctr].timepassed--;
    }

}

void record_data(char *data[], int ctr)
{
    if(ctr < MAX_DEVICE)
    {
        sprintf(active_device[ctr].mac, "%s", data[0]);
        sprintf(active_device[ctr].ip, "%s", data[1]);
        sprintf(active_device[ctr].stat, "%s", data[2]);
        sprintf(active_device[ctr].ver, "%s", data[3]);
        active_device[ctr].timepassed = 1000; //100s
        reply_msg(active_device[ctr].replycommand, active_device[ctr].replycommand2);
        if(!(strcmp(active_device[ctr].replycommand, replies[6])) //device info 
           || !(strcmp(active_device[ctr].replycommand, replies[0])) || //reboot
           !(strcmp(active_device[ctr].replycommand, replies[5])) || //find me
//           !(strcmp(active_device[ctr].replycommand, replies[7]))|| //adb mode
           !(strcmp(active_device[ctr].replycommand, replies[1]))) //shutdown
        	memset(active_device[ctr].replycommand, 0, MAX_MSG);
        //clear reply command
//        memset(active_device[ctr].replycommand, 0, MAX_MSG);
//        printf("reply1: %s reply2: %s\n",active_device[ctr].replycommand, active_device[ctr].replycommand2);
        memset(active_device[ctr].replycommand2, 0, MAX_MSG);
    printf("%s dev = %d\n", __func__, ctr);
    }
    else 
        printf("ERR: reached max device connected\n");

}
void add_data(char *raw_data[])
{
    int i, ret;
   
    for(i = 0; i < device_count; i++)
    {
        ret = strlen(active_device[i].mac);
        //check if device is already registered
        if(!(strcmp(active_device[i].mac, raw_data[0]))) 
        {
            record_data(raw_data, i);
            return;
        }
    }
    //search device list for available space
    for(i = 0; i < device_count; i++)
    {
        if(!strlen(active_device[i].mac)){
            record_data(raw_data, i);
            return;
        }
    }

    record_data(raw_data, device_count);
    device_count++;
}

int del_data(int ctr)
{
        memset(active_device[ctr].mac, 0, MAX_MSG);
        memset(active_device[ctr].ip, 0, MAX_MSG);
        memset(active_device[ctr].stat, 0, MAX_MSG);
        memset(active_device[ctr].ver, 0, MAX_MSG);
}

void parse_message()
{

}

/***********************************************************************/
//kelvs snippet
int f_size(char *file) {
	struct stat f;
	if (stat(file, &f) < 0) {
		fprintf(stderr, "%s: \n", file);
		return -1;
	} else {

		int size = f.st_size;

		return size;
	}

}

char *trim(char * s) {
	/* Initialize start, end pointers */
	char *s1 = s, *s2 = &s[strlen(s) - 1];

	/* Trim and delimit right side */
	while ((isspace(*s2)) && (s2 >= s1))
		s2--;
	*(s2 + 1) = '\0';

	/* Trim left side */
	while ((isspace(*s1)) && (s1 < s2))
		s1++;

	/* Copy finished string */
	strcpy(s, s1);
	return s;
}

int create_update_list(){
    FILE *file;
    int fileCount = 0;
    char command[MAX_BUFFER];
    char line[MAX_BUFFER];

	sprintf(command, "find %s -exec md5sum {} \\; > %s", NTEK_UPDATE_DIR, UPDATE_LIST);
	system(command);

	file = fopen(UPDATE_LIST, "r");
	while (fgets(line, sizeof line, file) != NULL) {
        fileCount++;    
    }
    fclose(file);
    return fileCount;
}

char** get_filenames(){

    FILE *file;
	char tempcheck[MAX_BUFFER];
    char tempfile[MAX_BUFFER];
    char line[MAX_BUFFER];
    char *s;
    char **checksum;
    char **filename;
    unsigned long filesize[MAX_BUFFER];
    int i, k, numProgs, fileCount;

    i = k = numProgs = 0;

    fileCount = create_update_list();

	file = fopen(UPDATE_LIST, "r");

    checksum = malloc(sizeof(char *) * fileCount);
    filename = malloc(sizeof(char *) * fileCount);
    update_file = malloc(sizeof(char *) * fileCount);

	while (fgets(line, sizeof line, file) != NULL) {
//        printf(" %s %s %d\n", __func__, line, fileCount);

		s = strtok(line, "  ");
		if (s == NULL)
			continue;
		else
			strcpy(tempcheck, s);
		s = strtok(NULL, "  ");
		if (s == NULL)
			continue;
		else
            strcpy(tempfile,s);
		//produce filename and checksum array
		checksum[i] = strdup(tempcheck);
        filename[i] = strdup(tempfile);
		i++; numProgs++;
	}
 
	//get filesize
    while (k < numProgs ) {
		trim(filename[k]);
		filesize[k] = f_size(filename[k]);
        sprintf(tempcheck,"%s:%lu:%s:",filename[k], filesize[k], checksum[k]);
        update_file[k] = strdup(tempcheck);
		k++;
	}
    
    fclose(file);
    return filename;
#if 0
    for(i=0; i <numProgs; i++){
        free(*(checksum+i));
        free(*(filename+i));
    }
   free(checksum);
   free(filename);
#endif
}

/***********************************************************************/

void clear_screen()
{
    system("clear");
}

//page 1 ~ MAX_DEVICE/MAX_DISPLAY_LINE
void set_page_number(int page)
{
    line_number = page * MAX_DISPLAY_LINE;
}

void print_dev_health(int dev_number)
{
    printf("Device Health %d\n", dev_number);
}

void gotoxy(int x,int y)
{
    printf("%c[%d;%df",0x1B,y,x);
}

void main_prev_page()
{

}
void main_next_page()
{

}

int level1_ui(void)
{
    int i;
    char ctrl;
    static int j=0;

    printf("ctrl \tMACAddr \t\tIPAddr \t\t\tVer \t\tStatus \t\tTimer \t\t%d %d\n", j++, page_number);
    for(i = 0, ctrl = 'a'; i<MAX_DISPLAY_LINE; i++, ctrl++){
        if(!strlen(active_device[line_number+i].mac))
        {
            printf("\t-----no device-----\n");
        }
        else{
            start_timer(line_number+i);

            if(active_device[line_number+i].timepassed <= 0)
            {
                del_data(line_number+i); //free resources
            }
            else{
                printf("%c \t%s \t%s \t\t%s \t\t%s \t\t%d\n", 
                        ctrl, 
                        active_device[line_number+i].mac, 
                        active_device[line_number+i].ip, 
                        active_device[line_number+i].ver, 
                        active_device[line_number+i].stat,
                        active_device[line_number+i].timepassed);
            }
        }
    }
    printf("<SpaceBar> NextPage\n");

    return 0;
}

int level2_ui(void)
{
    printf("<space>\tBACK\n");
    printf("<a>\tReboot\n");
    printf("<b>\tShutdown\n");
    printf("<c>\tFactory Reset\n");
    printf("<d>\tSwitch Mode\n");
    printf("<e>\tUpdate using selected Image\n");
    printf("<f>\tFindMe\n");
    printf("<g>\tDevice Info\n");
    printf("<h>\tAdb mode\n");
    printf("<i>\tClear\n");
    print_dev_health(0);

    return 0;
}

int level3_ui(void)
{
    printf("are you sure you want to continue?\n");
    printf("<a>\tReally REBOOT\n");
    printf("<b>\tReally SHUTDOWN\n");
    printf("<space>\tNo, I want to go back\n");
    return 0;
}

int level4_ui(void)
{
    char ctrl = 'a';
    int i = 0;
//    char **filenames;
//    int fileCount;
    if(!updateList){
        filenames = malloc(sizeof(char *) * 100);
        printf("<space>\tBACK\n");
        filenames = get_filenames();
        fileCount = create_update_list();
        updateList = 1;
    }
    for(i= 0; i < fileCount; i++){    
      printf("<%c>\t%s\n",ctrl,filenames[i]);
      ctrl++;
    }
    printf("\n<0>\tCANCEL DOWNLOAD\n");
    printf("\n\tPlease wait for download to finish before rebooting\n");
    return 0;
}

int level5_ui(void)
{
    printf("<space>\tBACK\n");
    printf("<a>\tnormal adb\n");
    printf("<b>\tadb over wifi\n");
    return 0;
}

//TODO:
int level6_ui(void)
{
	int i,a = 0;
	int alike = 0;
	
	printf("ctrl \t\tHardware \t\tStatus \n");
	for(i = 0; i < 10; i=i+2)
	{
		if(!receivedFlag)
				printf("\t-----no hardware-----\n");
		else
			if(i<=cntHardware){
				a = 0;
				while(strcmp(hardware[i],hwList[a++])==0){
					alike = 1;
					if(a>sizeof(hwList)){
						break;
					}
				}
				if((a<=sizeof(hwList))&& (alike))
					printf("%c \t\t%s \t\t\t%s \n", id, hardware[i],hardware[i+1]);
			}
	}
	
	if(stopFlag)
		printf("File transfer error.\n");
	
    return 0;
}

int level7_ui(void)
{
    printf("No internet connection.\n");
    printf("Server will be terminated.\n");
    printf("Press \"ESC\" to exit.\n");
    system("notify-send TESTFARM \"No internet connection, server will be terminated\" -t 500");
    return 0;
}

int get_ip()
{
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
		return -1;

	close(fd);

	if (strlen((const char*) inet_ntoa(&ifr.ifr_addr)) <= 0)
		return -1;

	return 0;
}

void check_network(void)
{
//	if (system("ping -c1 192.168.3.1 ") != 0)
		if(get_ip()<0)
			ui_level = 7;
}

void* ui_handler(void* data)
{
    while(1)
    {
        usleep(UPDATE_RATE_SEC*100000);
        if(screen_changes == 0) break;      //TODO: limit display activity
        else
        {
            screen_changes = 1;
            check_network();
            clear_screen();
            
            switch(ui_level)
            {
                case 1: level1_ui();break;      //device_farm
                case 2: level2_ui();break;      //device_specific
                case 3: level3_ui();break;      //
                case 4: level4_ui();break;      //update file list
                case 5: level5_ui();break;      //adb mode
                case 6: level6_ui();break;      //device_health
                case 7: level7_ui();break;      //internet status
                default: printf("Page Error\n");
            }
        }
    }
}

/***********************************************************************/

//randy snippet
char wait_for_keyinput()
{
    key_input = 0;

    struct termios org_opts, new_opts;
    int res=0;
    //-----  store old settings -----------
    res=tcgetattr(STDIN_FILENO, &org_opts);
    //assert(res==0);
    //---- set new terminal parms --------
    memcpy(&new_opts, &org_opts, sizeof(new_opts));
    new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
    key_input=getchar();
//    DPRINTF("you pressed: %c\n", key_input);

    //------  restore old settings ---------
    res=tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
    //assert(res==0);

    screen_changes = 1;
    return key_input;
}

int level1_input(int command)
{
    updateList = 0;
    switch(command)
    {
        case ' ': 
            switch(ui_level)
            {
                case 1:
                    page_number++;
                    if(page_number>=(MAX_DEVICE/MAX_DISPLAY_LINE))
                        page_number = 0;
                    set_page_number(page_number);
                break;
            }
            break;
        case 'a' ... 'z':
            device_index = (page_number + 1) * (command - 'a');
         	id = command;
            ui_level = 2;
            break;
        case KEY_ESC:    //ESC key
            return command;
        case '0' ... '9':
        case '\n':            
            break;
        default:
            DPRINTF("unhandled key %d\n", command);
            return KEY_ESC;
    }
    return 0;
}
int level2_input(int command)
{
    switch(command)
    {
        case ' ': 
            ui_level = 1;
            break;
        case 'a' ... 'b':
            ui_level = 3;
            break;
        case 'c' ... 'i':
            //DPRINTF("sending command %c to client\n", command);
            if(command == 'e')  //update firmware
            {
            	strcpy(active_device[device_index].replycommand, replies[command - 'a']); 
                ui_level = 4;
            }
            if(command == 'h')  //adb mode
            {
            	ui_level = 5;
    		}
            if(command == 'g')
            {
            	strcpy(active_device[device_index].replycommand, replies[command - 'a']); 
            	ui_level = 6;
            }
            break;
        case '0' ... '9':
        case '\n':            
            break;
        default:
            DPRINTF("unhandled key %d\n", command);
            return KEY_ESC;
    }
    return 0;
}

//shutdown and reboot
int level3_input(int command)
{
    switch(command){
        case ' ':
            ui_level = 2;
            break;
        case 'a' ... 'b': 
            strcpy(active_device[device_index].replycommand, replies[command - 'a']); 
            ui_level = 1;
            break;
        case '0' ... '9':
        case '\n':            
            break;
        default:
            DPRINTF("unhandled key %d\n", command);
            return KEY_ESC;
    }

    return 0;
}
//choose update file
int level4_input(int command)
{
    //char update_file[MAX_BUFFER] = "NTEK_UPDATE_1215141733.img:65078512345:8976112233445566:";
    int fileCount = create_update_list();
    char ceiling = 'a' + fileCount - 1;
    
    if (command > ceiling && command <= 'z'){
        printf("your choice is beyond our options\n");
        return 0;
    }

    switch(command){
        case ' ':
            ui_level = 2;
            break;
        case 'a' ... 'z': 
            //sprintf(update_file, "%s:%s", files[command - 'a'], checksum[command - 'a']);
            strcpy(active_device[device_index].replycommand2, update_file[command - 'a']);
            ui_level = 1;
            break;
        case '0':
            strcpy(active_device[device_index].replycommand2, "cancel");
            ui_level = 2;
            break;
        case '1' ... '9':
        case '\n':            
            break;
        default:
            DPRINTF("unhandled key %d %c\n", command, ceiling);
            return KEY_ESC;
    }

    return 0;
}

int level5_input(int command)
{
    switch(command){
        case ' ':
            ui_level = 2;
            break;
        case 'a': 
        	strcpy(active_device[device_index].replycommand, replies['h' - 'a']); 
            strcpy(active_device[device_index].replycommand2,"0");
            ui_level = 1;
            break;
        case 'b': 
        	strcpy(active_device[device_index].replycommand, replies['h' - 'a']); 
            strcpy(active_device[device_index].replycommand2,"1");
            ui_level = 1;
            break;
        case '1' ... '9':
        case '\n':   
        default:
        	DPRINTF("unhandled key %d\n", command);
        	return KEY_ESC;
    }
    return 0;
}

//TODO:
int level6_input(int command)
{
	if((completeFlag) || (stopFlag))
	{
		switch(command){
        	case ' ':
        		ui_level = 2;
        		break;
        	default:
        		DPRINTF("unhandled key %d\n", command);
        		return KEY_ESC;
		}
    }
    return 0;
}

int level7_input(int command)
{
	key_input = KEY_ESC;
	return KEY_ESC;
}

int user_input_handler(char input)
{
    switch(ui_level)
    {
        case 1: level1_input(input); break;
        case 2: level2_input(input); break;
        case 3: level3_input(input); break;
        case 4: level4_input(input); break;
        case 5: level5_input(input); break;
        case 6: level6_input(input); break;
        case 7: level7_input(input); break;
    }
}

/***************************************************/
//socket handling
/***************************************************/
void clear_params(void)
{
	printf("%s\n",__func__);
	memset(msg_cmd, 0, MAX_MSG_CMD);
	memset(msg_param, 0, sizeof(msg_param[0][0]) * MAX_EXPECTED_PARAMS * MAX_MSG); 
}

void start_socket_service()
{
	init_socket_server();
	set_socket_param();
}

void wait_socket_session()
{
	wait_socket_server_session();
}

void rcv_msg(char* msgbuff)
{
	memset(msg_socket_buff, 0, MAX_MSG);
	get_socket_message(msgbuff);
}

int countParams(char* s, char c)
{
    return *s == '\0' 
                ? 0 : countParams(s + 1, c) + (*s == c);
}

int msg_parser(char* msgbuff)
{
    /*parse string msg with dynamic parameters
     *see ktabmidid for ex. in amlcompleteFlag
     * */
    int i, count;
    int ret;
    i = count = ret = 0;
    char *buff;
    clear_params();
    count = countParams(msgbuff,':');
    
    buff = strtok(msgbuff, ":");
    if((buff == NULL) || (strncmp(header, buff,8) != 0)){
        	goto error1;
	}
	
	//parse string message
	for(i=0; i<count; i++)
	{
		buff = strtok(NULL, ":");
		if(strncmp("END", buff,3) == 0)
        {
            i = count;
            break;
        }
        if(i == 0)
            strcpy(msg_cmd, buff);
        else
            strcpy(msg_param[i], buff);
    }

    //check if valid message
	for(i=0; available_commands[i]!=NULL; i++){
		if(strncmp(msg_cmd, available_commands[i],strlen(msg_cmd)) == 0)
		return i; //return command index
	}

	error1:
	//DPRINTF("Invalid msg\n");
	printf("INVALID\n");
	return -1;

}

int reply_msg(char* reply1, char* reply2)
{
	char reply_buff[MAX_MSG];
	memset(reply_buff, 0, MAX_MSG);
	sprintf(reply_buff, "TESTFARM:ACK:%s:%s:END", reply1, reply2);
	send_socket_message(reply_buff);
	return 0;
}

int msg_handler(char* msgbuff)
{
	time_t t = time(NULL);
	struct tm tm;
	char buff[MAX_MSG];
	char target_path[MAX_MSG];
	int i,x, param;
	int cmd = 0;
	char position[1000];
	int msec = 0;

    char mac[MAX_MSG];
    char *ip;
    char *update;
    char *ver;
    char **client_info;
    char *mac_client;
    char *getFile;
    char *buffer;
    int filesize, ret;
    int cameraFlag;
    
    
	memset(buff, 0, MAX_MSG);
	memset(target_path, 0, MAX_MSG);

	if((cmd = msg_parser(msgbuff)) == -1)
		goto error2;

//	printf("%s cmd=%d\n",msg_cmd, cmd);
    switch(cmd){
        case 0: //connect, get mac addr, ip,
            client_info = malloc(sizeof(char *) * MAX_PARAM);
            memset(mac, 0, MAX_MSG);
            sprintf(mac,"%s:%s:%s:%s:%s:%s",msg_param[1],msg_param[2],msg_param[3],msg_param[4],msg_param[5],msg_param[6]);
            client_info[0]      =   mac;
            client_info[1]      =   msg_param[7];
            client_info[2]      =   msg_param[8];
            client_info[3]      =   msg_param[9];

        	printf("===get mac addr, ip===\n");
            add_data(client_info);
        	printf("added 1\n");
            free(client_info);
            break;
        case 1: //device info
        	printf("===device_info===\n");
            client_info = malloc(sizeof(char *) * MAX_PARAM);
            free(client_info);
            
        	hardware = malloc(sizeof(char *)* MAX_MSG);
        	buffer = malloc(sizeof(char *)* MAX_MSG);
        	
        	stopFlag = 0;
        	receivedFlag = 0;
        	completeFlag = 1;
        	cntHardware = 0;
        	cameraFlag = 0;
        	
        	memset(hardware, 0, MAX_MSG);
        	memset(buffer, 0, MAX_MSG);
        	for(i = 0; i < 10; i++)
        	{
        		if(strlen(msg_param[i+1]))
        		{
        			buffer = msg_param[i+1];
        			hardware[i] = strdup(buffer);
        			cntHardware = i;
        			receivedFlag = 1;
        			printf("hardware[%d]: %s\n",i, hardware[i]);
        		}
        	}
            reply_msg("transfer","now");
        	break;
        	
        case 2:
        	printf("===SEND_FILE===\n");
        	client_info = malloc(sizeof(char *) * MAX_PARAM);
        	free(client_info);
        	
        	time_t now = time(NULL);
        	struct tm *t = localtime(&now);
        	ret = 0;
        	completeFlag = 0;
        	
        	mac_client = malloc(sizeof(char *)* MAX_MSG);
        	getFile = malloc(sizeof(char *)* MAX_MSG);
        	
        	filesize = atoi(msg_param[7]);
        	printf("===filesize: %d===\n",filesize);
        	
        	if(strcmp(msg_param[1],"camera")==0)
        		if(strcmp(msg_param[1+1],"error")==0){
        			stopFlag = 1;
        			completeFlag = 1;
        			ui_level = 1;
        			break;
        		}
        	
        	memset(mac_client, 0, MAX_MSG);
        	sprintf(mac_client,"%s:%s:%s:%s:%s:%s",msg_param[1],msg_param[2],msg_param[3],msg_param[4],msg_param[5],msg_param[6]);
        	
        	memset(getFile, 0, MAX_MSG);
        	sprintf(getFile,"%s_%d%d%d.jpg",mac_client,t->tm_mon+1,t->tm_mday,t->tm_year+1900);
        	reply_msg("received","file");
        	
        	ret = get_file(getFile, filesize);
        	if(ret != 0){
        		printf("error in file transfer.\n");
//	   			reply_msg("transfer","Error");
        		stopFlag = 1;
        		completeFlag = 1;
        		ui_level = 1;
        		break;
        	}
        	completeFlag = 1;
        	receivedFlag = 0;
        	ui_level = 1;
        	printf("File received.\n");
        	free(hardware);
        	break;
            
        default:
            goto error2;
            break;
    }

    return 0;
    
    error2:
        reply_msg("INVALID", "MSG");
        return -1;
}

void close_socket_session()
{
	close_socket_server_session();
}

void stop_socket_service()
{
	end_socket_server();
}

void* socket_handler(void* arg)
{
    start_socket_service();
    while(1)
    {
        wait_socket_session();
        rcv_msg(msg_socket_buff);
       printf("-->  %s %s\n", __func__, msg_socket_buff);
        msg_handler(msg_socket_buff);
       printf("--> before closing\n");
        close_socket_session();
        sleep(1);
    }
    printf("stopping socket\n");
    stop_socket_service();
    return 0;
}

/***********************************************************************/

int main(int argc, char *argv[]) 
{
    int dont_exit= 1;
    pthread_t pid_socket;
    pthread_t pid_ui;

    pthread_create(&pid_socket, NULL, &socket_handler,  NULL);
    pthread_create(&pid_ui,     NULL, &ui_handler,      NULL);

    while(dont_exit != KEY_ESC)      //ESC key
    {
        dont_exit = user_input_handler(wait_for_keyinput());
        //sleep(UPDATE_RATE_SEC);
    }

    printf("End TestFarm\n");
	return 0;
}
