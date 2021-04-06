#ifndef _PLAY_SCENE_H_
#define _PLAY_SCENE_H_

#include <array>
#include "snake.h"

class PlayScene final : public pxr::Scene
{
  static constexpr const char* name {"play_scene"};

public:
  PlayScene(pxr::Game* owner);
  ~PlayScene();

  void onEnter();
  void onUpdate(double now, float dt);
  void onDraw(double now, float dt, int screenid);
  void onExit();

  std::string getName() const {return name;}

private:

  struct SnakeBlock
  {
    gfx::SpriteID_t _spriteid;
    int _row;
    int _col;
  };

private:
  void initializeSnake();
  void stepSnake();

  Snake::Direction findNeighbourDirection(const SnakeBlock& self, const SnakeBlock& neighbour);
  void updateBlockSpriteIDs();

  void animateTongue();

  void drawTongue();
  void drawSnake();

private:
  static constexpr size_t SNAKE_HEAD_BLOCK {0};
  std::array<SnakeBlock, makeSnakeLength> _snake;
  int _snakeLength;
  Direction _moveDirection;

  float _stepClock_s;
};

#endif
