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
	action[msgsize-1] = '\0';
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

void getResponseUpdate(int Socket){
        char rBuffer[BUFFERSIZE];
        recv(Socket, rBuffer, BUFFERSIZE-1, 0);
        printf("%s", rBuffer);
	close(Socket);
}

void getResponseDelete(int Socket){
        char rBuffer[BUFFERSIZE];
        recv(Socket, rBuffer, BUFFERSIZE-1, 0);
        printf("%s", rBuffer);
	close(Socket);
}

void getResponseRead(int Socket){
	int parseAction = 0;
	int filesize = 0;
	char filename[64];;
	filename[63] = '\0'; 
	unsigned int totalFileBytesReceived = 0;
	
	char rBuffer[BUFFERSIZE];
	while(TRUE){
		int recvMsgSize = recv(Socket, rBuffer, BUFFERSIZE-1, MSG_DONTWAIT);
		if (recvMsgSize < 0 && errno == EAGAIN) {
                        /* no data for now, call back when the socket is readable */
                        continue;
                }
		if (parseAction == 0){
			//save buffer for later use
			char *rBufferSave = strdup(rBuffer);
			if (rBufferSave == NULL){
				rc_check(12, "strdup() failed!");
			}
			
			char *token = strtok( rBuffer, " \n" );
			if (!strncmp(token, "no", 2)){
				printf("no such file\n");
				exit(0);
			}
			printf("%s ", token);
			token = strtok( NULL, " \n" );
			strncpy(filename, token, strlen(token));
			printf("%s ", filename);
			token = strtok( NULL, " \n" );
			filesize = atoi(token);
			printf("%d\n", filesize);
			
			int t;
			for (t = 0; t < 63; t++){
				if (rBufferSave[t] == '\n'){
					t++;
					break;
				} 
			}
			if (recvMsgSize == t){
				 
			}else{
				char *recvBuffer =(char *) malloc(sizeof(char) * 64);
				if (recvBuffer == NULL){
					rc_check(12, "realloc() failed!");
				}
				totalFileBytesReceived = sizeof(rBuffer)-t-1;
				strncpy(recvBuffer, rBuffer+t, sizeof(rBuffer)-t-1);
				printf("%s", recvBuffer);
				free(recvBuffer);
			}
			parseAction = 1;
		}else{
			printf("%s", rBuffer);
			totalFileBytesReceived += recvMsgSize;
               		if (filesize == totalFileBytesReceived){
               	 		break;
                	}
		}
		memset(rBuffer, 0, sizeof(rBuffer));

	}
	close(Socket);
}

void getResponseList(int Socket){
	int parseAction = 0;
	int numberoffiles = 0;
	int filecounter = 0;
	char rBuffer[BUFFERSIZE];
	while(TRUE){
		int size = recv(Socket, rBuffer, BUFFERSIZE-1, MSG_DONTWAIT); 
		if (size < 0 && errno == EAGAIN) {
			/* no data for now, call back when the socket is readable */
			continue;
		}
		if (parseAction == 0){
			char *rBufferSave = strdup(rBuffer);
			char *token = strtok( rBuffer, " \n" );
			printf("%s ", token);
			token = strtok( NULL, " \n" );
			numberoffiles = atoi(token);
			printf("%d\n", numberoffiles);
			if (numberoffiles == 0){
				break;
			}
			while (token != NULL){
				token = strtok( NULL, " \n" );
				if (token != NULL){
					filecounter++;
					printf("%s\n", token);
				}
			}
			//filecounter++;
			 
			int t;
                        for (t = 0; t < 63; t++){
                                if (rBufferSave[t] == '\n'){
                                        t++;
                                        break;
                                }
                        }
			//printf("saveeeme(%d)-t(%d)\n",  size, t);
			if (numberoffiles == t){
				//printf("not equal(%s)\n", rBuffer);
				break;
			}else{
				char *recvBuffer = (char *) malloc(sizeof(char) * 64);
				if (recvBuffer == NULL){
					
				}
				strncpy(recvBuffer, rBuffer+t, sizeof(rBuffer)-t-1);
				printf("%s", recvBuffer);
				 
			}
			parseAction = 1;
			
		}else{
			// count \n
			int t;
			for (t = 0; t < 63; t++){
				if (rBuffer[t] == '\n'){
					filecounter++;
				}
			}
			
			printf("%s", rBuffer);
			//filecounter++;
			//printf("%d-%d\n", numberoffiles, filecounter);
			if (numberoffiles == filecounter){
				break;
			}
		}
		memset(rBuffer, 0, sizeof(rBuffer));
	}
	close(Socket);
}

void getResponseCreate(int Socket){
	char rBuffer[BUFFERSIZE];
	while(TRUE){
                int size = recv(Socket, rBuffer, BUFFERSIZE-1, MSG_DONTWAIT);
                if (size < 0 && errno == EAGAIN) {
                        /* no data for now, call back when the socket is readable */
                        continue;
                }
		printf("%s", rBuffer);
		break;
	}
	
	close(Socket);
}


