#include <stdio.h>

#include <netinet/in.h>		/*sockaddr_in*/
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "genlib.h"
#include "tcp-server.h"
#include <string.h>

#define MAXCONNECTIONS 5

int initSocket(){
	serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);	/*open socket*/
	rc_check(serverSocket, "open socket");
	memset(&serverAddr, 0, sizeof(serverAddr));	/*write zeros to the structure*/
	serverAddr.sin_family = AF_INET;                /* Internet address family */
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	serverAddr.sin_port = htons(9999);
	
	int rc;
	rc = bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	rc_check(rc, "bind socket");
	rc = listen(serverSocket, MAXCONNECTIONS);
	rc_check(rc, "listen socket");
	clientAddrLen = sizeof(clientAddr);
	 
	return serverSocket;

}
