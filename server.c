#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include "game.h"

#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

typedef struct client_data client_data_t;
typedef struct data data_t;

struct data {
	int clientCount;
	int in, out;
	int server_fd;
	int clients[MAX_CLIENTS];
	client_data_t* clientData[MAX_CLIENTS];
	_Bool isOff;
	_Bool gameOver;
	_Bool isTimed;
	time_t startTime;
	int maxGameTime;
	game_t game;
	sem_t space;
	sem_t clientsSem;
	pthread_mutex_t mutex;
};

struct client_data {
	int client_fd;
	data_t* data;
	int id;
};

void remove_client(data_t* data, int index) {
	int last = data->clientCount - 1;
	data->>game.snakes[index].alive = 0;

	if (index != last) {
		data->clients[index] = data->clients[last];
		data->game.snakes[index] = data->game.snakes[last];
		data->clientData[index] = data->clientData[last];

		data->clientData[index]->id = index;
	}

	data->clientData[last] = NULL;
	data->clientCount--;
	data->game.playerCount--;
}

void* client_message(void* arg) {
	client_data_t* client = (client_data_t*)arg;
	data_t* data = client->data;
	game_t* game = &data->game;
	char ch;

	while (1) {
		int read = recv(client->client_fd, &ch, 1, 0);
		if (read <= 0)
			break;
		
		pthread_mutex_lock(&data->mutex);
		switch (ch) {
			case 'w': set_direction(&game->snakes[client->id], UP); break;			
			case 's': set_direction(&game->snakes[client->id], DOWN); break;			
			case 'a': set_direction(&game->snakes[client->id], LEFT); break;			
			case 'd': set_direction(&game->snakes[client->id], RIGHT); break;
		}
		pthread_mutex_unlock(&data->mutex);
	}
	pthread_mutex_lock(&data->mutex);	
	remove_client(data, client->id);
	sem_post(&data->space);
	pthread_mutex_unlock(&data->mutex);
	close(client->client_fd);
	free(client);
	return NULL;
}

void* accept_clients(void* arg) {
	data_t* data = (data_t*)arg;

	while (1) {
		pthread_mutex_lock(&data->mutex);
		if (data->isOff || data->gameOver) {
			pthread_mutex_unlock(&data->mutex);
			break;
		}
		pthread_mutex_unlock(&data->mutex);

		int client_fd = accept(data->server_fd, NULL, NULL);
		if (client_fd < 0) continue;
		
		client_data_t* client = malloc(sizeof(client_data_t));
		client->client_fd = client_fd;
		client->data = data;

		sem_wait(&data->space);
		pthread_mutex_lock(&data->mutex);

		int id = add_snake(&data->game);
		if (id < 0)
		{
			pthread_mutex_unlock(&data->mutex);
			close(client_fd);
			free(client);
			continue;
		}
		
		client->id = id;
		data->clients[id] = client_fd;
		data->clientData[id] = client;
		data->clientCount++;
			//printf("Klient sa pripojil!\n");

		pthread_mutex_unlock(&data->mutex);
		sem_post(&data->clientsSem);

		pthread_t client_th;
		pthread_create(&client_th, NULL, client_message, client);
		pthread_detach(client_th);	
		sleep(3);
	}
	
	for (int i = 0; i < data->game.playerCount; i++) {
		data->game.snakes[i].alive = 0;
	}

	return NULL;
}

void* server_shutdown(void* arg) {	
	data_t* data = (data_t*)arg;
	int countdown = 10;

	while (countdown != 0) {	
		pthread_mutex_lock(&data->mutex);
		int count = data->clientCount;
		pthread_mutex_unlock(&data->mutex);

		if (count == 0 || data->gameOver) {
			//printf("Server sa vypne za %d sekúnd.\n", countdown);
			sleep(1);
			countdown--;

			if (countdown == 0) {
				pthread_mutex_lock(&data->mutex);
				data->isOff = 1;
				close(data->server_fd);
				pthread_mutex_unlock(&data->mutex);
			}
		}
		else
			countdown = 10;
	}
	//printf("Server je vypnutý!\n");

	return NULL;
}

void* game_loop(void* arg) {
	data_t* data = (data_t*)arg;
	
	while (1) {
		usleep(200000);

		pthread_mutex_lock(&data->mutex);
		
		if (data->isTimed) {
			time_t now = time(NULL);
			if (difftime(now, data->startTime) >= data->maxGameTime) {
				data->gameOver = 1;
				pthread_mutex_unlock(&data->mutex);
				break;
			}
		}

		update_game(&data->game);
		for (int i = 0; i < data->clientCount; i++) {
			send(data->clients[i], &data->game, sizeof(game_t), 0);
		}
		pthread_mutex_unlock(&data->mutex);
	}

	return NULL;
}

int main(int argc, char** argv) {
	if (argc != 8) {
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
	int flags = fcntl(data.server_fd, F_GETFL, 0);
	fcntl(data.server_fd, F_SETFL, flags | O_NONBLOCK);

	data.clientCount = 0;
	data.isOff = 0;
	data.in = 0;
	data.out = 0;
	data.gameOver = 0;
	data.isTimed = 0;
	init_game(&data.game, atoi(argv[6]), atoi(argv[7]));
	if (atoi(argv[3]) == 2) {
		data.startTime = time(NULL);
		data.maxGameTime = atoi(argv[4]);
		data.isTimed = 1;
	}
	pthread_mutex_init(&data.mutex, NULL);
	sem_init(&data.space, 0, MAX_CLIENTS);
	sem_init(&data.clientsSem, 0, 0);


	pthread_t accept_th, shutdown_th, game_th;
	pthread_create(&accept_th, NULL, accept_clients, &data);
	pthread_create(&shutdown_th, NULL, server_shutdown, &data);
	pthread_create(&game_th, NULL, game_loop, &data);

	pthread_join(accept_th, NULL);
	pthread_join(shutdown_th, NULL);
	pthread_detach(game_th);

	pthread_mutex_destroy(&data.mutex);
	sem_destroy(&data.space);
	sem_destroy(&data.clientsSem);
	return 0;
}
