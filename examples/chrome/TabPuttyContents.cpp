#include "TabPuttyContents.h"

#include <cmath>

#include "base/metric/histogram.h"

#include "ui_gfx/size.h"

#include "tab_contents_delegate.h"
#include "tab_contents_view.h"

#include "putty_view.h"

struct printer_job_tag;
typedef struct printer_job_tag printer_job;
struct bidi_char;
#include "../putty/misc.h"
#include "../putty/terminal.h"
#define OUTSIDE_PUTTY
#include "../putty.h"
#include "native_putty_controller.h"

#include "atlconv.h"

TabPuttyContents::TabPuttyContents(int routing_id, const TabContents* base_tab_contents)
	: TabContents(routing_id, base_tab_contents),
	putty_view_(new view::PuttyView(this))
{
	SetView(putty_view_.get());
}


TabPuttyContents::~TabPuttyContents()
{
}




void TabPuttyContents::dupCfg2Global() const
{
	putty_view_->dupCfg2Global();
}

void TabPuttyContents::do_copy() const
{
	putty_view_->do_copy();
}

void TabPuttyContents::do_paste() const
{
	putty_view_->do_paste();
}

void TabPuttyContents::do_restart() const
{
	putty_view_->do_restart();
}

bool TabPuttyContents::isDisconnected() const
{
	return putty_view_->isDisconnected();
}

void TabPuttyContents::do_reconfig() const
{
	return putty_view_->do_reconfig();
}

void TabPuttyContents::do_copyAll() const
{
	return putty_view_->do_copyAll();
}

void TabPuttyContents::do_clearScrollbar() const
{
	return putty_view_->do_clearScrollbar();
}

void TabPuttyContents::do_log(bool isPressed) const
{
	return putty_view_->do_log(isPressed);
}

void TabPuttyContents::do_shortcutEnabler(bool isPressed) const
{
	return putty_view_->do_shortcutEnabler(isPressed);
}

bool TabPuttyContents::isLogStarted() const
{
	return putty_view_->isLogStarted();
}

bool TabPuttyContents::isShortcutEnabled() const
{
	return putty_view_->isShortcutEnabled();
}

void TabPuttyContents::searchNext(const string16& str) const
{
	return putty_view_->searchNext(str);
}

void TabPuttyContents::searchPrevious(const string16& str) const
{
	return putty_view_->searchPrevious(str);
}

void TabPuttyContents::resetSearch() const
{
	return putty_view_->resetSearch();
}

void TabPuttyContents::setFocus() const
{
	return putty_view_->setFocus();
}

void TabPuttyContents::cmdScat(int type, const char * buffer, int buflen, int interactive) const
{
	return putty_view_->cmdScat(type, buffer, buflen, interactive);
}

void TabPuttyContents::sendScript(int type, const char * buffer, int buflen, int interactive) const
{
	return putty_view_->sendScript(type, buffer, buflen, interactive);
}

Conf* TabPuttyContents::getCfg()
{
	return putty_view_->getCfg();
}

int TabPuttyContents::getScrollToEnd()
{
	Terminal* term = putty_view_->getTerminal();
	if (!term)
	{
		return TRUE;
	}
	return term->scroll_to_end;
}

void TabPuttyContents::setScrollToEnd(int isScrollToEnd)
{
	Terminal* term = putty_view_->getTerminal();
	if (!term)
	{
		return;
	}
	term->scroll_to_end = isScrollToEnd;
	if (isScrollToEnd)
	{
		term->seen_disp_event = TRUE;
		term_update(term);
	}
}

bool TabPuttyContents::CanClose()
{
	Terminal* term = putty_view_->getTerminal();
	if (!term)
	{
		return TRUE;
	}
	NativePuttyController* puttyController = putty_view_->getController();
	if (puttyController == NULL || puttyController->must_close_tab_)
	{
		return TRUE;
	}

	Conf* cfg = getCfg();
	int warn_on_close = conf_get_int(cfg, CONF_warn_on_close);
	if (warn_on_close)
	{
		USES_CONVERSION;
		//char *str = dupprintf("%s Exit Confirmation", W2A(this->GetTitle().c_str()));
		//if (MessageBoxA(putty_view_->getNativeView(), "Are you sure you want to close this session?",
		//                str, MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON1 | MB_TOPMOST)
		//        == IDOK)
		//{
		return TRUE;
		//}
		//return FALSE;
	}
	return TRUE;
}

void TabPuttyContents::notifyMsg(const char* msg, void* data)
{
	putty_view_->notifyMsg(msg, data);
}


const string16& TabPuttyContents::GetTitle() const
{
	// Transient entries take precedence. They are used for interstitial pages
	// that are shown on top of existing pages.
	//NavigationEntry* entry = controller_.GetTransientEntry();
	//std::string accept_languages =
	//    content::GetContentClient()->browser()->GetAcceptLangs(this);
	//if(entry)
	//{
	//    return entry->GetTitleForDisplay(accept_languages);
	//}
	//WebUI* our_web_ui = render_manager_.pending_web_ui() ?
	//    render_manager_.pending_web_ui() : render_manager_.web_ui();
	//if(our_web_ui)
	//{
	//    // Don't override the title in view source mode.
	//    entry = controller_.GetActiveEntry();
	//    if(!(entry && entry->IsViewSourceMode()))
	//    {
	//        // Give the Web UI the chance to override our title.
	//        const string16& title = our_web_ui->overridden_title();
	//        if(!title.empty())
	//        {
	//            return title;
	//        }
	//    }
	//}

	//// We use the title for the last committed entry rather than a pending
	//// navigation entry. For example, when the user types in a URL, we want to
	//// keep the old page's title until the new load has committed and we get a new
	//// title.
	//entry = controller_.GetLastCommittedEntry();
	//if(entry)
	//{
	//    return entry->GetTitleForDisplay(accept_languages);
	//}

	// |page_title_when_no_navigation_entry_| is finally used
	// if no title cannot be retrieved.
	return putty_view_->getWinTitle();
}


HWND TabPuttyContents::GetContentNativeView() const
{
	//return view_->GetContentNativeView();
	return putty_view_->getNativeView();
}

HWND TabPuttyContents::GetNativeView() const
{
	//return view_->GetNativeView();
	return putty_view_->getNativeView();
}

void TabPuttyContents::GetContainerBounds(gfx::Rect *out) const
{
	//view_->GetContainerBounds(out);
	*out = putty_view_->bounds();
}

void TabPuttyContents::Focus()
{
	//view_->Focus();
	return putty_view_->RequestFocus();
}


void TabPuttyContents::WasHidden()
{
	if (!capturing_contents())
	{
		// |render_view_host()| can be NULL if the user middle clicks a link to open
		// a tab in then background, then closes the tab before selecting it.  This
		// is because closing the tab calls TabContents::Destroy(), which removes
		// the |render_view_host()|; then when we actually destroy the window,
		// OnWindowPosChanged() notices and calls HideContents() (which calls us).
		//RenderWidgetHostView* rwhv = GetRenderWidgetHostView();
		//if(rwhv)
		//{
		//    rwhv->WasHidden();
		//}
		putty_view_->SetVisible(false);
	}
}

void TabPuttyContents::ShowContents()
{
	//RenderWidgetHostView* rwhv = GetRenderWidgetHostView();
	//if(rwhv)
	//{
	//    rwhv->DidBecomeSelected();
	//}
	putty_view_->SetVisible(true);
}

bool TabPuttyContents::IsLoading() const
{
	return putty_view_->isLoading(); /*|| (web_ui() && web_ui()->IsLoading())*/
}

