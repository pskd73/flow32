#pragma once

#include "AppStore.h"
#include "Canvas.h"
#include "Page.h"
#include "Rect.h"
#include "input/InputHub.h"
#include "ui/Theme.h"

/** Type-erased app for the shell (switch Gallery / Home / …). */
class AppBase {
public:
  virtual ~AppBase() = default;

  /** Load state (NVS) and prepare UI. */
  virtual bool open() = 0;
  /** Flush state and tear down. */
  virtual void close() = 0;

  /**
   * One frame: rebuild dirty page UI, dispatch input, tick, draw, persist.
   */
  virtual void frame(Canvas &canvas, InputHub &input, float dt) = 0;
};

/**
 * App = persistent state (AppStore) + one scroll Page + logical pages.
 *
 * Multiple screens share one Page (rebuild on `goTo`). `build(page, index)`
 * paints the active logical page.
 *
 *   class HomeApp : public App<HomeState> {
 *   public:
 *     explicit HomeApp(const Rect &vp) : App(vp) { setPageCount(2); }
 *   protected:
 *     const char *nvsNamespace() const override { return "home"; }
 *     void build(Page &page, uint8_t pageIndex) override;
 *   };
 */
template <typename S> class App : public AppBase {
public:
  static constexpr uint8_t kMaxPages = 8;

  explicit App(const Rect &viewport) : page_(viewport) {}

  ~App() override { close(); }

  App(const App &) = delete;
  App &operator=(const App &) = delete;

  const S &state() const { return store_.get(); }
  bool ready() const { return store_.ready(); }

  uint8_t pageCount() const { return pageCount_; }
  uint8_t pageIndex() const { return activePage_; }
  Page &page() { return page_; }
  const Page &page() const { return page_; }

  /** Switch logical page (marks dirty so `build` runs before next paint). */
  void goTo(uint8_t index) {
    if (index >= pageCount_) return;
    if (index == activePage_ && !store_.isDirty()) return;
    activePage_ = index;
    page_.scrollToImmediate(0);
    page_.clearFocus();
    store_.markDirty();
    // Don't defer behind press/scroll anim — avoids one-frame flash of old page.
    navPending_ = true;
  }

  bool open() override {
    if (!begin(nvsNamespace(), saveDebounceMs())) return false;
    activePage_ = 0;
    return true;
  }

  void close() override {
    if (!store_.ready()) return;
    end();
  }

  void frame(Canvas &canvas, InputHub &input, float dt) override {
    const Theme::ThemeTokens &th = Theme::active();
    page_.setContentBackground(th.base100);

    // Tree must exist before input (first open / deferred dirty).
    rebuildIfNeeded(canvas);

    // goTo / setters run here and mark dirty after the check above.
    input.dispatchTo(page_);

    // Apply navigation/state in the same frame so we don't paint the old page.
    rebuildIfNeeded(canvas);

    page_.tick(dt);
    page_.drawUI(canvas);
    store_.tick();
  }

protected:
  /** NVS namespace for this app (e.g. "home"). */
  virtual const char *nvsNamespace() const = 0;
  virtual uint32_t saveDebounceMs() const { return 300; }

  /** How many logical pages (1..kMaxPages). Call from ctor. */
  void setPageCount(uint8_t n) {
    if (n < 1) n = 1;
    if (n > kMaxPages) n = kMaxPages;
    pageCount_ = n;
  }

  /**
   * Build the widget tree for `pageIndex` into `page`.
   * Called when state is dirty (or after goTo).
   */
  virtual void build(Page &page, uint8_t pageIndex) = 0;

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
  void rebuildIfNeeded(Canvas &canvas) {
    if (!store_.isDirty()) return;
    // Defer in-page edits during press/scroll anim; never defer page switches.
    if (!navPending_ && page_.uiAnimating()) return;

    page_.beginUI();
    build(page_, activePage_);
    page_.layoutUI(canvas);
    page_.syncFocus();
    page_.invalidateContent();
    store_.consumeDirty();
    navPending_ = false;
  }

  AppStore<S> store_{};
  Page page_;
  uint8_t pageCount_ = 1;
  uint8_t activePage_ = 0;
  bool navPending_ = false;
};
