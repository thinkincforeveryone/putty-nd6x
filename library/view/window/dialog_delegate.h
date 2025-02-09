
#ifndef __view_dialog_delegate_h__
#define __view_dialog_delegate_h__

#pragma once

#include "ui_base/message_box_flags.h"

#include "dialog_client_view.h"
#include "view/widget/widget_delegate.h"

namespace view
{

class View;

///////////////////////////////////////////////////////////////////////////////
//
// DialogDelegate
//
//  DialogDelegate is an interface implemented by objects that wish to show a
//  dialog box Window. The window that is displayed uses this interface to
//  determine how it should be displayed and notify the delegate object of
//  certain events.
//
///////////////////////////////////////////////////////////////////////////////
class DialogDelegate : public WidgetDelegate
{
public:
    virtual DialogDelegate* AsDialogDelegate();

    // Returns a mask specifying which of the available DialogButtons are visible
    // for the dialog. Note: If an OK button is provided, you should provide a
    // CANCEL button. A dialog box with just an OK button is frowned upon and
    // considered a very special case, so if you're planning on including one,
    // you should reconsider, or beng says there will be stabbings.
    //
    // To use the extra button you need to override GetDialogButtons()
    virtual int GetDialogButtons() const;

    // Returns whether accelerators are enabled on the button. This is invoked
    // when an accelerator is pressed, not at construction time. This
    // returns true.
    virtual bool AreAcceleratorsEnabled(
        ui::MessageBoxFlags::DialogButton button);

    // Returns the label of the specified DialogButton.
    virtual std::wstring GetDialogButtonLabel(
        ui::MessageBoxFlags::DialogButton button) const;

    // Override this function if with a view which will be shown in the same
    // row as the OK and CANCEL buttons but flush to the left and extending
    // up to the buttons.
    virtual View* GetExtraView();

    // Returns whether the height of the extra view should be at least as tall as
    // the buttons. The default (false) is to give the extra view it's preferred
    // height. By returning true the height becomes
    // max(extra_view preferred height, buttons preferred height).
    virtual bool GetSizeExtraViewHeightToButtons();

    // Returns the default dialog button. This should not be a mask as only
    // one button should ever be the default button.  Return
    // MessageBoxFlags::DIALOGBUTTON_NONE if there is no default.  Default
    // behavior is to return MessageBoxFlags::DIALOGBUTTON_OK or
    // MessageBoxFlags::DIALOGBUTTON_CANCEL (in that order) if they are
    // present, MessageBoxFlags::DIALOGBUTTON_NONE otherwise.
    virtual int GetDefaultDialogButton() const;

    // Returns whether the specified dialog button is enabled.
    virtual bool IsDialogButtonEnabled(
        ui::MessageBoxFlags::DialogButton button) const;

    // Returns whether the specified dialog button is visible.
    virtual bool IsDialogButtonVisible(
        ui::MessageBoxFlags::DialogButton button) const;

    // For Dialog boxes, if there is a "Cancel" button, this is called when the
    // user presses the "Cancel" button or the Close button on the window or
    // in the system menu, or presses the Esc key. This function should return
    // true if the window can be closed after it returns, or false if it must
    // remain open.
    virtual bool Cancel();

    // For Dialog boxes, this is called when the user presses the "OK" button,
    // or the Enter key.  Can also be called on Esc key or close button
    // presses if there is no "Cancel" button.  This function should return
    // true if the window can be closed after the window can be closed after
    // it returns, or false if it must remain open.  If |window_closing| is
    // true, it means that this handler is being called because the window is
    // being closed (e.g.  by Window::Close) and there is no Cancel handler,
    // so Accept is being called instead.
    virtual bool Accept(bool window_closing);
    virtual bool Accept();

    // Overridden from WindowDelegate:
    virtual View* GetInitiallyFocusedView();
    virtual ClientView* CreateClientView(Widget* widget);

    // Called when the window has been closed.
    virtual void OnClose() {}

    // A helper for accessing the DialogClientView object contained by this
    // delegate's Window.
    const DialogClientView* GetDialogClientView() const;
    DialogClientView* GetDialogClientView();

protected:
    // Overridden from WindowDelegate:
    virtual ui::AccessibilityTypes::Role GetAccessibleWindowRole() const;
};

// A DialogDelegate implementation that is-a View. Used to override GetWidget()
// to call View's GetWidget() for the common case where a DialogDelegate
// implementation is-a View.
class DialogDelegateView : public DialogDelegate, public View
{
public:
    DialogDelegateView();
    virtual ~DialogDelegateView();

    // Overridden from DialogDelegate:
    virtual Widget* GetWidget();
    virtual const Widget* GetWidget() const;

private:
    DISALLOW_COPY_AND_ASSIGN(DialogDelegateView);
};

} //namespace view

#endif //__view_dialog_delegate_h__