#ifndef TCPSERVER
#define TCPSERVER

/*
used by server
*/

int serverSocket; /*Server socket descriptor*/
int clientSocket; /*Client socket descriptor*/

struct sockaddr_in serverAddr;
struct sockaddr_in clientAddr;
unsigned int clientAddrLen;

unsigned short serverPort;
unsigned short clientPort;

int initSocket();
#endif
