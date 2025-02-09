
#ifndef __native_view_photobooth_win_h__
#define __native_view_photobooth_win_h__

#pragma once

#include "base/basic_types.h"

#include "native_view_photobooth.h"

namespace view
{
class Widget;
}

///////////////////////////////////////////////////////////////////////////////
// HWNDPhotobooth
//
//  An object that a HWND "steps into" to have its picture taken. This is used
//  to generate a full size screen shot of the contents of a HWND including
//  any child windows.
//
//  Implementation note: This causes the HWND to be re-parented to a mostly
//  off-screen layered window.
//
class NativeViewPhotoboothWin : public NativeViewPhotobooth
{
public:
    // Creates the photo booth. Constructs a nearly off-screen window, parents
    // the HWND, then shows it. The caller is responsible for destroying this
    // window, since the photo-booth will detach it before it is destroyed.
    // |canvas| is a canvas to paint the contents into, and dest_bounds is the
    // target area in |canvas| to which painted contents will be clipped.
    explicit NativeViewPhotoboothWin(HWND initial_view);

    // Destroys the photo booth window.
    virtual ~NativeViewPhotoboothWin();

    // Replaces the view in the photo booth with the specified one.
    virtual void Replace(HWND new_view) OVERRIDE;

    // Paints the current display image of the window into |canvas|, clipped to
    // |target_bounds|.
    virtual void PaintScreenshotIntoCanvas(
        gfx::Canvas* canvas,
        const gfx::Rect& target_bounds) OVERRIDE;

private:
    // Creates a mostly off-screen window to contain the HWND to be captured.
    void CreateCaptureWindow(HWND initial_hwnd);

    // The nearly off-screen photo-booth layered window used to hold the HWND.
    view::Widget* capture_window_;

    // The current HWND being captured.
    HWND current_hwnd_;

    DISALLOW_COPY_AND_ASSIGN(NativeViewPhotoboothWin);
};

#endif //__native_view_photobooth_win_h__