
#include "sys_info.h"

#include <windows.h>

#include "file_path.h"
#include "logging.h"
#include "stringprintf.h"
#include "win/windows_version.h"

namespace base
{

// static
int SysInfo::NumberOfProcessors()
{
    return win::OSInfo::GetInstance()->processors();
}

// static
int64 SysInfo::AmountOfPhysicalMemory()
{
    MEMORYSTATUSEX memory_info;
    memory_info.dwLength = sizeof(memory_info);
    if(!GlobalMemoryStatusEx(&memory_info))
    {
        NOTREACHED();
        return 0;
    }

    int64 rv = static_cast<int64>(memory_info.ullTotalPhys);
    if(rv < 0)
    {
        rv = kint64max;
    }
    return rv;
}

// static
int64 SysInfo::AmountOfFreeDiskSpace(const FilePath& path)
{
    ULARGE_INTEGER available, total, free;
    if(!GetDiskFreeSpaceExW(path.value().c_str(), &available, &total, &free))
    {
        return -1;
    }
    int64 rv = static_cast<int64>(available.QuadPart);
    if(rv < 0)
    {
        rv = kint64max;
    }
    return rv;
}

// static
std::string SysInfo::OperatingSystemName()
{
    return "Windows NT";
}

// static
std::string SysInfo::OperatingSystemVersion()
{
    win::OSInfo* os_info = win::OSInfo::GetInstance();
    win::OSInfo::VersionNumber version_number = os_info->version_number();
    std::string version(StringPrintf("%d.%d", version_number.major,
                                     version_number.minor));
    win::OSInfo::ServicePack service_pack = os_info->service_pack();
    if(service_pack.major != 0)
    {
        version += StringPrintf(" SP%d", service_pack.major);
        if(service_pack.minor != 0)
        {
            version += StringPrintf(".%d", service_pack.minor);
        }
    }
    return version;
}

// static
std::string SysInfo::CPUArchitecture()
{
    // TODO: 支持不同架构的时候会变化.
    return "x86";
}

// static
void SysInfo::GetPrimaryDisplayDimensions(int* width, int* height)
{
    if(width)
    {
        *width = GetSystemMetrics(SM_CXSCREEN);
    }

    if(height)
    {
        *height = GetSystemMetrics(SM_CYSCREEN);
    }
}

// static
int SysInfo::DisplayCount()
{
    return GetSystemMetrics(SM_CMONITORS);
}

// static
size_t SysInfo::VMAllocationGranularity()
{
    return win::OSInfo::GetInstance()->allocation_granularity();
}

// static
void SysInfo::OperatingSystemVersionNumbers(int32 *major_version,
        int32* minor_version, int32* bugfix_version)
{
    win::OSInfo* os_info = win::OSInfo::GetInstance();
    *major_version = os_info->version_number().major;
    *minor_version = os_info->version_number().minor;
    *bugfix_version = 0;
}

} //namespace base