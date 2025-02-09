
#include "clipboard_util_win.h"

#include <shellapi.h>
#include <shlwapi.h>
#include <wininet.h> // 使用INTERNET_MAX_URL_LENGTH.

#include "base/basic_types.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/utf_string_conversions.h"
#include "base/win/scoped_hglobal.h"

namespace ui
{

namespace
{

bool GetUrlFromHDrop(IDataObject* data_object, std::wstring* url,
                     std::wstring* title)
{
    DCHECK(data_object && url && title);

    STGMEDIUM medium;
    if(FAILED(data_object->GetData(ClipboardUtil::GetCFHDropFormat(), &medium)))
    {
        return false;
    }

    HDROP hdrop = static_cast<HDROP>(GlobalLock(medium.hGlobal));

    if(!hdrop)
    {
        return false;
    }

    bool success = false;
    wchar_t filename[MAX_PATH];
    if(DragQueryFileW(hdrop, 0, filename, arraysize(filename)))
    {
        wchar_t url_buffer[INTERNET_MAX_URL_LENGTH];
        if(0==_wcsicmp(PathFindExtensionW(filename), L".url") &&
                GetPrivateProfileStringW(L"InternetShortcut", L"url", 0,
                                         url_buffer, arraysize(url_buffer), filename))
        {
            url->assign(url_buffer);
            PathRemoveExtension(filename);
            title->assign(PathFindFileName(filename));
            success = true;
        }
    }

    DragFinish(hdrop);
    GlobalUnlock(medium.hGlobal);
    // 据我所知不需要调用ReleaseStgMedium, DragFinish已经帮助我们释放了hGlobal.
    return success;
}

void SplitUrlAndTitle(const std::wstring& str,
                      std::wstring* url, std::wstring* title)
{
    DCHECK(url && title);
    size_t newline_pos = str.find('\n');
    if(newline_pos != std::wstring::npos)
    {
        url->assign(str, 0, newline_pos);
        title->assign(str, newline_pos+1, std::wstring::npos);
    }
    else
    {
        url->assign(str);
        title->assign(str);
    }
}

bool GetFileUrl(IDataObject* data_object, std::wstring* url,
                std::wstring* title)
{
    STGMEDIUM store;
    if(SUCCEEDED(data_object->GetData(ClipboardUtil::GetFilenameWFormat(),
                                      &store)))
    {
        bool success = false;
        {
            // unicode格式的filename.
            base::win::ScopedHGlobal<wchar_t> data(store.hGlobal);
            if(data.get() && data.get()[0] &&
                    (PathFileExists(data.get()) || PathIsUNC(data.get())))
            {
                wchar_t file_url[INTERNET_MAX_URL_LENGTH];
                DWORD file_url_len = arraysize(file_url);
                if(SUCCEEDED(::UrlCreateFromPathW(data.get(), file_url,
                                                  &file_url_len, 0)))
                {
                    url->assign(file_url);
                    title->assign(file_url);
                    success = true;
                }
            }
        }
        ReleaseStgMedium(&store);
        if(success)
        {
            return true;
        }
    }

    if(SUCCEEDED(data_object->GetData(ClipboardUtil::GetFilenameFormat(),
                                      &store)))
    {
        bool success = false;
        {
            // ascii格式的filename.
            base::win::ScopedHGlobal<char> data(store.hGlobal);
            if(data.get() && data.get()[0] &&
                    (PathFileExistsA(data.get()) || PathIsUNCA(data.get())))
            {
                char file_url[INTERNET_MAX_URL_LENGTH];
                DWORD file_url_len = arraysize(file_url);
                if(SUCCEEDED(::UrlCreateFromPathA(data.get(), file_url,
                                                  &file_url_len, 0)))
                {
                    url->assign(UTF8ToWide(file_url));
                    title->assign(*url);
                    success = true;
                }
            }
        }
        ReleaseStgMedium(&store);
        if(success)
        {
            return true;
        }
    }
    return false;
}

}


FORMATETC* ClipboardUtil::GetUrlFormat()
{
    static UINT cf = RegisterClipboardFormat(CFSTR_INETURLA);
    static FORMATETC format =
    {
        cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetUrlWFormat()
{
    static UINT cf = RegisterClipboardFormat(CFSTR_INETURLW);
    static FORMATETC format =
    {
        cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetMozUrlFormat()
{
    // 格式是"URL\nTitle".
    static UINT cf = RegisterClipboardFormat(L"text/x-moz-url");
    static FORMATETC format =
    {
        cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetPlainTextFormat()
{
    // 不需要注册此格式, 因为是内建格式.
    static FORMATETC format =
    {
        CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetPlainTextWFormat()
{
    // 不需要注册此格式, 因为是内建格式.
    static FORMATETC format =
    {
        CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetFilenameWFormat()
{
    static UINT cf = RegisterClipboardFormat(CFSTR_FILENAMEW);
    static FORMATETC format =
    {
        cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetFilenameFormat()
{
    static UINT cf = RegisterClipboardFormat(CFSTR_FILENAMEA);
    static FORMATETC format =
    {
        cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetHtmlFormat()
{
    static UINT cf = RegisterClipboardFormat(L"HTML Format");
    static FORMATETC format =
    {
        cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetTextHtmlFormat()
{
    static UINT cf = RegisterClipboardFormat(L"text/html");
    static FORMATETC format =
    {
        cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetCFHDropFormat()
{
    // 不需要注册此格式, 因为是内建格式.
    static FORMATETC format =
    {
        CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetFileDescriptorFormat()
{
    static UINT cf = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
    static FORMATETC format =
    {
        cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetFileContentFormatZero()
{
    static UINT cf = RegisterClipboardFormat(CFSTR_FILECONTENTS);
    static FORMATETC format =
    {
        cf, 0, DVASPECT_CONTENT, 0, TYMED_HGLOBAL
    };
    return &format;
}

FORMATETC* ClipboardUtil::GetWebKitSmartPasteFormat()
{
    static UINT cf = RegisterClipboardFormat(L"WebKit Smart Paste Format");
    static FORMATETC format =
    {
        cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    return &format;
}


bool ClipboardUtil::HasUrl(IDataObject* data_object)
{
    DCHECK(data_object);
    return SUCCEEDED(data_object->QueryGetData(GetMozUrlFormat())) ||
           SUCCEEDED(data_object->QueryGetData(GetUrlWFormat())) ||
           SUCCEEDED(data_object->QueryGetData(GetUrlFormat())) ||
           SUCCEEDED(data_object->QueryGetData(GetFilenameWFormat())) ||
           SUCCEEDED(data_object->QueryGetData(GetFilenameFormat()));
}

bool ClipboardUtil::HasFilenames(IDataObject* data_object)
{
    DCHECK(data_object);
    return SUCCEEDED(data_object->QueryGetData(GetCFHDropFormat()));
}

bool ClipboardUtil::HasFileContents(IDataObject* data_object)
{
    DCHECK(data_object);
    return SUCCEEDED(data_object->QueryGetData(GetFileContentFormatZero()));
}

bool ClipboardUtil::HasHtml(IDataObject* data_object)
{
    DCHECK(data_object);
    return SUCCEEDED(data_object->QueryGetData(GetHtmlFormat())) ||
           SUCCEEDED(data_object->QueryGetData(GetTextHtmlFormat()));
}

bool ClipboardUtil::HasPlainText(IDataObject* data_object)
{
    DCHECK(data_object);
    return SUCCEEDED(data_object->QueryGetData(GetPlainTextWFormat())) ||
           SUCCEEDED(data_object->QueryGetData(GetPlainTextFormat()));
}


bool ClipboardUtil::GetUrl(IDataObject* data_object, std::wstring* url,
                           std::wstring* title, bool convert_filenames)
{
    DCHECK(data_object && url && title);
    if(!HasUrl(data_object))
    {
        return false;
    }

    // 尝试从|data_object|的多种格式中提取URL.
    STGMEDIUM store;
    if(GetUrlFromHDrop(data_object, url, title))
    {
        return true;
    }

    if(SUCCEEDED(data_object->GetData(GetMozUrlFormat(), &store)) ||
            SUCCEEDED(data_object->GetData(GetUrlWFormat(), &store)))
    {
        {
            // Mozilla URL格式或者unicode URL.
            base::win::ScopedHGlobal<wchar_t> data(store.hGlobal);
            SplitUrlAndTitle(data.get(), url, title);
        }
        ReleaseStgMedium(&store);
        return true;
    }

    if(SUCCEEDED(data_object->GetData(GetUrlFormat(), &store)))
    {
        {
            // URL使用ascii.
            base::win::ScopedHGlobal<char> data(store.hGlobal);
            SplitUrlAndTitle(UTF8ToWide(data.get()), url, title);
        }
        ReleaseStgMedium(&store);
        return true;
    }

    if(convert_filenames)
    {
        return GetFileUrl(data_object, url, title);
    }
    else
    {
        return false;
    }
}

bool ClipboardUtil::GetFilenames(IDataObject* data_object,
                                 std::vector<std::wstring>* filenames)
{
    DCHECK(data_object && filenames);
    if(!HasFilenames(data_object))
    {
        return false;
    }

    STGMEDIUM medium;
    if(FAILED(data_object->GetData(GetCFHDropFormat(), &medium)))
    {
        return false;
    }

    HDROP hdrop = static_cast<HDROP>(GlobalLock(medium.hGlobal));
    if(!hdrop)
    {
        return false;
    }

    const int kMaxFilenameLen = 4096;
    const unsigned num_files = DragQueryFileW(hdrop, 0xffffffff, 0, 0);
    for(unsigned int i=0; i<num_files; ++i)
    {
        wchar_t filename[kMaxFilenameLen];
        if(!DragQueryFileW(hdrop, i, filename, kMaxFilenameLen))
        {
            continue;
        }
        filenames->push_back(filename);
    }

    DragFinish(hdrop);
    GlobalUnlock(medium.hGlobal);
    // 据我所知不需要调用ReleaseStgMedium, DragFinish已经帮助我们释放了hGlobal.
    return true;
}

bool ClipboardUtil::GetPlainText(IDataObject* data_object,
                                 std::wstring* plain_text)
{
    DCHECK(data_object && plain_text);
    if(!HasPlainText(data_object))
    {
        return false;
    }

    STGMEDIUM store;
    if(SUCCEEDED(data_object->GetData(GetPlainTextWFormat(), &store)))
    {
        {
            // Unicode文本.
            base::win::ScopedHGlobal<wchar_t> data(store.hGlobal);
            plain_text->assign(data.get());
        }
        ReleaseStgMedium(&store);
        return true;
    }

    if(SUCCEEDED(data_object->GetData(GetPlainTextFormat(), &store)))
    {
        {
            // ascii文本.
            base::win::ScopedHGlobal<char> data(store.hGlobal);
            plain_text->assign(UTF8ToWide(data.get()));
        }
        ReleaseStgMedium(&store);
        return true;
    }

    // 如果是window平台上拖放的文件, 没有提供任何类型的普通文本格式, 所以
    // 这里我们尝试强行获取一个url.
    std::wstring title;
    return GetUrl(data_object, plain_text, &title, false);
}

bool ClipboardUtil::GetHtml(IDataObject* data_object, std::wstring* html,
                            std::string* base_url)
{
    DCHECK(data_object && html && base_url);

    STGMEDIUM store;
    if(SUCCEEDED(data_object->QueryGetData(GetHtmlFormat())) &&
            SUCCEEDED(data_object->GetData(GetHtmlFormat(), &store)))
    {
        {
            // MS CF html
            base::win::ScopedHGlobal<char> data(store.hGlobal);

            std::string html_utf8;
            CFHtmlToHtml(std::string(data.get(), data.Size()), &html_utf8, base_url);
            html->assign(UTF8ToWide(html_utf8));
        }
        ReleaseStgMedium(&store);
        return true;
    }

    if(FAILED(data_object->QueryGetData(GetTextHtmlFormat())))
    {
        return false;
    }

    if(FAILED(data_object->GetData(GetTextHtmlFormat(), &store)))
    {
        return false;
    }

    {
        // text/html
        base::win::ScopedHGlobal<wchar_t> data(store.hGlobal);
        html->assign(data.get());
    }
    ReleaseStgMedium(&store);
    return true;
}

bool ClipboardUtil::GetFileContents(IDataObject* data_object, std::wstring* filename,
                                    std::string* file_contents)
{
    DCHECK(data_object && filename && file_contents);
    if(!SUCCEEDED(data_object->QueryGetData(GetFileContentFormatZero())) &&
            !SUCCEEDED(data_object->QueryGetData(GetFileDescriptorFormat())))
    {
        return false;
    }

    STGMEDIUM content;
    // 调用GetData有可能会很慢, 具体取决于|data_object|的实现.
    if(SUCCEEDED(data_object->GetData(GetFileContentFormatZero(), &content)))
    {
        if(TYMED_HGLOBAL == content.tymed)
        {
            base::win::ScopedHGlobal<char> data(content.hGlobal);
            file_contents->assign(data.get(), data.Size());
        }
        ReleaseStgMedium(&content);
    }

    STGMEDIUM description;
    if(SUCCEEDED(data_object->GetData(GetFileDescriptorFormat(),
                                      &description)))
    {
        {
            base::win::ScopedHGlobal<FILEGROUPDESCRIPTOR> fgd(description.hGlobal);
            // 这里至少有一个文件.
            DCHECK_GE(fgd->cItems, 1u);
            filename->assign(fgd->fgd[0].cFileName);
        }
        ReleaseStgMedium(&description);
    }
    return true;
}

// HtmlToCFHtml and CFHtmlToHtml are based on similar methods in
// WebCore/platform/win/ClipboardUtilitiesWin.cpp.
/*
* Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Helper method for converting from text/html to MS CF_HTML.
// Documentation for the CF_HTML format is available at
// http://msdn.microsoft.com/en-us/library/aa767917(VS.85).aspx
std::string ClipboardUtil::HtmlToCFHtml(const std::string& html,
                                        const std::string& base_url)
{
    if(html.empty())
    {
        return std::string();
    }

#define MAX_DIGITS 10
#define MAKE_NUMBER_FORMAT_1(digits) MAKE_NUMBER_FORMAT_2(digits)
#define MAKE_NUMBER_FORMAT_2(digits) "%0" #digits "u"
#define NUMBER_FORMAT MAKE_NUMBER_FORMAT_1(MAX_DIGITS)

    static const char* header = "Version:0.9\r\n"
                                "StartHTML:" NUMBER_FORMAT "\r\n"
                                "EndHTML:" NUMBER_FORMAT "\r\n"
                                "StartFragment:" NUMBER_FORMAT "\r\n"
                                "EndFragment:" NUMBER_FORMAT "\r\n";
    static const char* source_url_prefix = "SourceURL:";

    static const char* start_markup =
        "<html>\r\n<body>\r\n<!--StartFragment-->\r\n";
    static const char* end_markup =
        "\r\n<!--EndFragment-->\r\n</body>\r\n</html>";

    // Calculate offsets
    size_t start_html_offset = strlen(header) - strlen(NUMBER_FORMAT) * 4 +
                               MAX_DIGITS * 4;
    if(!base_url.empty())
    {
        start_html_offset += strlen(source_url_prefix) +
                             base_url.length() + 2; // Add 2 for \r\n.
    }
    size_t start_fragment_offset = start_html_offset + strlen(start_markup);
    size_t end_fragment_offset = start_fragment_offset + html.length();
    size_t end_html_offset = end_fragment_offset + strlen(end_markup);

    std::string result = base::StringPrintf(header, start_html_offset,
                                            end_html_offset, start_fragment_offset, end_fragment_offset);
    if(!base_url.empty())
    {
        result.append(source_url_prefix);
        result.append(base_url);
        result.append("\r\n");
    }
    result.append(start_markup);
    result.append(html);
    result.append(end_markup);

#undef MAX_DIGITS
#undef MAKE_NUMBER_FORMAT_1
#undef MAKE_NUMBER_FORMAT_2
#undef NUMBER_FORMAT

    return result;
}

// Helper method for converting from MS CF_HTML to text/html.
void ClipboardUtil::CFHtmlToHtml(const std::string& cf_html,
                                 std::string* html,
                                 std::string* base_url)
{
    // Obtain base_url if present.
    if(base_url)
    {
        static std::string src_url_str("SourceURL:");
        size_t line_start = cf_html.find(src_url_str);
        if(line_start != std::string::npos)
        {
            size_t src_end = cf_html.find("\n", line_start);
            size_t src_start = line_start + src_url_str.length();
            if(src_end!=std::string::npos && src_start!=std::string::npos)
            {
                *base_url = cf_html.substr(src_start, src_end-src_start);
                TrimWhitespace(*base_url, TRIM_ALL, base_url);
            }
        }
    }

    // Find the markup between "<!--StartFragment-->" and "<!--EndFragment-->".
    // If the comments cannot be found, like copying from OpenOffice Writer,
    // we simply fall back to using StartFragment/EndFragment bytecount values
    // to get the markup.
    if(html)
    {
        size_t fragment_start = std::string::npos;
        size_t fragment_end = std::string::npos;

        std::string cf_html_lower = StringToLowerASCII(cf_html);
        size_t markup_start = cf_html_lower.find("<html", 0);
        size_t tag_start = cf_html.find("<!--StartFragment", markup_start);
        if(tag_start == std::string::npos)
        {
            static std::string start_fragment_str("StartFragment:");
            size_t start_fragment_start = cf_html.find(start_fragment_str);
            if(start_fragment_start != std::string::npos)
            {
                fragment_start = static_cast<size_t>(atoi(cf_html.c_str() +
                                                     start_fragment_start + start_fragment_str.length()));
            }

            static std::string end_fragment_str("EndFragment:");
            size_t end_fragment_start = cf_html.find(end_fragment_str);
            if(end_fragment_start != std::string::npos)
            {
                fragment_end = static_cast<size_t>(atoi(cf_html.c_str() +
                                                        end_fragment_start + end_fragment_str.length()));
            }
        }
        else
        {
            fragment_start = cf_html.find('>', tag_start) + 1;
            size_t tag_end = cf_html.rfind("<!--EndFragment", std::string::npos);
            fragment_end = cf_html.rfind('<', tag_end);
        }
        if(fragment_start!=std::string::npos && fragment_end!=std::string::npos)
        {
            *html = cf_html.substr(fragment_start, fragment_end-fragment_start);
            TrimWhitespace(*html, TRIM_ALL, html);
        }
    }
}

} //namespace ui