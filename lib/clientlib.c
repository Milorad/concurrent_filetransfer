#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "genlib.h"


//#define CHUNKSIZE 64
#define BUFFERSIZE 64
#define TRUE 1


void f_list(int Socket){
	//zeile1 : type create
        char actiontype[63];
        actiontype[62] = '\000';
        sprintf(actiontype, "%s", "list");
        send(Socket, actiontype, sizeof(actiontype), 0);

        //zeile2 : list
        //char filenamestr[63];
        //sprintf(filenamestr, "%s", filename);
        //filenamestr[62] = '\000';
        //printf("z2 : %zu\n", sizeof(filenamestr));
        //send(Socket, filenamestr, sizeof(filenamestr), 0); 
}

void f_create(int Socket, char *filename){
	
	FILE *fp;
	fp = fopen(filename, "rb");
	//get file size
	fseek(fp, 0L, SEEK_END);
	unsigned long long sz = ftell(fp);
	
	//zeile1 : type create
	char actiontype[63];
	actiontype[62] = '\000';
	sprintf(actiontype, "%s", "create");
	send(Socket, actiontype, sizeof(actiontype), 0);
	 
	//zeile2 : filename
	char filenamestr[63];
	sprintf(filenamestr, "%s", filename); 
	filenamestr[62] = '\000';
	send(Socket, filenamestr, sizeof(filenamestr), 0);
	 
	//zeile3 : filesize
	char fsizestr[63];
	sprintf(fsizestr, "%lld", sz);
	fsizestr[62] = '\000';
	send(Socket, fsizestr, sizeof(fsizestr), 0);
	
	//zeile4 : undefined for create
	char remotefilename[63];
	sprintf(remotefilename, "%s", "/etc/passwd");
	remotefilename[62] = '\000';
	send(Socket, remotefilename, sizeof(remotefilename), 0);
	 
	//set pointer to the beginning of the file
        fseek(fp, 0L, SEEK_SET);
	
	unsigned int totalbytes;
        totalbytes = 0;
	
	char file[BUFFERSIZE];
        size_t bytes = 0;

	// do not send all at once
	while ( (bytes = fread(file, sizeof(char), BUFFERSIZE, fp)) > 0){
		int offset = 0;
        	int sent;
		while ((sent = send(Socket, file + offset, bytes, 0)) > 0) {
			totalbytes += sent;
                        offset += sent;
                        bytes -= sent;
		}
		
	}
        fclose(fp);
	 
	//unsigned int totalbytesint =  atoll(fsizestr);

}

void f_read(int Socket, char *filename){
	
	//zeile1: type
	char actiontype[63];
        actiontype[62] = '\000';
        sprintf(actiontype, "%s", "read");
        send(Socket, actiontype, sizeof(actiontype), 0);
	
	//zeile2 : filename
        char filenamestr[63];
        sprintf(filenamestr, "%s", filename);
        filenamestr[62] = '\000';
        send(Socket, filenamestr, sizeof(filenamestr), 0);

}

void f_update(int Socket, char *localfilename, char *remotefilename){
	 
	FILE *fp;
        fp = fopen(localfilename, "rb");
        //get file size
        fseek(fp, 0L, SEEK_END);
        unsigned long long sz = ftell(fp);

	//zeile1 : type create
        char actiontype[63];
        actiontype[62] = '\000';
        sprintf(actiontype, "%s", "update");
        send(Socket, actiontype, sizeof(actiontype), 0);

        //zeile2 : filename
        char filenamestr[63];
        sprintf(filenamestr, "%s", remotefilename);
        filenamestr[62] = '\000';
        send(Socket, filenamestr, sizeof(filenamestr), 0);

        //zeile3 : filesize
        char fsizestr[63];
        sprintf(fsizestr, "%lld", sz);
        fsizestr[62] = '\000';
        send(Socket, fsizestr, sizeof(fsizestr), 0);
	
	//zeile4 : undefined for update
        char undefined[63];
        sprintf(undefined, "%s", "undefined");
        undefined[62] = '\000';
        send(Socket, undefined, sizeof(undefined), 0);
	
	//set pointer to the beginning of the file
        fseek(fp, 0L, SEEK_SET);

        unsigned int totalbytes;
        totalbytes = 0;

        char file[BUFFERSIZE];
        size_t bytes = 0;

        // do not send all at once
        while ( (bytes = fread(file, sizeof(char), BUFFERSIZE, fp)) > 0){
                int offset = 0;
                int sent;
                while ((sent = send(Socket, file + offset, bytes, 0)) > 0) {
                        totalbytes += sent;
                        offset += sent;
                        bytes -= sent;
                }

        }
        //unsigned int totalbytesint =  atoll(fsizestr);
        fclose(fp);

}

