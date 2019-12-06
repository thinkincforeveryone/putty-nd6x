#ifndef NATIVE_PUTTY_CONTROLLER_H
#define NATIVE_PUTTY_CONTROLLER_H

#include "base/synchronization/lock.h"
#include "native_putty_common.h"
#include "putty.h"
#include "view/view.h" 
#include <vector>

class ZmodemSession;
class NativeSftpPage;

struct ParamPair
{
    WPARAM wParam;
    LPARAM lParam;
};

class NativeSftpController
{
public:
	NativeSftpController(Conf *cfg, view::View* theView);
    virtual ~NativeSftpController(); 
    int init(HWND hwndParent);
    void notifyMsg(const char* msg, void* data);
	void StartSftp();
	HWND getNativePage();
	void parentChanged(view::View* parent);
	void setPagePos(const RECT* rc);
	void onSetFocus();
	bool isLoading();
	int on_paint();
public:
    HWND nativeParentWin_;

    HDC m_SftpHdc;
    HPALETTE pal;

    NativeSftpPage* page_;
    view::View* view_;

    Conf* cfg;
    Terminal *term;
    void *logctx;
    RGBTRIPLE defpal[NALLCOLOURS];
    struct unicode_data ucsdata;

    HFONT fonts[FONT_MAXNO];
    LOGFONT lfont;
    int fontflag[FONT_MAXNO];
    bold_mode_t bold_font_mode;
    int bold_colours;
    und_mode_t und_mode;
    int extra_width, extra_height;
    int font_width, font_height, font_dualwidth, font_varpitch;
    int offset_width, offset_height;
    int caret_x, caret_y;
    int descent;
    int font_height_by_wheel;

    HBITMAP caretbm;

    int dbltime, lasttime, lastact;
    Mouse_Button lastbtn;

    Backend *back;
    void *backhandle;
    void *ldisc;

    int must_close_session, session_closed;
    int must_close_tab_;

    COLORREF colours[NALLCOLOURS];
    LPLOGPALETTE logpal;

    int send_raw_mouse;
    int wheel_accumulator;
    int busy_status;
    int compose_state;
    static UINT wm_mousewheel;

    const struct telnet_special *specials;
    HMENU specials_menu;
    int n_specials;

    int prev_rows, prev_cols;

    int ignore_clip;

    HRGN hRgn, hCloserRgn;
    int closerX, closerY;

    RECT rcDis;
    RECT preRect;
    char disRawName[256];
    string16 disName;
    char *window_name, *icon_name;

    HANDLE close_mutex;

    HWND logbox;
    unsigned nevents;
    unsigned negsize;
    char **events;

    std::vector<ParamPair> pending_param_list;

    static int kbd_codepage;
    UINT last_mousemove;
    bool isClickingOnPage;

    enum BackendState {LOADING = 0, CONNECTED = 1, DISCONNECTED = -1};
    BackendState backend_state;

    static HMENU popup_menu;

    long next_flash;
    int flashing;
    volatile bool is_frozen;

    int cursor_visible;
    int forced_visible;

    enum {TIMER_INTERVAL = 50}; //in ms
    //base::RepeatingTimer<NativeSftpController> checkTimer_;
    static base::Lock socketTreeLock_;
    bool isAutoCmdEnabled_;

    //ZmodemSession* zSession_;

};
#endif /* NATIVE_PUTTY_CONTROLLER_H */
