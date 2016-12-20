#ifndef __HARDWARE_CHECKER_H__
#define __HARDWARE_CHECKER_H__

#define MAX_LEN         20
#define MAX_CHAR		50

typedef struct {
	int special_function_register;
	int value;
} DTDRIVERS_DATA;

#define FAST				_IOW('p', 4, DTDRIVERS_DATA)
#define SLOW	 			_IOW('p', 3, DTDRIVERS_DATA)
#define EC_ON				_IO('z', 0xa1)
/**********************************************/

/*  getModel()-              open a uart device
 *  @param          -       uart parameters, should be initialized
 *  retval          -   0   -   return file descriptor for @rp
 *                  -   -1  -   invalid device
 */
int led_check(void);
int camera_check(void);
int e_canceller(void);
char* check_for_pictures(void);

#endif //__CLIENT_H__