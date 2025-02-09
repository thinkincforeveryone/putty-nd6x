#include <algorithm>
#include "range.h"

#include <limits>
#include <ostream>

#include "base/logging.h"

namespace ui
{

Range::Range() : start_(0), end_(0) {}

Range::Range(size_t start, size_t end) : start_(start), end_(end) {}

Range::Range(size_t position) : start_(position), end_(position) {}

// static
const Range Range::InvalidRange()
{
    return Range(std::numeric_limits<size_t>::max());
}

bool Range::IsValid() const
{
    return *this != InvalidRange();
}

size_t Range::GetMin() const
{
    return std::min(start(), end());
}

size_t Range::GetMax() const
{
    return std::max(start(), end());
}

bool Range::operator==(const Range& other) const
{
    return start()==other.start() && end()==other.end();
}

bool Range::operator!=(const Range& other) const
{
    return !(*this == other);
}

bool Range::EqualsIgnoringDirection(const Range& other) const
{
    return GetMin()==other.GetMin() && GetMax()==other.GetMax();
}

bool Range::Intersects(const Range& range) const
{
    return IsValid() && range.IsValid() &&
           !(range.GetMax()<GetMin() || range.GetMin()>=GetMax());
}

bool Range::Contains(const Range& range) const
{
    return IsValid() && range.IsValid() &&
           GetMin()<=range.GetMin() && range.GetMax()<=GetMax();
}

Range Range::Intersect(const Range& range) const
{
    size_t min = std::max(GetMin(), range.GetMin());
    size_t max = std::min(GetMax(), range.GetMax());

    if(min >= max) // No intersection.
    {
        return InvalidRange();
    }

    return Range(min, max);
}

Range::Range(const CHARRANGE& range, LONG total_length)
{
    // Check if this is an invalid range.
    if(range.cpMin==-1 && range.cpMax==-1)
    {
        *this = InvalidRange();
    }
    else
    {
        DCHECK_GE(range.cpMin, 0);
        set_start(range.cpMin);
        // {0,-1} is the "whole range" but that doesn't mean much out of context,
        // so use the |total_length| parameter.
        if(range.cpMax == -1)
        {
            DCHECK_EQ(0, range.cpMin);
            DCHECK_NE(-1, total_length);
            set_end(total_length);
        }
        else
        {
            set_end(range.cpMax);
        }
    }
}

CHARRANGE Range::ToCHARRANGE() const
{
    CHARRANGE r = { -1, -1 };
    if(!IsValid())
    {
        return r;
    }

    const LONG kLONGMax = std::numeric_limits<LONG>::max();
    DCHECK_LE(static_cast<LONG>(start()), kLONGMax);
    DCHECK_LE(static_cast<LONG>(end()), kLONGMax);
    r.cpMin = start();
    r.cpMax = end();
    return r;
}

std::ostream& operator<<(std::ostream& out, const Range& range)
{
    return out << "{" << range.start() << "," << range.end() << "}";
}

} //namespace ui