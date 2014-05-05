#include <stdio.h>
#include <signal.h>		// kill
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
int pidfstatus();
void writepid(pid_t pid);

char *type;
char *localfilename;
char *remotefilename;
FILE *pidfile; 
char *pidfilepath = "run.pid";


void usageserver(char **argv){
	printf("%s (start|stop|status)\n", argv[0]); 
	exit(0);
}

void usageserverparse(int argc, char **argv){
	 
	if (argc == 1){
		usageserver(argv);
	}
	if (argc == 2){
		if(!strcmp(argv[1], "start")){
			int rc = pidfstatus();
			if (rc == -1){ // file does not exist
				// ok O_o
			}else{
				printf("%s is already running(%d)\n", argv[0], rc);
				exit(0);
			}
		}else if(!strcmp(argv[1], "stop")){
			int rc = pidfstatus();
			if (rc == -1){ // file does not exist
				printf("%s is not running\n", argv[0]);
				exit(0);
			}
			kill(rc, SIGTERM);
			unlink(pidfilepath);
			exit(0);
		}else if(!strcmp(argv[1], "status")){
			//printf("3: %s::\n", argv[1]); 
			int rc = pidfstatus(); 
			if (rc == -1){
				printf("%s is not running\n", argv[0]); 
			}else{
				printf("%s is running(%d)\n", argv[0], rc);
				 
			}
			exit(0);
			 
		}else{
			usageserver(argv);
		}
	}
}
void writepid(pid_t pid){
	pidfile = fopen(pidfilepath, "w"); 
	if (pidfile == NULL){
		fclose(pidfile); 
		rc_check(5, "fopen() failed");
	}
	fprintf(pidfile, "%d", pid);
	fclose(pidfile);
}
int pidfstatus(){
	 
	int rc = access( pidfilepath, F_OK );
        if (rc == 0){ // file exists
		pidfile = fopen(pidfilepath, "r");
		if (pidfile == NULL){
			fclose(pidfile);
			rc_check(12, "fopen() failed");
			
		}
		int filesize;  
		int filereadsize;
		char *filebuffer;
		fseek(pidfile, 0L, SEEK_END);
		filesize = ftell(pidfile);
		fseek(pidfile, 0L, SEEK_SET);
		filebuffer = (char *) malloc(sizeof(char) * (filesize + 1) ); 
		if (filebuffer == NULL){
			printf("could not malloc()!\n"); 
			exit(1);
		}
		filebuffer[filesize+1] = '\000';
		filereadsize = fread(filebuffer,sizeof(char),filesize,pidfile);
		if (filereadsize == 0){
			// file is empty, delete it
			unlink(pidfilepath);
		}
		fclose(pidfile);
		//check if PID really exists
		int mypid = atoi(filebuffer);
		if (mypid == 0){
			errno = ESRCH;
			rc_check(-1, "pid not int");
			
		}
		int pidexists = kill(mypid, 0);
		rc_check(pidexists, "kill(0) failed");
		//printf("pidexists : %d\n", pidexists);
		return mypid;
	}else{ // file does not exist
		return -1;
	}
}

void usageclient(char **argv){
	printf("Usage:\n");
	printf("\tlist files   :  # %s list\n", argv[0]);
	printf("\tcreate file  :  # %s create <local-filename>\n", argv[0]);
	printf("\tread file    :  # %s read <remote-filename>\n", argv[0]);
	printf("\tupdate file  :  # %s update <remote-filename> <local-filename>\n", argv[0]);
	printf("\tdelete file  :  # %s delete <remote-filename>\n\n", argv[0]);
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

