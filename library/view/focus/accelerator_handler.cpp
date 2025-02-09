
#include "accelerator_handler.h"

#include "focus_manager.h"
#include "view/widget/widget.h"

namespace view
{

AcceleratorHandler::AcceleratorHandler() {}

bool AcceleratorHandler::Dispatch(const MSG& msg)
{
    bool process_message = true;

    if(msg.message>=WM_KEYFIRST && msg.message<=WM_KEYLAST)
    {
        Widget* widget = Widget::GetTopLevelWidgetForNativeView(msg.hwnd);
        FocusManager* focus_manager = widget ? widget->GetFocusManager() : NULL;
        if(focus_manager)
        {
            switch(msg.message)
            {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                process_message = focus_manager->OnKeyEvent(KeyEvent(msg));
                if(!process_message)
                {
                    // Record that this key is pressed so we can remember not to
                    // translate and dispatch the associated WM_KEYUP.
                    pressed_keys_.insert(msg.wParam);
                }
                break;
            }
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                std::set<WPARAM>::iterator iter = pressed_keys_.find(msg.wParam);
                if(iter != pressed_keys_.end())
                {
                    // Don't translate/dispatch the KEYUP since we have eaten the
                    // associated KEYDOWN.
                    pressed_keys_.erase(iter);
                    return true;
                }
                break;
            }
            }
        }
    }

    if(process_message)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return true;
}

} //namespace view