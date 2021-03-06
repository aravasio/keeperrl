/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _GUI_ELEM
#define _GUI_ELEM

#include "renderer.h"
#include "drag_and_drop.h"

class ViewObject;
class Clock;
class Options;
enum class SpellId;

class GuiElem {
  public:
  virtual void render(Renderer&) {}
  virtual bool onLeftClick(Vec2) { return false; }
  virtual bool onRightClick(Vec2) { return false; }
  virtual bool onMouseMove(Vec2) { return false;}
  virtual void onMouseGone() {}
  virtual void onMouseRelease(Vec2) {}
  virtual void onRefreshBounds() {}
  virtual bool onKeyPressed2(SDL_Keysym) { return false;}
  virtual bool onMouseWheel(Vec2 mousePos, bool up) { return false;}
  virtual optional<int> getPreferredWidth() { return none; }
  virtual optional<int> getPreferredHeight() { return none; }

  void setPreferredBounds(Vec2 origin);
  void setBounds(Rectangle);
  Rectangle getBounds();

  virtual ~GuiElem();

  private:
  Rectangle bounds;
};

class GuiFactory {
  public:
  GuiFactory(Renderer&, Clock*, Options*);
  void loadFreeImages(const string& path);
  void loadNonFreeImages(const string& path);

  vector<string> breakText(const string& text, int maxWidth);

  DragContainer& getDragContainer();
  void propagateEvent(const Event&, vector<GuiElem*>);

  static bool isShift(const SDL_Keysym&);
  static bool isAlt(const SDL_Keysym&);
  static bool isCtrl(const SDL_Keysym&);
  static bool keyEventEqual(const SDL_Keysym&, const SDL_Keysym&);

  SDL_Keysym getKey(SDL_Keycode);
  PGuiElem button(function<void()> fun, SDL_Keysym, bool capture = false);
  PGuiElem buttonChar(function<void()> fun, char, bool capture = false);
  PGuiElem button(function<void()> fun);
  PGuiElem reverseButton(function<void()> fun, vector<SDL_Keysym> = {}, bool capture = false);
  PGuiElem buttonRect(function<void(Rectangle buttonBounds)> fun, SDL_Keysym, bool capture = false);
  PGuiElem buttonRect(function<void(Rectangle buttonBounds)> fun);
  PGuiElem releaseButton(function<void()> fun);
  PGuiElem releaseButton(function<void(Rectangle buttonBounds)> fun);
  PGuiElem focusable(PGuiElem content, vector<SDL_Keysym> focusEvent,
      vector<SDL_Keysym> defocusEvent, bool& focused);
  PGuiElem mouseWheel(function<void(bool)>);
  PGuiElem keyHandler(function<void(SDL_Keysym)>, bool capture = false);
  PGuiElem keyHandler(function<void()>, vector<SDL_Keysym>, bool capture = false);
  PGuiElem stack(vector<PGuiElem>);
  PGuiElem stack(PGuiElem, PGuiElem);
  PGuiElem stack(PGuiElem, PGuiElem, PGuiElem);
  PGuiElem stack(PGuiElem, PGuiElem, PGuiElem, PGuiElem);
  PGuiElem external(GuiElem*);
  PGuiElem rectangle(Color color, optional<Color> borderColor = none);
  class ListBuilder {
    public:
    ListBuilder(GuiFactory&, int defaultSize = 0);
    ListBuilder& addElem(PGuiElem, int size = 0);
    ListBuilder& addSpace(int size = 0);
    ListBuilder& addElemAuto(PGuiElem);
    ListBuilder& addBackElemAuto(PGuiElem);
    ListBuilder& addBackElem(PGuiElem, int size = 0);
    PGuiElem buildVerticalList();
    PGuiElem buildHorizontalList();
    PGuiElem buildHorizontalListFit();
    int getSize() const;
    bool isEmpty() const;
    vector<PGuiElem>& getAllElems();
    void clear();

