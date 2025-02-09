
#ifndef __tab_strip_model_order_controller_h__
#define __tab_strip_model_order_controller_h__

#pragma once

#include "tab_strip_model.h"

class TabContentsWrapper;

///////////////////////////////////////////////////////////////////////////////
// TabStripModelOrderController
//
//  An object that allows different types of ordering and reselection to be
//  heuristics plugged into a TabStripModel.
//
class TabStripModelOrderController : public TabStripModelObserver
{
public:
    explicit TabStripModelOrderController(TabStripModel* tabstrip);
    virtual ~TabStripModelOrderController();

    // Sets the insertion policy. Default is INSERT_AFTER.
    void set_insertion_policy(TabStripModel::InsertionPolicy policy)
    {
        insertion_policy_ = policy;
    }
    TabStripModel::InsertionPolicy insertion_policy() const
    {
        return insertion_policy_;
    }

    // Determine where to place a newly opened tab by using the supplied
    // transition and foreground flag to figure out how it was opened.
    int DetermineInsertionIndex(TabContentsWrapper* new_contents,
                                bool foreground);

    // Returns the index to append tabs at.
    int DetermineInsertionIndexForAppending();

    // Determine where to shift selection after a tab is closed.
    int DetermineNewSelectedIndex(int removed_index) const;

    // Overridden from TabStripModelObserver:
    virtual void ActiveTabChanged(TabContentsWrapper* old_contents,
                                  TabContentsWrapper* new_contents,
                                  int index,
                                  bool user_gesture);

private:
    // Returns a valid index to be selected after the tab at |removing_index| is
    // closed. If |index| is after |removing_index|, |index| is adjusted to
    // reflect the fact that |removing_index| is going away.
    int GetValidIndex(int index, int removing_index) const;

    TabStripModel* tabstrip_;

    TabStripModel::InsertionPolicy insertion_policy_;

    DISALLOW_COPY_AND_ASSIGN(TabStripModelOrderController);
};

#endif //__tab_strip_model_order_controller_h__