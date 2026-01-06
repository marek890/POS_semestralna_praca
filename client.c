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

typedef struct {
	int client_fd;
	pthread_mutex_t mutex;
} data_t;

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
			case 'q':
				pthread_mutex_lock(&data->mutex);
				shutdown(data->client_fd, SHUT_RDWR);
				close(data->client_fd);
				pthread_mutex_unlock(&data->mutex);
				return NULL;
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
		int posX = game.width + 3;
		
		int r = recv(data->client_fd, &game, sizeof(game_t), 0);
		if (r <= 0) {
			pthread_mutex_unlock(&data->mutex);
			break;
		}

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
			mvaddch(game.obstacles[i].pos.y + 1, game.obstacles[i].pos.x + 1, '@');
		} 
		
		if (game.isTimed) {
			int remaining = game.maxGameTime - game.elapsedTime;
			if (remaining < 0) remaining = 0;

			mvprintw(1, posX, "ZOSTAVA:");
			mvprintw(2, posX, "%02d:%02d", remaining / 60, remaining % 60);
		}
		else {
			mvprintw(1, posX, "UPLYNULO:");
			mvprintw(2, posX, "%02d:%02d", game.elapsedTime / 60, game.elapsedTime % 60);
		}

		mvprintw(4, posX, "SKORE");

		for (int i = 0; i < game.playerCount; i++) {
			snake_t* snake = &game.snakes[i];
			int score = snake->length - 1;
	
			attron(COLOR_PAIR(snake->color));
			mvprintw(5 + i, posX, "Hrac %d: %d", snake->playerID, score);
			attroff(COLOR_PAIR(snake->color));

			for (int j = 0; j < snake->length; j++) {
				if (!snake->alive) continue;
				if (snake->body[j].x >= 0 && snake->body[j].x < game.width &&
					snake->body[j].y >= 0 && snake->body[j].y < game.length) {
						attron(COLOR_PAIR(snake->color));
						mvaddch(snake->body[j].y + 1, snake->body[j].x + 1, j == 0 ? 'O' : 'o');
						attroff(COLOR_PAIR(snake->color));
				}
			}
		}

		for (int i = 0; i < game.playerCount; i++) {
			fruit_t* fruit = &game.fruits[i];
			mvaddch(fruit->pos.y + 1, fruit->pos.x + 1, 'F');
		}

		mvprintw(15, posX, "Pre odpojenie stlac Q");

		refresh();
		
		pthread_mutex_unlock(&data->mutex);

	}
	endwin();
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
	
	initscr();
	curs_set(0);
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	start_color();
	use_default_colors();
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_BLUE, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);
	init_pair(5, COLOR_CYAN, COLOR_BLACK);

	pthread_mutex_init(&data->mutex, NULL);
	pthread_t input_th, render_th;
	pthread_create(&input_th, NULL, client_input, data);
	pthread_create(&render_th, NULL, client_render, data);

	pthread_join(input_th, NULL);
	pthread_join(render_th, NULL);

	pthread_mutex_destroy(&data->mutex);
	endwin();
	close(data->client_fd);
	
	return -1;
}

int show_main_menu() {
	int menuChoice = -1;
	int regimeChoice = -1;
	int worldChoice = -1;
	int multiplayerChoice = -1;
	int gameTime = 0;
	int x = 0;
	int y = 0;
	int port = 0;
	data_t data;
	
	do {
		printf("****Hlavné menu****\n");
		printf("[1] Nová hra\n");
		printf("[2] Pripojenie k hre\n");
		printf("[0] Ukončiť\n");
		printf("Vyber si jednu z možností (zadaj číselnú hodnotu)\n");
	
		scanf("%d", &menuChoice);
	} while (menuChoice < 0 || menuChoice > 2);

	if (menuChoice == 0) return 0;

	if (menuChoice == 1) {
		do {
			printf("Vyber počet hráčov\n");
			printf("[1] Jeden hráč\n");
			printf("[2] Viac hráčov\n");

			scanf("%d", &multiplayerChoice);
		} while (multiplayerChoice < 1 || multiplayerChoice > 2);

		do {
			printf("Vyber herný režim\n");
			printf("[1] Štandardný\n");
			printf("[2] Časový\n");

			scanf("%d", &regimeChoice);
		} while (regimeChoice < 1 || regimeChoice > 2);

		if (regimeChoice == 2) {
			do {
				printf("Zadaj čas hry (20 - 1000) \n");
				scanf("%d", &gameTime);
			} while (gameTime < 20 || gameTime > 1000);
		}
		
		do {
			printf("Vyber typ herného sveta\n");
			printf("[1] Bez prekážok\n");
			printf("[2] S prekážkami\n");

			scanf("%d", &worldChoice);
		} while (worldChoice < 1 || worldChoice > 2);

		do {
			printf("Zadaj výšku herného sveta (20 - 40)\n");
			scanf("%d", &y);
		} while (y < 20 || y > 40);

		do {
			printf("Zadaj šírku herného sveta (40 - 80)\n");
			scanf("%d", &x);
		} while (x < 40 || x > 80);
		
		do {
			printf("Zadaj port (1024 - 49151) \n");
			scanf("%d", &port);
		} while (port < 1024 || port > 49151);

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

int main(int argc, char** argv) {

	int quit = -1;
	while (quit != 0)
	{
		quit = show_main_menu();
	}
	return 0;
}
