
//  SH.CXX - Symbol Handler: Low level management
//
//      Copyright (C)1991-94, Microsoft Corporation
//
//      Purpose: Provide layer between SH functions and linked list manager.
//
//  Notes: Also included are fixup/unfixup functions until OSDEBUG is on-line.
//
//      DESCRIPTION OF INITIALIZATION CALLING SEQUENCE
//      ----------------------------------------------
//      During startup of debugging a user application, a large number of
//      Symbol Handler functions need to be called.  Here is the order in
//      which it makes sense to call them, and what they do:
//
//      (1) SHCreateProcess
//                  To create a handle for the new debuggee process which
//                  is being debugged.
//      (2) SHSetHpid
//                  This doesn't have to be called right now, but it should
//                  be called as soon as the HPID of the debuggee is known.
//      (3) SHAddDll
//                  Call this a number of times: once for the EXE which is
//                  being debugged, and once for each DLL being debugged
//                  (e.g., in CodeView, for all the /L xxxx.DLL files).
//                  This doesn't load the symbolic information off disk;
//                  it just lets the SH know that these files are being
//                  debugged.
//      (4) SHAddDllsToProcess
//                  This associates all added DLLs with the added EXE, so
//                  the SH knows that they are all being debugged as part
//                  of the same process.
//      (5) SHLoadDll
//                  Call this once for the EXE and each DLL, passing FALSE
//                  for the fLoading parameter.  This actually loads the
//                  symbolic information off disk.
//      (6) Start debuggee running
//      (7) SHLoadDll
//                  Call this for EXEs/DLLs as notifications are received
//                  indicating that they have been loaded into memory.
//                  This time, pass TRUE for the fLoading parameter.
//      (8) SHUnloadDll
//                  Call this for EXEs/DLLs as notifications are received
//                  indicating that they have been unloaded from memory.
//                  This does not actually unload the symbolic information.
//
//
//  Revision History:
//
//      07-Nov-94 BryanT
//          Merge in NT changes.  Changes include: removing PASCAL/LOADDS/FAR/NEAR
//          references.  Calls to STRxxx and MEMxxx functions (use strxxx/memxxx)
//          Replace calls to SHSplitPath with splitpath.  Remove non-WIN32 host
//          support.  Change EXS structure to EXE.  LLLock(hexe) yields a lpexe like
//          all other Handles do.  Eliminate need for EXR structure.  Consolidate
//          several static locals into global xxxCache variables so they can be
//          cleared when a process terminates (for multiprocess support).
//          Remove single user for SHHexeRemove (it can be done in place).
//          Add a vldChk arg to SHAddDllExt so one can pass a timestamp/checksum
//          to be used for image lookup.
//          Store just the name in exg.lszModule (SHAddDllExt).  The full path is
//          still stored in exg.lszName.  Needed for cmd window support.
//          Remove hlliMds references.  Rather than use the LL code, change
//          loadomf.cpp to keep an array with the index stored in each node
//          (no need to iterate to find, etc).
//          Define SHGetModule (used for ntsd syntax support)
//          Remove the dual global Exg lists and replace with a single one
//          Rewrite LoadDll to handle symbol sharing and multiple processes.
//          Add SHLszGetErrorText (to return the error message for an SH error code)
//          Add SHWantSymbols (To allow other parts of the debugger to indicate
//          symbols s/b loaded for a module).
//          Change SHChangeProcess to return the old HPDS.
//
//      [untagged] 21-Oct-93 MarkBro
//
//          Added SHUnloadSymbolHandler for NB10 notifications. Can
//          also be used in the future to free up memory so symbol
//          handler doesn't need to be free'd and reloaded.
//
//      [02] 05-Mar-93 DanS
//
//          Added critical section for win32 build.
//
//      [01] 31-dec-91 DavidGra
//
//          Fix bug with far addresses being unfixed up in the disassembler
//          when passed through the symbol handler.
//
//      [00] 11-dec-91 DavidGra
//
//          Make SHAddDll return an she indicating file not found or
//          out of memory or success.  Create SHAddDllExt to handle
//          the internal support and thunk it with SHAddDll to keep
//          the API the same.

#include "shinc.hpp"
#pragma hdrstop

HLLI    HlliPds;                // List of processes
HPDS    hpdsCur;                // Current process which is being debugged
HPID    hpidCurr;               // Current PID
MODCACHE    ModCache;           // Last module returned from SHHextFromHmod
LINECACHE   LineCache;          // Last Linenumber from SLFLineToAddr
HSFCACHE    HsfCache;           // Last Hsf returned from SLHmodFromHsf
CXTCACHE    CxtCache;           // Last Context info from SHSetCxtMod
SLCACHE     SlCache;            // Last Addr info from SLCAddrFromLine
ADDRCACHE   AddrCache;          // Last Line info from SLLineFromAddr

HLLI HlliExgExe;
//static char rgchFile [_MAX_CVPATH];
//static INT  fhCur = -1 ;

#define MATCH_NAMEONLY  1
#define MATCH_BACKGROUND 2
#define AllocAlign(cb) ( (PVOID) (((DWORD_PTR) MHAlloc(cb+1) + 1) & ~(DWORD_PTR)1 ))

//
// from windbg's math library
//
extern "C" {
ULARGE_INTEGER
strtouli ( const char *, char **, int );
}

// UNDONE: Investigate moving the cache data into the exg, exe, or mds struct.
//  VoidCaches
//
//  Purpose: Reinitialized the various caches to a known state

VOID
VoidCaches(
    VOID
    )
{
    memset(&LineCache, 0, sizeof(LineCache));
    memset(&AddrCache, 0, sizeof(AddrCache));
    memset(&ModCache, 0, sizeof(ModCache));
    memset(&HsfCache, 0, sizeof(HsfCache));
    memset(&CxtCache, 0, sizeof(CxtCache));
    if (SlCache.lpslp) {
        MHFree(SlCache.lpslp);
    }
    memset(&SlCache, 0, sizeof(SlCache));
    SlCache.cslp = -1;
}

//  SHCreateProcess
//
//  Purpose: Create a handle for a new debuggee process.  The debuggee
//           process doesn't actually have to be running yet; this is
//           just an abstract handle for the Symbol Handler's use, so
//           that it can keep track of symbols for multiple debuggee
//           processes at the same time.
//
//  Output:
//      Returns an HPDS, a handle to the new process, or 0 for failure.

HPDS
SHCreateProcess (
    VOID
    )
{
    HPDS hpds = SHFAddNewPds();
    SHChangeProcess(hpds);

    return hpds;
}

//  SHSetHpid
//
//  Purpose: Tell the SH what HPID to assign to the current process.
//           Each debuggee process has an HPID, and this call associates
//           an HPID with a HPDS in the SH.
//  Input:  hpid    The HPID to make current.

VOID
SHSetHpid (
    HPID hpid
    )
{
    LPPDS lppds = (LPPDS) LLLock(hpdsCur);

    lppds->hpid = hpidCurr = hpid;

    LLUnlock(hpdsCur);
}

//  SHDeleteProcess
//
//  Purpose: Delete a debuggee process handle (HPDS).  Removes it from
//          the SH's internal list of HPDS's.
//
//  Input:  hpds    The HPDS to delete.
//
//  Output:
//      TRUE for success, FALSE for failure.

BOOL
SHDeleteProcess (
    HPDS hpds
    )
{
    HPDS hpdsT = hpdsCur;
    HPID hpidT = hpidCurr;

    SHChangeProcess (hpds);

    LLDelete(HlliPds, hpdsCur);

    if (hpdsT != hpdsCur) {
        hpdsCur  = hpdsT;
        hpidCurr = hpidT;
    } else {
        hpdsCur = LLNext (HlliPds, NULL);
        if ( hpdsCur != 0 ) {
            LPPDS lppds = (LPPDS) LLLock (hpdsCur);
            hpidCurr = lppds->hpid;
            LLUnlock (hpdsCur);
        }
    }

    return TRUE;
}

