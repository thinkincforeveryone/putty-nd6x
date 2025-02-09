
#ifndef __view_flash_view_h__
#define __view_flash_view_h__

#include "flash10t.tlh"

#include "view/activex/ax_host.h"
#include "view/view.h"

namespace view
{

class FlashView : public View, public AxHostDelegate
{
public:
    explicit FlashView();
    virtual ~FlashView();

    ShockwaveFlashObjects::IShockwaveFlash* flash()  const
    {
        return flash_;
    }
    bool Play(const std::wstring& url);

    // Overridden from View:
    virtual bool OnSetCursor(const gfx::Point& p);
    virtual bool OnMousePressed(const MouseEvent& event);
    virtual bool OnMouseDragged(const MouseEvent& event);
    virtual void OnMouseReleased(const MouseEvent& event);
    virtual void OnMouseMoved(const MouseEvent& event);
    virtual bool OnKeyPressed(const KeyEvent& event);
    virtual bool OnKeyReleased(const KeyEvent& event);

protected:
    // Overridden from View:
    virtual void OnPaint(gfx::Canvas* canvas);
    virtual void Layout();
    virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);

    // Overridden from AxHostDelegate:
    virtual HWND GetAxHostWindow() const;
    virtual void OnAxCreate(AxHost* host);
    virtual void OnAxInvalidate(const gfx::Rect& rect);

private:
    scoped_ptr<AxHost> ax_host_;
    base::win::ScopedComPtr<ShockwaveFlashObjects::IShockwaveFlash> flash_;

    bool initialized_;
};

} //namespace view

#endif //__view_flash_view_h__