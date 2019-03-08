#ifndef SSOCKET_H
#define SSOCKET_H

extern char * doc_root;
extern char * index_name;

int open_server_socket (char* port);
char* accept_client_connection (void);
int close_server_socket (void);
int load_file (char* file_path);
void unload_quotes (void);

#endif
