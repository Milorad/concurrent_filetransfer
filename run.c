#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep
#include "include/genlib.h"
#include "include/tcp-server.h"
#include "include/sss.h"
#include <netinet/in.h>		//in_addr
#include <string.h> 
#include <signal.h>
#include <regex.h>
#include "math.h"

#define TRUE 1
#define FALSE 0

#define RECVBUFFERSIZE 64
#define SHMSZ 16777216


//current storage shmid
int *storageshmid;	
int storage_shmid;	

//if storage was grown, this is pointer to the previous storage
int *storageshmid_prev;
int storage_shmid_prev;

//semaphore used in case storage grow is necessery
sem_t *sem_value;
int sem_shmid;
int sem_semid;

//size of current storage
int *storemesize;
int storemesize_shmid;

//number of structs which are used
int *storemeused;
int storemeused_shmid;

//size of previous storage
int *storemesizeprev;
int storemesizeprev_shmid;

//generic var to catch return codes
int rc;

//used to stop the server
void signal_stopserver() {
	printf("Shutting down server.\n");
	freeStorageAll();
        rc = shmctl(storemeused_shmid, IPC_RMID, NULL);
	rc_check(rc, "1-shmctl() failed!");
        rc = shmctl(storemesize_shmid, IPC_RMID, NULL);
	rc_check(rc, "2-shmctl() failed!");
        rc = shmctl(storemesizeprev_shmid, IPC_RMID, NULL);
	rc_check(rc, "3-shmctl() failed!");
        rc = shmctl(storage_shmid, IPC_RMID, NULL);
	rc_check(rc, "4-shmctl() failed!");
        rc = shmctl(storage_shmid_prev, IPC_RMID, NULL);
	rc_check(rc, "5-shmctl() failed!");
	rc = shmctl(sem_shmid, IPC_RMID, NULL);
	rc_check(rc, "7-shmctl() failed!");

	exit(0);
}

//initialize vars
void initGlobs(){
	//number of slots used in current storage
	storemeused_shmid= shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0660|SHM_W);
	rc_check(storemeused_shmid, "shmget() failed!");
        storemeused =  (int *) shmat(storemeused_shmid, NULL, 0);
	*storemeused = 0;
	rc = shmdt(storemeused);
	rc_check(rc, "7-shmdt() failed!");
	
	//size of current storage, default 100
	storemesize_shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0660|SHM_W);
	rc_check(storemesize_shmid, "shmget() failed!");
	storemesize =  (int *) shmat(storemesize_shmid, NULL, 0);
	*storemesize = 100;
	rc = shmdt(storemesize);
	rc_check(rc, "8-shmdt() failed!");
	
	// used for grow memory and save size of old storage
	storemesizeprev_shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0660|SHM_W);
        rc_check(storemesizeprev_shmid, "shmget() failed!");
        storemesizeprev =  (int *) shmat(storemesizeprev_shmid, NULL, 0);
        *storemesizeprev = 100;
        rc =shmdt(storemesizeprev);
	rc_check(rc, "9-shmdt() failed!");

	
	// to storage shmid of storage
	storage_shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0660|SHM_W);
	rc_check(storage_shmid, "shmget() failed!");
	storageshmid = (int *) shmat(storage_shmid, NULL, 0);
	*storageshmid = storage_shmid;
	rc = shmdt(storageshmid);
	rc_check(rc, "10-shmdt() failed!");
	 
	//shmid of previous storage
	storage_shmid_prev = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0660|SHM_W);
        rc_check(storage_shmid_prev, "shmget() failed!");
	storageshmid_prev = (int *) shmat(storage_shmid_prev, NULL, 0);
        *storageshmid_prev = storage_shmid_prev;
        rc = shmdt(storageshmid_prev);
	rc_check(rc, "11-shmdt() failed!");

	//semaphore in case storage grow is needed
	sem_shmid = shmget(IPC_PRIVATE, sizeof(sem_t), IPC_CREAT | 0660|SHM_W);
	rc_check(sem_shmid, "4-shmget() failed!");
	sem_value = (sem_t *) shmat(sem_shmid, NULL, 0);
	sem_semid = sem_init(sem_value, 1, 1);
	rc = shmdt(sem_value);
	rc_check(rc, "111-shmdt() failed");
	 
}

