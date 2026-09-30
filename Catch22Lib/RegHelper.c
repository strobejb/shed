#include "RegHelper.h"

int GetSettingInt(HKEY hkey, LPCTSTR valueName, int defaultValue)
{
    DWORD type = 0;
    DWORD value = 0;
    DWORD valueSize = sizeof(value);

    if (RegQueryValueEx(hkey, valueName, NULL, &type, (LPBYTE)&value, &valueSize) == ERROR_SUCCESS && type == REG_DWORD)
    {
        return (int)value;
    }

    return defaultValue;
}

BOOL GetSettingBinary(HKEY hkey, LPCTSTR valueName, void *buffer, DWORD bufferSize)
{
    DWORD type = 0;
    DWORD valueSize = bufferSize;

    return RegQueryValueEx(hkey, valueName, NULL, &type, (LPBYTE)buffer, &valueSize) == ERROR_SUCCESS &&
           type == REG_BINARY &&
           valueSize <= bufferSize;
}

BOOL GetSettingStr(HKEY hkey, LPCTSTR valueName, LPCTSTR defaultValue, LPTSTR buffer, DWORD cchBuffer)
{
    DWORD type = 0;
    DWORD valueSize = cchBuffer * sizeof(TCHAR);

    if (RegQueryValueEx(hkey, valueName, NULL, &type, (LPBYTE)buffer, &valueSize) == ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_EXPAND_SZ))
    {
        buffer[cchBuffer - 1] = _T('\0');
        return TRUE;
    }

    if (defaultValue != buffer)
    {
        lstrcpyn(buffer, defaultValue, (int)cchBuffer);
    }
    else
    {
        buffer[cchBuffer - 1] = _T('\0');
    }

    return FALSE;
}

BOOL WriteSettingInt(HKEY hkey, LPCTSTR valueName, int value)
{
    DWORD dwordValue = (DWORD)value;
    return RegSetValueEx(hkey, valueName, 0, REG_DWORD, (const BYTE *)&dwordValue, sizeof(dwordValue)) == ERROR_SUCCESS;
}

BOOL WriteSettingBinary(HKEY hkey, LPCTSTR valueName, const void *buffer, DWORD bufferSize)
{
    return RegSetValueEx(hkey, valueName, 0, REG_BINARY, (const BYTE *)buffer, bufferSize) == ERROR_SUCCESS;
}

BOOL WriteSettingStr(HKEY hkey, LPCTSTR valueName, LPCTSTR value)
{
    DWORD valueSize = (lstrlen(value) + 1) * sizeof(TCHAR);
    return RegSetValueEx(hkey, valueName, 0, REG_SZ, (const BYTE *)value, valueSize) == ERROR_SUCCESS;
}
