
#include "NativeSftpController.h"
#include "NativeSftpPage.h"

#include "terminal.h"
#include "storage.h"
 
const LPCWSTR NativeSftpPage::WINTAB_SFTP_PAGE_CLASS = L"WintabSftpPage";

int NativeSftpPage::init(NativeSftpController* sftpController, Conf *cfg, HWND hwndParent)
{
    sftpController_ = sftpController;
    NativeSftpPage::registerPage();

    int winmode = WS_CHILD | WS_VSCROLL ;
    if (!conf_get_int(cfg, CONF_scrollbar))
        winmode &= ~(WS_VSCROLL);
    hwndCtrl = CreateWindowEx(
                   WS_EX_TOPMOST,
                   WINTAB_SFTP_PAGE_CLASS,
				   WINTAB_SFTP_PAGE_CLASS,
                   winmode,
                   0, 0, 0, 0,
                   hwndParent,
                   NULL,   /* hMenu */
                   hinst,
                   NULL);  /* lpParam */

    if (hwndCtrl == NULL)
    {
        ErrorExit("CreatePage");
        ExitProcess(2);
    }
    win_bind_data(hwndCtrl, this);
    return 0;

}
 
void NativeSftpPage::init_scrollbar(Terminal *term)
{
    SCROLLINFO si;

    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    si.nMin = 0;
    si.nMax = 49;
    si.nPage = 50;
    si.nPos = 0;
    SetScrollInfo(hwndCtrl, SB_VERT, &si, FALSE);
}

 
int NativeSftpPage::fini()
{
    DestroyWindow(hwndCtrl);
    return 0;
}

 
int NativeSftpPage::resize(const RECT *rc, const int cfg_winborder)
{
    RECT pc;
    int page_width = rc->right - rc->left;
    int page_height = rc->bottom - rc->top;
    MoveWindow(hwndCtrl, rc->left, rc->top,
               page_width,
               page_height, false);

    GetClientRect(hwndCtrl, &pc);
    extra_page_width = page_width - (pc.right - pc.left) + cfg_winborder*2;
    extra_page_height = page_height - (pc.bottom - pc.top) + cfg_winborder*2;
    return 0;
}


void NativeSftpPage::get_term_size(int *term_width, int *term_height)
{
    RECT rc;
    GetWindowRect(hwndCtrl, &rc);

    *term_width = rc.right - rc.left - extra_page_width;
    *term_height = rc.bottom - rc.top - extra_page_height;
}
 

//-----------------------------------------------------------------------
LRESULT CALLBACK NativeSftpPage::WndProc(HWND hwnd, UINT message,
	WPARAM wParam, LPARAM lParam)
{
	NativeSftpPage* page = (NativeSftpPage*)win_get_data(hwnd);
	if (page == NULL)
	{
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	NativeSftpController* puttyController = page->sftpController_;

	switch (message)
	{
	case WM_PAINT:
		
		break;
		
	default:
		break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

int NativeSftpPage::registerPage()
{  
    WNDCLASSEX wndclass;
    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hinst;
    wndclass.hIcon = NULL;
    wndclass.hCursor = NULL;
    wndclass.hbrBackground = CreateSolidBrush(0x00ffff00);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = WINTAB_SFTP_PAGE_CLASS;
    wndclass.hIconSm = 0;
    RegisterClassEx(&wndclass);
    return 0;
}

//-----------------------------------------------------------------------
 
int NativeSftpPage::unregisterPage()
{
    return UnregisterClass(WINTAB_SFTP_PAGE_CLASS, hinst);
}


