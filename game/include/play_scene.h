#ifndef _PLAY_SCENE_H_
#define _PLAY_SCENE_H_

#include <array>
#include "pxr_gfx.h"
#include "pxr_input.h"
#include "itzcoatl.h"

class PlayScene final : public pxr::Scene
{
public:
  static constexpr const char* name {"play_scene"};

public:
  PlayScene(pxr::Game* owner);
  ~PlayScene() = default;

  bool onInit();
  void onEnter();
  void onUpdate(double now, float dt);
  void onDraw(double now, float dt, const std::vector<gfx::ScreenID_t>& screens);
  void onExit();

  std::string getName() const {return name;}

private:
  enum class State { PLAYING, GAME_OVER, NONE };

  struct SnakeBlock
  {
    gfx::SpriteID_t _spriteid;
    Snake::Direction _currentMoveDirection;  // used for smooth movement
    int _row;
    int _col;
  };

  struct Nugget
  {
    Snake::NuggetClassID _classID;
    int _row;
    int _col;
    float _flashClock;
    bool _isVisible;
    bool _isAlive;
  };


private:
  void onEnterPlaying();
  void onUpdatePlaying(double now, float dt);
  void handlePlayingInput();
  void onExitPlaying();
  void onEnterGameOver();
  void onUpdateGameOver(double now, float dt);
  void handleGameOverInput();
  void onExitGameOver();
  void setNextState(State nextState);
  void switchState();

  void initializeSnake();
  void initializeNuggets();
  void populateHUD();
  void addGameOverToHUD();
  void clearHUD();
  void stepSnake();
  void growSnake();
  void spawnNugget();
  Snake::Direction findNeighbourDirection(const SnakeBlock& self, const SnakeBlock& neighbour);
  void updateSnakeBlockSpriteIDs();
  void updateSmoothSnakeBlockSpriteIDs();
  bool collideSnakeNuggets();
  bool collideSnakeSnake();
  void updateSpeedBar(int speedBarState, bool removeFirst = true);
  void updateSpeedBonusHUD();
  void eatSnake();
  void animateTongue(){}
  void drawBackground();
  void drawForeground();
  void drawTongue(){}
  void drawSnake(gfx::ScreenID_t screenid);
  void drawSmoothSnake(gfx::ScreenID_t screenid);
  void drawNuggets(gfx::ScreenID_t screenid);

  bool havePossibleSameCombo();
  bool havePossibleOrderCombo();
  bool haveSame3Combo();
  bool haveSame6Combo();
  bool haveOrderCombo();
  float doSpeedBonus(const Nugget& eaten);
  void clearEatHistory();
  void reduceEatHistory();
  int applyScoreBonuses(const Nugget& eaten);
  void spawnNuggetScorePopup(const Nugget& eaten, int score);
  void eatNugget(Nugget& nugget);

private:

  enum LabelID
  {
    LID_SCORE,
    LID_SCORE_VALUE,
    LID_GOLD_SPRITE,
    LID_GOLD_COUNT,
    LID_SILVER_SPRITE,
    LID_SILVER_COUNT,
    LID_OBSIDIAN_SPRITE,
    LID_OBSIDIAN_COUNT,
    LID_RUBY_SPRITE,
    LID_RUBY_COUNT,
    LID_JADE_SPRITE,
    LID_JADE_COUNT,
    LID_LAPIS_SPRITE,
    LID_LAPIS_COUNT,
    LID_AMETHYST_SPRITE,
    LID_AMETHYST_COUNT,
    LID_SPEED_BONUS,
    LID_SPEED_BONUS_VALUE,
    LID_SPEED_BAR,
    LID_COUNT
  };

  HUD* _hud;
  std::array<HUD::uid_t, LID_COUNT> _uidLabels;
  HUD::uid_t _uidGameOverLabel;

  State _currentState;
  State _nextState;

  Snake* _sk;

  static constexpr int SNAKE_HEAD_BLOCK {0};
  std::array<SnakeBlock, Snake::maxSnakeLength> _snake;
  int _snakeLength;
  int _snakeBlockEaten;
  int _accumulatedGrowths;
  Snake::Direction _nextMoveDirection;
  Snake::Direction _currentMoveDirection;
  bool _isSnakeSmoothMover;

  std::array<Nugget, Snake::maxNuggetsInWorld> _nuggets;
  int _numNuggetsInWorld;

  float _stepClock_s;
  float _gameOverClock_s;

  std::array<Snake::NuggetClassID, Snake::longestPossibleCombo> _eatHistory;
  int _eatHistorySize;

  float _speedClock_s;
  int _currentSpeedBonusAsInt;
  int _currentSpeedBarState;
  int _speedBonusTableIndex;
};

#endif
