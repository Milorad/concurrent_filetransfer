
#ifndef GENLIB
#define GENLIB
#endif


char *type;
char *localfilename;
char *remotefilename;


void usageserver(char *argv[]);
void usageserverparse(int argc, char *argv[]);


void usageclient(char *argv[]);
void usageclientparse(int argc, char *argv[]);

void rc_check(int rc, const char *message);

void writepid(pid_t pid);
