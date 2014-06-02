#ifndef CLIENTLIB
#define CLIENTLIB

/*
functions used by client
*/

void f_list(int Socket);
void f_create(int Socket, char *filename);
void f_read(int Socket, char *filename);
void f_update(int Socket, char *localfilename, char *remotefilename);
void f_delete(int Socket, char *remotefilename);

void getResponseList(int Socket);
void getResponseCreate(int Socket);
void getResponseRead(int Socket);
void getResponseUpdate(int Socket);
void getResponseDelete(int Socket);
#endif
