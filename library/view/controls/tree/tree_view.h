
#ifndef __view_tree_view_h__
#define __view_tree_view_h__

#pragma once

#include <windows.h>
#include <commctrl.h>

#include "ui_base/models/tree_model.h"

#include "view/controls/native_control.h"

namespace view
{

class TreeView;

// TreeViewController ---------------------------------------------------------

// Controller for the treeview.
class TreeViewController
{
public:
    // Notification that the selection of the tree view has changed. Use
    // GetSelectedNode to find the current selection.
    virtual void OnTreeViewSelectionChanged(TreeView* tree_view) = 0;

    // Returns true if the node can be edited. This is only used if the
    // TreeView is editable.
    virtual bool CanEdit(TreeView* tree_view, ui::TreeModelNode* node)
    {
        return true;
    }

    // Invoked when a key is pressed on the tree view.
    virtual void OnTreeViewKeyDown(ui::KeyboardCode keycode) {}
};

// TreeView -------------------------------------------------------------------

// TreeView displays hierarchical data as returned from a TreeModel. The user
// can expand, collapse and edit the items. A Controller may be attached to
// receive notification of selection changes and restrict editing.
class TreeView : public NativeControl, ui::TreeModelObserver
{
public:
    TreeView();
    virtual ~TreeView();

    // Is dragging enabled? The default is false.
    void set_drag_enabled(bool drag_enabled)
    {
        drag_enabled_ = drag_enabled;
    }
    bool drag_enabled() const
    {
        return drag_enabled_;
    }

    // Sets the model. TreeView does not take ownership of the model.
    void SetModel(ui::TreeModel* model);
    ui::TreeModel* model() const
    {
        return model_;
    }

    // Sets whether to automatically expand children when a parent node is
    // expanded. The default is false. If true, when a node in the tree is
    // expanded for the first time, its children are also automatically expanded.
    // If a node is subsequently collapsed and expanded again, the children
    // will not be automatically expanded.
    void set_auto_expand_children(bool auto_expand_children)
    {
        auto_expand_children_ = auto_expand_children;
    }

    // Sets whether the user can edit the nodes. The default is true. If true,
    // the Controller is queried to determine if a particular node can be edited.
    void SetEditable(bool editable);

    // Sets whether lines are drawn from the root node to child nodes (and
    // whether plus boxes show up next to the root node.) The default is false.
    // If root_shown_ is false, the children of the root act as the roots in the
    // native control, and so this setting takes effect for them.
    void set_lines_at_root(bool lines_at_root)
    {
        lines_at_root_ = lines_at_root;
    }

    // Overridden from View:
    virtual void GetAccessibleState(ui::AccessibleViewState* state);

    // Edits the specified node. This cancels the current edit and expands
    // all parents of node.
    void StartEditing(ui::TreeModelNode* node);

    // Cancels the current edit. Does nothing if not editing.
    void CancelEdit();

    // Commits the current edit. Does nothing if not editing.
    void CommitEdit();

    // If the user is editing a node, it is returned. If the user is not
    // editing a node, NULL is returned.
    ui::TreeModelNode* GetEditingNode();

    // Selects the specified node. This expands all the parents of node.
    void SetSelectedNode(ui::TreeModelNode* node);

    // Returns the selected node, or NULL if nothing is selected.
    ui::TreeModelNode* GetSelectedNode();

    // Make sure node and all its parents are expanded.
    void Expand(ui::TreeModelNode* node);

    // Invoked from ExpandAll(). Expands the supplied node and recursively
    // invokes itself with all children.
    void ExpandAll(ui::TreeModelNode* node);

    // Returns true if the specified node is expanded.
    bool IsExpanded(ui::TreeModelNode* node);

    // Sets whether the root is shown. If true, the root node of the tree is
    // shown, if false only the children of the root are shown. The default is
    // true.
    void SetRootShown(bool root_visible);

    // Begin TreeModelObserver implementation.
    // Don't call these directly, instead your model
    // should notify the observer TreeView adds to it.
    virtual void TreeNodesAdded(ui::TreeModel* model,
                                ui::TreeModelNode* parent,
                                int start,
                                int count);
    virtual void TreeNodesRemoved(ui::TreeModel* model,
                                  ui::TreeModelNode* parent,
                                  int start,
                                  int count);
    virtual void TreeNodeChanged(ui::TreeModel* model,
                                 ui::TreeModelNode* node);
    // End TreeModelObserver implementation.

    // Sets the controller, which may be null. TreeView does not take ownership
    // of the controller.
    void SetController(TreeViewController* controller)
    {
        controller_ = controller;
    }

    // Sets whether enter is processed when not editing. If true, enter will
    // expand/collapse the node. If false, enter is passed to the focus manager
    // so that an enter accelerator can be enabled. The default is false.
    //
    // NOTE: Changing this has no effect after the hwnd has been created.
    void SetProcessesEnter(bool process_enter)
    {
        process_enter_ = process_enter;
    }
    bool GetProcessedEnter()
    {
        return process_enter_;
    }

    // Sets when the ContextMenuController is notified. If true, the
    // ContextMenuController is only notified when a node is selected and the
    // mouse is over a node. The default is true.
    void SetShowContextMenuOnlyWhenNodeSelected(bool value)
    {
        show_context_menu_only_when_node_selected_ = value;
    }
    bool GetShowContextMenuOnlyWhenNodeSelected()
    {
        return show_context_menu_only_when_node_selected_;
    }

    // If true, a right click selects the node under the mouse. The default
    // is true.
    void SetSelectOnRightMouseDown(bool value)
    {
        select_on_right_mouse_down_ = value;
    }
    bool GetSelectOnRightMouseDown()
    {
        return select_on_right_mouse_down_;
    }

protected:
    // Overriden to return a location based on the selected node.
    virtual gfx::Point GetKeyboardContextMenuLocation();

