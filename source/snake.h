#ifndef _SNAKE_H_
#define _SNAKE_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
//  _______ __                           __   __    __                                          //
// |_     _|  |_.-----.----.-----.---.-.|  |_|  |  {00}                                         //
//  _|   |_|   _|-- __|  __|  _  |  _  ||   _|  |  \__/                                         //
// |_______|____|_____|____|_____|___._||____|__|  |^|                                          //
//  ______________________________________________/ /                                           //
// /  _____________________________________________/                                            //
// \_______ \                                                                                   //
//         \|                                                                                   //
//                                                                                              //
// FILE: Snake.h                                                                                //
// AUTHOR: Ian Murfin - github.com/pixrex                                                       //
//                                                                                              //
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include "pxr_game.h"
#include "pxr_gfx.h"
#include "pxr_sfx.h"
#include "pxr_vec.h"
#include "pxr_input.h"
#include "pxr_hud.h"

using namespace pxr;

class Snake final : public pxr::Game
{
public:
  static constexpr const char* name {"snake"};
  static constexpr int versionMajor {0};
  static constexpr int versionMinor {1};

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // CONFIGURATION
  ////////////////////////////////////////////////////////////////////////////////////////////////

  static constexpr Vector2i worldSize_rx       {200, 200};
  static constexpr Vector2i boardSize          {40, 36};
  static constexpr int      blockSize_rx       {4};

  static constexpr Vector2i boardPosition {
    (worldSize_rx._x - (boardSize._x * blockSize_rx)) / 2,
    (worldSize_rx._x - (boardSize._x * blockSize_rx)) / 2
  };

  static constexpr int      boardMarginLoX   {boardPosition._x};
  static constexpr int      boardMarginHiX   {boardPosition._x + (boardSize._x * blockSize_rx)};
  static constexpr int      boardMarginLoY   {boardPosition._y};
  static constexpr int      boardMarginHiY   {boardPosition._y + (boardSize._y * blockSize_rx)};

  static constexpr int      maxSnakeLength         {400};
  static constexpr int      babySnakeLength        {4};
  static constexpr float    stepFrequency_hz       {10.f};
  static constexpr float    stepPeriod_s           {1.f / stepFrequency_hz};
  static constexpr float    speedBonusCooldown_s   {5.f};
  static constexpr int      same3ComboBonus        {3};
  static constexpr int      same6ComboBonus        {6};
  static constexpr int      orderComboBonus        {9};

  static constexpr int      snakeHeadSpawnCol  {(boardSize._x / 2) - (babySnakeLength / 2)};
  static constexpr int      snakeHeadSpawnRow  {(boardSize._y / 2)};

  static constexpr int      maxNuggetsInWorld  {20};
  static constexpr int      growthsPerNugget   {1};

  static constexpr float    hudFlashPeriod           {1.f};
  static constexpr float    hudPhaseInPeriod         {0.05f};
  static constexpr float    scorePopupLifetime_s     {1.f};
  static constexpr float    scorePopupOffset         {blockSize_rx * 2};
  static constexpr int      numSpeedBarStates        {11};
  static constexpr float    speedBarStateTimeDelta_s {speedBonusCooldown_s / (numSpeedBarStates - 1)};

  static constexpr float    menuDisplaySwapInterval_s   {15.f};
  static constexpr float    menuDisplayDrawInterval_s   {10.f};
  static constexpr float    menuDisplayLabelLifetime_s  {14.f};

  static constexpr gfx::Color4u scorePopupColor   {127, 52, 0, 255};
  static constexpr gfx::Color4u menuHeaderColor   {219, 41, 0, 255};
  static constexpr gfx::Color4u menuTextColor     {gfx::colors::black};

  static constexpr int longestPossibleCombo       {7};

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // CONTROLS       
  ////////////////////////////////////////////////////////////////////////////////////////////////

  static constexpr pxr::input::KeyCode moveLeftKey  {pxr::input::KEY_LEFT  };
  static constexpr pxr::input::KeyCode moveRightKey {pxr::input::KEY_RIGHT };
  static constexpr pxr::input::KeyCode moveUpKey    {pxr::input::KEY_UP    };
  static constexpr pxr::input::KeyCode moveDownKey  {pxr::input::KEY_DOWN  };

