#ifndef __DEVLIST_CHECKER_API_H__
#define __DEVLIST_CHECKER_API_H__

#define BUILD_PROP_DIR  				"/system/build.prop"
#define HARDWARE_HEADER		"ro.ntek.hardware."
//#define MAX_LENGTH			100	


//char hardware[MAX_SIZE][MAX_LENGTH];

FILE *f_build_prop;

//Function Prototype
int open_build_prop();

char** search_for_hardware(void);

void close_build_prop(); 

int check_hardware(char* device); // use this only

#endif //__CLIENT_H__

