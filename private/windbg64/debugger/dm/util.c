/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This file contains a set of general utility routines for the
    Debug Monitor module

Author:

    Jim Schaad (jimsch) 9-12-92

Environment:

    Win32 user mode

--*/

#include "precomp.h"
#pragma hdrstop



extern EXPECTED_EVENT   masterEE, *eeList;

extern HTHDX        thdList;
extern HPRCX        prcList;
extern CRITICAL_SECTION csThreadProcList;
extern char  abEMReplyBuf[];      // Buffer for EM to reply to us in


static  HPRCX   HprcRead;
static  HANDLE  HFileRead = 0;          // Read File handle
static  DWORDLONG QwMemory = 0;          // Read File Address
static  ULONG   CbOffset = 0;           // Offset of read address



BOOL
AddrWriteMemory(
    HPRCX       hprc,
    HTHDX       hthd,
    LPADDR      paddr,
    LPVOID      lpv,
    DWORD       cb,
    LPDWORD     pcbWritten
    )
/*++

Routine Description:

    This function is used to do a verified write to memory.  Most of the
    time it will just do a simple call to WriteMemory but some times
    it will do validations of writes.

Arguments:

    hprc - Supplies the handle to the process

    paddr  - Supplies the address to be written at

    lpv    - Supplies a pointer to the bytes to be written

    cb     - Supplies the count of bytes to be written

    pcbWritten - Returns the number of bytes actually written

Return Value:

    TRUE if successful and FALSE otherwise

--*/

{
    BOOL        fRet;
    ADDR        addr;

    /*
     * Can't resolve linker indices from here.
     */

    assert(!(ADDR_IS_LI(*paddr)));
    if (ADDR_IS_LI(*paddr)) {
        return FALSE;
    }

    /*
     * Make a local copy to mess with
     */

    addr = *paddr;
    if (!ADDR_IS_FLAT(addr)) {
        fRet = TranslateAddress(hprc, hthd, &addr, TRUE);
        //assert(fRet);
        if (!fRet) {
            return fRet;
        }
    }

    return DbgWriteMemory(hprc,
                          GetAddrOff(addr),
                          lpv,
                          cb,
                          pcbWritten);

}                               /* AddrWriteMemory() */


BOOL
AddrReadMemory(
    HPRCX       hprc,
    HTHDX       hthd,
    LPADDR      paddr,
    LPVOID      lpv,
    DWORD       cb,
    LPDWORD     lpRead
    )
/*++

Routine Description:

    Read data from a process, using a full ADDR packet.

Arguments:

    hprc - Supplies the process structure

    hthd - Supplies the thread structure.  This must be valid if the
            address is not flat; otherwise the thread is not used.

    paddr  - Supplies the address to read from

    lpv    - Supplies a pointer to the local buffer

    cb     - supplies the count of bytes to read

    lpRead - Returns the number of bytes actually read

Return Value:

    TRUE if successful and FALSE otherwise

--*/

