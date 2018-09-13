/******************************************************************************

  Source File:  deskmon.h

  General include file

  Copyright (c) 1997-1998 by Microsoft Corporation

  Change History:

  12-01-97 AndreVa - Created It

******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shsemip.h>
#include <stdlib.h>
#include <shlobjp.h>
#include <shellp.h>
#include <string.h>
#include <tchar.h>
#include <winuserp.h>
#include <cfgmgr32.h>

#include <initguid.h>
#include <help.h>
#include "..\..\common\deskcplext.h"
#include "..\..\common\propsext.h"
#include "..\..\common\deskcmmn.h"
#include "resource.h"


//
// Avoid bringing in C runtime code for NO reason
// 
#if defined(__cplusplus)
inline void * __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus


class CMonitorPage
{
public:
    // Constructors / destructor
    CMonitorPage(HWND hDlg);
    ~CMonitorPage();

    // Message handlers
    void OnInitDialog();
    void OnDestroy();
    void OnApply();
    void OnCancel();
    void OnProperties();
    BOOL OnSetActive();
    void OnSelMonitorChanged();
    void OnFrequencyChanged();
    void OnPruningModeChanged();

#ifdef DBG
    void AssertValid() const;
#endif

private:
    // Helpers
    void InitPruningMode();
    void SaveMonitorInstancePath(DEVINST devInstAdapter, LPCTSTR pMonitorID, int nNewItem);
    void RefreshFrequenciesList();

    // Data members
    HWND       m_hDlg;
    LPDEVMODEW m_lpdmPrevious;
    BOOL       m_bCanBePruned;          // true if the raw modes list != pruned modes list
    BOOL       m_bIsPruningReadOnly;    // false if can be pruned and we can write the pruning mode
    BOOL       m_bIsPruningOn;          // non null if pruning mode is on
    int        m_cMonitors;
    HWND       m_hMonitorsList;
    LPDEVMODEW m_lpdmOnCancel;          // device mode to be restored on cancel
    BOOL       m_bOnCancelIsPruningOn;  // pruning mode to be restored on cancel
};

