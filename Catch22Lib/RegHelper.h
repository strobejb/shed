#ifndef CATCH22_REGHELPER_H
#define CATCH22_REGHELPER_H

#include <windows.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

int GetSettingInt(HKEY hkey, LPCTSTR valueName, int defaultValue);
BOOL GetSettingBinary(HKEY hkey, LPCTSTR valueName, void *buffer, DWORD bufferSize);
BOOL GetSettingStr(HKEY hkey, LPCTSTR valueName, LPCTSTR defaultValue, LPTSTR buffer, DWORD cchBuffer);

BOOL WriteSettingInt(HKEY hkey, LPCTSTR valueName, int value);
BOOL WriteSettingBinary(HKEY hkey, LPCTSTR valueName, const void *buffer, DWORD bufferSize);
BOOL WriteSettingStr(HKEY hkey, LPCTSTR valueName, LPCTSTR value);

#ifdef __cplusplus
}
#endif

#endif
