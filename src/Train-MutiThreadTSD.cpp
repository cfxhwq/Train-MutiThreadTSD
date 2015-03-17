//============================================================================
// Name        : Train-MutiThreadTSD.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <iostream>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
using namespace std;

#define PORT 1234
#define MAXDATASIZE 100
#define BACKLOG 2

void process_cli(int connectfd, struct sockaddr_in client);
void *start_routine(void *arg);
void savedata(char *recvbuf, int len, char *cli_data);
struct ARG {
	int connfd;
	sockaddr_in client;
};
struct ARG *arg;
int main() {

	int listenfd, connectfd;
	pthread_t tid;
	struct sockaddr_in server;
	struct sockaddr_in client;
	socklen_t addrlen;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket() error.");
		exit(-1);
	}

	int opt = SO_REUSEADDR;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))
			== -1) {
		perror("setsockopt() error ");
		exit(-1);
	}

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr *) &server, sizeof(server)) == -1) {
		cout << server.sin_addr.s_addr << endl;
		perror("bind() error");
		exit(-1);
	}
	cout << "bind finish" << endl;

	if (listen(listenfd, BACKLOG) == -1) {
		perror("listen() error");
		exit(-1);
	}
	cout << "listen finish" << endl;

	addrlen = sizeof(client);
	while (1) {
		if ((connectfd = accept(listenfd, (struct sockaddr *) &client, &addrlen))
				== -1) {
			perror("accept() error");
			exit(-1);
		}
		arg = (struct ARG*) malloc(sizeof(struct ARG));
		arg->connfd = connectfd;
		memcpy((void*) &arg->client, &client, sizeof(client));

		if (pthread_create(&tid, NULL, start_routine, (void*) arg)) {
			perror("pthread_create() error");
			exit(-1);
		}
	}
	close(listenfd);
}

static pthread_key_t key;
static pthread_once_t once = PTHREAD_ONCE_INIT;
struct DATA_THR {
	int index;
};
static void key_destroy(void *buf) {
	free(buf);
}
static void buffer_key_alloc() {
	cout << "create TSD!" << endl;
	pthread_key_create(&key, key_destroy);
}

void savedata(char *recvbuf, int len, char *cli_data) {
	struct DATA_THR *data;
	pthread_once(&once, buffer_key_alloc);
	if ((data = (struct DATA_THR*) pthread_getspecific(key)) == NULL) {
		data = (struct DATA_THR *) calloc(1, sizeof(struct DATA_THR));
		pthread_setspecific(key, data);
		data->index = 0;
	}
	int i;
	for (i = 0; i < len; ++i) {
		cli_data[data->index++] = recvbuf[i];
	}
	cli_data[data->index] = '\0';
}
void process_cli(int connectfd, struct sockaddr_in client) {
	int num, i;
	char recvbuf[MAXDATASIZE], sendbuf[MAXDATASIZE], cli_name[MAXDATASIZE];
	char client_data[5000];
	printf("process from %s:%d\n", inet_ntoa(client.sin_addr),
			ntohs(client.sin_port));
	send(connectfd, "Welcome!\n", 9, 0);
	num = recv(connectfd, cli_name, MAXDATASIZE, 0);
	if (num == 0) {
		close(connectfd);
		printf("disconnect!\n");
	} else if (num < 0) {
		close(connectfd);
		printf("connect broken!\n");
		return;
	}
	cli_name[num] = '\0';
	printf("client name: %s,len:%d\n", cli_name, num);
	bzero(recvbuf, 100);
//	int j=0;
	while (num = recv(connectfd, recvbuf, MAXDATASIZE, 0)) {
		savedata(recvbuf, num, client_data);
//		for (i = 0; i < num; ++i) {
//			client_data[j++] = recvbuf[i];
//		}
		cout << "key:" << key << endl;
		recvbuf[num] = '\0';
		printf("Msg: %s,len:%d\n", recvbuf, num);
		int i = 0;
		for (i = 0; i < num; ++i) {
			sendbuf[i] = recvbuf[num - i - 1];
		}
		sendbuf[num] = '\0';
		send(connectfd, sendbuf, strlen(sendbuf), 0);
	}
	printf("client name: %s disconnect!User's data:%s\n", cli_name,
			client_data);
	close(connectfd);
}

void *start_routine(void *arg) {
	struct ARG *info;
	info = (struct ARG*) arg;
	process_cli(info->connfd, info->client);
	free(arg);
	pthread_exit(NULL);
}
