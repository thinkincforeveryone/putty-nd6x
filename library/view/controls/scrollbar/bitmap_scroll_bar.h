
#ifndef __view_bitmap_scroll_bar_h__
#define __view_bitmap_scroll_bar_h__

#pragma once

#include "scroll_bar.h"
#include "view/context_menu_controller.h"
#include "view/controls/button/image_button.h"
#include "view/controls/menu/menu.h"
#include "view/repeat_controller.h"

namespace view
{

namespace
{
class BitmapScrollBarThumb;
}

///////////////////////////////////////////////////////////////////////////////
//
// BitmapScrollBar
//
//  A ScrollBar subclass that implements a scroll bar rendered using bitmaps
//  that the user provides. There are bitmaps for the up and down buttons, as
//  well as for the thumb and track. This is intended for creating UIs that
//  have customized, non-native appearances, like floating HUDs etc.
//
//  Maybe TODO(beng): (Cleanup) If we need to, we may want to factor rendering
//                    out of this altogether and have the user supply
//                    Background impls for each component, and just use those
//                    to render, so that for example we get native theme
//                    rendering.
//
///////////////////////////////////////////////////////////////////////////////
class BitmapScrollBar : public ScrollBar,
    public ButtonListener,
    public ContextMenuController,
    public Menu::Delegate
{
public:
    BitmapScrollBar(bool horizontal, bool show_scroll_buttons);
    virtual ~BitmapScrollBar() {}

    // Get the bounds of the "track" area that the thumb is free to slide within.
    gfx::Rect GetTrackBounds() const;

    // A list of parts that the user may supply bitmaps for.
    enum ScrollBarPart
    {
        // The button used to represent scrolling up/left by 1 line.
        PREV_BUTTON = 0,
        // The button used to represent scrolling down/right by 1 line.
        // IMPORTANT: The code assumes the prev and next
        // buttons have equal width and equal height.
        NEXT_BUTTON,
        // The top/left segment of the thumb on the scrollbar.
        THUMB_START_CAP,
        // The tiled background image of the thumb.
        THUMB_MIDDLE,
        // The bottom/right segment of the thumb on the scrollbar.
        THUMB_END_CAP,
        // The grippy that is rendered in the center of the thumb.
        THUMB_GRIPPY,
        // The tiled background image of the thumb track.
        THUMB_TRACK,
        PART_COUNT
    };

    // Sets the bitmap to be rendered for the specified part and state.
    void SetImage(ScrollBarPart part,
                  CustomButton::ButtonState state,
                  SkBitmap* bitmap);

    // An enumeration of different amounts of incremental scroll, representing
    // events sent from different parts of the UI/keyboard.
    enum ScrollAmount
    {
        SCROLL_NONE = 0,
        SCROLL_START,
        SCROLL_END,
        SCROLL_PREV_LINE,
        SCROLL_NEXT_LINE,
        SCROLL_PREV_PAGE,
        SCROLL_NEXT_PAGE,
    };

    // Scroll the contents by the specified type (see ScrollAmount above).
    void ScrollByAmount(ScrollAmount amount);

    // Scroll the contents to the appropriate position given the supplied
    // position of the thumb (thumb track coordinates). If |scroll_to_middle| is
    // true, then the conversion assumes |thumb_position| is in the middle of the
    // thumb rather than the top.
    void ScrollToThumbPosition(int thumb_position, bool scroll_to_middle);

    // Scroll the contents by the specified offset (contents coordinates).
    void ScrollByContentsOffset(int contents_offset);

    // View overrides:
    virtual gfx::Size GetPreferredSize();
    virtual void Layout();
    virtual bool OnMousePressed(const MouseEvent& event);
    virtual void OnMouseReleased(const MouseEvent& event);
    virtual void OnMouseCaptureLost();
    virtual bool OnKeyPressed(const KeyEvent& event);
    virtual bool OnMouseWheel(const MouseWheelEvent& event);

    // BaseButton::ButtonListener overrides:
    virtual void ButtonPressed(Button* sender, const Event& event);

    // ScrollBar overrides:
    virtual void Update(int viewport_size,
                        int content_size,
                        int contents_scroll_offset);
    virtual int GetLayoutSize() const;
    virtual int GetPosition() const;

    // ContextMenuController overrides.
    virtual void ShowContextMenuForView(View* source,
                                        const gfx::Point& p, bool is_mouse_gesture);

    // Menu::Delegate overrides:
    virtual std::wstring GetLabel(int id) const;
    virtual bool IsCommandEnabled(int id) const;
    virtual void ExecuteCommand(int id);

protected:
    // View overrides:
    virtual void OnPaint(gfx::Canvas* canvas);

private:
    // Called when the mouse is pressed down in the track area.
    void TrackClicked();

    // Responsible for scrolling the contents and also updating the UI to the
    // current value of the Scroll Offset.
    void ScrollContentsToOffset();

    // Returns the size (width or height) of the track area of the ScrollBar.
    int GetTrackSize() const;

    // Calculate the position of the thumb within the track based on the
    // specified scroll offset of the contents.
    int CalculateThumbPosition(int contents_scroll_offset) const;

    // Calculates the current value of the contents offset (contents coordinates)
    // based on the current thumb position (thumb track coordinates). See
    // |ScrollToThumbPosition| for an explanation of |scroll_to_middle|.
    int CalculateContentsOffset(int thumb_position,
                                bool scroll_to_middle) const;

    // Called when the state of the thumb track changes (e.g. by the user
    // pressing the mouse button down in it).
    void SetThumbTrackState(CustomButton::ButtonState state);

    // The thumb needs to be able to access the part images.
    friend class BitmapScrollBarThumb;
    SkBitmap* images_[PART_COUNT][CustomButton::BS_COUNT];

    // The size of the scrolled contents, in pixels.
    int contents_size_;

    // The current amount the contents is offset by in the viewport.
    int contents_scroll_offset_;

    // Up/Down/Left/Right buttons and the Thumb.
    ImageButton* prev_button_;
    ImageButton* next_button_;
    BitmapScrollBarThumb* thumb_;

    // The state of the scrollbar track. Typically, the track will highlight when
    // the user presses the mouse on them (during page scrolling).
    CustomButton::ButtonState thumb_track_state_;

    // The last amount of incremental scroll that this scrollbar performed. This
    // is accessed by the callbacks for the auto-repeat up/down buttons to know
    // what direction to repeatedly scroll in.
    ScrollAmount last_scroll_amount_;

    // An instance of a RepeatController which scrolls the scrollbar continuously
    // as the user presses the mouse button down on the up/down buttons or the
    // track.
    RepeatController repeater_;

    // The position of the mouse within the scroll bar when the context menu
    // was invoked.
    int context_menu_mouse_position_;

    // True if the scroll buttons at each end of the scroll bar should be shown.
    bool show_scroll_buttons_;

    DISALLOW_COPY_AND_ASSIGN(BitmapScrollBar);
};

} //namespace view

#endif //__view_bitmap_scroll_bar_h__