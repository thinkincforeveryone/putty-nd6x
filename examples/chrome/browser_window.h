
#ifndef __browser_window_h__
#define __browser_window_h__

#pragma once

#include "bookmark_bar.h"
#include "window_open_disposition.h"

namespace gfx
{
class Rect;
}

class Browser;
class LocationBar;
class TabContents;
class TabContentsWrapper;
class Url;

////////////////////////////////////////////////////////////////////////////////
// BrowserWindow interface
//  An interface implemented by the "view" of the Browser window.
//
// NOTE: All getters may return NULL.
class BrowserWindow
{
public:
    virtual ~BrowserWindow() {}

    // Show the window, or activates it if it's already visible.
    virtual void Show() = 0;

    // Show the window, but do not activate it. Does nothing if window
    // is already visible.
    virtual void ShowInactive() = 0;

    // Sets the window's size and position to the specified values.
    virtual void SetBounds(const gfx::Rect& bounds) = 0;

    // Closes the frame as soon as possible.  If the frame is not in a drag
    // session, it will close immediately; otherwise, it will move offscreen (so
    // events are still fired) until the drag ends, then close. This assumes
    // that the Browser is not immediately destroyed, but will be eventually
    // destroyed by other means (eg, the tab strip going to zero elements).
    // Bad things happen if the Browser dtor is called directly as a result of
    // invoking this method.
    virtual void Close() = 0;

    // Activates (brings to front) the window. Restores the window from minimized
    // state if necessary.
    virtual void Activate() = 0;

    // Deactivates the window, making the next window in the Z order the active
    // window.
    virtual void Deactivate() = 0;

    // Returns true if the window is currently the active/focused window.
    virtual bool IsActive() const = 0;

    // Flashes the taskbar item associated with this frame.
    virtual void FlashFrame() = 0;

    // Return a platform dependent identifier for this frame. On Windows, this
    // returns an HWND.
    virtual HWND GetNativeHandle() = 0;

    // Inform the receiving frame that an animation has progressed in the
    // selected tab.
    // TODO(beng): Remove. Infobars/Boomarks bars should talk directly to
    //             BrowserView.
    virtual void ToolbarSizeChanged(bool is_animating) = 0;

    // Inform the frame that the selected tab favicon or title has changed. Some
    // frames may need to refresh their title bar.
    virtual void UpdateTitleBar() = 0;

    // Invoked when the state of the bookmark bar changes. This is only invoked if
    // the state changes for the current tab, it is not sent when switching tabs.
    virtual void BookmarkBarStateChanged(
        BookmarkBar::AnimateChangeType change_type) = 0;

    // Update any loading animations running in the window. |should_animate| is
    // true if there are tabs loading and the animations should continue, false
    // if there are no active loads and the animations should end.
    virtual void UpdateLoadingAnimations(bool should_animate) = 0;

    // Sets the starred state for the current tab.
    virtual void SetStarredState(bool is_starred) = 0;

    // Returns the nonmaximized bounds of the frame (even if the frame is
    // currently maximized or minimized) in terms of the screen coordinates.
    virtual gfx::Rect GetRestoredBounds() const = 0;

    // Retrieves the window's current bounds, including its frame.
    // This will only differ from GetRestoredBounds() for maximized
    // and minimized windows.
    virtual gfx::Rect GetBounds() const = 0;

    // TODO(beng): REMOVE?
    // Returns true if the frame is maximized (aka zoomed).
    virtual bool IsMaximized() const = 0;

    // Returns true if the frame is minimized.
    virtual bool IsMinimized() const = 0;

    // Accessors for fullscreen mode state.
    virtual void SetFullscreen(bool fullscreen) = 0;
    virtual bool IsFullscreen() const = 0;

    // Returns true if the fullscreen bubble is visible.
    virtual bool IsFullscreenBubbleVisible() const = 0;

    // Returns the location bar.
    virtual LocationBar* GetLocationBar() const = 0;

    // Tries to focus the location bar.  Clears the window focus (to avoid
    // inconsistent state) if this fails.
    virtual void SetFocusToLocationBar(bool select_all) = 0;

