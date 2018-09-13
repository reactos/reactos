/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGMETA.C
 *  WOW32 16-bit GDI API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wgmeta.c);

typedef METAHEADER UNALIGNED *PMETAHEADER16;




// WARNING: This function may cause 16-bit memory to move
VOID CopyMetaFile16FromHMF32(HAND16 hMF16, HMETAFILE hMF32)
{
    UINT   cbMF32, cbMF16;
    VPVOID vp;
    PBYTE  pMF16;


    if((vp = GlobalLock16(hMF16, &cbMF16)) && hMF32) {

        GETMISCPTR(vp, pMF16);

        cbMF32 = GetMetaFileBitsEx(hMF32, 0, NULL);
    
        // Verify these are the same size within the 16-bit kernel memory
        // allocation granularity
        WOW32WARNMSGF((abs(cbMF16 - cbMF32) < 32),
                      ("WOW32: Size MF16 = %lu  MF32 = %lu\n", cbMF16, cbMF32));

        // copy the bits from the 32-bit metafile to the 16-bit metafile memory
        cbMF32 = GetMetaFileBitsEx(hMF32, min(cbMF16, cbMF32), pMF16);

        GlobalUnlock16(hMF16);
        FLUSHVDMPTR(vp, cbMF32, pMF16);
        FREEMISCPTR(pMF16);
    }
}




// WARNING: This function may cause 16-bit memory to move
HAND16 WinMetaFileFromHMF(HMETAFILE hmf, BOOL fFreeOriginal)
{
    UINT cbMetaData;
    VPVOID vpMetaData;
    PBYTE pMetaData;
    HAND16 h16;

    /*
     * Under Windows Metafiles were merely Global Handle to memory
     * so we have to mimick that behavior because some apps "operate"
     * on metafile handles directly.  (WinWord and PowerPoint to
     * GlobalSize and GlobalAlloc to size and create metafiles)
     */

    cbMetaData = GetMetaFileBitsEx(hmf, 0, NULL);

    if (!cbMetaData)
       return((HAND16)NULL);

    /*
     * Win 3.1 allocates extra space in MetaFile and OLE2 checks for this.
     * METAHEADER is defined to be the same size as the 16-bit structure.
     */

    cbMetaData += sizeof(METAHEADER);

    vpMetaData = GlobalAllocLock16(GMEM_MOVEABLE | GMEM_DDESHARE, cbMetaData, &h16);

    if (!vpMetaData)
       return((HAND16)NULL);


    GETOPTPTR(vpMetaData, 0, pMetaData);

    if (GetMetaFileBitsEx(hmf, cbMetaData, pMetaData)) {
       GlobalUnlock16(h16);
    } else {
       GlobalUnlockFree16(vpMetaData);
       return((HAND16)NULL);
    }

    if (fFreeOriginal)
        DeleteMetaFile(hmf);

    return(h16);
}

HMETAFILE HMFFromWinMetaFile(HAND16 h16, BOOL fFreeOriginal)
{
    INT cb;
    VPVOID vp;
    HMETAFILE hmf = (HMETAFILE)0;
    PMETAHEADER16 pMFH16;

    vp = GlobalLock16(h16, &cb);

    if (vp) {
        GETMISCPTR(vp, pMFH16);

        hmf = SetMetaFileBitsEx(cb, (LPBYTE)pMFH16);

        if (fFreeOriginal)
            GlobalUnlockFree16(vp);
        else
            GlobalUnlock16(h16);

        FREEMISCPTR(pMFH16);
    }

    return(hmf);
}


ULONG FASTCALL WG32CloseMetaFile(PVDMFRAME pFrame)
{
    HMETAFILE hmf;
    ULONG ulRet = 0;
    register PCLOSEMETAFILE16 parg16;

    GETARGPTR(pFrame, sizeof(CLOSEMETAFILE16), parg16);

    hmf = CloseMetaFile(HDC32(parg16->f1));

    if (hmf)
	ulRet = (ULONG)WinMetaFileFromHMF(hmf, TRUE);
    // WARNING: 16-bit memory may have moved - invalidate flat pointers now
    FREEVDMPTR(pFrame);
    FREEARGPTR(parg16);
    RETURN(ulRet);
}


