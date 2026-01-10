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
	int server_fd;
	int clients[MAX_CLIENTS];
	client_data_t* clientData[MAX_CLIENTS];
	_Bool isOff;
	_Bool gameOver;
	_Bool singleplayer;
	_Bool pauseGame;
	game_t game;
	pthread_mutex_t mutex;
};

struct client_data {
	int client_fd;
	data_t* data;
	int id;
};

void remove_client(data_t* data, int index) {
	int last = data->clientCount - 1;
	data->game.snakes[index].alive = 0;

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

	while (!data->isOff) {
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
	_Bool shutdown = data->isOff;
	if (!shutdown) {
		remove_client(data, client->id);
	}
	pthread_mutex_unlock(&data->mutex);
	if (!shutdown) {
		close(client->client_fd);
		free(client);
	}
	return NULL;
}

void* accept_clients(void* arg) {
	data_t* data = (data_t*)arg;
	int nextID = 0;
	
	while (!data->isOff && !data->gameOver) {	
		int client_fd = accept(data->server_fd, NULL, NULL);
		if (client_fd < 0) {
			usleep(10000);
			continue;
		}
		
		pthread_mutex_lock(&data->mutex);
		if (data->singleplayer && data->clientCount >= 1) {
			pthread_mutex_unlock(&data->mutex);
			close(client_fd);
			continue;
		}

		client_data_t* client = malloc(sizeof(client_data_t));
		client->client_fd = client_fd;
		client->data = data;

		int id = add_snake(&data->game, nextID);
		nextID++;
		

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
		data->pauseGame = 1;
		data->gameOver = 0;
		pthread_mutex_unlock(&data->mutex);

		pthread_t client_th;
		pthread_create(&client_th, NULL, client_message, client);
		pthread_detach(client_th);
	}
	
	return NULL;
}

void* server_shutdown(void* arg) {	
	data_t* data = (data_t*)arg;
	int countdown = 10;

	while (!data->isOff && countdown != 0) {	
		pthread_mutex_lock(&data->mutex);
		int count = data->clientCount;
		_Bool gameOver = data->gameOver;
		pthread_mutex_unlock(&data->mutex);

		if (count == 0 || gameOver) {
			sleep(1);
			countdown--;
		
		}
		else
			countdown = 10;
	}

	pthread_mutex_lock(&data->mutex);
	data->isOff = 1;
	pthread_mutex_unlock(&data->mutex);

	sleep(1);

	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (data->clientData[i]) {
			shutdown(data->clientData[i]->client_fd, SHUT_RDWR);
		}
	}

	pthread_mutex_lock(&data->mutex);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (data->clientData[i]) {
			close(data->clientData[i]->client_fd);
			free(data->clientData[i]);
			data->clientData[i] = NULL;
		}
	}

	close(data->server_fd);
	pthread_mutex_unlock(&data->mutex);
		
	return NULL;
}

void* game_loop(void* arg) {
	data_t* data = (data_t*)arg;

	while (!data->isOff) {
		usleep(200000);

		pthread_mutex_lock(&data->mutex);
	
		if(data->gameOver) {
			data->isOff = 1;
			pthread_mutex_unlock(&data->mutex);
			break;
		}

		time_t now = time(NULL);
		data->game.elapsedTime = (int)difftime(now, data->game.startTime);

		if (data->game.isTimed) {
			if (data->game.elapsedTime >= data->game.maxGameTime) {
				data->gameOver = 1;
			}
		}
	
		int dead = 0;
		for (int i = 0; i < data->clientCount; i++) 
			if(!data->game.snakes[i].alive)
				dead++;
		

		if (data->clientCount > 0 && dead == data->clientCount) 
			data->gameOver = 1;
		

		update_game(&data->game);	

		for (int i = 0; i < data->clientCount; i++) {

			ssize_t s = send(data->clients[i], &data->game, sizeof(game_t), 0);
		}

		if (data->pauseGame) {
			data->pauseGame = 0;
			sleep(3);
		}
		pthread_mutex_unlock(&data->mutex);
	}

	return NULL;
}

int main(int argc, char** argv) {
	if (argc != 8) {
		fprintf(stderr, "Nesprávny počet argumentov\n");
		return -1;
	}

	data_t data;
	memset(&data, 0, sizeof(data));

	data.server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (data.server_fd < 0) {
		perror("Chyba pri vytvárani socketu\n");
		return -2;
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
		return -3;
	}

	if (listen(data.server_fd, MAX_CLIENTS) < 0) {
		perror("Listen zlyhal\n");
		close(data.server_fd);
		return -4;
	}
	int flags = fcntl(data.server_fd, F_GETFL, 0);
	fcntl(data.server_fd, F_SETFL, flags | O_NONBLOCK);

	srand(time(NULL));
	data.pauseGame = 0;
	data.clientCount = 0;
	data.isOff = 0;
	data.gameOver = 0;
	data.singleplayer = (atoi(argv[2]) == 1);
	_Bool hasObstacles = (atoi(argv[5]) == 2);
	init_game(&data.game, hasObstacles, atoi(argv[6]), atoi(argv[7]));
	data.game.startTime = time(NULL);
	if (atoi(argv[3]) == 2) {
		data.game.maxGameTime = atoi(argv[4]);
		data.game.isTimed = 1;
	}
	pthread_mutex_init(&data.mutex, NULL);

	pthread_t accept_th, shutdown_th, game_th;
	pthread_create(&accept_th, NULL, accept_clients, &data);
	pthread_create(&shutdown_th, NULL, server_shutdown, &data);
	pthread_create(&game_th, NULL, game_loop, &data);

	pthread_join(accept_th, NULL);
	pthread_join(shutdown_th, NULL);
	pthread_join(game_th, NULL);

	pthread_mutex_destroy(&data.mutex);

	return 0;
}
