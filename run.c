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

#define TRUE 1
#define FALSE 0

#define RECVBUFFERSIZE 64
#define SHMSZ 16777216

struct zsss newFile();
struct zsss zsem;
void listFiles();
void createFile(int clientSocket, char *recvbuffer, char *filename, char *filesize);
void updateFile(int clientSocket, char *recvBuffer, char *filename, char *filesize);
void readFile(int clientSocket, char *filename);
void deleteFile(int clientSocket, char *filename);

void initStorageOnce();

void initStorage();
int getUsed();
int getStorageSize();
void setStorageSize(int newsize);
int FileExists(char *filename);

void freeStorage();
void reallocStorage();

struct zsss *storeme;
int *storemesize;
int storemesize_shmid;
int *storemeused;
int storemeused_shmid;

void signal_stopserver() {
	printf("Shutting down server.\n");
	freeStorage();
	//dele shm for storage vars
	shmctl(storemesize_shmid, IPC_RMID, NULL);
	shmctl(storemeused_shmid, IPC_RMID, NULL);
	exit(0);
}
void signal_pidcheck(){
	//printf("check PID.\n");
}


void initGlobs(){
	storemeused_shmid= shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0777|SHM_W);
	rc_check(storemeused_shmid, "shmget() failed!");
        storemeused =  (int *) shmat(storemeused_shmid, NULL, 0);
	*storemeused = 0;
	shmdt(&storemeused_shmid);
	
	storemesize_shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0777|SHM_W);
	rc_check(storemesize_shmid, "shmget() failed!");
	storemesize =  (int *) shmat(storemesize_shmid, NULL, 0);
	*storemesize = 100;
	shmdt(&storemesize_shmid);
	

}

int main(int argc, char *argv[]) {
	
	usageserverparse(argc, argv);
	
	pid_t pidd;
	 
	pidd = fork();
	if (pidd == -1){
		rc_check(-1, "fork() failed");
		
	}else if (pidd == 0){ // CHILD - used as daemon
		writepid(getpid());
		//exit(0);
		initGlobs(); 
		serverSocket = initSocket();
		// needed to stop
		struct sigaction saint;
		saint.sa_handler = signal_stopserver;
		sigemptyset(&saint.sa_mask);
		sigaddset(&saint.sa_mask, SIGTERM);
		saint.sa_flags = 0;
		sigaction(SIGTERM, &saint, NULL);
		//need to check if pid exists
		struct sigaction sachk;
		sachk.sa_handler = signal_pidcheck;
		sigemptyset(&sachk.sa_mask);
		sigaddset(&sachk.sa_mask, SIGINT);
		sachk.sa_flags = 0;
		sigaction(SIGINT, &sachk, NULL);
		
		
		// init storage. here are pointers to the files
		//http://stackoverflow.com/questions/12066046/array-of-structs-deleting-adding-elements-and-printing
		// 
		initStorageOnce();
		initStorage(0);
		while (TRUE){
			clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &clientAddrLen);
			rc_check(clientSocket, "accept socket");
			//new process for new client
			pid_t pid;
			pid = fork();
			unsigned int totalBytesReceived;
			unsigned int totalFileBytesReceived;
			switch (pid) {
				case -1:
					rc_check(-1, "fork() failed");
				case 0: // CHILD
					//shm
					 
					totalBytesReceived = 0;
					totalFileBytesReceived = 0;
					char type[64]; 
					char filename[64];
					char filesize[64];
					char *recvBuffer = (char *) malloc(sizeof(char) * 64);
					if (recvBuffer == NULL){
						rc_check(12, "malloc() failed!");
					}
					//int counter = 0;
					
					//storemeused = shmat(storemeused_shmid, 0, 0);
					 
					while(TRUE){
						char rBuffer[RECVBUFFERSIZE];
						int recvMsgSize = recv(clientSocket, rBuffer, RECVBUFFERSIZE-1, 0);
						totalBytesReceived += recvMsgSize;
						//printf("recvMsgSize(%d)", recvMsgSize);
						if (recvMsgSize == 0){
							//printf("size is zero.\n");
							//TRUE = 0;
							break;
						}else{
							
							if (totalBytesReceived == 63){
								strcpy(type, rBuffer);
								if (!strcmp(type, "list")){
									break; 
								}
								//printf("z1: %s\n", rBuffer);
							}else if(totalBytesReceived == 126){
								strcpy(filename, rBuffer);
								if (!strcmp(type, "read")){
									break; 
								}
								if (!strcmp(type, "delete")){
									break;
								}
							}else if(totalBytesReceived == 189){
								//printf("z3: %s\n", rBuffer);
								strcpy(filesize, rBuffer);
								recvBuffer = realloc(recvBuffer, atoi(rBuffer) * sizeof(char));
								if (recvBuffer == NULL){
									rc_check(12, "realloc() failed!");
								}
								memset(recvBuffer, 0, atoi(filesize));
							}else if(totalBytesReceived == 252){
								// not used, reserved for future use
							}else{
								totalFileBytesReceived += recvMsgSize;
								strcpy(recvBuffer+strlen(recvBuffer), rBuffer);
								if (atoi(filesize) == totalFileBytesReceived){
									//printf("FILE-END!\n");
									break;
								}
							}
							 
							memset(rBuffer, 0, sizeof(rBuffer));
						}
					}
					//printf("-----------------------------------------totalBytesReceived(%d)\n", totalBytesReceived);
					if (!strcmp(type, "list")){
						listFiles(clientSocket);
					}else if(!strcmp(type, "create")){
						createFile(clientSocket, recvBuffer, filename, filesize);
					}else if(!strcmp(type, "read")){
						readFile(clientSocket, filename);
					}else if(!strcmp(type, "update")){
						updateFile(clientSocket, recvBuffer, filename, filesize);
					}else if(!strcmp(type, "delete")){
						deleteFile(clientSocket, filename);
					}
					//shmdt(storemeused);
					
					
				default: // PARENT
					reallocStorage(); 
			}
		}
	}else{  //PARENT - daemonize child
		exit(0);
        }

}

