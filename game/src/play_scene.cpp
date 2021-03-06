#include <cassert>
#include <algorithm>
#include "../include/play_scene.h"
#include "pxr_mathutil.h"
#include "pxr_rand.h"
#include "pxr_gfx.h"

#include <iostream>
#include <menu_scene.h>

using namespace pxr;

PlayScene::PlayScene(pxr::Game* owner) :
  Scene(owner),
  _snake{},
  _snakeLength{0},
  _nextMoveDirection{Snake::WEST},
  _currentMoveDirection{Snake::WEST},
  _stepClock_s{0.f},
  _isSnakeSmoothMover{false}
{}

bool PlayScene::onInit()
{
  _sk = static_cast<Snake*>(_owner);
  _hud = _sk->getHUD();
  drawForeground();
  gfx::disableScreen(_sk->getScreenID(Snake::SCREEN_FOREGROUND));
  return true;
}

void PlayScene::onEnter()
{
  _currentState = State::NONE;
  drawBackground();
  gfx::enableScreen(_sk->getScreenID(Snake::SCREEN_FOREGROUND));
  populateHUD();
  setNextState(State::PLAYING);
  switchState();
}

void PlayScene::onUpdate(double now, float dt)
{
  switch(_currentState){
    case State::PLAYING:
      onUpdatePlaying(now, dt);
      break;
    case State::GAME_OVER:
      onUpdateGameOver(now, dt);
      break;
    default:
      assert(0);
  }

  updateSpeedBonusHUD();
  _hud->onUpdate(dt);

  if(_nextState != _currentState)
    switchState();
}

void PlayScene::onDraw(double now, float dt, const std::vector<gfx::ScreenID_t>& screens)
{
  gfx::clearScreenTransparent(screens[Snake::SCREEN_STAGE]);

  drawNuggets(screens[Snake::SCREEN_STAGE]);

  if(_isSnakeSmoothMover)
    drawSmoothSnake(screens[Snake::SCREEN_STAGE]);
  else
    drawSnake(screens[Snake::SCREEN_STAGE]);

  _hud->onDraw(screens[Snake::SCREEN_STAGE]);
}

void PlayScene::onExit()
{
  gfx::disableScreen(_sk->getScreenID(Snake::SCREEN_FOREGROUND));
  clearHUD();
  sfx::stopMusic();
}

void PlayScene::onEnterPlaying()
{
  _nextMoveDirection = Snake::WEST;
  _currentMoveDirection = Snake::WEST;
  _stepClock_s = 0.f;
  _speedClock_s = 0.f;
  _speedBonusTableIndex = 0;
  _currentSpeedBonusAsInt = 0;
  clearEatHistory();

  initializeSnake();
  initializeNuggets();
  sfx::playMusic(std::move(_sk->getMusicSequence(Snake::MUSIC_SEQUENCE_JUNGLE)));
}

void PlayScene::onUpdatePlaying(double now, float dt)
{
  handlePlayingInput();

  _speedClock_s -= dt;

  _stepClock_s += dt;
  if(_stepClock_s > Snake::stepPeriod_s){
    stepSnake();
    collideSnakeNuggets();
    collideSnakeSnake();
    _stepClock_s = 0.f;
  }

  while(_numNuggetsInWorld < Snake::maxNuggetsInWorld)
    spawnNugget();
}

void PlayScene::handlePlayingInput()
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

void PlayScene::onExitPlaying()
{
  sfx::stopMusic();
}

void PlayScene::onEnterGameOver()
{
  eatSnake();
  addGameOverToHUD();
  _gameOverClock_s = 0.f;
}

void PlayScene::onUpdateGameOver(double now, float dt)
{
  _gameOverClock_s += dt;
  if(_gameOverClock_s > Snake::gameOverPeriod_s){
    _sk->switchScene(MenuScene::name);
  }
}

void PlayScene::handleGameOverInput()
{
}

void PlayScene::onExitGameOver()
{
}

void PlayScene::setNextState(State nextState)
{
  _nextState = nextState;
}

