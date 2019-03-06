#ifndef SSOCKET_H
#define SSOCKET_H

extern char * doc_root;
extern char * index_name;

int open_server_socket (char* port);
int accept_client_connection (void);
int close_server_socket (void);
void set_doc_root (char * new_root);

#endif
