
#include "tab_strip_model_observer.h"

void TabStripModelObserver::TabInsertedAt(TabContentsWrapper* contents,
        int index,
        bool foreground) {}

void TabStripModelObserver::TabClosingAt(TabStripModel* tab_strip_model,
        TabContentsWrapper* contents,
        int index) {}

void TabStripModelObserver::TabDetachedAt(TabContentsWrapper* contents,
        int index) {}

void TabStripModelObserver::TabDeactivated(TabContentsWrapper* contents) {}

void TabStripModelObserver::ActiveTabChanged(TabContentsWrapper* old_contents,
        TabContentsWrapper* new_contents,
        int index,
        bool user_gesture) {}

void TabStripModelObserver::TabSelectionChanged(
    const TabStripSelectionModel& model) {}

void TabStripModelObserver::TabMoved(TabContentsWrapper* contents,
                                     int from_index,
                                     int to_index) {}

void TabStripModelObserver::TabChangedAt(TabContentsWrapper* contents,
        int index,
        TabChangeType change_type) {}

void TabStripModelObserver::TabReplacedAt(TabStripModel* tab_strip_model,
        TabContentsWrapper* old_contents,
        TabContentsWrapper* new_contents,
        int index) {}

void TabStripModelObserver::TabMiniStateChanged(TabContentsWrapper* contents,
        int index) {}

void TabStripModelObserver::TabBlockedStateChanged(TabContentsWrapper* contents,
        int index) {}

void TabStripModelObserver::TabStripEmpty() {}

void TabStripModelObserver::TabStripModelDeleted() {}

void TabStripModelObserver::ActiveTabClicked(int index) {}