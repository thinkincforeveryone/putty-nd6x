/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SkCubicClipper.h"
#include "SkGeometry.h"

SkCubicClipper::SkCubicClipper() {}

void SkCubicClipper::setClip(const SkIRect& clip)
{
    // conver to scalars, since that's where we'll see the points
    fClip.set(clip);
}


static bool chopMonoCubicAtY(SkPoint pts[4], SkScalar y, SkScalar* t)
{
    SkScalar ycrv[4];
    ycrv[0] = pts[0].fY - y;
    ycrv[1] = pts[1].fY - y;
    ycrv[2] = pts[2].fY - y;
    ycrv[3] = pts[3].fY - y;

#ifdef NEWTON_RAPHSON    // Quadratic convergence, typically <= 3 iterations.
    // Initial guess.
    // TODO(turk): Check for zero denominator? Shouldn't happen unless the curve
    // is not only monotonic but degenerate.
#ifdef SK_SCALAR_IS_FLOAT
    SkScalar t1 = ycrv[0] / (ycrv[0] - ycrv[3]);
#else  // !SK_SCALAR_IS_FLOAT
    SkScalar t1 = SkDivBits(ycrv[0], ycrv[0] - ycrv[3], 16);
#endif  // !SK_SCALAR_IS_FLOAT

    // Newton's iterations.
    const SkScalar tol = SK_Scalar1 / 16384;  // This leaves 2 fixed noise bits.
    SkScalar t0;
    const int maxiters = 5;
    int iters = 0;
    bool converged;
    do
    {
        t0 = t1;
        SkScalar y01   = SkScalarInterp(ycrv[0], ycrv[1], t0);
        SkScalar y12   = SkScalarInterp(ycrv[1], ycrv[2], t0);
        SkScalar y23   = SkScalarInterp(ycrv[2], ycrv[3], t0);
        SkScalar y012  = SkScalarInterp(y01,  y12,  t0);
        SkScalar y123  = SkScalarInterp(y12,  y23,  t0);
        SkScalar y0123 = SkScalarInterp(y012, y123, t0);
        SkScalar yder  = (y123 - y012) * 3;
        // TODO(turk): check for yder==0: horizontal.
#ifdef SK_SCALAR_IS_FLOAT
        t1 -= y0123 / yder;
#else  // !SK_SCALAR_IS_FLOAT
        t1 -= SkDivBits(y0123, yder, 16);
#endif  // !SK_SCALAR_IS_FLOAT
        converged = SkScalarAbs(t1 - t0) <= tol;  // NaN-safe
        ++iters;
    }
    while (!converged && (iters < maxiters));
    *t = t1;                  // Return the result.

    // The result might be valid, even if outside of the range [0, 1], but
    // we never evaluate a Bezier outside this interval, so we return false.
    if (t1 < 0 || t1 > SK_Scalar1)
        return false;         // This shouldn't happen, but check anyway.
    return converged;

#else  // BISECTION    // Linear convergence, typically 16 iterations.

    // Check that the endpoints straddle zero.
    SkScalar tNeg, tPos;    // Negative and positive function parameters.
    if (ycrv[0] < 0)
    {
        if (ycrv[3] < 0)
            return false;
        tNeg = 0;
        tPos = SK_Scalar1;
    }
    else if (ycrv[0] > 0)
    {
        if (ycrv[3] > 0)
            return false;
        tNeg = SK_Scalar1;
        tPos = 0;
    }
    else
    {
        *t = 0;
        return true;
    }

    const SkScalar tol = SK_Scalar1 / 65536;  // 1 for fixed, 1e-5 for float.
    int iters = 0;
    do
    {
        SkScalar tMid = (tPos + tNeg) / 2;
        SkScalar y01   = SkScalarInterp(ycrv[0], ycrv[1], tMid);
        SkScalar y12   = SkScalarInterp(ycrv[1], ycrv[2], tMid);
        SkScalar y23   = SkScalarInterp(ycrv[2], ycrv[3], tMid);
        SkScalar y012  = SkScalarInterp(y01,     y12,     tMid);
        SkScalar y123  = SkScalarInterp(y12,     y23,     tMid);
        SkScalar y0123 = SkScalarInterp(y012,    y123,    tMid);
        if (y0123 == 0)
        {
            *t = tMid;
            return true;
        }
        if (y0123 < 0)  tNeg = tMid;
        else            tPos = tMid;
        ++iters;
    }
    while (!(SkScalarAbs(tPos - tNeg) <= tol));     // Nan-safe

    *t = (tNeg + tPos) / 2;
    return true;
#endif  // BISECTION
}


bool SkCubicClipper::clipCubic(const SkPoint srcPts[4], SkPoint dst[4])
{
    bool reverse;

    // we need the data to be monotonically descending in Y
    if (srcPts[0].fY > srcPts[3].fY)
    {
        dst[0] = srcPts[3];
        dst[1] = srcPts[2];
        dst[2] = srcPts[1];
        dst[3] = srcPts[0];
        reverse = true;
    }
    else
    {
        memcpy(dst, srcPts, 4 * sizeof(SkPoint));
        reverse = false;
    }

    // are we completely above or below
    const SkScalar ctop = fClip.fTop;
    const SkScalar cbot = fClip.fBottom;
    if (dst[3].fY <= ctop || dst[0].fY >= cbot)
    {
        return false;
    }

    SkScalar t;
    SkPoint tmp[7]; // for SkChopCubicAt

    // are we partially above
    if (dst[0].fY < ctop && chopMonoCubicAtY(dst, ctop, &t))
    {
        SkChopCubicAt(dst, tmp, t);
        dst[0] = tmp[3];
        dst[1] = tmp[4];
        dst[2] = tmp[5];
    }

    // are we partially below
    if (dst[3].fY > cbot && chopMonoCubicAtY(dst, cbot, &t))
    {
        SkChopCubicAt(dst, tmp, t);
        dst[1] = tmp[1];
        dst[2] = tmp[2];
        dst[3] = tmp[3];
    }

    if (reverse)
    {
        SkTSwap<SkPoint>(dst[0], dst[3]);
        SkTSwap<SkPoint>(dst[1], dst[2]);
    }
    return true;
}

