/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    ntsdsup.c

Abstract:

    This file contains support for the ntsd expression evaluator

Author:

    Kent D. Forschmiedt (kentf) 18-Feb-97

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop


#include "ntsdsup.h"


extern CXF CxfIp;
extern LPSHF Lpshf;

ULONGLONG
GetRegFlagValue(
    int index
    )

/*++

Routine Description:

    Get flag value from index

Arguments:

    reg - Supplies flag index (from ntsdsup.h)

Return Value:

    flag contents

--*/

{
    ULONGLONG   val;
    RD          rd;
    FD          fd;
    XOSD        xosd;
    union {
        ULONGLONG   q;
        DWORD       d;
        WORD        w;
    } u;

    if (index & 0x80000000) {
        index &= 0x7fffffff;
        xosd = OSDGetFlagDesc(LppdCur->hpid, LptdCur->htid, index, &fd);
        xosd = OSDReadFlag(LppdCur->hpid, LptdCur->htid, fd.dwId, &u);
        val = u.d;
    } else {
        xosd = OSDGetRegDesc(LppdCur->hpid, LptdCur->htid, index, &rd);
        xosd = OSDReadRegister(LppdCur->hpid, LptdCur->htid, rd.dwId, &u);

        switch (rd.dwcbits) {
        case 16:
            val = u.w;
            break;
        case 32:
            val = u.d;
            break;
        case 64:
            val = u.q;
            break;
        default:
            Assert(0 && "Unsupported register size");
            val = 0;
            break;
        }
    }

    return val;
}

VOID
GetRegPCValue(
    PNTSDADDR ntsdaddr
    )

/*++

Routine Description:

    Get value of PC in an NTSDADDR

Arguments:

    addr - Returns PC value

Return Value:

    Current PC in flat ADDR format

--*/

{
    ADDR addr;
    OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrPC, &addr);
    SYFixupAddr(&addr);
    ADDRFLAT(ntsdaddr, GetAddrOff(addr));
}

int
GetRegString(
    PUCHAR regname,
    OPTIONAL RD * prd
    )

/*++

Routine Description:

    Get register index from register name.

Arguments:

    reg - Supplies register name

    prd - Returns register description structure OPTIONAL

Return Value:

    SUCCESS - register index
    FAILURE - (-1)

--*/

{
    ULONG   cRegs;
    int     i;
    RD      rd;
    FD      fd;

    if (NULL == LppdCur || NULL == LptdCur) {
        return -1;
    }

    OSDGetDebugMetric(LppdCur->hpid, LptdCur->htid, mtrcCRegs, &cRegs);
    for (i = 0; i < (int)cRegs; i++) {
        OSDGetRegDesc( LppdCur->hpid, LptdCur->htid, i, &rd);
        if (_stricmp( (PSTR) regname, rd.lszName) == 0) {
            if (prd) {
                *prd = rd;
            }
            return i;
        }
    }

    OSDGetDebugMetric(LppdCur->hpid, LptdCur->htid, mtrcCFlags, &cRegs);
    for (i = 0; i < (int)cRegs; i++) {
        OSDGetFlagDesc( LppdCur->hpid, LptdCur->htid, i, &fd);
        if (_stricmp( (PSTR) regname, fd.lszName) == 0) {
            if (prd) {
                ZeroMemory(prd, sizeof(RD));
            }
            return i | 0x80000000;
        }
    }

    return -1;
}

/**     fnCmp - name compare routine.
 *
 *      Compares the name described by the hInfo packet with a length
 *      prefixed name
 *
 *      fFlag = fnCmp (psearch_t pName, SYMPTR pSym, char *stName, int fCase);
 *
 *      Entry   pName = pointer to psearch_t packet describing name
 *              pSym = pointer to symbol structure (NULL if internal call)
 *              stName = pointer to a length preceeded name
 *                      can be NULL
 *              fCase = TRUE if case sensitive search
 *                      FALSE if case insensitive search
 *
 *      Exit    pName->lastsym = type of symbol checked
 *
 *      Returns 0 if names compared equal
 *              non-zero if names did not compare
 */