    // Informs the view whether or not a load is in progress for the current tab.
    // The view can use this notification to update the reload/stop button.
    virtual void UpdateReloadStopState(bool is_loading, bool force) = 0;

    // Updates the toolbar with the state for the specified |contents|.
    virtual void UpdateToolbar(TabContentsWrapper* contents,
                               bool should_restore_state) = 0;

    // Focuses the toolbar (for accessibility).
    virtual void FocusToolbar() = 0;

    // Focuses the app menu like it was a menu bar.
    //
    // Not used on the Mac, which has a "normal" menu bar.
    virtual void FocusAppMenu() = 0;

    // Focuses the bookmarks toolbar (for accessibility).
    virtual void FocusBookmarksToolbar() = 0;

    // Focuses the Chrome OS status view (for accessibility).
    virtual void FocusChromeOSStatus() = 0;

    // Moves keyboard focus to the next pane.
    virtual void RotatePaneFocus(bool forwards) = 0;

    // Returns whether the bookmark bar is visible or not.
    virtual bool IsBookmarkBarVisible() const = 0;

    // Returns whether the bookmark bar is animating or not.
    virtual bool IsBookmarkBarAnimating() const = 0;

    // Returns whether the tab strip is editable (for extensions).
    virtual bool IsTabStripEditable() const = 0;

    // Returns whether the tool bar is visible or not.
    virtual bool IsToolbarVisible() const = 0;

    // Tells the frame not to render as inactive until the next activation change.
    // This is required on Windows when dropdown selects are shown to prevent the
    // select from deactivating the browser frame. A stub implementation is
    // provided here since the functionality is Windows-specific.
    virtual void DisableInactiveFrame() {}

    // Shows or hides the bookmark bar depending on its current visibility.
    virtual void ToggleBookmarkBar() = 0;

    // Shows the About Chrome dialog box.
    virtual void ShowAboutChromeDialog() = 0;

    // Shows the Update Recommended dialog box.
    virtual void ShowUpdateChromeDialog() = 0;

    // Shows the Task manager.
    virtual void ShowTaskManager() = 0;

    // Shows task information related to background pages.
    virtual void ShowBackgroundPages() = 0;

    // Shows the Bookmark bubble. |url| is the URL being bookmarked,
    // |already_bookmarked| is true if the url is already bookmarked.
    virtual void ShowBookmarkBubble(const Url& url, bool already_bookmarked) = 0;

    // Shows the repost form confirmation dialog box.
    virtual void ShowRepostFormWarningDialog(TabContents* tab_contents) = 0;

    // Shows the collected cookies dialog box.
    virtual void ShowCollectedCookiesDialog(TabContents* tab_contents) = 0;

    // ThemeService calls this when a user has changed his or her theme,
    // indicating that it's time to redraw everything.
    virtual void UserChangedTheme() = 0;

    // Get extra vertical height that the render view should add to its requests
    // to webkit. This can help prevent sending extraneous layout/repaint requests
    // when the delegate is in the process of resizing the tab contents view (e.g.
    // during infobar animations).
    virtual int GetExtraRenderViewHeight() const = 0;

    // Notification that |tab_contents| got the focus through user action (click
    // on the page).
    virtual void TabContentsFocused(TabContents* tab_contents) = 0;

    // Shows the app menu (for accessibility).
    virtual void ShowAppMenu() = 0;

    // Clipboard commands applied to the whole browser window.
    virtual void Cut() = 0;
    virtual void Copy() = 0;
    virtual void Paste() = 0;

    // Switches between available tabstrip display modes.
    virtual void ToggleTabStripMode() = 0;

    // Return the correct disposition for a popup window based on |bounds|.
    virtual WindowOpenDisposition GetDispositionForPopupBounds(
        const gfx::Rect& bounds) = 0;

    // Construct a BrowserWindow implementation for the specified |browser|.
    static BrowserWindow* CreateBrowserWindow(Browser* browser);

protected:
    friend class BrowserList;
    friend class BrowserView;
    virtual void DestroyBrowser() = 0;
};

class BookmarkBarView;
class LocationBarView;

namespace view
{
class View;
}

#endif //__browser_window_h__