int main(int argc, char *argv[]) {
	
	usageserverparse(argc, argv);
	
	//create daemon
	pid_t pidd;
	pidd = fork();
	if (pidd == -1){
		rc_check(-1, "fork() failed");
		
	}else if (pidd == 0){ // CHILD - used as daemon
		//write pid to run.pid
		writepid(getpid());
		 
		initGlobs(); 
		//create serverSocket
		serverSocket = initSocket();
		// needed to stop
		struct sigaction saint;
		saint.sa_handler = signal_stopserver;
		sigemptyset(&saint.sa_mask);
		sigaddset(&saint.sa_mask, SIGTERM);
		saint.sa_flags = 0;
		sigaction(SIGTERM, &saint, NULL);
		
		// init storage. here are pointers to the structs
		initStorageOnce(100);
		initStorage(getStorageshmid(),0);
		 
		unsigned int totalFileBytesReceived;
		while (TRUE){
			clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &clientAddrLen);
			rc_check(clientSocket, "accept socket");
			//new process for new client
			pid_t pid;
			pid = fork();
			if (pid == -1){ //ERROR
				rc_check(-1, "fork() failed");
			}else if(pid == 0){// CHILD
				//check if 80% of current storage is used. If so, grow storage
				reallocStorage(); 
				 
				totalFileBytesReceived = 0;
				//to store clientRequest type:list,create,read,update or delete
				char type[64]; 
				type[63] = '\0';
				memset(type, 0, sizeof(type));
				//store filename client requested
				char filename[64];
				filename[63] = '\0';
				memset(filename, 0, sizeof(filename));
				//store filesize cilent sent
				char filesize[64];
				filesize[63] = '\0';
				memset(filesize, 0, sizeof(filesize));
				//store clients file into heap first. Afterwards it will be copied to shared memory
				char *recvBuffer = (char *) malloc(sizeof(char) * 64);
				if (recvBuffer == NULL){
					rc_check(12, "malloc() failed!");
				}
				 
				//helper to parse first line received from client
				int parseAction = 0;
				while(TRUE){
					char rBuffer[RECVBUFFERSIZE];
					int recvMsgSize = recv(clientSocket, rBuffer, RECVBUFFERSIZE-1, 0);
					 
					if (parseAction == 0){
							/* Client is sending one of the following Requests
							list\n
							create filename length\n
							read filename\n
							update filename length\n
							delete filename\n
							*/
							int t;
							for (t = 0; t < 64; t++){
								if (rBuffer[t] == '\n'){
                                                                        t++;
									break;
                                                                }
							}
							//split line
							char separator[]   = " \n";
							char *token;
							char *opt1 = "";
							char *opt2 = "";
							char *opt3 = "";
							token = strtok( rBuffer, separator );
							int counter = 0;
							while( token != NULL ){
								if (counter == 0){
									opt1 = token;
								}else if(counter == 1){
									opt2 = token;
								}else if(counter == 2){
									opt3 = token;
								}
								token = strtok( NULL, separator );
								counter++;
							}
							//save received values
							snprintf(type, sizeof(opt1), "%s", opt1);
							if (!strncmp(opt1, "list", 4)){
								break; 
							}else if(!strncmp(opt1, "create", 6)){
								snprintf(filename, sizeof(filename), "%s", opt2);
								snprintf(filesize, sizeof(filesize), "%s", opt3);
								recvBuffer  = (char *) realloc(recvBuffer,  sizeof(char) * atoi(filesize)+ 1);
								if (recvBuffer == NULL){
									rc_check(12, "realloc() failed!"); 
								}
							}else if(!strncmp(opt1, "read", 4)){
								snprintf(filename, sizeof(filename), "%s", opt2);
								break;
							}else if(!strncmp(opt1, "update", 6)){
								snprintf(filename, sizeof(filename), "%s", opt2);
								snprintf(filesize, sizeof(filesize), "%s", opt3);
								recvBuffer  = (char *) realloc(recvBuffer,  sizeof(char) * atoi(filesize)+ 1);
								if (recvBuffer == NULL){
									rc_check(12, "realloc() failed!");
								}
							}else if(!strncmp(opt1, "delete", 6)){
								snprintf(filename, sizeof(filename), "%s", opt2);
								break;
							}
							// ==  > rbuffer has only meta date "create filename\n", no content
							if (recvMsgSize == t){

							}else{
								//put stuff after \n into the buffer
								totalFileBytesReceived = sizeof(rBuffer)-t-1;
								strncpy(recvBuffer, rBuffer+t, sizeof(rBuffer)-t-1);
							}
							parseAction = 1;
					}else{
						totalFileBytesReceived += recvMsgSize;
						strncpy(recvBuffer+strlen(recvBuffer), rBuffer,sizeof(rBuffer));
						if (atoi(filesize) == totalFileBytesReceived){
							break;
						}
					}
					memset(rBuffer, 0, sizeof(rBuffer));
				}
				//do job client requested
				if (!strncmp(type, "list", 4)){
					listFiles(clientSocket);
				}else if(!strncmp(type, "create", 6)){
					createFile(clientSocket, recvBuffer, filename, filesize);
				}else if(!strncmp(type, "read", 4)){
					readFile(clientSocket, filename);
				}else if(!strncmp(type, "update", 6)){
					updateFile(clientSocket, recvBuffer, filename, filesize);
				}else if(!strncmp(type, "delete", 6)){
					deleteFile(clientSocket, filename);
				}
				//free(bufferaddr);
				//exit will cleanup all malloc'ed heap! see documentation
				exit(0);
			}		
			
		}
	}else{  //PARENT - daemonize child
		exit(0);
        }

}

