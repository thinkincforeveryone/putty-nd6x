
#ifndef __ui_base_scoped_clipboard_writer_h__
#define __ui_base_scoped_clipboard_writer_h__

#pragma once

// This file declares the ScopedClipboardWriter class, a wrapper around
// the Clipboard class which simplifies writing data to the system clipboard.
// Upon deletion the class atomically writes all data to |clipboard_|,
// avoiding any potential race condition with other processes that are also
// writing to the system clipboard.

#include "clipboard.h"

class Pickle;

namespace ui
{

// This class is a wrapper for |Clipboard| that handles packing data
// into a Clipboard::ObjectMap.
// NB: You should probably NOT be using this class if you include
// webkit_glue.h. Use ScopedClipboardWriterGlue instead.
class ScopedClipboardWriter
{
public:
    // Create an instance that is a simple wrapper around clipboard.
    explicit ScopedClipboardWriter(Clipboard* clipboard);

    ~ScopedClipboardWriter();

    // Converts |text| to UTF-8 and adds it to the clipboard.
    void WriteText(const string16& text);

    // Converts the text of the URL to UTF-8 and adds it to the clipboard, then
    // notifies the Clipboard that we just wrote a URL.
    void WriteURL(const string16& text);

    // Adds HTML to the clipboard.  The url parameter is optional, but especially
    // useful if the HTML fragment contains relative links.
    void WriteHTML(const string16& markup, const std::string& source_url);

    // Adds a bookmark to the clipboard.
    void WriteBookmark(const string16& bookmark_title,
                       const std::string& url);

    // Adds an html hyperlink (<a href>) to the clipboard. |anchor_text| should
    // be escaped prior to being passed in.
    void WriteHyperlink(const string16& anchor_text, const std::string& url);

    // Used by WebKit to determine whether WebKit wrote the clipboard last
    void WriteWebSmartPaste();

    // Adds a bitmap to the clipboard
    // Pixel format is assumed to be 32-bit BI_RGB.
    void WriteBitmapFromPixels(const void* pixels, const gfx::Size& size);

    // Adds arbitrary data to clipboard.
    void WritePickledData(const Pickle& pickle, Clipboard::FormatType format);

protected:
    // Converts |text| to UTF-8 and adds it to the clipboard.  If it's a URL, we
    // also notify the clipboard of that fact.
    void WriteTextOrURL(const string16& text, bool is_url);

    // We accumulate the data passed to the various targets in the |objects_|
    // vector, and pass it to Clipboard::WriteObjects() during object destruction.
    Clipboard::ObjectMap objects_;
    Clipboard* clipboard_;

    // We keep around the UTF-8 text of the URL in order to pass it to
    // Clipboard::DidWriteURL().
    std::string url_text_;

private:
    DISALLOW_COPY_AND_ASSIGN(ScopedClipboardWriter);
};

} //namespace ui

#endif //__ui_base_scoped_clipboard_writer_h__