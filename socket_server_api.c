/*  NOTE:
 *      This is for Native C service for android OS.
 *      socket server for autoloader launcher, objective
 *      of this socket api is to provide connectivity
 *      between android app and linux binaries
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h> 
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/stat.h>

#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "socket_server_api.h"
#include "common.h"

#define MAX_MSG      		255


int socketfd = 0, connfd;     
struct sockaddr_in server_addr;

char buff[MAX_BUFF];

void init_socket_server()
{
    //printf("init socket server\n");
    memset(&server_addr, 0, sizeof(server_addr));
    memset(buff, 0, MAX_BUFF);
    atexit(end_socket_server);              //TODO:check if this is working otherwise remove
    signal(SIGINT, end_socket_server);      //also this one
}

int set_socket_param()
{
    int yes = 1; //TODO: whats this for, related to setsockopt()
    //printf("set socket param\n");
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0){
        //DPRINTF("Error creating socket\n");
        //printf("\terror creating socket\n");
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(TARGET_PORT);     //port no
    //printf("\tsuccess\n");
    //This should allow the socket to be reused after recreation of socket
    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        printf("\terror on setsockopt\n");
        //DPRINTF("Error on setsockopt\n");
        return -1;
    }

    if(bind(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("\t Error on binding\n");
        //DPRINTF("Error on binding\n");
        return -1;
    }

    listen(socketfd, 10); 
    //printf("listening\n");

    return 0;
}

int wait_socket_server_session()
{
//   	printf("waiting\nreceived\n");
//    DPRINTF("Waiting for connection \n");
    connfd = accept(socketfd, (struct sockaddr*)NULL, NULL); 
    printf("Connection received \n");//had wrong spelling of received.
    return 0;
}

int get_socket_message(char* destination_buffer)
{
    int ret;
    ret = read(connfd, destination_buffer, MAX_BUFF);
    //DPRINTF("rcv : %s\n", destination_buffer);
    printf("get_socket_message\n\trcv: %s\n",destination_buffer);
    return ret;
}

//=========get file from client=========
//TODO:
int get_file(char* file, int size)
{
	char recvBUFF[size];
	FILE * recvFILE;
	int received = 0;
	int bytes, x, cnt,ret;
	char footer[]= "TESTFARM:CMD_SEND_FILE:08:d8:33:92:e7:60:35871:END";
	char compare[10];
	char *pointer;
	 
	x=0;
	cnt=-1;
	printf("===filename: %s===\n",file);
	recvFILE = fopen(file, "w");
	if (recvFILE == NULL)
	{
		printf("no file exist.\n");
		send_socket_message("TESTFARM:ACK:transfer:error:END");
		return -1;
	}

	memset(recvBUFF, 0, sizeof(recvBUFF));
	printf("writing a file\n");
	
	while(x < size ) {//1, sizeof(recvBUFF)
		bytes = recv(connfd, recvBUFF, 1, 0);
		fwrite(recvBUFF , sizeof(char), bytes, recvFILE );
		printf("x:%d|--%x--\n",x,recvBUFF[0]);
		recvBUFF[0] = 0;
		cnt = 0;
		x++;
	}
	printf("File closed.\n");
	fclose(recvFILE);
	
	if(cnt!=0)
		send_socket_message("TESTFARM:ACK:transfer:error:END");
	else
		send_socket_message("TESTFARM:ACK:transfer:complete:END");
	
	return cnt;
}

int send_socket_message(char* message)
{
    //DPRINTF("send : %s\n", message);
    printf("send_socket_message\n\tsend : %s\n",message);
    return write(connfd, message, strlen(message));
    
}

void close_socket_server_session()
{
    printf("close_socket_server_session\n");
    close(connfd);
}

void end_socket_server()
{
    //DPRINTF("%s\n", __func__);
    printf("end_socket_server\n");
    if(socketfd != 0)
    {
       // DPRINTF("TERMINATING socket connection \n");
        close(socketfd);
        socketfd = 0;
        //printf("\tterminating socket connection\n");
    }
}