void PlayScene::switchState()
{
  switch(_currentState){
    case State::PLAYING:
      onExitPlaying();
      break;
    case State::GAME_OVER:
      onExitGameOver();
      break;
    default:
      break;
  }
  switch(_nextState){
    case State::PLAYING:
      onEnterPlaying();
      break;
    case State::GAME_OVER:
      onEnterGameOver();
      break;
    default:
      assert(0);
  }
  _currentState = _nextState;
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

void PlayScene::populateHUD()
{
  _uidLabels[LID_SCORE] = _hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{24, 182},
    Snake::scorePopupColor,
    0.f,
    HUD::IMMORTAL_LIFETIME,
    std::string{"SCORE"},
    false,
    _sk->getFontKey(Snake::FID_KONGTEXT)
  ));
  _uidLabels[LID_SCORE_VALUE] = _hud->addLabel(std::make_unique<HUD::IntLabel>(
    Vector2i{66, 182},
    Snake::scorePopupColor,
    0.f,
    HUD::IMMORTAL_LIFETIME,
    _sk->getScoreReference(),
    5,
    _sk->getFontKey(Snake::FID_KONGTEXT)
  ));

  int sid = Snake::NuggetSpriteID::SID_NUGGET_GOLD;
  int cid = Snake::NuggetClassID::NUGGET_GOLD;
  int lid = LID_GOLD_SPRITE;
  for(int i{0}; i < Snake::nuggetClassCount; ++i){
    Vector2i position {
      6 + (27 * i),
      173
    };
    _uidLabels[lid] = _hud->addLabel(std::make_unique<HUD::BitmapLabel>(
      position,
      Snake::scorePopupColor,
      0.f,
      HUD::IMMORTAL_LIFETIME,
      _sk->getSpritesheetKey(Snake::SSID_NUGGETS),
      sid
    ));
    position._x += 5; 
    position._y -= 2;
    _uidLabels[lid + 1] = _hud->addLabel(std::make_unique<HUD::IntLabel>(
      position,
      Snake::scorePopupColor,
      0.f,
      HUD::IMMORTAL_LIFETIME,
      _sk->getNuggetsEatenReference(static_cast<Snake::NuggetClassID>(cid)),
      3,
      _sk->getFontKey(Snake::FID_KONGTEXT)
    ));
    ++sid;
    ++cid;
    lid += 2;
  }

  _uidLabels[LID_SPEED_BONUS] = _hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{142, 182},
    Snake::scorePopupColor,
    0.f,
    HUD::IMMORTAL_LIFETIME,
    std::string{"+      %"},
    false,
    _sk->getFontKey(Snake::FID_KONGTEXT)
  ));
  _uidLabels[LID_SPEED_BONUS_VALUE] = _hud->addLabel(std::make_unique<HUD::IntLabel>(
    Vector2i{150, 182},
    Snake::scorePopupColor,
    0.f,
    HUD::IMMORTAL_LIFETIME,
    _currentSpeedBonusAsInt,
    3,
    _sk->getFontKey(Snake::FID_KONGTEXT)
  ));

  updateSpeedBar(0, false);
}

void PlayScene::addGameOverToHUD()
{
  auto txtSize = gfx::calculateTextSize("GAME OVER", _sk->getFontKey(Snake::FID_KONGTEXT));
  auto txtPos = Vector2i{};
  txtPos._x = (Snake::worldSize_rx._x / 2) - (txtSize._x / 2);
  txtPos._y = (Snake::worldSize_rx._y / 2) - (txtSize._y / 2);
  _uidGameOverLabel = _hud->addLabel(std::make_unique<HUD::TextLabel>(
    txtPos,
    Snake::scorePopupColor,
    0.f,
    HUD::IMMORTAL_LIFETIME,
    std::string{"GAME OVER"},
    false,
    _sk->getFontKey(Snake::FID_KONGTEXT)
  ));
}

void PlayScene::clearHUD()
{
  for(auto& uid : _uidLabels)
    _hud->removeLabel(uid);

  _hud->removeLabel(_uidGameOverLabel);
}

void PlayScene::stepSnake()
{
  if(_accumulatedGrowths > 0){
    _snakeLength = std::min(_snakeLength + 1, Snake::maxSnakeLength - 1);
    --_accumulatedGrowths; 
  }
  for(int block {_snakeLength - 1}; block > SNAKE_HEAD_BLOCK; --block){
    _snake[block]._row = _snake[block - 1]._row;
    _snake[block]._col = _snake[block - 1]._col;
  }

  _currentMoveDirection = _nextMoveDirection;
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

  updateSnakeBlockSpriteIDs();
}

void PlayScene::growSnake()
{
  _accumulatedGrowths += Snake::growthsPerNugget;
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

  if(_isSnakeSmoothMover)
    updateSmoothSnakeBlockSpriteIDs();
}

