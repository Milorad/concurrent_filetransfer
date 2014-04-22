#include <stdio.h>

#include <netinet/in.h>		/*sockaddr_in*/
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "genlib.h"
#include <string.h>

#ifndef TCPSERVER
#define TCPSERVER
#endif

#define MAXCONNECTIONS 5

int serverSocket; /*Server socket descriptor*/
int clientSocket; /*Client socket descriptor*/

// declare function to avoid " warning: implicit declaration of function atoi" during compilation
int atoi();

struct sockaddr_in serverAddr;
struct sockaddr_in clientAddr;
unsigned int clientAddrLen;

unsigned short serverPort;
unsigned short clientPort;

int initSocket(){
	serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);	/*open socket*/
	//printf("check1\n");
	rc_check(serverSocket, "open socket");
	//printf("check2\n");
	memset(&serverAddr, 0, sizeof(serverAddr));	/*write zeros to the structure*/
	serverAddr.sin_family = AF_INET;                /* Internet address family */
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	//serverPort = atoi("9999");			
	serverAddr.sin_port = htons(9999);
	
	int rc;
	rc = bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	rc_check(rc, "bind socket");
	rc = listen(serverSocket, MAXCONNECTIONS);
	rc_check(rc, "listen socket");
	clientAddrLen = sizeof(clientAddr);
	 
	return serverSocket;

}
