#include "game.h"




void init_game(int width, int length) {
	game_t game;
	game.length = length;
	game.width = width;
	game.playerCount = 1;
	game.snakes[0].pos.x = width/2;
	game.snakes[0].pos.y = length/2;
	game.fruits[0].pos.x = rand() % width;
	game.fruits[0].pos.y = rand() % length;
}