//check if file exists first. If it does, update the content of the file
void updateFile(int clientSocket, char *recvBuffer, char *filename, char *filesize){
	unsigned long long fs = atoi(filesize);
	struct storagedef address;
        struct storagedef *addr;
        int i;
	int fexist = 0;
	struct storagedef *storage = (struct storagedef *) shmat(getStorageshmid(), NULL,0);
        for (i=0; i < getStorageSize(); i++){
		address = storage[i];
                addr = (struct storagedef *) shmat(address.shmid, NULL, 0);
		if (!strncmp(addr->filename, filename, sizeof(addr->filename))){
			sem_wait(&addr->sem);
			fexist = FileExists(filename);
			if (fexist == 1){ // file exists and can be updated
				// delete old shm
				rc = shmctl(addr->shmidcontent, IPC_RMID, NULL);
				rc_check(rc, "7-shmctl() failed!");
				// add new one
				int shmidcontent = shmget(IPC_PRIVATE, sizeof(char) *  atoi(filesize) + 20, IPC_CREAT | 0660|SHM_W);
				rc_check(shmidcontent, "shmget() failed!");
				addr->content = (char *) shmat(shmidcontent, NULL,0);
                                addr->shmidcontent = shmidcontent;
				
				//write file from heap to shm
				int i;
                                for (i = 0; i <= atoi(filesize);i++){
                                        addr->content[i] = recvBuffer[i];
                                }
				//set params
				snprintf(addr->filename, sizeof(addr->filename), "%s", filename);
                                addr->filesize = fs;
                                addr->state = 1;
				
				sem_post(&addr->sem);
                                rc = shmdt(addr->content);
				rc_check(rc, "12-shmdt() failed!");
                                rc = shmdt(addr);
				rc_check(rc, "13-shmdt() failed!");

                                break;
                        }else{
				sem_post(&addr->sem);
				rc = shmdt(addr);
				rc_check(rc, "14-shmdt() failed!");
				break;
			}
		}
		rc = shmdt(addr); 
		rc_check(rc, "15-shmdt() failed!");
	}
	rc = shmdt(storage);
	rc_check(rc, "16-shmdt() failed!");
	// send response
	char *response;
	int msgsize;
        if (fexist == 1){
		msgsize = 9;
		response = (char *) malloc(sizeof(char) * msgsize + 1);
		if (response == NULL){
			rc_check(12, "malloc() failed!");
		}
		response[msgsize] = '\0';
		snprintf(response, msgsize, "%s", "updated\n");
        }else{
		msgsize = 14;
		response = (char *) malloc(sizeof(char) * msgsize + 1);
		if (response == NULL){
                        rc_check(12, "malloc() failed!");
                }
		response[msgsize] = '\0';
		snprintf(response, msgsize, "%s", "no such file\n");
        }
	send(clientSocket, response, msgsize, 0);
	free(response);
}