  static constexpr pxr::input::KeyCode smoothToggle {pxr::input::KEY_s     };

  static constexpr pxr::input::KeyCode menuUpKey     {pxr::input::KEY_UP   };
  static constexpr pxr::input::KeyCode menuDownKey   {pxr::input::KEY_DOWN };
  static constexpr pxr::input::KeyCode menuPressKey  {pxr::input::KEY_ENTER};

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // FONTS
  ////////////////////////////////////////////////////////////////////////////////////////////////

  enum FontID
  {
    FID_KONGTEXT, 
    FID_COUNT
  };

  static constexpr std::array<gfx::ResourceName_t, FID_COUNT> fontNames {{
    "kongtext"
  }};

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // SNAKE BLOCKS 
  ////////////////////////////////////////////////////////////////////////////////////////////////

  //
  // Each snake block can exist in 1 of 24 possible states which depend on the arrangement of its
  // neighbouring blocks and on the direction to the head. These states are render states which
  // control how to draw the snake.
  //
  // A block can have either 1 or 2 neighbours. These neighbours can be located in 4 relative
  // positions: north, east, south, west. Each neighbouring block can be either in the direction
  // of (closer to) the head or the tail, which adds a direction property to blocks.
  //
  // There are 6 possible configurations of 2 neighbours:
  //
  // +------------------------------------------------------------------+
  // |  description              pattern      equivilent_to             |
  // +------------------------------------------------------------------+
  // |                           N                                      |
  // |  north-this-south         T            south-this-north          |
  // |                           S                                      |
  // |                                                                  |
  // |  west-this-east         W T E          east-this-west            |
  // |                                                                  |
  // |                           N                                      |
  // |  north-this-east          T E          east-this-north           |
  // |                                                                  |
  // |                           N                                      |
  // |  north-this-west        W T            west-this-north           |
  // |                                                                  |
  // |                         W T                                      |
  // |  south-this-west          S            west-this-south           |
  // |                                                                  |
  // |                                                                  |
  // |  south-this-east          T E          east-this-south           |
  // |                           S                                      |
  // +------------------------------------------------------------------+
  //
  // and 4 possible configurations of 1 neighbour:
  //
  // +------------------------------------------------------------------+
  // | description              pattern      equivilent_to              |
  // +------------------------------------------------------------------+
  // | this-west                T W          west-this                  |
  // |                                                                  |
  // |                                                                  |
  // | this-east                E T          east-this                  |
  // |                                                                  |
  // |                          T                                       |
  // | this-south               S            south-this                 |
  // |                                                                  |
  // |                          N                                       |
  // | this-north               T            north-this                 |
  // +------------------------------------------------------------------+
  //
  // Thus we have 10 possible states with 10 equivilencies where the equivilent versions of
  // each state are the same state but in the opposite direction, e.g. north-this-south could
  // mean the north neighbour is closer to the head, and south-this-north would then mean the 
  // south neighbour is closer to the head. You could also define the opposite, the choice is 
  // arbitrary.
  //
  // The 1 neighbour configurations correspond to either head or tail blocks depending on 
  // direction. So this-west could mean this is the tail and to the west is the head, and 
  // west-this could mean this is the head and west is the tail. Again you could define the
  // opposite.
  //
  // The IDs defined in this enumeration are used to index into the snake spritesheet to select
  // the sprite for a given block state. Thus the order of the sprites in the spritesheet must
  // match the order defined by the values in this enumeration.
  //
  enum SnakeBlockSpriteID
  {
    SID_NULL = -1,

    // BODY BLOCKS 

    SID_HEAD_NORTH_THIS_SOUTH_TAIL = 0,
    SID_HEAD_SOUTH_THIS_NORTH_TAIL = 1,
    SID_HEAD_WEST_THIS_EAST_TAIL   = 2,
    SID_HEAD_EAST_THIS_WEST_TAIL   = 3,
    SID_HEAD_NORTH_THIS_EAST_TAIL  = 4,
    SID_HEAD_EAST_THIS_NORTH_TAIL  = 4,
    SID_HEAD_NORTH_THIS_WEST_TAIL  = 4,
    SID_HEAD_WEST_THIS_NORTH_TAIL  = 4,
    SID_HEAD_SOUTH_THIS_WEST_TAIL  = 5,
    SID_HEAD_WEST_THIS_SOUTH_TAIL  = 5,
    SID_HEAD_SOUTH_THIS_EAST_TAIL  = 5,
    SID_HEAD_EAST_THIS_SOUTH_TAIL  = 5,

