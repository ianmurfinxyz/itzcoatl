#ifndef _MENU_SCENE_H_
#define _MENU_SCENE_H_

#include <string>
#include <array>
#include <vector>
#include "snake.h"

using namespace pxr;

class MenuScene final : public pxr::Scene
{
public:
  static constexpr const char* name {"menu_scene"};

public:

  MenuScene(pxr::Game* owner);
  ~MenuScene() = default;

  bool onInit();
  void onEnter();
  void onUpdate(double now, float dt);
  void onDraw(double now, float dt, const std::vector<gfx::ScreenID_t>& screens);
  void onExit();

  std::string getName() const {return name;}

  // convenience functions for use by buttons.
  HUD* getHUD() {return _sk->getHUD();}
  gfx::ResourceKey_t getMenuFontKey() {return _sk->getFontKey(Snake::FID_KONGTEXT);}

private:

  static constexpr gfx::Color4u buttonIdleColor {127, 52, 0, 255};
  static constexpr gfx::Color4u buttonHoverColor {219, 41, 0, 255};
  static constexpr gfx::Color4u buttonPressedColor {236, 236, 236, 255};

  class Button
  {
  public:
    using Callback_t = void (MenuScene::*)();

    enum class State
    {
      UNINITIALIZED,
      IDLE,
      HOVERED,
      PRESSED
    };

  public:
    Button();
    ~Button();

    void initialize(
      Vector2i position,
      std::string text,
      Callback_t onPress,
      MenuScene* menu
    );

    void uninitialize();

    void onIdle();
    void onHover();
    void onPress();

    State getState() const {return _state;}

  private:
    State _state;
    Vector2i _position;
    std::string _text;
    HUD::uid_t _uidLabel;
    Callback_t _onPress;
    MenuScene* _menu;
  };

private:
  void handleInput();

  void onPlayButtonPressed();
  void onHiscoresButtonPressed();
  void onSnakeButtonPressed();

  void updateDisplay(float dt);

  void populateRulesDisplay();
  void populateScoresDisplay();
  void populateCombosDisplay();
  void populateSpeedDisplay();

  void buildMenu();
  void startDisplay();
  void destroyMenu();
  void destroyDisplay();

  void addSnakeNameLabel(bool removeFirst);
  void drawBackground();

private:
  Snake* _sk;
  HUD* _hud;

  enum ButtonID
  {
    BID_PLAY,
    BID_HISCORES,
    BID_SNAKE,
    BID_COUNT
  };

  std::array<Button, BID_COUNT> _buttons;
  int _hoveredButtonID;

  enum DisplayID
  {
    DID_RULES,
    DID_SCORES,
    DID_COMBOS,
    DID_SPEED,
    DID_COUNT
  };

  int _currentDisplayID;
  float _displayClock_s;
  std::vector<HUD::uid_t> _uidDisplayLabels;

  HUD::uid_t _uidSnakeNameLabel;
};

#endif
