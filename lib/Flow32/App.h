#pragma once

#include "AppStore.h"
#include "Canvas.h"
#include "Page.h"
#include "Rect.h"
#include "input/InputHub.h"
#include "ui/Theme.h"
#include "ui/UIEvent.h"

/** How `App::goTo` updates the navigation stack. */
enum class NavMode : uint8_t {
  Push,    // push a new entry (previous scroll saved)
  Replace, // replace the top entry (stack depth unchanged)
};

/** Type-erased app for the shell (switch Gallery / Home / …). */
class AppBase {
public:
  virtual ~AppBase() = default;

  virtual bool open() = 0;
  virtual void close() = 0;

  /**
   * Content frame only (no chrome). Shell sets the content viewport first,
   * then draws the nav bar after this returns (unless fullscreen).
   */
  virtual void frame(Canvas &canvas, InputHub &input, float dt) = 0;

  /** Full panel bounds (Shell owns layout). */
  virtual void setPanel(const Rect &panel) = 0;
  /** Scrollable content region under (or instead of) the nav bar. */
  virtual void setContentViewport(const Rect &content) = 0;

  virtual const char *shellTitle() const = 0;
  virtual bool shellFullscreen() const = 0;

  /** Right-side status icons (Lucide names); Shell draws them. */
  virtual uint8_t shellStatusCount() const { return 0; }
  virtual const char *shellStatusIcon(uint8_t /*i*/) const { return nullptr; }
};

/**
 * App = persistent state (AppStore) + one scroll Page + a navigation stack.
 *
 * Register pages in the ctor with titles (shown in the Shell nav bar):
 *
 *   addPage("Home");
 *   addPage("Theme");
 *   addPage("Player", true);  // fullscreen
 *
 * Then `build(page, pageId)` paints the active page. Stack entries store
 * scroll/focus so Back restores them.
 */