    private:
    GuiFactory& gui;
    vector<PGuiElem> elems;
    vector<int> sizes;
    int defaultSize = 0;
    int backElems = 0;
  };
  ListBuilder getListBuilder(int defaultSize = 0);
  PGuiElem verticalList(vector<PGuiElem>, int elemHeight, int numAlignBottom = 0);
  PGuiElem verticalList(vector<PGuiElem>, vector<int> elemHeight, int numAlignBottom = 0);
  PGuiElem verticalListFit(vector<PGuiElem>, double spacing);
  PGuiElem horizontalList(vector<PGuiElem>, int elemWidth, int numAlignRight = 0);
  PGuiElem horizontalList(vector<PGuiElem>, vector<int> elemWidth, int numAlignRight = 0);
  PGuiElem horizontalListFit(vector<PGuiElem>, double spacing = 0);
  PGuiElem verticalAspect(PGuiElem, double ratio);
  PGuiElem empty();
  PGuiElem preferredSize(int width, int height);
  PGuiElem setHeight(int height, PGuiElem);
  PGuiElem setWidth(int width, PGuiElem);
  enum MarginType { TOP, LEFT, RIGHT, BOTTOM};
  PGuiElem margin(PGuiElem top, PGuiElem rest, int height, MarginType);
  PGuiElem marginAuto(PGuiElem top, PGuiElem rest, MarginType);
  PGuiElem margin(PGuiElem top, PGuiElem rest, function<int(Rectangle)> width, MarginType type);
  PGuiElem maybeMargin(PGuiElem top, PGuiElem rest, int width, MarginType, function<bool(Rectangle)>);
  PGuiElem marginFit(PGuiElem top, PGuiElem rest, double height, MarginType);
  PGuiElem margins(PGuiElem content, int left, int top, int right, int bottom);
  PGuiElem margins(PGuiElem content, int all);
  PGuiElem leftMargin(int size, PGuiElem content);
  PGuiElem rightMargin(int size, PGuiElem content);
  PGuiElem topMargin(int size, PGuiElem content);
  PGuiElem bottomMargin(int size, PGuiElem content);
  PGuiElem label(const string&, Color = colors[ColorId::WHITE], char hotkey = 0);
  PGuiElem labelHighlight(const string&, Color = colors[ColorId::WHITE], char hotkey = 0);
  PGuiElem labelHighlightBlink(const string& s, Color, Color);
  PGuiElem label(const string&, int size, Color = colors[ColorId::WHITE]);
  PGuiElem label(const string&, function<Color()>, char hotkey = 0);
  PGuiElem labelFun(function<string()>, function<Color()>);
  PGuiElem labelFun(function<string()>, Color = colors[ColorId::WHITE]);
  PGuiElem labelMultiLine(const string&, int lineHeight, int size = Renderer::textSize,
      Color = colors[ColorId::WHITE]);
  PGuiElem centeredLabel(Renderer::CenterType, const string&, int size, Color = colors[ColorId::WHITE]);
  PGuiElem centeredLabel(Renderer::CenterType, const string&, Color = colors[ColorId::WHITE]);
  PGuiElem variableLabel(function<string()>,
      Renderer::CenterType = Renderer::NONE, int size = Renderer::textSize, Color = colors[ColorId::WHITE]);
  PGuiElem mainMenuLabel(const string&, double vPadding, Color = colors[ColorId::MAIN_MENU_ON]);
  PGuiElem mainMenuLabelBg(const string&, double vPadding, Color = colors[ColorId::MAIN_MENU_OFF]);
  PGuiElem labelUnicode(const string&, Color = colors[ColorId::WHITE], int size = Renderer::textSize,
      Renderer::FontId = Renderer::SYMBOL_FONT);
  PGuiElem labelUnicode(const string&, function<Color()>, int size = Renderer::textSize,
      Renderer::FontId = Renderer::SYMBOL_FONT);
  PGuiElem viewObject(const ViewObject&, double scale = 1, Color = colors[ColorId::WHITE]);
  PGuiElem viewObject(ViewId, double scale = 1, Color = colors[ColorId::WHITE]);
  PGuiElem asciiBackground(ViewId);
  PGuiElem translate(PGuiElem, Vec2, Rectangle newSize);
  PGuiElem centerHoriz(PGuiElem, int width = -1);
  PGuiElem onRenderedAction(function<void()>);
  PGuiElem mouseOverAction(function<void()> callback, function<void()> onLeaveCallback = nullptr);
  PGuiElem mouseHighlight(PGuiElem highlight, int myIndex, int* highlighted);
  PGuiElem mouseHighlightClick(PGuiElem highlight, int myIndex, int* highlighted);
  PGuiElem mouseHighlight2(PGuiElem highlight);
  PGuiElem mouseHighlightGameChoice(PGuiElem, optional<GameTypeChoice> my, optional<GameTypeChoice>& highlight);
  static int getHeldInitValue();
  PGuiElem scrollable(PGuiElem content, double* scrollPos = nullptr, int* held = nullptr);
  PGuiElem getScrollButton();
  PGuiElem conditional2(PGuiElem elem, function<bool(GuiElem*)> cond);
  PGuiElem conditional(PGuiElem elem, function<bool()> cond);
  PGuiElem conditionalStopKeys(PGuiElem elem, function<bool()> cond);
  PGuiElem conditional2(PGuiElem elem, PGuiElem alter, function<bool(GuiElem*)> cond);
  PGuiElem conditional(PGuiElem elem, PGuiElem alter, function<bool()> cond);
  enum class Alignment { TOP, LEFT, BOTTOM, RIGHT, TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, CENTER,
      TOP_CENTER, LEFT_CENTER, BOTTOM_CENTER, RIGHT_CENTER, VERTICAL_CENTER, LEFT_STRETCHED, RIGHT_STRETCHED,
      CENTER_STRETCHED};
  PGuiElem sprite(Texture&, Alignment, bool vFlip = false, bool hFlip = false,
      Vec2 offset = Vec2(0, 0), function<Color()> = nullptr);
  PGuiElem sprite(Texture&, Alignment, Color);
  PGuiElem sprite(Texture&, double scale);
  PGuiElem tooltip(const vector<string>&, int delayMilli = 700);
  PGuiElem darken();
  PGuiElem stopMouseMovement();
  PGuiElem fullScreen(PGuiElem);
  PGuiElem alignment(GuiFactory::Alignment, PGuiElem, optional<Vec2> size = none);
  PGuiElem dragSource(DragContent, function<PGuiElem()>);
  PGuiElem dragListener(function<void(DragContent)>);
  PGuiElem renderInBounds(PGuiElem);

