#ifndef __SHCOMPUI_DEBUG_H
#define __SHCOMPUI_DEBUG_H
///////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  FILE: DEBUG.H
//
//  DESCRIPTION:
//
//    Header for debug support in SHCOMPUI.DLL.
//
//
//    REVISIONS:
//
//    Date       Description                                         Programmer
//    ---------- --------------------------------------------------- ----------
//    09/15/95   Initial creation.                                   brianau
//
///////////////////////////////////////////////////////////////////////////////
#ifdef ASSERT
#   undef ASSERT
#endif

#if defined(DEBUG) || defined(DBG)

#include <windows.h>
#include <tchar.h>

void WINAPI AssertFailed(LPCTSTR szFile, int line);

#ifdef UNICODE
#define ASSERT(f)                                 \
    {                                             \
        if (!(f)) {                               \
            TCHAR szFile[MAX_PATH];               \
            MultiByteToWideChar(CP_ACP,0,__FILE__,-1,szFile,MAX_PATH); \
            AssertFailed(szFile, __LINE__);       \
        }                                         \
    }
#else
#define ASSERT(f)                                 \
    {                                             \
        if (!(f))                                 \
            AssertFailed((LPCTSTR)__FILE__, __LINE__);       \
    }
#endif
#else

#define ASSERT(f)   (NULL)  // No action.

#endif

void DbgOut(LPCTSTR fmt, ...);

#endif