    // TAIL BLOCKS
    
    SID_HEAD_WEST_THIS  = 6,
    SID_HEAD_EAST_THIS  = 7,
    SID_HEAD_NORTH_THIS = 8,
    SID_HEAD_SOUTH_THIS = 9,

    // HEAD BLOCKS  

    SID_THIS_WEST_TAIL  = 10,
    SID_THIS_EAST_TAIL  = 11,
    SID_THIS_NORTH_TAIL = 12,
    SID_THIS_SOUTH_TAIL = 13, 

    //
    // TONGUE BLOCKS
    //
    SID_WESTWARD_TONGUE  = 14,
    SID_EASTWARD_TONGUE  = 15,
    SID_NORTHWARD_TONGUE = 16,
    SID_SOUTHWARD_TONGUE = 17,

    //
    // MISCELLANEOUS blocks
    //
    SID_BLOOD_BLOCK = 18,
    SID_EMPTY_BLOCK = 19
  };

  //
  // The offset between the sprites for each hero snake. So for example if Montezuma's sprites
  // start at spriteid = 0, then the next snakes sprites will start at spriteid = 0 + 
  // SID_SNAKE_OFFSET.
  //
  static constexpr int SID_SNAKE_OFFSET {20};

  enum Direction { NORTH, SOUTH, EAST, WEST, DIRECTION_COUNT };

  //
  // Defines a tree which maps all possible neighbour configurations for body blocks (blocks
  // with 2 neighbours) to snake spritesheet sprite ids.
  //
  // The tree has the structure,
  //                            
  //                          root
  //                            |
  //       +-------------+------+------+-------------+
  //       |             |             |             |
  //       N             S             E             W          [ head neighbour ]
  //       |             |             |             |
  //    +--+--+--+    +--+--+--+    +--+--+--+    +--+--+--+
  //    |  |  |  |    |  |  |  |    |  |  |  |    |  |  |  |
  //    N  S  E  W    N  S  E  W    N  S  E  W    N  S  E  W    [ tail neighbour ]
  //    :  :  :  :    :  :  :  :    :  :  :  :    :  :  :  :
  //    X  s  s  s    s  X  s  s    s  s  X  s    s  s  s  X   <-- s = placeholder for sprite id,
  //       ^                              ^                         (not the same in every node)
  //       |                              |
  //   so this path would be for,        the X's indicate invalid
  //    SID_HEAD_NORTH_THIS_SOUTH_TAIL     routes
  //
  // This tree can be indexed with the directions enum, thus to get the SID for a northward head
  // neighbour and a southward tail neighbour,
  //     sid = snakeBodyBlockTree[NORTH][SOUTH]
  //
  static constexpr std::array<std::array<SnakeBlockSpriteID, DIRECTION_COUNT>, DIRECTION_COUNT> snakeBodyBlockTree {{
    // NORTH [head]
    {
      SID_NULL, 
      SID_HEAD_NORTH_THIS_SOUTH_TAIL,
      SID_HEAD_NORTH_THIS_EAST_TAIL,
      SID_HEAD_NORTH_THIS_WEST_TAIL
    },
    // SOUTH [head]
    {
      SID_HEAD_SOUTH_THIS_NORTH_TAIL,
      SID_NULL,
      SID_HEAD_SOUTH_THIS_EAST_TAIL,
      SID_HEAD_SOUTH_THIS_WEST_TAIL
    },
    // EAST  [head]
    {
      SID_HEAD_EAST_THIS_NORTH_TAIL,
      SID_HEAD_EAST_THIS_SOUTH_TAIL,
      SID_NULL,
      SID_HEAD_EAST_THIS_WEST_TAIL
    },
    // WEST  [head]
    {
    SID_HEAD_WEST_THIS_NORTH_TAIL,
    SID_HEAD_WEST_THIS_SOUTH_TAIL,
    SID_HEAD_WEST_THIS_EAST_TAIL,
    SID_NULL
    }
  }};

