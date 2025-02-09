
#include "root_view.h"

#include <algorithm>

#include "base/logging.h"
#include "base/message_loop.h"

#include "ui_gfx/canvas_skia.h"

#include "ui_base/accessibility/accessible_view_state.h"

#include "view/focus/view_storage.h"
#include "view/layout/fill_layout.h"
#include "view/widget/widget.h"

namespace view
{
namespace internal
{

// static
const char RootView::kViewClassName[] = "view/RootView";

////////////////////////////////////////////////////////////////////////////////
// RootView, public:

// Creation and lifetime -------------------------------------------------------

RootView::RootView(Widget* widget)
    : widget_(widget),
      mouse_pressed_handler_(NULL),
      mouse_move_handler_(NULL),
      last_click_handler_(NULL),
      explicit_mouse_handler_(false),
      last_mouse_event_flags_(0),
      last_mouse_event_x_(-1),
      last_mouse_event_y_(-1),
      focus_search_(this, false, false),
      focus_traversable_parent_(NULL),
      focus_traversable_parent_view_(NULL) {}

RootView::~RootView()
{
    // If we have children remove them explicitly so to make sure a remove
    // notification is sent for each one of them.
    if(has_children())
    {
        RemoveAllChildViews(true);
    }
}

// Tree operations -------------------------------------------------------------

void RootView::SetContentsView(View* contents_view)
{
    DCHECK(contents_view && GetWidget()->native_widget()) <<
            "Can't be called until after the native widget is created!";
    // The ContentsView must be set up _after_ the window is created so that its
    // Widget pointer is valid.
    SetLayoutManager(new FillLayout);
    if(has_children())
    {
        RemoveAllChildViews(true);
    }
    AddChildView(contents_view);

    // Force a layout now, since the attached hierarchy won't be ready for the
    // containing window's bounds. Note that we call Layout directly rather than
    // calling the widget's size changed handler, since the RootView's bounds may
    // not have changed, which will cause the Layout not to be done otherwise.
    Layout();
}

View* RootView::GetContentsView()
{
    return child_count()>0 ? child_at(0) : NULL;
}

void RootView::NotifyNativeViewHierarchyChanged(bool attached,
        HWND native_view)
{
    PropagateNativeViewHierarchyChanged(attached, native_view, this);
}

// Input -----------------------------------------------------------------------

bool RootView::OnKeyEvent(const KeyEvent& event)
{
    bool consumed = false;

    View* v = GetFocusManager()->GetFocusedView();
    // Special case to handle right-click context menus triggered by the
    // keyboard.
    if(v && v->IsEnabled() && ((event.key_code()==ui::VKEY_APPS) ||
                               (event.key_code()==ui::VKEY_F10 && event.IsShiftDown())))
    {
        v->ShowContextMenu(v->GetKeyboardContextMenuLocation(), false);
        return true;
    }
    for(; v && v!=this&&!consumed; v=v->parent())
    {
        consumed = (event.type() == ui::ET_KEY_PRESSED) ?
                   v->OnKeyPressed(event) : v->OnKeyReleased(event);
    }
    return consumed;
}

// Focus -----------------------------------------------------------------------

void RootView::SetFocusTraversableParent(FocusTraversable* focus_traversable)
{
    DCHECK(focus_traversable != this);
    focus_traversable_parent_ = focus_traversable;
}

void RootView::SetFocusTraversableParentView(View* view)
{
    focus_traversable_parent_view_ = view;
}

// System events ---------------------------------------------------------------

void RootView::ThemeChanged()
{
    View::PropagateThemeChanged();
}

void RootView::LocaleChanged()
{
    View::PropagateLocaleChanged();
}

////////////////////////////////////////////////////////////////////////////////
// RootView, FocusTraversable implementation:

FocusSearch* RootView::GetFocusSearch()
{
    return &focus_search_;
}

FocusTraversable* RootView::GetFocusTraversableParent()
{
    return focus_traversable_parent_;
}

View* RootView::GetFocusTraversableParentView()
{
    return focus_traversable_parent_view_;
}

////////////////////////////////////////////////////////////////////////////////
// RootView, View overrides:

const Widget* RootView::GetWidget() const
{
    return widget_;
}

Widget* RootView::GetWidget()
{
    return const_cast<Widget*>(const_cast<const RootView*>(this)->GetWidget());
}

bool RootView::IsVisibleInRootView() const
{
    return IsVisible();
}

std::string RootView::GetClassName() const
{
    return kViewClassName;
}

void RootView::SchedulePaintInRect(const gfx::Rect& rect)
{
    gfx::Rect xrect = ConvertRectToParent(rect);
    gfx::Rect invalid_rect = GetLocalBounds().Intersect(xrect);
    if(!invalid_rect.IsEmpty())
    {
        widget_->SchedulePaintInRect(invalid_rect);
    }
}

bool RootView::OnSetCursor(const gfx::Point& p)
{
    View* v = GetEventHandlerForPoint(p);
    if(v && v!=this)
    {
        gfx::Point l(p);
        View::ConvertPointToView(this, v, &l);
        return v->OnSetCursor(l);
    }
    return false;
}

bool RootView::OnMousePressed(const MouseEvent& event)
{
    MouseEvent e(event, this);

    SetMouseLocationAndFlags(e);

    // If mouse_pressed_handler_ is non null, we are currently processing
    // a pressed -> drag -> released session. In that case we send the
    // event to mouse_pressed_handler_
    if(mouse_pressed_handler_)
    {
        MouseEvent mouse_pressed_event(e, this, mouse_pressed_handler_);
        drag_info.Reset();
        mouse_pressed_handler_->ProcessMousePressed(mouse_pressed_event,
                &drag_info);
        return true;
    }
    DCHECK(!explicit_mouse_handler_);

    bool hit_disabled_view = false;
    // Walk up the tree until we find a view that wants the mouse event.
    for(mouse_pressed_handler_=GetEventHandlerForPoint(e.location());
            mouse_pressed_handler_&&(mouse_pressed_handler_!=this);
            mouse_pressed_handler_=mouse_pressed_handler_->parent())
    {
        if(!mouse_pressed_handler_->IsEnabled())
        {
            // Disabled views should eat events instead of propagating them upwards.
            hit_disabled_view = true;
            break;
        }

        // See if this view wants to handle the mouse press.
        MouseEvent mouse_pressed_event(e, this, mouse_pressed_handler_);

        // Remove the double-click flag if the handler is different than the
        // one which got the first click part of the double-click.
        if(mouse_pressed_handler_!=last_click_handler_)
        {
            mouse_pressed_event.set_flags(e.flags() &
                                          ~ui::EF_IS_DOUBLE_CLICK);
        }

        drag_info.Reset();
        bool handled = mouse_pressed_handler_->ProcessMousePressed(
                           mouse_pressed_event, &drag_info);

        // The view could have removed itself from the tree when handling
        // OnMousePressed().  In this case, the removal notification will have
        // reset mouse_pressed_handler_ to NULL out from under us.  Detect this
        // case and stop.  (See comments in view.h.)
        //
        // NOTE: Don't return true here, because we don't want the frame to
        // forward future events to us when there's no handler.
        if(!mouse_pressed_handler_)
        {
            break;
        }

        // If the view handled the event, leave mouse_pressed_handler_ set and
        // return true, which will cause subsequent drag/release events to get
        // forwarded to that view.
        if(handled)
        {
            last_click_handler_ = mouse_pressed_handler_;
            return true;
        }
    }

    // Reset mouse_pressed_handler_ to indicate that no processing is occurring.
    mouse_pressed_handler_ = NULL;

    // In the event that a double-click is not handled after traversing the
    // entire hierarchy (even as a single-click when sent to a different view),
    // it must be marked as handled to avoid anything happening from default
    // processing if it the first click-part was handled by us.
    if(last_click_handler_ && e.flags()&ui::EF_IS_DOUBLE_CLICK)
    {
        hit_disabled_view = true;
    }

    last_click_handler_ = NULL;
    return hit_disabled_view;
}

bool RootView::OnMouseDragged(const MouseEvent& event)
{
    MouseEvent e(event, this);

    if(mouse_pressed_handler_)
    {
        SetMouseLocationAndFlags(e);

        MouseEvent mouse_event(e, this, mouse_pressed_handler_);
        return mouse_pressed_handler_->ProcessMouseDragged(mouse_event, &drag_info);
    }
    return false;
}

void RootView::OnMouseReleased(const MouseEvent& event)
{
    MouseEvent e(event, this);

    if(mouse_pressed_handler_)
    {
        MouseEvent mouse_released(e, this, mouse_pressed_handler_);
        // We allow the view to delete us from ProcessMouseReleased. As such,
        // configure state such that we're done first, then call View.
        View* mouse_pressed_handler = mouse_pressed_handler_;
        SetMouseHandler(NULL);
        mouse_pressed_handler->ProcessMouseReleased(mouse_released);
        // WARNING: we may have been deleted.
    }
}

void RootView::OnMouseCaptureLost()
{
    if(mouse_pressed_handler_)
    {
        // We allow the view to delete us from OnMouseCaptureLost. As such,
        // configure state such that we're done first, then call View.
        View* mouse_pressed_handler = mouse_pressed_handler_;
        SetMouseHandler(NULL);
        mouse_pressed_handler->OnMouseCaptureLost();
        // WARNING: we may have been deleted.
    }
}

void RootView::OnMouseMoved(const MouseEvent& event)
{
    MouseEvent e(event, this);

    View* v = GetEventHandlerForPoint(e.location());
    // Find the first enabled view, or the existing move handler, whichever comes
    // first.  The check for the existing handler is because if a view becomes
    // disabled while handling moves, it's wrong to suddenly send ET_MOUSE_EXITED
    // and ET_MOUSE_ENTERED events, because the mouse hasn't actually exited yet.
    while(v && !v->IsEnabled() && (v!=mouse_move_handler_))
    {
        v = v->parent();
    }
    if(v && v!=this)
    {
        if(v != mouse_move_handler_)
        {
            if(mouse_move_handler_ != NULL)
            {
                mouse_move_handler_->OnMouseExited(e);
            }
            mouse_move_handler_ = v;
            MouseEvent entered_event(e, this, mouse_move_handler_);
            mouse_move_handler_->OnMouseEntered(entered_event);
        }
        MouseEvent moved_event(e, this, mouse_move_handler_);
        mouse_move_handler_->OnMouseMoved(moved_event);
    }
    else if(mouse_move_handler_ != NULL)
    {
        mouse_move_handler_->OnMouseExited(e);
        widget_->SetCursor(NULL);
    }
}

void RootView::OnMouseExited(const MouseEvent& event)
{
    if(mouse_move_handler_ != NULL)
    {
        mouse_move_handler_->OnMouseExited(event);
        mouse_move_handler_ = NULL;
    }
}

bool RootView::OnMouseWheel(const MouseWheelEvent& event)
{
    MouseWheelEvent e(event, this);
    bool consumed = false;
    View* v = GetFocusManager()->GetFocusedView();
    for(; v && v!=this&&!consumed; v=v->parent())
    {
        consumed = v->OnMouseWheel(e);
    }
    return consumed;
}

void RootView::SetMouseHandler(View* new_mh)
{
    // If we're clearing the mouse handler, clear explicit_mouse_handler_ as well.
    explicit_mouse_handler_ = (new_mh != NULL);
    mouse_pressed_handler_ = new_mh;
}

void RootView::GetAccessibleState(ui::AccessibleViewState* state)
{
    state->role = ui::AccessibilityTypes::ROLE_APPLICATION;
}

////////////////////////////////////////////////////////////////////////////////
// RootView, protected:

void RootView::ViewHierarchyChanged(bool is_add, View* parent, View* child)
{
    widget_->ViewHierarchyChanged(is_add, parent, child);

    if(!is_add)
    {
        if(!explicit_mouse_handler_ && mouse_pressed_handler_ == child)
        {
            mouse_pressed_handler_ = NULL;
        }
        if(mouse_move_handler_ == child)
        {
            mouse_move_handler_ = NULL;
        }
    }
}

void RootView::OnPaint(gfx::Canvas* canvas)
{
    canvas->AsCanvasSkia()->drawColor(SK_ColorBLACK, SkXfermode::kClear_Mode);
}

const ui::Compositor* RootView::GetCompositor() const
{
    return widget_->GetCompositor();
}

ui::Compositor* RootView::GetCompositor()
{
    return widget_->GetCompositor();
}

void RootView::CalculateOffsetToAncestorWithLayer(gfx::Point* offset,
        ui::Layer** layer_parent)
{
    View::CalculateOffsetToAncestorWithLayer(offset, layer_parent);
    if(!layer())
    {
        widget_->CalculateOffsetToAncestorWithLayer(offset, layer_parent);
    }
}

////////////////////////////////////////////////////////////////////////////////
// RootView, private:

// Input -----------------------------------------------------------------------

void RootView::SetMouseLocationAndFlags(const MouseEvent& event)
{
    last_mouse_event_flags_ = event.flags();
    last_mouse_event_x_ = event.x();
    last_mouse_event_y_ = event.y();
}

} //namespace internal
} //namespace view