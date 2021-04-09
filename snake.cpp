#include <cassert>
#include "pxr_gfx.h"

#include "snake.h"
#include "play_scene.h"


using namespace pxr;

bool Snake::onInit()
{
  for(int i {Snake::SCREEN_BACKGROUND}; i < Snake::SCREEN_COUNT; ++i)
    _screens.push_back(gfx::createScreen(worldSize_rx));

  loadSpritesheets();
  loadSoundEffects();
  _snakeHero = ITZCOATL;

 _activeScene = std::shared_ptr<pxr::Scene>(new PlayScene(this));
  if(!_activeScene->onInit()) return false;
  _activeScene->onEnter();
  _scenes.emplace(_activeScene->getName(), _activeScene);

  return true;
}

void Snake::onShutdown()
{
}

sfx::ResourceKey_t Snake::getSoundEffectKey(SoundEffectID sfxid)
{
  assert(0 <= sfxid && sfxid < SFX_COUNT);
  return _soundEffectKeys[sfxid];
}

gfx::ResourceKey_t Snake::getSpritesheetKey(SpritesheetID sheetid)
{
  assert(0 <= sheetid && sheetid < SSID_COUNT);
  return _spritesheetKeys[sheetid];
}

gfx::ScreenID_t Snake::getScreenID(GFXScreenName screenName)
{
  return _screens[screenName];
}

void Snake::loadSpritesheets()
{
  for(int ssid {0}; ssid < SSID_COUNT; ++ssid)
    _spritesheetKeys[ssid] = gfx::loadSpritesheet(spritesheetNames[ssid]);
}

void Snake::loadSoundEffects()
{
  for(int sfxid {0}; sfxid < SFX_COUNT; ++sfxid)
    _soundEffectKeys[sfxid] = sfx::loadSoundWAV(soundEffectNames[sfxid]);
}
