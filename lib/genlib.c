#include <stdio.h>
#include <errno.h>		//errno variable

#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>




//prevent error if the file will be included twice
#ifndef GENLIB
#define GENLIB
#endif

//#endif
void rc_check(int rc, const char *message);

char *type;
char *localfilename;
char *remotefilename;


void usageserver(){
	printf("I am usage.\n"); 
	exit(0);
}

void usageserverparse(int argc, char **argv){
	 
}

void usageclient(char **argv){
	printf("\nUsage:\n\n");
	printf("\tlist files   :\n\t  # %s list\n", argv[0]);
	printf("\tcreate file  :\n\t  # %s create <local-filename>\n", argv[0]);
	printf("\tread file    :\n\t  # %s read <remote-filename>\n", argv[0]);
	printf("\tupdate file  :\n\t  # %s update <remote-filename> <local-filename>\n", argv[0]);
	printf("\tdelete file  :\n\t  # %s delete <remote-filename>\n\n", argv[0]);
	exit(0);
}
void usageclientparse(int argc, char **argv){
	 
	if (argc == 1){
		usageclient(argv); 
	}
	int rc;
	//check that provided filename is not longer then 64
	if (argv[2]){
		int filenamechars = strlen(argv[2])+1;
		//printf("chars: %d\n", filenamechars);
		if (argv[2]){
			if (filenamechars > 63){
				printf("Filename is limited to 63 charachters!\n"); 
				usageclient(argv);
			}
		}
		if (argv[3]){
			if (filenamechars > 63){
				printf("Filename is limited to 63 charachters!\n");
				usageclient(argv);
			}
		}
	}
	
	if (!strcmp(argv[1], "list")){
		type = "list";
		 
	}else if(!strcmp(argv[1], "create")){
		if (argv[2]){ //2nd arg exists
			rc = access( argv[2], F_OK );
			if (rc == 0){ // file exists
				type = "create";
				localfilename = argv[2];
			}else{ //file does not exist
				rc_check(rc, " access() : ");
				usageclient(argv); 
			}
			 
		}else{ //no 2nd arg
			printf("Please provide a filename!\n");
			usageclient(argv);
		}
	}else if(!strcmp(argv[1], "read")){
		if (argv[2]){
			type = "read";
			remotefilename = argv[2];
		}else{ //no 2nd arg
			usageclient(argv);
		}
	}else if(!strcmp(argv[1], "update")){
		if (argv[2]){ //remove filename provided
			if (argv[3]){
				rc = access(argv[3], F_OK); 
				if (rc == 0){
					type = "update";
					remotefilename = argv[2];
					localfilename = argv[3];
				}else{
					rc_check(rc, "access() : ");
					usageclient(argv);
				}
			}else{
				usageclient(argv); 
			}
			 
		}else{ //no 2nd arg
			usageclient(argv);
		}
	}else if(!strcmp(argv[1], "delete")){
		if (argv[2]){
			type = "delete";
			remotefilename = argv[2];
		}else{
			usageclient(argv);
		} 
	}else{
		usageclient(argv);
	}
}

void rc_check(int rc, const char *message){
	int linuxErrorNumber = errno; 
	const char *errorstr = strerror(linuxErrorNumber);
	if (rc < 0){
		if (message != NULL) {
			fprintf(stderr, " %s ", message);
			fprintf(stderr, "rc : %d : %s\n", rc, errorstr);
		}else{
			fprintf(stderr, "Undefined message by fserver: rc(%d): %s\n", rc, errorstr);
		}
		exit(1); 
	}else{
		//fprintf(stdout, "rc(%d): %s : %s\n", rc, message, errorstr);
	}
	fflush(stderr);
	fflush(stdout);
}

