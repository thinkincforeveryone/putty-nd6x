
#include "json_value_serializer.h"

#include "base/algorithm/json/json_reader.h"
#include "base/algorithm/json/json_writer.h"
#include "base/file_util.h"
#include "base/string_util.h"

const char* JSONFileValueSerializer::kAccessDenied = "Access denied.";
const char* JSONFileValueSerializer::kCannotReadFile = "Can't read file.";
const char* JSONFileValueSerializer::kFileLocked = "File locked.";
const char* JSONFileValueSerializer::kNoSuchFile = "File doesn't exist.";

JSONStringValueSerializer::~JSONStringValueSerializer() {}

bool JSONStringValueSerializer::Serialize(const base::Value& root)
{
    if(!json_string_ || initialized_with_const_string_)
    {
        return false;
    }

    base::JSONWriter::Write(&root, pretty_print_, json_string_);
    return true;
}

base::Value* JSONStringValueSerializer::Deserialize(int* error_code,
        std::string* error_str)
{
    if(!json_string_)
    {
        return NULL;
    }

    return base::JSONReader::ReadAndReturnError(*json_string_,
            allow_trailing_comma_,
            error_code,
            error_str);
}

/******* File Serializer *******/

bool JSONFileValueSerializer::Serialize(const base::Value& root)
{
    std::string json_string;
    JSONStringValueSerializer serializer(&json_string);
    serializer.set_pretty_print(true);
    bool result = serializer.Serialize(root);
    if(!result)
    {
        return false;
    }

    int data_size = static_cast<int>(json_string.size());
    if(base::WriteFile(json_file_path_,
                       json_string.data(),
                       data_size) != data_size)
    {
        return false;
    }

    return true;
}

int JSONFileValueSerializer::ReadFileToString(std::string* json_string)
{
    DCHECK(json_string);
    if(!base::ReadFileToString(json_file_path_, json_string))
    {
        int error = ::GetLastError();
        if(error==ERROR_SHARING_VIOLATION || error==ERROR_LOCK_VIOLATION)
        {
            return JSON_FILE_LOCKED;
        }
        else if(error == ERROR_ACCESS_DENIED)
        {
            return JSON_ACCESS_DENIED;
        }
        if(!base::PathExists(json_file_path_))
        {
            return JSON_NO_SUCH_FILE;
        }
        else
        {
            return JSON_CANNOT_READ_FILE;
        }
    }
    return JSON_NO_ERROR;
}

const char* JSONFileValueSerializer::GetErrorMessageForCode(int error_code)
{
    switch(error_code)
    {
    case JSON_NO_ERROR:
        return "";
    case JSON_ACCESS_DENIED:
        return kAccessDenied;
    case JSON_CANNOT_READ_FILE:
        return kCannotReadFile;
    case JSON_FILE_LOCKED:
        return kFileLocked;
    case JSON_NO_SUCH_FILE:
        return kNoSuchFile;
    default:
        NOTREACHED();
        return "";
    }
}

base::Value* JSONFileValueSerializer::Deserialize(int* error_code,
        std::string* error_str)
{
    std::string json_string;
    int error = ReadFileToString(&json_string);
    if(error != JSON_NO_ERROR)
    {
        if(error_code)
        {
            *error_code = error;
        }
        if(error_str)
        {
            *error_str = GetErrorMessageForCode(error);
        }
        return NULL;
    }

    JSONStringValueSerializer serializer(json_string);
    return serializer.Deserialize(error_code, error_str);
}