{
    BOOL        fRet;
    ADDR        addr;
#ifndef KERNEL
    PBREAKPOINT bp;
    DWORD       offset;
    BP_UNIT     instr;
#endif

    /*
     * We can't resolve linker indices from here.
     */
    DPRINT(1,("AddrReadMemory @%p: Flat:%i Off32:%i Li:%i Re:%i\n",
           GetAddrOff(*paddr),
           ADDR_IS_FLAT(*paddr),
           ADDR_IS_OFF32(*paddr),
           ADDR_IS_LI(*paddr),
           ADDR_IS_REAL(*paddr)
           ));

    assert(!(ADDR_IS_LI(*paddr)));
    if (ADDR_IS_LI(*paddr)) {
        return FALSE;
    }

    /*
     * Make a local copy to mess with
     */

    addr = *paddr;
    if (!ADDR_IS_FLAT(addr)) {
        fRet = TranslateAddress(hprc, hthd, &addr, TRUE);
        //assert(fRet);
        if (!fRet) {
            return fRet;
        }
    }

    if (!DbgReadMemory(hprc, GetAddrOff(addr), lpv, cb, lpRead)) {
        return FALSE;
    }

#ifndef KERNEL
    /* The memory has been read into the buffer now sanitize it : */
    /* (go through the entire list of breakpoints and see if any  */
    /* are in the range. If a breakpoint is in the range then an  */
    /* offset relative to the start address and the original inst */
    /* ruction is returned and put into the return buffer)        */

    for (bp=bpList->next; bp; bp=bp->next) {
        if (BPInRange(hprc, hthd, bp, &addr, *lpRead, &offset, &instr)) {
            if (offset < 0) {
                memcpy(lpv, ((char *) &instr) - offset,
                       sizeof(BP_UNIT) + offset);
            } else if (offset + sizeof(BP_UNIT) > *lpRead) {
                memcpy(((char *)lpv)+offset, &instr, *lpRead - offset);
            } else {
                *((BP_UNIT UNALIGNED *)((char *)lpv+offset)) = instr;
            }
#if defined (TARGET_IA64)
            // restore template to MLI if displaced instruction is MOVL and in range
            if(((bp->flags & BREAKPOINT_IA64_MOVL) && (offset >= 0)) && ((GetAddrOff(bp->addr) & 0xf) == 4)) {
                *((char *)lpv + offset - 4) &= ~(0x1e);
                *((char *)lpv + offset - 4) |= 0x4;
            }
#endif // TARGET_IA64
        }
    }
#endif  // !KERNEL

    return TRUE;
}                               /* AddrReadMemory() */


#if 0
BOOL
SanitizedMemoryRead(
    HPRCX      hprc,
    HTHDX      hthd,
    LPADDR     paddr,
    LPVOID     lpb,
    DWORD      cb,
    LPDWORD    lpcb
    )

/*++

Routine Description:

    This routine is provided to do the actual read of memory.  This allows
    multiple routines in the DM to do the read through a single common
    interface.  This routine will correct the read memory for any breakpoints
    currently set in memory.

Arguments:

    hprc        - Supplies the process handle for the read

    hthd        - Supplies the thread handle for the read

    paddr       - Supplies the address to read memory from

    lpb         - Supplies the buffer to do the read into

    cb          - Supplies the number of bytes to be read

    lpcb        - Returns the number of bytes actually read

Return Value:

    TRUE on success and FALSE on failure

--*/

{
    DWORD       offset;
    BP_UNIT     instr;
    BREAKPOINT  *bp;

    if (!AddrReadMemory(hprc, hthd, paddr, lpb, cb, lpcb)) {
        return FALSE;
    }

#ifndef KERNEL
    /* The memory has been read into the buffer now sanitize it : */
    /* (go through the entire list of breakpoints and see if any  */
    /* are in the range. If a breakpoint is in the range then an  */
    /* offset relative to the start address and the original inst */
    /* ruction is returned and put into the return buffer)        */

    for (bp=bpList->next; bp; bp=bp->next) {
        if (BPInRange(hprc, hthd, bp, paddr, *lpcb, &offset, &instr)) {
            if (offset < 0) {
                memcpy(lpb, ((char *) &instr) - offset,
                       sizeof(BP_UNIT) + offset);
            } else if (offset + sizeof(BP_UNIT) > *lpcb) {
                memcpy(((char *)lpb)+offset, &instr, *lpcb - offset);
            } else {
                *((BP_UNIT UNALIGNED *)((char *)lpb+offset)) = instr;
            }
#ifdef TARGET_IA64
            // restore template to MLI if displaced instruction is MOVL and in range
            if(((bp->flags & BREAKPOINT_IA64_MOVL) && (offset >= 0)) && ((GetAddrOff(bp->addr) & 0xf) == 4)) {
                *((char *)lpv + offset - 4) &= ~(0x1e);
                *((char *)lpv + offset - 4) |= 0x4;
            }
#endif // TARGET_IA64
        }
    }
#endif  // !KERNEL

    return TRUE;
}

#endif


ULONG
SetReadPointer(
    ULONG    cbOffset,
    int      iFrom
    )

