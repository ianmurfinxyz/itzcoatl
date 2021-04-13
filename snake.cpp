#include <cassert>
#include "pxr_gfx.h"

#include "snake.h"
#include "play_scene.h"
#include "menu_scene.h"

using namespace pxr;

bool Snake::onInit()
{
  for(int i {Snake::SCREEN_BACKGROUND}; i < Snake::SCREEN_COUNT; ++i)
    _screens.push_back(gfx::createScreen(worldSize_rx));

  loadSpritesheets();
  loadFonts();
  loadSoundEffects();
  loadMusicLoops();
  _snakeHero = MONTEZUMA;

  _hud = new HUD(hudFlashPeriod, hudPhaseInPeriod);

 //_activeScene = std::shared_ptr<pxr::Scene>(new PlayScene(this));
 _activeScene = std::shared_ptr<pxr::Scene>(new MenuScene(this));
  if(!_activeScene->onInit()) return false;
  _activeScene->onEnter();
  _scenes.emplace(_activeScene->getName(), _activeScene);

  return true;
}

void Snake::onShutdown()
{
}

sfx::ResourceKey_t Snake::getSoundEffectKey(SoundEffectID sfxID)
{
  assert(0 <= sfxID && sfxID < SFX_COUNT);
  return _soundEffectKeys[sfxID];
}

sfx::ResourceKey_t Snake::getMusicLoopKey(MusicLoopID musicID)
{
  assert(0 <= musicID && musicID < MUSIC_COUNT);
  return _musicLoopKeys[musicID];
}

gfx::ResourceKey_t Snake::getSpritesheetKey(SpritesheetID sheetID)
{
  assert(0 <= sheetID && sheetID < SSID_COUNT);
  return _spritesheetKeys[sheetID];
}

gfx::ResourceKey_t Snake::getFontKey(FontID fontID)
{
  assert(0 <= fontID && fontID < FID_COUNT);
  return _fontKeys[fontID];
}

gfx::ScreenID_t Snake::getScreenID(GFXScreenName screenName)
{
  return _screens[screenName];
}

sfx::MusicSequence_t Snake::getMusicSequence(MusicSequenceID sequenceID)
{
  assert(0 <= sequenceID && sequenceID <= MUSIC_SEQUENCE_COUNT);
  const MusicIDSequence_t& idSequence = musicIDSequences[sequenceID];
  sfx::MusicSequence_t sequence {};
  for(const auto& node : idSequence){
    sequence.push_back({
      getMusicLoopKey(node._musicID),
      static_cast<int>(node._fadeInDuration_s * 1000),
      static_cast<int>(node._playDuration_s * 1000),
      static_cast<int>(node._fadeOutDuration_s * 1000)
    });
  }
  return sequence;
}

void Snake::loadSpritesheets()
{
  for(int ssid {0}; ssid < SSID_COUNT; ++ssid)
    _spritesheetKeys[ssid] = gfx::loadSpritesheet(spritesheetNames[ssid]);
}

void Snake::loadFonts()
{
  for(int fid{0}; fid < FID_COUNT; ++fid)
    _fontKeys[fid] = gfx::loadFont(fontNames[fid]);
}

void Snake::loadSoundEffects()
{
  for(int sfxid {0}; sfxid < SFX_COUNT; ++sfxid)
    _soundEffectKeys[sfxid] = sfx::loadSoundWAV(soundEffectNames[sfxid]);
}

void Snake::loadMusicLoops()
{
  for(int musicID {0}; musicID < MUSIC_COUNT; ++musicID)
    _musicLoopKeys[musicID] = sfx::loadMusicWAV(musicLoopNames[musicID]);
}

