
#include "l10n_util_win.h"

#include <windowsx.h>
#include <algorithm>

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/string_number_conversions.h"
#include "base/win/windows_version.h"

#include "l10n_util.h"
#include "ui_base/resource/app_res_ids.h"

namespace
{

void AdjustLogFont(const std::wstring& font_family, double font_size_scaler,
                   LOGFONT* logfont)
{
    DCHECK(font_size_scaler > 0);
    font_size_scaler = std::max(std::min(font_size_scaler, 2.0), 0.7);
    logfont->lfHeight = static_cast<long>(font_size_scaler *
                                          static_cast<double>(abs(logfont->lfHeight))+0.5) *
                        (logfont->lfHeight>0?1:-1);

    // TODO(jungshik): We may want to check the existence of the font.
    // If it's not installed, we shouldn't adjust the font.
    if(font_family != L"default")
    {
        int name_len = std::min(static_cast<int>(font_family.size()),
                                LF_FACESIZE-1);
        memcpy(logfont->lfFaceName, font_family.data(), name_len*sizeof(WORD));
        logfont->lfFaceName[name_len] = 0;
    }
}

}

namespace ui
{

int GetExtendedStyles()
{
    return !base::i18n::IsRTL() ? 0 : WS_EX_LAYOUTRTL|WS_EX_RTLREADING;
}

int GetExtendedTooltipStyles()
{
    return !base::i18n::IsRTL() ? 0 : WS_EX_LAYOUTRTL;
}

void HWNDSetRTLLayout(HWND hwnd)
{
    DWORD ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);

    // We don't have to do anything if the style is already set for the HWND.
    if(!(ex_style & WS_EX_LAYOUTRTL))
    {
        ex_style |= WS_EX_LAYOUTRTL;
        ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);

        // Right-to-left layout changes are not applied to the window immediately
        // so we should make sure a WM_PAINT is sent to the window by invalidating
        // the entire window rect.
        ::InvalidateRect(hwnd, NULL, true);
    }
}

bool NeedOverrideDefaultUIFont(std::wstring* override_font_family,
                               double* font_size_scaler)
{
    // This is rather simple-minded to deal with the UI font size
    // issue for some Indian locales (ml, bn, hi) for which
    // the default Windows fonts are too small to be legible.  For those
    // locales, IDS_UI_FONT_FAMILY is set to an actual font family to
    // use while for other locales, it's set to 'default'.

    // XP and Vista or later have different font size issues and
    // we need separate ui font specifications.
    int ui_font_family_id = IDS_UI_FONT_FAMILY;
    int ui_font_size_scaler_id = IDS_UI_FONT_SIZE_SCALER;
    if(base::win::GetVersion() < base::win::VERSION_VISTA)
    {
        ui_font_family_id = IDS_UI_FONT_FAMILY_XP;
        ui_font_size_scaler_id = IDS_UI_FONT_SIZE_SCALER_XP;
    }

    std::wstring ui_font_family = GetStringUTF16(ui_font_family_id);
    int scaler100;
    if(!base::StringToInt(GetStringUTF16(ui_font_size_scaler_id), &scaler100))
    {
        return false;
    }

    // We use the OS default in two cases:
    // 1) The resource bundle has 'default' and '100' for font family and
    //    font scaler.
    // 2) The resource bundle is not available for some reason and
    //    ui_font_family is empty.
    if(ui_font_family==L"default" && scaler100==100 || ui_font_family.empty())
    {
        return false;
    }
    if(override_font_family && font_size_scaler)
    {
        override_font_family->swap(ui_font_family);
        *font_size_scaler = scaler100 / 100.0;
    }
    return true;
}

void AdjustUIFont(LOGFONT* logfont)
{
    std::wstring ui_font_family;
    double ui_font_size_scaler;
    if(NeedOverrideDefaultUIFont(&ui_font_family, &ui_font_size_scaler))
    {
        AdjustLogFont(ui_font_family, ui_font_size_scaler, logfont);
    }
}

void AdjustUIFontForWindow(HWND hwnd)
{
    std::wstring ui_font_family;
    double ui_font_size_scaler;
    if(NeedOverrideDefaultUIFont(&ui_font_family, &ui_font_size_scaler))
    {
        LOGFONT logfont;
        if(GetObject(GetWindowFont(hwnd), sizeof(logfont), &logfont))
        {
            AdjustLogFont(ui_font_family, ui_font_size_scaler, &logfont);
            HFONT hfont = CreateFontIndirect(&logfont);
            if(hfont)
            {
                SetWindowFont(hwnd, hfont, FALSE);
            }
        }
    }
}

} //namespace ui