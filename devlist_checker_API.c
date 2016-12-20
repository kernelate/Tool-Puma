
#include<stdio.h>
#include <string.h>
#include "devlist_checker_API.h"

#define MAX_SIZE			10

int open_build_prop(){
	FILE *fopen();
	f_build_prop=fopen(BUILD_PROP_DIR,"r");
	if (f_build_prop == NULL) {//failure to open file		
		return -1;
	}
	else{
		return 1;
	}
}

int open_sdcard()
{
	FILE *fopen();
	f_build_prop=fopen(BUILD_PROP_DIR,"r");
	if (f_build_prop == NULL) {//failure to open file		
		return -1;
	}
	else{
		return 1;
	}
}

char** search_for_hardware(void){

	char line[100];
	char devicename[100];
	char buff[100];
	char **hardware;
	char *buffer;
	char tempfile[100];
	
//	char devicename0[100];
	int x = 0;
	int ret=0;	
	
	hardware = malloc(sizeof(char *) * MAX_SIZE);
	buffer = malloc(sizeof(char *) * 100);
	
	ret=open_build_prop();
	
	if(ret<1)
	{
		printf("can't open build.prop.\n");
		return "none";
	}
	
	memset(hardware, 0, MAX_SIZE);
	memset(buffer, 0 , 100);
	while(fgets(line,100,f_build_prop)!=NULL){
		if(strncmp(line,HARDWARE_HEADER, strlen(HARDWARE_HEADER))==0){
			strcpy(buff,line+17);
			buffer= strtok(buff,"=");
			strcpy(tempfile,buffer);
			hardware[x] = strdup(tempfile);
			printf("hardware[%d]: %s\n",x,hardware[x]);
			x++;
		}
	}
	close_build_prop();
	hardware[x] = "END";
	printf("Close build.prop\n");
	return hardware;
}

void close_build_prop(){
	fclose(f_build_prop);
}

//int check_hardware(char* device){
//	int ret=0;	
//	ret=open_build_prop();
//	if (ret ==1){
//
//		ret=	search_for_hardware(device);
//	}
//	else{
//		printf("failed to open file");
//		return 0;
//	}
//
//	close_build_prop();
//	return 1;
//}

/*for checking purposes only
int main(){	
	int codec=120;
	codec=check_hardware("codec");
	printf("codec:%d\n",codec);
	return 0;
}
//*/
