#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int connected(int port) {
	char buffer[BUFFER_SIZE];
	int client_fd;
	client_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (client_fd < 0) {
		perror("Vytvorenie socketu zlyhalo\n");
		return 1;
	}
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));		
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, "127.0.0.1", (struct sockaddr*)&server_addr.sin_addr) < 0) {
		perror("Adresa je neplatna\n");
		return 2;
	}

	if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Pripojenie k serveru zlyhalo\n");
		return 3;
	}

	while (1) {
		
		send(client_fd, buffer, strlen(buffer), 0);

		if (strncmp(buffer, "q", 1) == 0) {
			printf("Klient ukončuje spojenie\n");
			break;
		}

		memset(buffer, 0, BUFFER_SIZE);
		
	}
	close(client_fd);
	return 0;

}


int main(int argc, char** argv) {
	int menuChoice = -1;
	int regimeChoice = -1;
	int worldChoice = -1;
	int gameTime = 0;
	int x = 0;
	int y = 0;
	_Bool isPaused = 0;
	int port = atoi(argv[1]);

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
		scanf("%d", &y);
	
		printf("Zadaj šírku herného sveta\n");
		scanf("%d", &x);
	}

	if (menuChoice == 2) {
		connected(port);
	}
	
	return 0;
}
