

#ifndef TCPSERVER
#define TCPSERVER
#endif


int serverSocket; /*Server socket descriptor*/
int clientSocket; /*Client socket descriptor*/

// declare function to avoid " warning: implicit declaration of function atoi" during compilation
int atoi();

struct sockaddr_in serverAddr;
struct sockaddr_in clientAddr;
unsigned int clientAddrLen;

unsigned short serverPort;
unsigned short clientPort;


int initSocket();
