//----------------------------------------------------------------------------------------------//
//                  _                                                                           //
//                 | |                                                                          //
//  ___ _ __   __ _| | _____     __                                                             //
// / __| '_ \ / _` | |/ / _ \   {OO}                                                            //
// \__ \ | | | (_| |   <  __/   \__/                                                            //
// |___/_| |_|\__,_|_|\_\___|   |^|                                                             //
//  ____________________________/ /                                                             //
// /  ___________________________/                                                              //
//  \_______ \                                                                                  //
//          \|                                                                                  //
//                                                                                              //
// FILE: snake.cpp                                                                              //
// AUTHOR: Ian Murfin - github.com/pixrex                                                       //
//                                                                                              //
// CREATED: 6th Apr 2021                                                                        //
// UPDATED: 6th Apr 2021                                                                        //
//                                                                                              //
//----------------------------------------------------------------------------------------------//

#include "pxr_game.h"
#include "pxr_vec.h"

class Snake final : public pxr::Game
{
public:
  static constexpr const char* name {"snake"};
  static constexpr int versionMajor {0};
  static constexpr int versionMinor {1};

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // CONFIGURATION
  ////////////////////////////////////////////////////////////////////////////////////////////////

  static constexpr Vector2i worldSize_rx       {256, 256};
  static constexpr Vector2i boardSize          {60, 60};
  static constexpr int      blockSize_rx       {4};

  static constexpr int      maxSnakeLength     {100};
  static constexpr int      babySnakeLength    {3};
  static constexpr float    stepFrequency_hz   {10.f};
  static constexpr float    stepPeriod_s       {1.f/ stepFrequency};

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
  };

  //
  // The tongue block IDs are also indicies into the snake spritesheet to access the tongue
  // sprites for the snake. These IDs are seperate because the tongue is an addon to the snake
  // and depends only on the direction of travel. The direction in the enum names corresponds
  // to the snake's direction of travel.
  //
  enum SnakeTongueBlockSpriteID
  {
    SID_WESTWARD_TONGUE  = 14,
    SID_EASTWARD_TONGUE  = 15,
    SID_NORTHWARD_TONGUE = 16,
    SID_SOUTHWARD_TONGUE = 17
  };

  enum Direction { NORTH, SOUTH, EAST, WEST, DIRECTION_COUNT };

  //
  // Defines a tree which maps all possible neighbour configurations for body blocks (blocJJks 
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
  static constexpr std::array<std::array<SnakeBlockSpriteID, DIRECTION_COUNT>> snakeBodyBlockTree {
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
  };

  //
  // Maps the neighbour configurations for blocks with a single head neighbour but no tail
  // neighbour to snake spritesheet sprite ids. This is not really much of a tree however as
  // it only has a single layer.
  //
  static constexpr std::array<SnakeBlockSpriteID, DIRECTION_COUNT> snakeHeadBlockTree {
    SID_HEAD_NORTH_THIS,
    SID_HEAD_SOUTH_THIS,
    SID_HEAD_EAST_THIS,
    SID_HEAD_WEST_THIS
  };

  //
  // Maps the neighbour configurations for blocks with a single tail neighbour but no head
  // neighbour to snake spritesheet sprite ids. This is not really much of a tree however as
  // it only has a single layer.
  //
  static constexpr std::array<SnakeBlockSpriteID, DIRECTION_COUNT> snakeHeadBlockTree {
    SID_THIS_NORTH_TAIL,
    SID_THIS_SOUTH_TAIL,
    SID_THIS_EAST_TAIL,
    SID_THIS_WEST_TAIL
  };

public:
  bool onInit();
  void onShutdown();

  std::string getName() const {return name;}
  int getVersionMajor() const {return versionMajor;}
  int getVersionMinor() const {return versionMinor;}

private:
};