//  SHChangeProcess
//
//  Purpose: Change the current debuggee process handle (HPDS).  The SH
//           can maintain symbols for multiple processes; this sets which
//           one is current, so that symbol lookups will be done on the
//           right set of symbolic information.
//
//  Input:  hpds    The HPDS to make current.

HPDS
SHChangeProcess(
    HPDS hpds
    )
{
    LPPDS lppds;
    HPDS  hpdsLast = hpdsCur;

    hpdsCur = hpds;

    lppds = (LPPDS) LLLock (hpdsCur);

    hpidCurr = lppds->hpid;

    LLUnlock (hpdsCur);
    return(hpdsLast);
}


BOOL
FInitLists (
    VOID
    )
{
    // Create the pds list
    HlliPds    = LLInit ( sizeof ( PDS ), 0, KillPdsNode, CmpPdsNode );
    HlliExgExe = LLInit ( sizeof ( EXG ), 0, KillExgNode, CmpExgNode );

    return HlliPds && HlliExgExe;
}

//  KillPdsNode
//
//  Purpose: Destroy private contents of a process node
//
//  Input:  pointer to node data
//
//  Notes: Only data in the pds structure to destroy is a list of exe's.

VOID
KillPdsNode (
    PVOID lpvPdsNode
    )
{
    LPPDS lppds = (LPPDS) lpvPdsNode;

    LLDestroy ( lppds->hlliExe );
}

int
CmpPdsNode (
    PVOID lpv1,
    PVOID lpv2,
    LONG lParam
    )
{
    LPPDS lppds   = (LPPDS) lpv1;
    HPID  *lphpid = (HPID *) lpv2;
    Unreferenced (lParam);

    return !(lppds->hpid == *lphpid);
}

VOID
KillAlm(
    LPALM lpalm
    )
{
    if (lpalm) {
        MHFree ( lpalm );
    }
}

VOID
KillSht(
    LPSHT lpsht
    )
{
    if (lpsht) {
        if (lpsht->rgib) {
            lpsht->rgib = 0;
        }

        if (lpsht->rgcib) {
            lpsht->rgcib = 0;
        }

        KillAlm (lpsht->lpalm);
        lpsht->lpalm = 0;
    }
}

VOID
KillSci(
    SymConvertInfo & sci
    )
{
    if (sci.rgOffMap)
        MHFree(sci.rgOffMap);
    if (sci.pbSyms)
        MHFree(sci.pbSyms);
    memset(&sci, 0, sizeof sci);
}

VOID
KillGst(
    LPGST lpgst
    )
{
    if (lpgst) {
        KillSht ( &lpgst->shtName );
        KillSht ( &lpgst->shtAddr );
        KillAlm ( lpgst->lpalm );
        KillSci ( lpgst->sci );
        lpgst->lpalm = 0;
    }
}

//  KillExgNode
//
//  Purpose: Destroy information contained in an exe node
//
//  Input: pointer to node data

VOID
KillExgNode (
    PVOID lpvExgNode
    )
{
    LPEXG   lpexg = (LPEXG) lpvExgNode;
    DWORD   i;

    // Debug info file name
    if (lpexg->lszDebug) {
        MHFree (lpexg->lszDebug);
        lpexg->lszDebug = 0;
    }

    // .exe/.com file name
    if (lpexg->lszName) {
        MHFree (lpexg->lszName);
        lpexg->lszName = 0;
    }

    // AltName

    if (lpexg->lszAltName) {
        MHFree(lpexg->lszAltName);
        lpexg->lszAltName = 0;
    }

    // Pdb name
    if (lpexg->lszPdbName) {
        MHFree (lpexg->lszPdbName);
        lpexg->lszPdbName = 0;
    }

    // Type table
    if (lpexg->lpalmTypes) {
        KillAlm(lpexg->lpalmTypes);
        lpexg->lpalmTypes = 0;
    }

    // Free up memory associated with the dll name
    if (lpexg->lszModule) {
        MHFree ( lpexg->lszModule );
        lpexg->lszModule = 0;
    }

    if (lpexg->pwti) {
        lpexg->pwti->release();
        lpexg->pwti = 0;
    }

    // Nuke the OMF symbolic.

    OLUnloadOmf(lpexg);

    // And any possible cache references to this module.

    VoidCaches();
}

LSZ
NameOnly(
    LSZ    lsz
    )
{
    LSZ p;

    p = lsz + _tcslen(lsz);

    // Search from the end back for the path delimiter.
    while ( p > lsz && *p != '\\' && *p != ':' ) {
        p = _tcsdec(lsz, p);
    }

    if (p > lsz) {
        // We're pointing at the delimiter.  Move forward one.
        p = _tcsinc(p);
    }

    return p;
}

//  CmpExgNode
//
//  Purpose: Compare global exe nodes
//
//  Input: pointer to node data

int
CmpExgNode (
    PVOID lpv1,
    PVOID lpv2,
    LONG lParam
    )
{
    LPEXG   lpexg1 = (LPEXG) lpv1;
    LSZ     lsz1   = lpexg1->lszName;
    LSZ     lsz2   = (LSZ) lpv2;

    if (lParam == MATCH_NAMEONLY) {
        lsz1 = NameOnly(lsz1);
        lsz2 = NameOnly(lsz2);
    }
    else if (lParam == MATCH_BACKGROUND) {
        //
        //  Return  the module if
        //      1)  The dll is marked for background loading
        //      2)  The dll is marked for unloading and it has been more
        //              than 10 minutes
        //

        BOOLEAN         f;
        LONGLONG        ll = *(LONGLONG *) lpv2;
        f = lpexg1->fOmfDefered ||
          ((lpexg1->llUnload != 0) && (ll - lpexg1->llUnload > 10*60*1000));
        return !f;
    }

    return _tcsicmp(lsz1, lsz2);
}


//  KillExeNode
//
//  Purpose: Destroy information contained in an exe node
//
//  Input: pointer to node data

VOID
KillExeNode(
    PVOID     lpvExeNode
    )
{
        // REVIEW: when a process dies at startup (missing dll for ex) , this ends up being
        // non-NULL because SHUnloadDll does not get called. We should re-enable this
        // later.
    // assert (((LPEXE)lpvExeNode)->pDebugData == NULL);

    // All we need to do is flush the global cache (in case we're still in it)

    VoidCaches();
}


int
CmpExeNode(
    PVOID     lpv1,
    PVOID     lpv2,
    LONG    l
    )
{
    LPEXE   lpexe1 = (LPEXE) lpv1;

    Unreferenced(l);

    return !(lpexe1->hexg == (HEXG) lpv2);
}

//  KillMdsNode
//
//  Purpose: Free up memory allocations associated with node
//
//  Input:  pointer to the mds node

VOID
KillMdsNode (
    PVOID lpvMdsNode
    )
{
    LPMDS   lpmds = (LPMDS)lpvMdsNode;

    if (lpmds->name) {
        MHFree (lpmds->name);
        lpmds->name = 0;
    }

    if (lpmds->lpsgc) {
        MHFree (lpmds->lpsgc);
        lpmds->lpsgc = 0;
    }

    if (lpmds->symbols) {
        if (lpmds->pmod) {
            // For PDB, symbols is a copy.
            MHFree(lpmds->symbols);
        }
        KillSci(lpmds->sci);
        lpmds->symbols = 0;
        lpmds->cbSymbols = 0;
    }

    if (lpmds->hst) {
        //  Just a pointer to the cv data
        lpmds->hst = 0;
    }

    if (lpmds->pmod) {
        if (!ModClose(lpmds->pmod)) {
            assert (FALSE);
        }
        lpmds->pmod = 0;
    }
}