/*++

Routine Description:

    This routine is used to deal with changing the location of where
    the next read should occur.  This will take effect on the current
    file pointer or debuggee memory pointer address.

Arguments:

    cbOffset    - Supplies the offset to set the file pointer at

    iFrom       - Supplies the type of set to be preformed.

Return Value:

    The new file offset

--*/

{
    if (QwMemory == 0) {
        CbOffset = SetFilePointer(HFileRead, cbOffset, NULL, iFrom);
    } else {
        switch( iFrom ) {
        case FILE_BEGIN:
            CbOffset = cbOffset;
            break;

        case FILE_CURRENT:
            CbOffset += cbOffset;
            break;

        default:
            assert(FALSE);
            break;
        }
    }

    return CbOffset;
}                               /* SetReadPointer() */


VOID
SetPointerToFile(
    HANDLE   hFile
    )

/*++

Routine Description:

    This routine is called to specify which file handle should be used for
    doing reads from

Arguments:

    hFile - Supplies the file handle to do future reads from

Return Value:

    None.

--*/

{
    HFileRead = hFile;
    HprcRead = NULL;
    QwMemory = 0;

    return;
}                               /* SetPointerToFile() */



VOID
SetPointerToMemory(
    HPRCX       hprc,
    DWORDLONG   qw
    )

/*++

Routine Description:

    This routine is called to specify where in debuggee memory reads should
    be done from.

Arguments:

    hProc - Supplies the handle to the process to read memory from

    lpv   - Supplies the base address of the dll to read memory at.

Return Value:

    None.

--*/

{
    HprcRead = hprc;
    QwMemory = qw;
    HFileRead = NULL;

    return;
}                               /* SetPointerToMemory() */


BOOL
DoRead(
    LPVOID           lpv,
    DWORD            cb
    )

/*++

Routine Description:

    This routine is used to preform the actual read operation from either
    a file handle or from the dlls memory.

Arguments:

    lpv - Supplies the pointer to read memory into

    cb  - Supplies the count of bytes to be read

Return Value:

    TRUE If read was fully successful and FALSE otherwise

--*/

{
    DWORD       cbRead;

    if (QwMemory) {
        if ( !DbgReadMemory( HprcRead, QwMemory+CbOffset, lpv, cb, &cbRead ) ||
                (cb != cbRead) ) {
            return FALSE;
        }
        CbOffset += cb;
    } else if ((ReadFile(HFileRead, lpv, cb, &cbRead, NULL) == 0) ||
            (cb != cbRead)) {
        return FALSE;
    }
    return TRUE;
}                               /* DoRead() */



BOOL
AreAddrsEqual(
    HPRCX     hprc,
    HTHDX     hthd,
    LPADDR    paddr1,
    LPADDR    paddr2
    )

/*++

Routine Description:

    This function is used to compare to addresses for equality

Arguments:

    hprc    - Supplies process for address context

    hthd    - Supplies thread for address context

    paddr1  - Supplies a pointer to an ADDR structure

    paddr2  - Supplies a pointer to an ADDR structure

Return Value:

    TRUE if the addresses are equivalent

--*/

{
    ADDR        addr1;
    ADDR        addr2;

    /*
     *  Step 1.  Addresses are equal if
     *          - Both addresses are flat
     *          - The two offsets are the same
     */

    if ((ADDR_IS_FLAT(*paddr1) == TRUE) &&
        (ADDR_IS_FLAT(*paddr1) == ADDR_IS_FLAT(*paddr2)) &&
        (paddr1->addr.off == paddr2->addr.off)) {
        return TRUE;
    }

    /*
     * Step 2.  Address are equal if the linear address are the same
     */

    addr1 = *paddr1;
    addr2 = *paddr2;

    if (addr1.addr.off == addr2.addr.off) {
        return TRUE;
    }

    return FALSE;
}                               /* AreAddrsEqual() */




HTHDX
HTHDXFromPIDTID(
    PID pid,
    TID tid
    )
{
    HTHDX hthd;

    EnterCriticalSection(&csThreadProcList);
    for ( hthd = thdList->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse;
          hthd;
          hthd = hthd->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse ) {
        if (hthd->tid == tid && hthd->hprc->pid == pid ) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);
    return hthd;
}



