#include <cassert>
#include "play_scene.h"
#include "pxr_mathutil.h"
#include "pxr_rand.h"

#include <iostream>


using namespace pxr;

PlayScene::PlayScene(pxr::Game* owner) :
  Scene(owner),
  _snake{},
  _snakeLength{0},
  _nextMoveDirection{Snake::WEST},
  _currentMoveDirection{Snake::WEST},
  _stepClock_s{0.f},
  _isSnakeSmoothMover{true}
{}

bool PlayScene::onInit()
{
  _sk = static_cast<Snake*>(_owner);
  return true;
}

void PlayScene::onEnter()
{
  _stepClock_s = 0.f;
  _nextMoveDirection = Snake::WEST;
  _currentMoveDirection = Snake::WEST;
  initializeSnake();
  initializeNuggets();
  drawBackground();
  drawForeground();
}

void PlayScene::onUpdate(double now, float dt)
{
  handleInput();

  _stepClock_s += dt;
  if(_stepClock_s > Snake::stepPeriod_s){
    if(collideSnakeNuggets()) growSnake();
    stepSnake();
    _stepClock_s = 0.f;
  }

  while(_numNuggetsInWorld < Snake::maxNuggetsInWorld)
    spawnNugget();
}

void PlayScene::onDraw(double now, float dt, const std::vector<gfx::ScreenID_t>& screens)
{
  gfx::clearScreenTransparent(screens[Snake::SCREEN_STAGE]);

  drawNuggets(screens[Snake::SCREEN_STAGE]);

  if(_isSnakeSmoothMover)
    drawSmoothSnake(screens[Snake::SCREEN_STAGE]);
  else
    drawSnake(screens[Snake::SCREEN_STAGE]);
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

void PlayScene::initializeNuggets()
{
  _numNuggetsInWorld = 0;
  for(auto& nugget : _nuggets)
    nugget._isAlive = false;
}

void PlayScene::stepSnake()
{
  for(int block {_snakeLength - 1}; block > SNAKE_HEAD_BLOCK; --block){
    _snake[block]._row = _snake[block - 1]._row;
    _snake[block]._col = _snake[block - 1]._col;
  }
  switch(_currentMoveDirection){
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
        _snake[SNAKE_HEAD_BLOCK]._col = Snake::boardSize._x - 1;
      break;
    default:
      assert(0);
  }

  _currentMoveDirection = _nextMoveDirection;

  updateSnakeBlockSpriteIDs();
}

void PlayScene::growSnake()
{
  _snakeLength = std::min(_snakeLength + 1, Snake::maxSnakeLength - 1);
}

void PlayScene::spawnNugget()
{
  assert(_numNuggetsInWorld < Snake::maxNuggetsInWorld);

  int choice = rand::uniformSignedInt(
      0, Snake::nuggetClasses[Snake::nuggetClassCount - 1]._spawnChance
  );

  Snake::NuggetClassID newNuggetClassID;
  for(int i {Snake::NUGGET_GOLD}; i < Snake::nuggetClassCount; ++i){
    if(choice <= Snake::nuggetClasses[i]._spawnChance){
      newNuggetClassID = static_cast<Snake::NuggetClassID>(i);
      break;
    }
  }

  int row, col;
  bool isRowValid {false}, isColValid {false};
  while(!isRowValid || !isColValid){
    row = rand::uniformSignedInt(0, Snake::boardSize._y - 1);
    col = rand::uniformSignedInt(0, Snake::boardSize._x - 1);

    //
    // do not spawn in front of the snake; the player must always have to take action to get
    // a nugget.
    //
    isRowValid = !(
      row == _snake[SNAKE_HEAD_BLOCK]._row && 
      (_currentMoveDirection == Snake::EAST || 
      _currentMoveDirection == Snake::WEST)
     );
    isColValid = !(
      col == _snake[SNAKE_HEAD_BLOCK]._col  && 
      (_currentMoveDirection == Snake::NORTH || 
      _currentMoveDirection == Snake::SOUTH)
     );
    if(!isRowValid || !isColValid) continue;

    //
    // nuggets cannot overlap.
    //
    for(const auto& nugget : _nuggets){
      if(!nugget._isAlive) continue;
      if(row == nugget._row && col == nugget._col){
        isRowValid = isColValid = false;
        break;
      }
    }
    if(!isRowValid || !isColValid) continue;

    //
    // nuggets cannot spawn on the snake.
    //
    for(int block {SNAKE_HEAD_BLOCK}; block < _snakeLength; ++block){
      if(row == _snake[block]._row && col == _snake[block]._col){
        isRowValid = isColValid = false;
        break;
      }
    }
    if(!isRowValid || !isColValid) continue;
  }

  bool spawnDone {false};
  for(int i{0}; i < Snake::maxNuggetsInWorld; ++i){
    if(_nuggets[i]._isAlive) continue;
    _nuggets[i]._classID = newNuggetClassID;
    _nuggets[i]._row = row;
    _nuggets[i]._col = col;
    _nuggets[i]._flashClock = 0.f;
    _nuggets[i]._isVisible = true;
    _nuggets[i]._isAlive = true;
    ++_numNuggetsInWorld;
    spawnDone = true;
    break;
  }
  assert(spawnDone);
}

void PlayScene::handleInput()
{
  bool lkey{false}, rkey{false}, ukey{false}, dkey{false};
  if(pxr::input::isKeyPressed(Snake::moveLeftKey)) lkey = true;
  if(pxr::input::isKeyPressed(Snake::moveRightKey)) rkey = true;
  if(pxr::input::isKeyPressed(Snake::moveUpKey)) ukey = true;
  if(pxr::input::isKeyPressed(Snake::moveDownKey)) dkey = true;
  
  int sum {lkey + rkey + ukey + dkey};
  if(sum > 1) return;

  if(lkey && _currentMoveDirection != Snake::EAST) _nextMoveDirection = Snake::WEST;
  if(rkey && _currentMoveDirection != Snake::WEST) _nextMoveDirection = Snake::EAST;
  if(ukey && _currentMoveDirection != Snake::SOUTH) _nextMoveDirection = Snake::NORTH;
  if(dkey && _currentMoveDirection != Snake::NORTH) _nextMoveDirection = Snake::SOUTH;

  if(pxr::input::isKeyPressed(Snake::smoothToggle)) _isSnakeSmoothMover = !_isSnakeSmoothMover;

  if(pxr::input::isKeyPressed(input::KEY_a)) 
    sfx::stopChannel(sfx::ALL_CHANNELS);
}

Snake::Direction PlayScene::findNeighbourDirection(const SnakeBlock& self, const SnakeBlock& neighbour)
{
  int dr = self._row - neighbour._row; 
  int dc = self._col - neighbour._col; 
  assert(dr ^ dc);

  // handle wrapping; the only way the abs(dr) and abs(dc) can be greater than 1 is if we have 
  // wrapped around the map. We have to handle this to ensure the correct sprite is selected 
  // since sprite selection is based on relative position of the neighbours.
  //
  // So for example, if dr > 1, it must mean that the neighbour is more than 1 tile away from
  // the self tile, which can only happen if the neighbour has jumped to the other side of the
  // world. Further if dr > 1 then the neighbour is far south of the self tile, meaning the wrap
  // was from the top of the world to the bottom. Equivilent conditions can be reasoned for the
  // other wrap directions too.
  //
  // The solution is to pretend the neighbour is actually 1 tile away in the opposite direction
  // to what it actually is. So if the neighbour is far south of self, we pretend it is one
  // tile to the north.

  if(dr > 1) dr = -1;
  if(dr < -1) dr = 1;
  if(dc > 1) dc = -1;
  if(dc < -1) dc = 1;

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
  _snake[SNAKE_HEAD_BLOCK]._currentMoveDirection = _currentMoveDirection;

  for(int block {SNAKE_HEAD_BLOCK + 1}; block < _snakeLength - 1; ++block){
    headDir = findNeighbourDirection(_snake[block], _snake[block - 1]);
    tailDir = findNeighbourDirection(_snake[block], _snake[block + 1]);
    _snake[block]._spriteid = Snake::snakeBodyBlockTree[headDir][tailDir];
    _snake[block]._currentMoveDirection = headDir;
  }

  headDir = findNeighbourDirection(_snake[_snakeLength - 1], _snake[_snakeLength - 2]);
  _snake[_snakeLength - 1]._spriteid = Snake::snakeTailBlockTree[headDir];
  _snake[_snakeLength - 1]._currentMoveDirection = headDir;
}

bool PlayScene::collideSnakeNuggets()
{
  for(auto& nugget : _nuggets){
    if(!nugget._isAlive) continue;
    bool collision = _snake[SNAKE_HEAD_BLOCK]._row == nugget._row &&
                     _snake[SNAKE_HEAD_BLOCK]._col == nugget._col;
    if(collision){ 
      eatNugget(nugget);
      return true;
    }
  }
  return false;
}

void PlayScene::eatNugget(Nugget& nugget)
{
  const auto& nc = Snake::nuggetClasses[nugget._classID];
  _sk->addScore(nc._score);
  nugget._isAlive = false;
  --_numNuggetsInWorld;
  sfx::SoundChannel_t channel = sfx::playSound(_sk->getSoundEffectKey(Snake::SFX_SCORE_BEEP));
}

void PlayScene::drawBackground()
{
  gfx::drawSprite(
    Vector2f{0.f, 0.f},
    _sk->getSpritesheetKey(Snake::SSID_BACKGROUND),
    0,
    _sk->getScreenID(Snake::SCREEN_BACKGROUND)
  );
}

void PlayScene::drawForeground()
{
  gfx::drawSprite(
    Vector2f{0.f, 0.f},
    _sk->getSpritesheetKey(Snake::SSID_FOREGROUND),
    0,
    _sk->getScreenID(Snake::SCREEN_FOREGROUND)
  );
}

void PlayScene::drawSnake(gfx::ScreenID_t screenid)
{
  for(int block {SNAKE_HEAD_BLOCK}; block < _snakeLength; ++block){
    Vector2i position {
      Snake::boardPosition._x + (_snake[block]._col * Snake::blockSize_rx),
      Snake::boardPosition._y + (_snake[block]._row * Snake::blockSize_rx)
    };
    gfx::drawSprite(
      position,
      _sk->getSpritesheetKey(Snake::SSID_SNAKES),
      _snake[block]._spriteid + (_sk->getSnakeHero() * Snake::SID_SNAKE_SHEET_COUNT),
      screenid
    );
  }
}

void PlayScene::drawSmoothSnake(gfx::ScreenID_t screenid)
{
  for(int block {_snakeLength - 1}; block >= SNAKE_HEAD_BLOCK; --block){
    Vector2i position {
      Snake::boardPosition._x + (_snake[block]._col * Snake::blockSize_rx),
      Snake::boardPosition._y + (_snake[block]._row * Snake::blockSize_rx)
    };

    gfx::SpriteID_t spriteid;
    if(block == SNAKE_HEAD_BLOCK)
      spriteid = Snake::smoothSnakeHeadBlockTree[_snake[block]._currentMoveDirection];
    else if(block == _snakeLength - 1)
      spriteid = Snake::snakeTailBlockTree[_snake[block]._currentMoveDirection];
    else
      spriteid = Snake::smoothSnakeBodyBlockTree[_snake[block]._currentMoveDirection];

    float t = _stepClock_s / Snake::stepPeriod_s;
    float limit = static_cast<float>(Snake::blockSize_rx) - 1.f;
    switch(_snake[block]._currentMoveDirection){
      case Snake::NORTH:
        position._y += lerp(0.f, limit, t);
        break;
      case Snake::SOUTH:
        position._y -= lerp(0.f, limit, t);
        break;
      case Snake::EAST:
        position._x += lerp(0.f, limit, t);
        break;
      case Snake::WEST:
        position._x -= lerp(0.f, limit, t);
        break;
      default:
        assert(0);
    }

    gfx::drawSprite(
      position,
      _sk->getSpritesheetKey(Snake::SSID_SNAKES),
      spriteid + (_sk->getSnakeHero() * Snake::SID_SNAKE_SHEET_COUNT),
      screenid
    );
  }
}

void PlayScene::drawNuggets(gfx::ScreenID_t screenid)
{
  for(const auto& nugget : _nuggets){
    if(!nugget._isAlive) continue;
    const auto& nc = Snake::nuggetClasses[nugget._classID];
    Vector2i position {
      Snake::boardPosition._x + (nugget._col * Snake::blockSize_rx),
      Snake::boardPosition._y + (nugget._row * Snake::blockSize_rx)
    };
    gfx::drawSprite(
      position,
      _sk->getSpritesheetKey(Snake::SSID_NUGGETS),
      nc._spriteid,
      screenid
    );
  }
}