//  CmpMdsNode
//
//  Purpose: To compare two mds nodes.
//
//  Input:
//      lpv1   far pointer to first node
//      lpv2   far pointer to second node
//      lParam comparison type ( MDS_INDEX is only valid one, for now)
//
//  Output: Returns zero if imds are equal, else non-zero

int
CmpMdsNode (
    PVOID lpv1,
    PVOID lpv2,
    LONG lParam
    )
{
    LPMDS   lpmds1 = (LPMDS) lpv1;
    LPMDS   lpmds2 = (LPMDS) lpv2;

    assert (lParam == MDS_INDEX);

    return lpmds1->imds != lpmds2->imds;
}


//  SHHexgFromHmod
//
//  Purpose: Get the hexg from the specified mds handle
//
//  Input: handle to a VALID mds node
//
//  Output: handle to the hmod's parent (hexe)

HEXG
SHHexgFromHmod (
    HMOD hmod
    )
{
    HEXG    hexg;

    assert(hmod);

    hexg = ((LPMDS)hmod)->hexg;
    return hexg;
}


//  SHHexeFromHmod
//
//  Purpose: Get the hexe from the specified module handle
//
//  Input: handle to a VALID mds node
//
//  Output: handle to the hmod's parent (hexe)

HEXE
SHHexeFromHmod (
    HMOD hmod
    )
{
    HEXG    hexg;
    LPEXG   lpexg;
    LPPDS   lppds;

    if (hmod == NULL) {
        return hexeNull;
    }

    // If this isn't the same hmod or hpds as last time, look up
    // the hexe in current process list and return.

    if ((hmod != ModCache.hmod) || (hpdsCur != ModCache.hpds)) {
        hexg = ((LPMDS) hmod)->hexg;

        if (hexg != NULL) {
            ModCache.hmod = hmod;
            ModCache.hpds = hpdsCur;
            lppds = (LPPDS) LLLock(hpdsCur);
            ModCache.hexe = LLFind(lppds->hlliExe, 0, (PVOID) hexg, 0L);
            LLUnlock(hpdsCur);
        }
    }

    return ModCache.hexe;
}

//  SHGetExeName
//
//  Purpose: Get the exe name for a specified hexe
//
//  Input: handle to the exe node
//
//  Output: pointer to the exe's full path-name file

LSZ
SHGetExeName (
    HEXE hexe
    )
{
    LSZ     lsz = NULL;
    HEXG    hexg;
    LPEXG   lpexg;

    assert(hpdsCur && hexe);

    if (!hexe || !VerifyHexe(hexe)) {
        return(NULL);
    }


    hexg = ((LPEXE) LLLock (hexe))->hexg;
    if (hexg) {
        
        lpexg = (LPEXG) LLLock(hexg);
        if (lpexg) {
            
            // If there's an alternate name (as in kernel debugging when hal.dll
            // may really be halmp or halncr or...), use it.
            
            if (lpexg->lszAltName) {
                lsz = lpexg->lszAltName;
            } else {
                lsz = lpexg->lszName;
            }
            
            // Bypass the fake "#:\" drive letter the DM added for a remote drive
            
            if (lsz[0] == '#' && lsz[1] == ':' && lsz[2] == '\\') {
                lsz += 3;
            }
            
            LLUnlock (hexg);
        }
        LLUnlock (hexe);
    }

    return lsz;
}

//  SHGetModNameFromHexe
//
//  Purpose: Get the module name from the specified hexe
//
//  Input:  handle to the exe node
//
//  Output: pointer to the exe's module name

LSZ
SHGetModNameFromHexe(
    HEXE hexe
    )
{
    LSZ     lsz;
    HEXG    hexg;
    LPEXG   lpexg;

    assert(hpdsCur && hexe);

    hexg = ((LPEXE) LLLock(hexe))->hexg;
    lpexg = (LPEXG) LLLock(hexg);
    lsz = lpexg->lszModule;

    LLUnlock (hexe);
    LLUnlock (hexg);

    return lsz;
}

//  SHGetSymFName
//
//  Purpose: Get the symbol file name for the specified hexe
//
//  Input: handle to the exe node
//
//  Output: pointer to the exe's full path-name file

LSZ
SHGetSymFName(
    HEXE hexe
    )
{
    LSZ     lsz;
    HEXG    hexg;
    LPEXG   lpexg;

    assert(hpdsCur && hexe);

    hexg  = ((LPEXE)LLLock(hexe))->hexg;
    lpexg = (LPEXG)LLLock(hexg);
    lsz = lpexg->lszDebug;
    if (!lsz) {
        lsz = lpexg->lszName;
    }

    // Bypass the fake "#:\" drive letter the DM added for a remote drive

    if (lsz[0] == '#' && lsz[1] == ':' && lsz[2] == '\\') {
        lsz += 3;
    }

    LLUnlock(hexe);
    LLUnlock(hexg);

    return lsz;
}

//  SHGetNextExe
//
//  Purpose: Get the handle to the next node in the exe list for the CURRENT
//           process. If the hexe is null, then get the first one in the list.
//
//  Input:  handle to the "previous" node.  If null, get the first one in
//          the exe list.
//
//  Output: Returns a handle to the next node.  Returns NULL if the end of
//          the list is reached (ie: hexe is last node in the list)

HEXE
SHGetNextExe (
    HEXE hexe
    )
{
    HEXE    hexeRet;
    HLLI    hlli;

    if (!hpdsCur)               // check for non-null process [rm]
        return NULL;

    hlli    = ((LPPDS) LLLock (hpdsCur))->hlliExe;
    hexeRet = LLNext (hlli, hexe);
    LLUnlock (hpdsCur);
    return hexeRet;
}

//  SHHmodGetNext
//
//  Purpose: Retrieve the next module in the list.  If a hmod is specified, get
//           the next in the list.  If the hmod is NULL, then get the first module
//           in the exe.  If no hexe is specified, then get the first exe in the list.
//
//  Input:
//      hexe    hexe containing list of hmod's.  If NULL, get first in CURRENT
//              process list.
//      hmod    module to get next in list.  If NULL, get first in the list.
//
//  Output: Returns an hmod of the next one in the list.  NULL if the end of
//          the list is reached.

HMOD
SHHmodGetNext (
    HEXE hexe,
    HMOD hmod
    )
{
    HMOD hmodRet = 0;

    if (!hpdsCur) {
        return hmodRet;
    }

    if (hmod) {
        hmodRet = (HMOD)((LPMDS)hmod + 1);
        if (((LPMDS)(hmodRet))->imds == 0xFFFF) {
            hmodRet = NULL;
        }

        return(hmodRet);
    }

    if (hexe) {
        MDS     *pMds;
        HEXG     hexg;
        LPEXG    lpexg;
        LPEXE    lpexe;

        lpexe = (LPEXE) LLLock(hexe);
        // The test for 10 is arbitrary.  Testing for only NULL
        // may allow some cases to slip through.
        if (lpexe < (LPEXE) 10) {
            return(hmodRet);
        }
        hexg = lpexe->hexg;
        LLUnlock(hexe);

        if (hexg == 0) {
            return(hmodRet);
        }

        lpexg = (LPEXG) LLLock(hexg);
        if (lpexg < (LPEXG) 10) {
            return(hmodRet);
        }

        pMds = lpexg->rgMod;
        if (pMds != NULL) {
            hmodRet = (HMOD) &pMds[1];
        }
        LLUnlock(hexg);
    }

    return hmodRet;
}

//  SHFAddNewPds
//
//  Purpose: Create a new process node and make it current!
//
//  Input: Word value to identify pds indexing
//
//  Output: Non-zero if successful, else zero.
//
//  Notes: Creates a new node and initializes private list of exe's.