HTHDX
HTHDXFromHPIDHTID(
    HPID hpid,
    HTID htid
    )
{
    HTHDX hthd;

    EnterCriticalSection(&csThreadProcList);
    for (hthd = thdList->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse;
         hthd;
         hthd = hthd->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse) {
        if (hthd->htid == htid && hthd->hprc->hpid == hpid ) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);
    return hthd;
}




HPRCX
HPRCFromPID(
    PID pid
    )
{
    HPRCX hprc;

    EnterCriticalSection(&csThreadProcList);
    for( hprc = prcList->next; hprc; hprc = hprc->next) {
        if (hprc->pid == pid) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);
    return hprc;
}



HPRCX
HPRCFromHPID(
    HPID hpid
    )
{
    HPRCX hprc;

    EnterCriticalSection(&csThreadProcList);
    for ( hprc = prcList->next; hprc; hprc = hprc->next ) {
        if (hprc->hpid == hpid) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);
    return hprc;
}



HPRCX
HPRCFromRwhand(
    HANDLE rwHand
    )
{
    HPRCX hprc;

    EnterCriticalSection(&csThreadProcList);
    for ( hprc=prcList->next; hprc; hprc=hprc->next ) {
        if (hprc->rwHand==rwHand) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);
    return hprc;
}


void
FreeHthdx(
    HTHDX hthd
    )
{
    HTHDX *             ppht;
    BREAKPOINT *        pbp;
    BREAKPOINT *        pbpT;

    EnterCriticalSection(&csThreadProcList);

    /*
     *  Free all breakpoints unique to thread
     */

    for (pbp = BPNextHthdPbp(hthd, NULL); pbp; pbp = pbpT) {
        pbpT = BPNextHthdPbp(hthd, pbp);
        RemoveBP(pbp);
    }


    for (ppht = &(hthd->hprc->hthdChild); *ppht; ppht = & ( (*ppht)->nextSibling ) ) {
        if (*ppht == hthd) {
            *ppht = (*ppht)->nextSibling;
            break;
        }
    }

    for (ppht = &(thdList->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse);
         *ppht;
         ppht = & ( (*ppht)->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse ) ) {
        if (*ppht == hthd) {
            *ppht = (*ppht)->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse;
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);

    MHFree(hthd);
}


VOID
ClearContextPointers(
    PKNONVOLATILE_CONTEXT_POINTERS ctxptrs
    )
/*++

  Routine -  Clear Context Pointers

  Purpose - clears the context pointer structure.

  Argument - lpvoid - pointer to context pointers structure;
             void on on architectures that don't have such.

--*/

{
    memset(ctxptrs, 0, sizeof (KNONVOLATILE_CONTEXT_POINTERS));
}


/*** FGETEXPORT
 *
 * PURPOSE:
 *        Given the base address of a DLL in the debuggee's memory space, find
 *        the value (if any) of a specified export in that DLL.
 *
 * INPUT:
 *        pdi        DLLLOAD_ITEM structure for module
 *        hfile      Handle to disk file of DLL
 *        szExport   Name of symbol to search for in its export table
 *
 * OUPTUT:
 *        *plpvValue Address of the symbol, from the export table.
 *                        plpvValue may be NULL if caller doesn't care.
 *        return code        TRUE if symbol was found, FALSE if not
 */


BOOL
FGetExport(
    PDLLLOAD_ITEM pdi,
    HFILE       hfile,
    LPCTSTR     szExport,
    LPVOID*     plpvValue
    )
{
    IMAGE_DOS_HEADER        doshdr;
    IMAGE_NT_HEADERS        nthdr;
    IMAGE_EXPORT_DIRECTORY  exphdr;
    LONG                    inameFirst, inameLast, iname;    // must be signed
    UOFFSET                 uoffExpTable;
    UOFFSET                 uoffNameTable;
    UOFFSET                 uoffFuncTable;
    UOFFSET                 uoffOrdinalTable;
    UOFFSET                 uoffString;
    INT                     iRet = 1;
    size_t                  cbValueRead = _ftcslen(szExport) + 1;
    LPTSTR                  szValueRead = MHAlloc( cbValueRead * sizeof(TCHAR));
    UOFFSET                 uoffBasePE = (UOFFSET)0;
    DWORD                   dw;

    /*
    ** Check for both initial MZ (oldheader) or initial PE header.
    */

    VERIFY(CbReadDllHdr(hfile, 0, &doshdr, sizeof(doshdr)) == sizeof(doshdr));

    if ( doshdr.e_magic != IMAGE_DOS_SIGNATURE ) {
        return FALSE;
    }

    uoffBasePE = doshdr.e_lfanew;

    VERIFY(CbReadDllHdr(hfile, uoffBasePE, &nthdr, sizeof(nthdr)) ==
        sizeof(nthdr));

    uoffExpTable = nthdr.OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    if (uoffExpTable == 0) {
        return FALSE;
    }

    uoffExpTable = FileOffFromVA(pdi, hfile, uoffBasePE, &nthdr, uoffExpTable);

    VERIFY(CbReadDllHdr(hfile, uoffExpTable, &exphdr, sizeof(exphdr)) ==
        sizeof(exphdr));

    if ( exphdr.NumberOfNames == 0L ) {
        return FALSE;
    }

    uoffNameTable = FileOffFromVA(pdi, hfile, uoffBasePE, &nthdr,
        (UOFFSET)exphdr.AddressOfNames);

    // Do a binary search through the export table
    inameFirst = 0;
    inameLast = exphdr.NumberOfNames - 1;
    while (inameFirst <= inameLast) {
        iname = (inameFirst + inameLast) / 2;

        VERIFY(CbReadDllHdr(hfile,
                            uoffNameTable + (iname * sizeof(DWORD)),
                            &dw,
                            sizeof(DWORD)) == sizeof(DWORD));

        uoffString = FileOffFromVA(pdi, hfile, uoffBasePE, &nthdr, dw);

        VERIFY(CbReadDllHdr(hfile, uoffString, szValueRead, cbValueRead) == cbValueRead);

        iRet = _ftcsncmp( szValueRead, szExport, cbValueRead );

        if (iRet < 0) {
            inameFirst = iname + 1;
        } else if (iRet > 0) {
            inameLast = iname - 1;
        } else /* iRet == 0: match */ {
            /* if caller wants its value, get value */
            if (plpvValue) {
                USHORT            usOrdinal;

                /* read symbol value from export table */
                uoffOrdinalTable = FileOffFromVA(pdi, hfile, uoffBasePE,
                    &nthdr, (UOFFSET)exphdr.AddressOfNameOrdinals);
                VERIFY(CbReadDllHdr(hfile,
                    uoffOrdinalTable + (iname * sizeof(USHORT)),
                    &usOrdinal, sizeof(usOrdinal)) == sizeof(usOrdinal));

                uoffFuncTable = FileOffFromVA(pdi, hfile, uoffBasePE,
                    &nthdr, (UOFFSET)exphdr.AddressOfFunctions);
                VERIFY(CbReadDllHdr(hfile,
                    uoffFuncTable + (usOrdinal * sizeof(DWORD)),
                    plpvValue, sizeof(*plpvValue)) == sizeof(*plpvValue));

                assert( Is64PtrSE(pdi->offBaseOfImage) );

                *plpvValue = (LPVOID) ((UOFFSET)*plpvValue + pdi->offBaseOfImage);
            }
            break;
        }
    }
    MHFree( szValueRead );
    return !iRet;

}


UOFFSET
FileOffFromVA(
    PDLLLOAD_ITEM           pdi,
    HFILE                   hfile,
    UOFFSET                 uoffBasePE,
    const IMAGE_NT_HEADERS *pnthdr,
    UOFFSET                 va
    )

/*++

Routine Description:

    Given a virtual address, calculate the file offset at which it
    can be found in an EXE/DLL.

Arguments:

    pdi - Supplies the DLLLOAD_ITEM structure for this exe/dll

    hfile - Supplies a read handle to the exe/dll file

    uoffBasePE - Supplies offset of beginning of PE header in exe/dll

    pnthdr - Supplies ptr to NTHDR for the exe/dll

    va - Supplies virtual address to convert

Return Value:

    the file offset for the given va, 0 for failure

--*/

{
    UOFFSET                 uoffObjs;
    WORD                    iobj, cobj;
    IMAGE_SECTION_HEADER    isecthdr;
    PIMAGE_SECTION_HEADER   psect;
    UOFFSET                 uoffFile = 0;

    uoffObjs = uoffBasePE +
               FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) +
               pnthdr->FileHeader.SizeOfOptionalHeader;

    cobj = pnthdr->FileHeader.NumberOfSections;

    /*
     * If we have not yet read the section headers into the DLLINFO, do
     * so now.
     */
    if (pdi->Sections == NULL) {
        pdi->Sections = MHAlloc(cobj * sizeof(IMAGE_SECTION_HEADER));

        VERIFY(
        CbReadDllHdr(hfile, uoffObjs, pdi->Sections, cobj * sizeof(IMAGE_SECTION_HEADER))
        );
    }

    /*
     * Look for the address.
     */
    for (iobj=0; iobj<cobj; iobj++) {
        DWORD    offset, cbObject;

        offset = pdi->Sections[iobj].VirtualAddress;
        cbObject = pdi->Sections[iobj].Misc.VirtualSize;
        if (cbObject == 0) {
            cbObject = pdi->Sections[iobj].SizeOfRawData;
        }

        if (va >= offset && va < offset + cbObject) {
            // found it
            uoffFile = pdi->Sections[iobj].PointerToRawData + va - offset;
            break;
        }
    }
    assert(uoffFile);    // caller shouldn't have called with a bogus VA
    return uoffFile;
}


