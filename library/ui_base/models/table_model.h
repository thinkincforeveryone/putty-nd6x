
#ifndef __ui_base_table_model_h__
#define __ui_base_table_model_h__

#pragma once

#include <vector>

#include "base/string16.h"

class SkBitmap;

namespace ui
{

class TableModelObserver;

// The model driving the TableView.
class TableModel
{
public:
    // See HasGroups, get GetGroupID for details as to how this is used.
    struct Group
    {
        // The title text for the group.
        string16 title;

        // Unique id for the group.
        int id;
    };
    typedef std::vector<Group> Groups;

    // Number of rows in the model.
    virtual int RowCount() = 0;

    // Returns the value at a particular location in text.
    virtual string16 GetText(int row, int column_id) = 0;

    // Returns the small icon (16x16) that should be displayed in the first
    // column before the text. This is only used when the TableView was created
    // with the ICON_AND_TEXT table type. Returns an isNull() bitmap if there is
    // no bitmap.
    virtual SkBitmap GetIcon(int row);

    // Returns the tooltip, if any, to show for a particular row.  If there are
    // multiple columns in the row, this will only be shown when hovering over
    // column zero.
    virtual string16 GetTooltip(int row);

    // If true, this row should be indented.
    virtual bool ShouldIndent(int row);

    // Returns true if the TableView has groups. Groups provide a way to visually
    // delineate the rows in a table view. When groups are enabled table view
    // shows a visual separator for each group, followed by all the rows in
    // the group.
    //
    // On win2k a visual separator is not rendered for the group headers.
    virtual bool HasGroups();

    // Returns the groups.
    // This is only used if HasGroups returns true.
    virtual Groups GetGroups();

    // Returns the group id of the specified row.
    // This is only used if HasGroups returns true.
    virtual int GetGroupID(int row);

    // Sets the observer for the model. The TableView should NOT take ownership
    // of the observer.
    virtual void SetObserver(TableModelObserver* observer) = 0;

    // Compares the values in the column with id |column_id| for the two rows.
    // Returns a value < 0, == 0 or > 0 as to whether the first value is
    // <, == or > the second value.
    //
    // This implementation does a case insensitive locale specific string
    // comparison.
    virtual int CompareValues(int row1, int row2, int column_id);

protected:
    virtual ~TableModel() {}
};

// TableColumn specifies the title, alignment and size of a particular column.
struct TableColumn
{
    enum Alignment
    {
        LEFT, RIGHT, CENTER
    };

    TableColumn();
    TableColumn(int id, const string16& title,
                Alignment alignment, int width);
    TableColumn(int id, const string16& title,
                Alignment alignment, int width, float percent);

    // It's common (but not required) to use the title's IDS_* tag as the column
    // id. In this case, the provided conveniences look up the title string on
    // bahalf of the caller.
    TableColumn(int id, Alignment alignment, int width);
    TableColumn(int id, Alignment alignment, int width, float percent);

    // A unique identifier for the column.
    int id;

    // The title for the column.
    string16 title;

    // Alignment for the content.
    Alignment alignment;

    // The size of a column may be specified in two ways:
    // 1. A fixed width. Set the width field to a positive number and the
    //    column will be given that width, in pixels.
    // 2. As a percentage of the available width. If width is -1, and percent is
    //    > 0, the column is given a width of
    //    available_width * percent / total_percent.
    // 3. If the width == -1 and percent == 0, the column is autosized based on
    //    the width of the column header text.
    //
    // Sizing is done in four passes. Fixed width columns are given
    // their width, percentages are applied, autosized columns are autosized,
    // and finally percentages are applied again taking into account the widths
    // of autosized columns.
    int width;
    float percent;

    // The minimum width required for all items in this column
    // (including the header)
    // to be visible.
    int min_visible_width;

    // Is this column sortable? Default is false
    bool sortable;
};

} //namespace ui

#endif //__ui_base_table_model_h__