HPDS
SHFAddNewPds (
    VOID
    )
{
    LPPDS   lppds;
    HPDS    hpds;

    if (hpds = (HIND) LLCreate (HlliPds)) {
        lppds = (LPPDS) LLLock (hpds);
        lppds->hlliExe = LLInit (sizeof(EXE), 0, KillExeNode, CmpExeNode);

        // If the list create failed, destroy the node and return failure
        if (!lppds->hlliExe) {
            LLUnlock (hpds);
            MMFree ((HDEP) hpds);
            hpds = 0;
        } else {
            // Otherwise, add the pds to the list and return success
            LLUnlock(hpds);
            LLAdd (HlliPds, hpds);
            hpdsCur = hpds;
        }
    }
    return hpds;
}

//  SHHexeAddNew
//
//  Purpose: Create and initialize an exe node.
//
//  Input:
//      hpds      Process hexe is assoceiated with
//      hexg      Symbol information for the exe.
//      LoadAddress a value of 0 means the DLL hasn't actually been loaded (we are just pre-loading its symbols)
//
//  Output: Returns hexg of newly created node.  NULL if OOM.

HEXE
SHHexeAddNew (
    HPDS hpds,
    HEXG hexg,
    UOFFSET LoadAddress
    )
{
    HEXE    hexe;
    LPEXE   lpexe;
    LPEXG   lpexg;
    HLLI    hlli;

    if (!hpds) {
        hpds = hpdsCur;
    }

    hlli = ((LPPDS)LLLock(hpds))->hlliExe;

    // Ensure that the hexg isn't already in the list
    hexe = LLFind(hlli, 0, (PVOID)hexg, 0L);

    if (!hexe && (hexe = LLCreate (hlli))) {
        // Not already there.  Create one, init the fields, and add it to the list
        lpexe = (LPEXE) LLLock (hexe);
        lpexe->hexg = hexg;
        lpexe->hpds = hpdsCur;
        lpexe->fIsLoaded = LoadAddress ? TRUE : FALSE;
        lpexe->LoadAddress = LoadAddress;
        lpexe->pDebugData = NULL;
        LLAdd (hlli, hexe);
        LLUnlock (hexe);

        // Increment the ref count on the image.
        lpexg = (LPEXG) LLLock(hexg);
#ifdef NT_BUILD_ONLY
        lpexg->cRef++;
#endif
        LLUnlock (hexg);
    } else {
        lpexe = (LPEXE) LLLock (hexe);
        lpexe->LoadAddress = LoadAddress;
        lpexg = (LPEXG) LLLock(hexg);
        lpexg->LoadAddress  = LoadAddress;
        LLUnlock (hexg);
        LLUnlock (hexe);
    }

    LLUnlock (hpds);
    return hexe;
}


//  SHAddDll
//
//  Purpose: Notify the SH about an EXE/DLL for which symbolic information
//           will need to be loaded later.
//
//           During the startup of a debuggee application, this function
//           will be called once for the EXE, and once for each DLL that
//           is used by the EXE.  After making these calls,
//           SHAddDllsToProcess will be called to associate those DLLs
//           with that EXE.
//
//           See the comments at the top of this file for more on when
//           this function should be called.
//
//  Input:
//      lsz     Fully qualified path/file specification.
//      fDll    TRUE if this is a DLL, FALSE if it is an EXE.
//
//  Output:
//      Returns nonzero for success, zero for out of memory.
//
//  Notes:
//      This function does NOT actually load the symbolic information;
//      SHLoadDll does that.

SHE
SHAddDll (
    LSZ lsz,
    BOOL fDll
    )
{
    HEXG hexg = hexgNull;
    SHE  sheRet;

    EnterCriticalSection(&csSh);

    sheRet = SHAddDllExt (lsz, fDll, TRUE, NULL, &hexg);

    LeaveCriticalSection(&csSh);

    return sheRet;
}


//  SHAddDllExt
//
//  Purpose: Notify the SH about an EXE/DLL for which symbolic information
//           will need to be loaded later.
//
//           During the startup of a debuggee application, this function
//           will be called once for the EXE, and once for each DLL that
//           is used by the EXE.  After making these calls,
//           SHAddDllsToProcess will be called to associate those DLLs
//           with that EXE.
//
//           See the comments at the top of this file for more on when
//           this function should be called.
//
//  Input:
//      lsz         Fully qualified path/file specification.
//      fDll        TRUE if this is a DLL, FALSE if it is an EXE.
//      pVldChk     Pointer to a VLDCHK structure. If NULL no checking should
//                  be done.
//      fMustExist  TRUE if success requires that the dll must be found
//                  i.e. the user asked for symbol info for this dll
//                  and would expect a warning if it isn't found.
//
//  Output:
//      [Public interface]
//          Returns nonzero for success, zero for out of memory.
//
//      [Private SAPI interface]
//          Returns HEXG of newly created node, or NULL if out of memory.
//
//  Notes:
//      This function does NOT actually load the symbolic information;
//      SHLoadDll does that.
//
//      This function is used internally, AND it is also exported
//      to the outside world.  When exported, the return value should
//      just be considered a BOOL: zero means out of memory, nonzero
//      means success.

SHE
SHAddDllExt (
    LSZ     lsz,
    BOOL    fDll,
    BOOL    fMustExist,
    VLDCHK *pVldChk,
    HEXG   *lphexg
    )
{
    HEXG            hexg;
    LPEXG           lpexg;
    _TCHAR          szAbsolutePath[_MAX_CVPATH];

    if (fDll) {
        struct _stat    statT;
        _TCHAR          szFullPath[_MAX_PATH];
        _TCHAR          szDrive[_MAX_PATH];
        _TCHAR          szDir[_MAX_PATH];
        _TCHAR          szFName[_MAX_FNAME + _MAX_EXT];
        _TCHAR          szExt[_MAX_EXT];
        _TCHAR          *lpszPath;

        _tsplitpath (lsz, szDrive, szDir, szFName, szExt);

        _tcscat(szFName, *szExt ? szExt : ".DLL");

        _tcscpy(szFullPath, szDrive);
        _tcscat(szFullPath, szDir);
        _tfullpath(szAbsolutePath, 
                   szFullPath, 
                   sizeof(szAbsolutePath) / sizeof(_TCHAR)
                   );
        _tcsupr(szAbsolutePath);

        lpszPath = szAbsolutePath;
        lpszPath = _tcsdec(lpszPath, _tcschr(lpszPath, '\0'));

        if (*lpszPath != '\\') {
            _tcscat(szAbsolutePath, "\\");
        }

        _tcsupr (szAbsolutePath);
        _tcscat (szAbsolutePath, szFName);

        if (!*szFullPath || _tstat (szAbsolutePath, &statT)) {
            _tsearchenv (szFName, "PATH", szAbsolutePath);
        }

        if (*szAbsolutePath == 0) {
            if (fMustExist) {
                *lphexg = hexgNull;
                return sheFileOpen;
            } else {
                // Retain the full path instead of just the filname
                // If we just remember the file name then during
                // a restart when we get the same dll load again
                // the IDE will report an error as the paths don't
                // match.
                _tcscpy (szAbsolutePath, lsz);
            }
        }
    } else {
        _tcscpy(szAbsolutePath, lsz);
    }

    hexg = LLFind (HlliExgExe, 0, &szAbsolutePath, pVldChk ? MATCH_NAMEONLY : 0L);

    // If we have an image and a validity check structure, make
    // sure we have the correct image...

    if ((hexg != hexgNull) && pVldChk) {
        lpexg = (LPEXG) LLLock(hexg);

        // A TimeAndDateStamp field of -1 means we don't care.
        if (pVldChk->ImgTimeDateStamp == 0xffffffff) {
            pVldChk->ImgTimeDateStamp = lpexg->ulTimeStamp;
        }

        if ((pVldChk->ImgTimeDateStamp != lpexg->ulTimeStamp) ||
            (pVldChk->ImgCheckSum != lpexg->ulCheckSum)) {
            LLUnlock(hexg);
            hexg = hexgNull;
        } else {
            LLUnlock(hexg);
        }
    }

    if ((hexg == hexgNull) &&
        ((hexg = LLCreate (HlliExgExe)) != hexgNull)) {

        lpexg = (LPEXG)LLLock(hexg);

        memset(lpexg, 0, sizeof(EXG));

        lpexg->fOmfLoaded = FALSE;

        if (lpexg->lszModule = (LSZ) MHAlloc (_tcslen(lsz) + 1)) {
            // Skip past the fake "#:\" drive the DM might have added

            if (lsz[0] == '#' && lsz[1] == ':' && lsz[2] == '\\') {
                lsz += 3;
            }
            _tsplitpath(lsz, NULL, NULL, lpexg->lszModule, NULL);
        }

        if (lpexg->lszName = (LSZ) MHAlloc(_tcslen(szAbsolutePath) + 1)) {
            _tcscpy(lpexg->lszName, (LSZ) szAbsolutePath);
        }

        if (!lpexg->lszName || !lpexg->lszModule || (*lpexg->lszName == '\0')) {
            // If any of the allocated fields are NULL,
            // then destroy the node and return failure

            KillExgNode((PVOID)lpexg);
            LLUnlock( hexg );
            hexg = hexgNull;
        } else {
            // Otherwise, add the node to the list
            LLAdd (HlliExgExe, hexg);
            LLUnlock(hexg);
        }
    }

    *lphexg = hexg;
    return (hexg == hexgNull) ? sheOutOfMemory : sheNone;
}


