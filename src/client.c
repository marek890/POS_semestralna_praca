#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include "game.h"

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"

typedef struct {
	int client_fd;
	_Bool isRunning;
	pthread_mutex_t mutex;
} data_t;

void* client_input(void* arg) {	
	data_t* data = (data_t*)arg;

	while (data->isRunning) {
		char ch;
		if (read(STDIN_FILENO, &ch, 1) <= 0) {
			usleep(5000);
			continue;
		}

		char out;
		switch (ch) {
			case 'q':
				pthread_mutex_lock(&data->mutex);
				data->isRunning = 0;
				shutdown(data->client_fd, SHUT_RDWR);
				pthread_mutex_unlock(&data->mutex);
				return NULL;
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
		if (data->isRunning)
			send(data->client_fd, &out, 1, 0);
		pthread_mutex_unlock(&data->mutex);
	}

	return NULL;
}

void* client_render(void* arg) {
	data_t* data = (data_t*)arg;
	game_t game;
	memset(&game, 0, sizeof(game));

	const char* colors[] = {
			COLOR_RED,
			COLOR_GREEN,
			COLOR_YELLOW,
			COLOR_BLUE,
			COLOR_MAGENTA
		};

	while (1) {
		usleep(50000);

		int r = recv(data->client_fd, &game, sizeof(game_t), 0);

		pthread_mutex_lock(&data->mutex);
		
		if (r <= 0) {
			data->isRunning = 0;
			pthread_mutex_unlock(&data->mutex);
			printf("\033[H\033[J");
			break;
		}

		printf("\033[H\033[J");

		for (int i = 0; i < game.width + 2; i++)
			printf("#");
		printf("\n");

		for (int y = 0; y < game.length; y++) {
			printf("#");
			for (int x = 0; x < game.width; x++) {
				char printed = ' ';
				const char* color = COLOR_RESET;

				for (int i = 0; i < game.playerCount; i++) {
					if (game.fruits[i].pos.x == x && game.fruits[i].pos.y == y) {
						printed = 'F';
						color = COLOR_RESET;
					}
				}

				for (int i = 0; i < game.playerCount; i++) {
					snake_t* snake = &game.snakes[i];
					for (int b = 0; b < snake->length; b++) {
						if (!snake->alive) continue;
						if (snake->body[b].x == x && snake->body[b].y == y) {
							printed = (b == 0) ? 'O': 'o';
							color = colors[snake->color];
						}
					}
				}

				for (int i = 0; i < game.obstacleCount; i++) {
					if (game.obstacles[i].pos.x == x && game.obstacles[i].pos.y == y) {
						printed = '@';
						color = COLOR_RESET;
					}
				}

				printf("%s%c%s", color, printed, COLOR_RESET);
			}
			printf("#\n");
		}

		for (int i = 0; i < game.width + 2; i++)
			printf("#");
		printf("\n");

		printf("Pre odpojenie stlac Q\n");

		if (game.isTimed) {
			int remaining = game.maxGameTime - game.elapsedTime;
			if (remaining < 0) remaining = 0;
			printf("ZOSTAVA: %02d:%02d\n", remaining / 60, remaining % 60);
		} else {
			printf("UPLYNULO: %02d:%02d\n", game.elapsedTime / 60, game.elapsedTime % 60);
		}

		printf("SKORE:\n");
		for (int i = 0; i < game.playerCount; i++) {
			snake_t* snake = &game.snakes[i];
    		int score = snake->length - 1;
			const char* color = colors[snake->color];

			if (snake->alive) {
				printf("%sHrac %d: %d%s\n", color, i + 1, score, COLOR_RESET);
			} else {
				printf("%sHrac %d: %d (DEAD)%s\n", color, i + 1, score, COLOR_RESET);
			}
		}
		
		fflush(stdout);
		
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

	data->isRunning = 1;

	struct termios original;
    
    tcgetattr(STDIN_FILENO, &original);
    struct termios raw = original;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

	pthread_mutex_init(&data->mutex, NULL);
	pthread_t input_th, render_th;
	pthread_create(&input_th, NULL, client_input, data);
	pthread_create(&render_th, NULL, client_render, data);

	pthread_join(input_th, NULL);
	pthread_join(render_th, NULL);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);

	pthread_mutex_destroy(&data->mutex);
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
	memset(&data, 0, sizeof(data));
	
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
			_exit(1);
		}

		if (pid == 0) {
			execl("./server", "./server", portStr, multiplayerStr, regimeStr, timeStr, worldStr, xStr, yStr, NULL);
			_exit(1);
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

int main(void) {

	int quit = -1;
	while (quit != 0)
	{
		quit = show_main_menu();
	}
	return 0;
}
