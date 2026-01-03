#include <stdlib.h>
#include <stdio.h>

#define MAX_PLAYERS 5

typedef struct {
	int x;
	int y;
} position_t;

typedef struct {
	position_t pos; 
	int length;
	int score;
} snake_t;

typedef struct {
	position_t pos;
} fruit_t;

typedef struct {
	snake_t snakes[MAX_PLAYERS];
	fruit_t fruits[MAX_PLAYERS];
	int playerCount;
	int width, length;
} game_t;

void init_game(int width, int length);

