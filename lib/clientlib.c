#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "genlib.h"
#include "clientlib.h"


//#define CHUNKSIZE 64
#define BUFFERSIZE 64
#define TRUE 1


void f_list(int Socket){
        char actiontype[63];
        actiontype[62] = '\0';
        snprintf(actiontype, sizeof(actiontype), "%s", "list");
        send(Socket, actiontype, sizeof(actiontype), 0);

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
	
	//int actionsize = (int) (sizeof("create ") + sizeof(filename) + 1 + sizeof(sz) + 1);
	char action[63];
	snprintf(action, sizeof action, "%s%s%s%llu%s", "create ", filename, " ", sz, "\n");
	send(Socket, action, sizeof(action), 0);
	 
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
	
	//int actionsize = (int) (sizeof("read ") + sizeof(filename) + 1);
	char action[63];
	snprintf(action, sizeof action, "%s%s%s", "read ", filename, "\n");
	send(Socket, action, sizeof(action), 0);
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

        //int actionsize = (int) (sizeof("update ") + sizeof(remotefilename) + 1 + sizeof(sz) + 1);
        char action[63];
        snprintf(action, sizeof action, "%s%s%s%llu%s", "update ", remotefilename, " ", sz, "\n");
        send(Socket, action, sizeof(action), 0);

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
	
	char action[63];
        snprintf(action, sizeof action, "%s%s%s", "delete ", remotefilename, "\n");
        send(Socket, action, sizeof(action), 0);

}

void getresponse(char *type, int Socket){
	unsigned int totalBytesReceived;
        unsigned int totalFileBytesReceived;
	
	//char zeile1[64];
        //char messagesize[64];
        //char zeile3[64];
	
	totalBytesReceived = 0;
        totalFileBytesReceived = 0;
	
	char *recvBuffer = (char *) malloc(sizeof(char) * 64);
	 if (recvBuffer == NULL){
                rc_check(12, "malloc() failed!");
        }
        //char filename[64];
        //char filesize[64];
	 
	int parseAction = 0;
	int numberoffiles = 0;
	int numberfilesCounter = 0;
	char messagesize[64];
	messagesize[63] = '\0';
	char filename[64];
	filename[63] = '\0';
	//memset(filename, 0, sizeof(filename));
        while(TRUE){
                char rBuffer[BUFFERSIZE];
                int recvMsgSize = recv(Socket, rBuffer, BUFFERSIZE-1, 0);
                totalBytesReceived += recvMsgSize;
		if (parseAction == 0){
			if (!strncmp(type, "list", 4) || !strncmp(type, "read", 4)){
				char separator[]   = " \n";
                       	 	char *token;
                       	 	//char *opt1 = "";
                       	 	//char *opt2 = "";
                       	 	//char *opt3 = "";
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
							//printf("looki\n");
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
			//printf(":messagesize:%s:%u:\n", messagesize, totalFileBytesReceived); 
			//printf("numberoffiles(%d), numberfilesCounter(%d)", numberoffiles, numberfilesCounter);
			
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
