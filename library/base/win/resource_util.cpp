
#include "resource_util.h"

#include "base/logging.h"

namespace base
{
namespace win
{

bool GetDataResourceFromModule(HMODULE module, int resource_id,
                               void** data, size_t* length)
{
    if(!module)
    {
        return false;
    }

    if(!IS_INTRESOURCE(resource_id))
    {
        NOTREACHED();
        return false;
    }

    HRSRC hres_info = FindResource(module, MAKEINTRESOURCE(resource_id),
                                   L"BINDATA");
    if(NULL == hres_info)
    {
        return false;
    }

    DWORD data_size = SizeofResource(module, hres_info);
    HGLOBAL hres = LoadResource(module, hres_info);
    if(!hres)
    {
        return false;
    }

    void* resource = LockResource(hres);
    if(!resource)
    {
        return false;
    }

    *data = resource;
    *length = static_cast<size_t>(data_size);
    return true;
}

} //namespace win
} //namespace base