#ifndef _PROVENANCE_H_
#define _PROVENANCE_H_

#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <netdb.h>

struct sock_info
{
	int client_socket;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	socklen_t server_addr_length;
};

#ifdef __cplusplus
extern "C" {
#endif

int spade_readlink(const char*, pid_t, int);
int spade_unlink(const char*, pid_t);
int spade_symlink(const char*, const char*, pid_t);
int spade_rename(const char*, const char*, pid_t, int, int);
int spade_link(const char*, const char*, pid_t);
int spade_read(const char*, pid_t, int, int, const char*, const char*);
int spade_write(const char*, pid_t, int, int);
int spade_receivefile(const char*, const char*, const char*, const char*, const char*);
int spade_create(const char*, pid_t);
int spade_sendfile(const char*, const char*, const char*);

#ifdef __cplusplus
}
#endif

#endif