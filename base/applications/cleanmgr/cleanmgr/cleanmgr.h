/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Main header file
 * COPYRIGHT:   Copyright 2023-2025 Mark Jansen <mark.jansen@reactos.org>
 */

#pragma once

#ifndef STRICT
#define STRICT
#endif

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#define _FORCENAMELESSUNION

#include <ndk/rtlfuncs.h>
#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <shlwapi.h>


#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <strsafe.h>
#include <emptyvc.h>
#include <atlcoll.h>


using namespace ATL;

#define NDEBUG
#include <reactos/debug.h>
#include <reactos/shellutils.h>
#include <ui/rosdlgs.h>


template <class T> class CLocalPtr
    : public CHeapPtr<T, CLocalAllocator>
{
public:
    CLocalPtr() throw()
    {
    }

    explicit CLocalPtr(_In_ T* pData) throw() :
        CHeapPtr<T, CLocalAllocator>(pData)
    {
    }
};

#include "resource.h"
#include "CProgressDlg.hpp"
#include "CCleanupHandler.hpp"
#include "CCleanupHandlerList.hpp"
#include "CEmptyVolumeCacheCallBack.hpp"

// CSelectDriveDlg.cpp
void
SelectDrive(WCHAR &Drive);
