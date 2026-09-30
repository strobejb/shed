#ifndef CATCH22_TRACE_H
#define CATCH22_TRACE_H

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

static void Catch22Trace(const char *format, ...)
{
    char buffer[512];
    va_list args;

    va_start(args, format);
    _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    va_end(args);

    OutputDebugStringA(buffer);
}

#ifdef __cplusplus
}
#endif

#ifdef _DEBUG
#define TRACE Catch22Trace
#else
#define TRACE __noop
#endif

#endif
