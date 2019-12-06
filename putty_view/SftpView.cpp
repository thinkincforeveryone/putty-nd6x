#include "SftpView.h"
#include "NativeSftpController.h"

#include "view/widget/widget.h"
#include "putty_global_config.h"

#include "terminal.h"
#include "storage.h"
#include "tab_contents.h"
extern int is_session_log_enabled(void *handle);
extern void log_restart(void *handle, Conf *cfg);
extern void log_stop(void *handle, Conf *cfg);

namespace view
{
// static
const char SftpView::kViewClassName[] = "view/SftpView";

SftpView::SftpView(TabContents* contents)
{
    extern Conf* cfg;
    tabContents_ = contents;
    sftpController_ = new NativeSftpController(cfg, this); 
    set_focusable(true); 
}
SftpView::~SftpView()
{
    //sftpController_->destroy();
}

void SftpView::ViewHierarchyChanged(bool is_add, View* parent, View* child)
{
    sftpController_->parentChanged(is_add ? parent : NULL);
}

// When SetVisible() changes the visibility of a view, this method is
// invoked for that view as well as all the children recursively.
void SftpView::VisibilityChanged(View* starting_from, bool is_visible)
{
    if (is_visible)
    {
        //sftpController_->showPage();
    }
    else
    {
        //sftpController_->hidePage();
    }
}

void SftpView::Layout()
{
    RECT viewRect = ConvertRectToWidget(bounds()).ToRECT();
    sftpController_->setPagePos(&viewRect);
}


void SftpView::Paint(gfx::Canvas* canvas)
{
    View::Paint(canvas);
}

bool SftpView::OnKeyPressed(const KeyEvent& event)
{
	return true;
}
bool SftpView::OnMousePressed(const MouseEvent& event)
{ 
    return true;
}
bool SftpView::OnMouseDragged(const MouseEvent& event)
{ 
    return true;
}
void SftpView::OnMouseReleased(const MouseEvent& event)
{ 
}

void SftpView::OnFocus()
{
	::SetFocus(sftpController_->getNativePage());
	if (GetWidget())
	{
		GetWidget()->NotifyAccessibilityEvent(
			this, ui::AccessibilityTypes::EVENT_FOCUS, false);
	}
}

string16& SftpView::getWinTitle()
{
    return sftpController_->disName;
}

HWND SftpView::getNativeView()
{
    return sftpController_->getNativePage();
}

bool SftpView::isLoading()
{
	return sftpController_->isLoading(); 
}
bool SftpView::isDisconnected()
{
    //return sftpController_->isDisconnected();
	return false;
}
void SftpView::dupCfg2Global()
{ 
}

void SftpView::do_copy()
{

}

void SftpView::do_paste()
{
	//sftpController_->request_paste();
}

void SftpView::do_restart()
{
    //sftpController_->restartBackend();
}

void SftpView::do_reconfig()
{
	//sftpController_->on_reconfig();
}

void SftpView::do_copyAll()
{
	//return term_copyall(sftpController_->term);
}

void SftpView::do_clearScrollbar()
{
	return term_clrsb(sftpController_->term);
}

void SftpView::do_log(bool isPressed)
{
    
}

void SftpView::do_shortcutEnabler(bool isPressed)
{
    //sftpController_->cfg.is_enable_shortcut = isPressed;
    PuttyGlobalConfig::GetInstance()->setShotcutKeyEnabled(isPressed);
}

bool SftpView::isLogStarted()
{
    if (sftpController_->page_ == NULL)
        return false;
    return is_session_log_enabled(sftpController_->logctx)!= 0;
}

bool SftpView::isShortcutEnabled()
{
    return PuttyGlobalConfig::GetInstance()->isShotcutKeyEnabled();
}

void SftpView::searchNext(const string16& str)
{
	//return term_find(sftpController_->term, str.c_str(), 0);
}

void SftpView::searchPrevious(const string16& str)
{
	//return term_find(sftpController_->term, str.c_str(), 1);
}

void SftpView::resetSearch()
{
	//return term_free_hits(sftpController_->term);
}

void SftpView::setFocus()
{
	SetFocus(sftpController_->getNativePage());
}

void SftpView::cmdScat(int type, const char * buffer, int buflen, int interactive)
{
	//sftpController_->cmdScat(type, buffer, buflen, interactive);
}

void SftpView::sendScript(int type, const char * buffer, int buflen, int interactive)
{
    //sftpController_->sendScript(type, buffer, buflen, interactive);
}

void SftpView::OnPaint(gfx::Canvas* canvas)
{
    sftpController_->on_paint();
}

Conf* SftpView::getCfg()
{
    return sftpController_->cfg;
}

Terminal* SftpView::getTerminal()
{
    return sftpController_->term;
}

void SftpView::notifyMsg(const char* msg, void* data)
{
    return sftpController_->notifyMsg(msg, data);
}

}