void updateFile(int clientSocket, char *recvBuffer, char *filename, char *filesize){
	unsigned long long fs = atoi(filesize);
	struct zsss address;
        struct zsss *addr;
        int i;
	int fexist = 0;
        for (i=0; i < *storemesize; i++){
		address = storeme[i];
                addr = (struct zsss *) shmat(address.shmid, NULL, 0);
		if (!strcmp(addr->filename, filename)){
			sem_wait(&addr->sem);
			fexist = FileExists(filename);
			if (fexist == 1){ // file exists and can be updated
				// delete old shm
				shmctl(addr->shmidcontent, IPC_RMID, NULL);
				// add new one
				int shmidcontent = shmget(IPC_PRIVATE, sizeof(char) *  atoi(filesize) + 20, IPC_CREAT | 0666|SHM_W);
				rc_check(shmidcontent, "shmget() failed!");
				addr->content = (char *) shmat(shmidcontent, NULL,0);
                                addr->shmidcontent = shmidcontent;
				
				int i;
                                for (i = 0; i <= atoi(filesize);i++){
                                        //printf("i(%d)--", i);
                                        addr->content[i] = recvBuffer[i];
                                }
				
				strcpy(addr->filename, filename);
                                addr->filesize = fs;
                                addr->state = 1;
				
				sem_post(&addr->sem);
                                shmdt(addr->content);
                                shmdt(&address.shmid);

                                //shmdt(addr->content);
                                break;
                        }else{
				sem_post(&addr->sem);
				shmdt(&address.shmid);
				break;
			}
		}
		shmdt(&address.shmid); 
	}
	char content[63];
        content[62] = '\000';
        if (fexist == 1){
                 sprintf(content, "%s\n", "updated");
        }else{
                 sprintf(content, "%s\n", "no such file");
        }
	
	//zeile 1
        char rtype[63];
        rtype[62] = '\000';
        sprintf(rtype, "%s", "update");
        send(clientSocket, rtype, sizeof(rtype), 0);
	
	//zeile 3size of filelist
        char fsize[63];
        fsize[62] = '\000';
        sprintf(fsize, "%zu", sizeof(content));
        send(clientSocket, fsize, sizeof(content), 0);
	
	// filename
        char fname[63];
        fname[62] = '\000';
        sprintf(fname, "%s", filename);
        send(clientSocket, fname, sizeof(fname), 0);

	//message
        send(clientSocket, content, sizeof(content), 0);





}