DWORD
CbReadDllHdr(
    HFILE hfile,
    UOFFSET uoff,
    LPVOID lpvBuf,
    DWORD cb
    )
{
    VERIFY(_llseek(hfile, (DWORD)uoff, 0) != HFILE_ERROR);
    return _lread(hfile, lpvBuf, cb);
}


XOSD
DMSendRequestReply (
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD cbInput,
    LPVOID lpInput,
    DWORD cbOutput,
    LPVOID lpOutput
    )
/*++

Routine Description:


Arguments:

    dbc - Supplies command to send to EM

    hpid - Supplies process handle

    htis - Supplies thread handle

    cbInput - Supplies size of packet to send

    lpInput - Supplies packet to send

    cbOutput - Supplies size of packet to receive

    lpOutput - Returns data from reply

Return Value:

    xosdNone or xosd error value

--*/
{
    XOSD xosd = xosdNone;

    if ( cbInput == 0 ) {
        RTP rtp;

        rtp.dbc  = dbc;
        rtp.hpid = hpid;
        rtp.htid = htid;
        rtp.cb   = 0;
        xosd = DmTlFunc
            ( tlfRequest, hpid, FIELD_OFFSET( RTP, rgbVar ), (LPARAM) &rtp );
    }
    else {
        LPRTP lprtp = MHAlloc ( FIELD_OFFSET( RTP, rgbVar ) + cbInput );

        lprtp->dbc  = dbc;
        lprtp->hpid = hpid;
        lprtp->htid = htid;
        lprtp->cb   = cbInput;

        _fmemcpy ( lprtp->rgbVar, lpInput, cbInput );

        xosd = DmTlFunc
            ( tlfRequest, hpid, FIELD_OFFSET( RTP, rgbVar ) + cbInput, (LPARAM) lprtp );

        MHFree ( lprtp );

    }

    if (xosd == xosdNone && cbOutput != 0) {
        _fmemcpy(lpOutput, abEMReplyBuf, cbOutput);
    }

    return xosd;

} /* DMSendRequestReply */