//  SHHmodGetNextGlobal
//
//  Purpose: Retrieve the next module in the current PROCESS.
//
//  Input:
//      phexe   Pointer to hexe.  This will be updated.  If NULL, then
//              start at the first exe in the current process.
//      hmod    Handle to mds.  If NULL, set *phexe to the next process.
//              and get the first module in it.  Otherwise get the next
//              module in the list.
//
//  Output: Returns a handle to the next module in the proces list.  Will
//          return hmodNull if the end of the list is reached.

HMOD
SHHmodGetNextGlobal (
    HEXE  *phexe,
    HMOD hmod
    )
{
    assert(hpdsCur);

    do {
        // If either the hexe or hmod is NULL, then on to the next exe.
        if (!*phexe || !hmod) {
            *phexe = SHGetNextExe (*phexe);
            hmod = hmodNull;        // Start at the beginning of the next one
        }

        // If we've got an exe, get the next module
        if (*phexe) {
            hmod = SHHmodGetNext(*phexe, hmod);
        }
    } while(!hmod && *phexe);
    return hmod;
}

//  SHGetSymbol
//
//  Searches for a symbol value in the symbol table containing the seg and off
//
//  Entry conditions:
//      paddrOp:
//          segment and offset of the symbol to be found
//      paddrLoc:
//          assumes that the module variable startaddr is set to the beginning
//            of the code line currently being disassembled.
//      sop:
//          symbol options: what kinds of symbols to match. (???)
//      lpodr:
//          pointer where delta to symbol will be stored as well as
//          near/farness, FPO/NON-FPO, cbProlog and symbol name.
//
//  Exit conditions:
//      *lpdoff:
//          offset from symbol to the address
//      return value:
//          ascii name of symbol, or NULL if no match found

LSZ
SHGetSymbol (
    LPADDR paddrOp,
    LPADDR paddrLoc,
    SOP    sop,
    LPODR  lpodr
    )
{
    CXT   cxt    = {0};
    ADDR  addrT  = *paddrLoc;
    ADDR  addrOp = *paddrOp;

    if (!ADDR_IS_FLAT(addrOp) && GetAddrSeg (addrOp) == 0) {
        return NULL;
    }

    SYUnFixupAddr ( &addrT );

    if (sop & sopStack) {

        // UNDONE: This is the only caller of FindBpOrReg...  Optimize
        if (SHFindBpOrReg (&addrT,
                           GetAddrOff ( addrOp ),
                           (WORD) (ADDR_IS_OFF32 (*paddrOp) ? S_BPREL32 : S_BPREL16),
                           lpodr->lszName))
        {
            lpodr->dwDeltaOff = 0;
            return lpodr->lszName;
        } else {
            return NULL;
        }
    } else {
        SYUnFixupAddr ( &addrOp );
    }

    cxt.hMod = 0;

    if (sop & sopData) {
        SHSetCxtMod (&addrT, &cxt);
    } else {
        SHSetCxtMod (&addrOp, &cxt);
    }
    cxt.addr = addrOp;

    // get the closest symbol, including locals

    SHdNearestSymbol (&cxt, sop, lpodr);

    if ((sop & sopExact) && lpodr->dwDeltaOff) {
        return NULL;
    } else {
        if (lpodr->dwDeltaOff == CV_MAXOFFSET) {
            return NULL;
        } else {
            return lpodr->lszName;
        }
    }
}


LSZ
SHGetModule (
    LPADDR paddrOp,
    LSZ    rgbName
    )
{
    CXT    cxt = {0};
    ADDR   addrOp = *paddrOp;
    HEXE   hexe;
    HEXG   hexg;
    LPEXG  lpexg;

    rgbName[0] = '\0';
    SYUnFixupAddr (&addrOp);
    cxt.hMod = 0;
    SHSetCxtMod (&addrOp, &cxt);
    if (!cxt.hMod ) {
        return NULL;
    }
    cxt.addr = addrOp;
    hexe = SHHexeFromHmod(cxt.hMod);
    if (hexe) {
        hexg = ((LPEXE) LLLock (hexe))->hexg;
        lpexg = (LPEXG) LLLock(hexg);
        _tcscpy(rgbName, lpexg->lszModule);
        LLUnlock (hexe);
        LLUnlock (hexg);
        return rgbName;
    }

    return NULL;
}


// SHHexeFromName - private function */

HEXE
SHHexeFromName (
    LSZ lszName
    )
{
    BOOL fFound = FALSE;
    HEXE hexe = hexeNull;

    // Find the hexe associated with the libname

    while(!fFound && (hexe = SHGetNextExe (hexe))) {
        HEXG hexg = ((LPEXE)LLLock(hexe))->hexg;
        LLUnlock (hexe);
        fFound = !_tcsicmp (((LPEXG)LLLock(hexg))->lszName, lszName);
        LLUnlock (hexg);
    }

    return fFound ? hexe : hexeNull;
}

//  SHUnloadDll
//
//  Purpose: Mark an EXE/DLL as no longer resident in memory.  The debugger
//           should call this function when it receives a notification from
//           the OS indicating that the module has been unloaded from
//           memory.  This does not unload the symbolic information for the
//           module.
//
//           See the comments at the top of this file for more on when this
//           function should be called.
//
//  Input:  hexe - The handle to the EXE/DLL which was unloaded. After
//                 getting a notification from the OS, the debugger can
//                 determine the HEXE by calling SHGethExeFromName.

