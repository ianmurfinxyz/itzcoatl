#include <cassert>
#include "pxr_gfx.h"

#include "snake.h"
#include "play_scene.h"


using namespace pxr;

bool Snake::onInit()
{
 _active = std::shared_ptr<pxr::Scene>(new PlayScene(this));
  if(!_active->onInit()) return false;
  _active->onEnter();
  _scenes.emplace(_active->getName(), _active);

  _activeScreenid = gfx::createScreen(worldSize_rx);

  loadSpritesheets();
  _snakeHero = MONTEZUMA;
  return true;
}

void Snake::onShutdown()
{
}

gfx::ResourceKey_t Snake::getSpritesheetKey(SpritesheetID sheetid)
{
  assert(0 <= sheetid && sheetid < SSID_COUNT);
  return _spritesheetKeys[sheetid];
}

void Snake::loadSpritesheets()
{
  for(int ssid {SSID_SNAKES}; ssid < SSID_COUNT; ++ssid)
    _spritesheetKeys[ssid] = gfx::loadSpritesheet(spritesheetNames[ssid]);
}
