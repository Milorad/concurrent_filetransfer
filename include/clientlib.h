
#ifndef CLIENTLIB
#define CLIENTLIB
#endif


void f_list(int Socket);
void f_create(int Socket, char *filename);
void f_read(int Socket, char *filename);
void f_update(int Socket, char *localfilename, char *remotefilename);
void f_delete(int Socket, char *remotefilename);

void getresponse(int Socket);
