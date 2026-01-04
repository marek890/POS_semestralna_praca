#include <stdlib.h>
#include <stdio.h>

#define MAX_PLAYERS 5

typedef enum {
	UP, DOWN, LEFT, RIGHT
} direction_t;

typedef struct {
	int x;
	int y;
} position_t;

typedef struct {
	position_t body[100]; 
	direction_t dir;
	int length;
	_Bool alive;
} snake_t;

typedef struct {
	position_t pos;
	_Bool active;
} fruit_t;

typedef struct {
	snake_t snakes[MAX_PLAYERS];
	fruit_t fruits[MAX_PLAYERS];
	int playerCount;
	int width, length;
} game_t;

void init_game(game_t* game, int width, int length);
void update_game(game_t* game);
void move_snake(snake_t* snake);

