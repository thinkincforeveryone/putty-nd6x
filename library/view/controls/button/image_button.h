
#ifndef __view_image_button_h__
#define __view_image_button_h__

#pragma once

#include "SkBitmap.h"

#include "custom_button.h"

namespace view
{

// An image button.

// Note that this type of button is not focusable by default and will not be
// part of the focus chain.  Call SetFocusable(true) to make it part of the
// focus chain.
class ImageButton : public CustomButton
{
public:
    explicit ImageButton(ButtonListener* listener);
    virtual ~ImageButton();

    // Set the image the button should use for the provided state.
    virtual void SetImage(ButtonState aState, const SkBitmap* anImage);

    // Set the background details.
    void SetBackground(SkColor color, const SkBitmap* image,
                       const SkBitmap* mask);

    enum HorizontalAlignment
    {
        ALIGN_LEFT = 0,
        ALIGN_CENTER,
        ALIGN_RIGHT,
    };

    enum VerticalAlignment
    {
        ALIGN_TOP = 0,
        ALIGN_MIDDLE,
        ALIGN_BOTTOM
    };

    // Sets how the image is laid out within the button's bounds.
    void SetImageAlignment(HorizontalAlignment h_align,
                           VerticalAlignment v_align);

    // Overridden from View:
    virtual gfx::Size GetPreferredSize();
    virtual void OnPaint(gfx::Canvas* canvas);

    // Sets preferred size, so it could be correctly positioned in layout even if
    // it is NULL.
    void SetPreferredSize(const gfx::Size& preferred_size)
    {
        preferred_size_ = preferred_size;
    }

protected:
    // Returns the image to paint. This is invoked from paint and returns a value
    // from images.
    virtual SkBitmap GetImageToPaint();

    // The images used to render the different states of this button.
    SkBitmap images_[BS_COUNT];

    // The background image.
    SkBitmap background_image_;

private:
    // Image alignment.
    HorizontalAlignment h_alignment_;
    VerticalAlignment v_alignment_;
    gfx::Size preferred_size_;

    DISALLOW_COPY_AND_ASSIGN(ImageButton);
};

////////////////////////////////////////////////////////////////////////////////
//
// ToggleImageButton
//
// A toggle-able ImageButton.  It swaps out its graphics when toggled.
//
////////////////////////////////////////////////////////////////////////////////
class ToggleImageButton : public ImageButton
{
public:
    explicit ToggleImageButton(ButtonListener* listener);
    virtual ~ToggleImageButton();

    // Change the toggled state.
    void SetToggled(bool toggled);

    // Like ImageButton::SetImage(), but to set the graphics used for the
    // "has been toggled" state.  Must be called for each button state
    // before the button is toggled.
    void SetToggledImage(ButtonState state, const SkBitmap* image);

    // Set the tooltip text displayed when the button is toggled.
    void SetToggledTooltipText(const string16& tooltip);

    // Overridden from ImageButton:
    virtual void SetImage(ButtonState aState, const SkBitmap* anImage);

    // Overridden from View:
    virtual bool GetTooltipText(const gfx::Point& p, string16* tooltip);

private:
    // The parent class's images_ member is used for the current images,
    // and this array is used to hold the alternative images.
    // We swap between the two when toggling.
    SkBitmap alternate_images_[BS_COUNT];

    // True if the button is currently toggled.
    bool toggled_;

    // The parent class's tooltip_text_ is displayed when not toggled, and
    // this one is shown when toggled.
    string16 toggled_tooltip_text_;

    DISALLOW_COPY_AND_ASSIGN(ToggleImageButton);
};

} //namespace view

#endif //__view_image_button_h__