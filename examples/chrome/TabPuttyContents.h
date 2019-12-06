#pragma once
#include "tab_contents.h"
#include "putty_view.h"
#include "base/memory/scoped_ptr.h"

namespace view
{
	class PuttyView;
}

class TabPuttyContents :
	public TabContents
{
public:
	TabPuttyContents(int routing_id, const TabContents* base_tab_contents);
	virtual ~TabPuttyContents();


	virtual void dupCfg2Global() const;
	virtual void do_copy() const;
	virtual void do_paste() const;
	virtual void do_restart() const;
	virtual bool isDisconnected() const;
	virtual void do_reconfig() const;
	virtual void do_copyAll() const;
	virtual void do_clearScrollbar() const;
	virtual void do_log(bool isPressed) const;
	virtual void do_shortcutEnabler(bool isPressed) const;
	virtual bool isLogStarted() const;
	virtual bool isShortcutEnabled() const;
	virtual void searchNext(const string16& str) const;
	virtual void searchPrevious(const string16& str) const;
	virtual void resetSearch() const;
	virtual void setFocus() const;
	virtual void cmdScat(int type, const char * buffer, int buflen, int interactive) const;
	virtual void sendScript(int type, const char * buffer, int buflen, int interactive) const;
	virtual Conf* getCfg();
	int getScrollToEnd();
	virtual void setScrollToEnd(int isScrollToEnd);
	virtual bool CanClose();
	virtual void notifyMsg(const char* msg, void* data);
	virtual const string16& GetTitle() const;
	virtual HWND GetContentNativeView() const;
	virtual HWND GetNativeView() const;
	virtual void GetContainerBounds(gfx::Rect *out) const;
	virtual void Focus();
	virtual void WasHidden();
	virtual void ShowContents();
	virtual bool IsLoading() const;
private:
	scoped_ptr<view::PuttyView> putty_view_;
};

