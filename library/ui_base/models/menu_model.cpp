
#include "menu_model.h"

namespace ui
{

int MenuModel::GetFirstItemIndex(HMENU native_menu) const
{
    return 0;
}

bool MenuModel::IsVisibleAt(int index) const
{
    return true;
}

bool MenuModel::GetModelAndIndexForCommandId(int command_id,
        MenuModel** model, int* index)
{
    const int item_count = (*model)->GetItemCount();
    const int index_offset = (*model)->GetFirstItemIndex(NULL);
    for(int i=0; i<item_count; ++i)
    {
        const int candidate_index = i + index_offset;
        if((*model)->GetTypeAt(candidate_index) == TYPE_SUBMENU)
        {
            MenuModel* submenu_model = (*model)->GetSubmenuModelAt(candidate_index);
            if(GetModelAndIndexForCommandId(command_id, &submenu_model, index))
            {
                *model = submenu_model;
                return true;
            }
        }
        if((*model)->GetCommandIdAt(candidate_index) == command_id)
        {
            *index = candidate_index;
            return true;
        }
    }
    return false;
}

const gfx::Font* MenuModel::GetLabelFontAt(int index) const
{
    return NULL;
}

// Default implementation ignores the event flags.
void MenuModel::ActivatedAt(int index, int event_flags)
{
    ActivatedAt(index);
}

} //namespace ui