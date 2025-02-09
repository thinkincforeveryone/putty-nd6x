
#ifndef __ui_base_tree_model_h__
#define __ui_base_tree_model_h__

#pragma once

#include <vector>

#include "base/string16.h"

class SkBitmap;

namespace ui
{

class TreeModel;

// TreeModelNode --------------------------------------------------------------

// Type of class returned from the model.
class TreeModelNode
{
public:
    // Returns the title for the node.
    virtual const string16& GetTitle() const = 0;

protected:
    virtual ~TreeModelNode() {}
};

// Observer for the TreeModel. Notified of significant events to the model.
class TreeModelObserver
{
public:
    // Notification that nodes were added to the specified parent.
    virtual void TreeNodesAdded(TreeModel* model,
                                TreeModelNode* parent,
                                int start,
                                int count) = 0;

    // Notification that nodes were removed from the specified parent.
    virtual void TreeNodesRemoved(TreeModel* model,
                                  TreeModelNode* parent,
                                  int start,
                                  int count) = 0;

    // Notification that the contents of a node has changed.
    virtual void TreeNodeChanged(TreeModel* model, TreeModelNode* node) = 0;

protected:
    virtual ~TreeModelObserver() {}
};

// TreeModel ------------------------------------------------------------------

// The model for TreeView.
class TreeModel
{
public:
    // Returns the root of the tree. This may or may not be shown in the tree,
    // see SetRootShown for details.
    virtual TreeModelNode* GetRoot() = 0;

    // Returns the number of children in |parent|.
    virtual int GetChildCount(TreeModelNode* parent) = 0;

    // Returns the child node of |parent| at |index|.
    virtual TreeModelNode* GetChild(TreeModelNode* parent, int index) = 0;

    // Returns the index of |child| in |parent|.
    virtual int GetIndexOf(TreeModelNode* parent, TreeModelNode* child) = 0;

    // Returns the parent of |node|, or NULL if |node| is the root.
    virtual TreeModelNode* GetParent(TreeModelNode* node) = 0;

    // Adds an observer of the model.
    virtual void AddObserver(TreeModelObserver* observer) = 0;

    // Removes an observer of the model.
    virtual void RemoveObserver(TreeModelObserver* observer) = 0;

    // Sets the title of |node|.
    // This is only invoked if the node is editable and the user edits a node.
    virtual void SetTitle(TreeModelNode* node, const string16& title);

    // Returns the set of icons for the nodes in the tree. You only need override
    // this if you don't want to use the default folder icons.
    virtual void GetIcons(std::vector<SkBitmap>* icons) {}

    // Returns the index of the icon to use for |node|. Return -1 to use the
    // default icon. The index is relative to the list of icons returned from
    // GetIcons.
    virtual int GetIconIndex(TreeModelNode* node);

protected:
    virtual ~TreeModel() {}
};

} //namespace ui

#endif //__ui_base_tree_model_h__