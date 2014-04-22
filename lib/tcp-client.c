#include <stdio.h>
#include <netinet/in.h>
#include "genlib.h"
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>  /* sockaddr_in , inet_addr() */
#include <unistd.h>             //sleep



#ifndef TCPCLIENT
#define TCPCLIENT
#endif

//int serverSocket; /*Server socket descriptor*/
int Socket; /*Client socket descriptor*/

// declare function to avoid " warning: implicit declaration of function atoi" during compilation
int atoi();

struct sockaddr_in serverAddr;
struct sockaddr_in clientAddr;
unsigned int serverAddrLen;

unsigned short serverPort;
unsigned short clientPort;

int initConn(){
	char *serverIP = "192.168.0.200";
	serverPort = 9999;
	Socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	rc_check(Socket, "client open socket");
	
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family	= AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);
	serverAddr.sin_port = htons(serverPort);
	
	int rc;
	rc = connect(Socket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	rc_check(rc, "client connect");
	serverAddrLen = sizeof(clientAddr);
	return Socket;
}


