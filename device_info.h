#ifndef __DEVICE_INFO_H__
#define __DEVICE_INFO_H__

#define MAX_LEN         20


char verinfo[MAX_LEN];
char mac[MAX_LEN]; 
char ip[MAX_LEN];
char project[MAX_LEN];

/**********************************************/

/*  getModel()-              open a uart device
 *  @param          -       uart parameters, should be initialized
 *  retval          -   0   -   return file descriptor for @rp
 *                  -   -1  -   invalid device
 */
char* getModel(void);

/*  getIP()-              open a uart device
 *  @param          -       uart parameters, should be initialized
 *  retval          -   0   -   return file descriptor for @rp
 *                  -   -1  -   invalid device
 */
int getIP(void);

/*  getMAC()-              open a uart device
 *  @param          -       uart parameters, should be initialized
 *  retval          -   0   -   return file descriptor for @rp
 *                  -   -1  -   invalid device
 */
int getMAC(void);

int connectWifi(void);

int getVersion(void);


#endif //__CLIENT_H__