  enum class TexId {
    SCROLLBAR,
    SCROLL_BUTTON,
    BACKGROUND_PATTERN,
    HORI_CORNER1,
    HORI_CORNER2,
    HORI_LINE,
    HORI_MIDDLE,
    VERT_BAR,
    HORI_BAR,
    HORI_BAR_MINI,
    VERT_BAR_MINI,
    CORNER_MINI,
    HORI_BAR_MINI2,
    VERT_BAR_MINI2,
    CORNER_MINI2,
    CORNER_TOP_LEFT,
    CORNER_TOP_RIGHT,
    CORNER_BOTTOM_RIGHT,
    SCROLL_UP,
    SCROLL_DOWN,
    WINDOW_CORNER,
    WINDOW_CORNER_EXIT,
    WINDOW_VERT_BAR,
    UI_HIGHLIGHT,
    MAIN_MENU_HIGHLIGHT,
    KEEPER_CHOICE,
    ADVENTURER_CHOICE,
    KEEPER_HIGHLIGHT,
    ADVENTURER_HIGHLIGHT,
    MENU_ITEM,
    MENU_CORE,
    MENU_MOUTH,
    SPLASH1,
    SPLASH2,
    LOADING_SPLASH,
  };

  PGuiElem sprite(TexId, Alignment, function<Color()> = nullptr);
  PGuiElem repeatedPattern(Texture& tex);
  PGuiElem background(Color);
  PGuiElem highlight(double height);
  PGuiElem highlightDouble();
  PGuiElem mainMenuHighlight();
  PGuiElem window(PGuiElem content, function<void()> onExitButton);
  PGuiElem miniWindow();
  PGuiElem miniWindow(PGuiElem content, function<void()> onExitButton = nullptr);
  PGuiElem miniWindow2(PGuiElem content, function<void()> onExitButton = nullptr);
  PGuiElem miniBorder();
  PGuiElem miniBorder2();
  PGuiElem mainDecoration(int rightBarWidth, int bottomBarHeight);
  PGuiElem invisible(PGuiElem content);
  PGuiElem background(PGuiElem content, Color);
  PGuiElem translucentBackground(PGuiElem content);
  PGuiElem translucentBackground();
  Color translucentBgColor = Color(0, 0, 0, 150);
  Color foreground1 = Color(0x20, 0x5c, 0x4a, 150);
  Color text = colors[ColorId::WHITE];
  Color titleText = colors[ColorId::YELLOW];
  Color inactiveText = colors[ColorId::LIGHT_GRAY];
  Color background1 = colors[ColorId::BLACK];

  enum IconId {
    WORLD_MAP,
    LIBRARY,
    DIPLOMACY,
    HELP,
    MINION,
    BUILDING,
    DEITIES,
    HIGHLIGHT,
    STAT_ATT,
    STAT_DEF,
    STAT_ACC,
    STAT_SPD,
    STAT_STR,
    STAT_DEX,
    MORALE_1,
    MORALE_2,
    MORALE_3,
    MORALE_4,
    TEAM_BUTTON,
    TEAM_BUTTON_HIGHLIGHT,
  };

  PGuiElem icon(IconId, Alignment = Alignment::CENTER, Color = colors[ColorId::WHITE]);
  Texture& get(TexId);
  PGuiElem spellIcon(SpellId);
  PGuiElem uiHighlightMouseOver(Color = colors[ColorId::GREEN]);
  PGuiElem uiHighlightConditional(function<bool()>, Color = colors[ColorId::GREEN]);
  PGuiElem uiHighlight(Color = colors[ColorId::GREEN]);
  PGuiElem uiHighlight(function<Color()>);
  PGuiElem rectangleBorder(Color);

  private:

  PGuiElem getScrollbar();
  Vec2 getScrollButtonSize();
  Texture& getIconTex(IconId);
  SDL_Keysym getHotkeyEvent(char) ;

  map<TexId, Texture> textures;
  vector<Texture> iconTextures;
  vector<Texture> spellTextures;
  Clock* clock;
  Renderer& renderer;
  Options* options;
  DragContainer dragContainer;
};


#endif
