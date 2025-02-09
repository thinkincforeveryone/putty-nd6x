
#ifndef __view_textfield_h__
#define __view_textfield_h__

#pragma once

#include "base/logging.h"

#include "ui_gfx/font.h"

#include "ui_base/ime/text_input_type.h"

#include "native_textfield_wrapper.h"
#include "view/view.h"

namespace ui
{
class Range;
}

namespace view
{

class KeyEvent;
class TextfieldController;

// This class implements a View that wraps a native text (edit) field.
class Textfield : public View
{
public:
    // The textfield's class name.
    static const char kViewClassName[];

    enum StyleFlags
    {
        STYLE_DEFAULT   = 0,
        STYLE_PASSWORD  = 1 << 0,
        STYLE_MULTILINE = 1 << 1,
        STYLE_LOWERCASE = 1 << 2
    };

    Textfield();
    explicit Textfield(StyleFlags style);
    virtual ~Textfield();

    // TextfieldController accessors
    void SetController(TextfieldController* controller);
    TextfieldController* GetController() const;

    // Gets/Sets whether or not the Textfield is read-only.
    bool read_only() const
    {
        return read_only_;
    }
    void SetReadOnly(bool read_only);

    // Gets/Sets whether or not this Textfield is a password field.
    // TODO(bryeung): Currently this is only used in
    // chrome/browser/chromeos/options/wifi_config_view.cc, which is being
    // converted to WebUI.  Please remove this when that happens.
    bool IsPassword() const;
    void SetPassword(bool password);

    // Gets/Sets the input type of this textfield.
    ui::TextInputType GetTextInputType() const;
    void SetTextInputType(ui::TextInputType type);

    // Gets the text currently displayed in the Textfield.
    const string16& text() const
    {
        return text_;
    }

    // Sets the text currently displayed in the Textfield.  This doesn't
    // change the cursor position if the current cursor is within the
    // new text's range, or moves the cursor to the end if the cursor is
    // out of the new text's range.
    void SetText(const string16& text);

    // Appends the given string to the previously-existing text in the field.
    void AppendText(const string16& text);

    // Returns the text that is currently selected.
    string16 GetSelectedText() const;

    // Causes the edit field to be fully selected.
    void SelectAll();

    // Clears the selection within the edit field and sets the caret to the end.
    void ClearSelection() const;

    // Checks if there is any selected text.
    bool HasSelection() const;

    // Accessor for |style_|.
    StyleFlags style() const
    {
        return style_;
    }

    // Gets/Sets the text color to be used when painting the Textfield.
    // Call |UseDefaultTextColor| to return to the system default colors.
    SkColor text_color() const
    {
        return text_color_;
    }
    void SetTextColor(SkColor color);

    // Gets/Sets whether the default text color should be used when painting the
    // Textfield.
    bool use_default_text_color() const
    {
        return use_default_text_color_;
    }
    void UseDefaultTextColor();

    // Gets/Sets the background color to be used when painting the Textfield.
    // Call |UseDefaultBackgroundColor| to return to the system default colors.
    SkColor background_color() const
    {
        return background_color_;
    }
    void SetBackgroundColor(SkColor color);

    // Gets/Sets whether the default background color should be used when painting
    // the Textfield.
    bool use_default_background_color() const
    {
        return use_default_background_color_;
    }
    void UseDefaultBackgroundColor();

    // Gets/Sets the font used when rendering the text within the Textfield.
    const gfx::Font& font() const
    {
        return font_;
    }
    void SetFont(const gfx::Font& font);

    // Sets the left and right margin (in pixels) within the text box. On Windows
    // this is accomplished by packing the left and right margin into a single
    // 32 bit number, so the left and right margins are effectively 16 bits.
    void SetHorizontalMargins(int left, int right);

    // Sets the top and bottom margins (in pixels) within the textfield.
    // NOTE: in most cases height could be changed instead.
    void SetVerticalMargins(int top, int bottom);

    // Sets the default width of the text control. See default_width_in_chars_.
    void set_default_width_in_chars(int default_width)
    {
        default_width_in_chars_ = default_width;
    }

    // Removes the border from the edit box, giving it a 2D look.
    bool draw_border() const
    {
        return draw_border_;
    }
    void RemoveBorder();

