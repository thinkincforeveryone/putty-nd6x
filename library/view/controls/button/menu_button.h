
#ifndef __view_menu_button_h__
#define __view_menu_button_h__

#pragma once

#include "text_button.h"

namespace view
{

class ViewMenuDelegate;

////////////////////////////////////////////////////////////////////////////////
//
// MenuButton
//
//  A button that shows a menu when the left mouse button is pushed
//
////////////////////////////////////////////////////////////////////////////////
class MenuButton : public TextButton
{
public:
    // The menu button's class name.
    static const char kViewClassName[];

    // Create a Button.
    MenuButton(ButtonListener* listener,
               const std::wstring& text,
               ViewMenuDelegate* menu_delegate,
               bool show_menu_marker);
    virtual ~MenuButton();

    void set_menu_marker(const SkBitmap* menu_marker)
    {
        menu_marker_ = menu_marker;
    }

    const gfx::Point& menu_offset() const
    {
        return menu_offset_;
    }
    void set_menu_offset(int x, int y)
    {
        menu_offset_.SetPoint(x, y);
    }

    // Activate the button (called when the button is pressed).
    virtual bool Activate();

    // Overridden from TextButton for the potential use of a drop marker.
    virtual void PaintButton(gfx::Canvas* canvas, PaintButtonMode mode);

    // Overridden from View:
    virtual gfx::Size GetPreferredSize();
    virtual std::string GetClassName() const;
    virtual bool OnMousePressed(const MouseEvent& event);
    virtual void OnMouseReleased(const MouseEvent& event);
    virtual void OnMouseExited(const MouseEvent& event);
    virtual bool OnKeyPressed(const KeyEvent& event);
    virtual bool OnKeyReleased(const KeyEvent& event);
    virtual void GetAccessibleState(ui::AccessibleViewState* state);

protected:
    // True if the menu is currently visible.
    bool menu_visible_;

    // Offset of the associated menu position.
    gfx::Point menu_offset_;

private:
    friend class TextButtonBackground;

    // Compute the maximum X coordinate for the current screen. MenuButtons
    // use this to make sure a menu is never shown off screen.
    int GetMaximumScreenXCoordinate();

    // We use a time object in order to keep track of when the menu was closed.
    // The time is used for simulating menu behavior for the menu button; that
    // is, if the menu is shown and the button is pressed, we need to close the
    // menu. There is no clean way to get the second click event because the
    // menu is displayed using a modal loop and, unlike regular menus in Windows,
    // the button is not part of the displayed menu.
    base::Time menu_closed_time_;

    // The associated menu's resource identifier.
    ViewMenuDelegate* menu_delegate_;

    // Whether or not we're showing a drop marker.
    bool show_menu_marker_;

    // The down arrow used to differentiate the menu button from normal
    // text buttons.
    const SkBitmap* menu_marker_;

    // If non-null the destuctor sets this to true. This is set while the menu is
    // showing and used to detect if the menu was deleted while running.
    bool* destroyed_flag_;

    DISALLOW_COPY_AND_ASSIGN(MenuButton);
};

} //namespace view

#endif //__view_menu_button_h__