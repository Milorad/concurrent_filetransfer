#include <stdio.h>
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep
#include "include/genlib.h"
#include "include/tcp-client.h"
#include "include/clientlib.h"
#include <netinet/in.h>		//in_addr
#include <string.h>

#define TRUE 1
#define FALSE 0

#define CHUNKSIZE 512

int Socket;

struct file_create {
	char size[64];
	char action[64];
} ff_create;

int main(int argc, char *argv[]) {
	usageclientparse(argc, argv);
	 
	//open socket
	Socket = initConn();
	//send different requests to the server and get the response
	if (!strcmp(type, "list")){
		f_list(Socket);
		getResponseList(Socket);
	}else if(!strcmp(type, "create")){
		f_create(Socket, localfilename);
		getResponseCreate(Socket);
	}else if(!strcmp(type, "read")){
		f_read(Socket, remotefilename);
		getResponseRead(Socket);
	}else if(!strcmp(type, "update")){
		f_update(Socket, localfilename, remotefilename);
		getResponseUpdate(Socket);
	}else if(!strcmp(type, "delete")){
		f_delete(Socket, remotefilename);
		getResponseDelete(Socket);
	}
	exit(0);

}