XOSD
DMSendDebugPacket (
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD cbInput,
    LPVOID lpInput
    )
/*++

Routine Description:


Arguments:

    dbc - Supplies command to send to EM

    hpid - Supplies process handle

    htid - Supplies thread handle

    cbInput - Supplies size of packet to send

    lpInput - Supplies packet to send

Return Value:

    xosdNone or xosd error value

--*/
{
    XOSD xosd = xosdNone;

    if ( cbInput == 0 ) {
        RTP rtp;

        rtp.dbc  = dbc;
        rtp.hpid = hpid;
        rtp.htid = htid;
        rtp.cb   = 0;
        xosd = DmTlFunc
            ( tlfDebugPacket, hpid, FIELD_OFFSET( RTP, rgbVar ), (LPARAM) &rtp );
    }
    else {
        LPRTP lprtp = MHAlloc ( FIELD_OFFSET( RTP, rgbVar ) + cbInput );

        lprtp->dbc  = dbc;
        lprtp->hpid = hpid;
        lprtp->htid = htid;
        lprtp->cb   = cbInput;

        _fmemcpy ( lprtp->rgbVar, lpInput, cbInput );

        xosd = DmTlFunc
            ( tlfDebugPacket, hpid, FIELD_OFFSET( RTP, rgbVar ) + cbInput, (LPARAM) lprtp );

        MHFree ( lprtp );

    }

    return xosd;

} /* DMSendRequestReply */


