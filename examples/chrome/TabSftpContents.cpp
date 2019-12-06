#include "TabSftpContents.h"

#include "SftpView.h"

TabSftpContents::TabSftpContents(int routing_id, const TabContents* base_tab_contents)
	: TabContents(routing_id, base_tab_contents)
	, m_SftpView(new view::SftpView(this))
{
	SetView(m_SftpView.get()); 
	
}


TabSftpContents::~TabSftpContents()
{
}




void TabSftpContents::dupCfg2Global() const
{
	//m_SftpView->dupCfg2Global();
}

void TabSftpContents::do_copy() const
{
	//m_SftpView->do_copy();
}

void TabSftpContents::do_paste() const
{
	//m_SftpView->do_paste();
}

void TabSftpContents::do_restart() const
{
	//m_SftpView->do_restart();
}

bool TabSftpContents::isDisconnected() const
{
	//return m_SftpView->isDisconnected();
	return false;
}

void TabSftpContents::do_reconfig() const
{
	//return m_SftpView->do_reconfig();
}

void TabSftpContents::do_copyAll() const
{
	//return m_SftpView->do_copyAll();
}

void TabSftpContents::do_clearScrollbar() const
{
	//return m_SftpView->do_clearScrollbar();
}

void TabSftpContents::do_log(bool isPressed) const
{
	//return m_SftpView->do_log(isPressed);
}

void TabSftpContents::do_shortcutEnabler(bool isPressed) const
{
	//return m_SftpView->do_shortcutEnabler(isPressed);
}

bool TabSftpContents::isLogStarted() const
{
	//return m_SftpView->isLogStarted();
	return true;
}

bool TabSftpContents::isShortcutEnabled() const
{
	//return m_SftpView->isShortcutEnabled();
	return true;
}

void TabSftpContents::searchNext(const string16& str) const
{
	//return m_SftpView->searchNext(str);
}

void TabSftpContents::searchPrevious(const string16& str) const
{
	//return m_SftpView->searchPrevious(str);
}

void TabSftpContents::resetSearch() const
{
	//return m_SftpView->resetSearch();
}

void TabSftpContents::setFocus() const
{
	//return m_SftpView->setFocus();
}

void TabSftpContents::cmdScat(int type, const char * buffer, int buflen, int interactive) const
{
	//return m_SftpView->cmdScat(type, buffer, buflen, interactive);
}

void TabSftpContents::sendScript(int type, const char * buffer, int buflen, int interactive) const
{
	//return m_SftpView->sendScript(type, buffer, buflen, interactive);
}

Conf* TabSftpContents::getCfg()
{
	//return m_SftpView->getCfg();
	return NULL;
}

int TabSftpContents::getScrollToEnd()
{
	//Terminal* term = m_SftpView->getTerminal();
	//if (!term)
	//{
	//	return TRUE;
	//}
	//return term->scroll_to_end;
	return -1;
}

void TabSftpContents::setScrollToEnd(int isScrollToEnd)
{
	/*Terminal* term = m_SftpView->getTerminal();
	if (!term)
	{
		return;
	}
	term->scroll_to_end = isScrollToEnd;
	if (isScrollToEnd)
	{
		term->seen_disp_event = TRUE;
		term_update(term);
	}*/
}

bool TabSftpContents::CanClose()
{
	//Terminal* term = m_SftpView->getTerminal();
	//if (!term)
	//{
	//	return TRUE;
	//}
	//NativePuttyController* puttyController = m_SftpView->getController();
	//if (puttyController == NULL || puttyController->must_close_tab_)
	//{
	//	return TRUE;
	//}

	//Conf* cfg = getCfg();
	//int warn_on_close = conf_get_int(cfg, CONF_warn_on_close);
	//if (warn_on_close)
	//{
	//	USES_CONVERSION;
	//	//char *str = dupprintf("%s Exit Confirmation", W2A(this->GetTitle().c_str()));
	//	//if (MessageBoxA(m_SftpView->getNativeView(), "Are you sure you want to close this session?",
	//	//                str, MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON1 | MB_TOPMOST)
	//	//        == IDOK)
	//	//{
	//	return TRUE;
	//	//}
	//	//return FALSE;
	//}
	return TRUE;
}

void TabSftpContents::notifyMsg(const char* msg, void* data)
{
	//m_SftpView->notifyMsg(msg, data);
}


const string16& TabSftpContents::GetTitle() const
{
	return m_SftpView->getWinTitle();
	//return m_TitleName;
}


HWND TabSftpContents::GetContentNativeView() const
{
	//return m_SftpView->getNativeView();
	return NULL;
}

HWND TabSftpContents::GetNativeView() const
{
	//return m_SftpView->getNativeView();
	return NULL;
}

void TabSftpContents::GetContainerBounds(gfx::Rect *out) const
{
	*out = m_SftpView->bounds();
}

void TabSftpContents::Focus()
{
	return m_SftpView->RequestFocus();
}


void TabSftpContents::WasHidden()
{
	if (!capturing_contents())
	{
		m_SftpView->SetVisible(false);
	}
}

void TabSftpContents::ShowContents()
{
	m_SftpView->SetVisible(true);
}

bool TabSftpContents::IsLoading() const
{
	return m_SftpView->isLoading(); /*|| (web_ui() && web_ui()->IsLoading())*/
	
}