  //
  // Maps the neighbour configurations for blocks with a single head neighbour but no tail
  // neighbour to snake spritesheet sprite ids. This is not really much of a tree however as
  // it only has a single layer.
  //
  static constexpr std::array<SnakeBlockSpriteID, DIRECTION_COUNT> snakeHeadBlockTree {
    SID_THIS_NORTH_TAIL,
    SID_THIS_SOUTH_TAIL,
    SID_THIS_EAST_TAIL,
    SID_THIS_WEST_TAIL
  };

  //
  // Maps the neighbour configurations for blocks with a single tail neighbour but no head
  // neighbour to snake spritesheet sprite ids. This is not really much of a tree however as
  // it only has a single layer.
  //
  static constexpr std::array<SnakeBlockSpriteID, DIRECTION_COUNT> snakeTailBlockTree {
    SID_HEAD_NORTH_THIS,
    SID_HEAD_SOUTH_THIS,
    SID_HEAD_EAST_THIS,
    SID_HEAD_WEST_THIS
  };

  //
  // Used when drawing smooth moving snakes. For smooth movement the sprite used depends only on
  // the direction of a block's movement not on the block's neighbour configuration.
  //
  static constexpr std::array<SnakeBlockSpriteID, DIRECTION_COUNT> smoothSnakeBodyBlockTree {
    SID_HEAD_NORTH_THIS_SOUTH_TAIL,
    SID_HEAD_SOUTH_THIS_NORTH_TAIL,
    SID_HEAD_EAST_THIS_WEST_TAIL,
    SID_HEAD_WEST_THIS_EAST_TAIL
  };

  //
  // Used when drawing the head for smooth moving snakes.
  //
  static constexpr std::array<SnakeBlockSpriteID, DIRECTION_COUNT> smoothSnakeHeadBlockTree {
    SID_THIS_SOUTH_TAIL,
    SID_THIS_NORTH_TAIL,
    SID_THIS_WEST_TAIL,
    SID_THIS_EAST_TAIL
  };
  
  ////////////////////////////////////////////////////////////////////////////////////////////////
  // SPRITESHEETS
  ////////////////////////////////////////////////////////////////////////////////////////////////

  //
  // Spritesheet gfx::ResourceKeys are assigned at runtime and so cannot be cleanly made compile
  // time constants. Thus these IDs must not be used raw but instead be used with a call to,
  //    getSpritesheetKey(SpritesheetID sheetID)
  //
  enum SpritesheetID
  {
    SSID_SNAKES,
    SSID_NUGGETS,
    SSID_PLAY_BACKGROUND,
    SSID_MENU_BACKGROUND,
    SSID_FOREGROUND,
    SSID_SPEED_BAR,
    SSID_COUNT
  };

  static constexpr std::array<gfx::ResourceName_t, SSID_COUNT> spritesheetNames {
    "snakes",
    "nuggets",
    "play_background",
    "menu_background",
    "foreground",
    "speedbar"
  };

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // SOUNDS        
  ////////////////////////////////////////////////////////////////////////////////////////////////

  enum SoundEffectID
  {
    SFX_SCORE_BEEP,
    SFX_CLICK,
    SFX_COUNT
  };

  static constexpr std::array<sfx::ResourceName_t, SFX_COUNT> soundEffectNames {
    "scorebeep",
    "click"
  };

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // MUSIC         
  ////////////////////////////////////////////////////////////////////////////////////////////////

  enum MusicLoopID
  {
    MUSIC_JUNGLE_DRUMS_0,
    MUSIC_JUNGLE_DRUMS_1,
    MUSIC_JUNGLE_DRUMS_2,
    MUSIC_JUNGLE_DRUMS_3,
    MUSIC_MENU_INTRO,
    MUSIC_MENU_AMBIENCE,
    MUSIC_COUNT
  };

  static constexpr std::array<sfx::ResourceName_t, MUSIC_COUNT> musicLoopNames {
    "jungle_drums_0",
    "jungle_drums_1",
    "jungle_drums_2",
    "jungle_drums_3",
    "menu_intro",
    "menu_ambience"
  };

  enum MusicSequenceID
  {
    MUSIC_SEQUENCE_JUNGLE,
    MUSIC_SEQUENCE_MENU,
    MUSIC_SEQUENCE_COUNT
  };

