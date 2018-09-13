//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: brfcase.c
//
//  This file contains the briefcase-style file synchronization code
// made integral with the shell.
//
// History:
//  08-03-93 ScottH     Created
//  01-27-94 ScottH     Changed for OLE 2-style and moniker ID list
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop

#include <limits.h>
#include <brfcasep.h>
#include "fstreex.h"
#include "brfcase.h"
#include "datautil.h"
#include "prop.h"

// forward decl.
STDMETHODIMP Briefcase_GetDetailsOf(BRFEXP * pbrfexp, IBriefcaseStg *pbrfstg,
    HWND hwnd, HANDLE hMutexDelay,
    LPCITEMIDLIST pidl, UINT iColumn, 
    SHELLDETAILS *pDetails );
    
HRESULT BrfStg_CreateInstance(LPCITEMIDLIST pidl, HWND hwnd, void **ppvOut)
{
    HRESULT hres = E_OUTOFMEMORY;
    TCHAR szFolder[MAX_PATH];
    
    // Create an instance of IBriefcaseStg
    if (SHGetPathFromIDList(pidl, szFolder))
    {
        IBriefcaseStg *pbrfstg;
        hres = CoCreateInstance(&CLSID_Briefcase, NULL, CLSCTX_INPROC_SERVER, &IID_IBriefcaseStg, &pbrfstg);
        if (SUCCEEDED(hres))
        {
            hres = pbrfstg->lpVtbl->Initialize(pbrfstg, szFolder, hwnd);
            if (SUCCEEDED(hres))
            {
                hres = pbrfstg->lpVtbl->QueryInterface(pbrfstg, &IID_IBriefcaseStg, ppvOut);
            }
            pbrfstg->lpVtbl->Release(pbrfstg);
        }
    }
    return hres;        // S_OK or E_NOINTERFACE
}



// CFSBrfIDLData

UINT g_cfBriefObj = 0;

UINT _GetBriefObjCF()
{
    if (g_cfBriefObj == 0)
    {
        g_cfBriefObj = RegisterClipboardFormat(CFSTR_BRIEFOBJECT);
    }

    return g_cfBriefObj;
}


/*
Purpose: Gets the root path of the briefcase storage and copies
it into the buffer.

  This function obtains the briefcase storage root by
  binding to an IShellFolder (briefcase) instance of the
  pidl.  This parent is be an CFSBrfFolder *, so we can
  call the IBriefcaseStg::GetExtraInfo member function.
  
    Returns: standard result
    Cond:    --
*/
HRESULT GetBriefcaseRoot(LPCITEMIDLIST pidl, LPTSTR pszBuf, int cchBuf)
{
    IBriefcaseStg *pbrfstg;
    HRESULT hres = BrfStg_CreateInstance(pidl, NULL, &pbrfstg);
    if (SUCCEEDED(hres))
    {
        hres = pbrfstg->lpVtbl->GetExtraInfo(pbrfstg, NULL, GEI_ROOT, (WPARAM)cchBuf, (LPARAM)pszBuf);
        pbrfstg->lpVtbl->Release(pbrfstg);
    }
    return hres;
}


// Creates an IDL of the root path of the briefcase storage.

BOOL ILGetBriefcaseRoot(IBriefcaseStg *pbrfstg, LPITEMIDLIST *ppidlRoot)
{
    BOOL bRet = FALSE;
    TCHAR sz[MAX_PATH];
    
    if (SUCCEEDED(pbrfstg->lpVtbl->GetExtraInfo(pbrfstg, NULL, GEI_ROOT, (WPARAM)ARRAYSIZE(sz), (LPARAM)sz)))
    {
        bRet = (NULL != (*ppidlRoot = ILCreateFromPath(sz)));
    }
    return bRet;
}


/*
Purpose: Packages a BriefObj struct into pmedium from a HIDA.

  Returns: standard
  Cond:    --
*/
HRESULT CFSBrfIDLData_GetBriefObj(IDataObject *pdtobj, LPSTGMEDIUM pmedium)
{
    HRESULT hres = E_OUTOFMEMORY;
    LPITEMIDLIST pidl = ILCreate();
    if (pidl)
    {
        STGMEDIUM medium;
        UINT cFiles, cbSize;
        PBRIEFOBJ pbo;
        
        if (DataObj_GetHIDA(pdtobj, &medium))
        {
            ASSERT(medium.hGlobal);
            
            cFiles = HIDA_GetCount(medium.hGlobal);
            // "cFiles+1" includes the briefpath...
            cbSize = SIZEOF(BriefObj) + MAX_PATH * SIZEOF(TCHAR) * (cFiles + 1)  + 1;
            
            pbo = GlobalAlloc(GPTR, cbSize);
            if (pbo)
            {
                LPITEMIDLIST pidlT;
                LPTSTR pszFiles = (LPTSTR)((LPBYTE)pbo + _IOffset(BriefObj, data));
                UINT i;
                
                pbo->cbSize = cbSize;
                pbo->cItems = cFiles;
                pbo->cbListSize = MAX_PATH*SIZEOF(TCHAR)*cFiles + 1;
                pbo->ibFileList = _IOffset(BriefObj, data);
                
                for (i = 0; i < cFiles; i++)
                {
                    pidlT = HIDA_FillIDList(medium.hGlobal, i, pidl);
                    if (NULL == pidlT)
                        break;      // out of memory
                    
                    pidl = pidlT;
                    SHGetPathFromIDList(pidl, pszFiles);
                    pszFiles += lstrlen(pszFiles)+1;
                }
                *pszFiles = TEXT('\0');
                
                if (i < cFiles)
                {
                    // Out of memory, fail
                    ASSERT(NULL == pidlT);
                }
                else
                {
                    // Make pszFiles point to beginning of szBriefPath buffer
                    pszFiles++;
                    pbo->ibBriefPath = (UINT) ((LPBYTE)pszFiles - (LPBYTE)pbo);
                    pidlT = HIDA_FillIDList(medium.hGlobal, 0, pidl);
                    if (pidlT)
                    {
                        pidl = pidlT;
                        hres = GetBriefcaseRoot(pidl, pszFiles, MAX_PATH);
                        
                        pmedium->tymed = TYMED_HGLOBAL;
                        pmedium->hGlobal = pbo;
                        
                        // Indicate that the caller should release hmem.
                        pmedium->pUnkForRelease = NULL;
                    }
                }
            }
            
            HIDA_ReleaseStgMedium(NULL, &medium);
        }
        ILFree(pidl);
    }
    return hres;
}


// IDataObject::GetData

