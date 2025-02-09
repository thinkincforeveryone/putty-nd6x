
#include "table_model.h"

#include "base/logging.h"

#include "SkBitmap.h"

#include "ui_base/l10n/l10n_util.h"

namespace ui
{

// TableColumn -----------------------------------------------------------------

TableColumn::TableColumn()
    : id(0),
      title(),
      alignment(LEFT),
      width(-1),
      percent(),
      min_visible_width(0),
      sortable(false) {}

TableColumn::TableColumn(int id, const string16& title,
                         Alignment alignment,
                         int width)
    : id(id),
      title(title),
      alignment(alignment),
      width(width),
      percent(0),
      min_visible_width(0),
      sortable(false) {}

TableColumn::TableColumn(int id, const string16& title,
                         Alignment alignment, int width, float percent)
    : id(id),
      title(title),
      alignment(alignment),
      width(width),
      percent(percent),
      min_visible_width(0),
      sortable(false) {}

// It's common (but not required) to use the title's IDS_* tag as the column
// id. In this case, the provided conveniences look up the title string on
// bahalf of the caller.
TableColumn::TableColumn(int id, Alignment alignment, int width)
    : id(id),
      alignment(alignment),
      width(width),
      percent(0),
      min_visible_width(0),
      sortable(false)
{
    title = ui::GetStringUTF16(id);
}

TableColumn::TableColumn(int id, Alignment alignment,
                         int width, float percent)
    : id(id),
      alignment(alignment),
      width(width),
      percent(percent),
      min_visible_width(0),
      sortable(false)
{
    title = ui::GetStringUTF16(id);
}

// TableModel -----------------------------------------------------------------

SkBitmap TableModel::GetIcon(int row)
{
    return SkBitmap();
}

string16 TableModel::GetTooltip(int row)
{
    return string16();
}

bool TableModel::ShouldIndent(int row)
{
    return false;
}

bool TableModel::HasGroups()
{
    return false;
}

TableModel::Groups TableModel::GetGroups()
{
    // If you override HasGroups to return true, you must override this as
    // well.
    NOTREACHED();
    return std::vector<Group>();
}

int TableModel::GetGroupID(int row)
{
    // If you override HasGroups to return true, you must override this as
    // well.
    NOTREACHED();
    return 0;
}

int TableModel::CompareValues(int row1, int row2, int column_id)
{
    DCHECK(row1>=0 && row1<RowCount() &&
           row2>=0 && row2<RowCount());
    string16 value1 = GetText(row1, column_id);
    string16 value2 = GetText(row2, column_id);

    return lstrcmpW(value1.c_str(), value2.c_str());
}

} //namespace ui