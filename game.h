#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define MAX_PLAYERS 5
#define MAX_SNAKE_LENGTH 100
#define MAX_OBSTACLES 50

typedef enum {
	UP, DOWN, LEFT, RIGHT
} direction_t;

typedef struct {
	int x;
	int y;
} position_t;

typedef struct {
	position_t body[MAX_SNAKE_LENGTH]; 
	direction_t dir;
	int length;
	_Bool alive;
} snake_t;

typedef struct {
	position_t pos;
} fruit_t;

typedef struct {
	position_t pos; 
} obstacle_t;

typedef struct {
	snake_t snakes[MAX_PLAYERS];
	fruit_t fruits[MAX_PLAYERS];
	obstacle_t obstacles[MAX_OBSTACLES];
	int obstacleCount;
	int playerCount;
	int width, length;
	_Bool hasObstacles;
	_Bool isTimed
	time_t startTime;
	int maxGameTime;
	int elapsedTime;
} game_t;

void init_game(game_t* game, _Bool hasObstacles, int width, int length);
void update_game(game_t* game);
void move_snake(snake_t* snake);
int add_snake(game_t* game);
void set_direction(snake_t* snake, direction_t dir);
int check_collision(game_t* game, int index);
_Bool is_position_occupied(game_t* game, int x, int y);

