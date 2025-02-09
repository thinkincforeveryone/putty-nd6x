
#ifndef __view_non_client_view_h__
#define __view_non_client_view_h__

#pragma once

#include "client_view.h"

namespace gfx
{
class Path;
}

namespace view
{

////////////////////////////////////////////////////////////////////////////////
// NonClientFrameView
//
//  An object that subclasses NonClientFrameView is a View that renders and
//  responds to events within the frame portions of the non-client area of a
//  window. This view does _not_ contain the ClientView, but rather is a sibling
//  of it.
class NonClientFrameView : public View
{
public:
    // Internal class name.
    static const char kViewClassName[];
    // Various edges of the frame border have a 1 px shadow along their edges; in
    // a few cases we shift elements based on this amount for visual appeal.
    static const int kFrameShadowThickness;
    // In restored mode, we draw a 1 px edge around the content area inside the
    // frame border.
    static const int kClientEdgeThickness;

    // Prevent the frame view from painting its inactive state. Prevents a related
    // window from causing its owner to appear deactivated. Used for windows like
    // bubbles.
    void DisableInactiveRendering(bool disable)
    {
        paint_as_active_ = disable;
        if(!paint_as_active_)
        {
            SchedulePaint();
        }
    }

    // Returns the bounds (in this View's parent's coordinates) that the client
    // view should be laid out within.
    virtual gfx::Rect GetBoundsForClientView() const = 0;

    virtual gfx::Rect GetWindowBoundsForClientBounds(
        const gfx::Rect& client_bounds) const = 0;

    // This function must ask the ClientView to do a hittest.  We don't do this in
    // the parent NonClientView because that makes it more difficult to calculate
    // hittests for regions that are partially obscured by the ClientView, e.g.
    // HTSYSMENU.
    virtual int NonClientHitTest(const gfx::Point& point) = 0;
    virtual void GetWindowMask(const gfx::Size& size,
                               gfx::Path* window_mask) = 0;
    virtual void EnableClose(bool enable) = 0;
    virtual void ResetWindowControls() = 0;
    virtual void UpdateWindowIcon() = 0;

    // Overridden from View:
    virtual bool HitTest(const gfx::Point& l) const;
    virtual void GetAccessibleState(ui::AccessibleViewState* state);
    virtual std::string GetClassName() const;

protected:
    virtual void OnBoundsChanged(const gfx::Rect& previous_bounds);

    NonClientFrameView() : paint_as_active_(false) {}


    // Helper for non-client view implementations to determine which area of the
    // window border the specified |point| falls within. The other parameters are
    // the size of the sizing edges, and whether or not the window can be
    // resized.
    int GetHTComponentForFrame(const gfx::Point& point,
                               int top_resize_border_height,
                               int resize_border_thickness,
                               int top_resize_corner_height,
                               int resize_corner_width,
                               bool can_resize);

    // Used to determine if the frame should be painted as active. Keyed off the
    // window's actual active state and the override, see
    // DisableInactiveRendering() above.
    bool ShouldPaintAsActive() const;

private:
    // True when the non-client view should always be rendered as if the window
    // were active, regardless of whether or not the top level window actually
    // is active.
    bool paint_as_active_;
};

////////////////////////////////////////////////////////////////////////////////
// NonClientView
//
//  The NonClientView is the logical root of all Views contained within a
//  Window, except for the RootView which is its parent and of which it is the
//  sole child. The NonClientView has two children, the NonClientFrameView which
//  is responsible for painting and responding to events from the non-client
//  portions of the window, and the ClientView, which is responsible for the
//  same for the client area of the window:
//
//  +- view::Window -------------------------------------+
//  | +- view::RootView -------------------------------+ |
//  | | +- view::NonClientView ----------------------+ | |
//  | | | +- view::NonClientFrameView subclas  ----+ | | |
//  | | | |                                        | | | |
//  | | | | << all painting and event receiving >> | | | |
//  | | | | << of the non-client areas of a     >> | | | |
//  | | | | << view::Window.                    >> | | | |
//  | | | |                                        | | | |
//  | | | +----------------------------------------+ | | |
//  | | | +- view::ClientView or subclass ---------+ | | |
//  | | | |                                        | | | |
//  | | | | << all painting and event receiving >> | | | |
//  | | | | << of the client areas of a         >> | | | |
//  | | | | << view::Window.                    >> | | | |
//  | | | |                                        | | | |
//  | | | +----------------------------------------+ | | |
//  | | +--------------------------------------------+ | |
//  | +------------------------------------------------+ |
//  +----------------------------------------------------+
//
// The NonClientFrameView and ClientView are siblings because due to theme
// changes the NonClientFrameView may be replaced with different
// implementations (e.g. during the switch from DWM/Aero-Glass to Vista Basic/
// Classic rendering).
class NonClientView : public View
{
public:
    // Internal class name.
    static const char kViewClassName[];

