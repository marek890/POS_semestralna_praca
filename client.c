#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	int menuChoice = -1;
	int regimeChoice = -1;
	int worldChoice = -1;
	int gameTime = 0;
	int x = 0;
	int y = 0;
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
		scanf("%d", &y);
	
		printf("Zadaj šírku herného sveta\n");
		scanf("%d", &x);



	}

	if (menuChoice == 2) {

	}
	
	return 0;
}
