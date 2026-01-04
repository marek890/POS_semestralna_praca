#include "game.h"

void init_game(game_t* game, int width, int length) {
	game.length = length;
	game.width = width;
	game.playerCount = 1;

	game.snakes[0].pos.x = width / 2;
	game.snakes[0].pos.y = length / 2;
	game.snakes[0].length = 1;
}