    NonClientView();
    virtual ~NonClientView();

    // Returns the current NonClientFrameView instance, or NULL if
    // it does not exist.
    NonClientFrameView* frame_view() const
    {
        return frame_view_.get();
    }

    // Replaces the current NonClientFrameView (if any) with the specified one.
    void SetFrameView(NonClientFrameView* frame_view);

    // Returns true if the ClientView determines that the containing window can be
    // closed, false otherwise.
    bool CanClose();

    // Called by the containing Window when it is closed.
    void WindowClosing();

    // Changes the frame from native to custom depending on the value of
    // |use_native_frame|.
    void UpdateFrame();

    // Prevents the window from being rendered as deactivated when |disable| is
    // true, until called with |disable| false. Used when a sub-window is to be
    // shown that shouldn't visually de-activate the window.
    // Subclasses can override this to perform additional actions when this value
    // changes.
    void DisableInactiveRendering(bool disable);

    // Returns the bounds of the window required to display the content area at
    // the specified bounds.
    gfx::Rect GetWindowBoundsForClientBounds(const gfx::Rect client_bounds) const;

    // Determines the windows HT* code when the mouse cursor is at the
    // specified point, in window coordinates.
    int NonClientHitTest(const gfx::Point& point);

    // Returns a mask to be used to clip the top level window for the given
    // size. This is used to create the non-rectangular window shape.
    void GetWindowMask(const gfx::Size& size, gfx::Path* window_mask);

    // Toggles the enable state for the Close button (and the Close menu item in
    // the system menu).
    void EnableClose(bool enable);

    // Tells the window controls as rendered by the NonClientView to reset
    // themselves to a normal state. This happens in situations where the
    // containing window does not receive a normal sequences of messages that
    // would lead to the controls returning to this normal state naturally, e.g.
    // when the window is maximized, minimized or restored.
    void ResetWindowControls();

    // Tells the NonClientView to invalidate the NonClientFrameView's window icon.
    void UpdateWindowIcon();

    // Get/Set client_view property.
    ClientView* client_view() const
    {
        return client_view_;
    }
    void set_client_view(ClientView* client_view)
    {
        client_view_ = client_view;
    }

    // Layout just the frame view. This is necessary on Windows when non-client
    // metrics such as the position of the window controls changes independently
    // of a window resize message.
    void LayoutFrameView();

    // Set the accessible name of this view.
    void SetAccessibleName(const string16& name);

    // NonClientView, View overrides:
    virtual gfx::Size GetPreferredSize();
    virtual gfx::Size GetMinimumSize();
    virtual void Layout();
    virtual void GetAccessibleState(ui::AccessibleViewState* state);
    virtual std::string GetClassName() const;

    virtual View* GetEventHandlerForPoint(const gfx::Point& point);

protected:
    // NonClientView, View overrides:
    virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);

private:
    // A ClientView object or subclass, responsible for sizing the contents view
    // of the window, hit testing and perhaps other tasks depending on the
    // implementation.
    ClientView* client_view_;

    // The NonClientFrameView that renders the non-client portions of the window.
    // This object is not owned by the view hierarchy because it can be replaced
    // dynamically as the system settings change.
    scoped_ptr<NonClientFrameView> frame_view_;

    // The accessible name of this view.
    string16 accessible_name_;

    DISALLOW_COPY_AND_ASSIGN(NonClientView);
};

} //namespace view

#endif //__view_non_client_view_h__