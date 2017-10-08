#ifndef __FSUTIL_H__
#define __FSUTIL_H__

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

typedef int (HandlerProc)(int argc, const TCHAR *argv[]);
typedef HandlerProc * pHandlerProc;

typedef struct
{
    pHandlerProc Handler;
    const TCHAR * Command;
    const TCHAR * Desc;
} HandlerItem;

int FindHandler(int argc,
                const TCHAR *argv[],
                HandlerItem * HandlersList,
                int HandlerListCount,
                void (*UsageHelper)(const TCHAR *));

HANDLE OpenVolume(const TCHAR * Volume,
                  BOOLEAN AllowRemote,
                  BOOLEAN NtfsOnly);

void PrintDefaultUsage(const TCHAR * Command,
                       const TCHAR * SubCommand,
                       HandlerItem * HandlersList,
                       int HandlerListCount);

int PrintErrorMessage(DWORD Error);

#endif
