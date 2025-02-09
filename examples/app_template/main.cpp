
#include <tchar.h>
#include <windows.h>
#include <initguid.h>
#include <oleacc.h>

#include <atlbase.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/message_loop.h"
#include "base/process_util.h"
#include "base/path_service.h"

#include "ui_base/resource/resource_bundle.h"
#include "ui_base/win/hwnd_util.h"

#include "view/focus/accelerator_handler.h"
#include "view/widget/widget.h"

#include "app_view_delegate.h"

#include "WindowView.h"

CComModule _Module;

// 程序入口.
int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HRESULT hRes = OleInitialize(NULL);

    // this resolves ATL window thunking problem when Microsoft Layer
    // for Unicode (MSLU) is used.
    ::DefWindowProc(NULL, 0, 0, 0L);

    AppViewDelegate view_delegate;

    CommandLine::Init(0, NULL);

    {
        base::EnableTerminationOnHeapCorruption();

        base::AtExitManager exit_manager;

        FilePath res_dll;
        PathService::Get(base::DIR_EXE, &res_dll);
        res_dll = res_dll.Append(L"wanui_res.dll");
        ui::ResourceBundle::InitSharedInstance(res_dll);

        MessageLoop main_message_loop(MessageLoop::TYPE_UI);

        view::Widget::CreateWindowWithBounds(new WindowView, gfx::Rect(800,480))->Show();
        //ui::CenterAndSizeWindow(NULL, window->GetNativeWindow(),
        //	gfx::Size(800, 400), false);

        view::AcceleratorHandler accelerator_handler;
        MessageLoopForUI::current()->Run(&accelerator_handler);
    }

    OleUninitialize();

    return 0;
}


// 提升公共控件样式.
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif