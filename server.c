#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#define MAX_CLIENTS 10

typedef struct {
	int clientCount;
	int server_fd;
	_Bool isOff;
	sem_t space;
	sem_t clients;
	pthread_mutex_t mutex;
} data_t;

void* accept_clients(void* arg) {
	data_t* data = (data_t*)arg;

	while (!data->isOff) {

		int client_fd = accept(data->server_fd, NULL, NULL);
		if (client_fd < 0) continue;

		sem_wait(&data->space);
		pthread_mutex_lock(&data->mutex);
		data->clientCount++;
		printf("Klient sa pripojil!\n");
		pthread_mutex_unlock(&data->mutex);
		sem_post(&data->clients);
	}

	return NULL;
}

void* server_shutdown(void* arg) {	
	data_t* data = (data_t*)arg;
	int countdown = 10;

	while (countdown != 0) {
		if (data->clientCount == 0) {
			printf("Server sa vypne za %d sekúnd.\n", countdown);
			sleep(1);
			countdown--;
			if (countdown == 0)
				data->isOff = 1;
		}
		else
			countdown = 10;
	}

	printf("Server je vypnutý!\n");

	return NULL;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "Nesprávny počet argumentov\n");
		return 1;
	}

	data_t data;

	data.server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (data.server_fd < 0) {
		perror("Chyba pri vytvárani socketu\n");
		return 2;
	}

	int opt = 1;
	setsockopt(data.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(atoi(argv[1]));

	if (bind(data.server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("Chyba pri binde\n");
		close(data.server_fd);
		return 3;
	}

	if (listen(data.server_fd, MAX_CLIENTS) < 0) {
		perror("Listen zlyhal\n");
		close(data.server_fd);
		return 4;
	}
	data.clientCount = 0;
	data.isOff = 0;
	pthread_mutex_init(&data.mutex, NULL);
	sem_init(&data.space, 0, MAX_CLIENTS);
	sem_init(&data.clients, 0, 0);

	pthread_t accept_th, shutdown_th;
	pthread_create(&accept_th, NULL, accept_clients, &data);
	pthread_create(&shutdown_th, NULL, server_shutdown, &data);

	pthread_detach(accept_th);
	pthread_join(shutdown_th, NULL);

	pthread_mutex_destroy(&data.mutex);
	sem_destroy(&data.space);
	sem_destroy(&data.clients);
	close(data.server_fd);
	return 0;
}