    // Creates and configures the tree_view.
    virtual HWND CreateNativeControl(HWND parent_container);

    // Invoked when the native control sends a WM_NOTIFY message to its parent.
    // Handles a variety of potential TreeView messages.
    virtual LRESULT OnNotify(int w_param, LPNMHDR l_param);

    // Invoked when the native control send a WM_DESTROY message to its parent.
    virtual void OnDestroy();

    // We pay attention to key down for two reasons: to circumvent VK_ENTER from
    // toggling the expaned state when processes_enter_ is false, and to have F2
    // start editting.
    virtual bool OnKeyDown(ui::KeyboardCode virtual_key_code);

    virtual void OnContextMenu(const POINT& location);

    // Returns the TreeModelNode for |tree_item|.
    ui::TreeModelNode* GetNodeForTreeItem(HTREEITEM tree_item);

    // Returns the tree item for |node|.
    HTREEITEM GetTreeItemForNode(ui::TreeModelNode* node);

private:
    // See notes in TableView::TableViewWrapper for why this is needed.
    struct TreeViewWrapper
    {
        explicit TreeViewWrapper(TreeView* view) : tree_view(view) {}
        TreeView* tree_view;
    };

    // Internally used to track the state of nodes. NodeDetails are lazily created
    // as the user expands nodes.
    struct NodeDetails
    {
        NodeDetails(int id, ui::TreeModelNode* node) : id(id), node(node),
            tree_item(NULL), loaded_children(false) {}

        // Unique identifier for the node. This corresponds to the lParam of
        // the tree item.
        const int id;

        // The node from the model.
        ui::TreeModelNode* node;

        // From the native TreeView.
        //
        // This should be treated as const, but can't due to timing in creating the
        // entry.
        HTREEITEM tree_item;

        // Whether the children have been loaded.
        bool loaded_children;
    };

    // Cleanup all resources used by treeview.
    void Cleanup();

    // Make sure the tree view is observing the tree model.
    void AddObserverToModel();

    // Make sure the tree view is no longer observing the tree model.
    void RemoveObserverFromModel();

    // Deletes the root items from the treeview. This is used when the model
    // changes.
    void DeleteRootItems();

    // Creates the root items in the treeview from the model. This is used when
    // the model changes.
    void CreateRootItems();

    // Creates and adds an item to the treeview. parent_item identifies the
    // parent and is null for root items. after dictates where among the
    // children of parent_item the item is to be created. node is the node from
    // the model.
    void CreateItem(HTREEITEM parent_item, HTREEITEM after,
                    ui::TreeModelNode* node);

    // Removes entries from the map for item. This method will also
    // remove the items from the TreeView because the process of
    // deleting an item will send an TVN_GETDISPINFO message, consulting
    // our internal map data.
    void RecursivelyDelete(NodeDetails* node);

    // Returns the NodeDetails by node from the model.
    NodeDetails* GetNodeDetails(ui::TreeModelNode* node);

    // Returns the NodeDetails by identifier (lparam of the HTREEITEM).
    NodeDetails* GetNodeDetailsByID(int id);

    // Returns the NodeDetails by HTREEITEM.
    NodeDetails* GetNodeDetailsByTreeItem(HTREEITEM tree_item);

    // Creates the image list to use for the tree.
    HIMAGELIST CreateImageList();

    // Returns the HTREEITEM for |node|. This is intended to be called when a
    // model mutation event occur with |node| as the parent. This returns null
    // if the user has never expanded |node| or all of its parents.
    HTREEITEM GetTreeItemForNodeDuringMutation(ui::TreeModelNode* node);

    // The window function installed on the treeview.
    static LRESULT CALLBACK TreeWndProc(HWND window,
                                        UINT message,
                                        WPARAM w_param,
                                        LPARAM l_param);

    // Handle to the tree window.
    HWND tree_view_;

    // The model, may be null.
    ui::TreeModel* model_;

    // Maps from id to NodeDetails.
    std::map<int, NodeDetails*> id_to_details_map_;

    // Maps from model entry to NodeDetails.
    std::map<ui::TreeModelNode*, NodeDetails*> node_to_details_map_;

    // Whether to automatically expand children when a parent node is expanded.
    bool auto_expand_children_;

    // Whether the user can edit the items.
    bool editable_;

    // Next id to create. Any time an item is added this is incremented by one.
    int next_id_;

    // The controller.
    TreeViewController* controller_;

    // Node being edited. If null, not editing.
    ui::TreeModelNode* editing_node_;

    // Whether or not the root is shown in the tree.
    bool root_shown_;

    // Whether lines are drawn from the root to the children.
    bool lines_at_root_;

    // Whether enter should be processed by the tree when not editing.
    bool process_enter_;

    // Whether we notify context menu controller only when mouse is over node
    // and node is selected.
    bool show_context_menu_only_when_node_selected_;

    // Whether the selection is changed on right mouse down.
    bool select_on_right_mouse_down_;

    // A wrapper around 'this', used for subclassing the TreeView control.
    TreeViewWrapper wrapper_;

    // Original handler installed on the TreeView.
    WNDPROC original_handler_;

    bool drag_enabled_;

    // Has an observer been added to the model?
    bool observer_added_;

    // Did the model return a non-empty set of icons from GetIcons?
    bool has_custom_icons_;

    HIMAGELIST image_list_;

    DISALLOW_COPY_AND_ASSIGN(TreeView);
};

} //namespace view

#endif //__view_tree_view_h__