VOID
SHUnloadDll (
    HEXE hexe
    )
{
    LPEXE   lpexe;
    LPEXG   lpexg;
    HPDS    hpds;
    HEXG    hexg;
    LPPDS   lppds;

    assert(hexe);
    lpexe = (LPEXE) LLLock (hexe);
    assert(lpexe);
    hexg = lpexe->hexg;
    assert(hexg);
    lpexg = (LPEXG) LLLock (hexg);
    assert(lpexg);
#ifdef NT_BUILD_ONLY
    assert(lpexg->cRef);
#endif
    hpds = lpexe->hpds;
    assert(hpds);
    lppds = (LPPDS) LLLock(hpds);
    assert(lppds);

        // Decrement the reference count

#ifdef NT_BUILD_ONLY
    lpexg->cRef--;
#endif

    // See if it's in the deferred load queue.  If so, remove it.

    if (lpexg->fOmfDefered) {
        UnloadDefered(hexg);
    }

    // If this exe has exg DEBUGDATA, free it.

    if (lpexe->pDebugData) {
        // Free the copy of the debug data that we got from the exg.

        if (lpexe->pDebugData != &lpexg->debugData) {
            // This must be a fixed up copy
            MHFree(lpexe->pDebugData->lpRtf);
            MHFree(lpexe->pDebugData);
        }
        lpexe->pDebugData = NULL;
    }

#ifndef NT_BUILD_ONLY
    // VC5 hacks here so that we don't unload anything, ever
    lpexe->fIsLoaded = FALSE;
    LLUnlock(hexg);
    LLUnlock (hexe);
    LLUnlock(hpds);
        return;
#else

    //
    // If we're the last reference to the module,
    // delete it from the image list.
    //

    if (lpexg->cRef == 0) {
        lpexe->fIsLoaded = FALSE;
        LLUnlock(hexg);
        LLDelete(HlliExgExe, hexg);
    } else {
        LLUnlock(hexg);
    }

    LLUnlock (hexe);

    // Remove this image from the process list.

    LLDelete(lppds->hlliExe, hexe);
    LLUnlock(hpds);

    return;
#endif
}

//  SHLoadDll
//
//  Purpose: This function serves two purposes:
//
//           (1) Load symbolic information for an EXE/DLL into memory,
//               so its symbols are available to the user.
//           (2) Indicate to the SH whether the EXE/DLL itself is loaded
//               into memory.
//
//           Because it serves two purposes, this function may be called
//           more than once for the same EXE/DLL.  See the comments at
//           the top of this file for more on when this function should
//           be called.
//
//  Input:
//      lszName     The name of the EXE or DLL.
//      fLoading    TRUE if the EXE/DLL itself is actually loaded at this
//                  time, FALSE if not.
//
//  Output:
//      Returns an SHE error code.

SHE
SHLoadDll (
    LSZ lszName,
    BOOL fLoading
    )
{
    SHE     she = sheNone;
    HEXE    hexe;
    HEXG    hexg = hexgNull;
    LSZ     lsz = lszName;
    LSZ     lsz2 = NULL;
    LSZ     lsz3;
    HANDLE  hfile = INVALID_HANDLE_VALUE;
    HEXG    hexgMaybe = hexgNull;
    LSZ     AltName = NULL;
    char    ch = 0;
    VLDCHK  vldChk= {0}; // 0 ==> unknown.
    LPEXG   lpexg;
    LSZ     lszFname;
    BOOL    fContinueSearch = TRUE;
    UOFFSET dllLoadAddress = 0;
    BOOL    fMpSystem = FALSE;
    ULONG64 uTmp = 0;

    EnterCriticalSection(&csSh);

    // Check for the possiblity that we have a module only name
    //
    // We may get two formats of names.  The first is just a name,
    // no other information.  The other is a big long string
    // for PE exes.

    if (*lszName == '|') {
        // name
        ++lszName;
        lsz2 = _tcschr(lszName, '|');

        lsz = lsz2;

        if (lsz && *lsz == '|') {
            // timestamp
            vldChk.ImgTimeDateStamp = _tcstoul(++lsz, &lsz, 16);
        }
        if (lsz && *lsz == '|') {
            // checksum
            vldChk.ImgCheckSum = _tcstoul(++lsz, &lsz, 16);
        }
        if (lsz && *lsz == '|') {
            // hfile

            // strtouli returns a ULARGE_INTEGER
            uTmp = (strtouli(++lsz, &lsz, 16)).QuadPart;

            hfile = (HANDLE) uTmp;
            // Always close the handle returned from the DM.
            // We'll reopen it iff need be in OlLoadOmf.
            if (hfile != INVALID_HANDLE_VALUE) {
                SYClose(hfile);
                hfile = INVALID_HANDLE_VALUE;
            }
        }
        if (lsz && *lsz == '|') {
            // image base

            // strtouli returns a ULARGE_INTEGER
            dllLoadAddress = (strtouli(++lsz, &lsz, 16)).QuadPart;
        }
        if (lsz && *lsz == '|') {
            // checksum
            vldChk.ImgSize = _tcstoul(++lsz, &lsz, 16);
        }
        if (lsz && *lsz == '|') {
            // alternate symbol file name
            lsz++;
            if (*lsz) {
                lsz3 = _tcschr(lsz, '|');
                *lsz3 = 0;
                if (AltName = (LSZ) MHAlloc(_tcslen(lsz) + 1)) {
                    _tcscpy(AltName, (LSZ) lsz);
                }
                *lsz3 = '|';
                lsz = lsz3;
            }
        }

        // isolate name
        if (lsz2) {
            *lsz2 = 0;
        }
    }

    // We can't just call SHExeFromName here because there may be
    // multiple images with the same name (for instance, when debugging
    // more than one process or more than one machine) and we want to
    // to share symbols wherever possible.

    // Look and see if we already have this debug information loaded
    while (fContinueSearch) {
        // Find the next OMF set by name of EXE module
                hexg = LLFind(HlliExgExe, hexg, lszName, MATCH_NAMEONLY);

        if (hexg == hexgNull) {
            fContinueSearch = FALSE;    // Nothing by this name.
        } else {
            // If we found one, do the checksum and timestamp match?

            lpexg = (LPEXG) LLLock(hexg);

            // If the timestamp is set to -1, use whatever this image has.

            if (vldChk.ImgTimeDateStamp == 0xffffffff) {
                vldChk.ImgTimeDateStamp = lpexg->ulTimeStamp;
            }


            // UNDONE: We don't compare LoadAddresses here to ensure they match
            // when the symbols are correct...  Investigate if this is a bug.

            //      Check the timestamp & checksum only if the Omf has been
            //      succesfully loaded

            if (fLoading && lpexg->fOmfLoaded &&
                ((vldChk.ImgTimeDateStamp != lpexg->ulTimeStamp) ||
                 (vldChk.ImgCheckSum != lpexg->ulCheckSum))
                ) {

#if NT_BUILD_ONLY
                // NB: the reference counting mechanism is not correct -- so
                // we cannot use it here.  Fix when we fix ref counting

                // The debug info no longer matches -- check to see if
                // there are any references to the debug info. If not then
                // free it as it has been superceded.

                if ( lpexg->cRef == 0 ) {
                    LLUnlock(hexg);
                    LLDelete(HlliExgExe, hexg);
                    hexg = hexgNull;
                }
                // Keep looking for a match.
#endif  // NTBUILD
            } else {

                // Timestamp and checksum are valid.  Has the user decided
                // to change the load status of this exe?

                lszFname = lpexg->lszName;

                if (!SYGetDefaultShe(lszFname, &she)) {
                    SYGetDefaultShe(NULL, &she);
                }

                switch (she) {
                    case sheDeferSyms:
                    case sheNone:
                        if (!lpexg->fOmfMissing && !lpexg->fOmfSkipped) {
                            fContinueSearch = FALSE;
                        } else {
                            hexgMaybe = hexg;
                        }
                        break;

                    case sheSuppressSyms:
                        if (lpexg->fOmfMissing || lpexg->fOmfSkipped) {
                            fContinueSearch = FALSE;
                        } else {
                            hexgMaybe = hexg;
                        }
                        break;

                    default:
                        assert((she == sheDeferSyms) ||
                               (she == sheSuppressSyms) ||
                               (she == sheNone));
                        break;
                }
            }

            if (hexg) {
                LLUnlock(hexg);
            }
        }
    }

    // If we did not find an OMF module -- create one

    if (hexg == hexgNull && hexgMaybe == hexgNull) {

        SHAddDllExt (lszName, TRUE, FALSE, fLoading? &vldChk:NULL, &hexg);
        if (hexg == hexgNull) {
            she = sheOutOfMemory;
        }
        else if (SHHexeAddNew (hpdsCur, hexg, dllLoadAddress) == NULL) {
            she = sheOutOfMemory;
        }
        else {
            if (AltName) {
                lpexg = (LPEXG) LLLock(hexg);
                lpexg->lszAltName = AltName;
                LLUnlock(hexg);
            }

            she = OLLoadOmf(hexg, &vldChk, dllLoadAddress);
        }
    } else if (hexg != hexgNull) {

        // We found the OMF lying around

        // If we found a partial match as well, see if it s/b discarded.

        if (hexgMaybe) {
#if NT_BUILD_ONLY
            // NB: comments above about #if'ing out reference counting code.

            lpexg = (LPEXG) LLLock(hexgMaybe);
            LLUnlock(hexgMaybe);
            if (lpexg->cRef == 0) {
                LLDelete(HlliExgExe, hexgMaybe);
            }
#endif // NT_BUILD_ONLY
        }

        if (SHHexeAddNew(hpdsCur, hexg, dllLoadAddress) == NULL) {
            she = sheOutOfMemory;
        }

                if ( !hexgMaybe )
                {
                        lpexg = (LPEXG) LLLock(hexg);
                        // REVIEW: The shell can call SHAddDll and later SHLoadDll to actually load
                        // the OMF. We have to load the dll in this case. This whole function needs to
                        // be revisited.
                        if (!lpexg->fOmfLoaded && !lpexg->fOmfMissing)
                        {
                                OLLoadOmf(hexg, &vldChk, dllLoadAddress);
                        }
                        LLUnlock(hexg);
                }

                if (fLoading)
                {
                        HLLI hlli = ((LPPDS)LLLock(hpdsCur))->hlliExe;
                    HEXE hexe = LLFind(hlli, 0, (PVOID)hexg, 0L);
                        LLUnlock(hlli);
                        if (hexe)
                        {
                                ((LPEXE)LLLock(hexe))->fIsLoaded = TRUE;
                                LLUnlock(hexe);
                        }
                        LLUnlock(hpdsCur);
                }

        if (hfile != INVALID_HANDLE_VALUE) {
            SYClose(hfile);
        }
    } else {
        // Found the right exg, but the wrong load state.
        lpexg = (LPEXG) LLLock(hexgMaybe);
        if (lpexg->fOmfMissing) {
            // no OMF in image.  We can't improve things.
            LLUnlock(hexgMaybe);
            she = sheNoSymbols;
        } else if (lpexg->fOmfSkipped) {

            // decided to load syms this time around.
            LLUnlock(hexgMaybe);
            she = OLLoadOmf(hexgMaybe, &vldChk, dllLoadAddress);
        } else {

            // have syms and want to discard them.
            OLUnloadOmf(lpexg);
            VoidCaches();       // Just in case.
            lpexg->fOmfSkipped = TRUE;
            LLUnlock( hexgMaybe );
            she = sheSuppressSyms;
        }

        if (SHHexeAddNew(hpdsCur, hexgMaybe, dllLoadAddress) == NULL) {
            she = sheOutOfMemory;
        }
    }

    // Restore damage to the input buffer

    if (lsz2 != NULL) {
        *lsz2 = '|';
    }

    LeaveCriticalSection(&csSh);

    return she;
}

