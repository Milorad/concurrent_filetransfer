#include <semaphore.h>
#include <sys/sem.h>    //GETALL, SETALL
#include <sys/shm.h>
#include <stdio.h>
#include <stdio.h>

#ifndef SEMDEFINITION
#define SEMDEFINITION

void listFiles();
void createFile(int clientSocket, char *recvbuffer, char *filename, char *filesize);
void updateFile(int clientSocket, char *recvBuffer, char *filename, char *filesize);
void readFile(int clientSocket, char *filename);
void deleteFile(int clientSocket, char *filename);

void initStorageOnce();

void initStorage(int init_shmid, int startint);
int getUsed();
int getStorageSize();
void setStorageSize(int newsize);
int getStorageshmid();
void setStorageshmid(int newshmid);
int getStorageshmidprev();
void setStorageshmidprev(int newshmidprev);
void getall();
int getStorageSizePrev();
void setStorageSizePrev(int size);


int FileExists(char *filename);

void freeStorage(int size, int shmid);
void freeStorageAll();
void reallocStorage();
void signal_stopserver();

struct storagedef {
	sem_t sem;
	int semid;
	int shmid;	//shared memory id
	unsigned long long filesize;	//size of content
	char filename[64];
	char *content;
	int shmidcontent;
	int state;	// 0 unused, 1 used
};
#endif
