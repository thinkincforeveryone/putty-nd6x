#include "view/view.h"
#include "view/widget/widget.h"
#include "view/focus/focus_manager.h"
#include "tab_strip.h"

#include "window_interface.h"
#include "NativeSftpController.h" 
#include "NativeSftpPage.h"
#include "native_putty_common.h"
#include "CmdLineHandler.h"
#include "putty_global_config.h"
#include "SftpCallback.h"

#include "terminal.h"
#include "storage.h"

#include "atlconv.h"

#include "Mmsystem.h"

#include "zmodem_session.h"
#include "PuttyFileDialog.h"
#include "putty_view.h"

extern int is_session_log_enabled(void *handle);
extern void log_restart(void *handle, Conf *cfg);
extern void log_stop(void *handle, Conf *cfg);

DECL_WINDOWS_FUNCTION(static, BOOL, FlashWindowEx, (PFLASHWINFO));
extern void init_flashwindow();
extern BOOL flash_window_ex(HWND hwnd, DWORD dwFlags, UINT uCount, DWORD dwTimeout);
extern int is_shift_pressed(void);

HMENU NativeSftpController::popup_menu = NULL;
int NativeSftpController::kbd_codepage = 0;
base::Lock NativeSftpController::socketTreeLock_; 


NativeSftpController::NativeSftpController(Conf *theCfg, view::View* theView)
{
    USES_CONVERSION;
    //zSession_ = new ZmodemSession(this);
    view_ = theView;
    adjust_host(theCfg);
    cfg = conf_copy(theCfg);
    page_ = NULL;
    must_close_tab_ = FALSE;

    //set_input_locale(GetKeyboardLayout(0));
    last_mousemove = 0;

    term = NULL;
	m_SftpHdc = NULL;
    send_raw_mouse = 0;
    wheel_accumulator = 0;
    busy_status = BUSY_NOT;
    compose_state = 0;
    offset_width = offset_height = conf_get_int(cfg, CONF_window_border);
    caret_x = -1;
    caret_y = -1;
    n_specials = 0;
    specials = NULL;
    specials_menu = NULL;
    extra_width = 25;
    extra_height = 28;
    font_width = 10;
    font_height = 20;
    font_height_by_wheel = 0;
    offset_width = offset_height = conf_get_int(cfg, CONF_window_border);
    lastact = MA_NOTHING;
    lastbtn = MBT_NOTHING;
    dbltime = GetDoubleClickTime();
    offset_width = conf_get_int(cfg, CONF_window_border);
    offset_height = conf_get_int(cfg, CONF_window_border);
    ignore_clip = FALSE;
    hRgn = NULL;
    hCloserRgn = NULL;
    logbox = NULL;
    nevents = 0;
    negsize = 0;
    events = NULL;
    window_name = icon_name = NULL;
    ldisc = NULL;
    pal = NULL;
    logpal = NULL;
    closerX = 0;
    closerY = 0;
    char* session_name = conf_get_str(cfg, CONF_session_name);
    char *disrawname = strrchr(session_name, '#');
    disrawname = (disrawname == NULL)? session_name : (disrawname + 1);
    strncpy(disRawName, disrawname, 256);
    disName = A2W(disrawname);
    close_mutex= CreateMutex(NULL, FALSE, NULL);
    backend_state = LOADING;
    isClickingOnPage = false;
    conf_set_int(cfg, CONF_is_enable_shortcut, true);
    next_flash = 0;
    flashing = 0;
    cursor_visible = 1;
    forced_visible = FALSE;
    nativeParentWin_ = NULL;
    isAutoCmdEnabled_ = TRUE;
}

NativeSftpController::~NativeSftpController()
{ 
}
 
 

UINT NativeSftpController::wm_mousewheel = WM_MOUSEWHEEL;
int NativeSftpController::init(HWND hwndParent)
{ 
    page_ = new NativeSftpPage();
    page_->init(this, cfg, hwndParent);
	
	//StartSftp();
	backend_state = CONNECTED;
    return 0;
}

void NativeSftpController::notifyMsg(const char* msg, void* data)
{

}

HWND NativeSftpController::getNativePage()
{
	return page_->getWinHandler();
}


