#include <cassert>
#include "pxr_mathutil.h"
#include "pxr_hud.h"
#include "menu_scene.h"
#include "play_scene.h"

MenuScene::Button::Button() :
  _state{State::UNINITIALIZED},
  _position{},
  _text{},
  _uidLabel{0},
  _onPress{nullptr},
  _menu{nullptr}
{}
  
MenuScene::Button::~Button()
{
  _onPress = nullptr;    // button does not own mem.
  _menu = nullptr;
}

void MenuScene::Button::initialize(
  Vector2i position,
  std::string text,
  Callback_t onPress,
  MenuScene* menu)
{
  assert(_state == State::UNINITIALIZED);

  _position = position;
  _text = text;
  _onPress = onPress;
  _menu = menu;

  _uidLabel = _menu->getHUD()->addLabel(std::make_unique<HUD::TextLabel>(
    _position,
    MenuScene::buttonIdleColor,
    0.f, 
    HUD::IMMORTAL_LIFETIME,
    _text,
    false,
    _menu->getMenuFontKey() 
  ));

  _state = State::IDLE;
}

void MenuScene::Button::uninitialize()
{
  _menu->getHUD()->removeLabel(_uidLabel);
  _state = State::UNINITIALIZED;
}

void MenuScene::Button::onIdle()
{
  _menu->getHUD()->setLabelColor(_uidLabel, MenuScene::buttonIdleColor);
  _state = State::IDLE;
}

void MenuScene::Button::onHover()
{
  _menu->getHUD()->setLabelColor(_uidLabel, MenuScene::buttonHoverColor);
  _state = State::HOVERED;
}

void MenuScene::Button::onPress()
{
  _menu->getHUD()->setLabelColor(_uidLabel, MenuScene::buttonPressedColor);
  (_menu->*_onPress)();
  _state = State::PRESSED;
}

MenuScene::SnakeAnimation::SnakeAnimation() :
  _basePosition{},
  _snake{},
  _direction{},
  _deltaRow{0},
  _stepClock_s{0.f},
  _sk{nullptr}
{}

void MenuScene::SnakeAnimation::initialize(Vector2i basePosition, Snake::Direction direction, Snake* sk)
{
  _basePosition = basePosition;
  _direction = direction;
  _sk = sk;

  if(direction == Snake::NORTH) 
    _deltaRow = 1;
  else if(direction == Snake::SOUTH) 
    _deltaRow = -1;
  else
    assert(0);

  reset();

  _snake[HEAD_BLOCK]._spriteid = Snake::smoothSnakeHeadBlockTree[direction];
  _snake[TAIL_BLOCK]._spriteid = Snake::snakeTailBlockTree[direction];

  for(int block{1}; block < snakeLength - 1; ++block)
    _snake[block]._spriteid = Snake::smoothSnakeBodyBlockTree[direction];
}

MenuScene::SnakeAnimation::~SnakeAnimation()
{
  _sk = nullptr; // explicit - does not own.
}

void MenuScene::SnakeAnimation::update(float dt)
{
  _stepClock_s += dt;
  if(_stepClock_s > Snake::stepPeriod_s){
    moveSnake();
    _stepClock_s = 0.f;
  }
}

void MenuScene::SnakeAnimation::moveSnake()
{
  for(auto& block : _snake){
    block._row += _deltaRow;
    if(block._row >= numRows) 
      block._row = 0;
    else if(block._row < 0) 
      block._row = numRows - 1;
  }
}

void MenuScene::SnakeAnimation::draw(gfx::ScreenID_t screenID)
{
  assert(_sk != nullptr);

  for(int block {HEAD_BLOCK}; block < snakeLength; ++block){
    Vector2i position {
      _basePosition._x,
      _basePosition._y + (_snake[block]._row * Snake::blockSize_rx)
    };
    gfx::drawSprite(
      position,
      _sk->getSpritesheetKey(Snake::SSID_SNAKES),
      _snake[block]._spriteid + (_sk->getSnakeHero() * Snake::SID_SNAKE_OFFSET),
      screenID
    );
  }
}

void MenuScene::SnakeAnimation::reset()
{
  _stepClock_s = 0.f;
  for(int i{0}; i < snakeLength; ++i){
    _snake[i]._row = (numRows / 2) - (_deltaRow * i);
    assert(0 <= _snake[i]._row && _snake[i]._row < numRows);
  }
}

MenuScene::MenuScene(pxr::Game* owner) :
  Scene(owner),
  _sk{nullptr},
  _hud{nullptr},
  _buttons{},
  _hoveredButtonID{0},
  _northwardSnake{},
  _southwardSnake{}
{}

