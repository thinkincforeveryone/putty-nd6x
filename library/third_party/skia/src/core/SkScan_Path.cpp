/* libs/graphics/sgl/SkScan_Path.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "SkScanPriv.h"
#include "SkBlitter.h"
#include "SkEdge.h"
#include "SkEdgeBuilder.h"
#include "SkGeometry.h"
#include "SkPath.h"
#include "SkQuadClipper.h"
#include "SkRegion.h"
#include "SkTemplates.h"

#define kEDGE_HEAD_Y    SK_MinS32
#define kEDGE_TAIL_Y    SK_MaxS32

#ifdef SK_DEBUG
static void validate_sort(const SkEdge* edge)
{
    int y = kEDGE_HEAD_Y;

    while (edge->fFirstY != SK_MaxS32)
    {
        edge->validate();
        SkASSERT(y <= edge->fFirstY);

        y = edge->fFirstY;
        edge = edge->fNext;
    }
}
#else
#define validate_sort(edge)
#endif

static inline void remove_edge(SkEdge* edge)
{
    edge->fPrev->fNext = edge->fNext;
    edge->fNext->fPrev = edge->fPrev;
}

static inline void swap_edges(SkEdge* prev, SkEdge* next)
{
    SkASSERT(prev->fNext == next && next->fPrev == prev);

    // remove prev from the list
    prev->fPrev->fNext = next;
    next->fPrev = prev->fPrev;

    // insert prev after next
    prev->fNext = next->fNext;
    next->fNext->fPrev = prev;
    next->fNext = prev;
    prev->fPrev = next;
}

static void backward_insert_edge_based_on_x(SkEdge* edge SkDECLAREPARAM(int, curr_y))
{
    SkFixed x = edge->fX;

    for (;;)
    {
        SkEdge* prev = edge->fPrev;

        // add 1 to curr_y since we may have added new edges (built from curves)
        // that start on the next scanline
        SkASSERT(prev && prev->fFirstY <= curr_y + 1);

        if (prev->fX <= x)
        {
            break;
        }
        swap_edges(prev, edge);
    }
}

static void insert_new_edges(SkEdge* newEdge, int curr_y)
{
    SkASSERT(newEdge->fFirstY >= curr_y);

    while (newEdge->fFirstY == curr_y)
    {
        SkEdge* next = newEdge->fNext;
        backward_insert_edge_based_on_x(newEdge  SkPARAM(curr_y));
        newEdge = next;
    }
}

#ifdef SK_DEBUG
static void validate_edges_for_y(const SkEdge* edge, int curr_y)
{
    while (edge->fFirstY <= curr_y)
    {
        SkASSERT(edge->fPrev && edge->fNext);
        SkASSERT(edge->fPrev->fNext == edge);
        SkASSERT(edge->fNext->fPrev == edge);
        SkASSERT(edge->fFirstY <= edge->fLastY);

        SkASSERT(edge->fPrev->fX <= edge->fX);
        edge = edge->fNext;
    }
}
#else
#define validate_edges_for_y(edge, curr_y)
#endif

#if defined _WIN32 && _MSC_VER >= 1300  // disable warning : local variable used without having been initialized
#pragma warning ( push )
#pragma warning ( disable : 4701 )
#endif

typedef void (*PrePostProc)(SkBlitter* blitter, int y, bool isStartOfScanline);
#define PREPOST_START   true
#define PREPOST_END     false

static void walk_edges(SkEdge* prevHead, SkPath::FillType fillType,
                       SkBlitter* blitter, int start_y, int stop_y,
                       PrePostProc proc)
{
    validate_sort(prevHead->fNext);

    int curr_y = start_y;
    // returns 1 for evenodd, -1 for winding, regardless of inverse-ness
    int windingMask = (fillType & 1) ? 1 : -1;

    for (;;)
    {
        int     w = 0;
        int     left SK_INIT_TO_AVOID_WARNING;
        bool    in_interval = false;
        SkEdge* currE = prevHead->fNext;
        SkFixed prevX = prevHead->fX;

        validate_edges_for_y(currE, curr_y);

        if (proc)
        {
            proc(blitter, curr_y, PREPOST_START);    // pre-proc
        }

        while (currE->fFirstY <= curr_y)
        {
            SkASSERT(currE->fLastY >= curr_y);

            int x = (currE->fX + SK_Fixed1/2) >> 16;
            w += currE->fWinding;
            if ((w & windingMask) == 0)   // we finished an interval
            {
                SkASSERT(in_interval);
                int width = x - left;
                SkASSERT(width >= 0);
                if (width)
                    blitter->blitH(left, curr_y, width);
                in_interval = false;
            }
            else if (!in_interval)
            {
                left = x;
                in_interval = true;
            }

            SkEdge* next = currE->fNext;
            SkFixed newX;

            if (currE->fLastY == curr_y)      // are we done with this edge?
            {
                if (currE->fCurveCount < 0)
                {
                    if (((SkCubicEdge*)currE)->updateCubic())
                    {
                        SkASSERT(currE->fFirstY == curr_y + 1);

                        newX = currE->fX;
                        goto NEXT_X;
                    }
                }
                else if (currE->fCurveCount > 0)
                {
                    if (((SkQuadraticEdge*)currE)->updateQuadratic())
                    {
                        newX = currE->fX;
                        goto NEXT_X;
                    }
                }
                remove_edge(currE);
            }
            else
            {
                SkASSERT(currE->fLastY > curr_y);
                newX = currE->fX + currE->fDX;
                currE->fX = newX;
NEXT_X:
                if (newX < prevX)   // ripple currE backwards until it is x-sorted
                {
                    backward_insert_edge_based_on_x(currE  SkPARAM(curr_y));
                }
                else
                {
                    prevX = newX;
                }
            }
            currE = next;
            SkASSERT(currE);
        }

        if (proc)
        {
            proc(blitter, curr_y, PREPOST_END);    // post-proc
        }

        curr_y += 1;
        if (curr_y >= stop_y)
        {
            break;
        }
        // now currE points to the first edge with a Yint larger than curr_y
        insert_new_edges(currE, curr_y);
    }
}

///////////////////////////////////////////////////////////////////////////////

// this guy overrides blitH, and will call its proxy blitter with the inverse
// of the spans it is given (clipped to the left/right of the cliprect)
//
// used to implement inverse filltypes on paths
//
class InverseBlitter : public SkBlitter
{
public:
    void setBlitter(SkBlitter* blitter, const SkIRect& clip, int shift)
    {
        fBlitter = blitter;
        fFirstX = clip.fLeft << shift;
        fLastX = clip.fRight << shift;
    }
    void prepost(int y, bool isStart)
    {
        if (isStart)
        {
            fPrevX = fFirstX;
        }
        else
        {
            int invWidth = fLastX - fPrevX;
            if (invWidth > 0)
            {
                fBlitter->blitH(fPrevX, y, invWidth);
            }
        }
    }

    // overrides
    virtual void blitH(int x, int y, int width)
    {
        int invWidth = x - fPrevX;
        if (invWidth > 0)
        {
            fBlitter->blitH(fPrevX, y, invWidth);
        }
        fPrevX = x + width;
    }

    // we do not expect to get called with these entrypoints
    virtual void blitAntiH(int, int, const SkAlpha[], const int16_t runs[])
    {
        SkASSERT(!"blitAntiH unexpected");
    }
    virtual void blitV(int x, int y, int height, SkAlpha alpha)
    {
        SkASSERT(!"blitV unexpected");
    }
    virtual void blitRect(int x, int y, int width, int height)
    {
        SkASSERT(!"blitRect unexpected");
    }
    virtual void blitMask(const SkMask&, const SkIRect& clip)
    {
        SkASSERT(!"blitMask unexpected");
    }
    virtual const SkBitmap* justAnOpaqueColor(uint32_t* value)
    {
        SkASSERT(!"justAnOpaqueColor unexpected");
        return NULL;
    }

private:
    SkBlitter*  fBlitter;
    int         fFirstX, fLastX, fPrevX;
};

static void PrePostInverseBlitterProc(SkBlitter* blitter, int y, bool isStart)
{
    ((InverseBlitter*)blitter)->prepost(y, isStart);
}

///////////////////////////////////////////////////////////////////////////////

#if defined _WIN32 && _MSC_VER >= 1300
#pragma warning ( pop )
#endif

extern "C" {
    static int edge_compare(const void* a, const void* b)
    {
        const SkEdge* edgea = *(const SkEdge**)a;
        const SkEdge* edgeb = *(const SkEdge**)b;

        int valuea = edgea->fFirstY;
        int valueb = edgeb->fFirstY;

        if (valuea == valueb)
        {
            valuea = edgea->fX;
            valueb = edgeb->fX;
        }

        // this overflows if valuea >>> valueb or vice-versa
        //     return valuea - valueb;
        // do perform the slower but safe compares
        return (valuea < valueb) ? -1 : (valuea > valueb);
    }
}

static SkEdge* sort_edges(SkEdge* list[], int count, SkEdge** last)
{
    qsort(list, count, sizeof(SkEdge*), edge_compare);

    // now make the edges linked in sorted order
    for (int i = 1; i < count; i++)
    {
        list[i - 1]->fNext = list[i];
        list[i]->fPrev = list[i - 1];
    }

    *last = list[count - 1];
    return list[0];
}

// clipRect may be null, even though we always have a clip. This indicates that
// the path is contained in the clip, and so we can ignore it during the blit
//
// clipRect (if no null) has already been shifted up
//
void sk_fill_path(const SkPath& path, const SkIRect* clipRect, SkBlitter* blitter,
                  int start_y, int stop_y, int shiftEdgesUp,
                  const SkRegion& clipRgn)
{
    SkASSERT(&path && blitter);

    SkEdgeBuilder   builder;

    int count = builder.build(path, clipRect, shiftEdgesUp);
    SkEdge**    list = builder.edgeList();

    if (count < 2)
    {
        if (path.isInverseFillType())
        {
            const SkIRect& clipRect = clipRgn.getBounds();
            blitter->blitRect(clipRect.fLeft << shiftEdgesUp,
                              clipRect.fTop << shiftEdgesUp,
                              clipRect.width() << shiftEdgesUp,
                              clipRect.height() << shiftEdgesUp);
        }

        return;
    }

    SkEdge headEdge, tailEdge, *last;
    // this returns the first and last edge after they're sorted into a dlink list
    SkEdge* edge = sort_edges(list, count, &last);

    headEdge.fPrev = NULL;
    headEdge.fNext = edge;
    headEdge.fFirstY = kEDGE_HEAD_Y;
    headEdge.fX = SK_MinS32;
    edge->fPrev = &headEdge;

    tailEdge.fPrev = last;
    tailEdge.fNext = NULL;
    tailEdge.fFirstY = kEDGE_TAIL_Y;
    last->fNext = &tailEdge;

    // now edge is the head of the sorted linklist

    start_y <<= shiftEdgesUp;
    stop_y <<= shiftEdgesUp;
    if (clipRect && start_y < clipRect->fTop)
    {
        start_y = clipRect->fTop;
    }
    if (clipRect && stop_y > clipRect->fBottom)
    {
        stop_y = clipRect->fBottom;
    }

    InverseBlitter  ib;
    PrePostProc     proc = NULL;

    if (path.isInverseFillType())
    {
        ib.setBlitter(blitter, clipRgn.getBounds(), shiftEdgesUp);
        blitter = &ib;
        proc = PrePostInverseBlitterProc;
    }

    walk_edges(&headEdge, path.getFillType(), blitter, start_y, stop_y, proc);
}

void sk_blit_above(SkBlitter* blitter, const SkIRect& ir, const SkRegion& clip)
{
    const SkIRect& cr = clip.getBounds();
    SkIRect tmp;

    tmp.fLeft = cr.fLeft;
    tmp.fRight = cr.fRight;
    tmp.fTop = cr.fTop;
    tmp.fBottom = ir.fTop;
    if (!tmp.isEmpty())
    {
        blitter->blitRectRegion(tmp, clip);
    }
}

void sk_blit_below(SkBlitter* blitter, const SkIRect& ir, const SkRegion& clip)
{
    const SkIRect& cr = clip.getBounds();
    SkIRect tmp;

    tmp.fLeft = cr.fLeft;
    tmp.fRight = cr.fRight;
    tmp.fTop = ir.fBottom;
    tmp.fBottom = cr.fBottom;
    if (!tmp.isEmpty())
    {
        blitter->blitRectRegion(tmp, clip);
    }
}

///////////////////////////////////////////////////////////////////////////////

SkScanClipper::SkScanClipper(SkBlitter* blitter, const SkRegion* clip,
                             const SkIRect& ir)
{
    fBlitter = NULL;     // null means blit nothing
    fClipRect = NULL;

    if (clip)
    {
        fClipRect = &clip->getBounds();
        if (!SkIRect::Intersects(*fClipRect, ir))   // completely clipped out
        {
            return;
        }

        if (clip->isRect())
        {
            if (fClipRect->contains(ir))
            {
                fClipRect = NULL;
            }
            else
            {
                // only need a wrapper blitter if we're horizontally clipped
                if (fClipRect->fLeft > ir.fLeft || fClipRect->fRight < ir.fRight)
                {
                    fRectBlitter.init(blitter, *fClipRect);
                    blitter = &fRectBlitter;
                }
            }
        }
        else
        {
            fRgnBlitter.init(blitter, clip);
            blitter = &fRgnBlitter;
        }
    }
    fBlitter = blitter;
}

///////////////////////////////////////////////////////////////////////////////

static bool clip_to_limit(const SkRegion& orig, SkRegion* reduced)
{
    const int32_t limit = 32767;

    SkIRect limitR;
    limitR.set(-limit, -limit, limit, limit);
    if (limitR.contains(orig.getBounds()))
    {
        return false;
    }
    reduced->op(orig, limitR, SkRegion::kIntersect_Op);
    return true;
}

void SkScan::FillPath(const SkPath& path, const SkRegion& origClip,
                      SkBlitter* blitter)
{
    if (origClip.isEmpty())
    {
        return;
    }

    // Our edges are fixed-point, and don't like the bounds of the clip to
    // exceed that. Here we trim the clip just so we don't overflow later on
    const SkRegion* clipPtr = &origClip;
    SkRegion finiteClip;
    if (clip_to_limit(origClip, &finiteClip))
    {
        if (finiteClip.isEmpty())
        {
            return;
        }
        clipPtr = &finiteClip;
    }
    // don't reference "origClip" any more, just use clipPtr

    SkIRect ir;
    path.getBounds().round(&ir);
    if (ir.isEmpty())
    {
        if (path.isInverseFillType())
        {
            blitter->blitRegion(*clipPtr);
        }
        return;
    }

    SkScanClipper   clipper(blitter, clipPtr, ir);

    blitter = clipper.getBlitter();
    if (blitter)
    {
        // we have to keep our calls to blitter in sorted order, so we
        // must blit the above section first, then the middle, then the bottom.
        if (path.isInverseFillType())
        {
            sk_blit_above(blitter, ir, *clipPtr);
        }
        sk_fill_path(path, clipper.getClipRect(), blitter, ir.fTop, ir.fBottom,
                     0, *clipPtr);
        if (path.isInverseFillType())
        {
            sk_blit_below(blitter, ir, *clipPtr);
        }
    }
    else
    {
        // what does it mean to not have a blitter if path.isInverseFillType???
    }
}

///////////////////////////////////////////////////////////////////////////////

static int build_tri_edges(SkEdge edge[], const SkPoint pts[],
                           const SkIRect* clipRect, SkEdge* list[])
{
    SkEdge** start = list;

    if (edge->setLine(pts[0], pts[1], clipRect, 0))
    {
        *list++ = edge;
        edge = (SkEdge*)((char*)edge + sizeof(SkEdge));
    }
    if (edge->setLine(pts[1], pts[2], clipRect, 0))
    {
        *list++ = edge;
        edge = (SkEdge*)((char*)edge + sizeof(SkEdge));
    }
    if (edge->setLine(pts[2], pts[0], clipRect, 0))
    {
        *list++ = edge;
    }
    return (int)(list - start);
}


static void sk_fill_triangle(const SkPoint pts[], const SkIRect* clipRect,
                             SkBlitter* blitter, const SkIRect& ir)
{
    SkASSERT(pts && blitter);

    SkEdge edgeStorage[3];
    SkEdge* list[3];

    int count = build_tri_edges(edgeStorage, pts, clipRect, list);
    if (count < 2)
    {
        return;
    }

    SkEdge headEdge, tailEdge, *last;

    // this returns the first and last edge after they're sorted into a dlink list
    SkEdge* edge = sort_edges(list, count, &last);

    headEdge.fPrev = NULL;
    headEdge.fNext = edge;
    headEdge.fFirstY = kEDGE_HEAD_Y;
    headEdge.fX = SK_MinS32;
    edge->fPrev = &headEdge;

    tailEdge.fPrev = last;
    tailEdge.fNext = NULL;
    tailEdge.fFirstY = kEDGE_TAIL_Y;
    last->fNext = &tailEdge;

    // now edge is the head of the sorted linklist
    int stop_y = ir.fBottom;
    if (clipRect && stop_y > clipRect->fBottom)
    {
        stop_y = clipRect->fBottom;
    }
    int start_y = ir.fTop;
    if (clipRect && start_y < clipRect->fTop)
    {
        start_y = clipRect->fTop;
    }
    walk_edges(&headEdge, SkPath::kEvenOdd_FillType, blitter, start_y, stop_y, NULL);
}

void SkScan::FillTriangle(const SkPoint pts[], const SkRegion* clip,
                          SkBlitter* blitter)
{
    if (clip && clip->isEmpty())
    {
        return;
    }

    SkRect  r;
    SkIRect ir;
    r.set(pts, 3);
    r.round(&ir);
    if (ir.isEmpty())
    {
        return;
    }

    SkScanClipper   clipper(blitter, clip, ir);

    blitter = clipper.getBlitter();
    if (NULL != blitter)
    {
        sk_fill_triangle(pts, clipper.getClipRect(), blitter, ir);
    }
}

