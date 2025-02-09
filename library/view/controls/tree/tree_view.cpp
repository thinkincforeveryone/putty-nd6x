
#include "tree_view.h"

#include "base/logging.h"
#include "base/stl_utilinl.h"
#include "base/win/win_util.h"

#include "ui_gfx/canvas_skia.h"
#include "ui_gfx/icon_util.h"

#include "ui_base/keycodes/keyboard_code_conversion_win.h"
#include "ui_base/l10n/l10n_util_win.h"
#include "ui_base/resource/app_res_ids.h"
#include "ui_base/resource/resource_bundle.h"
#include "ui_base/win/hwnd_util.h"

#include "view/accessibility/native_view_accessibility_win.h"
#include "view/focus/focus_manager.h"

namespace view
{

TreeView::TreeView()
    : tree_view_(NULL),
      model_(NULL),
      auto_expand_children_(false),
      editable_(true),
      next_id_(0),
      controller_(NULL),
      editing_node_(NULL),
      root_shown_(true),
      lines_at_root_(false),
      process_enter_(false),
      show_context_menu_only_when_node_selected_(true),
      select_on_right_mouse_down_(true),
      wrapper_(this),
      original_handler_(NULL),
      drag_enabled_(false),
      observer_added_(false),
      has_custom_icons_(false),
      image_list_(NULL) {}

TreeView::~TreeView()
{
    Cleanup();
}

void TreeView::GetAccessibleState(ui::AccessibleViewState* state)
{
    state->role = ui::AccessibilityTypes::ROLE_OUTLINE;
    state->state = ui::AccessibilityTypes::STATE_READONLY;
}

void TreeView::SetModel(ui::TreeModel* model)
{
    if(model == model_)
    {
        return;
    }
    if(model_ && tree_view_)
    {
        DeleteRootItems();
    }

    RemoveObserverFromModel();

    model_ = model;
    if(tree_view_ && model_)
    {
        CreateRootItems();
        AddObserverToModel();
        HIMAGELIST last_image_list = image_list_;
        image_list_ = CreateImageList();
        TreeView_SetImageList(tree_view_, image_list_, TVSIL_NORMAL);
        if(last_image_list)
        {
            ImageList_Destroy(last_image_list);
        }
    }
}

// Sets whether the user can edit the nodes. The default is true.
void TreeView::SetEditable(bool editable)
{
    if(editable == editable_)
    {
        return;
    }
    editable_ = editable;
    if(!tree_view_)
    {
        return;
    }
    LONG_PTR style = GetWindowLongPtr(tree_view_, GWL_STYLE);
    style &= ~TVS_EDITLABELS;
    SetWindowLongPtr(tree_view_, GWL_STYLE, style);
}

void TreeView::StartEditing(ui::TreeModelNode* node)
{
    DCHECK(node && tree_view_);
    // Cancel the current edit.
    CancelEdit();
    // Make sure all ancestors are expanded.
    if(model_->GetParent(node))
    {
        Expand(model_->GetParent(node));
    }
    const NodeDetails* details = GetNodeDetails(node);
    // Tree needs focus for editing to work.
    SetFocus(tree_view_);
    // Select the node, else if the user commits the edit the selection reverts.
    SetSelectedNode(node);
    TreeView_EditLabel(tree_view_, details->tree_item);
}

void TreeView::CancelEdit()
{
    DCHECK(tree_view_);
    TreeView_EndEditLabelNow(tree_view_, TRUE);
}

void TreeView::CommitEdit()
{
    DCHECK(tree_view_);
    TreeView_EndEditLabelNow(tree_view_, FALSE);
}

ui::TreeModelNode* TreeView::GetEditingNode()
{
    // I couldn't find a way to dynamically query for this, so it is cached.
    return editing_node_;
}

void TreeView::SetSelectedNode(ui::TreeModelNode* node)
{
    DCHECK(tree_view_);
    if(!node)
    {
        TreeView_SelectItem(tree_view_, NULL);
        return;
    }
    if(node != model_->GetRoot())
    {
        Expand(model_->GetParent(node));
    }
    if(!root_shown_ && node==model_->GetRoot())
    {
        // If the root isn't shown, we can't select it, clear out the selection
        // instead.
        TreeView_SelectItem(tree_view_, NULL);
    }
    else
    {
        // Select the node and make sure it is visible.
        TreeView_SelectItem(tree_view_, GetNodeDetails(node)->tree_item);
    }
}

ui::TreeModelNode* TreeView::GetSelectedNode()
{
    if(!tree_view_)
    {
        return NULL;
    }
    HTREEITEM selected_item = TreeView_GetSelection(tree_view_);
    if(!selected_item)
    {
        return NULL;
    }
    NodeDetails* details = GetNodeDetailsByTreeItem(selected_item);
    DCHECK(details);
    return details->node;
}

void TreeView::Expand(ui::TreeModelNode* node)
{
    DCHECK(model_ && node);
    if(!root_shown_ && model_->GetRoot()==node)
    {
        // Can only expand the root if it is showing.
        return;
    }
    ui::TreeModelNode* parent = model_->GetParent(node);
    if(parent)
    {
        // Make sure all the parents are expanded.
        Expand(parent);
    }
    // And expand this item.
    TreeView_Expand(tree_view_, GetNodeDetails(node)->tree_item, TVE_EXPAND);
}

void TreeView::ExpandAll(ui::TreeModelNode* node)
{
    DCHECK(node);
    // Expand the node.
    if(node!=model_->GetRoot() || root_shown_)
    {
        TreeView_Expand(tree_view_, GetNodeDetails(node)->tree_item, TVE_EXPAND);
    }
    // And recursively expand all the children.
    for(int i=model_->GetChildCount(node)-1; i>=0; --i)
    {
        ui::TreeModelNode* child = model_->GetChild(node, i);
        ExpandAll(child);
    }
}

bool TreeView::IsExpanded(ui::TreeModelNode* node)
{
    if(!tree_view_)
    {
        return false;
    }
    ui::TreeModelNode* parent = model_->GetParent(node);
    if(!parent)
    {
        return true;
    }
    if(!IsExpanded(parent))
    {
        return false;
    }
    NodeDetails* details = GetNodeDetails(node);
    return (TreeView_GetItemState(tree_view_, details->tree_item, TVIS_EXPANDED) &
            TVIS_EXPANDED) != 0;
}

void TreeView::SetRootShown(bool root_shown)
{
    if(root_shown_ == root_shown)
    {
        return;
    }
    root_shown_ = root_shown;
    if(!model_ || !tree_view_)
    {
        return;
    }
    // Repopulate the tree.
    DeleteRootItems();
    CreateRootItems();
}

void TreeView::TreeNodesAdded(ui::TreeModel* model,
                              ui::TreeModelNode* parent,
                              int start,
                              int count)
{
    DCHECK(parent && start>=0 && count>0);
    if(node_to_details_map_.find(parent)==node_to_details_map_.end() &&
            (root_shown_ || parent!=model_->GetRoot()))
    {
        // User hasn't navigated to this entry yet. Ignore the change.
        return;
    }
    HTREEITEM parent_tree_item = NULL;
    if(root_shown_ || parent!=model_->GetRoot())
    {
        const NodeDetails* details = GetNodeDetails(parent);
        if(!details->loaded_children)
        {
            if(count == model_->GetChildCount(parent))
            {
                // Reset the treeviews child count. This triggers the treeview to call
                // us back.
                TV_ITEM tv_item = { 0 };
                tv_item.mask = TVIF_CHILDREN;
                tv_item.cChildren = count;
                tv_item.hItem = details->tree_item;
                TreeView_SetItem(tree_view_, &tv_item);
            }

            // Ignore the change, we haven't actually created entries in the tree
            // for the children.
            return;
        }
        parent_tree_item = details->tree_item;
    }

    // The user has expanded this node, add the items to it.
    for(int i=0; i<count; ++i)
    {
        if(i==0 && start==0)
        {
            CreateItem(parent_tree_item, TVI_FIRST, model_->GetChild(parent, 0));
        }
        else
        {
            ui::TreeModelNode* previous_sibling = model_->GetChild(parent, i+start-1);
            CreateItem(parent_tree_item, GetNodeDetails(previous_sibling)->tree_item,
                       model_->GetChild(parent, i+start));
        }
    }
}

void TreeView::TreeNodesRemoved(ui::TreeModel* model,
                                ui::TreeModelNode* parent,
                                int start,
                                int count)
{
    DCHECK(parent && start>=0 && count>0);

    HTREEITEM tree_item;
    if(!root_shown_ && parent==model->GetRoot())
    {
        // NOTE: we can't call GetTreeItemForNodeDuringMutation here as in this
        // configuration the root has no treeitem.
        tree_item = TreeView_GetRoot(tree_view_);
    }
    else
    {
        HTREEITEM parent_tree_item = GetTreeItemForNodeDuringMutation(parent);
        if(!parent_tree_item)
        {
            return;
        }

        tree_item = TreeView_GetChild(tree_view_, parent_tree_item);
    }

    // Find the last item. Windows doesn't offer a convenient way to get the
    // TREEITEM at a particular index, so we iterate.
    for(int i=0; i<(start+count-1); ++i)
    {
        tree_item = TreeView_GetNextSibling(tree_view_, tree_item);
    }

    // NOTE: the direction doesn't matter here. I've made it backwards to
    // reinforce we're deleting from the end forward.
    for(int i=count-1; i>=0; --i)
    {
        HTREEITEM previous = (start+i)>0 ?
                             TreeView_GetPrevSibling(tree_view_, tree_item) : NULL;
        RecursivelyDelete(GetNodeDetailsByTreeItem(tree_item));
        tree_item = previous;
    }
}

void TreeView::TreeNodeChanged(ui::TreeModel* model, ui::TreeModelNode* node)
{
    if(node_to_details_map_.find(node) == node_to_details_map_.end())
    {
        // User hasn't navigated to this entry yet. Ignore the change.
        return;
    }
    const NodeDetails* details = GetNodeDetails(node);
    TV_ITEM tv_item = { 0 };
    tv_item.mask = TVIF_TEXT;
    tv_item.hItem = details->tree_item;
    tv_item.pszText = LPSTR_TEXTCALLBACK;
    TreeView_SetItem(tree_view_, &tv_item);
}

gfx::Point TreeView::GetKeyboardContextMenuLocation()
{
    int y = height() / 2;
    if(GetSelectedNode())
    {
        RECT bounds;
        RECT client_rect;
        if(TreeView_GetItemRect(tree_view_,
                                GetNodeDetails(GetSelectedNode())->tree_item,
                                &bounds, TRUE) &&
                GetClientRect(tree_view_, &client_rect) &&
                bounds.bottom>=0 && bounds.bottom<client_rect.bottom)
        {
            y = bounds.bottom;
        }
    }
    gfx::Point screen_loc(0, y);
    if(base::i18n::IsRTL())
    {
        screen_loc.set_x(width());
    }
    ConvertPointToScreen(this, &screen_loc);
    return screen_loc;
}

HWND TreeView::CreateNativeControl(HWND parent_container)
{
    int style = WS_CHILD | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS;
    if(!drag_enabled_)
    {
        style |= TVS_DISABLEDRAGDROP;
    }
    if(editable_)
    {
        style |= TVS_EDITLABELS;
    }
    if(lines_at_root_)
    {
        style |= TVS_LINESATROOT;
    }
    tree_view_ = ::CreateWindowExW(WS_EX_CLIENTEDGE|GetAdditionalExStyle(),
                                   WC_TREEVIEW,
                                   L"",
                                   style,
                                   0, 0, width(), height(),
                                   parent_container, NULL, NULL, NULL);
    ui::CheckWindowCreated(tree_view_);
    SetWindowLongPtr(tree_view_, GWLP_USERDATA,
                     reinterpret_cast<LONG_PTR>(&wrapper_));
    original_handler_ = ui::SetWindowProc(tree_view_, &TreeWndProc);
    ui::AdjustUIFontForWindow(tree_view_);

    if(model_)
    {
        CreateRootItems();
        AddObserverToModel();
        image_list_ = CreateImageList();
        TreeView_SetImageList(tree_view_, image_list_, TVSIL_NORMAL);
    }
    return tree_view_;
}

LRESULT TreeView::OnNotify(int w_param, LPNMHDR l_param)
{
    switch(l_param->code)
    {
    case TVN_GETDISPINFO:
    {
        // Windows is requesting more information about an item.
        // WARNING: At the time this is called the tree_item of the NodeDetails
        // in the maps is NULL.
        DCHECK(model_);
        NMTVDISPINFO* info = reinterpret_cast<NMTVDISPINFO*>(l_param);

        // WARNING: its possible for Windows to send a TVN_GETDISPINFO message
        // after the WM_DESTROY time of the native control.  Since the details
        // map will be cleaned up on OnDestroy(), don't try to access it in
        // this case.
        if(!id_to_details_map_.empty())
        {
            const NodeDetails* details =
                GetNodeDetailsByID(static_cast<int>(info->item.lParam));
            if(info->item.mask & TVIF_CHILDREN)
            {
                info->item.cChildren = model_->GetChildCount(details->node);
            }
            if(info->item.mask & TVIF_TEXT)
            {
                std::wstring text = details->node->GetTitle();
                DCHECK(info->item.cchTextMax);

                // Adjust the string direction if such adjustment is required.
                base::i18n::AdjustStringForLocaleDirection(&text);

                wcsncpy_s(info->item.pszText, info->item.cchTextMax, text.c_str(),
                          _TRUNCATE);
            }
            // Instructs windows to cache the values for this node.
            info->item.mask |= TVIF_DI_SETITEM;
        }
        else
        {
            if(info->item.mask & TVIF_CHILDREN)
            {
                info->item.cChildren = 0;
            }

            if(info->item.mask & TVIF_TEXT)
            {
                wcsncpy_s(info->item.pszText, info->item.cchTextMax, L"",
                          _TRUNCATE);
            }
        }

        // Return value ignored.
        return 0;
    }

    case TVN_ITEMEXPANDING:
    {
        // Notification that a node is expanding. If we haven't populated the
        // tree view with the contents of the model, we do it here.
        DCHECK(model_);
        NMTREEVIEW* info = reinterpret_cast<NMTREEVIEW*>(l_param);
        NodeDetails* details =
            GetNodeDetailsByID(static_cast<int>(info->itemNew.lParam));
        if(!details->loaded_children)
        {
            details->loaded_children = true;
            for(int i=0; i<model_->GetChildCount(details->node); ++i)
            {
                CreateItem(details->tree_item, TVI_LAST,
                           model_->GetChild(details->node, i));
                if(auto_expand_children_)
                {
                    Expand(model_->GetChild(details->node, i));
                }
            }
        }
        // Return FALSE to allow the item to be expanded.
        return FALSE;
    }

    case TVN_SELCHANGED:
        if(controller_)
        {
            controller_->OnTreeViewSelectionChanged(this);
        }
        break;

    case TVN_BEGINLABELEDIT:
    {
        NMTVDISPINFO* info = reinterpret_cast<NMTVDISPINFO*>(l_param);
        NodeDetails* details =
            GetNodeDetailsByID(static_cast<int>(info->item.lParam));
        // Return FALSE to allow editing.
        if(!controller_ || controller_->CanEdit(this, details->node))
        {
            editing_node_ = details->node;
            return FALSE;
        }
        return TRUE;
    }

    case TVN_ENDLABELEDIT:
    {
        NMTVDISPINFO* info = reinterpret_cast<NMTVDISPINFO*>(l_param);
        if(info->item.pszText)
        {
            // User accepted edit.
            NodeDetails* details =
                GetNodeDetailsByID(static_cast<int>(info->item.lParam));
            model_->SetTitle(details->node, info->item.pszText);
            editing_node_ = NULL;
            // Return FALSE so that the tree item doesn't change its text (if the
            // model changed the value, it should have sent out notification which
            // will have updated the value).
            return FALSE;
        }
        editing_node_ = NULL;
        // Return value ignored.
        return 0;
    }

    case TVN_KEYDOWN:
        if(controller_)
        {
            NMTVKEYDOWN* key_down_message =
                reinterpret_cast<NMTVKEYDOWN*>(l_param);
            controller_->OnTreeViewKeyDown(
                ui::KeyboardCodeForWindowsKeyCode(key_down_message->wVKey));
        }
        break;

    default:
        break;
    }
    return 0;
}

void TreeView::OnDestroy()
{
    Cleanup();
}

bool TreeView::OnKeyDown(ui::KeyboardCode virtual_key_code)
{
    if(virtual_key_code == VK_F2)
    {
        if(!GetEditingNode())
        {
            ui::TreeModelNode* selected_node = GetSelectedNode();
            if(selected_node)
            {
                StartEditing(selected_node);
            }
        }
        return true;
    }
    else if(virtual_key_code==ui::VKEY_RETURN && !process_enter_)
    {
        Widget* widget = GetWidget();
        DCHECK(widget);
        Accelerator accelerator(Accelerator(virtual_key_code,
                                            base::win::IsShiftPressed(),
                                            base::win::IsCtrlPressed(),
                                            base::win::IsAltPressed()));
        GetFocusManager()->ProcessAccelerator(accelerator);
        return true;
    }
    return false;
}

void TreeView::OnContextMenu(const POINT& location)
{
    if(!context_menu_controller())
    {
        return;
    }

    if(location.x==-1 && location.y==-1)
    {
        // Let NativeControl's implementation handle keyboard gesture.
        NativeControl::OnContextMenu(location);
        return;
    }

    if(show_context_menu_only_when_node_selected_)
    {
        if(!GetSelectedNode())
        {
            return;
        }

        // Make sure the mouse is over the selected node.
        TVHITTESTINFO hit_info;
        gfx::Point local_loc(location);
        ConvertPointToView(NULL, this, &local_loc);
        hit_info.pt = local_loc.ToPOINT();
        HTREEITEM hit_item = TreeView_HitTest(tree_view_, &hit_info);
        if(!hit_item ||
                GetNodeDetails(GetSelectedNode())->tree_item!=hit_item ||
                (hit_info.flags&
                 (TVHT_ONITEM|TVHT_ONITEMRIGHT|TVHT_ONITEMINDENT))==0)
        {
            return;
        }
    }
    ShowContextMenu(gfx::Point(location), true);
}

ui::TreeModelNode* TreeView::GetNodeForTreeItem(HTREEITEM tree_item)
{
    NodeDetails* details = GetNodeDetailsByTreeItem(tree_item);
    return details ? details->node : NULL;
}

HTREEITEM TreeView::GetTreeItemForNode(ui::TreeModelNode* node)
{
    NodeDetails* details = GetNodeDetails(node);
    return details ? details->tree_item : NULL;
}

void TreeView::Cleanup()
{
    RemoveObserverFromModel();

    // Both node_to_details_map_ and node_to_details_map_ have the same value,
    // as such only need to delete from one.
    STLDeleteContainerPairSecondPointers(id_to_details_map_.begin(),
                                         id_to_details_map_.end());
    id_to_details_map_.clear();
    node_to_details_map_.clear();

    if(image_list_)
    {
        ImageList_Destroy(image_list_);
        image_list_ = NULL;
    }
}

void TreeView::AddObserverToModel()
{
    if(model_ && !observer_added_)
    {
        model_->AddObserver(this);
        observer_added_ = true;
    }
}

void TreeView::RemoveObserverFromModel()
{
    if(model_ && observer_added_)
    {
        model_->RemoveObserver(this);
        observer_added_ = false;
    }
}

void TreeView::DeleteRootItems()
{
    HTREEITEM root = TreeView_GetRoot(tree_view_);
    if(root)
    {
        if(root_shown_)
        {
            RecursivelyDelete(GetNodeDetailsByTreeItem(root));
        }
        else
        {
            do
            {
                RecursivelyDelete(GetNodeDetailsByTreeItem(root));
            }
            while((root = TreeView_GetRoot(tree_view_)));
        }
    }
}

void TreeView::CreateRootItems()
{
    DCHECK(model_);
    DCHECK(tree_view_);
    ui::TreeModelNode* root = model_->GetRoot();
    if(root_shown_)
    {
        CreateItem(NULL, TVI_LAST, root);
    }
    else
    {
        for(int i=0; i<model_->GetChildCount(root); ++i)
        {
            CreateItem(NULL, TVI_LAST, model_->GetChild(root, i));
        }
    }
}

void TreeView::CreateItem(HTREEITEM parent_item,
                          HTREEITEM after, ui::TreeModelNode* node)
{
    DCHECK(node);
    TVINSERTSTRUCT insert_struct = { 0 };
    insert_struct.hParent = parent_item;
    insert_struct.hInsertAfter = after;
    insert_struct.itemex.mask = TVIF_PARAM | TVIF_CHILDREN | TVIF_TEXT |
                                TVIF_SELECTEDIMAGE | TVIF_IMAGE;
    // Call us back for the text.
    insert_struct.itemex.pszText = LPSTR_TEXTCALLBACK;
    // And the number of children.
    insert_struct.itemex.cChildren = I_CHILDRENCALLBACK;
    // Set the index of the icons to use. These are relative to the imagelist
    // created in CreateImageList.
    int icon_index = model_->GetIconIndex(node);
    if(icon_index == -1)
    {
        insert_struct.itemex.iImage = 0;
        insert_struct.itemex.iSelectedImage = 1;
    }
    else
    {
        // The first two images are the default ones.
        insert_struct.itemex.iImage = icon_index + 2;
        insert_struct.itemex.iSelectedImage = icon_index + 2;
    }
    int node_id = next_id_++;
    insert_struct.itemex.lParam = node_id;

    // Invoking TreeView_InsertItem triggers OnNotify to be called. As such,
    // we set the map entries before adding the item.
    NodeDetails* node_details = new NodeDetails(node_id, node);

    DCHECK(node_to_details_map_.count(node) == 0);
    DCHECK(id_to_details_map_.count(node_id) == 0);

    node_to_details_map_[node] = node_details;
    id_to_details_map_[node_id] = node_details;

    node_details->tree_item = TreeView_InsertItem(tree_view_, &insert_struct);
}

void TreeView::RecursivelyDelete(NodeDetails* node)
{
    DCHECK(node);
    HTREEITEM item = node->tree_item;
    DCHECK(item);

    // Recurse through children.
    for(HTREEITEM child=TreeView_GetChild(tree_view_, item); child;)
    {
        HTREEITEM next = TreeView_GetNextSibling(tree_view_, child);
        RecursivelyDelete(GetNodeDetailsByTreeItem(child));
        child = next;
    }

    TreeView_DeleteItem(tree_view_, item);

    // finally, it is safe to delete the data for this node.
    id_to_details_map_.erase(node->id);
    node_to_details_map_.erase(node->node);
    delete node;
}

TreeView::NodeDetails* TreeView::GetNodeDetails(ui::TreeModelNode* node)
{
    DCHECK(node &&
           node_to_details_map_.find(node)!=node_to_details_map_.end());
    return node_to_details_map_[node];
}

// Returns the NodeDetails by identifier (lparam of the HTREEITEM).
TreeView::NodeDetails* TreeView::GetNodeDetailsByID(int id)
{
    DCHECK(id_to_details_map_.find(id) != id_to_details_map_.end());
    return id_to_details_map_[id];
}

TreeView::NodeDetails* TreeView::GetNodeDetailsByTreeItem(HTREEITEM tree_item)
{
    DCHECK(tree_view_ && tree_item);
    TV_ITEM tv_item = { 0 };
    tv_item.hItem = tree_item;
    tv_item.mask = TVIF_PARAM;
    if(TreeView_GetItem(tree_view_, &tv_item))
    {
        return GetNodeDetailsByID(static_cast<int>(tv_item.lParam));
    }
    return NULL;
}

HIMAGELIST TreeView::CreateImageList()
{
    std::vector<SkBitmap> model_images;
    model_->GetIcons(&model_images);

    bool rtl = base::i18n::IsRTL();
    // Creates the default image list used for trees.
    SkBitmap* closed_icon =
        ui::ResourceBundle::GetSharedInstance().GetBitmapNamed(
            (rtl ? IDR_FOLDER_CLOSED_RTL : IDR_FOLDER_CLOSED));
    SkBitmap* opened_icon =
        ui::ResourceBundle::GetSharedInstance().GetBitmapNamed(
            (rtl ? IDR_FOLDER_OPEN_RTL : IDR_FOLDER_OPEN));
    int width = closed_icon->width();
    int height = closed_icon->height();
    DCHECK(opened_icon->width()==width && opened_icon->height()==height);
    HIMAGELIST image_list = ImageList_Create(width, height, ILC_COLOR32,
                            model_images.size()+2, model_images.size()+2);
    if(image_list)
    {
        // NOTE: the order the images are added in effects the selected
        // image index when adding items to the tree. If you change the
        // order you'll undoubtedly need to update itemex.iSelectedImage
        // when the item is added.
        HICON h_closed_icon = IconUtil::CreateHICONFromSkBitmap(*closed_icon);
        HICON h_opened_icon = IconUtil::CreateHICONFromSkBitmap(*opened_icon);
        ImageList_AddIcon(image_list, h_closed_icon);
        ImageList_AddIcon(image_list, h_opened_icon);
        DestroyIcon(h_closed_icon);
        DestroyIcon(h_opened_icon);
        for(size_t i=0; i<model_images.size(); ++i)
        {
            HICON model_icon;

            // Need to resize the provided icons to be the same size as
            // IDR_FOLDER_CLOSED if they aren't already.
            if(model_images[i].width()!=width || model_images[i].height()!=height)
            {
                gfx::CanvasSkia canvas(width, height, false);
                // Make the background completely transparent.
                canvas.drawColor(SK_ColorBLACK, SkXfermode::kClear_Mode);

                // Draw our icons into this canvas.
                int height_offset = (height - model_images[i].height()) / 2;
                int width_offset = (width - model_images[i].width()) / 2;
                canvas.DrawBitmapInt(model_images[i], width_offset, height_offset);
                model_icon = IconUtil::CreateHICONFromSkBitmap(canvas.ExtractBitmap());
            }
            else
            {
                model_icon = IconUtil::CreateHICONFromSkBitmap(model_images[i]);
            }
            ImageList_AddIcon(image_list, model_icon);
            DestroyIcon(model_icon);
        }
    }
    return image_list;
}

HTREEITEM TreeView::GetTreeItemForNodeDuringMutation(ui::TreeModelNode* node)
{
    if(node_to_details_map_.find(node) == node_to_details_map_.end())
    {
        // User hasn't navigated to this entry yet. Ignore the change.
        return NULL;
    }
    if(!root_shown_ || node!=model_->GetRoot())
    {
        const NodeDetails* details = GetNodeDetails(node);
        if(!details->loaded_children)
        {
            return NULL;
        }
        return details->tree_item;
    }
    return TreeView_GetRoot(tree_view_);
}

LRESULT CALLBACK TreeView::TreeWndProc(HWND window,
                                       UINT message,
                                       WPARAM w_param,
                                       LPARAM l_param)
{
    TreeViewWrapper* wrapper = reinterpret_cast<TreeViewWrapper*>(
                                   GetWindowLongPtr(window, GWLP_USERDATA));
    DCHECK(wrapper);
    TreeView* tree = wrapper->tree_view;

    // We handle the messages WM_ERASEBKGND and WM_PAINT such that we paint into
    // a DIB first and then perform a BitBlt from the DIB into the underlying
    // window's DC. This double buffering code prevents the tree view from
    // flickering during resize.
    switch(message)
    {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        gfx::CanvasSkiaPaint canvas(window);
        if(canvas.isEmpty())
        {
            return 0;
        }

        HDC dc = skia::BeginPlatformPaint(&canvas);
        if(base::i18n::IsRTL())
        {
            // gfx::CanvasSkia ends up configuring the DC with a mode of
            // GM_ADVANCED. For some reason a graphics mode of ADVANCED triggers
            // all the text to be mirrored when RTL. Set the mode back to COMPATIBLE
            // and explicitly set the layout. Additionally SetWorldTransform and
            // COMPATIBLE don't play nicely together. We need to use
            // SetViewportOrgEx when using a mode of COMPATIBLE.
            //
            // Reset the transform to the identify transform. Even though
            // SetWorldTransform and COMPATIBLE don't play nicely, bits of the
            // transform still carry over when we set the mode.
            XFORM xform = { 0 };
            xform.eM11 = xform.eM22 = 1;
            SetWorldTransform(dc, &xform);

            // Set the mode and layout.
            SetGraphicsMode(dc, GM_COMPATIBLE);
            SetLayout(dc, LAYOUT_RTL);

            // Transform the viewport such that the origin of the dc is that of
            // the dirty region. This way when we invoke WM_PRINTCLIENT tree-view
            // draws the dirty region at the origin of the DC so that when we
            // copy the bits everything lines up nicely. Without this we end up
            // copying the upper-left corner to the redraw region.
            SetViewportOrgEx(dc, -canvas.paintStruct().rcPaint.left,
                             -canvas.paintStruct().rcPaint.top, NULL);
        }
        SendMessage(window, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), 0);
        if(base::i18n::IsRTL())
        {
            // Reset the origin of the dc back to 0. This way when we copy the bits
            // over we copy the right bits.
            SetViewportOrgEx(dc, 0, 0, NULL);
        }
        skia::EndPlatformPaint(&canvas);
        return 0;
    }

    case WM_RBUTTONDOWN:
        if(tree->select_on_right_mouse_down_)
        {
            TVHITTESTINFO hit_info;
            hit_info.pt = gfx::Point(l_param).ToPOINT();
            HTREEITEM hit_item = TreeView_HitTest(window, &hit_info);
            if(hit_item && (hit_info.flags&
                            (TVHT_ONITEM|TVHT_ONITEMRIGHT|TVHT_ONITEMINDENT))!=0)
            {
                TreeView_SelectItem(tree->tree_view_, hit_item);
            }
        }
        // Fall through and let the default handler process as well.
        break;
    }
    WNDPROC handler = tree->original_handler_;
    DCHECK(handler);
    return CallWindowProc(handler, window, message, w_param, l_param);
}

} //namespace view