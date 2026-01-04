#include "game.h"

void init_game(game_t* game, int width, int length) {
	game.length = length;
	game.width = width;
	game.playerCount = 1;

	game.snakes[0].pos.x = width / 2;
	game.snakes[0].pos.y = length / 2;
	game.snakes[0].length = 1;
}

void update_game(game_t* game) {
	for (int i = 0; i < game->playerCount; ++) {
		snake_t* snake = &game->snakes[i];
		if (!snake->alive) continue;

		move_snake(snake);

		for (int j = 0; i < game->playerCount; j++) {
			fruit_t* fruit = &game->fruits[j];
			if (!fruit->active) continue;

			if (snake->body[0].x = fruit->pos.x &&
				snake->body[0].y = fruit->pos.y) {
				
				snake->length++;
				fruit->pos.x = rand() % game->width;
				fruit->pos.y = rand() % game->length;
			}
		}
	}
}

void move_snake(snake_t* snake) {
	for (int i = snake->length - 1; i > 0; i--)
		snake->body[i] = snake->body[i - 1];

	switch (snake->dir) {
		case UP:	snake->body[0].y--; break;
		case DOWN:	snake->body[0].y++; break;
		case LEFT:	snake->body[0].x--; break;
		case RIGHT:	snake->body[0].x++; break;
	}
}
