#ifndef WINDOW_INTERFACE_H
#define WINDOW_INTERFACE_H

#include "base/memory/singleton.h"
#include "base/message_loop.h"
#include "browser.h"
#include "browser_list.h"
#include "browser_window.h"
#include "tab_strip_model.h"
#include "tab_contents_wrapper.h"
#include "browser_view.h"
#include "toolbar_view.h"
#include "FsmInterface.h"
#include "WinMutex.h"

//#include "native_putty_common.h"
void fatalbox(const char *fmt, ...);

class WindowInterface
{
public:
    WindowInterface()
        : cmd_scatter_state_(CMD_DEFAULT)
    {}
    ~WindowInterface() {}

    static WindowInterface* GetInstance()
    {
        return Singleton<WindowInterface>::get();
    }

    int getTabCountInLastActiveBrowser()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            return 0;
        }

        int tab_count = browser->tab_count();
        return tab_count;
    }

    void createBrowser()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            return;
        }

        int tab_count = browser->tab_count();
        if (tab_count == 0)
        {
            return;
        }

        Browser* new_browser = Browser::Create();
        new_browser->window()->Show();
    }

    void createNewSession()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return ;
        }
        if (NULL != browser->AddBlankTab(true))
        {
            browser->window()->Show();
        }
    }

    void createNewSessionWithGlobalCfg()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return ;
        }
        if (NULL != browser->AddTabWithGlobalCfg(true))
        {
            browser->window()->Show();
        }
    }

    void dupCurSession()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return ;
        }
        browser->DuplicateCurrentTab();
        browser->window()->Show();
    }

	void sftpCurSession()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL)
		{
			fatalbox("%s", "last ative window is not found");
			return;
		}
		browser->sftpCurSession();
		browser->window()->Show();
	}

    void selectNextTab()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return ;
        }
        browser->SelectNextTab();
    }

    void selectPreviousTab()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return ;
        }
        browser->SelectPreviousTab();
    }

    void moveTabNext()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return ;
        }
        browser->MoveTabNext();
    }

    void moveTabPrevious()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return ;
        }
        browser->MoveTabPrevious();
    }

    void selectTab(int index)
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return ;
        }
        browser->SelectNumberedTab(index);
    }

    void closeCurrentTab()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return ;
        }
        browser->CloseTab();
    }

    void reloadCurrentSession()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return ;
        }
        TabContents* tab = browser->GetSelectedTabContents();
        if (tab)
        {
            tab->do_restart();
        }
    }

    BrowserWindow* getBrowserView()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            fatalbox("%s", "last ative window is not found");
            return NULL;
        }
        return browser->window();
    }

    static HINSTANCE getMainInst()
    {
        extern HINSTANCE hinst;
        return hinst;
    }

    static HWND getNativeTopWnd()
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
            return NULL;
        BrowserWindow* bw = browser->window();
        if (bw == NULL)
            return NULL;
        return bw->GetNativeHandle();
    }

    void UpdateToolbar(bool should_restore_state)
    {
        Browser* browser = BrowserList::GetLastActive();
        if (browser == NULL)
        {
            return;
        }
        BrowserWindow* bw = browser->window();
        if (bw == NULL)
        {
            return;
        }
        TabContentsWrapper* wrapper = browser->GetSelectedTabContentsWrapper();
        if (wrapper == NULL)
        {
            return;
        }

        bw->UpdateToolbar(wrapper, should_restore_state);
    }

    void AllToolbarSizeChanged(bool should_restore_state)
    {
        BrowserList::const_iterator it = BrowserList::begin();
        for (; it != BrowserList::end(); it++)
        {
            Browser* browser = *it;
            if (browser == NULL)
            {
                continue;
            }
            BrowserWindow* bw = browser->window();
            if (bw == NULL)
            {
                continue;
            }

            bw->ToolbarSizeChanged(should_restore_state);
        }

    }

    enum {CMD_DEFAULT = 0, CMD_TO_ALL, CMD_TO_WITHIN_WINDOW, CMD_TO_ACTIVE_TAB};
    void setCmdScatterState(int state)
    {
        if (state == cmd_scatter_state_) return;
        cmd_scatter_state_ = state;
        BrowserList::const_iterator it = BrowserList::begin();
        for (; it != BrowserList::end(); it++)
        {
            ((BrowserView*)(*it)->window())->toolbar()->UpdateButton() ;
        }
    }
    int getCmdScatterState()
    {
        return cmd_scatter_state_;
    }
    bool ifNeedCmdScat()
    {
        return CMD_DEFAULT != getCmdScatterState();
    }
    void cmdScat(int type, const char * buffer, int buflen, int interactive)
    {
        sendCmd(getCmdScatterState(), type, buffer, buflen, interactive);
    }
    static void sendScript(int state, int type, const char * buffer, int buflen, int interactive)
    {
        if (CMD_DEFAULT == state)
        {
            Browser* browser = BrowserList::GetLastActive();
            if (browser == NULL)
            {
                return;
            }
            TabContents* tab = browser->GetSelectedTabContents();
            if (tab == NULL)
            {
                return;
            }
            tab->sendScript(type, buffer, buflen, interactive);
        }
        else if (CMD_TO_ALL == state)
        {
            BrowserList::const_iterator it = BrowserList::begin();
            for (; it != BrowserList::end(); it++)
            {
                TabStripModel* tabStripModel = (*it)->tabstrip_model();
                for (int i = 0; i < tabStripModel->count(); i++)
                {
                    TabContentsWrapper* tabContentsWrapper = tabStripModel->GetTabContentsAt(i);
                    if (NULL == tabContentsWrapper) continue;
                    TabContents* tab = tabContentsWrapper->tab_contents();
                    tab->sendScript(type, buffer, buflen, interactive);
                }
            }
        }
        else if (CMD_TO_WITHIN_WINDOW == state)
        {
            Browser* browser = BrowserList::GetLastActive();
            TabStripModel* tabStripModel = browser->tabstrip_model();
            for (int i = 0; i < tabStripModel->count(); i++)
            {
                TabContentsWrapper* tabContentsWrapper = tabStripModel->GetTabContentsAt(i);
                if (NULL == tabContentsWrapper) continue;
                TabContents* tab = tabContentsWrapper->tab_contents();
                tab->sendScript(type, buffer, buflen, interactive);
            }
        }
        else if(CMD_TO_ACTIVE_TAB == state)
        {
            BrowserList::const_iterator it = BrowserList::begin();
            for (; it != BrowserList::end(); it++)
            {
                TabStripModel* tabStripModel = (*it)->tabstrip_model();
                TabContentsWrapper* tabContentsWrapper = tabStripModel->GetActiveTabContents();
                if (NULL == tabContentsWrapper) continue;
                TabContents* tab = tabContentsWrapper->tab_contents();
                tab->sendScript(type, buffer, buflen, interactive);
            }
        }
    }

    static void sendCmd(int state, int type, const char * buffer, int buflen, int interactive)
    {
        if (CMD_DEFAULT == state)
        {
            Browser* browser = BrowserList::GetLastActive();
            if (browser == NULL)
            {
                return;
            }
            TabContents* tab = browser->GetSelectedTabContents();
            if (tab == NULL)
            {
                return;
            }
            tab->cmdScat(type, buffer, buflen, interactive);
        }
        else if (CMD_TO_ALL == state)
        {
            BrowserList::const_iterator it = BrowserList::begin();
            for (; it != BrowserList::end(); it++)
            {
                TabStripModel* tabStripModel = (*it)->tabstrip_model();
                for (int i = 0; i < tabStripModel->count(); i++)
                {
                    TabContentsWrapper* tabContentsWrapper = tabStripModel->GetTabContentsAt(i);
                    if (NULL == tabContentsWrapper) continue;
                    TabContents* tab = tabContentsWrapper->tab_contents();
                    tab->cmdScat(type, buffer, buflen, interactive);
                }
            }
        }
        else if (CMD_TO_WITHIN_WINDOW == state)
        {
            Browser* browser = BrowserList::GetLastActive();
            TabStripModel* tabStripModel = browser->tabstrip_model();
            for (int i = 0; i < tabStripModel->count(); i++)
            {
                TabContentsWrapper* tabContentsWrapper = tabStripModel->GetTabContentsAt(i);
                if (NULL == tabContentsWrapper) continue;
                TabContents* tab = tabContentsWrapper->tab_contents();
                tab->cmdScat(type, buffer, buflen, interactive);
            }
        }
        else if (CMD_TO_ACTIVE_TAB == state)
        {
            BrowserList::const_iterator it = BrowserList::begin();
            for (; it != BrowserList::end(); it++)
            {
                TabStripModel* tabStripModel = (*it)->tabstrip_model();
                TabContentsWrapper* tabContentsWrapper = tabStripModel->GetActiveTabContents();
                if (NULL == tabContentsWrapper) continue;
                TabContents* tab = tabContentsWrapper->tab_contents();
                tab->cmdScat(type, buffer, buflen, interactive);
            }
        }
    }

    void init_ui_msg_loop()
    {
        ui_msg_loop_ = MessageLoopForUI::current();
    }
    void process_in_msg_loop(FSM_FUNCTION<void(void)> func)
    {
        if (ui_msg_loop_ == NULL)
        {
            return;
        }
        ui_msg_loop_->PostTask(new FsmTask(func));
    }

    void register_atexit(void* key, std::function<void()>& cb)
    {
        AutoLock lock(at_exit_map_mutex_);
        at_exit_map_[key] = cb;
    }
    bool remove_atexit(void* key)
    {
        AutoLock lock(at_exit_map_mutex_);
        std::map < void*, std::function<void()>>::iterator it = at_exit_map_.find(key);
        if (it != at_exit_map_.end())
        {
            at_exit_map_.erase(it);
            return true;
        }
        return false;
    }
    void at_exit();

    void open_wait_session_in_new_window()
    {
        wait_open_sessions_.push_front("");
    }
    void push_wait_open_session(const char* session_name)
    {
        wait_open_sessions_.push_back(session_name);
    }
    std::string pop_wait_open_session()
    {
        if (wait_open_sessions_.empty())
        {
            return "";
        }
        std::string ret = *wait_open_sessions_.begin();
        wait_open_sessions_.pop_front();

        if (ret.length() == 0 && !wait_open_sessions_.empty())
        {
            createBrowser();
            return pop_wait_open_session();
        }
        return ret;
    }


private:
    int cmd_scatter_state_;
    MessageLoopForUI* ui_msg_loop_;

    Lock at_exit_map_mutex_;
    std::map < void*, std::function<void()>> at_exit_map_;

    std::list<std::string> wait_open_sessions_;

    friend struct DefaultSingletonTraits<WindowInterface>;
    DISALLOW_COPY_AND_ASSIGN(WindowInterface);
};

#endif /* WINDOW_INTERFACE_H */
