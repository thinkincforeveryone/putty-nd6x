#ifndef NATIVE_PUTTY_PAGE_H
#define NATIVE_PUTTY_PAGE_H


#include "native_putty_common.h"
#include "putty.h"


class NativeSftpController;
class NativeSftpPage
{
public:
    int init(NativeSftpController* puttyController, Conf *cfg, HWND hwndParent);
    void init_scrollbar(Terminal *term);
    int fini();
    int resize(const RECT *rc, const int cfg_winborder);
	NativeSftpController* get_item(HWND hwndPage);
    void get_term_size(int *term_width, int *term_height);

    int registerPage();
    int unregisterPage();
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message,
                                    WPARAM wParam, LPARAM lParam);

    HWND getWinHandler()
    {
        return hwndCtrl;
    }
    friend class NativeSftpController;
private:
    static const LPCWSTR WINTAB_SFTP_PAGE_CLASS;
    HWND hwndCtrl;
	NativeSftpController* sftpController_;
    int extra_page_width, extra_page_height; //gaps from term to page
    int extra_width, extra_height; //gaps from page to tab
};




#endif /* NATIVE_PUTTY_PAGE_H */