ULONG FASTCALL WG32CopyMetaFile(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    HMETAFILE hmfNew;
    HMETAFILE hmf;
    register PCOPYMETAFILE16 parg16;

    GETARGPTR(pFrame, sizeof(COPYMETAFILE16), parg16);
    GETPSZPTR(parg16->f2, psz2);

    if (psz2) {
        hmf = HMFFromWinMetaFile(parg16->f1, FALSE);
        hmfNew = CopyMetaFile(hmf, psz2);
        DeleteMetaFile(hmf);
	ul = (ULONG)WinMetaFileFromHMF(hmfNew, TRUE);
	// WARNING: 16-bit memory may have moved - invalidate flat pointers now
	FREEVDMPTR(pFrame);
	FREEARGPTR(parg16);
	FREEPSZPTR(psz2);
    } else {
        UINT cb;
        VPVOID vp, vpNew;
        PBYTE pMF, pMFNew;
        HAND16 h16New, h16;

        h16 = (HAND16)parg16->f1;

        ul = (ULONG) NULL;

        vp = GlobalLock16(h16, &cb);
        if (vp) {

        /*
         * Windows app such as WinWord uses GlobalSize to determine
         * the size of the metafile.  However, this size can be larger
         * than the true size of a metafile.  We have to make sure that
         * both source and destination sizes are identical so that
         * WinWord doesn't crash.
         */

	    vpNew = GlobalAllocLock16(GMEM_MOVEABLE | GMEM_DDESHARE, cb, &h16New);

	    // 16-bit memory may have moved - invalidate flat pointers now
	    FREEVDMPTR(pFrame);
	    FREEARGPTR(parg16);
	    FREEPSZPTR(psz2);

            if (vpNew) {
		GETMISCPTR(vp, pMF);
                GETOPTPTR(vpNew, 0, pMFNew);

                RtlCopyMemory(pMFNew, pMF, cb);

                GlobalUnlock16(h16New);
                FLUSHVDMPTR(vpNew, cb, pMFNew);
                FREEOPTPTR(pMFNew);
                ul = h16New;
            }

            GlobalUnlock16(h16);
            FREEMISCPTR(pMF);
        }
    }

    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32CreateMetaFile(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    register PCREATEMETAFILE16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEMETAFILE16), parg16);
    GETPSZPTR(parg16->f1, psz1);

    ul = GETHDC16(CreateMetaFile(psz1));

    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(ul);
}

//
// This routine does what the 16-bit parameter validation layer would
// normally do for metafile handles, but since it is currently disabled,
// we'll do it here to fix WordPerfect that relies on it. Once true
// win31-style parameter validation has been re-enabled for metafile
// handles, all code within the ifndefs here and in WG32DeleteMetaFile
// can be removed.
//
#ifndef PARAMETER_VALIDATION_16_RE_ENABLED
#define MEMORYMETAFILE 1
#define DISKMETAFILE 2
#define HEADERSIZE          (sizeof(METAHEADER)/sizeof(WORD))
#define METAVERSION         0x0300
#define METAVERSION100      0x0100

BOOL IsValidMetaFile16(PMETAHEADER16 lpMetaData)
{
    BOOL            sts = FALSE;

        sts = (lpMetaData->mtType == MEMORYMETAFILE ||
           lpMetaData->mtType == DISKMETAFILE) &&
              (lpMetaData->mtHeaderSize == HEADERSIZE) &&
              ((lpMetaData->mtVersion ==METAVERSION) ||
           (lpMetaData->mtVersion ==METAVERSION100)) ;
    return sts;
}
#endif

ULONG FASTCALL WG32DeleteMetaFile(PVDMFRAME pFrame)
{
    ULONG ul = FALSE;
    VPVOID vp;
#ifndef PARAMETER_VALIDATION_16_RE_ENABLED
    PMETAHEADER16 lpMetaData;
#endif

    register PDELETEMETAFILE16 parg16;

    GETARGPTR(pFrame, sizeof(DELETEMETAFILE16), parg16);

    if (vp = GlobalLock16(parg16->f1,NULL)) {
#ifdef PARAMETER_VALIDATION_16_RE_ENABLED
        GlobalUnlockFree16(vp);
        ul = TRUE;
#else
        GETVDMPTR(vp, 1, lpMetaData);

        if (IsValidMetaFile16(lpMetaData)) {
            GlobalUnlockFree16(vp);
            ul = TRUE;
        }

        FREEVDMPTR(lpMetaData);
#endif
    }


    // If this metafile was in DDE conversation, then DDE cleanup code
    // needs to free its 32 bit counter part. So give DDE clean up
    // code a chance.
    // ChandanC

    W32DdeFreeHandle16 (parg16->f1);

    FREEARGPTR(parg16);
    RETURN(ul);
}