void deleteFile(int clientSocket, char *filename){
	struct zsss address;
        struct zsss *addr;
        int fexist = 0;          //does not exist
        int i;
	for (i=0; i < *storemesize; i++){
		address = storeme[i];
		addr = (struct zsss *) shmat(address.shmid, NULL, 0);
		addr = (struct zsss *) shmat(address.shmid, NULL, 0);
		if (!strcmp(addr->filename, filename)){
			sem_wait(&addr->sem);
                        fexist = FileExists(filename);
			if (fexist == 1){ // file exists and can be deleted
				 
				// delete content
                                shmctl(addr->shmidcontent, IPC_RMID, NULL);
				addr->filesize = 0;
				addr->state = 0;
				addr->filename[0] = '\0';
				addr->shmidcontent = 0;
				
				sem_post(&addr->sem);
				shmdt(&address.shmid);
				break;

			}
		}
		shmdt(&address.shmid);
	}
	char content[63];
        content[62] = '\000';
        if (fexist == 1){
                 sprintf(content, "%s\n", "deleted");
        }else{
                 sprintf(content, "%s\n", "no such file");
        }
	//zeile 1
        char rtype[63];
        rtype[62] = '\000';
        sprintf(rtype, "%s", "delete");
        send(clientSocket, rtype, sizeof(rtype), 0);
	
	//zeile 3size of filelist
        char fsize[63];
        fsize[62] = '\000';
        sprintf(fsize, "%zu", sizeof(content));
        send(clientSocket, fsize, sizeof(content), 0);

	// filename
        char fname[63];
        fname[62] = '\000';
        sprintf(fname, "%s", filename);
        send(clientSocket, fname, sizeof(fname), 0);

	//message
        send(clientSocket, content, sizeof(content), 0);

	

 

}

void readFile(int clientSocket, char *filename){
	 
	struct zsss address;
        struct zsss *addr;
        int fexist = 0;          //does not exist
        int i;
	//char *recvBuffer = (char *) malloc(sizeof(char) * 64);;
	char *recvBuffer =(char *) malloc(sizeof(char) * 64);;
	rc_check(*recvBuffer, "malloc() failed!");
	unsigned long long filesize;
	filesize = 0;
        for (i=0; i < *storemesize; i++){
                address = storeme[i];
                addr = (struct zsss *) shmat(address.shmid, NULL, 0);
		//sem_wait(&addr->sem);
                if (!strcmp(addr->filename, filename)){
                        sem_wait(&addr->sem);
			fexist = FileExists(filename);
			if (fexist == 0){ // file does not exist anymore!
				sem_post(&addr->sem);
				//shmdt(addr->content);
				break;
			}
			// file exists and I can read it from shm and write to heap
			//recvBuffer = realloc(sizeof(char) * addr->filesize);
			recvBuffer = realloc(recvBuffer,  sizeof(char) * addr->filesize);
			if (recvBuffer == NULL){
				rc_check(12, "realloc() failed!");
			}
			
			memset(recvBuffer, 0, addr->filesize);
			 
			addr->content = (char *) shmat(addr->shmidcontent, NULL,0);
			int e;
			for (e = 0; e <= addr->filesize;e++){
				recvBuffer[e] = addr->content[e];
			} 
			filesize = addr->filesize;
			sem_post(&addr->sem);
                        shmdt(addr->content);
                        shmdt(&address.shmid);
			break;
                }
                shmdt(&address.shmid);
        }
	if (fexist == 0 ){
                sprintf(recvBuffer, "%s\n", "No such file");
                filesize = 64;  //64B for string No such file
        }
	//zeile1
	char rtype[63];
	rtype[62] = '\000';
	sprintf(rtype, "%s", "read");
	send(clientSocket, rtype, sizeof(rtype), 0);
	
	 //zeile 3size of filelist
        char szflist[63];
        szflist[62] = '\000';
        sprintf(szflist, "%lld", filesize);
        send(clientSocket, szflist, sizeof(szflist), 0);
	
	 //zeile 3size of filelist
	char fname[63];
	fname[62] = '\000';
	sprintf(fname, "%s", filename);
	send(clientSocket, fname, sizeof(fname), 0);
	 
	//content
	send(clientSocket, recvBuffer, filesize, 0);
	//free up memory
	free(recvBuffer);
}

