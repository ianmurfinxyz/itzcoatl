#include <cassert>
#include "play_state.h"

void PlayState::onEnter()
{
  _snakeLength = Snake::babySnakeLength;
}

void PlayState::onUpdate(double now, float dt)
{

}

Snake::Direction PlayState::findNeighbourDirection(const SnakeBlock& self, const SnakeBlock& neighbour)
{
  int dr = self._row - neighbour._row; 
  int dc = self._col - neighbour._col; 
  assert(dr ^ dc);
  assert(dr == 1 || dr == -1 || dr == 0);
  assert(dc == 1 || dc == -1 || dc == 0);
  if(dr > 0) return SOUTH;
  if(dr < 0) return NORTH;
  if(dc > 0) return WEST;
  if(dc < 0) return EAST;
  assert(0);
}

void PlayState::updateBlockSpriteIDs()
{
  Direction headDir, tailDir;

  tailDir = findNeighbourDirection(_snake[SNAKE_HEAD_BLOCK], _snake[SNAKE_HEAD_BLOCK + 1]);
  _snake[SNAKE_HEAD_BLOCK]._sid = Snake::snakeHeadBlockTree[tailDir];

  for(int block {SNAKE_HEAD_BLOCK + 1}; block < _snakeLength - 1; ++block){
    headDir = findNeighbourDirection(_snake[block], _snake[block - 1]);
    tailDir = findNeighbourDirection(_snake[block], _snake[block + 1]);
    _snake[block]._sid = Snake::snakeBodyBlockTree[headDir][tailDir];
  }

  headDir = findNeighbourDirection(_snake[_snakeSize - 1], _snake[_snakeSize - 2]);
  _snake[_snakeSize - 1]._sid = Snake::snakeTailBlockTree[tailDir];
}