STDMETHODIMP CFSBrfIDLData_GetData(IDataObject *pdtobj,
                                   LPFORMATETC pformatetcIn,
                                   LPSTGMEDIUM pmedium)        // put data in here
{
    HRESULT hres = E_INVALIDARG;
    
    if (pformatetcIn->cfFormat == _GetBriefObjCF() && (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        hres = CFSBrfIDLData_GetBriefObj(pdtobj, pmedium);
    }
    else
    {
        hres = CFSIDLData_GetData(pdtobj, pformatetcIn, pmedium);
    }
    
    return hres;
}


// IDataObject::QueryGetData

STDMETHODIMP CFSBrfIDLData_QueryGetData(IDataObject *pdtobj,
                                        LPFORMATETC pformatetc)
{
    if (pformatetc->cfFormat == _GetBriefObjCF() && (pformatetc->tymed & TYMED_HGLOBAL))
        return NOERROR;
    
    return CFSIDLData_QueryGetData(pdtobj, pformatetc);
}


const IDataObjectVtbl c_CFSBrfIDLDataVtbl = {
    CIDLData_QueryInterface, CIDLData_AddRef, CIDLData_Release,
    CFSBrfIDLData_GetData,              // special member function
    CIDLData_GetDataHere,
    CFSBrfIDLData_QueryGetData,         // special member function
    CIDLData_GetCanonicalFormatEtc,
    CIDLData_SetData,
    CIDLData_EnumFormatEtc,
    CIDLData_Advise,
    CIDLData_Unadvise,
    CIDLData_EnumAdvise
};

enum
{
    ICOL_BRIEFCASE_NAME = 0,
    ICOL_BRIEFCASE_ORIGIN,
    ICOL_BRIEFCASE_STATUS,
    ICOL_BRIEFCASE_SIZE,
    ICOL_BRIEFCASE_TYPE,
    ICOL_BRIEFCASE_MODIFIED,
};

const COL_DATA s_briefcase_cols[] = {
    {ICOL_BRIEFCASE_NAME,       IDS_NAME_COL,       20,     LVCFMT_LEFT, &SCID_NAME},
    {ICOL_BRIEFCASE_ORIGIN,     IDS_ORIGINAL_COL,   24,     LVCFMT_LEFT, &SCID_ORIGINALLOCATION},
    {ICOL_BRIEFCASE_STATUS,     IDS_STATUS_COL,     18,     LVCFMT_LEFT, &SCID_STATUS},
    {ICOL_BRIEFCASE_SIZE,       IDS_SIZE_COL,        8,     LVCFMT_LEFT, &SCID_SIZE},
    {ICOL_BRIEFCASE_TYPE,       IDS_TYPE_COL,       18,     LVCFMT_LEFT, &SCID_TYPE},
    {ICOL_BRIEFCASE_MODIFIED,   IDS_MODIFIED_COL,   18,     LVCFMT_LEFT, &SCID_WRITETIME}
};


#define MAX_NAME    32

#define HACK_IGNORETYPE     0x04000000

TCHAR g_szDetailsUnknown[MAX_NAME] = TEXT("");

typedef struct
{
    TCHAR    szOrigin[MAX_PATH];
    TCHAR    szStatus[MAX_NAME];
    BOOL    bDetermined:1;
    BOOL    bUpToDate:1;
    BOOL    bDeleted:1;
} BRFINFO, * PBRFINFO;

typedef struct
{
    LPITEMIDLIST    pidl;       // Indexed value
    BRFINFO         bi;
} BRFINFOHDR, * PBRFINFOHDR;


// Values for BrfExp_FindNextState
#define FNS_UNDETERMINED   1
#define FNS_STALE          2
#define FNS_DELETED        3

#ifdef DEBUG

void BrfExp_EnterCS(PBRFEXP this);
void BrfExp_LeaveCS(PBRFEXP this);
#define BrfExp_AssertInCS(this)     ASSERT(0 < (this)->cCSRef)
#define BrfExp_AssertNotInCS(this)  ASSERT(0 == (this)->cCSRef)

#else

#define BrfExp_EnterCS(this)    EnterCriticalSection(&(this)->cs)
#define BrfExp_LeaveCS(this)    LeaveCriticalSection(&(this)->cs)
#define BrfExp_AssertInCS(this)
#define BrfExp_AssertNotInCS(this)

#endif

DWORD CALLBACK BrfExp_CalcThread(PBRFEXP this);


#ifdef DEBUG
void BrfExp_EnterCS(PBRFEXP this)
{
    EnterCriticalSection(&this->cs);
    this->cCSRef++;
}

void BrfExp_LeaveCS(PBRFEXP this)
{
    BrfExp_AssertInCS(this);
    
    this->cCSRef--;
    LeaveCriticalSection(&this->cs);
}
#endif


//---------------------------------------------------------------------------
// Brfview functions:    Expensive cache stuff
//---------------------------------------------------------------------------


/*
Purpose: Comparison function for the DPA list

  Returns: standard
  Cond:    --
*/
int CALLBACK BrfExp_CompareIDs(void *pv1, void *pv2, LPARAM lParam)
{
    IShellFolder *psf = ((PBRFEXP)lParam)->psf;
    PBRFINFOHDR pbihdr1 = (PBRFINFOHDR)pv1;
    PBRFINFOHDR pbihdr2 = (PBRFINFOHDR)pv2;
    HRESULT hres = psf->lpVtbl->CompareIDs(psf, HACK_IGNORETYPE,
        pbihdr1->pidl, pbihdr2->pidl);
    
    ASSERT(SUCCEEDED(hres));
    return (short)SCODE_CODE(GetScode(hres));   // (the short cast is important!)
}


/*
Purpose: Create the secondary thread for the expensive cache

  Returns: TRUE on success
  Cond:    --
*/
BOOL BrfExp_CreateThread(PBRFEXP this)
{
    BOOL bRet = FALSE;
    DWORD idThread;
    
    // The semaphore is used to determine whether anything
    // needs to be refreshed in the cache.
    this->hSemPending = CreateSemaphore(NULL, 0, INT_MAX, NULL);
    if (this->hSemPending)
    {
#ifdef DEBUG
        this->cStale = 0;
        this->cUndetermined = 0;
        this->cDeleted = 0;
#endif
        
        ASSERT(NULL == this->hEventDie);
        
        this->hEventDie = CreateEvent(NULL, FALSE, FALSE, NULL);
        
        if (this->hEventDie)
        {
            // Create the thread that will calculate expensive data
            this->hThreadPaint = CreateThread(NULL, 0, (LPVOID)BrfExp_CalcThread,
                this, CREATE_SUSPENDED, &idThread);
            if (this->hThreadPaint)
            {
                ResumeThread(this->hThreadPaint);
                bRet = TRUE;
            }
            else
            {
                CloseHandle(this->hEventDie);
                this->hEventDie = NULL;
                
                CloseHandle(this->hSemPending);
                this->hSemPending = NULL;
            }
        }
        else
        {
            CloseHandle(this->hSemPending);
            this->hSemPending = NULL;
        }
    }
    
    return bRet;
}


/*
Purpose: Initialize the cache

  Returns: TRUE on success
  Cond:    --
*/
BOOL BrfExp_Init(PBRFEXP this, IBriefcaseStg *pbrfstg, HWND hwndMain, HANDLE hMutexDelay)
{
    BOOL bRet = FALSE;
    
    ASSERT(pbrfstg);
    
#ifndef WINNT
    // The critical section should have been initialized when this
    // IShellView's IShellFolder interface was created.
    ReinitializeCriticalSection(&this->cs);
#endif
    
    BrfExp_EnterCS(this);
    {
        if (this->hdpa)
        {
            bRet = TRUE;
        }
        else
        {
            LoadString(HINST_THISDLL, IDS_DETAILSUNKNOWN, g_szDetailsUnknown, SIZEOF(g_szDetailsUnknown));
            
            this->hwndMain = hwndMain;
            this->hMutexDelay = hMutexDelay;
            this->idpaStaleCur = 0;
            this->idpaUndeterminedCur = 0;
            this->idpaDeletedCur = 0;
            
            this->hdpa = DPA_Create(8);
            if (this->hdpa)
            {
                bRet = BrfExp_CreateThread(this);
                
                if (bRet)
                {
                    this->pbrfstg = pbrfstg;
                    pbrfstg->lpVtbl->AddRef(pbrfstg);
                }
                else
                {
                    // Failed
                    DPA_Destroy(this->hdpa);
                    this->hdpa = NULL;
                }
            }
        }
    }
    BrfExp_LeaveCS(this);
    
    return bRet;
}


/*
Purpose: Clean up the cache of expensive data
Returns: --

  
    Cond:    IMPORTANT!!  The caller must guarantee it is not
    holding the BrfExp critical section before calling
    otherwise we can deadlock during MsgWaitObjectsSendMessage below.
    
*/
void BrfExp_Free(PBRFEXP this)
{
    BrfExp_EnterCS(this);
    {
        if (this->hEventDie)
        {
            if (this->hThreadPaint)
            {
                HANDLE hThread = this->hThreadPaint;
                
                SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
                
                // Signal the secondary thread to end
                SetEvent(this->hEventDie);
                
                // Make sure we are not in the critical section when
                // we wait for the secondary thread to exit.  Without
                // this check, hitting F5 twice in a row could deadlock.
                BrfExp_LeaveCS(this);
                {
                    // Wait for the threads to exit
                    BrfExp_AssertNotInCS(this);
                    
                    WaitForSendMessageThread(hThread, INFINITE);
                }
                BrfExp_EnterCS(this);
                
                DebugMsg(DM_TRACE, TEXT("Briefcase Secondary thread ended"));
                
                CloseHandle(this->hThreadPaint);
                this->hThreadPaint = NULL;
            }
            
            CloseHandle(this->hEventDie);
            this->hEventDie = NULL;
        }
        
        if (this->hdpa)
        {
            int idpa = DPA_GetPtrCount(this->hdpa);
            while (--idpa >= 0)
            {
                PBRFINFOHDR pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(this->hdpa, idpa);
                ILFree(pbihdr->pidl);
                LocalFree((HLOCAL)pbihdr);
            }
            DPA_Destroy(this->hdpa);
            this->hdpa = NULL;
        }
        
        if (this->hSemPending)
        {
            CloseHandle(this->hSemPending);
            this->hSemPending = NULL;
        }
        
        if (this->pbrfstg)
        {
            this->pbrfstg->lpVtbl->Release(this->pbrfstg);
            this->pbrfstg = NULL;
        }
    }
    BrfExp_LeaveCS(this);
}


/*
Purpose: Resets the expensive data cache
Returns: --

  Cond:    IMPORTANT!!  The caller must guarantee it is not
  holding the BrfExp critical section before calling
  otherwise we can deadlock during MsgWaitObjectsSendMessage below.
  
*/
void BrfExp_Reset(PBRFEXP this)
{
    BrfExp_AssertNotInCS(this);
    
    BrfExp_EnterCS(this);
    {
        IBriefcaseStg *pbrfstg = this->pbrfstg;
        
        if (FALSE == this->bFreePending && pbrfstg)
        {
            HWND hwndMain = this->hwndMain;
            HANDLE hMutex = this->hMutexDelay;
            
            pbrfstg->lpVtbl->AddRef(pbrfstg);
            
            // Since we won't be in the critical section when we
            // wait for the paint thread to exit, set this flag to
            // avoid nasty re-entrant calls.
            this->bFreePending = TRUE;
            
            // Reset by freeing and reinitializing.
            BrfExp_LeaveCS(this);
            {
                BrfExp_Free(this);
                BrfExp_Init(this, pbrfstg, hwndMain, hMutex);
            }
            BrfExp_EnterCS(this);
            
            this->bFreePending = FALSE;
            
            pbrfstg->lpVtbl->Release(pbrfstg);
        }
    }
    BrfExp_LeaveCS(this);
}


/*
Purpose: Finds a cached name structure and returns a copy of
it in *pbi.

  Returns: TRUE if the pidl was found in the cache
  FALSE otherwise
  Cond:    --
*/
BOOL BrfExp_FindCachedName(PBRFEXP this, LPCITEMIDLIST pidl, PBRFINFO pbi)
{
    BOOL bRet = FALSE;
    
    ASSERT(pbi);
    
    BrfExp_EnterCS(this);
    {
        if (this->hdpa)
        {
            int idpa;
            BRFINFOHDR bihdrT;
            
            // Was the pidl found?
            bihdrT.pidl = ILFindLastID(pidl);
            idpa = DPA_Search(this->hdpa, (LPVOID)&bihdrT, 0, BrfExp_CompareIDs, (LPARAM)this, DPAS_SORTED);
            if (DPA_ERR != idpa)
            {
                // Yes
                PBRFINFOHDR pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(this->hdpa, idpa);
                ASSERT(pbihdr);
                
                *pbi = pbihdr->bi;
                bRet = TRUE;
            }
        }
    }
    BrfExp_LeaveCS(this);
    
    return bRet;
}


/*
Purpose: Deletes a cached name structure.

  Returns: TRUE if the pidl was found in the cache
  FALSE otherwise
  Cond:    --
*/
BOOL BrfExp_DeleteCachedName(PBRFEXP this, LPCITEMIDLIST pidl)
{
    BOOL bRet = FALSE;
    
    BrfExp_EnterCS(this);
    {
        if (this->hdpa)
        {
            int idpa;
            BRFINFOHDR bihdrT;
            
            // Was the pidl found?
            bihdrT.pidl = ILFindLastID(pidl);
            idpa = DPA_Search(this->hdpa, (LPVOID)&bihdrT, 0, BrfExp_CompareIDs, (LPARAM)this, DPAS_SORTED);
            if (DPA_ERR != idpa)
            {
                // Yes
#ifdef DEBUG
                PBRFINFOHDR pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(this->hdpa, idpa);
                ASSERT(pbihdr);
                
                this->cDeleted--;
                
                if (!pbihdr->bi.bDetermined)
                    this->cUndetermined--;
                else if (!pbihdr->bi.bUpToDate)
                    this->cStale--;
#endif
                
                // Keep index pointers current
                if (this->idpaStaleCur >= idpa)
                    this->idpaStaleCur--;
                if (this->idpaUndeterminedCur >= idpa)
                    this->idpaUndeterminedCur--;
                if (this->idpaDeletedCur >= idpa)
                    this->idpaDeletedCur--;
                
                DPA_DeletePtr(this->hdpa, idpa);
                bRet = TRUE;
            }
        }
    }
    BrfExp_LeaveCS(this);
    
    return bRet;
}


/*
Purpose: Finds the next cached name structure that matches the
requested state.

  Returns: TRUE if an item was found in the cache
  FALSE otherwise
  Cond:    --
*/
BOOL BrfExp_FindNextState(PBRFEXP this,
                          UINT uState,            // FNS_UNDETERMINED, FNS_STALE or FNS_DELETED
                          PBRFINFOHDR pbihdrOut)  // Structure with filled in values
{
    BOOL bRet = FALSE;
    
    ASSERT(pbihdrOut);
    
    BrfExp_EnterCS(this);
    {
        if (this->hdpa)
        {
            HDPA hdpa = this->hdpa;
            int idpaCur;
            int idpa;
            int cdpaMax;
            PBRFINFOHDR pbihdr;
            
            cdpaMax = DPA_GetPtrCount(hdpa);
            
            switch (uState)
            {
            case FNS_UNDETERMINED:
                // Iterate thru the entire list starting at idpa.  We roll this
                // loop out to be two loops: the first iterates the last portion
                // of the list, the second iterates the first portion if the former
                // failed to find anything.
                idpaCur = this->idpaUndeterminedCur + 1;
                for (idpa = idpaCur; idpa < cdpaMax; idpa++)
                {
                    pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(hdpa, idpa);
                    if (!pbihdr->bi.bDetermined)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(idpaCur <= cdpaMax);
                for (idpa = 0; idpa < idpaCur; idpa++)
                {
                    pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(hdpa, idpa);
                    if (!pbihdr->bi.bDetermined)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(0 == this->cUndetermined);
                break;
                
            case FNS_STALE:
                // Iterate thru the entire list starting at idpa.  We roll this
                // loop out to be two loops: the first iterates the last portion
                // of the list, the second iterates the first portion if the former
                // failed to find anything.
                idpaCur = this->idpaStaleCur + 1;
                for (idpa = idpaCur; idpa < cdpaMax; idpa++)
                {
                    pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(hdpa, idpa);
                    if (!pbihdr->bi.bUpToDate)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(idpaCur <= cdpaMax);
                for (idpa = 0; idpa < idpaCur; idpa++)
                {
                    pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(hdpa, idpa);
                    if (!pbihdr->bi.bUpToDate)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(0 == this->cStale);
                break;
                
            case FNS_DELETED:
                // Iterate thru the entire list starting at idpa.  We roll this
                // loop out to be two loops: the first iterates the last portion
                // of the list, the second iterates the first portion if the former
                // failed to find anything.
                idpaCur = this->idpaDeletedCur + 1;
                for (idpa = idpaCur; idpa < cdpaMax; idpa++)
                {
                    pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(hdpa, idpa);
                    if (pbihdr->bi.bDeleted)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(idpaCur <= cdpaMax);
                for (idpa = 0; idpa < idpaCur; idpa++)
                {
                    pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(hdpa, idpa);
                    if (pbihdr->bi.bDeleted)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(0 == this->cDeleted);
                break;
                
            default:
                ASSERT(0);      // should never get here
                break;
            }
            goto Done;
            
Found:
            ASSERT(0 <= idpa && idpa < cdpaMax);
            
            // Found the next item of the requested state
            switch (uState)
            {
            case FNS_UNDETERMINED:
                this->idpaUndeterminedCur = idpa;
                break;
                
            case FNS_STALE:
                this->idpaStaleCur = idpa;
                break;
                
            case FNS_DELETED:
                this->idpaDeletedCur = idpa;
                break;
            }
            
            *pbihdrOut = *pbihdr;
            pbihdrOut->pidl = ILClone(pbihdr->pidl);
            if (pbihdrOut->pidl)
                bRet = TRUE;
        }
Done:;
    }
    BrfExp_LeaveCS(this);
    return bRet;
}
    
    
/* Purpose: Recalculates a cached name structure.  This can be an expensive operation. */
void BrfExp_CalcCachedName(PBRFEXP this, LPCITEMIDLIST pidl, PBRFINFO pbi)
{
    BrfExp_EnterCS(this);
    
    if (this->hdpa && this->pbrfstg)
    {
        int idpa;
        BRFINFOHDR bihdrT;
        LPIDFOLDER pidf = (LPIDFOLDER)ILFindLastID(pidl);
        IBriefcaseStg *pbrfstg = this->pbrfstg;
        
        pbrfstg->lpVtbl->AddRef(pbrfstg);
        
        // Make sure we're out of the critical section when we call
        // the expensive functions!
        BrfExp_LeaveCS(this);
        {
            TCHAR szTmp[MAX_PATH];
            FS_CopyName(pidf, szTmp, MAX_PATH);
            
            pbrfstg->lpVtbl->GetExtraInfo(pbrfstg, szTmp,
                GEI_ORIGIN, (WPARAM)ARRAYSIZE(pbi->szOrigin), (LPARAM)pbi->szOrigin);
            
            pbrfstg->lpVtbl->GetExtraInfo(pbrfstg, szTmp,
                GEI_STATUS, (WPARAM)ARRAYSIZE(pbi->szStatus), (LPARAM)pbi->szStatus);
        }
        BrfExp_EnterCS(this);
        
        pbrfstg->lpVtbl->Release(pbrfstg);
        
        // Check again if we are valid
        if (this->hdpa)
        {
            // Is the pidl still around so we can update it?
            bihdrT.pidl = (LPITEMIDLIST)pidf;
            idpa = DPA_Search(this->hdpa, (LPVOID)&bihdrT, 0, BrfExp_CompareIDs, (LPARAM)this, DPAS_SORTED);
            if (DPA_ERR != idpa)
            {
                // Yes; update it
                PBRFINFOHDR pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(this->hdpa, idpa);
                
                ASSERT(!pbihdr->bi.bUpToDate || !pbihdr->bi.bDetermined)
                    
                    // This entry may have been marked for deletion while the
                    // expensive calculations were in process above.  Check for
                    // it now.
                    if (pbihdr->bi.bDeleted)
                    {
                        BrfExp_DeleteCachedName(this, pidl);
                    }
                    else
                    {
                        pbihdr->bi = *pbi;
                        pbihdr->bi.bUpToDate = TRUE;
                        pbihdr->bi.bDetermined = TRUE;
                        
#ifdef DEBUG
                        if (!pbi->bDetermined)
                            this->cUndetermined--;
                        else if (!pbi->bUpToDate)
                            this->cStale--;
                        else
                            ASSERT(0);
#endif
                    }
            }
        }
    }
    BrfExp_LeaveCS(this);
}

  
// Finds a cached name structure and marks it stale
void BrfExp_CachedNameIsStale(PBRFEXP this,
                              LPCITEMIDLIST pidl,
                              BOOL bDeleted)
{
    BrfExp_EnterCS(this);
    {
        if (this->hdpa)
        {
            int idpa;
            BRFINFOHDR bihdrT;
            
            // Was the pidl found?
            bihdrT.pidl = ILFindLastID(pidl);
            idpa = DPA_Search(this->hdpa, (LPVOID)&bihdrT, 0, BrfExp_CompareIDs, (LPARAM)this, DPAS_SORTED);
            if (DPA_ERR != idpa)
            {
                // Yes; mark it stale
                PBRFINFOHDR pbihdr = (PBRFINFOHDR)DPA_FastGetPtr(this->hdpa, idpa);
                ASSERT(pbihdr);
                
                // Is this cached name pending calculation yet?
                if (pbihdr->bi.bDetermined && pbihdr->bi.bUpToDate &&
                    !pbihdr->bi.bDeleted)
                {
                    // No; signal the calculation thread
                    if (bDeleted)
                    {
                        pbihdr->bi.bDeleted = TRUE;
#ifdef DEBUG
                        this->cDeleted++;
#endif
                    }
                    else
                    {
                        pbihdr->bi.bUpToDate = FALSE;
#ifdef DEBUG
                        this->cStale++;
#endif
                    }
                    
                    // Notify the calculating thread of an item that is pending
                    // calculation
                    ReleaseSemaphore(this->hSemPending, 1, NULL);
                }
                else if (bDeleted)
                {
                    // Yes; but mark for deletion anyway
                    pbihdr->bi.bDeleted = TRUE;
#ifdef DEBUG
                    this->cDeleted++;
#endif
                }
            }
        }
    }
    BrfExp_LeaveCS(this);
}
  
  
/*
Purpose: Marks all cached name structures stale

  Returns: --
  Cond:    --
*/
void BrfExp_AllNamesAreStale(PBRFEXP this)
{
    BrfExp_EnterCS(this);
    {
        if (this->pbrfstg)
        {
            UINT uFlags;
            
            // Dirty the briefcase storage cache
            this->pbrfstg->lpVtbl->Notify(this->pbrfstg, NULL, NOE_DIRTYALL, &uFlags, NULL);
        }
    }
    BrfExp_LeaveCS(this);
    
    // (It is important that we call BrfExp_Reset outside of the critical
    // section.  Otherwise, we can deadlock when this function is called
    // while the secondary thread is calculating (hit F5 twice in a row).)
    
    // Clear the entire expensive data cache
    BrfExp_Reset(this);
}


/*
Purpose: Adds a new item with default values to the extra info list

  Returns: TRUE on success
  
    Cond:    --
*/
BOOL BrfExp_AddCachedName(
                          PBRFEXP this,
                          LPCITEMIDLIST pidl,
                          PBRFINFO pbi,
                          IBriefcaseStg *pbrfstg,
                          HWND hwndMain,
                          HANDLE hMutexDelay)
{
    BOOL bRet = FALSE;
    
    ASSERT(pbi);
    
    BrfExp_EnterCS(this);
    {
        if (this->hdpa || BrfExp_Init(this, pbrfstg, hwndMain, hMutexDelay))
        {
            PBRFINFOHDR pbihdr;
            
            ASSERT(this->hdpa);
            
            pbihdr = (void*)LocalAlloc(LPTR, SIZEOF(*pbihdr));
            if (pbihdr)
            {
                pbihdr->pidl = ILClone(ILFindLastID(pidl));
                if (pbihdr->pidl)
                {
                    int idpa = DPA_AppendPtr(this->hdpa, pbihdr);
                    if (DPA_ERR != idpa)
                    {
                        pbihdr->bi.bUpToDate = FALSE;
                        pbihdr->bi.bDetermined = FALSE;
                        pbihdr->bi.bDeleted = FALSE;
                        lstrcpy(pbihdr->bi.szOrigin, g_szDetailsUnknown);
                        lstrcpy(pbihdr->bi.szStatus, g_szDetailsUnknown);
                        
#ifdef DEBUG
                        this->cUndetermined++;
#endif
                        DPA_Sort(this->hdpa, BrfExp_CompareIDs, (LPARAM)this);
                        
                        // Notify the calculating thread of an item that is pending
                        // calculation
                        ReleaseSemaphore(this->hSemPending, 1, NULL);
                        
                        *pbi = pbihdr->bi;
                        bRet = TRUE;
                    }
                    else
                    {
                        // Failed. Cleanup
                        ILFree(pbihdr->pidl);
                        LocalFree((HLOCAL)pbihdr);
                    }
                }
                else
                {
                    // Failed.  Cleanup
                    LocalFree((HLOCAL)pbihdr);
                }
            }
        }
    }
    BrfExp_LeaveCS(this);
    
    return bRet;
}


/*
Purpose: Thread to process the expensive calculations for
details view.

  Returns: 0
  Cond:    --
*/
DWORD CALLBACK BrfExp_CalcThread(PBRFEXP this)
{
    BRFINFOHDR bihdr;
    HANDLE rghObjPending[2] = {this->hEventDie, this->hSemPending};
    HANDLE rghObjDelay[2] = {this->hEventDie, this->hMutexDelay};
    DWORD dwRet;
    
    DebugMsg(DM_TRACE, TEXT("Briefcase: Entering paint thread"));
    
    while (TRUE)
    {
        // Wait for an end event or for a job to do
        dwRet = WaitForMultipleObjects(ARRAYSIZE(rghObjPending), rghObjPending, FALSE, INFINITE);
        
        if (WAIT_OBJECT_0 == dwRet)
        {
            // Exit thread
            break;
        }
        else
        {
#ifdef DEBUG
            BrfExp_EnterCS(this);
            {
                ASSERT(0 < this->cUndetermined ||
                    0 < this->cStale ||
                    0 < this->cDeleted);
            }
            BrfExp_LeaveCS(this);
#endif
            
            // Now wait for an end event or for the delay-calculation mutex
            dwRet = WaitForMultipleObjects(ARRAYSIZE(rghObjDelay), rghObjDelay, FALSE, INFINITE);
            
            if (WAIT_OBJECT_0 == dwRet)
            {
                // Exit thread
                break;
            }
            else
            {
                // Address deleted entries first
                if (BrfExp_FindNextState(this, FNS_DELETED, &bihdr))
                {
                    BrfExp_DeleteCachedName(this, bihdr.pidl);
                    ILFree(bihdr.pidl);
                }
                // Calculate undetermined entries before stale entries
                // to fill the view as quickly as possible
                else if (BrfExp_FindNextState(this, FNS_UNDETERMINED, &bihdr) ||
                    BrfExp_FindNextState(this, FNS_STALE, &bihdr))
                {
                    BrfExp_CalcCachedName(this, bihdr.pidl, &bihdr.bi);
                    ShellFolderView_RefreshObject(this->hwndMain, &bihdr.pidl);
                    ILFree(bihdr.pidl);
                }
                else
                {
                    ASSERT(0);      // Should never get here
                }
                
                ReleaseMutex(this->hMutexDelay);
            }
        }
    }
    
    DebugMsg(DM_TRACE, TEXT("Briefcase: Exiting paint thread"));
    return 0;
}



//---------------------------------------------------------------------------
// BrfView functions:
//---------------------------------------------------------------------------



// Merge the briefcase menu with the defview's menu bar.
HRESULT BrfView_MergeMenu(PBRFVIEW this, LPQCMINFO pinfo)
{
    // Merge the briefcase menu onto the menu that CDefView created.
    if (pinfo->hmenu)
    {
        HMENU hmSync = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(POPUP_BRIEFCASE));
        if (hmSync)
        {
            Shell_MergeMenus(pinfo->hmenu, hmSync, pinfo->indexMenu,
                pinfo->idCmdFirst,
                pinfo->idCmdLast, MM_SUBMENUSHAVEIDS);
            DestroyMenu(hmSync);
        }
    }
    
    return NOERROR;
}


/*
Purpose: Get the data object of the root folder of this briefcase.

  Returns: standard hresult
  Cond:    --
*/
HRESULT BrfView_GetRootObject(PBRFVIEW this, HWND hwnd, IDataObject **ppdtobj)
{
    HRESULT hres = E_OUTOFMEMORY;
    LPITEMIDLIST pidl = ILClone(this->pidlRoot);
    if (pidl)
    {
        LPITEMIDLIST pidlLast = ILClone(ILFindLastID(pidl));
        if (pidlLast)
        {
            ILRemoveLastID(pidl);
            hres = CIDLData_CreateFromIDArray2(&c_CFSBrfIDLDataVtbl,
                pidl, 1, &pidlLast, (IDataObject **)ppdtobj);
            ILFree(pidlLast);
        }
        ILFree(pidl);
    }
    return hres;
}


HRESULT BrfView_GetSelectedObjects(IShellFolder *psf, HWND hwnd, IDataObject **ppdtobj)
{
    HRESULT hres = E_FAIL;
    LPCITEMIDLIST * apidl;
    UINT cidl = (UINT) ShellFolderView_GetSelectedObjects(hwnd, &apidl);
    if (cidl > 0 && apidl)
    {
        hres = psf->lpVtbl->GetUIObjectOf(psf, hwnd, cidl, apidl,
            &IID_IDataObject, 0, ppdtobj);
        // We are not supposed to free apidl
    }
    else if (-1 == cidl)
    {
        hres = E_OUTOFMEMORY;
    }
    
    if (SUCCEEDED(hres))
    {
        hres = ResultFromShort((SHORT)cidl);
    }
    
    return hres;
}

  
  /*
  Purpose: DVM_WINDOWCREATED handler
  Returns: --
  Cond:    --
  */
void BrfView_OnCreate(PBRFVIEW this, HWND hwndMain, HWND hwndView)
{
    SHChangeNotifyEntry fsne;

    BrfExp_Init(this->pbrfexp, this->pbrfstg, hwndMain, this->hMutexDelay);

    // Register an extra SHChangeNotifyRegister for our pidl to try to catch things
    // like UpdateDir 
    fsne.pidl = this->pidl;
    fsne.fRecursive = FALSE;
    this->uSCNRExtra = SHChangeNotifyRegister(hwndView, SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
                                             SHCNE_DISKEVENTS, WM_DSV_FSNOTIFY, 1, &fsne);
}
  
  
// DVM_QUERYFSNOTIFY handler
HRESULT BrfView_OnQueryFSNotify(PBRFVIEW this, SHChangeNotifyEntry * pfsne)
{
    // Register to receive global events
    pfsne->pidl = NULL;
    pfsne->fRecursive = TRUE;
    
    return NOERROR;
}


/*
Purpose: DVM_WINDOWDESTROY handler
Returns: --
Cond:    --
*/
void BrfView_OnDestroy(PBRFVIEW this, HWND hwndView)
{
    BrfExp_Free(this->pbrfexp);
    //need to release pbrfstg as well
    if(this->pbrfstg)
    {
        this->pbrfstg->lpVtbl->Release(this->pbrfstg);
        this->pbrfstg = NULL;
    }
    if (this->uSCNRExtra)
        SHChangeNotifyDeregister(this->uSCNRExtra);
}


/*
Purpose: WM_COMMAND handler

  Returns: NOERROR
  Cond:    --
*/
HRESULT BrfView_Command(PBRFVIEW this, IShellFolder *psf, HWND hwnd, UINT uID)
{
    IDataObject *pdtobj;
    
    switch (uID)
    {
    case FSIDM_UPDATEALL:
        // Update the entire briefcase
        
        if (SUCCEEDED(BrfView_GetRootObject(this, hwnd, &pdtobj)))
        {
            this->pbrfstg->lpVtbl->UpdateObject(this->pbrfstg, pdtobj, hwnd);
            pdtobj->lpVtbl->Release(pdtobj);
        }
        break;
        
    case FSIDM_UPDATESELECTION:
        // Update the selected objects
        if (SUCCEEDED(BrfView_GetSelectedObjects(psf, hwnd, &pdtobj)))
        {
            this->pbrfstg->lpVtbl->UpdateObject(this->pbrfstg, pdtobj, hwnd);
            pdtobj->lpVtbl->Release(pdtobj);
        }
        break;
        
    case FSIDM_SPLIT:
        // Split the selected objects
        if (SUCCEEDED(BrfView_GetSelectedObjects(psf, hwnd, &pdtobj)))
        {
            this->pbrfstg->lpVtbl->ReleaseObject(this->pbrfstg, pdtobj, hwnd);
            pdtobj->lpVtbl->Release(pdtobj);
        }
        break;
    }
    
    return NOERROR;
}

  
// DVM_GETDETAILSOF handler

HRESULT BrfView_OnGetDetailsOf(PBRFVIEW this, HWND hwndMain, UINT iColumn, PDETAILSINFO lpDetails)
{
    return Briefcase_GetDetailsOf(this->pbrfexp, this->pbrfstg, hwndMain, this->hMutexDelay,
                                   lpDetails->pidl, iColumn, (SHELLDETAILS *)&lpDetails->fmt);
}

void BrfView_ForceRefresh(HWND hwndMain)
{
    IShellView *psv;
    IShellBrowser * psb = FileCabinet_GetIShellBrowser(hwndMain);
    if (psb && SUCCEEDED(psb->lpVtbl->QueryActiveShellView(psb, &psv)))
    {
        // Yes; refresh the window
        ASSERT(psv);
        
        DebugMsg(DM_TRACE, TEXT("Briefcase: Forced refresh of briefcase window"));
        
        psv->lpVtbl->Refresh(psv);
        psv->lpVtbl->Release(psv);
    }
}


/*
Purpose: Secondary DVM_FSNOTIFY handler

Returns: S_OK (NOERROR) to forward the event onto the defview window proc
         S_FALSE to retain the event from the defview
Cond:    --
*/
HRESULT BrfView_HandleFSNotifyForDefView(PBRFVIEW this, HWND hwndMain,
                                         LONG lEvent, LPCITEMIDLIST * ppidl, LPTSTR pszBuf)
{
    HRESULT hres;
    
    switch (lEvent)
    {
    case SHCNE_RENAMEITEM:
    case SHCNE_RENAMEFOLDER:
        if (!ILIsParent(this->pidl, ppidl[0], TRUE))
        {
            // move to this folder
            hres = BrfView_HandleFSNotifyForDefView(this, hwndMain, SHCNE_CREATE, &ppidl[1], pszBuf);
        }
        else if (!ILIsParent(this->pidl, ppidl[1], TRUE))
        {
            // move from this folder
            hres = BrfView_HandleFSNotifyForDefView(this, hwndMain, SHCNE_DELETE, &ppidl[0], pszBuf);
        }
        else
        {
            // have the defview handle it
            BrfExp_CachedNameIsStale(this->pbrfexp, ppidl[0], TRUE);
            hres = NOERROR;
        }
        break;
        
    case SHCNE_DELETE:
    case SHCNE_RMDIR:
        BrfExp_CachedNameIsStale(this->pbrfexp, ppidl[0], TRUE);
        hres = NOERROR;
        break;
        
    default:
        hres = NOERROR;
        break;
    }
    
    return hres;
}


/*
Purpose: Converts a shell change notify event to a briefcase
         storage event.

Returns: see above
Cond:    --
*/
LONG NOEFromSHCNE( LONG lEvent)
{
    switch (lEvent)
    {
    case SHCNE_RENAMEITEM:      return NOE_RENAME;
    case SHCNE_RENAMEFOLDER:    return NOE_RENAMEFOLDER;
    case SHCNE_CREATE:          return NOE_CREATE;
    case SHCNE_MKDIR:           return NOE_CREATEFOLDER;
    case SHCNE_DELETE:          return NOE_DELETE;
    case SHCNE_RMDIR:           return NOE_DELETEFOLDER;
    case SHCNE_UPDATEITEM:      return NOE_DIRTY;
    case SHCNE_UPDATEDIR:       return NOE_DIRTYFOLDER;
    default:                    return 0;
    }
}


/*
Purpose: DVM_FSNOTIFY handler

Returns: S_OK (NOERROR) to forward the event onto the defview window proc
         S_FALSE to retain the event from the defview

Cond:    Note the briefcase receives global events
*/
HRESULT BrfView_OnFSNotify(PBRFVIEW this, HWND hwndMain, LONG lEvent, LPCITEMIDLIST * ppidl)
{
    HRESULT hres;
    TCHAR szPath[MAX_PATH*2];
    
    if (lEvent == SHCNE_UPDATEIMAGE || lEvent == SHCNE_FREESPACE)
    {
        return S_FALSE;
    }
    
    if (ppidl && SHGetPathFromIDList(ppidl[0], szPath))
    {
        UINT uFlags;
        LONG lEventNOE;
        
        if ((SHCNE_RENAMEFOLDER == lEvent) || (SHCNE_RENAMEITEM == lEvent))
        {
            ASSERT(ppidl[1]);
            ASSERT(ARRAYSIZE(szPath) >= lstrlen(szPath)*2);    // rough estimate
            
            // Tack the new name after the old name, separated by the null
            SHGetPathFromIDList(ppidl[1], &szPath[lstrlen(szPath)+1]);
        }
        
        // Tell the briefcase the path has potentially changed
        lEventNOE = NOEFromSHCNE(lEvent);
        this->pbrfstg->lpVtbl->Notify(this->pbrfstg, szPath, lEventNOE, &uFlags, hwndMain);
        
        // Was this item marked?
        if (uFlags & NF_ITEMMARKED)
        {
            // Yes; mark it stale in the expensive cache
            BrfExp_CachedNameIsStale(this->pbrfexp, ppidl[0], FALSE);
        }
        
        // Does the window need to be refreshed?
        if (uFlags & NF_REDRAWWINDOW)
        {
            // Yes
            BrfView_ForceRefresh(hwndMain);
        }
        
        // Did this event occur in this folder?
        if (NULL == ppidl ||
            ILIsParent(this->pidl, ppidl[0], TRUE) ||
            (((SHCNE_RENAMEITEM == lEvent) || (SHCNE_RENAMEFOLDER == lEvent)) && ILIsParent(this->pidl, ppidl[1], TRUE)) ||
            (SHCNE_UPDATEDIR == lEvent && ILIsEqual(this->pidl, ppidl[0])))
        {
            // Yes; deal with it
            hres = BrfView_HandleFSNotifyForDefView(this, hwndMain, lEvent, ppidl, szPath);
        }
        else
        {
            // No
            hres = S_FALSE;
        }
    }
    else
    {
        ASSERT(0);
        hres = S_FALSE;
    }
    
    return hres;
}

/*
Purpose: DVM_NOTIFYCOPYHOOK handler

Returns: IDCANCEL to cancel the operation
         NOERROR to allow the operation

Cond:    --
*/
HRESULT BrfView_OnNotifyCopyHook(PBRFVIEW this, HWND hwndMain, const COPYHOOKINFO * pchi)
{
    HRESULT hres = NOERROR;
    
    DebugMsg(DM_TRACE, TEXT("Briefcase: BrfView_ViewCallBack: DVM_NOTIFYCOPYHOOK is sent (wFunc=%d, pszSrc=%s)"),
        pchi->wFunc, pchi->pszSrcFile);
    
    // Is this a pertinent operation?
    if (FO_MOVE == pchi->wFunc ||
        FO_RENAME == pchi->wFunc ||
        FO_DELETE == pchi->wFunc)
    {
        // Yes; don't allow the briefcase root or a parent folder to get moved
        // while the briefcase is still open.  (The database is locked while
        // the briefcase is open, and will fail the move/rename operation
        // in an ugly way.)
        LPITEMIDLIST pidl = ILCreateFromPath(pchi->pszSrcFile);
        if (pidl)
        {
            // Is the folder that is being moved or renamed a parent or equal
            // of the Briefcase root?
            if (ILIsParent(pidl, this->pidlRoot, FALSE) ||
                ILIsEqual(pidl, this->pidlRoot))
            {
                // Yes; don't allow it until the briefcase is closed.
                int ids;
                
                if (FO_MOVE == pchi->wFunc ||
                    FO_RENAME == pchi->wFunc)
                {
                    ids = IDS_MOVEBRIEFCASE;
                }
                else
                {
                    ASSERT(FO_DELETE == pchi->wFunc);
                    ids = IDS_DELETEBRIEFCASE;
                }
                
                ShellMessageBox(
                    HINST_THISDLL,
                    hwndMain,
                    MAKEINTRESOURCE(ids),
                    NULL,       // copy the title from the owner window.
                    MB_OK|MB_ICONINFORMATION);
                hres = IDCANCEL;
            }
            ILFree(pidl);
        }
    }
    return hres;
}


// DVM_INITMENUPOPUP handler
HRESULT BrfView_InitMenuPopup(PBRFVIEW this, HWND hwnd, UINT idCmdFirst, int nIndex, HMENU hmenu)
{
    BOOL bEnabled = ShellFolderView_GetSelectedCount(hwnd);
    EnableMenuItem(hmenu, idCmdFirst+FSIDM_UPDATESELECTION, bEnabled ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hmenu, idCmdFirst+FSIDM_SPLIT, bEnabled ? MF_ENABLED : MF_GRAYED);
    
    return NOERROR;
}


// DVM_INSERTITEM handler

HRESULT BrfView_OnInsertItem(PBRFVIEW this, HWND hwndMain, LPCITEMIDLIST pidl)
{
    HRESULT hres;
    TCHAR szPath[MAX_PATH];
    
    if (SHGetPathFromIDList(pidl, szPath))
    {
        // Always hide the desktop.ini and the database file.
        LPTSTR pszName = PathFindFileName(szPath);
        
        if (0 == lstrcmpi(pszName, c_szDesktopIni) ||
            0 == lstrcmpi(pszName, this->szDBName))
            hres = S_FALSE; // don't add
        else
            hres = S_OK;
    }
    else
        hres = S_OK;        // Let it be added...
    
    return hres;
}


// DVM_SELCHANGE handler

HRESULT BrfView_OnSelChange(PBRFVIEW this, HWND hwndMain, UINT idCmdFirst)
{
    IShellBrowser * psb = FileCabinet_GetIShellBrowser(hwndMain);
    if (psb)
    {
        psb->lpVtbl->SendControlMsg(psb, FCW_TOOLBAR, TB_ENABLEBUTTON,
            idCmdFirst + FSIDM_UPDATESELECTION,
            (LPARAM)(ShellFolderView_GetSelectedCount(hwndMain)), NULL);
    }
    return E_FAIL;     // (we did not update the status area)
}

// This function creates an instance of IShellView.
// The BrfView class does very little.  It aggregates to defviewx.c.

HRESULT BrfView_CreateInstance(IShellFolder2 *psf, LPCITEMIDLIST pidl,
                               PBRFEXP pbrfexp, HWND hwnd, void ** ppvOut)
{
    BrfView bv;
    HRESULT hres;

    *ppvOut = NULL;     // assume the worst

    // Create an instance of the briefcase storage BEFORE creating
    // the defview.  Otherwise the hMuteDelay will not be set
    // before the paint thread is created.
    hres = BrfStg_CreateInstance(pidl, hwnd, &bv.pbrfstg);
    if (SUCCEEDED(hres))
    {
        bv.pidl = pidl;
        bv.pbrfexp = pbrfexp;
        bv.uSCNRExtra = 0;

        // Get the global delay mutex
        bv.pbrfstg->lpVtbl->GetExtraInfo(bv.pbrfstg, NULL, GEI_DELAYHANDLE, 0, (LPARAM)&bv.hMutexDelay);
        // Get the database name
        bv.pbrfstg->lpVtbl->GetExtraInfo(bv.pbrfstg, NULL, GEI_DATABASENAME, ARRAYSIZE(bv.szDBName), (LPARAM)bv.szDBName);

        if (ILGetBriefcaseRoot(bv.pbrfstg, &bv.pidlRoot))
        {
            SFV_CREATE sSFV;

            sSFV.cbSize   = sizeof(sSFV);
            sSFV.pshf     = (IShellFolder *)psf;
            sSFV.psvOuter = NULL;
            sSFV.psfvcb   = BrfView_CreateSFVCB((IShellFolder *)psf, &bv);

            hres = SHCreateShellFolderView(&sSFV, (IShellView **)ppvOut);

            if (sSFV.psfvcb)
                sSFV.psfvcb->lpVtbl->Release(sSFV.psfvcb);

            if (FAILED(hres))
            {
                if (bv.pidlRoot)
                    ILFree(bv.pidlRoot);
            }
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }

        if (FAILED(hres))
            bv.pbrfstg->lpVtbl->Release(bv.pbrfstg);
    }

    ASSERT(FAILED(hres) ? (*ppvOut == NULL) : TRUE);

    return hres;        // NOERROR or E_NOINTERFACE
}

//===========================================================================
// CFSBrfFolder : class definition

typedef struct _CFSBrfFolder
{
    CFSFolder fsf;    // file system folder
    BRFEXP    brfexp; // extra goo
} CFSBrfFolder;

typedef struct
{
    IShellDetails sd;
    LONG cRef;
    HWND               hwndMain;
    CFSBrfFolder *     pbfsf;
    IBriefcaseStg *    pbrfstg;
    HANDLE             hMutexDelay;
} CFSBrfDetails;

STDMETHODIMP CFSBrfDetails_QueryInterface(IShellDetails *psd, REFIID riid, void **ppvObj)
{
    CFSBrfDetails *this = IToClass(CFSBrfDetails, sd, psd);

    if (IsEqualIID(riid, &IID_IShellDetails)  ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = &this->sd;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    this->cRef++;
    return NOERROR;
}

STDMETHODIMP_(ULONG) CFSBrfDetails_AddRef(IShellDetails *psd)
{
    CFSBrfDetails *this = IToClass(CFSBrfDetails, sd, psd);
    return InterlockedIncrement(&this->cRef);
}

STDMETHODIMP_(ULONG) CFSBrfDetails_Release(IShellDetails *psd)
{
    CFSBrfDetails *this = IToClass(CFSBrfDetails, sd, psd);
    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    if (this->pbrfstg)
        this->pbrfstg->lpVtbl->Release(this->pbrfstg);

    CFSFolder_Release(&this->pbfsf->fsf.sf);
    LocalFree((HLOCAL)this);
    return 0;
}

STDMETHODIMP CFSBrfDetails_GetDetailsOf(IShellDetails *psd, LPCITEMIDLIST pidl,
    UINT iColumn, SHELLDETAILS *pDetails)
{
    CFSBrfDetails *this = IToClass(CFSBrfDetails, sd, psd);

    // delay create the Briefcase storage....
    if ( this->pbrfstg == NULL )
    {
        HRESULT hr = BrfStg_CreateInstance( this->pbfsf->fsf._pidl, this->hwndMain, &this->pbrfstg );
        if ( FAILED( hr ))
            return hr;

        this->pbrfstg->lpVtbl->GetExtraInfo(this->pbrfstg, NULL, GEI_DELAYHANDLE, 0, (LPARAM)&this->hMutexDelay);
    }
    
    return Briefcase_GetDetailsOf( &this->pbfsf->brfexp, this->pbrfstg,
                                   this->hwndMain, this->hMutexDelay,
                                   pidl, iColumn, pDetails );
}

STDMETHODIMP Briefcase_GetDetailsOf(BRFEXP * pbrfexp, IBriefcaseStg *pbrfstg, 
    HWND hwnd, HANDLE hMutexDelay,
    LPCITEMIDLIST pidl, UINT iColumn, 
    SHELLDETAILS *pDetails )
{
    HRESULT hres = S_OK;
    LPIDFOLDER pidf = (LPIDFOLDER)pidl;
    TCHAR szTemp[MAX_PATH];

    pDetails->str.uType = STRRET_CSTR;
    pDetails->str.cStr[0] = 0;
    
    if (iColumn >= ARRAYSIZE(s_briefcase_cols))
        return E_NOTIMPL;
    
    if (!pidf)
    {
        pDetails->fmt = s_briefcase_cols[iColumn].iFmt;
        pDetails->cxChar = s_briefcase_cols[iColumn].cchCol;
        return ResToStrRet(s_briefcase_cols[iColumn].ids, &pDetails->str);
    }
    
    switch (iColumn)
    {
    case ICOL_BRIEFCASE_NAME:
        FS_CopyName(pidf, szTemp, ARRAYSIZE(szTemp));
        hres = StringToStrRet(szTemp, &pDetails->str);
        break;
        
    case ICOL_BRIEFCASE_ORIGIN:
    case ICOL_BRIEFCASE_STATUS: 
        {
            BRFINFO bi;
            LPTSTR lpsz;

            // Did we find extra info for this file or
            // was the new item added to the extra info list?
            if (BrfExp_FindCachedName(pbrfexp, pidl, &bi) ||
                BrfExp_AddCachedName(pbrfexp, pidl, &bi, pbrfstg,
                hwnd, hMutexDelay))
            {
                // Yes; take what's in there
                if (ICOL_BRIEFCASE_ORIGIN == iColumn)
                    lpsz =  bi.szOrigin;
                else
                {
                    ASSERT(ICOL_BRIEFCASE_STATUS == iColumn);
                    lpsz = bi.szStatus;
                }
                hres = StringToStrRet(lpsz, &pDetails->str);
            }
        }
        break;
        
    case ICOL_BRIEFCASE_SIZE:
        if (!FS_IsFolder(pidf))
        {
            ShortSizeFormat(pidf->fs.dwSize, szTemp);
            hres = StringToStrRet(szTemp, &pDetails->str);
        }
        break;
        
    case ICOL_BRIEFCASE_TYPE:
        FS_GetTypeName(pidf, szTemp, ARRAYSIZE(szTemp));
        hres = StringToStrRet(szTemp, &pDetails->str);
        break;
        
    case ICOL_BRIEFCASE_MODIFIED:
        DosTimeToDateTimeString(pidf->fs.dateModified, pidf->fs.timeModified, szTemp, ARRAYSIZE(szTemp), pDetails->fmt & LVCFMT_DIRECTION_MASK);
        hres = StringToStrRet(szTemp, &pDetails->str);
        break;
    }
    return hres;
}

STDMETHODIMP CFSBrfDetails_ColumnClick(IShellDetails *psd, UINT iColumn)
{
    // tell defview to do the work....
    return S_FALSE;
}

IShellDetailsVtbl c_FSBrfDetailVtbl =
{
    CFSBrfDetails_QueryInterface, CFSBrfDetails_AddRef, CFSBrfDetails_Release,
    CFSBrfDetails_GetDetailsOf,
    CFSBrfDetails_ColumnClick,
};


HRESULT CFSBrfDetails_Create(CFSBrfFolder *pbfsf, HWND hwndMain, void **ppvOut)
{
    CFSBrfDetails *psd = (void*)LocalAlloc(LPTR, SIZEOF(CFSBrfDetails));
    if (!psd)
        return E_OUTOFMEMORY;

    psd->sd.lpVtbl = &c_FSBrfDetailVtbl;
    psd->cRef = 1;

    psd->hwndMain = hwndMain;
    psd->pbfsf = pbfsf;
    psd->pbrfstg = NULL;

    ASSERT(psd->pbfsf->fsf.sf.lpVtbl->AddRef == CFSFolder_AddRef);
    CFSFolder_AddRef(&psd->pbfsf->fsf.sf);

    *ppvOut = psd;

    return S_OK;
}

/* Entry of "drop thread" for briefcase.

     The one special thing this does differently is
     call the IBriefcaseStg->AddObject to handle the
     copy case.
*/

DWORD CALLBACK BrfDropTargetThreadProc(void *pv)
{
    FSTHREADPARAM *pfsthp = (FSTHREADPARAM *)pv;
 
    // Is this a sync copy operation?
    if ((DROPEFFECT_COPY == pfsthp->dwEffect) && pfsthp->bSyncCopy)
    {
        // Yes; add the object to the briefcase storage and
        // let it handle the file operation
        if (pfsthp->pDataObj == NULL)
        {
            CoGetInterfaceAndReleaseStream(pfsthp->pstmDataObj, &IID_IDataObject, (void **)&pfsthp->pDataObj);
            pfsthp->pstmDataObj = NULL;
        }

        if (pfsthp->pDataObj)
        {
            IBriefcaseStg *pbrfstg;
        
            if (SUCCEEDED(BrfStg_CreateInstance(pfsthp->pidl, pfsthp->hwndOwner, &pbrfstg)))
            {
                UINT uFlags = (DDIDM_SYNCCOPYTYPE == pfsthp->idCmd) ? AOF_FILTERPROMPT : AOF_DEFAULT;
                pbrfstg->lpVtbl->AddObject(pbrfstg, pfsthp->pDataObj, NULL, uFlags, pfsthp->hwndOwner);
                pbrfstg->lpVtbl->Release(pbrfstg);
            }
            SHChangeNotifyHandleEvents();    // force update now
            DataObj_SetDWORD(pfsthp->pDataObj, g_cfPerformedDropEffect, pfsthp->dwEffect);
            DataObj_SetDWORD(pfsthp->pDataObj, g_cfLogicalPerformedDropEffect, pfsthp->dwEffect);
        }

        FreeFSThreadParam(pfsthp);
    }
    else
    {
        FileDropTargetThreadProc(pfsthp);
    }
    
    return 0;
}


/*
Purpose: Returns TRUE if the object is from the same briefcase
         as pidl.

Returns: see above
Cond:    --
*/
BOOL IsFromSameBriefcase(LPCTSTR pszBriefPath, LPCTSTR pszPath, LPCITEMIDLIST pidl)
{
    BOOL bRet;
    TCHAR szPathTgt[MAX_PATH];
    int cch;
    
    SHGetPathFromIDList(pidl, szPathTgt);
    cch = PathCommonPrefix(pszPath, szPathTgt, NULL);
    bRet = (0 < cch && lstrlen(pszBriefPath) <= cch);
    
    return bRet;
}


// TRUE if any folders are in hdrop

BOOL DroppingAnyFolders(HDROP hDrop)
{
    UINT i;
    TCHAR szPath[MAX_PATH];
    
    for (i = 0; DragQueryFile(hDrop, i, szPath, ARRAYSIZE(szPath)); i++)
    {
        if (PathIsDirectory(szPath))
            return TRUE;
    }
    
    return FALSE;
}


// Determines the correct operation based on the package being
// dropped into the briefcase.
// Returns: dwDefEffect
DWORD PickDefBriefOperation(CIDLDropTarget *pdroptgt, IDataObject *pdtobj,
                            DWORD grfKeyState, BOOL *pbSyncCopy,
                            UINT *pidMenu, DWORD *pdwEffect)
{
    HRESULT hres;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    DWORD dwDefEffect;
    BOOL bSyncCopy;
    UINT idMenu;
    
    // Are these objects file-system objects?
    hres = pdtobj->lpVtbl->QueryGetData(pdtobj, &fmte);
    if (S_OK == hres)
    {
        // Yes; are they from within a briefcase?
        STGMEDIUM medium;
        FORMATETC fmteBrief = {(CLIPFORMAT)_GetBriefObjCF(), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        
        if (S_OK == pdtobj->lpVtbl->QueryGetData(pdtobj, &fmteBrief))
        {
            // Yes; are they from the same briefcase as the target?
            if (SUCCEEDED(pdtobj->lpVtbl->GetData(pdtobj, &fmteBrief, &medium)))
            {
                PBRIEFOBJ pbo = (PBRIEFOBJ)GlobalLock(medium.hGlobal);
                TCHAR szBriefPath[MAX_PATH];
                TCHAR szPath[MAX_PATH];
                
                lstrcpy(szBriefPath, BOBriefcasePath(pbo));
                
                // Picking the first file will suffice.
                lstrcpy(szPath, BOFileList(pbo));
                
                GlobalUnlock(medium.hGlobal);
                
                if (IsFromSameBriefcase(szBriefPath, szPath, pdroptgt->pidl))
                {
                    // Yes; don't allow the user to create a sync copy.
                    // Just use the default NDD menu and commands
                    bSyncCopy = FALSE;
                    
                    *pdwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;
                    dwDefEffect = CFSIDLDropTarget_GetDefaultEffect(pdroptgt, grfKeyState, pdwEffect, NULL);
                    idMenu = POPUP_NONDEFAULTDD;
                }
                else
                {
                    // No; allow the sync copy
                    ReleaseStgMedium(&medium);
                    goto AllowSync;
                }
                
                ReleaseStgMedium(&medium);
            }
            else
            {
                // Failure case
                bSyncCopy = FALSE;
                
                *pdwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;
                dwDefEffect = CFSIDLDropTarget_GetDefaultEffect(pdroptgt, grfKeyState, pdwEffect, NULL);
                idMenu = POPUP_NONDEFAULTDD;
            }
        }
        else
        {
            // No; allow the sync copy
AllowSync:
            bSyncCopy = TRUE;
            *pdwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;
        
            // We pick the default operation with following logic:
            //  If CTRL key is down             -> "sync copy"
            //  else if SHIFT key is down       -> "move"
            //  else                            -> "sync copy"
            //
            //  (For the briefcase, it does not matter whether this is
            //  across volumes.)
            //
            if (grfKeyState & MK_CONTROL)
                dwDefEffect = DROPEFFECT_COPY;
            else if (grfKeyState & MK_SHIFT)
                dwDefEffect = DROPEFFECT_MOVE;
            else
                dwDefEffect = DROPEFFECT_COPY;
        
            if (SUCCEEDED(pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium)))
            {
                HDROP hDrop = GlobalLock(medium.hGlobal);
            
                // Are any folders being dropped?
                if (DroppingAnyFolders(hDrop))
                    idMenu = POPUP_BRIEFCASE_FOLDER_NONDEFAULTDD;   // Yes
                else
                    idMenu = POPUP_BRIEFCASE_NONDEFAULTDD;          // No
            
                GlobalUnlock(medium.hGlobal);
                ReleaseStgMedium(&medium);
            }
            else
            {
                idMenu = POPUP_NONDEFAULTDD;
            }
        }
    }
    else
    {
        // check to see if it is one of the other file types..
        BOOL fContents = ((pdroptgt->dwData & (DTID_CONTENTS | DTID_FDESCA)) == (DTID_CONTENTS | DTID_FDESCA) ||
            (pdroptgt->dwData & (DTID_CONTENTS | DTID_FDESCW)) == (DTID_CONTENTS | DTID_FDESCW));
        
        if (fContents || (pdroptgt->dwData & DTID_HIDA))
        {
            if (pdroptgt->dwData & DTID_HIDA)
            {
                dwDefEffect = DROPEFFECT_LINK;
            }
            
            if (fContents)
            {
                //
                // HACK: if there is a preferred drop effect and no HIDA
                // then just take the preferred effect as the available effects
                // this is because we didn't actually check the FD_LINKUI bit
                // back when we assembled dwData! (performance)
                //
                if ((pdroptgt->dwData & (DTID_PREFERREDEFFECT | DTID_HIDA)) ==
                    DTID_PREFERREDEFFECT)
                {
                    dwDefEffect = pdroptgt->dwEffectPreferred;
                }
                else if (pdroptgt->dwData & DTID_FD_LINKUI)
                {
                    dwDefEffect = DROPEFFECT_LINK;
                }
                else
                {
                    dwDefEffect = DROPEFFECT_COPY;
                }
                idMenu = POPUP_BRIEFCASE_FILECONTENTS;
            }
            bSyncCopy = TRUE;
        }
        else
        {
            // This used to allow Link even when nothing was on the clipboard
            // (or just text, for example).  The briefcase can't link to such things,
            // so I can't see why we would allow links.
            
            bSyncCopy = FALSE;
            *pdwEffect = DROPEFFECT_NONE;
            dwDefEffect = DROPEFFECT_NONE;
            idMenu = POPUP_NONDEFAULTDD;
        }
    }
    
    if (pbSyncCopy)
        *pbSyncCopy = bSyncCopy;
    
    if (pidMenu)
        *pidMenu = idMenu;
    
    return dwDefEffect;
}

// IDropTarget::DragEnter

STDMETHODIMP CFSBrfIDLDropTarget_DragEnter(IDropTarget *pdropt,
                                           IDataObject *pdtobj,
                                           DWORD grfKeyState,
                                           POINTL pt,
                                           DWORD *pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdropt);
    DWORD dwDefault;
    
    // Let the base-class process it first
    CIDLDropTarget_DragEnter(pdropt, pdtobj, grfKeyState, pt, pdwEffect);
    
    dwDefault = PickDefBriefOperation(this, pdtobj, grfKeyState,
        NULL, NULL, pdwEffect);
    
    // The cursor always indicates the default action.
    *pdwEffect = dwDefault;

    this->dwEffectLastReturned = *pdwEffect;
    
    return S_OK;
}


// IDropTarget::DragOver
STDMETHODIMP CFSBrfIDLDropTarget_DragOver(IDropTarget *pdropt,
                                          DWORD grfKeyState,
                                          POINTL pt,
                                          DWORD *pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdropt);
    
    if (this->grfKeyStateLast != grfKeyState)
    {
        DWORD dwDefault;
        
        this->grfKeyStateLast = grfKeyState;
        
        dwDefault = PickDefBriefOperation(this, this->pdtobj, grfKeyState,
            NULL, NULL, pdwEffect);
        
        // The cursor always indicates the default action.
        *pdwEffect = dwDefault;
        this->dwEffectLastReturned = *pdwEffect;
    }
    else
    {
        *pdwEffect = this->dwEffectLastReturned;
    }
    
    return NOERROR;
}

/*
Purpose: IDropTarget::Drop

         Handle drops into the briefcase folder.  Unlike normal
         file-system containers, a briefcase folder ALWAYS
         interprets a drop as a "synchronized copy" by default.
         In this case, we add the dropped object(s) to the
         briefcase storage and let the storage handle the action.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP CFSBrfIDLDropTarget_Drop(IDropTarget *pdropt, IDataObject *pdtobj,
                                      DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdropt);
    HRESULT hres;
    DWORD dwDefEffect;
    HKEY hkeyBaseProgID;
    HKEY hkeyProgID;
    UINT idMenu;
    BOOL bSyncCopy;
    DRAGDROPMENUPARAM ddm;

    IUnknown_Set((IUnknown **)&this->pdtobj, (IUnknown *)pdtobj);
    
    dwDefEffect = PickDefBriefOperation(this, pdtobj, grfKeyState, &bSyncCopy, &idMenu, pdwEffect);
    
    // Get the hkeyProgID and hkeyBaseProgID
    SHGetClassKey(this->pidl, &hkeyProgID, &hkeyBaseProgID);

    ddm.dwDefEffect = dwDefEffect;
    ddm.pdtobj = pdtobj;
    ddm.pt = pt;
    ddm.pdwEffect = pdwEffect;
    ddm.hkeyProgID = hkeyProgID;
    ddm.hkeyBase = hkeyBaseProgID;
    ddm.idMenu = idMenu;
    ddm.grfKeyState = grfKeyState;
    hres = CIDLDropTarget_DragDropMenuEx(this, &ddm);
    
    SHCloseClassKey(hkeyProgID);
    SHCloseClassKey(hkeyBaseProgID);
    
    // Continue with the operation (based on return value of DragDropMenu)?
    if (S_FALSE == hres)
    {
        if (idMenu == POPUP_BRIEFCASE_FILECONTENTS &&
            !((*pdwEffect & DROPEFFECT_LINK) && (this->dwData & DTID_HIDA)))
        {
            TCHAR szPath[MAX_PATH];
            BOOL fIsBkDropTarget = ShellFolderView_IsBkDropTarget(this->hwndOwner, &this->dropt);

            CIDLDropTarget_GetPath(this, szPath);

            hres = FS_CreateFileFromClip(this->hwndOwner, szPath, pdtobj, pt, pdwEffect, fIsBkDropTarget);
        }
        else
        {
            FSTHREADPARAM *pfsthp = (void*)LocalAlloc(LPTR, SIZEOF(*pfsthp));
            if (pfsthp)
            {
                CoMarshalInterThreadInterfaceInStream(&IID_IDataObject, (IUnknown *)pdtobj, &pfsthp->pstmDataObj);

                ASSERT(pfsthp->pDataObj == NULL);
                
                pfsthp->dwEffect = *pdwEffect;
                pfsthp->fSameHwnd = ShellFolderView_IsDropOnSource(this->hwndOwner, &this->dropt);
                pfsthp->bSyncCopy = bSyncCopy;
                pfsthp->idCmd = ddm.idCmd;
                pfsthp->hwndOwner = this->hwndOwner;
                pfsthp->grfKeyState = this->grfKeyStateLast;
                pfsthp->pidl = ILClone(this->pidl);
                CIDLDropTarget_GetPath(this, pfsthp->szPath);

                ShellFolderView_GetAnchorPoint(this->hwndOwner, FALSE, &pfsthp->ptDrop);
                
                if (DataObj_CanGoAsync(pdtobj))
                {
                    // create another thread to avoid blocking the source thread.
                    if (SHCreateThread(BrfDropTargetThreadProc, pfsthp, CTF_COINIT, NULL))
                    {
                        hres = NOERROR;
                    }
                    else
                    {
                        FreeFSThreadParam(pfsthp);
                        hres = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    BrfDropTargetThreadProc(pfsthp);    // do this on this thread
                }
            }
        }
    }
    
    CIDLDropTarget_DragLeave(pdropt);
    
    return hres;
}

const IDropTargetVtbl c_CFSBrfDropTargetVtbl =
{
    CIDLDropTarget_QueryInterface, CIDLDropTarget_AddRef, CIDLDropTarget_Release,
    CFSBrfIDLDropTarget_DragEnter,
    CFSBrfIDLDropTarget_DragOver,
    CIDLDropTarget_DragLeave,
    CFSBrfIDLDropTarget_Drop,           // special member function
};


// IShellFolder::Release

STDMETHODIMP CFSBrfFolderUnk_QueryInterface(IUnknown *punk, REFIID riid, void **ppv)
{
    CFSBrfFolder * this = IToClass(CFSBrfFolder, fsf.iunk, punk);
    
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = &this->fsf.iunk;
    }
    else if (IsEqualIID(riid, &IID_IShellFolder))
    {
        *ppv = &this->fsf.sf;
    }
    else if (IsEqualIID(riid, &IID_IPersist) ||
             IsEqualIID(riid, &IID_IPersistFolder) ||
             IsEqualIID(riid, &IID_IPersistFolder2) || 
             IsEqualIID(riid, &IID_IPersistFolder3))
    {
        *ppv = &this->fsf.pf;
    }
    else if (IsEqualIID(riid, &IID_INeedRealCFSFolder))
    {
        *ppv = this;     // return unreffed pointer
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    
    ((IUnknown*)(*ppv))->lpVtbl->AddRef(*ppv);
    return S_OK;
}

STDMETHODIMP_(ULONG) CFSBrfFolderUnk_Release(IUnknown *punk)
{
    CFSBrfFolder *this = IToClass(CFSBrfFolder, fsf.iunk, punk);

    if (InterlockedDecrement(&this->fsf.cRef))
        return this->fsf.cRef;

    CFSFolder_Reset(&this->fsf);

    DeleteCriticalSection(&this->brfexp.cs);
    
    LocalFree((HLOCAL)this);
    return 0;
}

// IShellFolder::CompareIDs
// BUGBUG: share more code with fstreex.c CFSFolder_CompareIDs()

STDMETHODIMP CFSBrfFolder_CompareIDs(IShellFolder2 *psf, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    CFSBrfFolder *this = IToClass(CFSBrfFolder, fsf.sf, psf);
    HRESULT hres;
    short nCmp;
    LPCIDFOLDER pidf1 = FS_IsValidID(pidl1);
    LPCIDFOLDER pidf2 = FS_IsValidID(pidl2);
    
    if (!pidf1 || !pidf2)
    {
        ASSERT(0);
        return E_INVALIDARG;
    }
    
    hres = FS_CompareFolderness(pidf1, pidf2);
    if (hres != ResultFromShort(0))
        return hres;
    
    switch (lParam & SHCIDS_COLUMNMASK)
    {
    case ICOL_BRIEFCASE_SIZE:
        if (pidf1->fs.dwSize < pidf2->fs.dwSize)
            return ResultFromShort(-1);
        if (pidf1->fs.dwSize > pidf2->fs.dwSize)
            return ResultFromShort(1);
        goto DoDefault;
        
    case ICOL_BRIEFCASE_TYPE:
        nCmp = _CompareFileTypes((IShellFolder *)psf, pidf1, pidf2);
        if (nCmp)
            return ResultFromShort(nCmp);
        goto DoDefault;
        
    case ICOL_BRIEFCASE_MODIFIED:
        hres = FS_CompareModifiedDate(pidf1, pidf2);
        if (!hres)
            goto DoDefault;
        break;
        
    case ICOL_BRIEFCASE_NAME:
        // We need to treat this differently from others bacause
        // pidf1/2 might not be simple.
        hres = FS_CompareNamesCase(pidf1, pidf2);
        
        // REVIEW: (Possible performance gain with some extra code)
        //   We should probably aviod bindings by walking down
        //  the IDList here instead of calling this helper function.
        //
        if (hres == ResultFromShort(0))
        {
            hres = ILCompareRelIDs((IShellFolder *)psf, pidl1, pidl2);
        }
        goto DoDefaultModification;
        
    case ICOL_BRIEFCASE_ORIGIN:
    case ICOL_BRIEFCASE_STATUS: 
        {
            BRFINFO bi1, bi2;
        
            BOOL bVal1 = BrfExp_FindCachedName(&this->brfexp, pidl1, &bi1);
            BOOL bVal2 = BrfExp_FindCachedName(&this->brfexp, pidl2, &bi2);
            // Do we have this info in our cache?
            if (!bVal1 || !bVal2)
            {
                // No; one or both of them are missing.  Have unknowns gravitate
                // to the bottom of the list.
                // (Don't bother adding them)
            
                if (!bVal1 && !bVal2)
                    hres = ResultFromShort(0);
                else if (!bVal1)
                    hres = ResultFromShort(1);
                else
                    hres = ResultFromShort(-1);
            }
            else
            {
                // Found the info; do a comparison
                if (ICOL_BRIEFCASE_ORIGIN == (lParam & SHCIDS_COLUMNMASK))
                {
                    hres = ResultFromShort(lstrcmp(bi1.szOrigin, bi2.szOrigin));
                }
                else
                {
                    ASSERT(ICOL_BRIEFCASE_STATUS == (lParam & SHCIDS_COLUMNMASK));
                    hres = ResultFromShort(lstrcmp(bi1.szStatus, bi2.szStatus));
                }
            }
        }
        break;
        
    default:
DoDefault:
        // Sort it based on the primary (long) name -- ignore case.
        {
            TCHAR szName1[MAX_PATH], szName2[MAX_PATH];

            FS_CopyName(pidf1, szName1, ARRAYSIZE(szName1));
            FS_CopyName(pidf2, szName2, ARRAYSIZE(szName2));

            hres = ResultFromShort(lstrcmpi(szName1, szName2));
        }

DoDefaultModification:
        if (hres == S_OK && (lParam & SHCIDS_ALLFIELDS)) {
            //
            // Must sort by modified date to pick up any file changes!
            //
            hres = FS_CompareModifiedDate(pidf1, pidf2);
            if (!hres)
                hres = FS_CompareAttribs(pidf1, pidf2);
        }
    }
    
    return hres;
}

// IShellFolder::CreateViewObject

STDMETHODIMP CFSBrfFolder_CreateViewObject(IShellFolder2 *psf, HWND hwnd, REFIID riid, void **ppvOut)
{
    CFSBrfFolder *this = IToClass(CFSBrfFolder, fsf.sf, psf);
    
    if (IsEqualIID(riid, &IID_IShellView))
    {
        // Use the default shell view
        return BrfView_CreateInstance(psf, this->fsf._pidl, &this->brfexp, hwnd, ppvOut);
    }
    else if (IsEqualIID(riid, &IID_IDropTarget))
    {
        // Create an IDropTarget interface instance with our
        // own vtable
        return CIDLDropTarget_Create(hwnd, &c_CFSBrfDropTargetVtbl,
            this->fsf._pidl, (IDropTarget **)ppvOut);
    }
    else if (IsEqualIID(riid, &IID_IContextMenu))
    {
        return CFSFolder_CreateViewObject(psf, hwnd, riid, ppvOut);
    }
    else if (IsEqualIID(riid, &IID_IShellDetails))
    {
        return CFSBrfDetails_Create(this, hwnd, ppvOut);
    }
    else
    {
        *ppvOut = NULL;
        return E_NOINTERFACE;
    }
}


// IShellFolder::GetAttributesOf

STDMETHODIMP CFSBrfFolder_GetAttributesOf(IShellFolder2 *psf,
                                          UINT cidl, LPCITEMIDLIST *apidl,
                                          ULONG * prgfInOut)
{
    CFSBrfFolder *this = IToClass(CFSBrfFolder, fsf.sf, psf);
    
    // Validate this pidl?
    if (*prgfInOut & SFGAO_VALIDATE)
    {
        // Yes; dirty the briefcase storage entry by sending an update
        // notification
        DebugMsg(DM_TRACE, TEXT("Briefcase: Receiving F5, dirty entire briefcase storage"));
        
        BrfExp_AllNamesAreStale(&this->brfexp);
        
        DebugMsg(DM_TRACE, TEXT("Briefcase: Finished staling everything"));
    }
    
    // Pass onto standard CFSFolder class member function
    return CFSFolder_GetAttributesOf(psf, cidl, apidl, prgfInOut);
}

// IShellFolder::GetUIObjectOf

STDMETHODIMP CFSBrfFolder_GetUIObjectOf(IShellFolder2 *psf, HWND hwnd, UINT cidl, 
                                        LPCITEMIDLIST * apidl, REFIID riid,
                                        UINT * prgfInOut, void **ppvOut)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);

    *ppvOut = NULL;

    if (IsEqualIID(riid, &IID_IExtractIconA) || 
        IsEqualIID(riid, &IID_IExtractIconW) ||
        IsEqualIID(riid, &IID_IContextMenu)  ||
        IsEqualIID(riid, &IID_IQueryAssociations)  ||
        IsEqualIID(riid, &IID_IDropTarget))
    {
        return CFSFolder_GetUIObjectOf(psf, hwnd, cidl, apidl, riid, prgfInOut, ppvOut);
    }
    else if (cidl > 0 && IsEqualIID(riid, &IID_IDataObject))
    {
        // Create an IDataObject interface instance with our
        // own vtable because we support the CFSTR_BRIEFOBJECT clipboard format
        return CIDLData_CreateFromIDArray2(&c_CFSBrfIDLDataVtbl, this->_pidl, cidl, apidl, (IDataObject **)ppvOut);
    }
    return E_INVALIDARG;
}

IShellFolder2Vtbl c_FSBrfFolderVtbl =
{
    CFSFolder_QueryInterface, CFSFolder_AddRef, CFSFolder_Release,
    CFSFolder_ParseDisplayName,
    CFSFolder_EnumObjects,
    CFSFolder_BindToObject,
    CFSFolder_BindToStorage,
    CFSBrfFolder_CompareIDs,
    CFSBrfFolder_CreateViewObject,      // special member function
    CFSBrfFolder_GetAttributesOf,
    CFSBrfFolder_GetUIObjectOf,         // special member function
    CFSFolder_GetDisplayNameOf,
    CFSFolder_SetNameOf,

    // NOTE: we don't support IShellFolder2 in our QI
    // CFSFolder_EnumSearches,
    FindFileOrFolders_GetDefaultSearchGUID,
};

const IUnknownVtbl c_FSBrfFolderUnkVtbl =
{
    CFSBrfFolderUnk_QueryInterface, CFSFolderUnk_AddRef, CFSBrfFolderUnk_Release,
};
  
extern IPersistFolder3Vtbl c_CFSFolderPFVtbl;

// CFSBrfFolder constructor


STDAPI CFSBrfFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    CFSBrfFolder *this = (void*)LocalAlloc(LPTR, SIZEOF(CFSBrfFolder));
    if (this)
    {
        HRESULT hres;
        this->fsf.iunk.lpVtbl = &c_FSBrfFolderUnkVtbl;
        this->fsf.sf.lpVtbl = &c_FSBrfFolderVtbl;
        this->fsf.pf.lpVtbl = &c_CFSFolderPFVtbl;
        this->fsf.punkOuter = &this->fsf.iunk;
        this->fsf.cRef = 1;
        this->fsf._clsidBind = CLSID_BriefcaseFolder;

        InitializeCriticalSection(&this->brfexp.cs);
        this->brfexp.psf = (IShellFolder *)&this->fsf.sf;
            
        hres = this->fsf.sf.lpVtbl->QueryInterface(&this->fsf.sf, riid, ppvOut);
        this->fsf.sf.lpVtbl->Release(&this->fsf.sf);

        return hres;
    }
    
    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}

// dead export, don't know if anyone calls this
STDAPI_(void) Desktop_UpdateBriefcaseOnEvent(HWND hwndMain, UINT uEvent)
{
}
