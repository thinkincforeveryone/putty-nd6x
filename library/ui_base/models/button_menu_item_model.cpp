
#include "button_menu_item_model.h"

#include "ui_base/l10n/l10n_util.h"

namespace ui
{

bool ButtonMenuItemModel::Delegate::IsItemForCommandIdDynamic(
    int command_id) const
{
    return false;
}

string16 ButtonMenuItemModel::Delegate::GetLabelForCommandId(
    int command_id) const
{
    return string16();
}

bool ButtonMenuItemModel::Delegate::IsCommandIdEnabled(
    int command_id) const
{
    return true;
}

bool ButtonMenuItemModel::Delegate::DoesCommandIdDismissMenu(
    int command_id) const
{
    return true;
}

struct ButtonMenuItemModel::Item
{
    int command_id;
    ButtonType type;
    string16 label;
    int icon_idr;
    bool part_of_group;
};

ButtonMenuItemModel::ButtonMenuItemModel(
    int string_id,
    ButtonMenuItemModel::Delegate* delegate)
    : item_label_(ui::GetStringUTF16(string_id)),
      delegate_(delegate) {}

ButtonMenuItemModel::~ButtonMenuItemModel() {}

void ButtonMenuItemModel::AddGroupItemWithStringId(
    int command_id, int string_id)
{
    Item item = { command_id, TYPE_BUTTON,
                  ui::GetStringUTF16(string_id), -1, true
                };
    items_.push_back(item);
}

void ButtonMenuItemModel::AddItemWithImage(int command_id,
        int icon_idr)
{
    Item item = { command_id, TYPE_BUTTON, string16(),
                  icon_idr, false
                };
    items_.push_back(item);
}

void ButtonMenuItemModel::AddButtonLabel(int command_id, int string_id)
{
    Item item = { command_id, TYPE_BUTTON_LABEL,
                  ui::GetStringUTF16(string_id), -1, false
                };
    items_.push_back(item);
}

void ButtonMenuItemModel::AddSpace()
{
    Item item = { 0, TYPE_SPACE, string16(), -1, false };
    items_.push_back(item);
}

int ButtonMenuItemModel::GetItemCount() const
{
    return static_cast<int>(items_.size());
}

ButtonMenuItemModel::ButtonType ButtonMenuItemModel::GetTypeAt(
    int index) const
{
    return items_[index].type;
}

int ButtonMenuItemModel::GetCommandIdAt(int index) const
{
    return items_[index].command_id;
}

bool ButtonMenuItemModel::IsItemDynamicAt(int index) const
{
    if(delegate_)
    {
        return delegate_->IsItemForCommandIdDynamic(
                   GetCommandIdAt(index));
    }
    return false;
}

string16 ButtonMenuItemModel::GetLabelAt(int index) const
{
    if(IsItemDynamicAt(index))
    {
        return delegate_->GetLabelForCommandId(GetCommandIdAt(index));
    }
    return items_[index].label;
}

bool ButtonMenuItemModel::GetIconAt(int index, int* icon_idr) const
{
    if(items_[index].icon_idr == -1)
    {
        return false;
    }

    *icon_idr = items_[index].icon_idr;
    return true;
}

bool ButtonMenuItemModel::PartOfGroup(int index) const
{
    return items_[index].part_of_group;
}

void ButtonMenuItemModel::ActivatedCommand(int command_id)
{
    if(delegate_)
    {
        delegate_->ExecuteCommand(command_id);
    }
}

bool ButtonMenuItemModel::IsEnabledAt(int index) const
{
    return IsCommandIdEnabled(items_[index].command_id);
}

bool ButtonMenuItemModel::DismissesMenuAt(int index) const
{
    return DoesCommandIdDismissMenu(items_[index].command_id);
}

bool ButtonMenuItemModel::IsCommandIdEnabled(int command_id) const
{
    if(delegate_)
    {
        return delegate_->IsCommandIdEnabled(command_id);
    }
    return true;
}

bool ButtonMenuItemModel::DoesCommandIdDismissMenu(int command_id) const
{
    if(delegate_)
    {
        return delegate_->DoesCommandIdDismissMenu(command_id);
    }
    return true;
}

} //namespace ui