PVOID
DMCopyLargeReply(
    DWORD size
    )
/*++

Routine Description:

    This routine is called to get a reply packet whose size is not known
    before calling DMSendRequestReply.  It allocates a buffer and copies
    the reply packet into the buffer.

Arguments:

    size - Supplies the size in bytes of the packet to be copied

Return Value:

    Returns the copy of the reply packet.  If memory could not be allocated,
    returns NULL.

--*/
{
    PVOID pv = MHAlloc(size);
    if (pv) {
        memcpy(pv, abEMReplyBuf, size);
    }
    return pv;
} /* DMCopyLargeReply */


DWORD64
GetEndOfRange (
    HPRCX   hprc,
    HTHDX   hthd,
    DWORD64 Addr
    )
/*++

Routine Description:

    Given an address, gets the end of the range for that address.

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

    Addr    -   Supplies the address

Return Value:

    DWORD   -   End of range

--*/

{
    ADDR AddrPC;

    AddrFromHthdx(&AddrPC, hthd);
    SetAddrOff( &AddrPC, Addr );

    DMSendRequestReply(
        dbcLastAddr,
        hprc->hpid,
        hthd->htid,
        sizeof(ADDR),
        &AddrPC,
        sizeof(DWORD),
        &Addr
        );

    Addr =  (*(DWORD *)abEMReplyBuf);

    // NOTENOTE : jimsch --- Is this correct?
    return (DWORD) Addr;
}




DWORD
GetCanStep (
    HPID    hpid,
    HTID    htid,
    LPADDR  Addr,
    LPCANSTEP CanStep
    )
/*++

Routine Description:


Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

    Addr    -   Supplies Address

Return Value:

    CANSTEP_YES or CANSTEP_NO (or CANSTEP_THUNK?)

--*/

{
    return DMSendRequestReply(
        dbcCanStep,
        hpid,
        htid,
        sizeof(ADDR),
        Addr,
        sizeof(CANSTEP),
        CanStep
        );
}


BOOL
CheckBpt(
    HTHDX       hthd,
    PBREAKPOINT pbp
    )
{
    DEBUG_EVENT64 de;

    if (pbp->bpNotify == bpnsStop) {
        return TRUE;
    } else if (pbp->bpNotify == bpnsCheck) {
        de.dwDebugEventCode = CHECK_BREAKPOINT_DEBUG_EVENT;
        de.dwProcessId = hthd->hprc->pid;
        de.dwThreadId  = hthd->tid;
        de.u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_BREAKPOINT;

        NotifyEM(&de, hthd, 0, (UINT_PTR)pbp);

        return *(DWORD *)abEMReplyBuf;
    }
    return FALSE;
}


LPTSTR
MHStrdup(
    LPCTSTR s
    )
{
    int l = _tcslen(s);
    LPTSTR p = MHAlloc(l + sizeof(TCHAR));
    _tcscpy(p, s);
    return p;
}

