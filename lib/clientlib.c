#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "genlib.h"
#include "clientlib.h"
#include "math.h"

#define CLIENTLIB
#define BUFFERSIZE 64
#define TRUE 1

void f_list(int Socket){
	char *action = malloc(sizeof(char) * 6);
	action[5] = '\0';
        snprintf(action, 5, "%s", "list\n");
        send(Socket, action, 5, 0);
	free(action);
}

void f_create(int Socket, char *filename){
	
	FILE *fp;
	fp = fopen(filename, "rb");
	if (fp == NULL){
		rc_check(-1, "fopen() failed!");
	}
	//get file size
	fseek(fp, 0L, SEEK_END);
	unsigned long long sz = ftell(fp);
	
	int szl =  (int)log10(sz)+1;
	int msgsize = 7 + strlen(filename) + 1 + szl + 2;
	char *action = malloc ( sizeof(char) * msgsize);
						// 7           11        1  4     1 = 24
	snprintf(action, msgsize, "%s%s%s%llu%s", "create ", filename, " ", sz, "\n");
	send(Socket, action, msgsize-1, 0);
	free(action);
	 
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
}

void f_read(int Socket, char *filename){
	
	int msgsize = 5+ strlen(filename) + 1 + 2;
	char *action = malloc(sizeof(char) * msgsize);
	action[msgsize-1] = '\0';
	snprintf(action, msgsize, "%s%s%s", "read ", filename, "\n");
	send(Socket, action, msgsize-1, 0);
	free(action);
}

void f_update(int Socket, char *localfilename, char *remotefilename){
	 
	FILE *fp;
        fp = fopen(localfilename, "rb");
	if (fp == NULL){
		rc_check(-1, "fopen() failed!");
	}
        //get file size
        fseek(fp, 0L, SEEK_END);
        unsigned long long sz = ftell(fp);

	int szl =  (int)log10(sz)+1;
	int msgsize = 7 + strlen(remotefilename) + 1 + szl + 2;
        char *action = malloc ( sizeof(char) * msgsize);
	action[msgsize-1] = '\0';
        snprintf(action, msgsize, "%s%s%s%llu%s", "update ", remotefilename, " ", sz, "\n");
        send(Socket, action, msgsize-1, 0);
	free(action);

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
}

void f_delete(int Socket, char *remotefilename){
	
	int msgsize = 7 + strlen(remotefilename) + 2;
	
	char *action = malloc ( sizeof(char) * msgsize);
	action[msgsize] = '\0';
        snprintf(action, msgsize, "%s%s%s", "delete ", remotefilename, "\n");
        send(Socket, action, msgsize-1, 0);
	
}

void getresponse(char *type, int Socket){
	unsigned int totalBytesReceived;
        unsigned int totalFileBytesReceived;
	
	totalBytesReceived = 0;
        totalFileBytesReceived = 0;
	
	char *recvBuffer = (char *) malloc(sizeof(char) * 64);
	 if (recvBuffer == NULL){
                rc_check(12, "malloc() failed!");
        }
	 
	//helper to parse first line received
	int parseAction = 0;
	int numberoffiles = 0;
	int numberfilesCounter = 0;
	char messagesize[64];
	messagesize[63] = '\0';
	char filename[64];
	filename[63] = '\0';
        while(TRUE){
                char rBuffer[BUFFERSIZE];
                int recvMsgSize = recv(Socket, rBuffer, BUFFERSIZE-1, 0);
                totalBytesReceived += recvMsgSize;
		if (parseAction == 0){
			if (!strncmp(type, "list", 4) || !strncmp(type, "read", 4)){
				char separator[]   = " \n";
                       	 	char *token;
				token = strtok( rBuffer, separator );
				int counter = 0;
				int escape = 0; 
				while( token != NULL ){
					if (counter == 0){
						if ((!strncmp(type, "read", 4)) && (!strncmp(token, "no", 4))){
							escape = 1;
							printf("no such file\n");
							break;
						}
					}else if (counter ==1){
						if (!strncmp(type, "list", 4)){
							numberoffiles = atoi(token);
							printf("ACK %d\n", numberoffiles);
							if (numberoffiles == 0){
								escape = 1;
								break;
							}
						}else if (!strncmp(type, "read", 4)){
							snprintf(filename, sizeof(filename), "%s", token);
						}
					}else if (counter == 2){
						snprintf(messagesize, sizeof(messagesize), "%s", token);
						printf("FILECONTENT %s %s\n", filename, messagesize);
					}
					counter++;
                       	         	token = strtok(NULL, separator );
				}
				parseAction = 1;
				if (escape == 1){
					break;
				}
			}else{
				printf("%s", rBuffer);
				break;
			}
 
		}else{
			
			if (!strncmp(type, "read", 4)){
				printf("%s", rBuffer);
				totalFileBytesReceived += recvMsgSize;
				if (atoi(messagesize) == totalFileBytesReceived){
					break;
				}
			}else if (!strncmp(type, "list", 4)){
				printf("%s", rBuffer);
				numberfilesCounter++; 
				if (numberoffiles == numberfilesCounter){
					break;
				}
				 
			}
		}
		memset(rBuffer, 0, sizeof(rBuffer));
	}
	
}
