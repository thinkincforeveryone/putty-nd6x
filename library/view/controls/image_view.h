
#ifndef __view_image_view_h__
#define __view_image_view_h__

#pragma once

#include "SkBitmap.h"

#include "view/view.h"

namespace gfx
{
class Canvas;
}

namespace view
{

/////////////////////////////////////////////////////////////////////////////
//
// ImageView class.
//
// An ImageView can display an image from an SkBitmap. If a size is provided,
// the ImageView will resize the provided image to fit if it is too big or will
// center the image if smaller. Otherwise, the preferred size matches the
// provided image size.
//
/////////////////////////////////////////////////////////////////////////////
class ImageView : public View
{
public:
    enum Alignment
    {
        LEADING = 0,
        CENTER,
        TRAILING
    };

    ImageView();
    virtual ~ImageView();

    // Set the bitmap that should be displayed.
    void SetImage(const SkBitmap& bm);

    // Set the bitmap that should be displayed from a pointer. Reset the image
    // if the pointer is NULL. The pointer contents is copied in the receiver's
    // bitmap.
    void SetImage(SkBitmap* bm);

    // Returns the bitmap currently displayed or NULL of none is currently set.
    // The returned bitmap is still owned by the ImageView.
    const SkBitmap& GetImage();

    // Set the desired image size for the receiving ImageView.
    void SetImageSize(const gfx::Size& image_size);

    // Return the preferred size for the receiving view. Returns false if the
    // preferred size is not defined, which means that the view uses the image
    // size.
    bool GetImageSize(gfx::Size* image_size);

    // Returns the actual bounds of the visible image inside the view.
    gfx::Rect GetImageBounds() const;

    // Reset the image size to the current image dimensions.
    void ResetImageSize();

    // Set / Get the horizontal alignment.
    void SetHorizontalAlignment(Alignment ha);
    Alignment GetHorizontalAlignment();

    // Set / Get the vertical alignment.
    void SetVerticalAlignment(Alignment va);
    Alignment GetVerticalAlignment();

    // Set / Get the tooltip text.
    void SetTooltipText(const string16& tooltip);
    string16 GetTooltipText();

    // Overriden from View
    virtual gfx::Size GetPreferredSize();
    virtual void OnPaint(gfx::Canvas* canvas);
    virtual void GetAccessibleState(ui::AccessibleViewState* state);
    virtual bool GetTooltipText(const gfx::Point& p, string16* tooltip);

private:
    // Compute the image origin given the desired size and the receiver alignment
    // properties.
    gfx::Point ComputeImageOrigin(const gfx::Size& image_size) const;

    // Whether the image size is set.
    bool image_size_set_;

    // The actual image size.
    gfx::Size image_size_;

    // The underlying bitmap.
    SkBitmap image_;

    // Horizontal alignment.
    Alignment horiz_alignment_;

    // Vertical alignment.
    Alignment vert_alignment_;

    // The current tooltip text.
    string16 tooltip_text_;

    DISALLOW_COPY_AND_ASSIGN(ImageView);
};

} //namespace view

#endif //__view_image_view_h__