void PlayScene::updateSmoothSnakeBlockSpriteIDs()
{
  for(int block {SNAKE_HEAD_BLOCK}; block < _snakeLength - 1; ++block){
    gfx::SpriteID_t spriteid;
    if(block == SNAKE_HEAD_BLOCK)
      spriteid = Snake::smoothSnakeHeadBlockTree[_snake[block]._currentMoveDirection];
    else if(block == _snakeLength - 1)
      spriteid = Snake::snakeTailBlockTree[_snake[block]._currentMoveDirection];
    else
      spriteid = Snake::smoothSnakeBodyBlockTree[_snake[block]._currentMoveDirection];
    _snake[block]._spriteid = spriteid;
  }
}

bool PlayScene::collideSnakeNuggets()
{
  for(auto& nugget : _nuggets){
    if(!nugget._isAlive) continue;
    bool collision = _snake[SNAKE_HEAD_BLOCK]._row == nugget._row &&
                     _snake[SNAKE_HEAD_BLOCK]._col == nugget._col;
    if(collision){ 
      eatNugget(nugget);
      growSnake();
      return true;
    }
  }
  return false;
}

bool PlayScene::collideSnakeSnake()
{
  int headRow {_snake[SNAKE_HEAD_BLOCK]._row};
  int headCol {_snake[SNAKE_HEAD_BLOCK]._col};
  for(int block{SNAKE_HEAD_BLOCK + 1}; block < _snakeLength; ++block){
    bool collision = headRow == _snake[block]._row &&
                     headCol == _snake[block]._col;
    if(collision){
      _snakeBlockEaten = block;
      setNextState(State::GAME_OVER);
      return true;
    }
  }
  return false;
}

void PlayScene::updateSpeedBar(int speedBarState, bool removeFirst)
{
  if(removeFirst)
    _hud->removeLabel(_uidLabels[LID_SPEED_BAR]);

  _uidLabels[LID_SPEED_BAR] = _hud->addLabel(std::make_unique<HUD::BitmapLabel>(
    Vector2i{110, 183},
    Snake::scorePopupColor,
    0.f,
    HUD::IMMORTAL_LIFETIME,
    _sk->getSpritesheetKey(Snake::SSID_SPEED_BAR),
    speedBarState
  ));

  _currentSpeedBarState = speedBarState;
}

void PlayScene::updateSpeedBonusHUD()
{
  int speedBarState;
  if(_speedClock_s < 0.f){
    _currentSpeedBonusAsInt = 0;
    speedBarState = 0;
  }
  else{
    speedBarState = std::ceil(_speedClock_s / Snake::speedBarStateTimeDelta_s);
    assert(1 <= speedBarState && speedBarState < Snake::numSpeedBarStates);
  }
  if(speedBarState != _currentSpeedBarState)
    updateSpeedBar(speedBarState);
}

bool PlayScene::havePossibleSameCombo()
{
  if(_eatHistorySize <= 1) 
    return true;

  if(_eatHistorySize == 2)
    return _eatHistory[0] == _eatHistory[1];

  for(int i{1}; i < _eatHistorySize - 1; ++i)
    if(_eatHistory[0] != _eatHistory[i]) return false;

  return true;
}

bool PlayScene::havePossibleOrderCombo()
{
  if(_eatHistorySize == 0) return true;

  if(_eatHistory[0] != Snake::NUGGET_AMETHYST)
    return false;

  for(int i{1}; i < _eatHistorySize; ++i){
    if(_eatHistory[i] != Snake::NUGGET_AMETHYST - i)
      return false;
  }

  return true;
}

bool PlayScene::haveSame3Combo()
{
  if(_eatHistorySize < 4) return false;
  if(_eatHistorySize > 5) return false; // same6 takes over this case.
  int sameCount {1};
  for(int i{1}; i < _eatHistorySize; ++i){
    if(_eatHistory[0] == _eatHistory[i])
      ++sameCount;
    else 
      break;
  }
  if(3 <= sameCount && sameCount < _eatHistorySize)
    return true;

  return false;
}

bool PlayScene::haveSame6Combo()
{
  if(_eatHistorySize < 6) return false;
  for(int i{1}; i < 6; ++i)
    if(_eatHistory[0] != _eatHistory[i]) return false;
  return true;
}

bool PlayScene::haveOrderCombo()
{
  if(_eatHistorySize < Snake::nuggetClassCount) return false;
  for(int i{0}; i < Snake::nuggetClassCount; ++i)
    if(_eatHistory[i] != Snake::NUGGET_AMETHYST - i)
      return false;
  return true;
}