void deleteFile(int clientSocket, char *filename){
	struct storagedef address;
        struct storagedef *addr;
        int fexist = 0;          //does not exist
        int i;
	struct storagedef *storage = (struct storagedef *) shmat(getStorageshmid(), NULL,0);
	for (i=0; i < getStorageSize(); i++){
		address = storage[i];
		addr = (struct storagedef *) shmat(address.shmid, NULL, 0);
		if (!strncmp(addr->filename, filename, sizeof(addr->filename))){
			sem_wait(&addr->sem);
                        fexist = FileExists(filename);
			if (fexist == 1){ // file exists and can be deleted
				 
				// delete content
                                rc = shmctl(addr->shmidcontent, IPC_RMID, NULL);
				rc_check(rc, "8-shmctl() failed!");
				addr->filesize = 0;
				addr->state = 0;
				addr->filename[0] = '\0';
				addr->shmidcontent = 0;
				
				sem_post(&addr->sem);
				rc = shmdt(addr);
				rc_check(rc, "17-shmdt() failed!");
				break;

			}
		}
		rc = shmdt(addr);
		rc_check(rc, "18-shmdt() failed!");
	}
	rc = shmdt(storage);
	rc_check(rc, "19-shmdt() failed!");
        char *response;
        int msgsize;
        if (fexist == 1){
                msgsize = 9;
                response = (char *) malloc(sizeof(char) * msgsize + 1);
                if (response == NULL){
                        rc_check(12, "malloc() failed!");
                }
                response[msgsize] = '\0';
                snprintf(response, msgsize, "%s", "deleted\n");
        }else{
                msgsize = 14;
                response = (char *) malloc(sizeof(char) * msgsize + 1);
                if (response == NULL){
                        rc_check(12, "malloc() failed!");
                }
                response[msgsize] = '\0';
                snprintf(response, msgsize, "%s", "no such file\n");
        }
        send(clientSocket, response, msgsize, 0);
        free(response);

}