template <typename S> class App : public AppBase {
public:
  static constexpr uint8_t kMaxPages = 8;
  static constexpr uint8_t kMaxStack = 8;

  explicit App(const Rect &viewport)
      : panel_(viewport), page_(viewport) {
    stack_[0] = NavEntry{};
    depth_ = 1;
  }

  ~App() override { close(); }

  App(const App &) = delete;
  App &operator=(const App &) = delete;

  const S &state() const { return store_.get(); }
  bool ready() const { return store_.ready(); }

  uint8_t pageCount() const { return pageCount_; }
  /** Top-of-stack logical page id. */
  uint8_t pageId() const { return stack_[depth_ - 1].pageId; }
  uint8_t pageIndex() const { return pageId(); }
  uint8_t stackDepth() const { return depth_; }
  bool canGoBack() const { return depth_ > 1; }

  /** Nav title for a registered page (empty if unknown). */
  const char *titleOf(uint8_t id) const {
    if (id >= pageCount_ || !pages_[id].title) return "";
    return pages_[id].title;
  }

  Page &page() { return page_; }
  const Page &page() const { return page_; }

  void setPanel(const Rect &panel) override { panel_ = panel; }

  void setContentViewport(const Rect &content) override {
    if (content.x == page_.viewport().x && content.y == page_.viewport().y &&
        content.w == page_.viewport().w && content.h == page_.viewport().h) {
      return;
    }
    page_.setViewport(content);
    store_.markDirty();
  }

  const char *shellTitle() const override { return pageTitle(pageId()); }
  bool shellFullscreen() const override { return pageFullscreen(pageId()); }

  /**
   * Navigate to a logical page.
   * Push saves the current scroll/focus; Replace overwrites the top entry.
   */
  void goTo(uint8_t id, NavMode mode = NavMode::Push) {
    if (id >= pageCount_) return;

    if (mode == NavMode::Push) {
      if (id == pageId() && !store_.isDirty()) return;
      saveTopChrome();
      if (depth_ >= kMaxStack) {
        // Stack full — replace top instead of overflowing.
        mode = NavMode::Replace;
      } else {
        stack_[depth_] = NavEntry{};
        stack_[depth_].pageId = id;
        depth_++;
        activateTop(/*restoreScroll=*/false);
        return;
      }
    }

    // Replace
    if (id == pageId() && !store_.isDirty()) return;
    stack_[depth_ - 1].pageId = id;
    stack_[depth_ - 1].scrollY = 0;
    stack_[depth_ - 1].focusIndex = Page::kNoFocus;
    activateTop(/*restoreScroll=*/false);
  }

  /** Pop the stack (Back). Returns false if already at root. */
  bool back() {
    if (depth_ <= 1) return false;
    depth_--;
    activateTop(/*restoreScroll=*/true);
    return true;
  }

  bool open() override {
    if (!begin(nvsNamespace(), saveDebounceMs())) return false;
    stack_[0] = NavEntry{};
    depth_ = 1;
    restoreScroll_ = false;
    restoreScrollY_ = 0;
    store_.markDirty();
    return true;
  }

  void close() override {
    if (!store_.ready()) return;
    end();
  }

  void frame(Canvas &canvas, InputHub &input, float dt) override {
    const Theme::ThemeTokens &th = Theme::active();
    page_.setContentBackground(th.base100);

    rebuildIfNeeded(canvas);
    dispatchInput(input);
    rebuildIfNeeded(canvas);

    page_.tick(dt);
    page_.drawUI(canvas);
    store_.tick();
  }

protected:
  virtual const char *nvsNamespace() const = 0;
  virtual uint32_t saveDebounceMs() const { return 300; }

  /**
   * Register a page (id = 0, 1, … in call order). `title` is shown in the
   * Shell nav bar. Pointer must outlive the app (use string literals).
   */
  uint8_t addPage(const char *title, bool fullscreen = false) {
    if (pageCount_ >= kMaxPages) return 0xFF;
    const uint8_t id = pageCount_;
    pages_[id].title = title ? title : "";
    pages_[id].fullscreen = fullscreen;
    pageCount_++;
    return id;
  }

  /** Update a registered page's nav title (literal / stable pointer). */
  void setPageTitle(uint8_t id, const char *title) {
    if (id >= pageCount_) return;
    pages_[id].title = title ? title : "";
  }

  void setPageFullscreen(uint8_t id, bool fullscreen) {
    if (id >= pageCount_) return;
    pages_[id].fullscreen = fullscreen;
  }

  virtual void build(Page &page, uint8_t pageId) = 0;

  /** Default: title from `addPage`. Override for dynamic titles. */
  virtual const char *pageTitle(uint8_t id) const { return titleOf(id); }
  virtual bool pageFullscreen(uint8_t id) const {
    return id < pageCount_ && pages_[id].fullscreen;
  }

  virtual void onOpen() {}
  virtual void onClose() {}

  S &data() { return store_.data(); }

  template <typename T> bool set(T &field, const T &value) {
    return store_.set(field, value);
  }

  bool begin(const char *nvsNs, uint32_t saveDebounceMs = 300) {
    if (!store_.begin(nvsNs, saveDebounceMs)) return false;
    onOpen();
    return true;
  }

  void end() {
    if (!store_.ready()) return;
    onClose();
    store_.end();
  }

private:
  struct PageDef {
    const char *title = "";
    bool fullscreen = false;
  };

  struct NavEntry {
    uint8_t pageId = 0;
    int16_t scrollY = 0;
    uint8_t focusIndex = Page::kNoFocus;
  };

  void saveTopChrome() {
    NavEntry &top = stack_[depth_ - 1];
    top.scrollY = page_.scrollY();
    top.focusIndex = page_.focusIndex();
  }

  void activateTop(bool restoreScroll) {
    page_.clearFocus();
    if (!restoreScroll) {
      page_.scrollToImmediate(0);
      restoreScroll_ = false;
    } else {
      restoreScroll_ = true;
      restoreScrollY_ = stack_[depth_ - 1].scrollY;
      restoreFocus_ = stack_[depth_ - 1].focusIndex;
    }
    store_.markDirty();
    navPending_ = true;
  }

  void dispatchInput(InputHub &input) {
    UIEvent e;
    while (input.pop(e)) {
      if (e.key == UIKey::Back && e.phase == UIKeyPhase::Down) {
        if (back()) continue;
      }
      page_.dispatch(e);
    }
  }

  void rebuildIfNeeded(Canvas &canvas) {
    if (!store_.isDirty()) return;
    if (!navPending_ && page_.uiAnimating()) return;

    page_.beginUI();
    build(page_, pageId());
    page_.layoutUI(canvas);
    page_.syncFocus();

    if (restoreScroll_) {
      page_.scrollToImmediate(restoreScrollY_);
      if (restoreFocus_ != Page::kNoFocus) {
        page_.setFocusIndex(restoreFocus_);
      }
      restoreScroll_ = false;
    }

    page_.invalidateContent();
    store_.consumeDirty();
    navPending_ = false;
  }

  AppStore<S> store_{};
  Rect panel_{};
  Page page_;
  PageDef pages_[kMaxPages] = {};
  NavEntry stack_[kMaxStack] = {};
  uint8_t depth_ = 1;
  uint8_t pageCount_ = 0;
  bool navPending_ = false;
  bool restoreScroll_ = false;
  int16_t restoreScrollY_ = 0;
  uint8_t restoreFocus_ = Page::kNoFocus;
};