void f_delete(int Socket, char *remotefilename){
	//zeile1: type
        char actiontype[63];
        actiontype[62] = '\000';
        sprintf(actiontype, "%s", "delete");
        send(Socket, actiontype, sizeof(actiontype), 0);
	
	        //zeile2 : filename
        char filenamestr[63];
        sprintf(filenamestr, "%s", remotefilename);
        filenamestr[62] = '\000';
        send(Socket, filenamestr, sizeof(filenamestr), 0);

}

void getresponse(int Socket){
	unsigned int totalBytesReceived;
	unsigned int totalFileBytesReceived;
	
	char zeile1[64];
	char messagesize[64];	
	char zeile3[64];
	//char zeile4[64];
	totalBytesReceived = 0;
        totalFileBytesReceived = 0;
	char *recvBuffer = (char *) malloc(sizeof(char) * 64);
	if (recvBuffer == NULL){
		rc_check(12, "malloc() failed!");
	}
	  
	while(TRUE){
                char rBuffer[BUFFERSIZE];
                int recvMsgSize = recv(Socket, rBuffer, BUFFERSIZE-1, 0);
		totalBytesReceived += recvMsgSize;
		//printf("msgSize(%d)\n", recvMsgSize);
		if (recvMsgSize == 0){
			break;
		}else{
			if (totalBytesReceived == 63){
				//list/create/read/update/delete
				strcpy(zeile1, rBuffer);
				//printf("zeile1: %s\n", zeile1);
			}else if(totalBytesReceived == 126){
				//content size
				strcpy(messagesize, rBuffer);
				recvBuffer = realloc(recvBuffer, atoi(rBuffer) * sizeof(char));
				if (recvBuffer == NULL){
					rc_check(12, "realloc() failed!");
				}
                                memset(recvBuffer, 0, atoi(rBuffer));
				//printf("messagesize: %s\n", messagesize);
			}else if(totalBytesReceived == 189){
				//list:size of list
				//create:created or exists
				//read:filesize
				//update: updated or no such file or directory
				//delete. deleted or no such file
				strcpy(zeile3, rBuffer);
				//printf("filename: %s\n", rBuffer);
				if (!strcmp(zeile1, "create")){
					//printf("%s %s\n", messagesize, zeile3); 
					//break;
				}else if (!strcmp(zeile1, "update")){
					 
				}else if (!strcmp(zeile1, "delete")){
					 
				}
			}else{
				totalFileBytesReceived += recvMsgSize;
				strcpy(recvBuffer+strlen(recvBuffer), rBuffer);
				//printf("rBuffer: %s\n", rBuffer);
				//printf("size: %s\n", zeile3);
				//printf("totalsize: %d\n", totalFileBytesReceived);
				if (atoi(messagesize) == totalFileBytesReceived){
					break;
				}
			} 
			memset(rBuffer, 0, sizeof(rBuffer));
		}
		
        }
	if (!strcmp(zeile1, "list")){
		printf("Files : %s\n", zeile3);
	}
	printf("%s", recvBuffer);
	free(recvBuffer);
        close(Socket);
}
