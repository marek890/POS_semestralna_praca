#include "game.h"

void init_game(game_t* game, _Bool hasObstacles, int width, int length) {
	game->length = length;
	game->width = width;
	game->playerCount = 0;
	game->hasObstacles = hasObstacles;
	game->obstacleCount = 0;
	game->isTimed = 0;
	game->elapsedTime = 0;
	game->maxGameTime = 0;
	game->startTime = 0;

	if (hasObstacles) {
		for (int i = 0; i < MAX_OBSTACLES; i++) {	
			int x, y;
			do {
				x = rand() % game->width;
				y = rand() % game->length;
			} while (is_position_occupied(game, x, y));

			game->obstacles[i].pos.x = x;
			game->obstacles[i].pos.y = y;
			game->obstacleCount++;
		}
	}
}

void update_game(game_t* game) {
	for (int i = 0; i < game->playerCount; i++) {
		snake_t* snake = &game->snakes[i];
		if (!snake->alive) continue;

		move_snake(snake);
		
		if (snake->body[0].x < 0)
			snake->body[0].x = game->width - 1;
		else if (snake->body[0].x >= game->width)
			snake->body[0].x = 0;

		if (snake->body[0].y < 0)
			snake->body[0].y = game->length - 1;
		else if (snake->body[0].y >= game->length)
			snake->body[0].y = 0;

		if (check_collision(game, i)) {
			snake->alive = 0;
			continue;
		}

		for (int j = 0; j < game->playerCount; j++) {
			fruit_t* fruit = &game->fruits[j];

			if (snake->body[0].x == fruit->pos.x &&
				snake->body[0].y == fruit->pos.y) {
				
				if (snake->length < MAX_SNAKE_LENGTH)
					snake->length++;

				int x, y;
				do {
					x =  rand() % game->width;
					y = rand() % game->length;
				} while(is_position_occupied(game, x, y));
				
				fruit->pos.x = x;
				fruit->pos.y = y;
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

int add_snake(game_t* game) {
	if (game->playerCount >= MAX_PLAYERS)
		return -1;

	int index = game->playerCount;
	snake_t* snake = &game->snakes[index];

	snake->length = 1;
	snake->dir = RIGHT;
	snake->alive = 1;

	int x, y; 
	do {
		x = rand() % game->width;
		y = rand() % game->length;
	} while (is_position_occupied(game, x, y));

	snake->body[0].x = x;
	snake->body[0].y = y;

	do {
		x = rand() % game->width;
		y = rand() % game->length;
	} while (is_position_occupied(game, x, y));

	game->fruits[index].pos.x = x;
	game->fruits[index].pos.y = y;

	game->playerCount++;

	return index;
}

void set_direction(snake_t* snake, direction_t dir) {
	if (
		(snake->dir == UP && dir == DOWN) ||
		(snake->dir == DOWN && dir == UP) ||
		(snake->dir == LEFT && dir == RIGHT) ||
		(snake->dir == RIGHT && dir == LEFT)
	) {
		return;
	}

	snake->dir = dir;
}

_Bool is_position_occupied(game_t* game, int x, int y) {
	for (int i = 0; i < game->obstacleCount; i++) {
		if (game->obstacles[i].pos.x == x && game->obstacles[i].pos.y == y)
			return 1;
	}
	for (int i = 0; i < game->playerCount; i++) {
		if (game->fruits[i].pos.x == x && game->fruits[i].pos.y == y)
			return 1;
	}
	for (int i = 0; i < game->playerCount; i++) {
		snake_t* snake = &game->snakes[i];
		for (int j = 0; j < snake->length; j++)
			if (snake->body[j].x == x && snake->body[j].y == y)
				return 1;
	}
	return 0;
}

int check_collision(game_t* game, int index) {
	snake_t* snake = &game->snakes[index];
	int x = snake->body[0].x;
	int y = snake->body[0].y;

	for (int i = 0; i < game->obstacleCount; i++) {
		if (game->obstacles[i].pos.x == x && game->obstacles[i].pos.y == y)
			return 1;
	}
	for (int i = 1; i < snake->length; i++) {
		if (snake->body[i].x == x && snake->body[i].y == y)
			return 1;
	}
	for (int i = 0; i < game->playerCount; i++) {
		if (i == index) continue;
		snake_t* snakeOther = &game->snakes[i];
		if (!snakeOther->alive) continue;
		
		for (int j = 0; j < snakeOther->length; j++)
			if (snakeOther->body[j].x == x && snakeOther->body[j].y == y)
				return 1;
	}
	return 0;
}

