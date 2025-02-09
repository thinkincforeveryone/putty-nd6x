
#include "platform_file.h"

#include "logging.h"
#include "threading/thread_restrictions.h"

namespace base
{

PlatformFileInfo::PlatformFileInfo()
    : size(0),
      is_directory(false),
      is_symbolic_link(false) {}

PlatformFileInfo::~PlatformFileInfo() {}


PlatformFile CreatePlatformFile(const FilePath& name,
                                int flags, bool* created, PlatformFileError* error_code)
{
    ThreadRestrictions::AssertIOAllowed();

    DWORD disposition = 0;
    if(created)
    {
        *created = false;
    }

    if(flags & PLATFORM_FILE_OPEN)
    {
        disposition = OPEN_EXISTING;
    }

    if(flags & PLATFORM_FILE_CREATE)
    {
        DCHECK(!disposition);
        disposition = CREATE_NEW;
    }

    if(flags & PLATFORM_FILE_OPEN_ALWAYS)
    {
        DCHECK(!disposition);
        disposition = OPEN_ALWAYS;
    }

    if(flags & PLATFORM_FILE_CREATE_ALWAYS)
    {
        DCHECK(!disposition);
        disposition = CREATE_ALWAYS;
    }

    if(flags & PLATFORM_FILE_TRUNCATE)
    {
        DCHECK(!disposition);
        DCHECK(flags & PLATFORM_FILE_WRITE);
        disposition = TRUNCATE_EXISTING;
    }

    if(!disposition)
    {
        NOTREACHED();
        return NULL;
    }

    DWORD access = (flags & PLATFORM_FILE_READ) ? GENERIC_READ : 0;
    if(flags & PLATFORM_FILE_WRITE)
    {
        access |= GENERIC_WRITE;
    }
    if(flags & PLATFORM_FILE_WRITE_ATTRIBUTES)
    {
        access |= FILE_WRITE_ATTRIBUTES;
    }

    DWORD sharing = (flags & PLATFORM_FILE_EXCLUSIVE_READ) ? 0
                    : FILE_SHARE_READ;
    if(!(flags & PLATFORM_FILE_EXCLUSIVE_WRITE))
    {
        sharing |= FILE_SHARE_WRITE;
    }
    if(flags & PLATFORM_FILE_SHARE_DELETE)
    {
        sharing |= FILE_SHARE_DELETE;
    }

    DWORD create_flags = 0;
    if(flags & PLATFORM_FILE_ASYNC)
    {
        create_flags |= FILE_FLAG_OVERLAPPED;
    }
    if(flags & PLATFORM_FILE_TEMPORARY)
    {
        create_flags |= FILE_ATTRIBUTE_TEMPORARY;
    }
    if(flags & PLATFORM_FILE_HIDDEN)
    {
        create_flags |= FILE_ATTRIBUTE_HIDDEN;
    }
    if(flags & PLATFORM_FILE_DELETE_ON_CLOSE)
    {
        create_flags |= FILE_FLAG_DELETE_ON_CLOSE;
    }

    HANDLE file = CreateFile(name.value().c_str(), access, sharing,
                             NULL, disposition, create_flags, NULL);

    if(created && (INVALID_HANDLE_VALUE != file))
    {
        if(flags & (PLATFORM_FILE_OPEN_ALWAYS))
        {
            *created = (ERROR_ALREADY_EXISTS != GetLastError());
        }
        else if(flags & (PLATFORM_FILE_CREATE_ALWAYS|PLATFORM_FILE_CREATE))
        {
            *created = true;
        }
    }

    if(error_code)
    {
        if(file != kInvalidPlatformFileValue)
        {
            *error_code = PLATFORM_FILE_OK;
        }
        else
        {
            DWORD last_error = GetLastError();
            switch(last_error)
            {
            case ERROR_SHARING_VIOLATION:
                *error_code = PLATFORM_FILE_ERROR_IN_USE;
                break;
            case ERROR_FILE_EXISTS:
                *error_code = PLATFORM_FILE_ERROR_EXISTS;
                break;
            case ERROR_FILE_NOT_FOUND:
                *error_code = PLATFORM_FILE_ERROR_NOT_FOUND;
                break;
            case ERROR_ACCESS_DENIED:
                *error_code = PLATFORM_FILE_ERROR_ACCESS_DENIED;
                break;
            default:
                *error_code = PLATFORM_FILE_ERROR_FAILED;
            }
        }
    }

    return file;
}

bool ClosePlatformFile(PlatformFile file)
{
    ThreadRestrictions::AssertIOAllowed();
    return (CloseHandle(file) != 0);
}

int ReadPlatformFile(PlatformFile file, int64 offset, char* data, int size)
{
    ThreadRestrictions::AssertIOAllowed();
    if(file == kInvalidPlatformFileValue)
    {
        return -1;
    }

    LARGE_INTEGER offset_li;
    offset_li.QuadPart = offset;

    OVERLAPPED overlapped = { 0 };
    overlapped.Offset = offset_li.LowPart;
    overlapped.OffsetHigh = offset_li.HighPart;

    DWORD bytes_read;
    if(ReadFile(file, data, size, &bytes_read, &overlapped) != 0)
    {
        return bytes_read;
    }
    else if(ERROR_HANDLE_EOF == GetLastError())
    {
        return 0;
    }

    return -1;
}

int ReadPlatformFileNoBestEffort(PlatformFile file, int64 offset,
                                 char* data, int size)
{
    return ReadPlatformFile(file, offset, data, size);
}

int WritePlatformFile(PlatformFile file, int64 offset,
                      const char* data, int size)
{
    ThreadRestrictions::AssertIOAllowed();
    if(file == kInvalidPlatformFileValue)
    {
        return -1;
    }

    LARGE_INTEGER offset_li;
    offset_li.QuadPart = offset;

    OVERLAPPED overlapped = { 0 };
    overlapped.Offset = offset_li.LowPart;
    overlapped.OffsetHigh = offset_li.HighPart;

    DWORD bytes_written;
    if(WriteFile(file, data, size, &bytes_written, &overlapped) != 0)
    {
        return bytes_written;
    }

    return -1;
}

bool TruncatePlatformFile(PlatformFile file, int64 length)
{
    ThreadRestrictions::AssertIOAllowed();
    if(file == kInvalidPlatformFileValue)
    {
        return false;
    }

    // 获取当前文件指针.
    LARGE_INTEGER file_pointer;
    LARGE_INTEGER zero;
    zero.QuadPart = 0;
    if(SetFilePointerEx(file, zero, &file_pointer, FILE_CURRENT) == 0)
    {
        return false;
    }

    LARGE_INTEGER length_li;
    length_li.QuadPart = length;
    // 如果length>文件大小, SetFilePointerEx()在所有的Windows标准文件系统
    // (NTFS, FATxx)上都会用0扩展文件长度.
    if(!SetFilePointerEx(file, length_li, NULL, FILE_BEGIN))
    {
        return false;
    }

    // 设置新的文件长度并移动文件指针到原始位置. 和ftruncate()的行为一致,
    // 文件指针位置可能会超过文件结束位置.
    return ((SetEndOfFile(file)!=0) &&
            (SetFilePointerEx(file, file_pointer, NULL, FILE_BEGIN)!=0));
}

bool FlushPlatformFile(PlatformFile file)
{
    ThreadRestrictions::AssertIOAllowed();
    return ((file!=kInvalidPlatformFileValue) && FlushFileBuffers(file));
}

bool TouchPlatformFile(PlatformFile file, const base::Time& last_access_time,
                       const base::Time& last_modified_time)
{
    ThreadRestrictions::AssertIOAllowed();
    if(file == kInvalidPlatformFileValue)
    {
        return false;
    }

    FILETIME last_access_filetime = last_access_time.ToFileTime();
    FILETIME last_modified_filetime = last_modified_time.ToFileTime();
    return (SetFileTime(file, NULL, &last_access_filetime,
                        &last_modified_filetime) != 0);
}

bool GetPlatformFileInfo(PlatformFile file, PlatformFileInfo* info)
{
    ThreadRestrictions::AssertIOAllowed();
    if(!info)
    {
        return false;
    }

    BY_HANDLE_FILE_INFORMATION file_info;
    if(GetFileInformationByHandle(file, &file_info) == 0)
    {
        return false;
    }

    LARGE_INTEGER size;
    size.HighPart = file_info.nFileSizeHigh;
    size.LowPart = file_info.nFileSizeLow;
    info->size = size.QuadPart;
    info->is_directory =
        (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    info->last_modified = base::Time::FromFileTime(file_info.ftLastWriteTime);
    info->last_accessed = base::Time::FromFileTime(file_info.ftLastAccessTime);
    info->creation_time = base::Time::FromFileTime(file_info.ftCreationTime);
    return true;
}

} //namespace base