void listFiles(int clientSocket){
	
	unsigned long long numberOfFiles = *storemeused;
	char *filelist[numberOfFiles];
	int filenamesize[numberOfFiles];
	struct zsss address;
	struct zsss *addr;
	int i;
	int y = 0;
	unsigned long long listsize = 0;
	for (i=0; i < *storemesize; i++){
		address = storeme[i];
		addr = (struct zsss *) shmat(address.shmid, NULL, 0);
		if (addr->state == 1){
			listsize = listsize + strlen(addr->filename) + 1;
			filenamesize[y] = strlen(addr->filename)+1; // + 1 for \n
			filelist[y] = (char *) malloc(sizeof(char) * addr->filesize + 1);
			if (filelist == NULL){
				rc_check(12, "malloc() failed!");
			}

			//rc_check(filelist, "malloc() failed!");
			sprintf(filelist[y], "%s\n",  addr->filename);
			y++;
		}
		shmdt(&address.shmid);
	}
	//zeile1
	char rtype[63];
        rtype[62] = '\000';
        sprintf(rtype, "%s", "list");
        send(clientSocket, rtype, sizeof(rtype), 0);
	
	 //zeile 3size of filelist
	char content[63];
	char szflist[63];
	szflist[62] = '\000';
	if (numberOfFiles > 0){
		sprintf(szflist, "%lld", listsize);
	}else{
		
		sprintf(content, "%s\n", "no files available");
		sprintf(szflist, "%zu", sizeof(content));
	}
	send(clientSocket, szflist, sizeof(szflist), 0);


	//zeile2 number of files
        char nfiles[63];
        nfiles[62] = '\000';
        sprintf(nfiles, "%lld", numberOfFiles);
        send(clientSocket, nfiles, 63, 0);

	if (numberOfFiles > 0){
		int z;
		for (z=0; z < numberOfFiles; z++){
			//printf("%zu\n", sizeof(filelist[z]));
			//printf("%d\n", filenamesize[z]);
			send(clientSocket, filelist[z], filenamesize[z], 0);
			free(filelist[z]);
		}
	}else{ // no files in memory
		 //message
		send(clientSocket, content, sizeof(content), 0);

	}
	 
	
}

int FileExists(char *filename){
	struct zsss address;
        struct zsss *addr;
	int exist = 0;		//does not exist
        int i;
        for (i=0; i < *storemesize; i++){
		address = storeme[i];
                addr = (struct zsss *) shmat(address.shmid, NULL, 0);
		if (!strcmp(addr->filename, filename)){
			exist = 1;
		}
		shmdt(&address.shmid);
	}
	return exist;
}

// return 0 = created
// return 1 = exists