bool MenuScene::onInit()
{
  _sk = static_cast<Snake*>(_owner);
  _hud = _sk->getHUD();
  initSnakeAnimations();
  return true;
}

void MenuScene::onEnter()
{
  buildMenu();
  startDisplay();
  drawBackground();
  _northwardSnake.reset();
  _southwardSnake.reset();
  sfx::playMusic(_sk->getMusicSequence(Snake::MUSIC_SEQUENCE_MENU));
}

void MenuScene::onUpdate(double now, float dt)
{
  handleInput();
  updateDisplay(dt);
  _southwardSnake.update(dt);
  _northwardSnake.update(dt);
  _hud->onUpdate(dt);
}

void MenuScene::onDraw(double now, float dt, const std::vector<gfx::ScreenID_t>& screens)
{
  gfx::clearScreenTransparent(screens[Snake::SCREEN_STAGE]);
  _southwardSnake.draw(screens[Snake::SCREEN_STAGE]);
  _northwardSnake.draw(screens[Snake::SCREEN_STAGE]);
  _hud->onDraw(screens[Snake::SCREEN_STAGE]);
}

void MenuScene::onExit()
{
  destroyMenu();
  destroyDisplay();
  sfx::stopMusic();
}

void MenuScene::handleInput()
{
  bool ukey {false}, dkey{false}, ekey_pressed{false}, ekey_released{false}; 
  if(pxr::input::isKeyPressed(Snake::menuUpKey)) ukey = true;
  if(pxr::input::isKeyPressed(Snake::menuDownKey)) dkey = true;
  if(pxr::input::isKeyPressed(Snake::menuPressKey)) ekey_pressed = true;
  if(pxr::input::isKeyReleased(Snake::menuPressKey)) ekey_released = true;

  if(ekey_pressed && ekey_released) // should never happen.
    return;

  if(ekey_pressed){
    _buttons[_hoveredButtonID].onPress();
    sfx::playSound(_sk->getSoundEffectKey(Snake::SFX_CLICK));
    return;
  }
  if(ekey_released){
    _buttons[_hoveredButtonID].onHover();
    return;
  }

  // don't allow changing the hovered button while pressed.
  if(_buttons[_hoveredButtonID].getState() == Button::State::PRESSED)
    return;

  int shift {0};
  if(ukey && dkey) shift = 0;
  else if(ukey) shift = -1;
  else if(dkey) shift = +1;

  if(shift){
    _buttons[_hoveredButtonID].onIdle();
    _hoveredButtonID = pxr::wrap<int>(_hoveredButtonID + shift, BID_PLAY, BID_SNAKE);
    _buttons[_hoveredButtonID].onHover();
    sfx::playSound(_sk->getSoundEffectKey(Snake::SFX_CLICK));
  }
}

void MenuScene::onPlayButtonPressed()
{
  _sk->switchScene(PlayScene::name);
}

void MenuScene::onSnakeButtonPressed()
{
  _sk->nextSnakeHero();
  addSnakeNameLabel(true);
}

void MenuScene::updateDisplay(float dt)
{
  _displayClock_s += dt;
  if(_displayClock_s > Snake::menuDisplaySwapInterval_s){
    _currentDisplayID = pxr::wrap<int>(_currentDisplayID + 1, DID_RULES, DID_SPEED);
    switch(_currentDisplayID){
      case DID_RULES:  {populateRulesDisplay(); break;}
      case DID_SCORES: {populateScoresDisplay(); break;}
      case DID_COMBOS: {populateCombosDisplay(); break;}
      case DID_SPEED:  {populateSpeedDisplay(); break;}
      default:         assert(0);
    }
    _displayClock_s = 0.f;
  }
}

void MenuScene::onHiscoresButtonPressed()
{
}

void MenuScene::populateRulesDisplay()
{
  static constexpr int rulesTextLabelCount {5};
  float delays_s[rulesTextLabelCount];
  for(int i {0}; i < rulesTextLabelCount; ++i)
    delays_s[i] = i * (Snake::menuDisplayDrawInterval_s / rulesTextLabelCount);

  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{83, 89},
    Snake::menuHeaderColor,
    delays_s[0],
    Snake::menuDisplayLabelLifetime_s,
    "RULES",
    true,
    getMenuFontKey() 
  )));
  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{20, 74},
    Snake::menuTextColor,
    delays_s[1],
    Snake::menuDisplayLabelLifetime_s,
    "Eat nuggets to gain score.",
    true,
    getMenuFontKey() 
  )));
  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{46, 62},
    Snake::menuTextColor,
    delays_s[2],
    Snake::menuDisplayLabelLifetime_s,
    "Don't eat yourself.",
    true,
    getMenuFontKey() 
  )));
  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{32, 51},
    Snake::menuTextColor,
    delays_s[3],
    Snake::menuDisplayLabelLifetime_s,
    "Bonus score for speed,",
    true,
    getMenuFontKey() 
  )));
  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{47, 39},
    Snake::menuTextColor,
    delays_s[4],
    Snake::menuDisplayLabelLifetime_s,
    "and nugget combos.",
    true,
    getMenuFontKey() 
  )));
}

