#ifndef __FSUTIL_H__
#define __FSUTIL_H__

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

typedef struct
{
    int (*Handler)(int argc, const TCHAR *argv[]);
    const TCHAR * Command;
    const TCHAR * Desc;
} HandlerItem;

#endif