void readFile(int clientSocket, char *filename){
	 
	struct storagedef address;
        struct storagedef *addr;
        int fexist = 0;          //does not exist
        int i;
	char *rrecvBuffer =(char *) malloc(sizeof(char) * 64);
	rc_check(*rrecvBuffer, "malloc() failed!");
	unsigned long long filesize;
	filesize = 0;
	struct storagedef *storage = (struct storagedef *) shmat(getStorageshmid(), NULL,0);
        for (i=0; i < getStorageSize(); i++){
                address = storage[i];
                addr = (struct storagedef *) shmat(address.shmid, NULL, 0);
		if (!strncmp(addr->filename, filename, sizeof(addr->filename))){
                        sem_wait(&addr->sem);
			fexist = FileExists(filename);
			if (fexist == 0){ // file does not exist anymore!
				sem_post(&addr->sem);
				break;
			}
			// file exists and I can read it from shm and write to heap
			rrecvBuffer = realloc(rrecvBuffer,  sizeof(char) * addr->filesize);
			if (rrecvBuffer == NULL){
				rc_check(12, "realloc() failed!");
			}
			
			memset(rrecvBuffer, 0, addr->filesize);
			 
			addr->content = (char *) shmat(addr->shmidcontent, NULL,0);
			int e;
			for (e = 0; e <= addr->filesize;e++){
				rrecvBuffer[e] = addr->content[e];
			} 
			filesize = addr->filesize;
			sem_post(&addr->sem);
                        rc = shmdt(addr->content);
			rc_check(rc, "20-shmdt() failed!");
                        rc = shmdt(addr);
			rc_check(rc, "21-shmdt() failed!");
			break;
                }
                rc = shmdt(addr);
		rc_check(rc, "22-shmdt() failed!");
        }
	rc = shmdt(storage);
	rc_check(rc, "23-shmdt() failed!");
	// send response to the client
	/*
	int szl =  (int)log10(filesize)+1;
	int msgsize = 
	char listresp[7];
	listresp[6] = '\0';
	snprintf(listresp, 6, "%s%d%s", "ACK ", getUsed(), "\n");
	send(clientSocket, listresp, 6, 0);
	*/

	if (fexist == 0 ){
		int msgsize = 14;
		char response[msgsize];
		response[msgsize-1] = '\0';
		snprintf(response, msgsize-1, "%s", "no such file\n");
		send(clientSocket, response, msgsize-1, 0);
        }else{
		int szl =  (int)log10(filesize)+1;
		int msgsize = 12 + strlen(filename) + 1 + szl + 2;
		char response[msgsize];
		response[msgsize-1] =  '\0';
		snprintf(response, msgsize, "%s%s%s%lld%s", "FILECONTENT ", filename, " ", filesize, "\n"); 
		send(clientSocket, response, msgsize-1, 0);
		send(clientSocket, rrecvBuffer, filesize, 0);
	}
	free(rrecvBuffer);
	
}

//get list of stored files and send them to the client
void listFiles(int clientSocket){
	 
	int i;
	int y = 0;
	struct storagedef *storage = (struct storagedef *) shmat(getStorageshmid(), NULL,0);
	//printf("getused(%d)\n", getUsed());
	if (getUsed() == 0){
		char listresp[7]; 
		listresp[6] = '\0';
		snprintf(listresp, 6, "%s%d%s", "ACK ", getUsed(), "\n");
		send(clientSocket, listresp, 6, 0);
		rc = shmdt(storage);
		rc_check(rc, "25-shmdt() failed!");
	}else{
	
		int szl =  (int)log10(getUsed())+1;
		int msgsize = 4 + szl + 1;
		char listresp[msgsize];
		listresp[msgsize-1] = '\0';
        	snprintf(listresp, msgsize, "%s%d%s", "ACK ", getUsed(), "\n");
		//printf("server(%s)", listresp);
        	send(clientSocket, listresp, msgsize-1, 0);
		 
		for (i=0; i < getStorageSize(); i++){
			struct storagedef address = storage[i];
			struct storagedef *addr = (struct storagedef *) shmat(address.shmid, NULL, 0);
			if (addr->state == 1){
				int lsize = strlen(addr->filename) + 2;
				char fname[lsize];
				fname[lsize-1] = '\n';
				//fname[strlen(addr->filename)+1] = '\0';
				//snprintf(fname, strlen(addr->filename) + 1, "%s%s", addr->filename ,"\n");
				snprintf(fname, lsize, "%s%s", addr->filename ,"\n");
				//printf("s(%s)-(%d)", fname, lsize);
			
				send(clientSocket, fname, lsize, 0);
				y++;
			}
			rc = shmdt(addr);
			rc_check(rc, "24-shmdt() failed!");
		}
		rc = shmdt(storage);
		rc_check(rc, "25-shmdt() failed!");
	}
}
//check if specifig file exists
// return 1 = file exists
// return 0 = file does not exist
int FileExists(char *filename){
	struct storagedef address;
        struct storagedef *addr;
	int exist = 0;		//0 = does not exist, 1 = file exists
        int i;
	struct storagedef *storage = (struct storagedef *) shmat(getStorageshmid(), NULL,0);
        for (i=0; i < getStorageSize(); i++){
		address = storage[i];
                addr = (struct storagedef *) shmat(address.shmid, NULL, 0);
		if (!strncmp(addr->filename, filename, sizeof(addr->filename))){
			exist = 1;
		}
		rc = shmdt(addr);
		rc_check(rc, "26-shmdt() failed!");
	}
	rc = shmdt(storage);
	rc_check(rc, "27-shmdt() failed!");
	return exist;
}

