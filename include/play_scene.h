#ifndef _PLAY_SCENE_H_
#define _PLAY_SCENE_H_

#include <array>
#include "pxr_gfx.h"
#include "snake.h"

class PlayScene final : public pxr::Scene
{
  static constexpr const char* name {"play_scene"};

public:
  PlayScene(pxr::Game* owner);
  ~PlayScene() = default;

  bool onInit();
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
  void updateSnakeBlockSpriteIDs();

  void animateTongue(){}

  void drawTongue(){}
  void drawSnake(int screenid);

private:
  Snake* _sk;

  static constexpr size_t SNAKE_HEAD_BLOCK {0};
  std::array<SnakeBlock, Snake::maxSnakeLength> _snake;
  int _snakeLength;
  Snake::Direction _moveDirection;

  float _stepClock_s;
};

#endif