SHFLAG
fnCmp (
    HVOID hvSstr,
    PVOID notused,
    char *stName,
    SHFLAG fCase
    )
{
    int cmpflag;
    LPSSTR lpsstr = (LPSSTR) hvSstr;
    DWORD cstName;

    if (stName == NULL || stName[0] == 0) {
        return (1);
    }

    cstName = *((PUCHAR)stName);
    stName++;

    if (lpsstr->cb != cstName) {
        cmpflag = 1;
    } else if (fCase == TRUE) {
        cmpflag = _tcsncmp ( (PSTR) lpsstr->lpName, stName, lpsstr->cb);
    } else {
        cmpflag = _tcsnicmp ( (PSTR) lpsstr->lpName, stName, lpsstr->cb);
    }

    if (cmpflag) {
        //
        // make counted string into normal one
        //
        char * ppatAlloc = (PSTR) malloc(cstName + 1);
        Assert(ppatAlloc);
        char * ppat = ppatAlloc;
        char * ptemp;
        BOOL doit = FALSE;

        strncpy(ppat, stName, cstName);
        ppat[cstName] = 0;

        // strip 1 _
        if (*ppat == '_') {
            ppat++;
            doit = TRUE;
        }
        // or two ..
        else if (ppat[0] == '.' && ppat[1] == '.') {
            ppat +=2;
            doit = TRUE;
        }

        //
        // search for @
        //
        ptemp = _tcschr(ppat, '@');
        if (ptemp) {
            *ptemp = '\0';
            doit = TRUE;
        }

        if (doit && (_tcslen(ppat) == lpsstr->cb)) {
            if (fCase == TRUE) {
                cmpflag = _tcsncmp ( (PSTR) lpsstr->lpName, ppat, lpsstr->cb);
            } else {
                cmpflag = _tcsnicmp ( (PSTR) lpsstr->lpName, ppat, lpsstr->cb);
            }
        }

        free(ppatAlloc);
    }
    return cmpflag;

}


BOOL
GetOffsetFromSym(
    PTCHAR  pString,
    PADDR   paddr
    )
{
    HEXE    hexe;
    HEXE    hexeFirst;
    HSYM    hsym = NULL;
    ADDR    xaddr;
    TCHAR   modname[MAX_PATH];
    PTCHAR  p;
    PTCHAR  pName;
    SSTR    sstr;

    if (p = _tcschr(pString, '!')) {

        memcpy(modname, pString, (size_t) (p-pString));
        modname[p-pString] = '\0';
        pName = p+1;

        hexe = SHGethExeFromModuleName(modname);
        //
        // this will make it stop after the first try:
        //
        hexeFirst = SHGetNextExe(hexe);
        if (hexeFirst == NULL) {
            hexeFirst = SHGetNextExe(hexeFirst);
        }

    } else {

        if (ADDR_IS_LI(*SHpADDRFrompCXT(SHpCXTFrompCXF(&CxfIp)))) {
            hexe = SHpADDRFrompCXT(SHpCXTFrompCXF(&CxfIp))->emi;
        } else {
            hexe = SHGetNextExe(NULL);
        }
        hexeFirst = hexe;
        pName = pString;

    }

    while (hexe) {

        emiAddr(xaddr) = (HEMI) hexe;

        ZeroMemory(&sstr, sizeof(SSTR));

        sstr.lpName = (PUCHAR) pName;
        sstr.cb  = (BYTE) _tcslen (pName);
        sstr.searchmask = SSTR_FuzzyPublic;

        // Look for the name in the public symbols of that .EXE
        hsym = PHFindNameInPublics(NULL,
                                   hexe,
                                   &sstr,
                                   0,
                                   fnCmp
                                  );
        if (!hsym) {
            sstr.searchmask |= SSTR_NoHash;
            hsym = PHFindNameInPublics(NULL,
                                       hexe,
                                       &sstr,
                                       0,
                                       fnCmp
                                      );
        }

        if (hsym) {
            break;
        }

        hexe = SHGetNextExe(hexe);
        if (hexe == NULL) {
            hexe = SHGetNextExe(hexe);
        }
        if (hexe == hexeFirst) {
            hexe = NULL;
        }
    }

    if (!hsym) {
        return FALSE;
    } else {
        SHAddrFromHsym(paddr, hsym);
        emiAddr (*paddr) = (HEMI) hexe;
        SYFixupAddr(paddr);
        return TRUE;
    }
}