//store the file into shm
void createFile(int clientSocket, char *recvBuffer, char *filename, char *filesize){
	unsigned long long fs = atoi(filesize); 
	struct storagedef address;
	struct storagedef *addr;
	int i;
	int fexist = 0;
	struct storagedef *storage = (struct storagedef *) shmat(getStorageshmid(), NULL,0);
	for (i=0; i < getStorageSize(); i++){
		 
		address = storage[i];
		addr = (struct storagedef *) shmat(address.shmid, NULL, 0);
		sem_wait(&addr->sem);
		if (addr->state == 0){	//unbesetzt!
			fexist = FileExists(filename);
			if (fexist == 1){ // file exists
				sem_post(&addr->sem);
				rc = shmdt(addr);
				rc_check(rc, "28-shmdt() failed!");
				break;
			}
			int shmidcontent = shmget(IPC_PRIVATE, sizeof(char) *  atoi(filesize) + 20, IPC_CREAT | 0660|SHM_W);
			rc_check(shmidcontent, "shmget() failed!");
			addr->content = (char *) shmat(shmidcontent, NULL,0);
			addr->shmidcontent = shmidcontent;
			int i;
                       	for (i = 0; i <= atoi(filesize);i++){
                       		addr->content[i] = recvBuffer[i];
                       	}
			snprintf(addr->filename, sizeof(addr->filename), "%s", filename);
			addr->filesize = fs;
			addr->state = 1;
			sem_post(&addr->sem);
			rc = shmdt(addr->content);
			rc_check(rc, "29-shmdt() failed!");
			rc = shmdt(addr);
			rc_check(rc, "30-shmdt() failed!"); 
			break;
		}else{
			sem_post(&addr->sem);
			rc = shmdt(addr);
			rc_check(rc, "31-shmdt() failed!");
		}
	}
	rc = shmdt(storage);
	rc_check(rc, "32-shmdt() failed!");
	//send response to the client
	 
	if (fexist == 1){
		char resp[14];
		resp[13] = '\0';
		snprintf(resp, 13, "%s", "file exists\n");
		send(clientSocket, resp, 13, 0);
	}else{
		char resp[15];
		resp[14] = '\0';
		snprintf(resp, 14, "%s", "file created\n");
		send(clientSocket, resp, 14, 0);
	}
	
}

void initStorageOnce(int storagesize){
	int new_storage_shmid= shmget(IPC_PRIVATE, storagesize*sizeof(struct storagedef), IPC_CREAT | 0660|SHM_W);
	rc_check(new_storage_shmid , "malloc() failed!");
	setStorageshmid(new_storage_shmid);

}

void freeStorage(int size, int shmid){
	int i;
	struct storagedef address;
	struct storagedef *storage = (struct storagedef *) shmat(shmid, NULL,0);
	for (i = 0; i < size; i++){
		address = storage[i];
		rc = shmctl(address.shmid, IPC_RMID, NULL);
		rc_check(rc, "9-shmctl() failed!");
	}
        rc = shmctl(getStorageshmidprev(), IPC_RMID, NULL);
	rc_check(rc, "10-shmctl() failed!");
	rc = shmdt(storage);
	rc_check(rc, "33-shmdt() failed!");
}

