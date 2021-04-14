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

MenuScene::MenuScene(pxr::Game* owner) :
  Scene(owner),
  _sk{nullptr},
  _hud{nullptr},
  _buttons{},
  _hoveredButtonID{0}
{}

bool MenuScene::onInit()
{
  _sk = static_cast<Snake*>(_owner);
  _hud = _sk->getHUD();
  return true;
}

void MenuScene::onEnter()
{
  buildMenu();
  startDisplay();
  drawBackground();
}

void MenuScene::onUpdate(double now, float dt)
{
  handleInput();
  updateDisplay(dt);
  _hud->onUpdate(dt);
}

void MenuScene::onDraw(double now, float dt, const std::vector<gfx::ScreenID_t>& screens)
{
  gfx::clearScreenTransparent(screens[Snake::SCREEN_STAGE]);
  _hud->onDraw(screens[Snake::SCREEN_STAGE]);
}

void MenuScene::onExit()
{
  destroyMenu();
  destroyDisplay();
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
      case DID_SCORES: {populateRulesDisplay(); break;}
      case DID_COMBOS: {populateRulesDisplay(); break;}
      case DID_SPEED:  {populateRulesDisplay(); break;}
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
  float delays_s[5];
  for(int i {0}; i < 5; ++i)
    delays_s[i] = i * (Snake::menuDisplayLabelLifetime_s / 5);

  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{83, 90},
    Snake::menuHeaderColor,
    delays_s[0],
    Snake::menuDisplayLabelLifetime_s,
    "RULES",
    true,
    getMenuFontKey() 
  )));
  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{20, 75},
    Snake::menuTextColor,
    delays_s[1],
    Snake::menuDisplayLabelLifetime_s,
    "Eat nuggets to gain score.",
    true,
    getMenuFontKey() 
  )));
  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{46, 63},
    Snake::menuTextColor,
    delays_s[2],
    Snake::menuDisplayLabelLifetime_s,
    "Don't eat yourself.",
    true,
    getMenuFontKey() 
  )));
  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{32, 52},
    Snake::menuTextColor,
    delays_s[3],
    Snake::menuDisplayLabelLifetime_s,
    "Bonus score for speed,",
    true,
    getMenuFontKey() 
  )));
  _uidDisplayLabels.push_back(_hud->addLabel(std::make_unique<HUD::TextLabel>(
    Vector2i{47, 40},
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
}

void MenuScene::populateCombosDisplay()
{
}

void MenuScene::populateSpeedDisplay()
{
}

void MenuScene::buildMenu()
{
  _buttons[BID_PLAY].initialize(
    Vector2i{87, 139},
    "PLAY",
    &MenuScene::onPlayButtonPressed,
    this
  );
  _buttons[BID_HISCORES].initialize(
    Vector2i{77, 128},
    "HISCORES",
    &MenuScene::onHiscoresButtonPressed,
    this
  );
  _buttons[BID_SNAKE].initialize(
    Vector2i{82, 117},
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
    Vector2i{67, 105},
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
