#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/errno.h>
#include <netdb.h>

#include "provenance.h"

#define SERVER_PORT 3301
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

// provide socket connection info
struct sock_info get_sock_info() {

	struct sock_info new_sock_info;

	struct sockaddr_in client_addr;
	bzero(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	client_addr.sin_port = htons(0);

	new_sock_info.client_addr = client_addr;

	int client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0) {
		printf("Create socket failed!\n");
		return;
	}

	new_sock_info.client_socket = client_socket;

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_family = AF_INET;
	if (inet_aton(SERVER_IP, &server_addr.sin_addr) == 0) {
		printf("Server address error!\n");
		return;
	}

	new_sock_info.server_addr = server_addr;

	socklen_t server_addr_length = sizeof(server_addr);

	new_sock_info.server_addr_length = server_addr_length;

	return new_sock_info;
}

void printLog(struct sock_info new_sock_info) {
	printf("Client Sock -- %d \n", new_sock_info.client_socket);
}

int spade_receivefile(const char *original_path, const char *original_ip, const char *local_path, const char *size, const char *mtime) {

	struct sock_info new_sock_info = get_sock_info();
	int client_socket = new_sock_info.client_socket;
	struct sockaddr_in client_addr = new_sock_info.client_addr;
	struct sockaddr_in server_addr = new_sock_info.server_addr;
	socklen_t server_addr_length = new_sock_info.server_addr_length;

	if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
		printf("Client bind port failed!\n");
		return 1;
	}

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
		printf("Cannot connect to %s\n", SERVER_IP);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	char *oper = "receivefile";

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, oper);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, original_path);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, original_ip);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, local_path);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, size);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, mtime);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int length = recv(client_socket, buffer, BUFFER_SIZE, 0);
	char *ret;
	strcpy(ret, buffer);
	if (length < 0 || strcmp(ret, "FAIL") == 0)
		return 1;

	close(client_socket);

	return 0;
}

int spade_sendfile(const char *source_path, const char *dist_path, const char *dist_ip) {

	struct sock_info new_sock_info = get_sock_info();
	int client_socket = new_sock_info.client_socket;
	struct sockaddr_in client_addr = new_sock_info.client_addr;
	struct sockaddr_in server_addr = new_sock_info.server_addr;
	socklen_t server_addr_length = new_sock_info.server_addr_length;

	if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
		printf("Client bind port failed!\n");
		return 1;
	}

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
		printf("Cannot connect to %s\n", SERVER_IP);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	char *oper = "sendfile";

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, oper);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, source_path);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, dist_path);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, dist_ip);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	close(client_socket);

	return 0;

}

int spade_create(const char *path, pid_t pid) {
	struct sock_info new_sock_info = get_sock_info();
	int client_socket = new_sock_info.client_socket;
	struct sockaddr_in client_addr = new_sock_info.client_addr;
	struct sockaddr_in server_addr = new_sock_info.server_addr;
	socklen_t server_addr_length = new_sock_info.server_addr_length;

	if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
		printf("Client bind port failed!\n");
		return 1;
	}

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
		printf("Cannot connect to %s\n", SERVER_IP);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	char *oper = "create";

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, oper);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, path);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int i_pid = (int) pid;
	sprintf(buffer, "%d", i_pid);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int length = recv(client_socket, buffer, BUFFER_SIZE, 0);
	char *ret;
	strcpy(ret, buffer);
	if (length < 0 || strcmp(ret, "FAIL") == 0)
		return 1;

	close(client_socket);

	return 0;
}

int spade_readlink(const char *path, pid_t pid, int iotime) {

	struct sock_info new_sock_info = get_sock_info();
	int client_socket = new_sock_info.client_socket;
	struct sockaddr_in client_addr = new_sock_info.client_addr;
	struct sockaddr_in server_addr = new_sock_info.server_addr;
	socklen_t server_addr_length = new_sock_info.server_addr_length;

	printLog(new_sock_info);

	if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
		printf("Client bind port failed!\n");
		return 1;
	}

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
		printf("Cannot connect to %s\n", SERVER_IP);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	char *oper = "readlink";

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, oper);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, path);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int i_pid = (int) pid;
	sprintf(buffer, "%d", i_pid);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	sprintf(buffer, "%d", iotime);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	close(client_socket);

	return 0;
}

int spade_unlink(const char *path, pid_t pid) {

	struct sock_info new_sock_info = get_sock_info();
	int client_socket = new_sock_info.client_socket;
	struct sockaddr_in client_addr = new_sock_info.client_addr;
	struct sockaddr_in server_addr = new_sock_info.server_addr;
	socklen_t server_addr_length = new_sock_info.server_addr_length;

	printLog(new_sock_info);

	if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
		printf("Client bind port failed!\n");
		return 1;
	}

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
		printf("Cannot connect to %s\n", SERVER_IP);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	char *oper = "unlink";

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, oper);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, path);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int i_pid = (int) pid;
	sprintf(buffer, "%d", i_pid);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	close(client_socket);

	return 0;
}

int spade_symlink(const char *from, const char *to, pid_t pid) {

	struct sock_info new_sock_info = get_sock_info();
	int client_socket = new_sock_info.client_socket;
	struct sockaddr_in client_addr = new_sock_info.client_addr;
	struct sockaddr_in server_addr = new_sock_info.server_addr;
	socklen_t server_addr_length = new_sock_info.server_addr_length;

	printLog(new_sock_info);

	if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
		printf("Client bind port failed!\n");
		return 1;
	}

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
		printf("Cannot connect to %s\n", SERVER_IP);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	char *oper = "symlink";

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, oper);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, from);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, to);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int i_pid = (int) pid;
	sprintf(buffer, "%d", i_pid);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	close(client_socket);

	return 0;

}