void freeStorageAll(){
	int i;
        struct storagedef address;
        struct storagedef *storage = (struct storagedef *) shmat(getStorageshmid(), NULL,0);
	struct storagedef *addr;
	for (i = 0; i < getStorageSize(); i++){
                address = storage[i];
                addr = (struct storagedef *) shmat(address.shmid, NULL, 0);
		if (addr->shmidcontent > 0){
                	rc = shmctl(addr->shmidcontent, IPC_RMID, NULL);
			rc_check(rc, "11-shmctl() failed!");
		}
                rc = shmdt(addr);
		rc_check(rc, "34-shmdt() failed!");
                rc = shmctl(address.shmid, IPC_RMID, NULL);
		rc_check(rc, "12-shmctl() failed!");
	}
	rc = shmdt(storage);
	rc_check(rc, "35-shmdt() failed!");
	rc = shmctl(getStorageshmid(), IPC_RMID, NULL);
        rc_check(rc, "13-shmctl() failed!");

}

void initStorage(int init_shmid, int startint){
	int i;
	struct storagedef *storage = (struct storagedef *) shmat(init_shmid, NULL,0);
	for(i = startint; i < getStorageSize(); i++){
		int shmid = shmget(IPC_PRIVATE, sizeof(struct storagedef), IPC_CREAT | 0660|SHM_W);
		rc_check(shmid, "shmget() failed!");
		struct storagedef *addr = (struct storagedef *) shmat(shmid, NULL, 0);
		int semid = sem_init(&addr->sem, 1, 1);
		rc_check(semid, "sem_init() failed!");
		addr->semid = semid;
		addr->shmid = shmid;
		addr->shmidcontent = 0;
		addr->state = 0;	// 0 = unsed, 1=used
		addr->filename[0] = '\0';
		addr->filesize = 0;
		storage[i] = *addr;
		rc = shmdt(addr);
		rc_check(rc, "36-shmdt() failed!");
	}
	rc = shmdt(storage);
	rc_check(rc, "37-shmdt() failed!");
	getUsed();
}

void reallocStorage(){
	 
	sem_value = (sem_t *) shmat(sem_shmid, NULL, 0);
	rc = sem_wait(sem_value);
	rc_check(rc, "sem_wait failed!()");
	int ratio = (getUsed()*100)/(getStorageSize()) ;
	if (ratio > 80){
		//sleep(5);
		int oldsize = getStorageSize();
		int newsize = oldsize * 2;
		setStorageSize(newsize);
		setStorageSizePrev(oldsize);
		 
		
		int old_storage_shmid = getStorageshmid();
		int new_storage_shmid = shmget(IPC_PRIVATE, newsize*sizeof(struct storagedef), IPC_CREAT | 0660|SHM_W);
		rc_check(new_storage_shmid, "shmget() failed!");
		setStorageshmid(new_storage_shmid);
		setStorageshmidprev(old_storage_shmid);
		 
		struct storagedef *oldstorage = (struct storagedef *) shmat(getStorageshmidprev(), NULL,0);
		 
		struct storagedef *newstorage = (struct storagedef *) shmat(getStorageshmid(), NULL,0);
		initStorage(new_storage_shmid, 0);
		struct storagedef oldaddress;
		struct storagedef *oldaddr;
		struct storagedef newaddress;
		struct storagedef *newaddr;
		int i;
		for (i=0; i < oldsize; i++){
			oldaddress = oldstorage[i];
			oldaddr = (struct storagedef *) shmat(oldaddress.shmid, NULL, 0);
			newaddress = newstorage[i];
			newaddr = (struct storagedef *) shmat(newaddress.shmid, NULL, 0);
			 
			newaddr->shmid = oldaddr->shmid;
			newaddr->state = oldaddr->state;
			snprintf(newaddr->filename, sizeof(newaddr->filename), "%s", oldaddr->filename);
			newaddr->sem = oldaddr->sem;
			newaddr->semid = oldaddr->semid;
			newaddr->filesize = oldaddr->filesize;
			newaddr->shmidcontent = oldaddr->shmidcontent;
			 
			rc = shmdt(oldaddr);
			rc_check(rc, "38-shmdt() failed!");
			rc = shmdt(newaddr);
			rc_check(rc, "39-shmdt() failed!");
		}
		
		rc = shmdt(newstorage);
		rc_check(rc, "40-shmdt() failed!");
		rc = shmdt(oldstorage);
		rc_check(rc, "41-shmdt() failed!");
		freeStorage(getStorageSizePrev(),getStorageshmidprev());
		 
	}
	sem_post(sem_value);
	rc = shmdt(sem_value);
	rc_check(rc, "60-shmdt failed!");
	 
}
void setStorageSize(int newsize){
	storemesize =  (int *) shmat(storemesize_shmid, NULL, 0);
        *storemesize = newsize;
        rc = shmdt(storemesize);
	rc_check(rc, "42-shmdt() failed!");
}
int getStorageshmid(){
	storageshmid = (int *) shmat(storage_shmid, NULL, 0);
	int storshmid = *storageshmid;
	rc = shmdt(storageshmid);
	rc_check(rc, "43-shmdt() failed!");
	return storshmid;
}
void setStorageshmid(int newshmid){
	storageshmid = (int *) shmat(storage_shmid, NULL, 0);
	*storageshmid = newshmid;
	rc = shmdt(storageshmid);
	rc_check(rc, "44-shmdt() failed!");
}
int getStorageshmidprev(){
        storageshmid_prev = (int *) shmat(storage_shmid_prev, NULL, 0);
        int storshmidprev = *storageshmid_prev;
        rc = shmdt(storageshmid_prev);
	rc_check(rc, "45-shmdt() failed!");
        return storshmidprev;
}
void setStorageshmidprev(int newshmidprev){
        storageshmid_prev = (int *) shmat(storage_shmid_prev, NULL, 0);
        *storageshmid_prev = newshmidprev;
        rc = shmdt(storageshmid_prev);
	rc_check(rc, "46-shmdt() failed!");
}


