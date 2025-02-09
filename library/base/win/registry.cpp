
#include "registry.h"

#include <shlwapi.h>

#include "base/logging.h"
#include "base/threading/thread_restrictions.h"

#pragma comment(lib, "shlwapi.lib") // SHDeleteKey

namespace base
{
namespace win
{

RegKey::RegKey() : key_(NULL), watch_event_(0) {}

RegKey::RegKey(HKEY rootkey, const wchar_t* subkey, REGSAM access)
    : key_(NULL), watch_event_(0)
{
    ThreadRestrictions::AssertIOAllowed();

    if(rootkey)
    {
        if(access & (KEY_SET_VALUE|KEY_CREATE_SUB_KEY|KEY_CREATE_LINK))
        {
            Create(rootkey, subkey, access);
        }
        else
        {
            Open(rootkey, subkey, access);
        }
    }
    else
    {
        DCHECK(!subkey);
    }
}

RegKey::~RegKey()
{
    Close();
}

void RegKey::Close()
{
    ThreadRestrictions::AssertIOAllowed();
    StopWatching();
    if(key_)
    {
        ::RegCloseKey(key_);
        key_ = NULL;
    }
}

LONG RegKey::Create(HKEY rootkey, const wchar_t* subkey, REGSAM access)
{
    DWORD disposition_value;
    return CreateWithDisposition(rootkey, subkey, &disposition_value, access);
}

LONG RegKey::CreateWithDisposition(HKEY rootkey, const wchar_t* subkey,
                                   DWORD* disposition, REGSAM access)
{
    ThreadRestrictions::AssertIOAllowed();
    DCHECK(rootkey && subkey && access && disposition);
    Close();

    LONG result = RegCreateKeyEx(rootkey,
                                 subkey,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 access,
                                 NULL,
                                 &key_,
                                 disposition);
    return result;
}

LONG RegKey::Open(HKEY rootkey, const wchar_t* subkey, REGSAM access)
{
    ThreadRestrictions::AssertIOAllowed();
    DCHECK(rootkey && subkey && access);
    Close();

    LONG result = RegOpenKeyEx(rootkey, subkey, 0, access, &key_);
    return result;
}

LONG RegKey::CreateKey(const wchar_t* name, REGSAM access)
{
    ThreadRestrictions::AssertIOAllowed();
    DCHECK(name && access);

    HKEY subkey = NULL;
    LONG result = RegCreateKeyEx(key_, name, 0, NULL,
                                 REG_OPTION_NON_VOLATILE, access, NULL, &subkey, NULL);
    Close();

    key_ = subkey;
    return result;
}

LONG RegKey::OpenKey(const wchar_t* name, REGSAM access)
{
    ThreadRestrictions::AssertIOAllowed();
    DCHECK(name && access);

    HKEY subkey = NULL;
    LONG result = RegOpenKeyEx(key_, name, 0, access, &subkey);

    Close();

    key_ = subkey;
    return result;
}

DWORD RegKey::ValueCount() const
{
    ThreadRestrictions::AssertIOAllowed();
    DWORD count = 0;
    LONG result = RegQueryInfoKey(key_, NULL, 0, NULL, NULL,
                                  NULL, NULL, &count, NULL, NULL, NULL, NULL);
    return (result!=ERROR_SUCCESS) ? 0 : count;
}

LONG RegKey::ReadName(int index, std::wstring* name)
{
    ThreadRestrictions::AssertIOAllowed();
    wchar_t buf[256];
    DWORD bufsize = arraysize(buf);
    LONG r = ::RegEnumValue(key_, index, buf, &bufsize, NULL,
                            NULL, NULL, NULL);
    if(r == ERROR_SUCCESS)
    {
        *name = buf;
    }
    return r;
}

bool RegKey::ValueExists(const wchar_t* name)
{
    ThreadRestrictions::AssertIOAllowed();
    LONG result = RegQueryValueEx(key_, name, 0, NULL, NULL, NULL);
    return result == ERROR_SUCCESS;
}

LONG RegKey::ReadValue(const wchar_t* name, void* data,
                       DWORD* dsize, DWORD* dtype) const
{
    ThreadRestrictions::AssertIOAllowed();
    LONG result = RegQueryValueEx(key_, name, 0, dtype,
                                  reinterpret_cast<LPBYTE>(data), dsize);
    return result;
}

LONG RegKey::ReadValue(const wchar_t* name, std::wstring* value) const
{
    ThreadRestrictions::AssertIOAllowed();
    DCHECK(value);
    const size_t kMaxStringLength = 1024; // 展开后的长度.
    // 如果1024太小, 使用其它ReadValue方式.
    wchar_t raw_value[kMaxStringLength];
    DWORD type = REG_SZ, size = sizeof(raw_value);
    LONG result = ReadValue(name, raw_value, &size, &type);
    if(result == ERROR_SUCCESS)
    {
        if(type == REG_SZ)
        {
            *value = raw_value;
        }
        else if(type == REG_EXPAND_SZ)
        {
            wchar_t expanded[kMaxStringLength];
            size = ExpandEnvironmentStrings(raw_value, expanded, kMaxStringLength);
            // 成功: 返回拷贝的字符串长度.
            // 失败: 如果缓冲区太小返回需要的长度.
            // 失败: 其它返回0.
            if(size==0 || size>kMaxStringLength)
            {
                result = ERROR_MORE_DATA;
            }
            else
            {
                *value = expanded;
            }
        }
        else
        {
            // 不是字符串.
            result = ERROR_CANTREAD;
        }
    }

    return result;
}

LONG RegKey::ReadValueDW(const wchar_t* name, DWORD* value) const
{
    DCHECK(value);
    DWORD type = REG_DWORD;
    DWORD size = sizeof(DWORD);
    DWORD local_value = 0;
    LONG result = ReadValue(name, &local_value, &size, &type);
    if(result == ERROR_SUCCESS)
    {
        if((type==REG_DWORD || type==REG_BINARY) && size==sizeof(DWORD))
        {
            *value = local_value;
        }
        else
        {
            result = ERROR_CANTREAD;
        }
    }

    return result;
}

LONG RegKey::ReadInt64(const wchar_t* name, int64* value) const
{
    DCHECK(value);
    DWORD type = REG_QWORD;
    int64 local_value = 0;
    DWORD size = sizeof(local_value);
    LONG result = ReadValue(name, &local_value, &size, &type);
    if(result == ERROR_SUCCESS)
    {
        if((type==REG_QWORD || type==REG_BINARY) && size==sizeof(local_value))
        {
            *value = local_value;
        }
        else
        {
            result = ERROR_CANTREAD;
        }
    }

    return result;
}

LONG RegKey::WriteValue(const wchar_t* name, const void * data,
                        DWORD dsize, DWORD dtype)
{
    ThreadRestrictions::AssertIOAllowed();
    DCHECK(data || !dsize);

    LONG result = RegSetValueEx(key_, name, 0, dtype,
                                reinterpret_cast<LPBYTE>(const_cast<void*>(data)), dsize);
    return result;
}

LONG RegKey::WriteValue(const wchar_t* name, const wchar_t* value)
{
    return WriteValue(name, value,
                      static_cast<DWORD>(sizeof(*value)*(wcslen(value)+1)), REG_SZ);
}

LONG RegKey::WriteValue(const wchar_t* name, DWORD value)
{
    return WriteValue(name, &value,
                      static_cast<DWORD>(sizeof(value)), REG_DWORD);
}

LONG RegKey::DeleteKey(const wchar_t* name)
{
    ThreadRestrictions::AssertIOAllowed();
    DCHECK(key_);
    DCHECK(name);
    LONG result = SHDeleteKey(key_, name);
    return result;
}

LONG RegKey::DeleteValue(const wchar_t* value_name)
{
    ThreadRestrictions::AssertIOAllowed();
    DCHECK(key_);
    DCHECK(value_name);
    LONG result = RegDeleteValue(key_, value_name);
    return result;
}

LONG RegKey::StartWatching()
{
    DCHECK(key_);
    if(!watch_event_)
    {
        watch_event_ = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    DWORD filter = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES |
                   REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_SECURITY;

    // Watch the registry key for a change of value.
    LONG result = RegNotifyChangeKeyValue(key_, TRUE, filter, watch_event_, TRUE);
    if(result != ERROR_SUCCESS)
    {
        CloseHandle(watch_event_);
        watch_event_ = 0;
    }

    return result;
}

bool RegKey::HasChanged()
{
    if(watch_event_)
    {
        if(WaitForSingleObject(watch_event_, 0) == WAIT_OBJECT_0)
        {
            StartWatching();
            return true;
        }
    }
    return false;
}

LONG RegKey::StopWatching()
{
    LONG result = ERROR_INVALID_HANDLE;
    if(watch_event_)
    {
        CloseHandle(watch_event_);
        watch_event_ = 0;
        result = ERROR_SUCCESS;
    }
    return result;
}

// RegistryValueIterator ------------------------------------------------------

RegistryValueIterator::RegistryValueIterator(HKEY root_key,
        const wchar_t* folder_key)
{
    ThreadRestrictions::AssertIOAllowed();

    LONG result = RegOpenKeyEx(root_key, folder_key, 0, KEY_READ, &key_);
    if(result != ERROR_SUCCESS)
    {
        key_ = NULL;
    }
    else
    {
        DWORD count = 0;
        result = ::RegQueryInfoKey(key_, NULL, 0, NULL, NULL, NULL, NULL,
                                   &count, NULL, NULL, NULL, NULL);

        if(result != ERROR_SUCCESS)
        {
            ::RegCloseKey(key_);
            key_ = NULL;
        }
        else
        {
            index_ = count - 1;
        }
    }

    Read();
}

RegistryValueIterator::~RegistryValueIterator()
{
    ThreadRestrictions::AssertIOAllowed();
    if(key_)
    {
        ::RegCloseKey(key_);
    }
}

bool RegistryValueIterator::Valid() const
{
    return key_!=NULL && index_>=0;
}

void RegistryValueIterator::operator++()
{
    --index_;
    Read();
}

bool RegistryValueIterator::Read()
{
    ThreadRestrictions::AssertIOAllowed();

    if(Valid())
    {
        DWORD ncount = arraysize(name_);
        value_size_ = sizeof(value_);
        LONG r = ::RegEnumValue(key_, index_, name_, &ncount, NULL,
                                &type_, reinterpret_cast<BYTE*>(value_), &value_size_);
        if(ERROR_SUCCESS == r)
        {
            return true;
        }
    }

    name_[0] = '\0';
    value_[0] = '\0';
    value_size_ = 0;
    return false;
}

DWORD RegistryValueIterator::ValueCount() const
{
    ThreadRestrictions::AssertIOAllowed();

    DWORD count = 0;
    LONG result = ::RegQueryInfoKey(key_, NULL, 0, NULL, NULL, NULL,
                                    NULL, &count, NULL, NULL, NULL, NULL);

    if(result != ERROR_SUCCESS)
    {
        return 0;
    }

    return count;
}

// RegistryKeyIterator --------------------------------------------------------

RegistryKeyIterator::RegistryKeyIterator(HKEY root_key,
        const wchar_t* folder_key)
{
    ThreadRestrictions::AssertIOAllowed();

    LONG result = RegOpenKeyEx(root_key, folder_key, 0, KEY_READ, &key_);
    if(result != ERROR_SUCCESS)
    {
        key_ = NULL;
    }
    else
    {
        DWORD count = 0;
        LONG result = ::RegQueryInfoKey(key_, NULL, 0, NULL, &count,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        if(result != ERROR_SUCCESS)
        {
            ::RegCloseKey(key_);
            key_ = NULL;
        }
        else
        {
            index_ = count - 1;
        }
    }

    Read();
}

RegistryKeyIterator::~RegistryKeyIterator()
{
    ThreadRestrictions::AssertIOAllowed();

    if(key_)
    {
        ::RegCloseKey(key_);
    }
}

bool RegistryKeyIterator::Valid() const
{
    return key_!=NULL && index_>=0;
}

void RegistryKeyIterator::operator++()
{
    --index_;
    Read();
}

bool RegistryKeyIterator::Read()
{
    ThreadRestrictions::AssertIOAllowed();
    if(Valid())
    {
        DWORD ncount = arraysize(name_);
        FILETIME written;
        LONG r = ::RegEnumKeyEx(key_, index_, name_, &ncount,
                                NULL, NULL, NULL, &written);
        if(ERROR_SUCCESS == r)
        {
            return true;
        }
    }

    name_[0] = '\0';
    return false;
}

DWORD RegistryKeyIterator::SubkeyCount() const
{
    ThreadRestrictions::AssertIOAllowed();
    DWORD count = 0;
    LONG result = ::RegQueryInfoKey(key_, NULL, 0, NULL, &count,
                                    NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if(result != ERROR_SUCCESS)
    {
        return 0;
    }

    return count;
}

} //namespace win
} //namespace base