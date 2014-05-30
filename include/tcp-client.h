#ifndef TCPCLIENT
#define TCPCLIENT

/*
used by client
*/

int serverSocket; /*Server socket descriptor*/
int clientSocket; /*Client socket descriptor*/

struct sockaddr_in serverAddr;
struct sockaddr_in clientAddr;

unsigned int serverAddrLen;

unsigned short serverPort;
unsigned short clientPort;


int initConn ();
#endif