  struct MusicIDSequenceNode
  {
    MusicLoopID _musicID;
    float _fadeInDuration_s;
    float _playDuration_s;
    float _fadeOutDuration_s;
  };

  //using MusicIDSequence_t = std::vector<MusicIDSequenceNode>;
  using MusicIDSequence_t = std::array<MusicIDSequenceNode, 4>;

  static constexpr std::array<MusicIDSequence_t, MUSIC_SEQUENCE_COUNT> musicIDSequences {{
  //----------------------------------------------------------------------------------------------
  //    loopID                 fadeIn   playDuration   fadeOut
  //----------------------------------------------------------------------------------------------
    {{
       {{MUSIC_JUNGLE_DRUMS_0}, 1.f,    25.f,           1.f  },
       {{MUSIC_JUNGLE_DRUMS_1}, 1.f,    25.f,           1.f  },
       {{MUSIC_JUNGLE_DRUMS_2}, 1.f,    25.f,           1.f  },
       {{MUSIC_JUNGLE_DRUMS_3}, 1.f,    25.f,           1.f  }
    }},
    {{
       {{MUSIC_MENU_INTRO    }, 1.f,    24.f,           2.f  },
       {{MUSIC_MENU_AMBIENCE }, 1.f,    999999999999.f, 2.f  }
    }}
  }};

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // SNAKES        
  ////////////////////////////////////////////////////////////////////////////////////////////////

  //
  // The snakes availible to play as.
  //
  enum SnakeHero
  {
    SNAKE_ITZCOATL,
    SNAKE_MONTEZUMA_I,
    SNAKE_AXAYACTL,
    SNAKE_TIZOC,
    SNAKE_AHUITZOTL,
    SNAKE_MONTEZUMA_II,
    SNAKE_CUITLAHUAC,
    SNAKE_CUAUHTEMOC,
    SNAKE_COUNT
  };

  static constexpr std::array<const char*, SNAKE_COUNT> snakeNames {{
    "Itzcoatl",
    "Montezuma I",
    "Axayactl",
    "Tizoc",
    "Ahuitzotl",
    "Montezuma II",
    "Cuitlahuac",
    "Cuauhtemoc"
  }};

  static constexpr std::array<gfx::Color4u, SNAKE_COUNT> snakeColors {{
    {116, 100, 90 , 255},
    {219, 41 , 0  , 255},
    {112, 210, 188, 255},
    {88 , 241, 110, 255},
    {245, 207, 0  , 255},
    {224, 17 , 95 , 255},
    {245, 207, 0  , 255},
    {236, 236, 236, 255}
  }};

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // NUGGETS       
  ////////////////////////////////////////////////////////////////////////////////////////////////

  enum NuggetSpriteID
  {
    SID_NUGGET_GOLD,
    SID_NUGGET_SILVER,
    SID_NUGGET_OBSIDIAN,
    SID_NUGGET_RUBY,
    SID_NUGGET_JADE,
    SID_NUGGET_LAPIS,
    SID_NUGGET_AMETHYST,
    SID_NUGGET_SET,
    SID_NUGGET_ANY_X2,
    SID_NUGGET_ANY_X3,
    SID_NUGGET_ANY_X4,
    SID_NUGGET_ANY_X5,
    SID_NUGGET_ANY_X6,
    SID_NUGGET_ANY_X7,
    SID_NUGGET_ANY_X8,
    SID_NUGGET_ANY_X9
  };

  enum NuggetClassID
  {
    NUGGET_GOLD, 
    NUGGET_SILVER,
    NUGGET_OBSIDIAN,
    NUGGET_RUBY,
    NUGGET_JADE,
    NUGGET_LAPIS,
    NUGGET_AMETHYST
  };

  static constexpr int nuggetClassCount {7};

  struct NuggetClass
  {
    gfx::SpriteID_t _spriteid; 
    float _lifetime;
    int _spawnChance;
    int _score;
    const char* _name;
  };

