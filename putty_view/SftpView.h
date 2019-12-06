#ifndef PUTTY_VIEW_H
#define PUTTY_VIEW_H

#include <algorithm>
#include "view/view.h"

struct conf_tag;
typedef struct conf_tag Conf;
struct terminal_tag;
typedef struct terminal_tag Terminal;
class NativeSftpController;
class TabContents;
namespace view
{

// 实现无窗口的richedit控件.
class SftpView : public View
{
public:
    static const char kViewClassName[];

    explicit SftpView(TabContents* contents);
    virtual ~SftpView();
    TabContents* GetContents()
    {
        return tabContents_;
    }
    virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
    virtual void VisibilityChanged(View* starting_from, bool is_visible);
    void OnFocus();
    virtual bool OnKeyPressed(const KeyEvent& event);
    bool SkipDefaultKeyEventProcessing(const KeyEvent& event)
    {
        return event.key_code() == ui::VKEY_TAB;
    }

    virtual bool OnMousePressed(const MouseEvent& event);
    virtual bool OnMouseDragged(const MouseEvent& event);
    virtual void OnMouseReleased(const MouseEvent& event);
    virtual void OnPaint(gfx::Canvas* canvas);

    string16& getWinTitle();
    HWND getNativeView();
    bool isLoading();
    bool isDisconnected();
    void dupCfg2Global();
    void do_copy();
    void do_paste();
    void do_restart();
    void do_reconfig();
    void do_copyAll();
    void do_clearScrollbar();
    void do_log(bool isPressed);
    void do_shortcutEnabler(bool isPressed);
    bool isLogStarted();
    bool isShortcutEnabled();
    void searchNext(const string16& str) ;
    void searchPrevious(const string16& str) ;
    void resetSearch() ;
    void setFocus();
    void cmdScat(int type, const char * buffer, int buflen, int interactive);
    void sendScript(int type, const char * buffer, int buflen, int interactive);
    Conf* getCfg();
    Terminal* getTerminal();
    void notifyMsg(const char* msg, void* data);
    NativeSftpController* getController()
    {
        return sftpController_;
    };
protected:
    virtual void Layout();
    virtual void Paint(gfx::Canvas* canvas);
    NativeSftpController* sftpController_;
    TabContents* tabContents_;

};

}

#endif