void MenuScene::populateScoresDisplay()
{
  static constexpr int scoresTextLabelCount {16};
  float delays_s[scoresTextLabelCount];
  for(int i {0}; i < scoresTextLabelCount; ++i)
    delays_s[i] = i * (Snake::menuDisplayDrawInterval_s / scoresTextLabelCount);

  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{52, 89},
    Snake::menuHeaderColor,
    delays_s[0],
    Snake::menuDisplayLabelLifetime_s,
    "NUGGET",
    true,
    getMenuFontKey() 
  )));
  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{113, 89},
    Snake::menuHeaderColor,
    delays_s[1],
    Snake::menuDisplayLabelLifetime_s,
    "SCORE",
    true,
    getMenuFontKey() 
  )));

  static constexpr int delayOffset {2};

  for(int i {0}; i < Snake::nuggetClassCount; ++i){
    _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::BitmapLabel>(
      Vector2i{51, 80 - (i * 9)},
      Snake::menuTextColor,
      delays_s[delayOffset + (i * 2)],
      Snake::menuDisplayLabelLifetime_s,
      _sk->getSpritesheetKey(Snake::SSID_NUGGETS),
      Snake::SID_NUGGET_GOLD + i
    )));
    _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
      Vector2i{57, 79 - (i * 9)},
      Snake::menuTextColor,
      delays_s[delayOffset + (i * 2)],
      Snake::menuDisplayLabelLifetime_s,
      std::string{Snake::nuggetClasses[Snake::NUGGET_GOLD + i]._name},
      true,
      getMenuFontKey() 
    )));
    _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
      Vector2i{129, 79 - (i * 9)},
      Snake::menuTextColor,
      delays_s[delayOffset + ((i * 2) + 1)],
      Snake::menuDisplayLabelLifetime_s,
      std::to_string(Snake::nuggetClasses[Snake::NUGGET_GOLD + i]._score),
      true,
      getMenuFontKey() 
    )));
  }
}

void MenuScene::populateCombosDisplay()
{
  static constexpr int combosTextLabelCount {5};
  float delays_s[combosTextLabelCount];
  for(int i {0}; i < combosTextLabelCount; ++i)
    delays_s[i] = i * (Snake::menuDisplayDrawInterval_s / combosTextLabelCount);

  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{57, 89},
    Snake::menuHeaderColor,
    delays_s[0],
    Snake::menuDisplayLabelLifetime_s,
    "NUGGET COMBOS",
    true,
    getMenuFontKey() 
  )));

  static constexpr int delayOffset {1};

  static constexpr int comboCount {3};
  static constexpr std::array<Snake::NuggetSpriteID, comboCount> sids {{
    Snake::SID_NUGGET_ANY_X3,
    Snake::SID_NUGGET_ANY_X6,
    Snake::SID_NUGGET_SET
  }};
  static constexpr std::array<const char*, comboCount> strs {{
    "X3 same, X3 score",
    "X6 same, X6 score",
    "in order, x9 score"
  }};

  for(int i {0}; i < comboCount; ++i){
    _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::BitmapLabel>(
      Vector2i{23, 74 - (i * 11)},
      Snake::menuTextColor,
      delays_s[delayOffset + i],
      Snake::menuDisplayLabelLifetime_s,
      _sk->getSpritesheetKey(Snake::SSID_NUGGETS),
      sids[i]
    )));
    _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
      Vector2i{65, 74 - (i * 11)},
      Snake::menuTextColor,
      delays_s[delayOffset + i],
      Snake::menuDisplayLabelLifetime_s,
      strs[i],
      true,
      getMenuFontKey() 
    )));
  }

  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{37, 36},
    Snake::menuTextColor,
    delays_s[combosTextLabelCount - 1],
    Snake::menuDisplayLabelLifetime_s,
    "Bonus on last nugget",
    true,
    getMenuFontKey() 
  )));
}