  static constexpr std::array<NuggetClass, nuggetClassCount> nuggetClasses {{
  //----------------------------------------------------------------------------------------------
  // spriteid             lifetime   chance   score   name
  //----------------------------------------------------------------------------------------------
    {SID_NUGGET_GOLD    , 2.f,          10,   70,     "Gold"     },
    {SID_NUGGET_SILVER  , 3.f,          20,   60,     "Silver"   },
    {SID_NUGGET_OBSIDIAN, 4.f,          30,   50,     "Obsidian" },
    {SID_NUGGET_RUBY    , 5.f,          40,   40,     "Ruby"     },
    {SID_NUGGET_JADE    , 6.f,          50,   30,     "Jade"     },
    {SID_NUGGET_LAPIS   , 7.f,          70,   20,     "Lapis"    },
    {SID_NUGGET_AMETHYST, 8.f,         100,   10,     "Amethyst" }
  }};

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // SPEED BONUS TABLE
  ////////////////////////////////////////////////////////////////////////////////////////////////

  //
  // Score bonuses are percentage gains earned by eating nuggets in quick succession. This table
  // maps the number of nuggets you must eat in quick succession to earn specific score bonuses.
  //
  // Each element number in the table corresponds to the number of nuggets plus one you must eat 
  // to earn the percentage gain stored in that element. So index 0 stores the percentage gain 
  // for eating 1 nuggets in quick succession, i.e. eating a nugget and then a second nugget 
  // quickly afterwards (the first in quick succession).
  //
  // The percentages in the table are the percentages of the base score earned added to the base
  // score, so if the base score is 10, and you get a percentage gain of 2.5f == 250% then you
  // get a score of 10 + (2.5 * 10) = 35.
  //

  static constexpr int speedBonusCount {8};
  static constexpr std::array<float, speedBonusCount> speedBonusTable {{
  //----------------------------------------------------------------------------------------------
  // bonus
  //----------------------------------------------------------------------------------------------
    { 0.1f },
    { 0.2f },
    { 0.4f },
    { 0.8f },
    { 1.2f },
    { 1.6f },
    { 1.8f },
    { 2.0f }
  }};

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // GFX SCREENS       
  ////////////////////////////////////////////////////////////////////////////////////////////////

  //
  // Screens are created in the order they are defined in this enum, this means they will also
  // be drawn in this order, meaning the first defined is the bottom of the screen layers.
  //
  enum GFXScreenName
  {
    SCREEN_BACKGROUND,
    SCREEN_STAGE,
    SCREEN_FOREGROUND,
    SCREEN_COUNT
  };

public:
  Snake() = default;
  ~Snake() = default;

  bool onInit();
  void onShutdown();

  std::string getName() const {return name;}
  int getVersionMajor() const {return versionMajor;}
  int getVersionMinor() const {return versionMinor;}

  gfx::ResourceKey_t getSpritesheetKey(SpritesheetID sheetID);
  gfx::ResourceKey_t getFontKey(FontID fontID);
  sfx::ResourceKey_t getSoundEffectKey(SoundEffectID sfxID);
  sfx::ResourceKey_t getMusicLoopKey(MusicLoopID sfxID);
  gfx::ScreenID_t getScreenID(GFXScreenName screenName);

  sfx::MusicSequence_t getMusicSequence(MusicSequenceID sequenceID);

  SnakeHero getSnakeHero() const {return _snakeHero;}
  void nextSnakeHero();

  void addScore(int score) {_score += score;}
  int getScore() const {return _score;}
  int& getScoreReference() {return _score;}
  int& getNuggetsEatenReference(NuggetClassID classID){return _nuggetsEaten[classID];}
  void addNuggetEaten(NuggetClassID classID, int count);
  int getNuggetsEaten(NuggetClassID classID) const {return _nuggetsEaten[classID];}

  HUD* getHUD() {return _hud;}

private:
  void loadSpritesheets();
  void loadFonts();
  void loadSoundEffects();
  void loadMusicLoops();

private:
  HUD* _hud;

  std::array<gfx::ResourceKey_t, SSID_COUNT> _spritesheetKeys;
  std::array<gfx::ResourceKey_t, FID_COUNT> _fontKeys;
  std::array<sfx::ResourceKey_t, SFX_COUNT> _soundEffectKeys;
  std::array<sfx::ResourceKey_t, MUSIC_COUNT> _musicLoopKeys;

  SnakeHero _snakeHero;

  int _score;
  int _nuggetsEaten[nuggetClassCount];
};

#endif
