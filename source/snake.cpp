
#include "pxr_gfx.h"

#include "snake.h"
#include "play_scene.h"


using namespace pxr;

bool Snake::onInit()
{
  _activeScreenId = gfx::createScreen(worldSize_rx);

 _active = std::shared_ptr<pxr::Scene>(new PlayScene(this));
  if(!_active->onInit())
    return false;
  _scenes.emplace(_active->getName(), _active);
}

void Snake::onShutdown()
{
}