int getStorageSize(){
	storemesize =  (int *) shmat(storemesize_shmid, NULL, 0);
	int storagesize = *storemesize;
	rc = shmdt(storemesize);
	rc_check(rc, "47-shmdt() failed!");
	return storagesize;
}
int getStorageSizePrev(){
	storemesizeprev =  (int *) shmat(storemesizeprev_shmid, NULL, 0);
        int storagesizeprev = *storemesizeprev;
        rc = shmdt(storemesizeprev);
	rc_check(rc, "48-shmdt() failed!");
        return storagesizeprev;
}
void setStorageSizePrev(int size){
	storemesizeprev = (int *) shmat(storemesizeprev_shmid, NULL, 0);
	*storemesizeprev = size;
	rc = shmdt(storemesizeprev);
	rc_check(rc, "49-shmdt() failed!");
}


int getUsed(){
        storemeused =  (int *) shmat(storemeused_shmid, NULL, 0);
	struct storagedef address;
	struct storagedef *addr;
	int i;
	int counter = 0;
	struct storagedef *storage = (struct storagedef *) shmat(getStorageshmid(), NULL,0);
	for (i=0; i < getStorageSize(); i++){	
		address = storage[i];
		addr = (struct storagedef *) shmat(address.shmid, NULL, 0);
		if (addr->state == 1){
			counter += 1;
		}
		rc = shmdt(addr);
		rc_check(rc, "shmdt() failed!");
	}
	rc = shmdt(storage);
	rc_check(rc, "50-shmdt() failed!");
        *storemeused = counter;
        rc = shmdt(storemeused);
	rc_check(rc, "51-shmdt() failed!");
	return counter; 
}

void getall(){

	int i;
	struct storagedef *storage = (struct storagedef *) shmat(getStorageshmid(), NULL,0);
	for (i=0; i < getStorageSize(); i++){
		struct storagedef address = storage[i];
		struct storagedef *addr = (struct storagedef *) shmat(address.shmid, NULL, 0);
                if (addr->state == 1){
			printf("filename:%s\n", addr->filename);
			printf("filesize:%lld\n", addr->filesize);
			printf("state:%d\n", addr->state);
		}
        	rc = shmdt(addr);
		rc_check(rc, "51-shmdt() failed!");
        }
        rc = shmdt(storage);
	rc_check(rc, "52-shmdt() failed!");
}
