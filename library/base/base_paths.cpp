
#include "base_paths.h"

#include <windows.h>
#include <shlobj.h>

#include "file_path.h"
#include "file_util.h"
#include "path_service.h"
#include "win/windows_version.h"

// http://blogs.msdn.com/oldnewthing/archive/2004/10/25/247180.aspx
extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace base
{

bool PathProvider(int key, FilePath* result)
{
    // NOTE: DIR_CURRENT is a special cased in PathService::Get

    FilePath cur;
    switch(key)
    {
    case base::DIR_EXE:
        PathService::Get(base::FILE_EXE, &cur);
        cur = cur.DirName();
        break;
    case base::DIR_MODULE:
        PathService::Get(base::FILE_MODULE, &cur);
        cur = cur.DirName();
        break;
    case base::DIR_TEMP:
        if(!base::GetTempDir(&cur))
        {
            return false;
        }
        break;
    default:
        return false;
    }

    *result = cur;
    return true;
}

bool PathProviderWin(int key, FilePath* result)
{
    // We need to go compute the value. It would be nice to support paths with
    // names longer than MAX_PATH, but the system functions don't seem to be
    // designed for it either, with the exception of GetTempPath (but other
    // things will surely break if the temp path is too long, so we don't bother
    // handling it.
    wchar_t system_buffer[MAX_PATH];
    system_buffer[0] = 0;

    FilePath cur;
    switch(key)
    {
    case base::FILE_EXE:
        GetModuleFileName(NULL, system_buffer, MAX_PATH);
        cur = FilePath(system_buffer);
        break;
    case base::FILE_MODULE:
    {
        // the resource containing module is assumed to be the one that
        // this code lives in, whether that's a dll or exe
        HMODULE this_module = reinterpret_cast<HMODULE>(&__ImageBase);
        GetModuleFileName(this_module, system_buffer, MAX_PATH);
        cur = FilePath(system_buffer);
        break;
    }
    case base::DIR_WINDOWS:
        GetWindowsDirectory(system_buffer, MAX_PATH);
        cur = FilePath(system_buffer);
        break;
    case base::DIR_SYSTEM:
        GetSystemDirectory(system_buffer, MAX_PATH);
        cur = FilePath(system_buffer);
        break;
    case base::DIR_PROGRAM_FILESX86:
        if(base::win::OSInfo::GetInstance()->architecture() !=
                base::win::OSInfo::X86_ARCHITECTURE)
        {
            if(FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILESX86, NULL,
                                      SHGFP_TYPE_CURRENT, system_buffer)))
            {
                return false;
            }
            cur = FilePath(system_buffer);
            break;
        }
    // Fall through to base::DIR_PROGRAM_FILES if we're on an X86 machine.
    case base::DIR_PROGRAM_FILES:
        if(FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL,
                                  SHGFP_TYPE_CURRENT, system_buffer)))
        {
            return false;
        }
        cur = FilePath(system_buffer);
        break;
    case base::DIR_IE_INTERNET_CACHE:
        if(FAILED(SHGetFolderPath(NULL, CSIDL_INTERNET_CACHE, NULL,
                                  SHGFP_TYPE_CURRENT, system_buffer)))
        {
            return false;
        }
        cur = FilePath(system_buffer);
        break;
    case base::DIR_COMMON_START_MENU:
        if(FAILED(SHGetFolderPath(NULL, CSIDL_COMMON_PROGRAMS, NULL,
                                  SHGFP_TYPE_CURRENT, system_buffer)))
        {
            return false;
        }
        cur = FilePath(system_buffer);
        break;
    case base::DIR_START_MENU:
        if(FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAMS, NULL,
                                  SHGFP_TYPE_CURRENT, system_buffer)))
        {
            return false;
        }
        cur = FilePath(system_buffer);
        break;
    case base::DIR_APP_DATA:
        if(FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                                  system_buffer)))
        {
            return false;
        }
        cur = FilePath(system_buffer);
        break;
    case base::DIR_PROFILE:
        if(FAILED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT,
                                  system_buffer)))
        {
            return false;
        }
        cur = FilePath(system_buffer);
        break;
    case base::DIR_LOCAL_APP_DATA_LOW:
        if(win::GetVersion() < win::VERSION_VISTA)
        {
            return false;
        }
        // TODO(nsylvain): We should use SHGetKnownFolderPath instead. Bug 1281128
        if(FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                                  system_buffer)))
        {
            return false;
        }
        cur = FilePath(system_buffer).DirName().AppendASCII("LocalLow");
        break;
    case base::DIR_LOCAL_APP_DATA:
        if(FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL,
                                  SHGFP_TYPE_CURRENT, system_buffer)))
        {
            return false;
        }
        cur = FilePath(system_buffer);
        break;
    case base::DIR_SOURCE_ROOT:
    {
        FilePath executableDir;
        // On Windows, unit tests execute two levels deep from the source root.
        // For example:  chrome/{Debug|Release}/ui_tests.exe
        PathService::Get(base::DIR_EXE, &executableDir);
        cur = executableDir.DirName().DirName();
        FilePath checkedPath =
            cur.Append(FILE_PATH_LITERAL("base/base_paths.cpp"));
        if(!PathExists(checkedPath))
        {
            // Check for WebKit-only checkout. Executable files are put into
            // WebKit/WebKit/chromium/{Debug|Relese}, and we should return
            // WebKit/WebKit/chromium.
            cur = executableDir.DirName();
        }
        break;
    }
    default:
        return false;
    }

    *result = cur;
    return true;
}

} //namespace base