/*** ISTHUNK
 *
 * PURPOSE:
 *      Determine if we are in a thunk
 *
 * INPUT:
 *      hthd            - Handle to thread
 *      uoffEIP         - address to check for a thunk
 *      lpf             - Type of thunk
 *      lpuoffThunkDest - Where the thunk is going to
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL
IsThunk (
    HTHDX       hthd,
    UOFFSET     uoffset,
    LPINT       lpfThunkType,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    BYTE        rgbBuffer[CB_THUNK_MAX];
    DWORD       dwLength = CB_THUNK_MAX;
    INT         ThunkType = THUNK_NONE;
    UOFFSET     ThunkDest = 0;
    DWORD       ThunkSize = 0;
    BOOL        Is = FALSE;

    //
    // Read until a read succeeds or there's no room left to read
    //
    if (DbgReadMemory ( hthd->hprc, uoffset, rgbBuffer, dwLength, &dwLength)) {

        //
        // System or ilink thunks
        //
        if (FIsDirectJump( rgbBuffer, dwLength, hthd, uoffset, &ThunkDest, &ThunkSize ) ||
            FIsIndirectJump( rgbBuffer, dwLength, hthd, uoffset, &ThunkDest, &ThunkSize )
        ) {
            Is = TRUE;
            if (IsInSystemDll(ThunkDest)) {
                ThunkType = THUNK_SYSTEM;
            } else {
                ThunkType = THUNK_USER;
            }
        } else {

            //
            // Note: it is possible that the offset passed in is NOT the PC
            // for the current thread.  Some of the thunk checks below
            // may require valid registers.  The following thunks require
            // valid registers to determine the destination address for a
            // thunk.  If uoffset is NOT the PC, then do not check for
            // these thunks.  These thunks are actually C++ thunks.
            //


            if ( PC(hthd) == uoffset &&
                ( FIsVCallThunk( rgbBuffer, dwLength, hthd, uoffset, &ThunkDest, &ThunkSize ) ||
                  FIsVTDispAdjustorThunk( rgbBuffer, dwLength, hthd, uoffset, &ThunkDest, &ThunkSize ) ||
                  FIsAdjustorThunk( rgbBuffer, dwLength, hthd, uoffset, &ThunkDest, &ThunkSize ) )
            ) {
                Is = TRUE;
                ThunkType = THUNK_USER;
            }
        }
    }

    if (lpuoffThunkDest) {
        *lpuoffThunkDest = ThunkDest;
    }
    if (lpdwThunkSize) {
        *lpdwThunkSize = ThunkSize;
    }
    if (lpfThunkType) {
        *lpfThunkType = ThunkType;
    }
    return Is;
}


#if 0
BOOL
IsPassingException(
    HPRCX   hprc
    )
/*++

Routine Description:

    TRUE if there is an exception and we are passing it on to the app.  This
    routine looks at the exception status of the thread we got the debug event
    on.  It doesn't make sense to look at any other thread.

Arguments:


Return Value:


--*/
{
    HTHDX   hthd = HTHDXFromPIDTID (hprc->pid, hprc->lastTidDebugEvent);
    return ((hthd->tstate & (ts_first | ts_second)) && !hthd->fExceptionHandled);
}
#endif


int
NumberOfThreadsInProcess(
    HPRCX hprc
    )
{
    int nthrds = 0;
    HTHDX hthd;
    for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
        nthrds++;
    }
    return nthrds;
}


void
SetHandledStateInStoppedThreads(
    HPRCX hprc,
    BOOL ContinueHandled
    )
/*++

Routine Description:



Arguments:


Return Value:


--*/
{
    HTHDX hthd;

    for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {

        if (hthd->tstate & ts_stopped) {
            //
            // don't just set this to ContinueHandled;
            // it may already be set.
            //
            if (ContinueHandled) {
                hthd->fExceptionHandled = TRUE;
            }
        }
    }
}

HTHDX
FindStoppedThread(
    HPRCX hprc
    )
{
    HTHDX hthd;
    for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
        if (hthd->tstate & ts_stopped) {
            return hthd;
        }
    }
    return NULL;
}
