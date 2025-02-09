
#include "tween.h"

#include <float.h>
#include <math.h>

#include "base/logging.h"

#include "ui_gfx/rect.h"

namespace ui
{

// static
double Tween::CalculateValue(Tween::Type type, double state)
{
    DCHECK_GE(state, 0);
    DCHECK_LE(state, 1);

    switch(type)
    {
    case EASE_IN:
        return pow(state, 2);

    case EASE_IN_OUT:
        if(state < 0.5)
        {
            return pow(state * 2, 2) / 2.0;
        }
        return 1.0 - (pow((state - 1.0) * 2, 2) / 2.0);

    case FAST_IN_OUT:
        return (pow(state - 0.5, 3) + 0.125) / 0.25;

    case LINEAR:
        return state;

    case EASE_OUT_SNAP:
        state = 0.95 * (1.0 - pow(1.0 - state, 2));
        break;

    case EASE_OUT:
        return 1.0 - pow(1.0 - state, 2);

    case ZERO:
        return 0;
    }

    NOTREACHED();
    return state;
}

// static
double Tween::ValueBetween(double value, double start, double target)
{
    return start + (target - start) * value;
}

// static
int Tween::ValueBetween(double value, int start, int target)
{
    if(start == target)
    {
        return start;
    }
    double delta = static_cast<double>(target - start);
    if(delta < 0)
    {
        delta--;
    }
    else
    {
        delta++;
    }
    return start + static_cast<int>(value * _nextafter(delta, 0));
}

// static
gfx::Rect Tween::ValueBetween(double value, const gfx::Rect& start_bounds,
                              const gfx::Rect& target_bounds)
{
    return gfx::Rect(ValueBetween(value, start_bounds.x(), target_bounds.x()),
                     ValueBetween(value, start_bounds.y(), target_bounds.y()),
                     ValueBetween(value, start_bounds.width(),
                                  target_bounds.width()),
                     ValueBetween(value, start_bounds.height(),
                                  target_bounds.height()));
}

} //namespace ui