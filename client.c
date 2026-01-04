#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include "game.h"

#define BUFFER_SIZE 1024

typedef struct {
	int client_fd;
	pthread_mutex_t mutex;
} data_t;

void* client_input(void* arg) {	
	data_t* data = (data_t*)arg;

	while (1) {
		char ch = getch();
		if (ch != ERR) {
			send(data->client_fd, &ch, 1, 0);
		}
	}

	return NULL;
}

void* client_render(void* arg) {
	data_t* data = (data_t*)arg;
	game_t game;
	memset(&game, 0, sizeof(game));

	while (1) {
		int r = recv(data->client_fd, &game, sizeof(game_t), 0);
		if (r <= 0) break;

		pthread_mutex_lock(&data->mutex);
		clear();
		snake_t* snake = &game.snakes[0];
		for (int i = 0; i < snake->length; i++) {
			mvaddch(snake->body[i].y,  snake->body[i].x, 'O');
		}
		refresh();
		pthread_mutex_unlock(&data->mutex);
	}

	return NULL;
}

int connected(int port) {
	char buffer[BUFFER_SIZE];
	data_t data;
	data.client_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (data.client_fd < 0) {
		perror("Vytvorenie socketu zlyhalo\n");
		return 1;
	}
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));		
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, "127.0.0.1", (struct sockaddr*)&server_addr.sin_addr) < 0) {
		perror("Adresa je neplatna\n");
		close(data.client_fd);
		return 2;
	}

	if (connect(data.client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Pripojenie k serveru zlyhalo\n");
		close(data.client_fd);
		return 3;
	}	

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);

	
	pthread_mutex_init(&data.mutex, NULL);
	pthread_t input_th, render_th;
	pthread_create(&input_th, NULL, client_input, &data);
	pthread_create(&render_th, NULL, client_render, &data);

	pthread_join(input_th, NULL);
	pthread_join(render_th, NULL);

	pthread_mutex_destroy(&data.mutex);
	endwin();
	close(data.client_fd);
	return 0;

}


int main(int argc, char** argv) {
	int menuChoice = -1;
	int regimeChoice = -1;
	int worldChoice = -1;
	int gameTime = 0;
	char* x;
	char* y;
	int port;
	_Bool isPaused = 0;

	printf("****Hlavné menu****\n");
	printf("[1] Nová hra\n");
	printf("[2] Pripojenie k hre\n");
	if (isPaused)
		printf("[3] Pokračovať v hre\n");
	printf("[0] Ukončiť\n");
	printf("Vyber si jednu z možností (zadaj číselnú hodnotu)\n");
	
	scanf("%d", &menuChoice);

	if (menuChoice == 1) {
		printf("Vyber herný režim\n");
		printf("[1] Štandardný\n");
		printf("[2] Časový\n");

		scanf("%d", &regimeChoice);
		
		if (regimeChoice == 2) {
			printf("Zadaj čas hry\n");
			scanf("%d", &gameTime);
		}

		printf("Vyber typ herného sveta\n");
		printf("[1] Bez prekážok\n");
		printf("[2] S prekážkami\n");

		scanf("%d", &worldChoice);

		printf("Zadaj výšku herného sveta\n");
		scanf("%s", &y);
	
		printf("Zadaj šírku herného sveta\n");
		scanf("%s", &x);

		printf("Zadaj port\n");
		scanf("%d", &port);
		char portStr[6];
		snprintf(portStr, sizeof(portStr), "%d", port);

		pid_t pid = fork();
		if (pid < 0) {
			perror("Fork zlyhal\n");
			exit(1);
		}

		if (pid == 0) {
			execl("./server", "./server", portStr, x, y, NULL);
		}
		else {
			sleep(1);
			return connected(port);
		}


	}

	if (menuChoice == 2) {
		printf("Zadaj port\n");
		scanf("%d", &port);
		return connected(port);
	}
	
	return 0;
}
