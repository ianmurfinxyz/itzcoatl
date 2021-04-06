#include <cassert>
#include "play_scene.h"

PlayScene::PlayScene(pxr::Game* owner) :
  Scene(owner),
  _snake{},
  _snakeLength{0},
  _moveDirection{Snake::WEST},
  _stepClock_s{0.f}
{}

bool PlayScene::onInit()
{
  _sk = static_cast<Snake*>(_owner);
  return true;
}

void PlayScene::onEnter()
{
  _stepClock_s = 0.f;
  _moveDirection = Snake::WEST;
  initializeSnake();
}

void PlayScene::onUpdate(double now, float dt)
{
  _stepClock_s += dt;
  if(_stepClock_s > Snake::stepPeriod_s){
    stepSnake();
    _stepClock_s = 0.f;
  }
}

void PlayScene::onDraw(double now, float dt, int screenid)
{
  gfx::clearScreenTransparent(screenid);
  drawSnake(screenid);
}

void PlayScene::onExit()
{
}

void PlayScene::initializeSnake()
{
  _snakeLength = Snake::babySnakeLength;
  for(int block {SNAKE_HEAD_BLOCK}; block < Snake::babySnakeLength; ++block){
    _snake[block]._col = Snake::snakeHeadSpawnCol + block;
    _snake[block]._row = Snake::snakeHeadSpawnRow;
  }
  updateSnakeBlockSpriteIDs();
}

void PlayScene::stepSnake()
{
  for(int block {_snakeLength - 1}; block > SNAKE_HEAD_BLOCK; --block){
    _snake[block]._row = _snake[block - 1]._row;
    _snake[block]._col = _snake[block - 1]._col;
  }
  switch(_moveDirection){
    case Snake::NORTH:
      _snake[SNAKE_HEAD_BLOCK]._row++;
      if(_snake[SNAKE_HEAD_BLOCK]._row >= Snake::boardSize._y)
        _snake[SNAKE_HEAD_BLOCK]._row = 0;
      break;
    case Snake::SOUTH:
      _snake[SNAKE_HEAD_BLOCK]._row--;
      if(_snake[SNAKE_HEAD_BLOCK]._row < 0)
        _snake[SNAKE_HEAD_BLOCK]._row = Snake::boardSize._y - 1;
      break;
    case Snake::EAST:
      _snake[SNAKE_HEAD_BLOCK]._col++;
      if(_snake[SNAKE_HEAD_BLOCK]._col >= Snake::boardSize._x)
        _snake[SNAKE_HEAD_BLOCK]._col = 0;
      break;
    case Snake::WEST:
      _snake[SNAKE_HEAD_BLOCK]._col--;
      if(_snake[SNAKE_HEAD_BLOCK]._col < 0)
        _snake[SNAKE_HEAD_BLOCK]._col = Snake::boardSize._x;
      break;
    default:
      assert(0);
  }
}

Snake::Direction PlayScene::findNeighbourDirection(const SnakeBlock& self, const SnakeBlock& neighbour)
{
  int dr = self._row - neighbour._row; 
  int dc = self._col - neighbour._col; 
  assert(dr ^ dc);
  assert(dr == 1 || dr == -1 || dr == 0);
  assert(dc == 1 || dc == -1 || dc == 0);
  if(dr > 0) return Snake::SOUTH;
  if(dr < 0) return Snake::NORTH;
  if(dc > 0) return Snake::WEST;
  if(dc < 0) return Snake::EAST;
  assert(0);
}

void PlayScene::updateSnakeBlockSpriteIDs()
{
  Snake::Direction headDir, tailDir;

  tailDir = findNeighbourDirection(_snake[SNAKE_HEAD_BLOCK], _snake[SNAKE_HEAD_BLOCK + 1]);
  _snake[SNAKE_HEAD_BLOCK]._spriteid = Snake::snakeHeadBlockTree[tailDir];

  for(int block {SNAKE_HEAD_BLOCK + 1}; block < _snakeLength - 1; ++block){
    headDir = findNeighbourDirection(_snake[block], _snake[block - 1]);
    tailDir = findNeighbourDirection(_snake[block], _snake[block + 1]);
    _snake[block]._spriteid = Snake::snakeBodyBlockTree[headDir][tailDir];
  }

  headDir = findNeighbourDirection(_snake[_snakeLength - 1], _snake[_snakeLength - 2]);
  _snake[_snakeLength - 1]._spriteid = Snake::snakeTailBlockTree[headDir];
}

void PlayScene::drawSnake(int screenid)
{
  for(int block {SNAKE_HEAD_BLOCK}; block < _snakeLength; ++block){
    Vector2i position {
      _snake[block]._col * Snake::blockSize_rx,
      _snake[block]._row * Snake::blockSize_rx
    };
    gfx::drawSprite(
      position,
      _sk->getSpritesheetKey(Snake::SSID_SNAKES),
      _snake[block]._spriteid + (_sk->getSnakeHero() * Snake::SID_COUNT),
      screenid
    );
  }
}
