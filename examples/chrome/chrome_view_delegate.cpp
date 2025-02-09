
#include "chrome_view_delegate.h"

#include "base/logging.h"

#include "ui_gfx/icon_util.h"
#include "ui_gfx/image/image.h"

#include "ui_base/clipboard/clipboard.h"
#include "ui_base/resource/resource_bundle.h"

#include "../wanui_res/resource.h"

ChromeViewDelegate::ChromeViewDelegate() : default_parent_view_(NULL)
{
    DCHECK(!view::ViewDelegate::view_delegate);
    view::ViewDelegate::view_delegate = this;
}

ChromeViewDelegate::~ChromeViewDelegate()
{
    view::ViewDelegate::view_delegate = NULL;
}

ui::Clipboard* ChromeViewDelegate::GetClipboard() const
{
    if(!clipboard_.get())
    {
        // Note that we need a MessageLoop for the next call to work.
        clipboard_.reset(new ui::Clipboard());
    }
    return clipboard_.get();
}

view::View* ChromeViewDelegate::GetDefaultParentView()
{
    return default_parent_view_;
}

void ChromeViewDelegate::SaveWindowPlacement(
    const view::Widget* widget,
    const std::wstring& window_name,
    const gfx::Rect& bounds,
    ui::WindowShowState show_state) {}

bool ChromeViewDelegate::GetSavedWindowPlacement(
    const std::wstring& window_name,
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const
{
    return false;
}

HICON ChromeViewDelegate::GetDefaultWindowIcon() const
{
    return IconUtil::CreateHICONFromSkBitmap(
               ui::ResourceBundle::GetSharedInstance().GetImageNamed(
                   IDR_PRODUCT_LOGO_32));
}

int ChromeViewDelegate::GetDispositionForEvent(int event_flags)
{
    return 0;
}