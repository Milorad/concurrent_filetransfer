#include <semaphore.h>
#include <sys/sem.h>    //GETALL, SETALL
#include <sys/shm.h>
#include <stdio.h>
#include <stdio.h>

#ifndef SEMDEFINITION
#define SEMDEFINITION
#endif

union semun {
        int val;                
        struct semid_ds *buf;   
        unsigned short array[1];
        struct seminfo *__buf;  
};

struct zsss {
	sem_t sem;
	int semid;
	int shmid;	//shared memory id
	unsigned long long filesize;	//size of content
	char filename[64];
	char *content;
	int shmidcontent;
	int state;	// 0 unused, 1 used
};
