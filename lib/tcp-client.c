#include <stdio.h>
#include <netinet/in.h>
#include "genlib.h"
#include "tcp-client.h"
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>  /* sockaddr_in , inet_addr() */
#include <unistd.h>     //sleep

#define TCPCLIENT


int initConn(){
	char *serverIP = "127.0.0.1";
	serverPort = 9999;
	clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	rc_check(clientSocket, "client open socket");
	
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family	= AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);
	serverAddr.sin_port = htons(serverPort);
	
	int rc;
	rc = connect(clientSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	rc_check(rc, "client connect");
	serverAddrLen = sizeof(clientAddr);
	return clientSocket;
}


