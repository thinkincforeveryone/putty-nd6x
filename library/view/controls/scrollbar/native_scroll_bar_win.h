
#ifndef __view_native_scroll_bar_win_h__
#define __view_native_scroll_bar_win_h__

#pragma once

#include "native_scroll_bar_wrapper.h"
#include "view/controls/native_control_win.h"

namespace view
{

class ScrollBarContainer;

/////////////////////////////////////////////////////////////////////////////
//
// NativeScrollBarWin
//
// A View subclass that wraps a Native Windows scrollbar control.
//
// A scrollbar is either horizontal or vertical.
//
/////////////////////////////////////////////////////////////////////////////
class NativeScrollBarWin : public NativeControlWin,
    public NativeScrollBarWrapper
{
public:
    // Create new scrollbar, either horizontal or vertical.
    explicit NativeScrollBarWin(NativeScrollBar* native_scroll_bar);
    virtual ~NativeScrollBarWin();

private:
    // Overridden from View for layout purpose.
    virtual void Layout();
    virtual gfx::Size GetPreferredSize();

    // Overridden from View for keyboard UI purpose.
    virtual bool OnKeyPressed(const KeyEvent& event);
    virtual bool OnMouseWheel(const MouseWheelEvent& e);

    // Overridden from NativeControlWin.
    virtual void CreateNativeControl();

    // Overridden from ScrollBarWrapper.
    virtual int GetPosition() const;
    virtual View* GetView();
    virtual void Update(int viewport_size, int content_size, int current_pos);

    // The NativeScrollBar we are bound to.
    NativeScrollBar* native_scroll_bar_;

    // sb_container_ is a custom hwnd that we use to wrap the real
    // windows scrollbar. We need to do this to get the scroll events
    // without having to do anything special in the high level hwnd.
    scoped_ptr<ScrollBarContainer> sb_container_;

    // Last scrollbar state we wrote to the scrollbar. We keep it here, so we can
    // reinitialize the scrollbar to its previous state in case it gets hidden and
    // recreated.
    SCROLLINFO scroll_info_;

    DISALLOW_COPY_AND_ASSIGN(NativeScrollBarWin);
};

} //namespace view

#endif //__view_native_scroll_bar_win_h__