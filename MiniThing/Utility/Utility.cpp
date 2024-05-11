#include <Windows.h>
#include <atlbase.h>

#include <iostream>
#include <string>
#include <vector>

#include "Utility.h"

bool DelRegValue(const char* pMidPath, const std::string& key, const std::string& value)
{
    HKEY hkey = nullptr;
    LSTATUS res = ::RegOpenKeyExA(HKEY_CURRENT_USER, pMidPath, 0, KEY_SET_VALUE, &hkey);
    if (res != ERROR_SUCCESS)
    {
        return false;
    }

    res = ::RegDeleteKeyValueA(hkey, key.c_str(), value.c_str());
    return res;
}

bool SetRegValue(const char* pMidPath, const std::string& key, const std::string& value)
{
    HKEY hkey = nullptr;
    LSTATUS res = ::RegOpenKeyExA(HKEY_CURRENT_USER, pMidPath, 0, KEY_WRITE, &hkey);
    if (res != ERROR_SUCCESS)
    {
        res = ::RegCreateKeyA(HKEY_CURRENT_USER, pMidPath, &hkey);
    }

    if (res != ERROR_SUCCESS)
    {
        return false;
    }
    std::shared_ptr<void> close_key
    (
        nullptr, [&](void*)
        {
            if (hkey != nullptr)
            {
                ::RegCloseKey(hkey);
                hkey = nullptr;
            }
        }
    );
    res = ::RegSetValueExA(hkey, key.c_str(), 0, REG_SZ, (BYTE*)value.c_str(), value.length());
    if (res != ERROR_SUCCESS)
    {
        return false;
    }
    return true;
}

std::string GetRegValue(const char *pMidPath, const std::string& key)
{
    HKEY hkey = nullptr;

    LSTATUS res = ::RegOpenKeyExA(HKEY_CURRENT_USER, pMidPath, 0, KEY_READ, &hkey);
    if (res != ERROR_SUCCESS)
    {
        return "";
    }

    std::shared_ptr<void> close_key
    (
        nullptr, [&](void*)
        {
            if (hkey != nullptr)
            {
                ::RegCloseKey(hkey);
                hkey = nullptr;
            }
        }
    );

    DWORD type = REG_SZ;
    DWORD size = 0;
    res = ::RegQueryValueExA(hkey, key.c_str(), 0, &type, nullptr, &size);
    if (res != ERROR_SUCCESS || size <= 0)
    {
        return "";
    }

    std::vector<BYTE> value_data(size);
    res = ::RegQueryValueExA(hkey, key.c_str(), 0, &type, value_data.data(), &size);
    if (res != ERROR_SUCCESS)
    {
        return "";
    }

    return std::string(value_data.begin(), value_data.end());
}