
#ifndef __view_root_view_h__
#define __view_root_view_h__

#pragma once

#include <string>

#include "view/focus/focus_manager.h"
#include "view/focus/focus_search.h"

namespace view
{

class Widget;

// This is a views-internal API and should not be used externally.
// Widget exposes this object as a View*.
namespace internal
{

////////////////////////////////////////////////////////////////////////////////
// RootView class
//
//  The RootView is the root of a View hierarchy. A RootView is attached to a
//  Widget. The Widget is responsible for receiving events from the host
//  environment, converting them to views-compatible events and then forwarding
//  them to the RootView for propagation into the View hierarchy.
//
//  A RootView can have only one child, called its "Contents View" which is
//  sized to fill the bounds of the RootView (and hence the client area of the
//  Widget). Call SetContentsView() after the associated Widget has been
//  initialized to attach the contents view to the RootView.
//  TODO(beng): Enforce no other callers to AddChildView/tree functions by
//              overriding those methods as private here.
//  TODO(beng): Clean up API further, make Widget a friend.
class RootView : public View, public FocusTraversable
{
public:
    static const char kViewClassName[];

    // Creation and lifetime -----------------------------------------------------
    explicit RootView(Widget* widget);
    virtual ~RootView();

    // Tree operations -----------------------------------------------------------

    // Sets the "contents view" of the RootView. This is the single child view
    // that is responsible for laying out the contents of the widget.
    void SetContentsView(View* contents_view);
    View* GetContentsView();

    // Called when parent of the host changed.
    void NotifyNativeViewHierarchyChanged(bool attached, HWND native_view);

    // Input ---------------------------------------------------------------------

    // Process a key event. Send the event to the focused view and up the focus
    // path, and finally to the default keyboard handler, until someone consumes
    // it. Returns whether anyone consumed the event.
    bool OnKeyEvent(const KeyEvent& event);

    // Focus ---------------------------------------------------------------------

    // Used to set the FocusTraversable parent after the view has been created
    // (typically when the hierarchy changes and this RootView is added/removed).
    virtual void SetFocusTraversableParent(FocusTraversable* focus_traversable);

    // Used to set the View parent after the view has been created.
    virtual void SetFocusTraversableParentView(View* view);

    // System events -------------------------------------------------------------

    // Public API for broadcasting theme change notifications to this View
    // hierarchy.
    void ThemeChanged();

    // Public API for broadcasting locale change notifications to this View
    // hierarchy.
    void LocaleChanged();

    // Overridden from FocusTraversable:
    virtual FocusSearch* GetFocusSearch();
    virtual FocusTraversable* GetFocusTraversableParent();
    virtual View* GetFocusTraversableParentView();

    // Overridden from View:
    virtual const Widget* GetWidget() const;
    virtual Widget* GetWidget();
    virtual bool IsVisibleInRootView() const;
    virtual std::string GetClassName() const;
    virtual void SchedulePaintInRect(const gfx::Rect& rect);
    virtual bool OnSetCursor(const gfx::Point& p);
    virtual bool OnMousePressed(const MouseEvent& event);
    virtual bool OnMouseDragged(const MouseEvent& event);
    virtual void OnMouseReleased(const MouseEvent& event);
    virtual void OnMouseCaptureLost();
    virtual void OnMouseMoved(const MouseEvent& event);
    virtual void OnMouseExited(const MouseEvent& event);
    virtual bool OnMouseWheel(const MouseWheelEvent& event);
    virtual void SetMouseHandler(View* new_mouse_handler);
    virtual void GetAccessibleState(ui::AccessibleViewState* state);

protected:
    // Overridden from View:
    virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
    virtual void OnPaint(gfx::Canvas* canvas);
    virtual const ui::Compositor* GetCompositor() const;
    virtual ui::Compositor* GetCompositor();
    virtual void CalculateOffsetToAncestorWithLayer(
        gfx::Point* offset,
        ui::Layer** layer_parent);

private:
    friend class View;
    friend class Widget;

    // Input ---------------------------------------------------------------------

    // Updates the last_mouse_* fields from e. The location of the mouse should be
    // in the current coordinate system (i.e. any necessary transformation should
    // be applied to the point prior to calling this).
    void SetMouseLocationAndFlags(const MouseEvent& event);

    //////////////////////////////////////////////////////////////////////////////

    // Tree operations -----------------------------------------------------------

    // The host Widget
    Widget* widget_;

    // Input ---------------------------------------------------------------------

    // The view currently handing down - drag - up
    View* mouse_pressed_handler_;

    // The view currently handling enter / exit
    View* mouse_move_handler_;

    // The last view to handle a mouse click, so that we can determine if
    // a double-click lands on the same view as its single-click part.
    View* last_click_handler_;

    // true if mouse_pressed_handler_ has been explicitly set
    bool explicit_mouse_handler_;

    // Last position/flag of a mouse press/drag. Used if capture stops and we need
    // to synthesize a release.
    int last_mouse_event_flags_;
    int last_mouse_event_x_;
    int last_mouse_event_y_;

    // Focus ---------------------------------------------------------------------

    // The focus search algorithm.
    FocusSearch focus_search_;

    // Whether this root view belongs to the current active window.
    // bool activated_;

    // The parent FocusTraversable, used for focus traversal.
    FocusTraversable* focus_traversable_parent_;

    // The View that contains this RootView. This is used when we have RootView
    // wrapped inside native components, and is used for the focus traversal.
    View* focus_traversable_parent_view_;

    // Drag and drop -------------------------------------------------------------

    // Tracks drag state for a view.
    View::DragInfo drag_info;

    DISALLOW_IMPLICIT_CONSTRUCTORS(RootView);
};

}  //namespace internal
} //namespace view

#endif //__view_root_view_h__