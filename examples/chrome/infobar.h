
#ifndef __infobar_h__
#define __infobar_h__

#pragma once

#include <utility>

#include "base/basic_types.h"
#include "base/memory/scoped_ptr.h"

#include "SkColor.h"

#include "ui_gfx/size.h"

#include "ui_base/animation/animation_delegate.h"
#include "ui_base/animation/slide_animation.h"

#include "infobar_delegate.h"

// TODO(sail): These functions should be static methods in the InfoBar class
// below once all platforms use that class.
SkColor GetInfoBarTopColor(InfoBarDelegate::Type infobar_type);
SkColor GetInfoBarBottomColor(InfoBarDelegate::Type infobar_type);

typedef std::pair<InfoBarDelegate*, bool> InfoBarRemovedDetails;
typedef std::pair<InfoBarDelegate*, InfoBarDelegate*> InfoBarReplacedDetails;

class InfoBarContainer;
class TabContentsWrapper;

class InfoBar : public ui::AnimationDelegate
{
public:
    InfoBar(TabContentsWrapper* owner, InfoBarDelegate* delegate);
    virtual ~InfoBar();

    // Platforms must define these.
    static const int kDefaultBarTargetHeight;
    static const int kSeparatorLineHeight;
    static const int kDefaultArrowTargetHeight;
    static const int kMaximumArrowTargetHeight;
    // The half-width (see comments on |arrow_half_width_| below) scales to its
    // default and maximum values proportionally to how the height scales to its.
    static const int kDefaultArrowTargetHalfWidth;
    static const int kMaximumArrowTargetHalfWidth;

    InfoBarDelegate* delegate()
    {
        return delegate_;
    }
    void set_container(InfoBarContainer* container)
    {
        container_ = container;
    }

    // Makes the infobar visible.  If |animate| is true, the infobar is then
    // animated to full size.
    void Show(bool animate);

    // Makes the infobar hidden.  If |animate| is true, the infobar is first
    // animated to zero size.  Once the infobar is hidden, it is removed from its
    // container (triggering its deletion), and its delegate is closed.
    void Hide(bool animate);

    // Changes the target height of the arrow portion of the infobar.  This has no
    // effect once the infobar is animating closed.
    void SetArrowTargetHeight(int height);

    // Notifies the infobar that it is no longer owned and should close its
    // delegate once it is invisible.
    void CloseSoon();

    const ui::SlideAnimation& animation() const
    {
        return animation_;
    }
    int arrow_height() const
    {
        return arrow_height_;
    }
    int arrow_target_height() const
    {
        return arrow_target_height_;
    }
    int arrow_half_width() const
    {
        return arrow_half_width_;
    }
    int total_height() const
    {
        return arrow_height_ + bar_height_;
    }

protected:
    // ui::AnimationDelegate:
    virtual void AnimationProgressed(const ui::Animation* animation);

    // Forwards a close request to our owner.
    void RemoveSelf();

    // Changes the target height of the main ("bar") portion of the infobar.
    void SetBarTargetHeight(int height);

    // Given a control with size |prefsize|, returns the centered y position
    // within us, taking into account animation so the control "slides in" (or
    // out) as we animate open and closed.
    int OffsetY(const gfx::Size& prefsize) const;

    bool owned() const
    {
        return !!owner_;
    }
    const InfoBarContainer* container() const
    {
        return container_;
    }
    InfoBarContainer* container()
    {
        return container_;
    }
    ui::SlideAnimation* animation()
    {
        return &animation_;
    }
    int bar_height() const
    {
        return bar_height_;
    }
    int bar_target_height() const
    {
        return bar_target_height_;
    }

    // Platforms may optionally override these if they need to do work during
    // processing of the given calls.
    virtual void PlatformSpecificShow(bool animate) {}
    virtual void PlatformSpecificHide(bool animate) {}
    virtual void PlatformSpecificOnHeightsRecalculated() {}

private:
    // ui::AnimationDelegate:
    virtual void AnimationEnded(const ui::Animation* animation) OVERRIDE;

    // Finds the new desired arrow and bar heights, and if they differ from the
    // current ones, calls PlatformSpecificOnHeightRecalculated().  Informs our
    // container our state has changed if either the heights have changed or
    // |force_notify| is set.
    void RecalculateHeights(bool force_notify);

    // Checks whether we're closed.  If so, notifies the container that it should
    // remove us (which will cause the platform-specific code to asynchronously
    // delete us) and closes the delegate.
    void MaybeDelete();

    TabContentsWrapper* owner_;
    InfoBarDelegate* delegate_;
    InfoBarContainer* container_;
    ui::SlideAnimation animation_;

    // The current and target heights of the arrow and bar portions, and half the
    // current arrow width.  (It's easier to work in half-widths as we draw the
    // arrow as two halves on either side of a center point.)
    int arrow_height_;         // Includes both fill and top stroke.
    int arrow_target_height_;
    int arrow_half_width_;     // Includes only fill.
    int bar_height_;           // Includes both fill and bottom separator.
    int bar_target_height_;

    DISALLOW_COPY_AND_ASSIGN(InfoBar);
};

#endif //__infobar_h__