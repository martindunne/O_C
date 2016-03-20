#ifndef OC_MENUS_H
#define OC_MENUS_H

#include "weegfx.h"
#include "OC_bitmaps.h"
#include "util_framebuffer.h"
#include "SH1106_128x64_Driver.h"
#include "util/util_debugpins.h"
#include "util/util_misc.h"
#include "util/util_settings.h"

extern weegfx::Graphics graphics;
extern FrameBuffer<SH1106_128x64_Driver::kFrameSize, 2> frame_buffer;

void init_circle_lut();
void visualize_pitch_classes(uint8_t *normalized, weegfx::coord_t centerx, weegfx::coord_t centery);

void screensaver();
void scope();
void scope_render();

namespace menu {

// Helper to manage cursor position in settings-type menus.
// The "fancy" mode tries to indicate that there may be more items by not
// moving to line 0 until the first item is selected (and vice versa for end
// of list).
// Currently assumes there are at least 4 items!
template <int screen_lines, bool fancy = true>
class ScreenCursor {
public:
  ScreenCursor() { }
  ~ScreenCursor() { }

  void Init(int start, int end) {
    editing_ = false;
    start_ = start;
    end_ = end;
    cursor_pos_ = start;
    screen_line_ = 0;
  }

  void AdjustEnd(int end) {
    // WARN This has a specific use case where we don't have to adjust screen line!
    end_ = end;
  }

  void Scroll(int amount) {
    int pos = cursor_pos_ + amount;
    CONSTRAIN(pos, start_, end_);
    cursor_pos_ = pos;

    int screen_line = screen_line_ + amount;
    if (fancy) {
      if (amount < 0) {
        if (screen_line < 2) {
          if (pos >= start_ + 1)
            screen_line = 1;
          else
            screen_line = 0;
        }
      } else {
        if (screen_line >= screen_lines - 2) {
          if (pos <= end_ - 1)
            screen_line = screen_lines - 2;
          else
            screen_line = screen_lines - 1;
        }
      }
    } else {
      CONSTRAIN(screen_line, 0, screen_lines - 1);
    }
    screen_line_ = screen_line;
  }

  inline int cursor_pos() const {
    return cursor_pos_;
  }

  inline int first_visible() const {
    return cursor_pos_ - screen_line_;
  }

  inline int last_visible() const {
    return cursor_pos_ - screen_line_ + screen_lines - 1;
  }

  inline bool editing() const {
    return editing_;
  }

  inline void toggle_editing() {
    editing_ = !editing_;
  }

  inline void set_editing(bool enable) {
    editing_ = enable;
  }

private:
  bool editing_;

  int start_, end_;
  int cursor_pos_;
  int screen_line_;
};

inline void DrawEditIcon(weegfx::coord_t x, weegfx::coord_t y, int value, const settings::value_attr &attr) {
  const uint8_t *src = OC::bitmap_edit_indicators_8;
  if (value == attr.max_)
    src += OC::kBitmapEditIndicatorW * 2;
  else if (value == attr.min_)
    src += OC::kBitmapEditIndicatorW;

  graphics.drawBitmap8(x - 5, y + 1, OC::kBitmapEditIndicatorW, src);
}

template <bool rtl, size_t max_bits>
void DrawMask(weegfx::coord_t y, uint32_t mask, size_t count) {
  weegfx::coord_t x, dx;
  if (count > max_bits) count = max_bits;
  if (rtl) {
    x = 128 - 3;
    dx = -3;
  } else {
    x = 128 - count * 3;
    dx = 3;
  }

  for (size_t i = 0; i < count; ++i, mask >>= 1, x += dx) {
    if (mask & 0x1)
      graphics.drawRect(x, y + 1, 2, 8);
    else
      graphics.drawRect(x, y + 8, 2, 1);
  }
}

};

#define MENU_DEFAULT_FONT nullptr

static const uint8_t kUiDefaultFontH = 12;
static const uint8_t kUiTitleTextY = 2;
static const uint8_t kUiItemsStartY = kUiDefaultFontH + 2 + 1;
static const uint8_t kUiLineH = 12;

static const uint8_t kUiVisibleItems = 4;
static const uint8_t kUiDisplayWidth = 128;

static const uint8_t kUiWideMenuCol1X = 96;

#define UI_DRAW_TITLE(xstart) \
  do { \
  graphics.setPrintPos(xstart + 2, kUiTitleTextY); \
  graphics.drawHLine(xstart, kUiDefaultFontH, kUiDisplayWidth); \
  } while (0)

#define UI_SETUP_ITEM(sel) \
  const bool __selected = sel; \
  do { \
    graphics.setPrintPos(xstart + 2, y + 1); \
  } while (0)

#define UI_END_ITEM() \
  do { \
    if (__selected) \
      graphics.invertRect(xstart, y, kUiDisplayWidth - xstart, kUiLineH - 1); \
  } while (0)


#define UI_START_MENU(x) \
  const weegfx::coord_t xstart = x; \
  weegfx::coord_t y = kUiItemsStartY; \
  graphics.setPrintPos(xstart + 2, y + 1); \
  do {} while (0)


#define UI_BEGIN_ITEMS_LOOP(xstart, first, last, selected, startline) \
  y = kUiItemsStartY + startline * kUiLineH; \
  for (int i = 0, current_item = first; i < kUiVisibleItems - startline && current_item < last; ++i, ++current_item, y += kUiLineH) { \
    UI_SETUP_ITEM(selected == current_item);

#define UI_END_ITEMS_LOOP() } do {} while (0)

#define UI_DRAW_SETTING(attr, value, xoffset) \
  do { \
    const int val = value; \
    graphics.print(attr.name); \
    if (xoffset) graphics.setPrintPos(xoffset, y + 1); \
    if(attr.value_names) \
      graphics.print(attr.value_names[val]); \
    else \
      graphics.pretty_print(val); \
    UI_END_ITEM(); \
  } while (0)

#define UI_DRAW_EDITABLE(b) \
  do { \
     if (__selected && (b)) { \
      graphics.print(' '); \
      graphics.drawBitmap8(xstart + 1, y + 1, 6, OC::bitmap_edit_indicator_6x8); \
    } \
 } while (0)

#define GRAPHICS_BEGIN_FRAME(wait) \
do { \
  DEBUG_PIN_SCOPE(DEBUG_PIN_1); \
  uint8_t *frame = NULL; \
  do { \
    if (frame_buffer.writeable()) \
      frame = frame_buffer.writeable_frame(); \
  } while (!frame && wait); \
  if (frame) { \
    graphics.Begin(frame, true); \
    do {} while(0)

#define GRAPHICS_END_FRAME() \
    graphics.End(); \
    frame_buffer.written(); \
  } \
} while (0)

#endif // OC_MENUS_H
