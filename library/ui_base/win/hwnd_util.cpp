
#include "hwnd_util.h"

#include "base/i18n/rtl.h"
#include "base/string_util.h"

#include "ui_gfx/rect.h"
#include "ui_gfx/size.h"

namespace
{

// Adjust the window to fit, returning true if the window was resized or moved.
bool AdjustWindowToFit(HWND hwnd, const RECT& bounds)
{
    // Get the monitor.
    HMONITOR hmon = MonitorFromRect(&bounds, MONITOR_DEFAULTTONEAREST);
    if(!hmon)
    {
        NOTREACHED() << "Unable to find default monitor";
        // No monitor available.
        return false;
    }

    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hmon, &mi);
    gfx::Rect window_rect(bounds);
    gfx::Rect monitor_rect(mi.rcWork);
    gfx::Rect new_window_rect = window_rect.AdjustToFit(monitor_rect);
    if(!new_window_rect.Equals(window_rect))
    {
        // Window doesn't fit on monitor, move and possibly resize.
        SetWindowPos(hwnd, 0, new_window_rect.x(), new_window_rect.y(),
                     new_window_rect.width(), new_window_rect.height(),
                     SWP_NOACTIVATE|SWP_NOZORDER);
        return true;
    }
    else
    {
        return false;
    }
}

}

namespace ui
{

std::wstring GetClassName(HWND window)
{
    // 如果接收缓冲区不是足够大, GetClassNameW会返回截断的结果(保证null结尾).
    // 所以, 没法确定整个类名的长度.
    DWORD buffer_size = MAX_PATH;
    while(true)
    {
        std::wstring output;
        DWORD size_ret = GetClassNameW(window,
                                       WriteInto(&output, buffer_size), buffer_size);
        if(size_ret == 0)
        {
            break;
        }
        if(size_ret < (buffer_size-1))
        {
            output.resize(size_ret);
            return output;
        }
        buffer_size *= 2;
    }
    return std::wstring(); // 错误.
}

#pragma warning(push)
#pragma warning(disable:4312 4244)
WNDPROC SetWindowProc(HWND hwnd, WNDPROC proc)
{
    // 不能直接使用SetwindowLongPtr()函数的返回值, 原因是它返回的是原始窗口过程而
    // 不是当前窗口过程. 不知道这是一个bug还是有意而为.
    WNDPROC oldwindow_proc = reinterpret_cast<WNDPROC>(
                                 GetWindowLongPtr(hwnd, GWLP_WNDPROC));
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc));
    return oldwindow_proc;
}

void* SetWindowUserData(HWND hwnd, void* user_data)
{
    return reinterpret_cast<void*>(SetWindowLongPtr(hwnd,
                                   GWLP_USERDATA, reinterpret_cast<LONG_PTR>(user_data)));
}

void* GetWindowUserData(HWND hwnd)
{
    return reinterpret_cast<void*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}
#pragma warning(pop)

bool DoesWindowBelongToActiveWindow(HWND window)
{
    DCHECK(window);
    HWND top_window = ::GetAncestor(window, GA_ROOT);
    if(!top_window)
    {
        return false;
    }

    HWND active_top_window = ::GetAncestor(::GetForegroundWindow(), GA_ROOT);
    return (top_window == active_top_window);
}

void CenterAndSizeWindow(HWND parent, HWND window, const gfx::Size& pref,
                         bool pref_is_client)
{
    DCHECK(window && pref.width()>0 && pref.height()>0);

    // Calculate the ideal bounds.
    RECT window_bounds;
    RECT center_bounds = { 0 };
    if(parent)
    {
        // If there is a parent, center over the parents bounds.
        ::GetWindowRect(parent, &center_bounds);
    }
    else
    {
        // No parent. Center over the monitor the window is on.
        HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        if(monitor)
        {
            MONITORINFO mi = { 0 };
            mi.cbSize = sizeof(mi);
            GetMonitorInfo(monitor, &mi);
            center_bounds = mi.rcWork;
        }
        else
        {
            NOTREACHED() << "Unable to get default monitor";
        }
    }
    window_bounds.left = center_bounds.left +
                         (center_bounds.right - center_bounds.left - pref.width()) / 2;
    window_bounds.right = window_bounds.left + pref.width();
    window_bounds.top = center_bounds.top +
                        (center_bounds.bottom - center_bounds.top - pref.height()) / 2;
    window_bounds.bottom = window_bounds.top + pref.height();

    // If we're centering a child window, we are positioning in client
    // coordinates, and as such we need to offset the target rectangle by the
    // position of the parent window.
    if(::GetWindowLong(window, GWL_STYLE) & WS_CHILD)
    {
        DCHECK(parent && ::GetParent(window)==parent);
        POINT topleft = { window_bounds.left, window_bounds.top };
        ::MapWindowPoints(HWND_DESKTOP, parent, &topleft, 1);
        window_bounds.left = topleft.x;
        window_bounds.top = topleft.y;
        window_bounds.right = window_bounds.left + pref.width();
        window_bounds.bottom = window_bounds.top + pref.height();
    }

    // Get the WINDOWINFO for window. We need the style to calculate the size
    // for the window.
    WINDOWINFO win_info = { 0 };
    win_info.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(window, &win_info);

    // Calculate the window size needed for the content size.

    if(!pref_is_client || AdjustWindowRectEx(&window_bounds, win_info.dwStyle,
            FALSE, win_info.dwExStyle))
    {
        if(!AdjustWindowToFit(window, window_bounds))
        {
            // The window fits, reset the bounds.
            SetWindowPos(window, 0, window_bounds.left, window_bounds.top,
                         window_bounds.right-window_bounds.left,
                         window_bounds.bottom-window_bounds.top,
                         SWP_NOACTIVATE | SWP_NOZORDER);
        } // else case, AdjustWindowToFit set the bounds for us.
    }
    else
    {
        NOTREACHED() << "Unable to adjust window to fit";
    }
}

void CheckWindowCreated(HWND hwnd)
{
    if(!hwnd)
    {
        LOG_GETLASTERROR(FATAL);
    }
}

void ShowSystemMenu(HWND window, int screen_x, int screen_y)
{
    UINT flags = TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD;
    if(base::i18n::IsRTL())
    {
        flags |= TPM_RIGHTALIGN;
    }
    HMENU system_menu = GetSystemMenu(window, FALSE);
    int command = TrackPopupMenu(system_menu, flags, screen_x, screen_y, 0,
                                 window, NULL);
    if(command)
    {
        SendMessage(window, WM_SYSCOMMAND, command, 0);
    }
}

} //namespace ui