void createFile(int clientSocket, char *recvBuffer, char *filename, char *filesize){
	//printf("%s", recvBuffer);
	unsigned long long fs = atoi(filesize); 
	//int shmid = shmget(IPC_PRIVATE, fs, IPC_CREAT | 0666|SHM_W); 
	//struct zsss *addr = (struct zsss *) shmat(shmid, NULL, 0);
	struct zsss address;
	struct zsss *addr;
	int i;
	int fexist = 0;
	for (i=0; i < *storemesize; i++){
		 
		address = storeme[i];
		addr = (struct zsss *) shmat(address.shmid, NULL, 0);
		sem_wait(&addr->sem);
		if (addr->state == 0){	//unbesetzt!
			fexist = FileExists(filename);
			if (fexist == 1){ // file exists
				sem_post(&addr->sem);
				shmdt(addr->content);
				break;
			}
			int shmidcontent = shmget(IPC_PRIVATE, sizeof(char) *  atoi(filesize) + 20, IPC_CREAT | 0666|SHM_W);
			rc_check(shmidcontent, "shmget() failed!");
			addr->content = (char *) shmat(shmidcontent, NULL,0);
			addr->shmidcontent = shmidcontent;
			int i;
                       	for (i = 0; i <= atoi(filesize);i++){
                       		addr->content[i] = recvBuffer[i];
                       	}
			strcpy(addr->filename, filename);
			addr->filesize = fs;
			addr->state = 1;
			sem_post(&addr->sem);
			shmdt(addr->content);
			shmdt(&address.shmid);
			 
			break;
		}else{
			sem_post(&addr->sem);
			shmdt(&address.shmid);
		}
	}
	char content[63];
	content[62] = '\000';
	if (fexist == 1){
		 sprintf(content, "%s\n", "exist");
	}else{
		 sprintf(content, "%s\n", "created.");
	}
	//zeile 1
	char rtype[63];
	rtype[62] = '\000';
	sprintf(rtype, "%s", "create");
	send(clientSocket, rtype, sizeof(rtype), 0);
	
	//zeile 3size of filelist
        char fsize[63];
        fsize[62] = '\000';
        sprintf(fsize, "%zu", sizeof(content));
        send(clientSocket, fsize, sizeof(content), 0);
	
	// filename
	char fname[63];
        fname[62] = '\000';
        sprintf(fname, "%s", filename);
        send(clientSocket, fname, sizeof(fname), 0);
	 
	//message
	send(clientSocket, content, sizeof(content), 0);
	 
}

void initStorageOnce(){
	storeme = malloc(100 * sizeof(struct zsss) * 128);
	if (storeme == NULL){
		rc_check(12, "malloc() failed!");
	}

}

void freeStorage(){
	int i;
	struct zsss address;
	for (i = 0; i < getStorageSize(); i++){
		address = storeme[i];
		shmctl(address.shmid, IPC_RMID, NULL);
	}
}

void initStorage(int startint){
	//printf("getStorageSize(%d)\n", getStorageSize());
	int i ;
	for(i = startint; i < getStorageSize(); i++){
		int shmid = shmget(IPC_PRIVATE, sizeof(struct zsss), IPC_CREAT | 0666|SHM_W);
		rc_check(shmid, "shmget() failed!");
		// SEGV possible as maybe out of shared memory!
		struct zsss *addr = (struct zsss *) shmat(shmid, NULL, 0);
		int semid = sem_init(&addr->sem, 1, 1);
		addr->semid = semid;
		addr->shmid = shmid;
		addr->state = 0;	// 0 = unsed, 1=used
		/////////////addr->filesize = 0;
		addr->filename[0] = '\0';
		addr->filesize = 0;
		storeme[i] = *addr; 
	}
	getUsed();
	//memset(storeme, 0, sizeof(storeme));
	//storage= malloc(sizeof(struct storagedef)* 100);
}

void reallocStorage(){
	int ratio = (getUsed()*100)/(getStorageSize()) ;
	//printf("ratio(%d)\n", ratio);
	if (ratio > 80){
		//printf("RATIO: %d\n", ratio);
		int oldsize = getStorageSize();
		int newsize = oldsize * 2;
		setStorageSize(newsize);
		storeme = realloc(storeme, newsize * sizeof(struct zsss));
		if (storeme == NULL){
			rc_check(12, "realloc() failed!");
		}else{
			//init new structs
			initStorage(oldsize);
		}
	}
	 
}
void setStorageSize(int newsize){
	storemesize =  (int *) shmat(storemesize_shmid, NULL, 0);
        *storemesize = newsize;
        shmdt(&storemesize_shmid);
}

int getStorageSize(){
	storemesize =  (int *) shmat(storemesize_shmid, NULL, 0);
	int storagesize = *storemesize;
	shmdt(&storemesize_shmid);
	return storagesize;
}

int getUsed(){
	//storemeused_shmid= shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0777|SHM_W);
        storemeused =  (int *) shmat(storemeused_shmid, NULL, 0);

	struct zsss address;
	struct zsss *addr;
	int i;
	int counter = 0;
	for (i=0; i < getStorageSize(); i++){	
		address = storeme[i];
		addr = (struct zsss *) shmat(address.shmid, NULL, 0);
		if (addr->state == 1){
			counter += 1;
		}
	}
        *storemeused = counter;
        shmdt(&storemeused_shmid);
	return counter; 
}