//  SHAddDllsToProcess
//
//  Purpose: Associate all DLLs that have been loaded with the current EXE.
//
//           The debugger, at init time, will call SHAddDll on one EXE
//           and zero or more DLLs.  Then it should call this function
//           to indicate that those DLLs are associated with (used by)
//           that EXE; thus, a user request for a symbol from the EXE
//           will also search the symbolic information from those DLLs.
//
//  Output: Returns an SHE error code.  At this writing, the only legal values
//          are sheNone and sheOutOfMemory.

SHE
SHAddDllsToProcess (
    VOID
    )
{
    SHE  she = sheNone;
    HEXG hexg;

    if (!SHHexeAddNew (hpdsCur, LLLast (HlliExgExe), NULL)) {
        she = sheOutOfMemory;
    } else {
        if (she == sheNone) {
            for (hexg = LLNext (HlliExgExe, hexgNull);
                 (hexg != hexgNull) && (she == sheNone);
                 hexg = LLNext (HlliExgExe, hexg))
            {
                if (!SHHexeAddNew (hpdsCur, hexg, NULL)) {
                    she = sheOutOfMemory;
                    break;
                }
            }
        }
    }

    return she;
}


//  BuildALM
//
//  Purpose: Build an ALM (ALigned Memory) structure.  To be used later on
//           for demand symbol table loads.

LPALM
BuildALM (
    BOOL    fSeq,
    WORD    btAlign,
    LPB     lpbData,
    DWORD   cb,
    WORD    cbBlock
    )
{
    LPALM  lpalm;
    LPUFOP rgufop = NULL;

    lpalm = (LPALM) AllocAlign (sizeof(ALM) + sizeof(UFOP) + sizeof(WORD));
    if (lpalm == NULL) {
        return NULL;
    }

    lpalm->fSeq         = fSeq;
    lpalm->btAlign      = btAlign;
    lpalm->cb           = cb;
    lpalm->cbBlock      = cbBlock;
    lpalm->pbData       = lpbData;

    return lpalm;
}


PVOID
LpvFromAlmLfo (
    LPALM lpalm,
    DWORD lfo
    )
{
    if (lfo > lpalm->cb) {
        return(NULL);
    } else {
        return lpalm->pbData + lfo;
    }
}


//  GetNextSym
//
//  Purpose: Return the next symbol from a file.  Handle S_ALIGN records as
//          they're encountered.

SYMPTR
GetNextSym (
    SYMPTR psym,
    LPALM  lpalm
    )
{
    SYMPTR psymNext = (SYMPTR) (((LPB) psym) + psym->reclen + sizeof(WORD));

    if ((BYTE *) psymNext > (lpalm->pbData + lpalm->cb)) {
        return(NULL);
    }

    if (psymNext->rectyp == S_ALIGN) {
        // If we hit a S_ALIGN record, skip over it.

        psymNext = (SYMPTR) (((LPB) psymNext) + psymNext->reclen + sizeof(WORD));
    }

    return(psymNext);
}


//  GetSymbols
//
//  Purpose: Return a pointer to the symbols for a module.

PVOID
GetSymbols(
    LPMDS lpmds
    )
{
    PVOID lpvRet = NULL;

    if (lpmds->symbols) {
        lpvRet = lpmds->symbols;
    } else if (lpmds->pmod) {
        // Allocate space for this module's local symbols, then load them
        // from the PDB.

        if (ModQuerySymbols(lpmds->pmod, 0, (CB *)&lpmds->cbSymbols) &&
            (lpmds->symbols = (LPB) MHAlloc(lpmds->cbSymbols))) {

            if (ModQuerySymbols(lpmds->pmod, lpmds->symbols, (CB *)&lpmds->cbSymbols)) {
                lpvRet = lpmds->symbols;
            } else {
                MHFree(lpmds->symbols);
                lpmds->symbols = NULL;
                lpmds->cbSymbols = 0;
            }
        }
    }

    return lpvRet;
}