void MenuScene::populateSpeedDisplay()
{
  static constexpr int speedTextLabelCount {10};
  float delays_s[speedTextLabelCount];
  for(int i {0}; i < speedTextLabelCount; ++i)
    delays_s[i] = i * (Snake::menuDisplayDrawInterval_s / speedTextLabelCount);

  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{65, 89},
    Snake::menuHeaderColor,
    delays_s[0],
    Snake::menuDisplayLabelLifetime_s,
    "SPEED BONUS",
    true,
    getMenuFontKey() 
  )));

  static constexpr int delayOffset {1};

  Vector2i bitmapPosition {25, 77};
  Vector2i textPosition {56, 76};
  for(int i{0}; i < Snake::speedBonusCount; ++i){
    _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::BitmapLabel>(
      Vector2i{bitmapPosition._x, bitmapPosition._y - ((i % 4) * 11)},
      Snake::menuTextColor,
      delays_s[delayOffset + i],
      Snake::menuDisplayLabelLifetime_s,
      _sk->getSpritesheetKey(Snake::SSID_NUGGETS),
      Snake::SID_NUGGET_ANY_X2 + i
    )));
    _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
      Vector2i{textPosition._x, textPosition._y - ((i % 4) * 11)},
      Snake::menuTextColor,
      delays_s[delayOffset + i],
      Snake::menuDisplayLabelLifetime_s,
      std::string{"+"} + std::to_string(static_cast<int>(Snake::speedBonusTable[i] * 100)) + "%",
      true,
      getMenuFontKey() 
    )));
    if(i == 3){
      bitmapPosition._x += 66;
      textPosition._x += 86;
    }
  }

  static constexpr int speedBarState {6};

  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::BitmapLabel>(
    Vector2i{29, 30},
    Snake::menuTextColor,
    delays_s[speedTextLabelCount - 1],
    Snake::menuDisplayLabelLifetime_s,
    _sk->getSpritesheetKey(Snake::SSID_SPEED_BAR),
    speedBarState
  )));
  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{65, 30},
    Snake::menuTextColor,
    delays_s[speedTextLabelCount - 1],
    Snake::menuDisplayLabelLifetime_s,
    "2 second cooldown",
    true,
    getMenuFontKey() 
  )));

}

void MenuScene::buildMenu()
{
  _buttons[BID_PLAY].initialize(
    Vector2i{87, 143},
    "PLAY",
    &MenuScene::onPlayButtonPressed,
    this
  );
  _buttons[BID_HISCORES].initialize(
    Vector2i{77, 132},
    "HISCORES",
    &MenuScene::onHiscoresButtonPressed,
    this
  );
  _buttons[BID_SNAKE].initialize(
    Vector2i{82, 121},
    "SNAKE",
    &MenuScene::onSnakeButtonPressed,
    this
  );
  _buttons[BID_PLAY].onHover();
  _hoveredButtonID = BID_PLAY;
  addSnakeNameLabel(false);
}

void MenuScene::startDisplay()
{
  _currentDisplayID = DID_RULES;
  _displayClock_s = 0.f;
  populateRulesDisplay();
}

void MenuScene::initSnakeAnimations()
{
  _southwardSnake.initialize(
    Vector2i{5, 20},
    Snake::Direction::SOUTH,
    _sk
  );
  _northwardSnake.initialize(
    Vector2i{191, 20},
    Snake::Direction::NORTH,
    _sk
  );
}

void MenuScene::destroyMenu()
{
  for(auto& button : _buttons)
    button.uninitialize();

  _hud->removeLabel(_uidSnakeNameLabel);
}

void MenuScene::destroyDisplay()
{
  for(auto& uid : _uidDisplayLabels)
    _hud->removeLabel(uid);

  _uidDisplayLabels.clear();
}

void MenuScene::addSnakeNameLabel(bool removeFirst)
{
  if(removeFirst)
    _hud->removeLabel(_uidSnakeNameLabel);

  _uidSnakeNameLabel = _hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{61, 105},
    Snake::snakeColors[_sk->getSnakeHero()],
    0.f, 
    HUD::IMMORTAL_LIFETIME,
    Snake::snakeNames[_sk->getSnakeHero()],
    false,
    getMenuFontKey() 
  ));
}

void MenuScene::drawBackground()
{
  gfx::drawSprite(
    Vector2f{0.f, 0.f},
    _sk->getSpritesheetKey(Snake::SSID_MENU_BACKGROUND),
    0,
    _sk->getScreenID(Snake::SCREEN_BACKGROUND)
  );
}