INT WG32EnumMetaFileCallBack(HDC hdc, LPHANDLETABLE lpht, LPMETARECORD lpMR, LONG nObj, PMETADATA pMetaData )
{
    INT iReturn;
    DWORD nWords;

    // update object table if we have one
    if (pMetaData->parmemp.vpHandleTable)
        PUTHANDLETABLE16(pMetaData->parmemp.vpHandleTable,nObj,lpht);

    // update MetaRecord

    // don't trash the heap with a bogus record, halt the enumeration
    nWords = lpMR->rdSize;
    if (nWords > pMetaData->mtMaxRecordSize) {
        LOGDEBUG(0,("WOW:bad metafile record during enumeration\n"));
        WOW32ASSERT(FALSE); // contact barryb
        return 0;   // all done
    }
    putstr16(pMetaData->parmemp.vpMetaRecord, (LPSZ)lpMR, nWords*sizeof(WORD));

    CallBack16(RET_ENUMMETAFILEPROC, (PPARM16)&pMetaData->parmemp, pMetaData->vpfnEnumMetaFileProc, (PVPVOID)&iReturn);

    // update the metarec in case the app altered it (Approach does)
    getstr16(pMetaData->parmemp.vpMetaRecord, (LPSZ)lpMR, nWords*sizeof(WORD));

    // update object table if we have one
    if (pMetaData->parmemp.vpHandleTable)
        GETHANDLETABLE16(pMetaData->parmemp.vpHandleTable,nObj,lpht);

    return (SHORT)iReturn;

    hdc;    // quiet the compilier; we already know the DC
}

ULONG FASTCALL WG32EnumMetaFile(PVDMFRAME pFrame)
{
    ULONG       ul = 0;
    register    PENUMMETAFILE16 parg16;
    METADATA    metadata;
    VPVOID      vpMetaFile = (VPVOID) NULL;
    PBYTE       pMetaFile;
    HMETAFILE   hmf = (HMETAFILE) 0;
    HAND16      hMetaFile16;
    HDC 	hDC = 0;

    GETARGPTR(pFrame, sizeof(ENUMMETAFILE16), parg16);

    hMetaFile16 = parg16->f2;

    metadata.vpfnEnumMetaFileProc = DWORD32(parg16->f3);
    metadata.parmemp.vpData = (VPVOID)DWORD32(parg16->f4);
    metadata.parmemp.vpMetaRecord = (VPVOID) NULL;
    metadata.parmemp.vpHandleTable = (VPVOID) NULL;
    metadata.parmemp.hdc = parg16->f1;

    // WinWord never calls SetMetaFileBits; they peeked and know that
    // a metafile is really a GlobalHandle in Windows so we have
    // to look for that case.

    hmf = HMFFromWinMetaFile(hMetaFile16, FALSE);
    if (!hmf)
        goto EMF_Exit;

    // Get the metafile bits so we can get max record size and number of objects

    vpMetaFile = GlobalLock16(hMetaFile16, NULL);
    FREEARGPTR(parg16); 	// memory may have moved
    FREEVDMPTR(pFrame);
    if (!vpMetaFile)
        goto EMF_Exit;

    GETOPTPTR(vpMetaFile, 0, pMetaFile);
    if (!pMetaFile)
        goto EMF_Exit;

    metadata.parmemp.nObjects = ((PMETAHEADER16)pMetaFile)->mtNoObjects;
    metadata.mtMaxRecordSize = ((PMETAHEADER16)pMetaFile)->mtMaxRecord;

    if (metadata.parmemp.nObjects)
    {
	PBYTE pHT;
	DWORD cb = ((PMETAHEADER16)pMetaFile)->mtNoObjects*sizeof(HAND16);

	metadata.parmemp.vpHandleTable = GlobalAllocLock16(GMEM_MOVEABLE, cb, NULL);
	FREEOPTPTR(pMetaFile);	 // memory may have moved
	FREEARGPTR(parg16);
	FREEVDMPTR(pFrame);
        if (!metadata.parmemp.vpHandleTable)
            goto EMF_Exit;

        GETOPTPTR(metadata.parmemp.vpHandleTable, 0, pHT);
	RtlZeroMemory(pHT, cb);
    }

    metadata.parmemp.vpMetaRecord = GlobalAllocLock16(GMEM_MOVEABLE, metadata.mtMaxRecordSize*sizeof(WORD), NULL);
    FREEOPTPTR(pMetaFile);	 // memory may have moved
    FREEARGPTR(parg16);
    FREEVDMPTR(pFrame);
    if (!metadata.parmemp.vpMetaRecord)
        goto EMF_Exit;

    // Corel Draw passes a NULL hDC, we'll create a dummy to keep GDI32 happy.
    if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_GETDUMMYDC) {
	if ((hDC = HDC32(metadata.parmemp.hdc)) == 0) {
            hDC = CreateMetaFile(NULL);
        }
    }
    else {
	hDC = HDC32(metadata.parmemp.hdc);
    }

    ul = GETBOOL16(EnumMetaFile(hDC,
                                hmf,
                                (MFENUMPROC)WG32EnumMetaFileCallBack,
				((LPARAM)(LPVOID)&metadata)));
    // 16-bit memory may have moved - nothing to do as no flat ptrs exist now

    // copy the 32-bit metafile back to 16-bit land (the app may have altered
    // some of the metarecs in its MetaRecCallBackFunc -- Approach does)
    CopyMetaFile16FromHMF32(hMetaFile16, hmf);

    // Cleanup the dummy hDC created for Corel Draw 5.0.
    if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_GETDUMMYDC) {
	if (HDC32(metadata.parmemp.hdc) == 0) {
            DeleteMetaFile(CloseMetaFile(hDC));
        }
    }

