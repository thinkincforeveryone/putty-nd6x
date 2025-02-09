
#include "menu_model_adapter.h"

#include "base/logging.h"
#include "base/utf_string_conversions.h"

#include "ui_base/l10n/l10n_util.h"
#include "ui_base/models/menu_model.h"

#include "submenu_view.h"
#include "view/view_delegate.h"

namespace view
{

MenuModelAdapter::MenuModelAdapter(ui::MenuModel* menu_model)
    : menu_model_(menu_model),
      triggerable_event_flags_(ui::EF_LEFT_BUTTON_DOWN |
                               ui::EF_RIGHT_BUTTON_DOWN)
{
    DCHECK(menu_model);
}

MenuModelAdapter::~MenuModelAdapter() {}

void MenuModelAdapter::BuildMenu(MenuItemView* menu)
{
    DCHECK(menu);

    // Clear the menu.
    if(menu->HasSubmenu())
    {
        const int subitem_count = menu->GetSubmenu()->child_count();
        for(int i=0; i<subitem_count; ++i)
        {
            menu->RemoveMenuItemAt(0);
        }
    }

    // Leave entries in the map if the menu is being shown.  This
    // allows the map to find the menu model of submenus being closed
    // so ui::MenuModel::MenuClosed() can be called.
    if(!menu->GetMenuController())
    {
        menu_map_.clear();
    }
    menu_map_[menu] = menu_model_;

    // Repopulate the menu.
    BuildMenuImpl(menu, menu_model_);
    menu->ChildrenChanged();
}

MenuItemView* MenuModelAdapter::CreateMenu()
{
    MenuItemView* item = new MenuItemView(this);
    BuildMenu(item);
    return item;
}

// MenuModelAdapter, MenuDelegate implementation:

void MenuModelAdapter::ExecuteCommand(int id)
{
    ui::MenuModel* model = menu_model_;
    int index = 0;
    if(ui::MenuModel::GetModelAndIndexForCommandId(id, &model, &index))
    {
        model->ActivatedAt(index);
        return;
    }

    NOTREACHED();
}

bool MenuModelAdapter::IsTriggerableEvent(MenuItemView* source,
        const MouseEvent& e)
{
    return (triggerable_event_flags_ & e.flags()) != 0;
}

void MenuModelAdapter::ExecuteCommand(int id, int mouse_event_flags)
{
    ui::MenuModel* model = menu_model_;
    int index = 0;
    if(ui::MenuModel::GetModelAndIndexForCommandId(id, &model, &index))
    {
        model->ActivatedAt(index, mouse_event_flags);
        return;
    }

    NOTREACHED();
}

bool MenuModelAdapter::GetAccelerator(int id, Accelerator* accelerator)
{
    ui::MenuModel* model = menu_model_;
    int index = 0;
    if(ui::MenuModel::GetModelAndIndexForCommandId(id, &model, &index))
    {
        return model->GetAcceleratorAt(index, accelerator);
    }

    NOTREACHED();
    return false;
}

std::wstring MenuModelAdapter::GetLabel(int id) const
{
    ui::MenuModel* model = menu_model_;
    int index = 0;
    if(ui::MenuModel::GetModelAndIndexForCommandId(id, &model, &index))
    {
        return UTF16ToWide(model->GetLabelAt(index));
    }

    NOTREACHED();
    return std::wstring();
}

const gfx::Font& MenuModelAdapter::GetLabelFont(int id) const
{
    ui::MenuModel* model = menu_model_;
    int index = 0;
    if(ui::MenuModel::GetModelAndIndexForCommandId(id, &model, &index))
    {
        const gfx::Font* font = model->GetLabelFontAt(index);
        return font ? *font : MenuDelegate::GetLabelFont(id);
    }

    // This line may be reached for the empty menu item.
    return MenuDelegate::GetLabelFont(id);
}

bool MenuModelAdapter::IsCommandEnabled(int id) const
{
    ui::MenuModel* model = menu_model_;
    int index = 0;
    if(ui::MenuModel::GetModelAndIndexForCommandId(id, &model, &index))
    {
        return model->IsEnabledAt(index);
    }

    NOTREACHED();
    return false;
}

bool MenuModelAdapter::IsItemChecked(int id) const
{
    ui::MenuModel* model = menu_model_;
    int index = 0;
    if(ui::MenuModel::GetModelAndIndexForCommandId(id, &model, &index))
    {
        return model->IsItemCheckedAt(index);
    }

    NOTREACHED();
    return false;
}

void MenuModelAdapter::SelectionChanged(MenuItemView* menu)
{
    // Ignore selection of the root menu.
    if(menu == menu->GetRootMenuItem())
    {
        return;
    }

    const int id = menu->GetCommand();
    ui::MenuModel* model = menu_model_;
    int index = 0;
    if(ui::MenuModel::GetModelAndIndexForCommandId(id, &model, &index))
    {
        model->HighlightChangedTo(index);
        return;
    }

    NOTREACHED();
}

void MenuModelAdapter::WillShowMenu(MenuItemView* menu)
{
    // Look up the menu model for this menu.
    const std::map<MenuItemView*, ui::MenuModel*>::const_iterator map_iterator =
        menu_map_.find(menu);
    if(map_iterator != menu_map_.end())
    {
        map_iterator->second->MenuWillShow();
        return;
    }

    NOTREACHED();
}

void MenuModelAdapter::WillHideMenu(MenuItemView* menu)
{
    // Look up the menu model for this menu.
    const std::map<MenuItemView*, ui::MenuModel*>::const_iterator map_iterator =
        menu_map_.find(menu);
    if(map_iterator != menu_map_.end())
    {
        map_iterator->second->MenuClosed();
        return;
    }

    NOTREACHED();
}

// MenuModelAdapter, private:

void MenuModelAdapter::BuildMenuImpl(MenuItemView* menu, ui::MenuModel* model)
{
    DCHECK(menu);
    DCHECK(model);
    const int item_count = model->GetItemCount();
    for(int i=0; i<item_count; ++i)
    {
        const int index = i + model->GetFirstItemIndex(NULL);
        MenuItemView* item = menu->AppendMenuItemFromModel(
                                 model, index, model->GetCommandIdAt(index));

        if(model->GetTypeAt(index) == ui::MenuModel::TYPE_SUBMENU)
        {
            DCHECK(item);
            DCHECK_EQ(MenuItemView::SUBMENU, item->GetType());
            ui::MenuModel* submodel = model->GetSubmenuModelAt(index);
            DCHECK(submodel);
            BuildMenuImpl(item, submodel);

            menu_map_[item] = submodel;
        }
    }

    menu->set_has_icons(model->HasIcons());
}

} //namespace view