#ifndef GENLIB
#define GENLIB
/*
used for client and server
*/

char *type;
char *localfilename;
char *remotefilename;
int pidfstatus();


void usageserver(char *argv[]);
void usageserverparse(int argc, char *argv[]);


void usageclient(char *argv[]);
void usageclientparse(int argc, char *argv[]);

void rc_check(int rc, const char *message);
void writepid(pid_t pid);
#endif

