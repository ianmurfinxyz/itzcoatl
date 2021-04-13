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
  drawBackground();
}

void MenuScene::onUpdate(double now, float dt)
{
  handleInput();
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

void MenuScene::onHiscoresButtonPressed()
{

}

void MenuScene::onSnakeButtonPressed()
{
  _sk->nextSnakeHero();
  addSnakeNameLabel(true);
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

void MenuScene::destroyMenu()
{
  for(auto& button : _buttons)
    button.uninitialize();

  _hud->removeLabel(_uidSnakeNameLabel);
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