int spade_rename(const char *from, const char *to, pid_t pid, int link, int iotime) {

	struct sock_info new_sock_info = get_sock_info();
	int client_socket = new_sock_info.client_socket;
	struct sockaddr_in client_addr = new_sock_info.client_addr;
	struct sockaddr_in server_addr = new_sock_info.server_addr;
	socklen_t server_addr_length = new_sock_info.server_addr_length;

	printLog(new_sock_info);

	if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
		printf("Client bind port failed!\n");
		return 1;
	}

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
		printf("Cannot connect to %s\n", SERVER_IP);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	char *oper = "rename";

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, oper);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, from);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, to);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int i_pid = (int) pid;
	sprintf(buffer, "%d", i_pid);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	sprintf(buffer, "%d", link);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	sprintf(buffer, "%d", iotime);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	close(client_socket);

	return 0;
}

int spade_link(const char *from, const char *to, pid_t pid) {

	struct sock_info new_sock_info = get_sock_info();
	int client_socket = new_sock_info.client_socket;
	struct sockaddr_in client_addr = new_sock_info.client_addr;
	struct sockaddr_in server_addr = new_sock_info.server_addr;
	socklen_t server_addr_length = new_sock_info.server_addr_length;

	printLog(new_sock_info);

	if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
		printf("Client bind port failed!\n");
		return 1;
	}

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
		printf("Cannot connect to %s\n", SERVER_IP);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	char *oper = "link";

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, oper);
	printf("Sending %s...\n", buffer);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, from);
	printf("Sending %s...\n", buffer);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, to);
	printf("Sending %s...\n", buffer);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int i_pid = (int) pid;
	sprintf(buffer, "%d", i_pid);
	printf("Sending %s...\n", buffer);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	close(client_socket);

	return 0;
}

int spade_read(const char *path, pid_t pid, int iotime, int link, const char *size, const char *mtime) {

	struct sock_info new_sock_info = get_sock_info();
	int client_socket = new_sock_info.client_socket;
	struct sockaddr_in client_addr = new_sock_info.client_addr;
	struct sockaddr_in server_addr = new_sock_info.server_addr;
	socklen_t server_addr_length = new_sock_info.server_addr_length;

	printLog(new_sock_info);

	if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
		printf("Client bind port failed!\n");
		return 1;
	}

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
		printf("Cannot connect to %s\n", SERVER_IP);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	char *oper = "read";

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, oper);
	printf("Sending %s...\n", buffer);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, path);
	printf("Sending %s...\n", buffer);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int i_pid = (int) pid;
	sprintf(buffer, "%d", i_pid);
	printf("Sending %s...\n", buffer);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	sprintf(buffer, "%d", iotime);
	printf("Sending %s...\n", buffer);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	sprintf(buffer, "%d", link);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, size);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, mtime);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int length = recv(client_socket, buffer, BUFFER_SIZE, 0);
	char *ret;
	strcpy(ret, buffer);
	if (length < 0 || strcmp(ret, "FAIL") == 0)
		return 1;

	close(client_socket);

	return 0;
}

int spade_write(const char *path, pid_t pid, int iotime, int link) {

	struct sock_info new_sock_info = get_sock_info();
	int client_socket = new_sock_info.client_socket;
	struct sockaddr_in client_addr = new_sock_info.client_addr;
	struct sockaddr_in server_addr = new_sock_info.server_addr;
	socklen_t server_addr_length = new_sock_info.server_addr_length;

	printLog(new_sock_info);

	if(bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr))) {
		printf("Client bind port failed!\n");
		return 1;
	}

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {
		printf("Cannot connect to %s\n", SERVER_IP);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	char *oper = "write";

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, oper);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, path);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int i_pid = (int) pid;
	sprintf(buffer, "%d", i_pid);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	sprintf(buffer, "%d", iotime);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	sprintf(buffer, "%d", link);
	send(client_socket, buffer, BUFFER_SIZE, 0);

	bzero(buffer, BUFFER_SIZE);
	int length = recv(client_socket, buffer, BUFFER_SIZE, 0);
	char *ret;
	strcpy(ret, buffer);
	if (length < 0 || strcmp(ret, "FAIL") == 0)
		return 1;

	close(client_socket);

	return 0;
}

// main(int argc, char *argv) {

// 	pid_t pid = (pid_t)3301;
// 	spade_readlink("ubuntu/cshou/fakepath", pid, 2);
// 	spade_unlink("ubuntu/cshou/fakepath", pid);
// 	spade_link("ubuntu/cshou/fakepath/name1", "ubuntu/cshou/fakepath/name2", pid);
// 	spade_symlink("ubuntu/cshou/fakepath/name1", "ubuntu/cshou/fakepath/name2", pid);
// 	spade_rename("ubuntu/cshou/fakepath/name1", "ubuntu/cshou/fakepath/name2", pid, 1, 5);
// 	spade_read("ubuntu/cshou/fakepath", pid, 3, 1);
// 	spade_write("ubuntu/cshou/fakepath", pid, 3, 1);

// }