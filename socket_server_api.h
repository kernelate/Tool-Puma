#ifndef __SOCKET_JNI_H__
#define __SOCKET_JNI_H__

#define MAX_BUFF        255
#define TARGET_PORT     5001
#define MAX_MSG_LEN     0
#define LAST_CHAR       '\xFF\xD9'
#define FILENAME "/home/ntekcom11/testfarm/received.jpg"

void init_socket_server();
int set_socket_param();
int wait_socket_server_session();
int get_socket_message(char* destination_buffer);
int get_file(char* filename, int size);
int send_socket_message(char* message);
void close_socket_server_session();
void end_socket_server();


#endif //__SOCKET_JNI_H__