float PlayScene::doSpeedBonus(const Nugget& eaten) // TODO: unused parameter - remove it
{
  float speedBonus {1.f};
  if(_speedClock_s > 0.f){
    speedBonus += Snake::speedBonusTable[_speedBonusTableIndex];
    _speedBonusTableIndex = std::clamp(_speedBonusTableIndex + 1, 0, Snake::speedBonusCount - 1);
  }
  else{
    _speedBonusTableIndex = 0;
  }
  _speedClock_s = Snake::speedBonusCooldown_s;
  _currentSpeedBonusAsInt = std::round((speedBonus - 1.f) * 100.f);
  return speedBonus;
}

void PlayScene::clearEatHistory()
{
  _eatHistorySize = 0;
}

void PlayScene::reduceEatHistory()
{
  if(_eatHistorySize <= 1) return;
  while(_eatHistorySize > 1 && !havePossibleOrderCombo()){
    //std::shift_left(_eatHistory.begin(), _eatHistory.begin() + _eatHistorySize, 1); 
    for(int i {0}; i < _eatHistorySize - 1; ++i){
      _eatHistory[i] = _eatHistory[i + 1];
    }
    --_eatHistorySize;
  }
}

int PlayScene::applyScoreBonuses(const Nugget& eaten)
{
  assert(_eatHistorySize != Snake::longestPossibleCombo);
  _eatHistory[_eatHistorySize++] = eaten._classID;

  if(!(havePossibleSameCombo() || havePossibleOrderCombo()))
    reduceEatHistory();

  int sum;
  bool haveSame3, haveSame6, haveOrder;
  haveSame3 = haveSame3Combo();
  haveSame6 = haveSame6Combo();
  haveOrder = haveOrderCombo();

  sum = haveSame3 + haveSame6 + haveOrder; 
  assert(sum <= 1);

  int comboBonus {1};
  if     (haveSame3) comboBonus = Snake::same3ComboBonus;
  else if(haveSame6) comboBonus = Snake::same6ComboBonus;
  else if(haveOrder) comboBonus = Snake::orderComboBonus;

  if(sum) clearEatHistory();

  float speedBonus = doSpeedBonus(eaten);
  return Snake::nuggetClasses[eaten._classID]._score * comboBonus * speedBonus;
}

void PlayScene::spawnNuggetScorePopup(const Nugget& eaten, int score)
{
  Vector2i position {
    Snake::boardPosition._x + (eaten._col * Snake::blockSize_rx),
    Snake::boardPosition._y + (eaten._row * Snake::blockSize_rx)
  };

  position._x += (eaten._col > Snake::boardSize._x / 2) ? -Snake::scorePopupOffset : Snake::scorePopupOffset;
  position._y += (eaten._row > Snake::boardSize._y / 2) ? -Snake::scorePopupOffset : Snake::scorePopupOffset;

  _hud->addLabel(std::make_unique<HUD::TextLabel>(
    position,
    Snake::scorePopupColor,
    0.f,
    Snake::scorePopupLifetime_s,
    std::to_string(score),
    false,
    _sk->getFontKey(Snake::FID_KONGTEXT)
  ));
}

void PlayScene::eatNugget(Nugget& nugget)
{
  int score = applyScoreBonuses(nugget);
  spawnNuggetScorePopup(nugget, score);
  _sk->addScore(score);
  _sk->addNuggetEaten(nugget._classID, 1);
  nugget._isAlive = false;
  --_numNuggetsInWorld;
  sfx::SoundChannel_t channel = sfx::playSound(_sk->getSoundEffectKey(Snake::SFX_SCORE_BEEP));
}

void PlayScene::eatSnake()
{
  assert(SNAKE_HEAD_BLOCK < _snakeBlockEaten && _snakeBlockEaten < _snakeLength);
  _snake[_snakeBlockEaten]._spriteid = Snake::SID_EMPTY_BLOCK;
  if(_snakeBlockEaten - 1 > SNAKE_HEAD_BLOCK)
    _snake[_snakeBlockEaten - 1]._spriteid = Snake::SID_BLOOD_BLOCK;
  if(_snakeBlockEaten + 1 < _snakeLength)
    _snake[_snakeBlockEaten + 1]._spriteid = Snake::SID_BLOOD_BLOCK;
}

void PlayScene::drawBackground()
{
  gfx::drawSprite(
    Vector2f{0.f, 0.f},
    _sk->getSpritesheetKey(Snake::SSID_PLAY_BACKGROUND),
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
      _snake[block]._spriteid + (_sk->getSnakeHero() * Snake::SID_SNAKE_OFFSET),
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
      _snake[block]._spriteid + (_sk->getSnakeHero() * Snake::SID_SNAKE_OFFSET),
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