EMF_Exit:
    if (vpMetaFile)
        GlobalUnlock16(hMetaFile16);

    if (hmf)
        DeleteMetaFile(hmf);

    if (metadata.parmemp.vpHandleTable)
        GlobalUnlockFree16(metadata.parmemp.vpHandleTable);

    if (metadata.parmemp.vpMetaRecord)
        GlobalUnlockFree16(metadata.parmemp.vpMetaRecord);

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetMetaFile(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    HMETAFILE hmf;
    register PGETMETAFILE16 parg16;

    GETARGPTR(pFrame, sizeof(GETMETAFILE16), parg16);
    GETPSZPTR(parg16->f1, psz1);

    hmf = GetMetaFile(psz1);

    if (hmf)
        ul = WinMetaFileFromHMF(hmf, TRUE);
    else
        ul = 0;

    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32PlayMetaFile(PVDMFRAME pFrame)
{
    ULONG ul;
    HMETAFILE hmf;
    register PPLAYMETAFILE16 parg16;

    GETARGPTR(pFrame, sizeof(PLAYMETAFILE16), parg16);

    hmf = HMFFromWinMetaFile(parg16->f2, FALSE);

    ul = GETBOOL16(PlayMetaFile(HDC32(parg16->f1), hmf));

    if (hmf)
        DeleteMetaFile(hmf);

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32PlayMetaFileRecord(PVDMFRAME pFrame)
{
    ULONG ul = FALSE;
    LPHANDLETABLE pHT = NULL;
    PBYTE pMetaData;
    WORD wHandles;
    VPHANDLETABLE16 vpHT;
    register PPLAYMETAFILERECORD16 parg16;

    GETARGPTR(pFrame, sizeof(PLAYMETAFILERECORD16), parg16);

    wHandles = parg16->f4;
    vpHT     = parg16->f2;
    if (wHandles && vpHT) {
        ALLOCHANDLETABLE16(wHandles, pHT);
        if (!pHT)
            goto PMFR_Exit;

        GETHANDLETABLE16(vpHT, wHandles, pHT);
    }
    GETOPTPTR(parg16->f3, 0, pMetaData);

    ul = (ULONG) PlayMetaFileRecord(HDC32(parg16->f1),
                                    pHT,
                                    (LPMETARECORD)pMetaData,
                                    (UINT)wHandles);


    if (wHandles && vpHT) {
        PUTHANDLETABLE16(vpHT, wHandles, pHT);
        FREEHANDLETABLE16(pHT);
    }
PMFR_Exit:
    FREEARGPTR(parg16);
    RETURN(ul);
}

#if 0  // implemented in gdi.exe

ULONG FASTCALL WG32GetMetaFileBits(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PGETMETAFILEBITS16 parg16;

    GETARGPTR(pFrame, sizeof(GETMETAFILEBITS16), parg16);

    if (GlobalLock16(parg16->f1,NULL))
    {
        GlobalUnlock16(parg16->f1);
        ul = parg16->f1;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}

ULONG FASTCALL WG32SetMetaFileBits(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETMETAFILEBITS16 parg16;

    GETARGPTR(pFrame, sizeof(SETMETAFILEBITS16), parg16);

    ul = parg16->f1;

    FREEARGPTR(parg16);
    RETURN(ul);
}

#endif
