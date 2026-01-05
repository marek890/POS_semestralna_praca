#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include <locale.h>
#include "game.h"

#define BUFFER_SIZE 1024

typedef struct {
	int client_fd;
	_Bool isPaused;
	pthread_mutex_t mutex;
} data_t;

ssize_t recv_all(int fd, void* buffer, size_t size) {
	size_t received = 0;
	while (received < size) {
		ssize_t r = recv(fd, (char*)buffer + received, size - received, 0);
		if (r <= 0) return r;
		received += r;
	}
	return received;
}

int show_main_menu(data_t* data) {
	int choice = -1;

	while(1) {
		erase();
		mvprintw(2, 2, "**** Hlavné menu ****");
		mvprintw(4, 2, "[1] Nová hra");
		mvprintw(5, 2, "[2] Pripojenie k hre");
		if (data->isPaused)
			mvprintw(6, 2, "[3] Pokračovať v hre");
		mvprintw(7, 2, "[0] Ukončiť");

		int ch = getch();
		if (ch == ERR) {
			usleep(5000);
			continue;
		}

		if (ch == '0') return 0;
		if (ch == '1') return 1;
		if (ch == '2') return 2;
		if (ch == '3' && data->isPaused) return 3;

		refresh();
	}
}

void* client_input(void* arg) {	
	data_t* data = (data_t*)arg;

	while (1) {
	
		int ch = getch();
		if (ch == ERR) {
			usleep(5000);
			continue;
		}

		char out;
		switch (ch) {
			case 'p':
				pthread_mutex_lock(&data->mutex);
				data->isPaused = 1;
				pthread_mutex_unlock(&data->mutex);
				continue;
			case KEY_UP:	out = 'w'; break;
			case KEY_DOWN:	out = 's'; break;
			case KEY_LEFT:	out = 'a'; break;
			case KEY_RIGHT:	out = 'd'; break;
			case 'w':
			case 'a':
			case 's':
			case 'd':
				out = ch;
				break;
			default:
				continue;

		}
		pthread_mutex_lock(&data->mutex);
		if (!data->isPaused)
			send(data->client_fd, &out, 1, 0);
		pthread_mutex_unlock(&data->mutex);
	}

	return NULL;
}

void* client_render(void* arg) {
	data_t* data = (data_t*)arg;
	game_t game;
	memset(&game, 0, sizeof(game));

	while (1) {
		usleep(50000);
		pthread_mutex_lock(&data->mutex);

		if (!data->isPaused) {
			int r = recv_all(data->client_fd, &game, sizeof(game_t));
			if (r <= 0) break;

			erase();
	
			for (int i = 0; i < game.width + 2; i++) {
				mvaddch(0, i, '#');
				mvaddch(game.length + 1, i, '#');
			}

			for (int i = 0; i < game.length + 2; i++) {
				mvaddch(i, 0, '#');
				mvaddch(i, game.width + 1, '#');
			}

			for (int i = 0; i < game.obstacleCount; i++) {
				int x = game.obstacles[i].pos.x;
				int y = game.obstacles[i].pos.y;
				mvaddch(y, x, '@');
			} 

			for (int i = 0; i < game.playerCount; i++) {
				snake_t* snake = &game.snakes[i];
				for (int j = 0; j < snake->length; j++) {
					if (snake->body[j].x >= 0 && snake->body[j].x < game.width &&
						snake->body[j].y >= 0 && snake->body[j].y < game.length) {
							mvaddch(snake->body[j].y + 1, snake->body[j].x + 1, j == 0 ? 'O' : 'o');
					}
				}
			}

			for (int i = 0; i < game.playerCount; i++) {
				fruit_t* fruit = &game.fruits[i];
				mvaddch(fruit->pos.y + 1, fruit->pos.x + 1, 'F');
			}

			refresh();
		}
		pthread_mutex_unlock(&data->mutex);

	}

	return NULL;
}

int connected(int port, data_t* data) {
	data->client_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (data->client_fd < 0) {
		perror("Vytvorenie socketu zlyhalo\n");
		return 1;
	}
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));		
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, "127.0.0.1", (struct sockaddr*)&server_addr.sin_addr) < 0) {
		perror("Adresa je neplatna\n");
		close(data->client_fd);
		return 2;
	}

	if (connect(data->client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Pripojenie k serveru zlyhalo\n");
		close(data->client_fd);
		return 3;
	}	
	
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);

	pthread_mutex_init(&data->mutex, NULL);
	pthread_t input_th, render_th;
	pthread_create(&input_th, NULL, client_input, data);
	pthread_create(&render_th, NULL, client_render, data);

	while (1) {
		if (data->isPaused) {
			int choice = show_main_menu(data);
			pthread_mutex_lock(&data->mutex);
			
			if (choice == 3) {
				data->isPaused = 0;
			}
			else if (choice == 0) {
				pthread_mutex_unlock(&data->mutex);
				break;
			} else if (choice == 1) {
				pthread_mutex_unlock(&data->mutex);
				close(data->client_fd);
				endwin();
				return 1;
			}
			pthread_mutex_unlock(&data->mutex);
		}
		usleep(50000);
	}

	pthread_join(input_th, NULL);
	pthread_join(render_th, NULL);

	pthread_mutex_destroy(&data->mutex);
	endwin();
	close(data->client_fd);
	return 0;

}


int main(int argc, char** argv) {
	int menuChoice = -1;
	int regimeChoice = -1;
	int worldChoice = -1;
	int multiplayerChoice = -1;
	int gameTime = 0;
	int x = 0;
	int y = 0;
	int port = 0;
	data_t data;
	data.isPaused = 0;

	printf("****Hlavné menu****\n");
	printf("[1] Nová hra\n");
	printf("[2] Pripojenie k hre\n");
	printf("[0] Ukončiť\n");
	printf("Vyber si jednu z možností (zadaj číselnú hodnotu)\n");
	
	scanf("%d", &menuChoice);

	if (menuChoice == 0) return 0;

	if (menuChoice == 1) {
		printf("Vyber počet hráčov\n");
		printf("[1] Jeden hráč\n");
		printf("[2] Viac hráčov\n");

		scanf("%d", &multiplayerChoice);

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
		scanf("%d", &y);
	
		printf("Zadaj šírku herného sveta\n");
		scanf("%d", &x);

		printf("Zadaj port\n");
		scanf("%d", &port);

		char portStr[6];
		char multiplayerStr[2];
		char regimeStr[2];
		char timeStr[4];
		char worldStr[2];
		char xStr[5];
		char yStr[5];

		snprintf(portStr, sizeof(portStr), "%d", port);
		snprintf(multiplayerStr, sizeof(multiplayerStr), "%d", multiplayerChoice);
		snprintf(regimeStr, sizeof(regimeStr), "%d", regimeChoice);
		snprintf(timeStr, sizeof(timeStr), "%d", gameTime);
		snprintf(worldStr, sizeof(worldStr), "%d", worldChoice);
		snprintf(xStr, sizeof(xStr), "%d", x);
		snprintf(yStr, sizeof(yStr), "%d", y);

		pid_t pid = fork();
		if (pid < 0) {
			perror("Fork zlyhal\n");
			exit(1);
		}

		if (pid == 0) {
			execl("./server", "./server", portStr, multiplayerStr, regimeStr, timeStr, worldStr, xStr, yStr, NULL);
		}
		else {
			sleep(1);
			return connected(port, &data);
		}


	}

	if (menuChoice == 2) {
		printf("Zadaj port\n");
		scanf("%d", &port);
		return connected(port, &data);
	}
	
	return 0;
}