PNTSDADDR
AddrAdd(
    PNTSDADDR paddr,
    ULONG64 scalar
    )
{
//  assert(fFlat(paddr));
    if (fnotFlat(*paddr)) {
        ComputeFlatAddress(paddr, NULL);
    }

    Flat(*paddr) += scalar;
    paddr->off  += scalar;

    return paddr;
}

PNTSDADDR
AddrSub(
    PNTSDADDR paddr,
    ULONG64 scalar
    )
{
//  assert(fFlat(paddr));
    if (fnotFlat(*paddr)) {
        ComputeFlatAddress(paddr, NULL);
    }

    Flat(*paddr) -= scalar;
    paddr->off  -= scalar;

    return paddr;
}


BOOL
LookupSelector(
    LPTD lptd,
    PXDESCRIPTOR_TABLE_ENTRY pdesc
    )
{
    return FALSE;
}



UINT
DHGetDebuggeeBytes(
    ADDR addr,
    UINT cb,
    void * lpb
    );

BOOL
GetMemByte(
    PNTSDADDR Address,
    PBYTE buf
    )
{
    ADDR addr;
    NtsdAddrToAddr(Address, &addr);
    return DHGetDebuggeeBytes(addr, 1, buf) != 0;
}

BOOL
GetMemWord(
    PNTSDADDR Address,
    PWORD buf
    )
{
    ADDR addr;
    NtsdAddrToAddr(Address, &addr);
    return DHGetDebuggeeBytes(addr, 2, buf) != 0;
}

BOOL
GetMemDword(
    PNTSDADDR Address,
    PULONG buf
    )
{
    ADDR addr;
    NtsdAddrToAddr(Address, &addr);
    return DHGetDebuggeeBytes(addr, 4, buf) != 0;
}

void
NtsdAddrToAddr(
    PNTSDADDR NtsdAddr,
    PADDR Addr
    )
{
    NTSDADDR a = *NtsdAddr;
    ComputeFlatAddress(&a, NULL);
    AddrInit(Addr,
             0,
             NtsdAddr->seg,
             NtsdAddr->off,
             1,
             1,
             0,
             0
             );
    SYFixupAddr(Addr);
}

void
AddrToNtsdAddr(
    PADDR Addr,
    PNTSDADDR NtsdAddr
    )
{
    if (ADDR_IS_LI(*Addr)) {
        SYFixupAddr(Addr);
    }
    ADDRFLAT(NtsdAddr, GetAddrOff(*Addr));
}

void
ComputeFlatAddress (
    PNTSDADDR paddr,
    PVOID pvArg
    )
{
    PXDESCRIPTOR_TABLE_ENTRY pdesc = (PXDESCRIPTOR_TABLE_ENTRY) pvArg;
    XDESCRIPTOR_TABLE_ENTRY desc;

    static LONG lastSelector = -1;
    static ULONG lastBaseOffset;

    if (paddr->type & FLAT_COMPUTED) {
        return;
    }

    switch (paddr->type & (~INSTR_POINTER)) {

        case ADDR_V86:
            paddr->off &= 0xffff;
            Flat(*paddr) = ((ULONG)paddr->seg << 4) + paddr->off;
            break;

        case ADDR_16:
            paddr->off &= 0xffff;

        case ADDR_1632: {

                if (paddr->seg!=(USHORT)lastSelector) {
                    lastSelector = paddr->seg;
                    desc.Selector = (ULONG)paddr->seg;
                    if (!pdesc) {
                        LookupSelector(LptdCur, pdesc = &desc);
                    }
                    lastBaseOffset =
                        ((ULONG)pdesc->Descriptor.HighWord.Bytes.BaseHi << 24) |
                        ((ULONG)pdesc->Descriptor.HighWord.Bytes.BaseMid << 16) |
                        (ULONG)pdesc->Descriptor.BaseLow;
                }
                Flat(*paddr) = paddr->off + lastBaseOffset;
            }
            break;

        case ADDR_FLAT:
            Flat(*paddr) = paddr->off;
            break;

        default:
            return;
    }

    paddr->type |= FLAT_COMPUTED;
}