VOID
SHUnloadSymbolHandler(
    BOOL fResetLists
    )
{
    // Put code here to execute just before
    // the symbol handler is Freed from memory (FreeLibrary...)

    if (HlliPds) {
        LLDestroy(HlliPds);
        HlliPds = NULL;
    }

    hpdsCur = hpdsNull;

    if (HlliExgExe) {
        LLDestroy(HlliExgExe);
        HlliExgExe = NULL;
    }

    if (fResetLists) {
        // If fResetLists, the symbol handler is NOT being unloaded, but
        // the information in the linked lists are no longer valid so we
        // will have destroyed the list info, just reinitialize the lists.

        FInitLists();

    } else {
        // Otherwise, free up the last system resources.

        //  Tell the lazy loader to go away

        StopLazyLoader();

        // Release the symcvt ptr.

        if (hLib != NULL) {
            FreeLibrary(hLib);
            hLib = NULL;
        }

        // null out the function pointer
        pfConvertSymbolsForImage = NULL;

        // cleanup synchronization objects

        DeleteCriticalSection(&csSh);
#if 0
        DeleteCriticalSection(&CsSymbolLoad);
        DeleteCriticalSection(&CsSymbolProcess);
        CloseHandle(hEventLoaded);
#endif
    }
}


//  Get the time stamp of the EXE (which has the CV info)
//
//  Returns: sheFileOpen
//          sheCorruptOmf
//          sheNone

SHE
SHGetExeTimeStamp(
    LSZ szExeName,
    DWORD *lplTimeStamp
    )
{
    IMAGE_DOS_HEADER    doshdr;         // Old format MZ header
    IMAGE_FILE_HEADER   PEHeader;
    DWORD               dwMagic;
    HANDLE              hfile;
    SHE                 sheRet;

    if ((hfile = SYOpen (szExeName)) == INVALID_HANDLE_VALUE) {
        sheRet = sheFileOpen;
        goto cleanup;
    }

    // Go to beginning of file and read old EXE header

    if (SYReadFar (hfile, (LPB) &doshdr, sizeof(IMAGE_DOS_HEADER)) !=
            sizeof (IMAGE_DOS_HEADER))
    {
        sheRet = sheCorruptOmf;
        goto cleanup;
    }

    // Go to beginning of new header, read it in and verify

    if (doshdr.e_magic == IMAGE_DOS_SIGNATURE) {
        // DOS/Win16/Win32 image with stub.  See if this is a PE image
        //  and read from there.

        if (SYSeek (hfile, doshdr.e_lfanew, SEEK_SET) != doshdr.e_lfanew) {
            sheRet = sheCorruptOmf;
            goto cleanup;
        }
    } else if (doshdr.e_magic == IMAGE_NT_SIGNATURE) {
        // Win32 Image w/o a stub or a ROM image.  restart at the beginning.
        if (SYSeek(hfile, 0, SEEK_SET) != 0) {
            sheRet = sheCorruptOmf;
            goto cleanup;
        }
    }

    if ((SYReadFar (hfile, (LPB) &dwMagic, sizeof(dwMagic)) != sizeof(dwMagic)) ||
        (dwMagic != IMAGE_NT_SIGNATURE))
    {
        sheRet = sheCorruptOmf;
        goto cleanup;
    }

    // Retrieve the timestamp.

    if (SYReadFar (hfile, (LPB) &PEHeader, sizeof(IMAGE_FILE_HEADER)) !=
         sizeof(IMAGE_FILE_HEADER))
    {
        sheRet = sheCorruptOmf;
        goto cleanup;
    }

    *lplTimeStamp = PEHeader.TimeDateStamp;

cleanup:
    if (hfile != INVALID_HANDLE_VALUE) {
        SYClose (hfile);
    }
    return sheRet;
}

//  SHLszGetErrorText
//
//  Synopsis:   lsz = SHLszGetErrorText( she )
//
//  Entry:      she - error number to get text for
//
//  Returns:    pointer to the string containing the error text
//
//  Description: This routine is used by the debugger to get the
//              text for an error which occured in the symbol handler.

#define DECL_STR(s, v, t) t,
char *
RgszShError[] = {
#include "sherror.h"
};
#undef DECL_STR

LSZ
SHLszGetErrorText(
    SHE she
    )
{
    if (she < sheMax) {
        return RgszShError[she];
    }
    return RgszShError[sheMax];
}

//  SHWantSymbols
//
//  Purpose:    Loads symbols of a defered module
//
//  Input:      HEXE for which to load symbols

BOOL
SHWantSymbols(
    HEXE hexe
    )
{
    HEXG    hexg;
    LPEXE   lpexe;
    LPEXG   lpexg;
    BOOL    fRet = TRUE;

    if (!hexe) {
        fRet = FALSE;
    } else {
        lpexe = (LPEXE)LLLock(hexe);
        hexg  = lpexe->hexg;

        if (!hexg) {
            fRet = FALSE;
        } else {
            lpexg = (LPEXG)LLLock (hexg);

            if (lpexg->fOmfDefered) {
                LoadDefered(hexg);
            }

            LLUnlock (hexg);
        }

        LLUnlock (hexe);
    }

    return fRet;
}

//  SHSymbolsLoaded
//
//  Purpose:    Checks to see if the symbols are loaded for an HEXE
//
//  Input:      HEXE for which to load symbols

BOOL
SHSymbolsLoaded(
    HEXE hexe,
    SHE * pshe
    )
{
    HEXG    hexg;
    LPEXE   lpexe;
    LPEXG   lpexg;
    BOOL    fRet = FALSE;

    if (pshe != NULL) {
        *pshe = sheNoSymbols;
    }

    if (hexe) {
        lpexe = (LPEXE)LLLock(hexe);
        hexg  = lpexe->hexg;

        if (hexg) {
            lpexg = (LPEXG)LLLock (hexg);

            fRet = lpexg->fOmfLoaded;
            if (pshe != NULL) {
                *pshe = lpexg->sheLoadStatus;
            }

            LLUnlock (hexg);
        }

        LLUnlock (hexe);
    }

    return fRet;
}

//  SHSymbolsLoadError
//
//  Purpose:    Checks to see if an error ocurred while
//              loading the symbols.
//
//  Input:      HEXE for which to load symbols

BOOL
SHSymbolsLoadError(
    HEXE hexe,
    SHE * pshe
    )
{
    HEXG    hexg;
    LPEXE   lpexe;
    LPEXG   lpexg;
    BOOL    fRet = FALSE;

    if (pshe != NULL) {
        *pshe = sheNoSymbols;
    }

    if (hexe) {
        lpexe = (LPEXE)LLLock(hexe);
        hexg  = lpexe->hexg;

        if (hexg) {
            lpexg = (LPEXG)LLLock (hexg);

            fRet = lpexg->fOmfLoaded;
            if (pshe != NULL) {
                *pshe = lpexg->sheLoadError;
            }

            LLUnlock (hexg);
        }

        LLUnlock (hexe);
    }

    return fRet;
}

//  SHUnloadSymbols
//
//  Purpose:    Forces the symbols for a DLL to be unloaded
//
//  Input:      HEXE for which to unload symbols

SHE
SHUnloadSymbols(
    HEXE hexe
    )
{
    HEXG    hexg;
    LPEXE   lpexe;
    LPEXG   lpexg;
    SHE     she = sheNone;

    if (!hexe) {
        ;
    } else {
        lpexe = (LPEXE)LLLock(hexe);
        hexg  = lpexe->hexg;

        if (!hexg) {
            ;
        } else {
            lpexg = (LPEXG)LLLock (hexg);

            she = UnloadNow(hexg);

            LLUnlock (hexg);
        }

        LLUnlock (hexe);
    }

    return she;
}

//  SHSplitPath
//
//  Custom split path that allows parameters to be null

VOID
SHSplitPath (
    LSZ lszPath,
    LSZ lszDrive,
    LSZ lszDir,
    LSZ lszName,
    LSZ lszExt
    )
{
    // For 32-bit versions, there's no need for all the extra work
    _tsplitpath(lszPath, lszDrive, lszDir, lszName, lszExt);
}
