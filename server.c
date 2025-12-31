#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define MAX_CLIENTS 10

void* accept_clients(void* arg) {
	int server_fd = *(int*)arg;
	while (1) {
		int client_fd = accept(server_fd, NULL, NULL);
		if (client_fd < 0) continue;

		printf("Klient sa pripojil!\n");
	}
} 

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "Nesprávny počet argumentov\n");
		return 1;
	}

	int server_fd;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("Chyba pri vytvárani socketu\n");
		return 2;
	}

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(atoi(argv[1]));

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("Chyba pri binde\n");
		close(server_fd);
		return 3;
	}

	if (listen(server_fd, MAX_CLIENTS) < 0) {
		perror("Listen zlyhal\n");
		close(server_fd);
		return 4;
	}

	pthread_t accept_th;
	pthread_create(&accept_th, NULL, accept_clients, &server_fd);

	close(server_fd);
	return 0;
}
