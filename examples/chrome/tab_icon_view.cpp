
#include "tab_icon_view.h"

#include <windows.h>
#include <shellapi.h>

#include "SkBitmap.h"

#include "ui_gfx/canvas.h"
#include "ui_gfx/favicon_size.h"
#include "ui_gfx/icon_util.h"

#include "ui_base/resource/app_res_ids.h"
#include "ui_base/resource/resource_bundle.h"
#include "ui_base/theme_provider.h"

#include "../wanui_res/resource.h"

static bool g_initialized = false;
static SkBitmap* g_default_favicon = NULL;

// static
void TabIconView::InitializeIfNeeded()
{
    if(!g_initialized)
    {
        g_initialized = true;

        // The default window icon is the application icon, not the default
        // favicon.
        //HICON app_icon = GetAppIcon();
        //g_default_favicon =
        //    IconUtil::CreateSkBitmapFromHICON(app_icon, gfx::Size(16, 16));
        //DestroyIcon(app_icon);

        g_default_favicon =
            ui::ResourceBundle::GetSharedInstance().GetBitmapNamed(
                IDR_PRODUCT_LOGO_16);
    }
}

TabIconView::TabIconView(TabIconViewModel* model)
    : model_(model),
      throbber_running_(false),
      is_light_(false),
      throbber_frame_(0)
{
    InitializeIfNeeded();
}

TabIconView::~TabIconView()
{
}

void TabIconView::Update()
{
    static bool initialized = false;
    static int throbber_frame_count = 0;
    if(!initialized)
    {
        initialized = true;
        SkBitmap throbber(
            *ui::ResourceBundle::GetSharedInstance().GetBitmapNamed(
                IDR_THROBBER));
        throbber_frame_count = throbber.width() / throbber.height();
    }

    if(throbber_running_)
    {
        // We think the tab is loading.
        if(!model_->ShouldTabIconViewAnimate())
        {
            // Woops, tab is invalid or not loading, reset our status and schedule
            // a paint.
            throbber_running_ = false;
            SchedulePaint();
        }
        else
        {
            // The tab is still loading, increment the frame.
            throbber_frame_ = (throbber_frame_ + 1) % throbber_frame_count;
            SchedulePaint();
        }
    }
    else if(model_->ShouldTabIconViewAnimate())
    {
        // We didn't think we were loading, but the tab is loading. Reset the
        // frame and status and schedule a paint.
        throbber_running_ = true;
        throbber_frame_ = 0;
        SchedulePaint();
    }
}

void TabIconView::PaintThrobber(gfx::Canvas* canvas)
{
    SkBitmap throbber(*GetThemeProvider()->GetBitmapNamed(
                          is_light_ ? IDR_THROBBER_LIGHT : IDR_THROBBER));
    int image_size = throbber.height();
    PaintIcon(canvas, throbber, throbber_frame_ * image_size, 0, image_size,
              image_size, false);
}

void TabIconView::PaintFavicon(gfx::Canvas* canvas, const SkBitmap& bitmap)
{
    PaintIcon(canvas, bitmap, 0, 0, bitmap.width(), bitmap.height(), true);
}

void TabIconView::PaintIcon(gfx::Canvas* canvas,
                            const SkBitmap& bitmap,
                            int src_x,
                            int src_y,
                            int src_w,
                            int src_h,
                            bool filter)
{
    // For source images smaller than the favicon square, scale them as if they
    // were padded to fit the favicon square, so we don't blow up tiny favicons
    // into larger or nonproportional results.
    float float_src_w = static_cast<float>(src_w);
    float float_src_h = static_cast<float>(src_h);
    float scalable_w, scalable_h;
    if(src_w<=kFaviconSize && src_h<=kFaviconSize)
    {
        scalable_w = scalable_h = kFaviconSize;
    }
    else
    {
        scalable_w = float_src_w;
        scalable_h = float_src_h;
    }

    // Scale proportionately.
    float scale = std::min(static_cast<float>(width()) / scalable_w,
                           static_cast<float>(height()) / scalable_h);
    int dest_w = static_cast<int>(float_src_w * scale);
    int dest_h = static_cast<int>(float_src_h * scale);

    // Center the scaled image.
    canvas->DrawBitmapInt(bitmap, src_x, src_y, src_w, src_h,
                          (width() - dest_w) / 2, (height() - dest_h) / 2, dest_w,
                          dest_h, filter);
}

void TabIconView::OnPaint(gfx::Canvas* canvas)
{
    bool rendered = false;

    if(throbber_running_)
    {
        rendered = true;
        PaintThrobber(canvas);
    }
    else
    {
        SkBitmap favicon = model_->GetFaviconForTabIconView();
        if(!favicon.isNull())
        {
            rendered = true;
            PaintFavicon(canvas, favicon);
        }
    }

    if(!rendered)
    {
        PaintFavicon(canvas, *g_default_favicon);
    }
}

gfx::Size TabIconView::GetPreferredSize()
{
    return gfx::Size(kFaviconSize, kFaviconSize);
}