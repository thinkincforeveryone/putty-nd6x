
#include "native_view_host.h"

#include "base/logging.h"

#include "ui_gfx/canvas.h"

#include "native_view_host_wrapper.h"
#include "native_view_host_view.h"
#include "view/widget/widget.h"

namespace view
{

// static
const char NativeViewHost::kViewClassName[] = "view/NativeViewHost";

// static
const bool NativeViewHost::kRenderNativeControlFocus = true;

////////////////////////////////////////////////////////////////////////////////
// NativeViewHost, public:

NativeViewHost::NativeViewHost()
    : native_view_(NULL),
      view_view_(NULL),
      fast_resize_(false),
      focus_view_(NULL) {}

NativeViewHost::~NativeViewHost() {}

void NativeViewHost::Attach(HWND native_view)
{
    DCHECK(native_view);
    DCHECK(!native_view_);
    DCHECK(!view_view_);
    native_view_ = native_view;
    // If set_focus_view() has not been invoked, this view is the one that should
    // be seen as focused when the native view receives focus.
    if(!focus_view_)
    {
        focus_view_ = this;
    }
    native_wrapper_->NativeViewAttached();
}

void NativeViewHost::AttachToView(View* view)
{
    if(view == view_view_)
    {
        return;
    }
    DCHECK(view);
    DCHECK(!native_view_);
    DCHECK(!view_view_);
    native_wrapper_.reset(new NativeViewHostView(this));
    view_view_ = view;
    // If set_focus_view() has not been invoked, this view is the one that should
    // be seen as focused when the native view receives focus.
    if(!focus_view_)
    {
        focus_view_ = this;
    }
    native_wrapper_->NativeViewAttached();
}

void NativeViewHost::Detach()
{
    Detach(false);
}

void NativeViewHost::SetPreferredSize(const gfx::Size& size)
{
    preferred_size_ = size;
    PreferredSizeChanged();
}

void NativeViewHost::NativeViewDestroyed()
{
    // Detach so we can clear our state and notify the native_wrapper_ to release
    // ref on the native view.
    Detach(true);
}

////////////////////////////////////////////////////////////////////////////////
// NativeViewHost, View overrides:

gfx::Size NativeViewHost::GetPreferredSize()
{
    return preferred_size_;
}

void NativeViewHost::Layout()
{
    if((!native_view_ && !view_view_) || !native_wrapper_.get())
    {
        return;
    }

    gfx::Rect vis_bounds = GetVisibleBounds();
    bool visible = !vis_bounds.IsEmpty();

    if(visible && !fast_resize_)
    {
        if(vis_bounds.size() != size())
        {
            // Only a portion of the Widget is really visible.
            int x = vis_bounds.x();
            int y = vis_bounds.y();
            native_wrapper_->InstallClip(x, y, vis_bounds.width(),
                                         vis_bounds.height());
        }
        else if(native_wrapper_->HasInstalledClip())
        {
            // The whole widget is visible but we installed a clip on the widget,
            // uninstall it.
            native_wrapper_->UninstallClip();
        }
    }

    if(visible)
    {
        // Since widgets know nothing about the View hierarchy (they are direct
        // children of the Widget that hosts our View hierarchy) they need to be
        // positioned in the coordinate system of the Widget, not the current
        // view.  Also, they should be positioned respecting the border insets
        // of the native view.
        gfx::Rect local_bounds = ConvertRectToWidget(GetContentsBounds());
        native_wrapper_->ShowWidget(local_bounds.x(), local_bounds.y(),
                                    local_bounds.width(), local_bounds.height());
    }
    else
    {
        native_wrapper_->HideWidget();
    }
}

void NativeViewHost::OnPaint(gfx::Canvas* canvas)
{
    // Paint background if there is one. NativeViewHost needs to paint
    // a background when it is hosted in a TabbedPane. For Gtk implementation,
    // NativeTabbedPaneGtk uses a WidgetGtk as page container and because
    // WidgetGtk hook "expose" with its root view's paint, we need to
    // fill the content. Otherwise, the tab page's background is not properly
    // cleared. For Windows case, it appears okay to not paint background because
    // we don't have a container window in-between. However if you want to use
    // customized background, then this becomes necessary.
    OnPaintBackground(canvas);

    // The area behind our window is black, so during a fast resize (where our
    // content doesn't draw over the full size of our native view, and the native
    // view background color doesn't show up), we need to cover that blackness
    // with something so that fast resizes don't result in black flash.
    //
    // It would be nice if this used some approximation of the page's
    // current background color.
    if(native_wrapper_->HasInstalledClip())
    {
        canvas->FillRectInt(SK_ColorWHITE, 0, 0, width(), height());
    }
}

void NativeViewHost::VisibilityChanged(View* starting_from, bool is_visible)
{
    Layout();
}

bool NativeViewHost::NeedsNotificationWhenVisibleBoundsChange() const
{
    // The native widget is placed relative to the root. As such, we need to
    // know when the position of any ancestor changes, or our visibility relative
    // to other views changed as it'll effect our position relative to the root.
    return true;
}

void NativeViewHost::OnVisibleBoundsChanged()
{
    Layout();
}

void NativeViewHost::ViewHierarchyChanged(bool is_add, View* parent,
        View* child)
{
    if(is_add && GetWidget())
    {
        if(!native_wrapper_.get())
        {
            native_wrapper_.reset(NativeViewHostWrapper::CreateWrapper(this));
        }
        native_wrapper_->AddedToWidget();
    }
    else if(!is_add)
    {
        native_wrapper_->RemovedFromWidget();
    }
}

std::string NativeViewHost::GetClassName() const
{
    return kViewClassName;
}

void NativeViewHost::OnFocus()
{
    native_wrapper_->SetFocus();
    GetWidget()->NotifyAccessibilityEvent(
        this, ui::AccessibilityTypes::EVENT_FOCUS, true);
}

IAccessible* NativeViewHost::GetNativeViewAccessible()
{
    if(native_wrapper_.get())
    {
        IAccessible* accessible_view =
            native_wrapper_->GetNativeViewAccessible();
        if(accessible_view)
        {
            return accessible_view;
        }
    }

    return View::GetNativeViewAccessible();
}

////////////////////////////////////////////////////////////////////////////////
// NativeViewHost, private:

void NativeViewHost::Detach(bool destroyed)
{
    DCHECK(native_view_ || view_view_);
    native_wrapper_->NativeViewDetaching(destroyed);
    native_view_ = NULL;
    view_view_ = NULL;
}

} //namespace view