void NativeSftpController::parentChanged(view::View* parent)
{
	if (parent
		&& parent->GetWidget()
		&& parent->GetWidget()->GetTopLevelWidget()
		&& (nativeParentWin_ = parent->GetWidget()->GetTopLevelWidget()->GetNativeView()))
	{
		hTopWnd = nativeParentWin_;
		if (NULL == page_)
		{
			init(nativeParentWin_);
		}
		HWND pageHwnd = page_->getWinHandler();
		SetParent(pageHwnd, nativeParentWin_);
		view_->Layout(); 
		ShowWindow(pageHwnd, SW_SHOW);
		InvalidateRect(nativeParentWin_, NULL, TRUE);
		//update_mouse_pointer();
		parent->GetWidget()->GetTopLevelWidget()->Activate();
		assert(IsChild(nativeParentWin_, pageHwnd));
	}
	else
	{
		ShowWindow(page_->getWinHandler(), SW_HIDE);
		SetParent(page_->getWinHandler(), NULL);
	}
}


#undef IsMinimized
void NativeSftpController::setPagePos(const RECT* rc)
{
	bool getWidget = view_->GetWidget() != NULL;
	bool getTopWindow = getWidget && view_->GetWidget()->GetTopLevelWidget();
	bool isTopWindowNotMinimized = getTopWindow && !view_->GetWidget()->GetTopLevelWidget()->IsMinimized();
	if (/*memcmp(rc, &preRect, sizeof(RECT)) != 0
		&&*/ isTopWindowNotMinimized)
	{

		preRect = *rc;
		page_->resize(rc, conf_get_int(cfg, CONF_window_border)); 
	}
}



void NativeSftpController::onSetFocus()
{ 
	//reset the hTopWnd
	extern HWND hTopWnd;
	hTopWnd = WindowInterface::GetInstance()->getNativeTopWnd();

	//term_set_focus(term, TRUE);
	::CreateCaret(getNativePage(), caretbm, font_width, font_height);
	ShowCaret(getNativePage());
	//flash_window(0);	       /* stop */
	compose_state = 0;
	//term_update(term);
	ShowWindow(getNativePage(), SW_SHOW);
}

bool NativeSftpController::isLoading()
{
	return backend_state == LOADING;
}


int NativeSftpController::on_paint()
{
	return 0;

	PAINTSTRUCT p;

	HideCaret(page_->getWinHandler());
	m_SftpHdc = BeginPaint(page_->getWinHandler(), &p);
	if (pal)
	{
		SelectPalette(m_SftpHdc, pal, TRUE);
		RealizePalette(m_SftpHdc);
	}
	 
	//term_paint(term, (Context)this,
	//	(p.rcPaint.left - offset_width) / font_width,
	//	(p.rcPaint.top - offset_height) / font_height,
	//	(p.rcPaint.right - offset_width - 1) / font_width,
	//	(p.rcPaint.bottom - offset_height - 1) / font_height,
	//	!term->window_update_pending);

	if (p.fErase ||
		p.rcPaint.left < offset_width ||
		p.rcPaint.top < offset_height ||
		p.rcPaint.right >= offset_width + font_width * term->cols ||
		p.rcPaint.bottom >= offset_height + font_height * term->rows)
	{
		HBRUSH fillcolour, oldbrush;
		HPEN   edge, oldpen;
		fillcolour = CreateSolidBrush(
			colours[ATTR_DEFBG >> ATTR_BGSHIFT]);
		oldbrush = (HBRUSH__*)SelectObject(m_SftpHdc, fillcolour);
		edge = CreatePen(PS_SOLID, 0,
			colours[ATTR_DEFBG >> ATTR_BGSHIFT]);
		oldpen = (HPEN__*)SelectObject(m_SftpHdc, edge);

		/*
		 * Jordan Russell reports that this apparently
		 * ineffectual IntersectClipRect() call masks a
		 * Windows NT/2K bug causing strange display
		 * problems when the PuTTY window is taller than
		 * the primary monitor. It seems harmless enough...
		 */
		IntersectClipRect(m_SftpHdc,
			p.rcPaint.left, p.rcPaint.top,
			p.rcPaint.right, p.rcPaint.bottom);

		//ExcludeClipRect(m_SftpHdc,
		//	offset_width, offset_height,
		//	offset_width + font_width * term->cols,
		//	offset_height + font_height * term->rows);

		Rectangle(m_SftpHdc, p.rcPaint.left, p.rcPaint.top,
			p.rcPaint.right, p.rcPaint.bottom);

		/* SelectClipRgn(m_PuttyHdc, NULL); */

		SelectObject(m_SftpHdc, oldbrush);
		DeleteObject(fillcolour);
		SelectObject(m_SftpHdc, oldpen);
		DeleteObject(edge);
	}
	SelectObject(m_SftpHdc, GetStockObject(SYSTEM_FONT));
	SelectObject(m_SftpHdc, GetStockObject(WHITE_PEN));
	EndPaint(page_->getWinHandler(), &p);
	ShowCaret(page_->getWinHandler());

	return 0;
}
