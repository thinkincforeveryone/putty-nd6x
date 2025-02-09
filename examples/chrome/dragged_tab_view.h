
#ifndef __dragged_tab_view_h__
#define __dragged_tab_view_h__

#pragma once

#include <vector>

#include "ui_gfx/point.h"
#include "ui_gfx/rect.h"
#include "ui_gfx/size.h"

#include "view/view.h"

class NativeViewPhotobooth;

class DraggedTabView : public view::View
{
public:
    // Creates a new DraggedTabView using |renderers| as the Views. DraggedTabView
    // takes ownership of the views in |renderers| and |photobooth|.
    DraggedTabView(const std::vector<view::View*>& renderers,
                   const std::vector<gfx::Rect>& renderer_bounds,
                   const gfx::Point& mouse_tab_offset,
                   const gfx::Size& contents_size,
                   NativeViewPhotobooth* photobooth);
    virtual ~DraggedTabView();

    // Moves the DraggedTabView to the appropriate location given the mouse
    // pointer at |screen_point|.
    void MoveTo(const gfx::Point& screen_point);

    // Sets the offset of the mouse from the upper left corner of the tab.
    void set_mouse_tab_offset(const gfx::Point& offset)
    {
        mouse_tab_offset_ = offset;
    }

    // Notifies the DraggedTabView that it should update itself.
    void Update();

private:
    // Overridden from views::View:
    virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;
    virtual void Layout() OVERRIDE;
    virtual gfx::Size GetPreferredSize() OVERRIDE;

    // Paint the view, when it's not attached to any TabStrip.
    void PaintDetachedView(gfx::Canvas* canvas);

    // Paint the view, when "Show window contents while dragging" is disabled.
    void PaintFocusRect(gfx::Canvas* canvas);

    // Returns the preferred size of the container.
    gfx::Size PreferredContainerSize();

    // Utility for scaling a size by the current scaling factor.
    int ScaleValue(int value);

    // The window that contains the DraggedTabView.
    scoped_ptr<view::Widget> container_;

    // The renderer that paints the Tab shape.
    std::vector<view::View*> renderers_;

    // Bounds of the renderers.
    std::vector<gfx::Rect> renderer_bounds_;

    // True if "Show window contents while dragging" is enabled.
    bool show_contents_on_drag_;

    // The unscaled offset of the mouse from the top left of the dragged Tab.
    // This is used to maintain an appropriate offset for the mouse pointer when
    // dragging scaled and unscaled representations, and also to calculate the
    // position of detached windows.
    gfx::Point mouse_tab_offset_;

    // The size of the tab renderer.
    gfx::Size tab_size_;

    // A handle to the DIB containing the current screenshot of the TabContents
    // we are dragging.
    scoped_ptr<NativeViewPhotobooth> photobooth_;

    // Size of the TabContents being dragged.
    gfx::Size contents_size_;

    DISALLOW_COPY_AND_ASSIGN(DraggedTabView);
};

#endif //__dragged_tab_view_h__