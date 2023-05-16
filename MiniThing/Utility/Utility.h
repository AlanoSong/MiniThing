#pragma once

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stringapiset.h>
#include <vector>

// No 1.1
inline int CharToWchar(const char* pChar, wchar_t* pWchar)
{
    int len = MultiByteToWideChar(CP_ACP, 0, pChar, (int)strlen(pChar), NULL, 0);

    if (pWchar == nullptr)
    {
        // Need 1 more to store 0x00 end
        return len + 1;
    }

    MultiByteToWideChar(CP_ACP, 0, pChar, (int)strlen(pChar), pWchar, len);

    pWchar[len] = 0x00;

    return len;
}

// No 1.2
inline std::string CharToString(const char* pChar)
{
    return std::string(pChar);
}

// No 1.3
inline std::wstring CharToWstring(const char* pChar)
{
    wchar_t* pWchar;
    int len = MultiByteToWideChar(CP_ACP, 0, pChar, (int)strlen(pChar), NULL, 0);

    pWchar = new wchar_t[len + 1];
    MultiByteToWideChar(CP_ACP, 0, pChar, (int)strlen(pChar), pWchar, len);
    pWchar[len] = 0x00;

    std::wstring wStr = pWchar;
    delete[] pWchar;

    return wStr;
}

// No 2.1
inline int WcharToChar(const wchar_t* pWchar, char* pChar)
{
    int len = WideCharToMultiByte(CP_ACP, 0, pWchar, (int)wcslen(pWchar), NULL, 0, NULL, NULL);

    if (pChar == nullptr)
    {
        // Need 1 more to store 0x00 end
        return len + 1;
    }

    WideCharToMultiByte(CP_ACP, 0, pWchar, (int)wcslen(pWchar), pChar, len, NULL, NULL);

    pChar[len] = 0x00;

    return len;
}

// No 2.2
inline std::wstring WcharToWstring(const wchar_t* pWchar)
{
    std::wstring wStr = pWchar;

    return wStr;
}

// No 2.3
inline std::string WcharToString(const wchar_t* pWchar)
{
    int len = WcharToChar(pWchar, nullptr);
    char* pChar = new char[len + 1];
    WcharToChar(pWchar, pChar);

    std::string str = pChar;
    delete[] pChar;

    return str;
}

// No 3.1
inline int StringToChar(const std::string str, char* pChar)
{
    int len = (int)str.length();

    if (pChar == nullptr)
    {
        // Need 1 more to store 0x00 end
        return len + 1;
    }

    memcpy(pChar, str.c_str(), len);
    pChar[len] = 0x00;

    return len;
}

// No 3.2
inline int StringToWchar(const std::string str, wchar_t* pWchar)
{
    int len = CharToWchar(str.c_str(), nullptr);

    if (pWchar == nullptr)
    {
        return len;
    }

    CharToWchar(str.c_str(), pWchar);

    return len;

}

// No 3.3
inline std::wstring StringToWstring(const std::string str)
{
    int len = StringToWchar(str, nullptr);
    wchar_t* pWchar = new wchar_t[len];
    StringToWchar(str, pWchar);

    std::wstring wStr(pWchar);

    delete[] pWchar;

    return wStr;
}

// No 4.1
inline int WstringToWchar(const std::wstring& wStr, wchar_t* pWchar)
{
    int len = (int)wStr.length();
    if (pWchar == nullptr)
    {
        // Need 1 more to store 0x00 end
        return len + 1;
    }

    // Each wchar_t occupy 2 bytes
    memcpy(pWchar, wStr.c_str(), len * 2);
    pWchar[len] = 0x00;

    return len;
}

// No 4.2
inline int WstringToChar(const std::wstring& wStr, char* pChar)
{
    int len = WstringToWchar(wStr, nullptr);
    wchar_t* pWchar = new wchar_t[len];
    if (!pWchar)
    {
        assert(0);
    }

    WstringToWchar(wStr, pWchar);

    len = WcharToChar(pWchar, nullptr);

    if (pChar == nullptr)
    {
        return len;
    }

    WcharToChar(pWchar, pChar);

    return len;
}

// No 4.3
inline std::string WstringToString(const std::wstring& wStr)
{
    int len = WstringToChar(wStr, nullptr);
    char* pChar = new char[len];
    WstringToChar(wStr, pChar);

    std::string str(pChar);

    delete[] pChar;

    return str;
}

// No 5.1
inline std::string UnicodeToUtf8(const std::wstring& wStr)
{
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8Size == 0)
    {
        throw std::exception("Error in conversion.");
    }

    char* pChar = new char[utf8Size];
    int ret = WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), -1, pChar, utf8Size, NULL, NULL);
    if (ret != utf8Size)
    {
        throw std::exception("La falla!");
    }

    std::string str(pChar);
    delete[] pChar;

    return str;
}

// No 5.2
inline std::wstring Utf8ToUnicode(const std::string& utf8Str)
{
    int wideSize = ::MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);

    if (wideSize == ERROR_NO_UNICODE_TRANSLATION)
    {
        throw std::exception("Invalid UTF-8 sequence.");
    }

    if (wideSize == 0)
    {
        throw std::exception("Error in conversion.");
    }

    wchar_t* pWchar = new wchar_t[wideSize];

    int ret = ::MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, pWchar, wideSize);

    if (ret != wideSize)
    {
        throw std::exception("La falla!");
    }

    std::wstring wStr(pWchar);
    delete[] pWchar;

    return wStr;
}

inline void SetChsPrintEnv(void)
{
    // Set chinese debug output
    std::wcout.imbue(std::locale("chs"));
    setlocale(LC_ALL, "zh-CN");
}

inline std::wstring GetFileNameAccordPath(std::wstring path)
{
    return path.substr(path.find_last_of(L"\\") + 1);
}

inline std::wstring GetPathAccordPath(std::wstring path)
{
    std::wstring name = GetFileNameAccordPath(path);
    return path.substr(0, path.length() - name.length());
}

inline void GetSystemError(void)
{
    LPCTSTR   lpMsgBuf;
    DWORD lastError = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        lastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0,
        NULL
    );

    LPCTSTR strValue = lpMsgBuf;
    wprintf_s(L"Err msg: %s\n", strValue);
}