    // Sets the text to display when empty.
    void set_text_to_display_when_empty(const string16& text)
    {
        text_to_display_when_empty_ = text;
        NOTIMPLEMENTED();
    }
    const string16& text_to_display_when_empty()
    {
        return text_to_display_when_empty_;
    }

    // Getter for the horizontal margins that were set. Returns false if
    // horizontal margins weren't set.
    bool GetHorizontalMargins(int* left, int* right);

    // Getter for the vertical margins that were set. Returns false if vertical
    // margins weren't set.
    bool GetVerticalMargins(int* top, int* bottom);

    // Updates all properties on the textfield. This is invoked internally.
    // Users of Textfield never need to invoke this directly.
    void UpdateAllProperties();

    // Invoked by the edit control when the value changes. This method set
    // the text_ member variable to the value contained in edit control.
    // This is important because the edit control can be replaced if it has
    // been deleted during a window close.
    void SyncText();

    // Returns whether or not an IME is composing text.
    bool IsIMEComposing() const;

    // Gets the selected range. This is views-implementation only and
    // has to be called after the wrapper is created.
    void GetSelectedRange(ui::Range* range) const;

    // Selects the text given by |range|. This is views-implementation only and
    // has to be called after the wrapper is created.
    void SelectRange(const ui::Range& range);

    // Returns the current cursor position. This is views-implementation
    // only and has to be called after the wrapper is created.
    size_t GetCursorPosition() const;

    // Set the accessible name of the text field.
    void SetAccessibleName(const string16& name);

    // Provided only for testing:
    HWND GetTestingHandle() const
    {
        return native_wrapper_ ? native_wrapper_->GetTestingHandle() : NULL;
    }
    NativeTextfieldWrapper* GetNativeWrapperForTesting() const
    {
        return native_wrapper_;
    }

    // Overridden from View:
    virtual void Layout();
    virtual gfx::Size GetPreferredSize();
    virtual bool IsFocusable() const;
    virtual void AboutToRequestFocusFromTabTraversal(bool reverse);
    virtual bool SkipDefaultKeyEventProcessing(const KeyEvent& e);
    virtual void OnEnabledChanged();
    virtual void OnPaintBackground(gfx::Canvas* canvas);
    virtual void OnPaintFocusBorder(gfx::Canvas* canvas);
    virtual bool OnKeyPressed(const KeyEvent& e);
    virtual bool OnKeyReleased(const KeyEvent& e);
    virtual void OnFocus();
    virtual void OnBlur();
    virtual void GetAccessibleState(ui::AccessibleViewState* state);

protected:
    virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
    virtual std::string GetClassName() const;

    // The object that actually implements the native text field.
    NativeTextfieldWrapper* native_wrapper_;

private:
    // This is the current listener for events from this Textfield.
    TextfieldController* controller_;

    // The mask of style options for this Textfield.
    StyleFlags style_;

    // The font used to render the text in the Textfield.
    gfx::Font font_;

    // The text displayed in the Textfield.
    string16 text_;

    // True if this Textfield cannot accept input and is read-only.
    bool read_only_;

    // The default number of average characters for the width of this text field.
    // This will be reported as the "desired size". Defaults to 0.
    int default_width_in_chars_;

    // Whether the border is drawn.
    bool draw_border_;

    // The text color to be used when painting the Textfield, provided
    // |use_default_text_color_| is set to false.
    SkColor text_color_;

    // When true, the system text color for Textfields is used when painting this
    // Textfield. When false, the value of |text_color_| determines the
    // Textfield's text color.
    bool use_default_text_color_;

    // The background color to be used when painting the Textfield, provided
    // |use_default_background_color_| is set to false.
    SkColor background_color_;

    // When true, the system background color for Textfields is used when painting
    // this Textfield. When false, the value of |background_color_| determines the
    // Textfield's background color.
    bool use_default_background_color_;

    // TODO(beng): remove this once NativeTextfieldWin subclasses
    //             NativeControlWin.
    bool initialized_;

    // Holds inner textfield margins.
    gfx::Insets margins_;

    // Holds whether margins were set.
    bool horizontal_margins_were_set_;
    bool vertical_margins_were_set_;

    // Text to display when empty.
    string16 text_to_display_when_empty_;

    // The accessible name of the text field.
    string16 accessible_name_;

    // The input type of this text field.
    ui::TextInputType text_input_type_;

    DISALLOW_COPY_AND_ASSIGN(Textfield);
};

} //namespace view

#endif //__view_textfield_h__