#include <string>
#include <map>
#include <cmath>
#include <cassert>
#include <vector>
#include <SDL2/SDL_mixer.h>
#include "pxr_sfx.h"
#include "pxr_log.h"
#include "pxr_wav.h"

namespace pxr
{
namespace sfx
{

/////////////////////////////////////////////////////////////////////////////////////////////////
// MODULE DATA
/////////////////////////////////////////////////////////////////////////////////////////////////

struct SoundResource
{
  std::string _name = "";
  Mix_Chunk* _chunk = nullptr;
  int _referenceCount = 0;
};

//
// Nyquist-Shannon sampling theorem states sampling frequency should be atleast twice 
// that of largest wave frequency. Thus do not make the wave freq > half sampling frequency.
//
static constexpr int errorSoundFreq_hz {200};
static constexpr float errorSoundDuration_s {0.5f};
static constexpr int errorSoundVolume {MAX_VOLUME};
static ResourceName_t errorSoundName {"sfxerror"};
static ResourceKey_t errorSoundKey {0};

//
// The set of all loaded sounds accessed via their resource key.
//
ResourceKey_t nextResourceKey {0};
static std::map<ResourceKey_t, SoundResource> sounds;

//
// The configuration this module was initialized with.
//
SFXConfiguration sfxconfiguration;

//
// Maintains data on which channel is playing which sound. Channel ids range from 0 up to
// sfxconfiguration._numMixChannels - 1.
//
static ResourceKey_t nullResourceKey {-1};
static std::vector<ResourceKey_t> channelPlayback;

//
// An array of current volumes for all mix channels. Stored here because in SDL_Mixer there 
// appears to be no way to get the volume of a channel without setting it.
//
static std::vector<int> channelVolume;

//
// The set of all sounds waiting to be unloaded once all running instances have stopped
// playback.
//
static std::vector<ResourceKey_t> unloadQueue;

/////////////////////////////////////////////////////////////////////////////////////////////////
// MODULE FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////

void onChannelFinished(int channel)
{
  assert(0 <= channel && channel < sfxconfiguration._numMixChannels);
  channelPlayback[channel] = nullResourceKey;
}

//
// Generates a short sinusoidal beep.
//
template<typename T>
MixChunk* generateSineBeep(int waveFreq_hz, float waveDuration_s)
{
  static_assert(std::numeric_limits<T>::is_integer);

  float waveFreq_rad_per_s {waveFreq_hz * 2.f * M_PI};
  int sampleCount = sfxconfiguration._samplingFreq_hz * waveDuration_s;
  float samplePeriod_s = 1.f / sfxconfiguration._samplingFreq_hz;
  T* pcm = new T[sampleCount];
  for(int s = 0; s < sampleCount; ++s){
    float sf = sinf(wavFreq_rad_per_s * (s * samplePeriod_s));   // wave equation = sin(wt)
    if(std::numeric_limits<T>::is_signed){
      if(sf < 0.f)  pcm[s] = static_cast<T>(sf * std::numeric_limits<T>::min());
      if(sf >= 0.f) pcm[s] = static_cast<T>(sf * std::numeric_limits<T>::max());
    }
    else{
      pcm[s] = static_cast<T>((sf + 1.f) * (std::numeric_limits<T>::max() / 2));
    }
  }

  Mix_Chunk* chunk = new Mix_Chunk();
  chunk.allocated = 1;
  chunk.abuf = static_cast<uint8_t*>(pcm);
  chunk.alen = sampleCount * sizeof(T);
  chunk.volume = errorSoundVolume;
  return chunk;
}

static void generateErrorSound(SampleFormat format)
{
  Mix_Chunk* chunk {nullptr};
  switch(format){
    case SAMPLE_FORMAT_U8     : {chunk = generateSineBeep<uint8_t >(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_S8     : {chunk = generateSineBeep<int8_t  >(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_U16LSB : {chunk = generateSineBeep<uint16_t>(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_S16LSB : {chunk = generateSineBeep<int16_t >(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_U16    : {chunk = generateSineBeep<uint16_t>(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_S16    : {chunk = generateSineBeep<int16_t >(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_S32LSB : {chunk = generateSineBeep<int32_t >(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_S32    : {chunk = generateSineBeep<int32_t >(errorSoundFreq_hz, errorSoundDuration_s); break;}
    default: assert(0);
  }
  SoundResource resource {};
  resource._name = errorSoundName;
  resource._chunk = chunk;
  resource._referenceCount = 0;
  errorSoundKey = nextResourceKey++;
  sounds.emplace(std::make_pair(errorSoundKey, resource));
}

static void freeErrorSound()
{
  auto search = sounds.find(errorSoundKey);
  assert(search != sounds.end());
  auto& resource = search.second;
  delete[] resource._chunk.abuf;
  delete resource._chunk;
  resource._chunk = nullptr;
  sounds.erase(search);
}

static void logSpec()
{
  const SDL_version* version = Mix_Linked_Version();
  log::log(LOG::INFO, "SDL_Mixer Version:");
  log::log(LOG::INFO, "major:", std::to_string(version->major));
  log::log(LOG::INFO, "minor:", std::to_string(version->minor));
  log::log(LOG::INFO, "patch:", std::to_string(version->patch));

  int freq, channels; uint16_t format;
  if(!Mix_QuerySpec(&freq, &format, &channels)){
    log::log(Log::WARN, msg_sfx_fail_query_spec, std::string{Mix_GetError()});
    return;
  }
  
  const char* formatString {nullptr};
  switch(format){
    case SAMPLE_FORMAT_U8    : {formatString = "U8";     break;}
    case SAMPLE_FORMAT_S8    : {formatString = "S8";     break;}
    case SAMPLE_FORMAT_U16LSB: {formatString = "U16LSB"; break;}
    case SAMPLE_FORMAT_S16LSB: {formatString = "S16LSB"; break;}
    case SAMPLE_FORMAT_U16MSB: {formatString = "U16MSB"; break;}
    case SAMPLE_FORMAT_S16MSB: {formatString = "S16MSB"; break;}
    case SAMPLE_FORMAT_U16   : {formatString = "U16";    break;}
    case SAMPLE_FORMAT_S16   : {formatString = "S16";    break;}
    case SAMPLE_FORMAT_S32LSB: {formatString = "S32LSB"; break;}
    case SAMPLE_FORMAT_S32MSB: {formatString = "S32MSB"; break;}
    case SAMPLE_FORMAT_S32   : {formatString = "S32";    break;}
    case SAMPLE_FORMAT_F32LSB: {formatString = "F32LSB"; break;}
    case SAMPLE_FORMAT_F32MSB: {formatString = "F32MSB"; break;}
    case SAMPLE_FORMAT_F32   : {formatString = "F32";    break;}
    case SAMPLE_FORMAT_U16SYS: {formatString = "U16SYS"; break;}
    case SAMPLE_FORMAT_S16SYS: {formatString = "S16SYS"; break;}
    case SAMPLE_FORMAT_S32SYS: {formatString = "S32SYS"; break;}
    case SAMPLE_FORMAT_F32SYS: {formatString = "F32SYS"; break;}
    default                    {formatString = "unknown format";}
  }

  const char* modeString {nullptr};
  switch(channels){
    case OutputMode::MONO:   {modeString = "mono";   break;}
    case OutputMode::STEREO: {modeString = "stereo"; break;}
    default:                 {modeString = "unknown mode";}
  }

  log::log(LOG::INFO, "SDL_Mixer Audio Device Spec: ");
  log::log(LOG::INFO, "sample frequency: ", to_string(freq));
  log::log(LOG::INFO, "sample format: ", formatString);
  log::log(LOG::INFO, "output mode: ", modeString);
}

bool initialize(SFXConfiguration sfxconf)
{
  assert(!(SDL_AUDIO_ISFLOAT(sfxconf._sampleFormat)));
  log::log(log::INFO, log::msg_sfx_initializing);
  sfxconfiguration = sfxconf;
  int result = Mix_OpenAudio(
    sfxconf._samplingFreq_hz, 
    sfxconf._sampleFormat, 
    sfxconf.outputMode, 
    sfxconf._chunkSize
  );
  if(result != 0){
    log->log(Log::ERROR, log::sfx_fail_init, std::string{Mix_GetError()});
    return false;
  }
  Mix_AllocateChannels(sfxconf._numMixChannels);
  Mix_ChannelFinished(&onChannelFinished);
  channelPlayback.resize(sfxconf._numMixChannels, nullResourceKey);
  channelPlayback.shrink_to_fit();
  channelVolumes.resize(sfxconf._numMixChannels, MAX_VOLUME);
  channelVolumes.shrink_to_fit();
  generateErrorSound(sfxconf._sampleFormat);
  logSpec();
  return true;
}

void shutdown()
{
  stopChannel(ALL_CHANNELS);
  freeErrorSound();
  for(auto& pair : sounds)
    Mix_FreeChunk(pair.second._chunk);
  sounds.clear();
  Mix_CloseAudio();
}

static void unloadSound(ResourceKey_t soundKey)
{
  assert(soundKey != errorSoundKey);
  auto search = sounds(soundKey);
  assert(search != sounds.end());
  Mix_FreeChunk(search->second._chunk);
  sounds.erase(search);
}

static void unloadUnusedSounds()
{
  auto first = std::remove_if(unloadQueue.begin(), unloadQueue.end(), [](ResourceKey_t key){
    return std::find(channelPlayback.begin(), channelPlayback.end(), resourceKey) == channelPlayback.end();
  });
  auto first_copy = first;
  while(first_copy != unloadQueue.end()){
    unloadSound(*first_copy);
    ++first_copy;
  }
  unloadQueue.erase(first, unloadQueue.end());
}

void service(float dt)
{
  unloadUnusedSounds();
}

static ResourceKey_t returnErrorSound()
{
  auto search = sounds(errorSoundKey);
  assert(search != sounds.end());
  search->second._referenceCount++;
  log::log(log::INFO, log::msg_sfx_error_sound_usage, std::to_string(search->second._referenceCount));
  return errorSoundKey;
}

ResourceKey_t loadSoundWAV(ResourceName_t soundName)
{
  log::log(log::INFO, log::msg_sfx_loading_sound, soundName);

  for(auto& pair : sounds){
    if(pair.second._name == soundName){
      pair.second._referenceCount++;
      std::string addendum {"reference count="};
      addendum += std::to_string(pair.second._referenceCount);
      log::log(log::INFO, log::msg_sfx_sound_already_loaded, addendum);
      return pair.first;
    }
  }

  SoundResource resource {};
  std::string wavpath {};
  wavpath += RESOURCE_PATH_SOUNDS;
  wavpath += soundName;
  wavpath += Wav::FILE_EXTENSION;
  resource._chunk = Mix_LoadWav(wavpath.c_str());
  if(resource._chunk = nullptr){
    log::log(log::ERROR, log::msg_sfx_fail_load_sound, wavpath);
    log::log(log::INFO, log::msg_sfx_using_error_sound, wavpath);
    return useErrorSound();
  }
  resource._name = soundName;
  resource._referenceCount = 1;

  ResourceKey_t newKey = nextResourceKey++;
  sounds.emplace(std::make_pair(newKey, resource));

  std::string addendum{};
  addendum += "[name:key]=[";
  addendum += soundName;
  addendum += ":";
  addendum += std::to_string(newKey);
  addendum += "]";
  log::log(log::INFO, log::msg_sfx_load_sound_success, addendum);

  return newKey;
}

void queueUnloadSound(ResourceKey_t soundKey)
{
  assert(soundKey != errorSoundKey);
  auto search = sounds.find(soundKey);
  if(search == sounds.end()){
    log::log(log::WARN, log::msg_sfx_unloading_nonexistent_sound, std::to_string(soundKey));
    return;
  }
  unloadQueue.push_back(soundKey);
}

static Mix_Chunk* findChunk(ResourceKey_t soundKey)
{
  auto search = sounds.find(soundKey);
  if(search == sounds.end()){
    log::log(log::WARN, log::msg_sfx_playing_nonexistent_sound, std::to_string(soundKey));
    return nullptr;
  }
  return search->second._chunk;
}

static SoundChannel_t onPlayError(ResourceKey_t soundKey)
{
  std::string addendum{};
  addendum += std::to_string(soundKey);
  addendum += " : ";
  addendum += Mix_GetError();
  log::log(log::WARN, log::msg_sfx_fail_play_sound, addendum);
  return NULL_CHANNEL;
}

SoundChannel_t playSound(ResourceKey_t soundKey, int loops)
{
  auto* chunk = findChunk(soundKey);
  if(chunk == nullptr) return NULL_CHANNEL;
  SoundChannel_t channel = Mix_PlayChannel(-1, chunk, loops);
  if(channel == -1) return onPlayError();
  assert(channelPlayback[channel] == nullResourceKey);
  channelPlayback[channel] = soundKey;
  return channel;
}

SoundChannel_t playSoundTimed(ResourceKey_t soundKey, int loops, int playDuration_ms)
{
  auto* chunk = findChunk(soundKey);
  if(chunk == nullptr) return NULL_CHANNEL;
  SoundChannel_t channel = Mix_PlayChannelTimed(-1, chunk, loops, playDuration_ms);
  if(channel == -1) return onPlayError();
  assert(channelPlayback[channel] == nullResourceKey);
  channelPlayback[channel] = soundKey;
  return channel;
}

SoundChannel_t playSoundFadeIn(ResourceKey_t soundKey, int loops, int fadeDuration_ms)
{
  auto* chunk = findChunk(soundKey);
  if(chunk == nullptr) return NULL_CHANNEL;
  SoundChannel_t channel = Mix_FadeInChannel(-1, chunk, loops, fadeDuration_ms);
  if(channel == -1) return onPlayError();
  assert(channelPlayback[channel] == nullResourceKey);
  channelPlayback[channel] = soundKey;
  return channel;
}

SoundChannel_t playSoundFadeInTimed(ResourceKey_t soundKey, int loops, int fadeDuration_ms, int playDuration_ms)
{
  auto* chunk = findChunk(soundKey);
  if(chunk == nullptr) return NULL_CHANNEL;
  SoundChannel_t channel = Mix_FadeInChannelTimed(-1, chunk, loops, fadeDuration_ms, playDuration_ms);
  if(channel == -1) return onPlayError();
  assert(channelPlayback[channel] == nullResourceKey);
  channelPlayback[channel] = soundKey;
  return channel;
}

void stopChannel(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  Mix_HaltChannel(channel);
}

void stopChannelTimed(SoundChannel_t channel, int durationUntilStop_ms)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  Mix_ExpireChannel(channel, durationUntilStop_ms);
}

void stopChannelFadeOut(SoundChannel_t channel, int fadeDuration_ms)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  Mix_FadeOutChannel(channel, fadeDuration_ms);
}

void pauseChannel(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  Mix_Pause(channel);
}

void resumeChannel(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  Mix_Resume(channel);
}

bool isChannelPlaying(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return false;
  if(channel == ALL_CHANNELS) return false;
  assert(0 <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  return Mix_Playing(channel) == 1;
}

bool isChannelPaused(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return false;
  if(channel == ALL_CHANNELS) return false;
  assert(0 <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  return Mix_Paused(channel) == 1;
}

void setChannelVolume(SoundChannel_t channel, int volume)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  int vol = std::clamp(volume, MIN_VOLUME, MAX_VOLUME);
  channelVolume[channel] = Mix_Volume(channel, vol); 
}

int getChannelVolume(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return 0;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  return channelVolume[channel];
}

} // namespace sfx
} // namespace pxr
