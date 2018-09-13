/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    cv.c

Abstract:

    This module handles the conversion activities requires for converting
    COFF debug data to CODEVIEW debug data.

Author:

    Wesley A. Witt (wesw) 19-April-1993

Environment:

    Win32, User Mode

--*/

#include <windows.h>
#include <dbghelp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cv.h"
#include "symcvt.h"
#include "cvcommon.h"

typedef struct tagOFFSETSORT {
    DWORD       dwOffset;          // offset for the symbol
    DWORD       dwSection;         // section number of the symbol
    DATASYM32   *dataSym;          // pointer to the symbol info
} OFFSETSORT;


#define n_name          N.ShortName
#define n_zeroes        N.Name.Short
#define n_nptr          N.LongName[1]
#define n_offset        N.Name.Long

static LPSTR GetSymName( PIMAGE_SYMBOL Symbol, PUCHAR StringTable, char *s );
DWORD  CreateModulesFromCoff( PPOINTERS p );
DWORD  CreatePublicsFromCoff( PPOINTERS p );
DWORD  CreateSegMapFromCoff( PPOINTERS p );
DWORD  CreateSrcLinenumbers( PPOINTERS p );



LONG
GuardPageFilterFunction(
    DWORD                ec,
    LPEXCEPTION_POINTERS lpep
    )

/*++

Routine Description:

    This function catches all exceptions from the convertcofftocv function
    and all that it calls.  The purpose of this function is allocate memory
    when it is necessary.  This happens because the cofftocv conversion cannot
    estimate the memory requirements before the conversion takes place.  To
    handle this properly space in the virtual address space is reserved, the
    reservation amount is 10 times the image size.  The first page is commited
    and then the conversion is started.  When an access violation occurs and the
    page that is trying to be access has a protection of noaccess then the
    page is committed.  Any other exception is not handled.

Arguments:

    ec      - the ecxeption code (should be EXCEPTION_ACCESS_VIOLATION)
    lpep    - pointer to the exception record and context record


Return Value:

    EXCEPTION_CONTINUE_EXECUTION    - access violation handled
    EXCEPTION_EXECUTE_HANDLER       - unknown exception and is not handled

--*/

{
    LPVOID                      vaddr;
    SYSTEM_INFO                 si;
    MEMORY_BASIC_INFORMATION    mbi;


    if (ec == EXCEPTION_ACCESS_VIOLATION) {
        vaddr = (LPVOID)lpep->ExceptionRecord->ExceptionInformation[1];
        VirtualQuery( vaddr, &mbi, sizeof(mbi) );
        if (mbi.AllocationProtect == PAGE_NOACCESS) {
            GetSystemInfo( &si );
            VirtualAlloc( vaddr, si.dwPageSize, MEM_COMMIT, PAGE_READWRITE );
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

//  return EXCEPTION_CONTINUE_SEARCH;
    return EXCEPTION_EXECUTE_HANDLER;
}


BOOL
ConvertCoffToCv( PPOINTERS p )

/*++

Routine Description:

    This is the control function for the conversion of COFF to CODEVIEW
    debug data.  It calls individual functions for the conversion of
    specific types of debug data.


Arguments:

    p        - pointer to a POINTERS structure (see cofftocv.h)


Return Value:

    TRUE     - conversion succeded
    FALSE    - conversion failed

--*/

{
    SYSTEM_INFO                 si;
    DWORD                       cbsize;
    BOOL                        rval = TRUE;


    ValidateHeap();

    GetSystemInfo( &si );
    cbsize = max( p->iptrs.fsize * 10, si.dwPageSize * 10 );

    //
    // reserve all necessary pages
    //
    p->pCvCurr = p->pCvStart.ptr = VirtualAlloc( NULL, cbsize, MEM_RESERVE, PAGE_NOACCESS );

    //
    // commit the first pages
    //
    VirtualAlloc( p->pCvCurr, min( cbsize, 5 * si.dwPageSize), MEM_COMMIT, PAGE_READWRITE );


    __try {

        ValidateHeap();
        CreateSignature( p );
        ValidateHeap();
        CreateModulesFromCoff( p );
        ValidateHeap();
        CreatePublicsFromCoff( p );
        ValidateHeap();
        CreateSymbolHashTable( p );
        ValidateHeap();
        CreateAddressSortTable( p );
        ValidateHeap();
        CreateSegMapFromCoff( p );
        ValidateHeap();
//      CreateSrcLinenumbers( p );
        ValidateHeap();
        CreateDirectories( p );
        ValidateHeap();

    } __except ( GuardPageFilterFunction( GetExceptionCode(), GetExceptionInformation() )) {

        VirtualFree( p->pCvStart.ptr, cbsize, MEM_DECOMMIT );
        p->pCvStart.ptr = NULL;
        rval = FALSE;

    }

    ValidateHeap();
    if (rval) {
        p->pCvCurr = LocalAlloc( NONZEROLPTR, p->pCvStart.size );
        CopyMemory( p->pCvCurr, p->pCvStart.ptr, p->pCvStart.size );
        VirtualFree( p->pCvStart.ptr, cbsize, MEM_DECOMMIT );
        p->pCvStart.ptr = p->pCvCurr;
    }
    ValidateHeap();

    return rval;
}


DWORD
CreateModulesFromCoff( PPOINTERS p )

/*++

Routine Description:

    Creates the individual CV module records.  There is one CV module
    record for each .FILE record in the COFF debug data.  This is true
    even if the COFF size is zero.


Arguments:

    p        - pointer to a POINTERS structure (see cofftocv.h)


Return Value:

    The number of modules that were created.

--*/

{
    unsigned short              i;
    OMFModule                   *m = NULL;
    char *                      pb;
    PIMAGE_SECTION_HEADER       sh;

    p->pMi = (LPMODULEINFO) LocalAlloc( LPTR, sizeof(MODULEINFO) );

    m = (OMFModule *) p->pCvCurr;
    m->ovlNumber = 0;
    m->iLib = 0;
    m->Style[0] = 'C';
    m->Style[1] = 'V';

    sh = p->iptrs.sectionHdrs;

    for (i=0; i < p->iptrs.numberOfSections; i++) {
        m->SegInfo[i].Seg = i + 1;
        m->SegInfo[i].cbSeg = sh->Misc.VirtualSize;
        // Note: Off isn't the va where this section starts,
        //  it's where the symbolic in this section starts.
        m->SegInfo[i].Off = 0;
        sh++;
    }

    m->cSeg = i;

    pb = (char *) &m->SegInfo[i];
    *pb = strlen("foo.c");
    memcpy(pb+1, "foo.c", *pb);

    p->pMi[0].name = "foo.c";
    p->pMi[0].iMod = 1;
    p->pMi[0].cb = 0;
    p->pMi[0].SrcModule = 0;

    m = NextMod(m);
    p->modcnt = 1;
    UpdatePtrs( p, &p->pCvModules, (LPVOID)m, 1 );

    return 1;
}


DWORD
CreatePublicsFromCoff( PPOINTERS p )

/*++

Routine Description:

    Creates the individual CV public symbol records.  There is one CV
    public record created for each COFF symbol that is marked as EXTERNAL
    and has a section number greater than zero.  The resulting CV publics
    are sorted by section and offset.


Arguments:

    p        - pointer to a POINTERS structure (see cofftocv.h)


Return Value:

    The number of publics created.

--*/

{
    int                 i;
    DWORD               numaux;
    DWORD               numsyms = 0;
    char                szSymName[256];
    PIMAGE_SYMBOL       Symbol;
    OMFSymHash          *omfSymHash;
    DATASYM32           *dataSym;
    DATASYM32           *dataSym2;

    omfSymHash = (OMFSymHash *) p->pCvCurr;
    dataSym = (DATASYM32 *) (PUCHAR)((DWORD)omfSymHash + sizeof(OMFSymHash));

    for (i= 0, Symbol = p->iptrs.AllSymbols;
         i < p->iptrs.numberOfSymbols;
         i += numaux + 1, Symbol += numaux + 1) {

        if ((Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL) &&
            (Symbol->SectionNumber > 0)) {

            if (GetSymName( Symbol, p->iptrs.stringTable, szSymName )) {
                dataSym->rectyp = S_PUB32;
                dataSym->seg = Symbol->SectionNumber;
                dataSym->off = Symbol->Value -
                     p->iptrs.sectionHdrs[Symbol->SectionNumber-1].VirtualAddress;
                dataSym->typind = 0;
                dataSym->name[0] = strlen( szSymName );
                strcpy( &dataSym->name[1], szSymName );
                dataSym2 = NextSym32( dataSym );
                dataSym->reclen = (USHORT) ((DWORD)dataSym2 - (DWORD)dataSym) - 2;
                dataSym = dataSym2;
                numsyms += 1;
            }
        }
        numaux = Symbol->NumberOfAuxSymbols;
    }

    UpdatePtrs( p, &p->pCvPublics, (LPVOID)dataSym, numsyms );

    omfSymHash->cbSymbol = p->pCvPublics.size - sizeof(OMFSymHash);
    omfSymHash->symhash  = 0;
    omfSymHash->addrhash = 0;
    omfSymHash->cbHSym   = 0;
    omfSymHash->cbHAddr  = 0;

    return numsyms;
}                               /* CreatePublisFromCoff() */


DWORD
CreateSrcLinenumbers(
    PPOINTERS p
    )

/*++

Routine Description:

    Creates the individual CV soure line number records.


Arguments:

    p        - pointer to a POINTERS structure (see cofftocv.h)


Return Value:

    The number of publics created.

--*/

{
    typedef struct _SEGINFO {
        DWORD   start;
        DWORD   end;
        DWORD   cbLines;
        DWORD   ptrLines;
        DWORD   va;
        DWORD   num;
        BOOL    used;
    } SEGINFO, *LPSEGINFO;

    typedef struct _SRCINFO {
        LPSEGINFO   seg;
        DWORD       numSeg;
        DWORD       cbSeg;
        CHAR        name[MAX_PATH+1];
    } SRCINFO, *LPSRCINFO;

    typedef struct _SECTINFO {
        DWORD       va;
        DWORD       size;
        DWORD       ptrLines;
        DWORD       numLines;
    } SECTINFO, *LPSECTINFO;


    DWORD               i;
    DWORD               j;
    DWORD               k;
    DWORD               l;
    DWORD               actual;
    DWORD               sidx;
    DWORD               NumSrcFiles;
    DWORD               SrcFileCnt;
    DWORD               numaux;
    PIMAGE_SYMBOL       Symbol;
    PIMAGE_AUX_SYMBOL   AuxSymbol;
    BOOL                first = TRUE;
    OMFSourceModule     *SrcModule;
    OMFSourceFile       *SrcFile;
    OMFSourceLine       *SrcLine;
    LPBYTE              lpb;
    LPDWORD             lpdw;
    PUSHORT             lps;
    PUCHAR              lpc;
    PIMAGE_LINENUMBER   pil, pilSave;
    LPSRCINFO           si;
    LPSECTINFO          sections;


    //
    // setup the section info structure
    //
    sections = (LPSECTINFO) LocalAlloc( NONZEROLPTR, sizeof(SECTINFO) * p->iptrs.numberOfSections );
    for (i=0; i<(DWORD)p->iptrs.numberOfSections; i++) {
        sections[i].va        = p->iptrs.sectionHdrs[i].VirtualAddress;
        sections[i].size      = p->iptrs.sectionHdrs[i].SizeOfRawData;
        sections[i].ptrLines  = p->iptrs.sectionHdrs[i].PointerToLinenumbers;
        sections[i].numLines  = p->iptrs.sectionHdrs[i].NumberOfLinenumbers;
    }

    //
    // count the number of source files that contibute linenumbers
    //
    SrcFileCnt = 100;
    si = (LPSRCINFO) LocalAlloc( LPTR, sizeof(SRCINFO) * SrcFileCnt );
    for (i=0, j=0, Symbol=p->iptrs.AllSymbols, NumSrcFiles=0;
         i<(DWORD)p->iptrs.numberOfSymbols;
         i+=(numaux+1), Symbol+=(numaux + 1)) {

        numaux = Symbol->NumberOfAuxSymbols;
        AuxSymbol = (PIMAGE_AUX_SYMBOL) (Symbol+1);

        if (Symbol->StorageClass == IMAGE_SYM_CLASS_FILE) {

            if (!first) {
                si[NumSrcFiles].cbSeg = j;
                NumSrcFiles++;
                if (NumSrcFiles == SrcFileCnt) {
                    SrcFileCnt += 100;
                    si = (LPSRCINFO) LocalReAlloc( si, sizeof(SRCINFO) * SrcFileCnt, LMEM_ZEROINIT );
                }
            }

            memcpy(si[NumSrcFiles].name, (char *)AuxSymbol, numaux*sizeof(IMAGE_AUX_SYMBOL));
            si[NumSrcFiles].name[numaux*sizeof(IMAGE_AUX_SYMBOL)] = 0;
            si[NumSrcFiles].numSeg = 100;
            si[NumSrcFiles].seg = (LPSEGINFO) LocalAlloc( LPTR, sizeof(SEGINFO) * si[NumSrcFiles].numSeg );
            first = FALSE;
            j = 0;

        }

        //
        // we do not want to look for segment information until we
        // have found a valid source file
        //
        if (first) {
            continue;
        }

        //
        // check the symbol to see if it is a segment record
        //
        if (numaux && Symbol->StorageClass == IMAGE_SYM_CLASS_STATIC &&
            (*Symbol->n_name == '.' ||
             ((Symbol->Type & 0xf) == IMAGE_SYM_TYPE_NULL && AuxSymbol->Section.Length)) &&
            AuxSymbol->Section.NumberOfLinenumbers > 0) {

            //
            // find the section that this symbol belongs to
            //
            for (k=0; k<(DWORD)p->iptrs.numberOfSections; k++) {
                if (Symbol->Value >= sections[k].va &&
                    Symbol->Value < sections[k].va + sections[k].size) {

                    sidx = k;
                    break;

                }
            }

            if (k != (DWORD)p->iptrs.numberOfSections &&
                p->iptrs.sectionHdrs[k].NumberOfLinenumbers) {

                pil = (PIMAGE_LINENUMBER) (p->iptrs.fptr + sections[sidx].ptrLines);
                k = 0;

                while( k < AuxSymbol->Section.NumberOfLinenumbers ) {

                    //
                    // count the linenumbers in this section or sub-section
                    //
                    for ( pilSave=pil,l=0;
                          k<AuxSymbol->Section.NumberOfLinenumbers;
                          k++,pilSave++,l++ ) {

                        if ((k != (DWORD)AuxSymbol->Section.NumberOfLinenumbers-1) &&
                            (pilSave->Linenumber > (pilSave+1)->Linenumber)) {
                            pilSave++;
                            l++;
                            break;
                        }

                    }

                    //
                    // pil     == beginning of the range
                    // pilSave == end of the range
                    //

                    si[NumSrcFiles].seg[j].start =
                                     (pil->Type.VirtualAddress - sections[sidx].va);

                    if (sections[sidx].numLines == l) {
                        pilSave--;
                        si[NumSrcFiles].seg[j].end =
                                     (pilSave->Type.VirtualAddress - sections[sidx].va) + 1;
//                                   (Symbol->Value - sections[sidx].va) + 1;
                    } else {
                        si[NumSrcFiles].seg[j].end =
                                     (pilSave->Type.VirtualAddress - sections[sidx].va) - 1;
//                                   (Symbol->Value - sections[sidx].va) - 1;
                    }

                    si[NumSrcFiles].seg[j].ptrLines = sections[sidx].ptrLines;
                    si[NumSrcFiles].seg[j].cbLines = l;
                    si[NumSrcFiles].seg[j].va = sections[sidx].va;
                    si[NumSrcFiles].seg[j].num = sidx + 1;
                    si[NumSrcFiles].seg[j].used = FALSE;

                    sections[sidx].ptrLines += (l * sizeof(IMAGE_LINENUMBER));
                    sections[sidx].numLines -= l;

                    j++;
                    if (j == si[NumSrcFiles].numSeg) {
                        si[NumSrcFiles].numSeg += 100;
                        si[NumSrcFiles].seg = (LPSEGINFO) LocalReAlloc( si[NumSrcFiles].seg,
                                                                        sizeof(SEGINFO) * si[NumSrcFiles].numSeg,
                                                                        LMEM_ZEROINIT );
                    }
                    k++;
                    pil = pilSave;
                }

            }

        }

    }

    lpb = (LPBYTE) p->pCvCurr;

    //
    // if there is nothing to do then bail out
    //
    if (!NumSrcFiles) {
        UpdatePtrs( p, &p->pCvSrcModules, (LPVOID)lpb, 0 );
        return 0;
    }

    for (i=0,actual=0,l=0; i<NumSrcFiles; i++) {

        if (si[i].cbSeg == 0) {
            continue;
        }

        //
        // create the source module header
        //
        SrcModule = (OMFSourceModule*) lpb;
        SrcModule->cFile = 1;
        SrcModule->cSeg = (USHORT)si[i].cbSeg;
        SrcModule->baseSrcFile[0] = 0;

        //
        // write the start/end pairs
        //
        lpdw = (LPDWORD) ((LPBYTE)SrcModule + sizeof(OMFSourceModule));
        for (k=0; k<si[i].cbSeg; k++) {
            *lpdw++ = si[i].seg[k].start;
            *lpdw++ = si[i].seg[k].end;
        }

        //
        // write the segment numbers
        //
        lps = (PUSHORT) lpdw;
        for (k=0; k<si[i].cbSeg; k++) {
            *lps++ = (USHORT)si[i].seg[k].num;
        }

        //
        // align to a dword boundry
        //
        lps = (PUSHORT) ((LPBYTE)lps + align(lps));

        //
        // update the base pointer
        //
        SrcModule->baseSrcFile[0] = (DWORD) ((LPBYTE)lps - (LPBYTE)SrcModule);

        //
        // write the source file record
        //
        SrcFile = (OMFSourceFile*) lps;
        SrcFile->cSeg = (USHORT)si[i].cbSeg;
        SrcFile->reserved = 0;

        for (k=0; k<si[i].cbSeg; k++) {
            SrcFile->baseSrcLn[k] = 0;
        }

        //
        // write the start/end pairs
        //
        lpdw = (LPDWORD) ((LPBYTE)SrcFile + 4 + (4 * si[i].cbSeg));
        for (k=0; k<si[i].cbSeg; k++) {
            *lpdw++ = si[i].seg[k].start;
            *lpdw++ = si[i].seg[k].end;
        }

        //
        // write the source file name
        //
        lpc = (PUCHAR) lpdw;
        k = strlen(si[i].name);
        *lpc++ = (UCHAR) k;
        strcpy( lpc, si[i].name );
        lpb = lpc + k;

        //
        // find the module info struct
        //
        for (; l<p->modcnt; l++) {
            if (_stricmp(p->pMi[l].name,si[i].name)==0) {
                break;
            }
        }

        p->pMi[l].SrcModule = (DWORD) SrcModule;

        //
        // align to a dword boundry
        //
        lpb = (LPBYTE) (lpb + align(lpb));

        //
        // create the line number pairs
        //
        for (k=0; k<si[i].cbSeg; k++) {

            //
            // find the first line number that applies to this segment
            //
            pil = (PIMAGE_LINENUMBER) (p->iptrs.fptr + si[i].seg[k].ptrLines);

            //
            // update the base pointer
            //
            SrcFile->baseSrcLn[k] = (DWORD) (lpb - (LPBYTE)SrcModule);

            //
            // write the line numbers
            //
            SrcLine = (OMFSourceLine*) lpb;
            SrcLine->Seg = (USHORT)si[i].seg[k].num;
            SrcLine->cLnOff = (USHORT) si[i].seg[k].cbLines;
            pilSave = pil;
            lpdw = (LPDWORD) (lpb + 4);
            for (j=0; j<SrcLine->cLnOff; j++) {
                *lpdw++ = pil->Type.VirtualAddress - si[i].seg[k].va;
                pil++;
            }
            lps = (PUSHORT) lpdw;
            pil = pilSave;
            for (j=0; j<SrcLine->cLnOff; j++) {
                *lps++ = pil->Linenumber;
                pil++;
            }

            //
            // align to a dword boundry
            //
            lps = (PUSHORT) ((LPBYTE)lps + align(lps));

            lpb = (LPBYTE) lps;
        }

        p->pMi[l].cb = (DWORD)lpb - (DWORD)SrcModule;
        actual++;

    }

    UpdatePtrs( p, &p->pCvSrcModules, (LPVOID)lpb, actual );

    //
    // cleanup all allocated memory
    //

    LocalFree( sections );

    for (i=0; i<SrcFileCnt; i++) {
        if (si[i].seg) {
            LocalFree( si[i].seg );
        }
    }

    LocalFree( si );

    return NumSrcFiles;
}                               /* CreateSrcLinenumbers() */


DWORD
CreateSegMapFromCoff( PPOINTERS p )

/*++

Routine Description:

    Creates the CV segment map.  The segment map is used by debuggers
    to aid in address lookups.  One segment is created for each COFF
    section in the image.

Arguments:

    p        - pointer to a POINTERS structure (see cofftocv.h)


Return Value:

    The number of segments in the map.

--*/

{
    int                         i;
    SGM                         *sgm;
    SGI                         *sgi;
    PIMAGE_SECTION_HEADER       sh;
    ULONG                       align;


    sgm = (SGM *) p->pCvCurr;
    sgi = (SGI *) ((DWORD)p->pCvCurr + sizeof(SGM));

    sgm->cSeg = (unsigned short) p->iptrs.numberOfSections;
    sgm->cSegLog = (unsigned short) p->iptrs.numberOfSections;

    sh = p->iptrs.sectionHdrs;

    if (p->iptrs.optHdr) {
        align = p->iptrs.optHdr->FileAlignment;
    } else {
        align = p->iptrs.sepHdr->SectionAlignment;
    }

    for (i=0; i<p->iptrs.numberOfSections; i++, sh++) {
        sgi->sgf.fRead        = (USHORT) ((sh->Characteristics & IMAGE_SCN_MEM_READ)    == IMAGE_SCN_MEM_READ);
        sgi->sgf.fWrite       = (USHORT) ((sh->Characteristics & IMAGE_SCN_MEM_WRITE)   == IMAGE_SCN_MEM_WRITE);
        sgi->sgf.fExecute     = (USHORT) ((sh->Characteristics & IMAGE_SCN_MEM_EXECUTE) == IMAGE_SCN_MEM_EXECUTE);
        sgi->sgf.f32Bit       = 1;
        sgi->sgf.fSel         = 0;
        sgi->sgf.fAbs         = 0;
        sgi->sgf.fGroup       = 1;
        sgi->iovl             = 0;
        sgi->igr              = 0;
        sgi->isgPhy           = (USHORT) i + 1;
        sgi->isegName         = 0;
        sgi->iclassName       = 0;
        sgi->doffseg          = 0;
        sgi->cbSeg            = ((sh->Misc.VirtualSize + (align-1)) & ~(align-1));
        sgi++;
    }

    UpdatePtrs( p, &p->pCvSegMap, (LPVOID)sgi, i );

    return i;
}


LPSTR
GetSymName( PIMAGE_SYMBOL Symbol, PUCHAR StringTable, char *s )

/*++

Routine Description:

    Extracts the COFF symbol from the image symbol pointer and puts
    the ascii text in the character pointer passed in.


Arguments:

    Symbol        - COFF Symbol Record
    StringTable   - COFF string table
    s             - buffer for the symbol string


Return Value:

    void

--*/

{
    DWORD i;

    if (Symbol->n_zeroes) {
        for (i=0; i<8; i++) {
            if ((Symbol->n_name[i]>0x1f) && (Symbol->n_name[i]<0x7f)) {
                *s++ = Symbol->n_name[i];
            }
        }
        *s = 0;
    }
    else {
        if (StringTable[Symbol->n_offset] == '?') {
            i = UnDecorateSymbolName( &StringTable[Symbol->n_offset],
                                  s,
                                  255,
                                  UNDNAME_COMPLETE                |
                                  UNDNAME_NO_LEADING_UNDERSCORES  |
                                  UNDNAME_NO_MS_KEYWORDS          |
                                  UNDNAME_NO_FUNCTION_RETURNS     |
                                  UNDNAME_NO_ALLOCATION_MODEL     |
                                  UNDNAME_NO_ALLOCATION_LANGUAGE  |
                                  UNDNAME_NO_MS_THISTYPE          |
                                  UNDNAME_NO_CV_THISTYPE          |
                                  UNDNAME_NO_THISTYPE             |
                                  UNDNAME_NO_ACCESS_SPECIFIERS    |
                                  UNDNAME_NO_THROW_SIGNATURES     |
                                  UNDNAME_NO_MEMBER_TYPE          |
                                  UNDNAME_NO_RETURN_UDT_MODEL     |
                                  UNDNAME_NO_ARGUMENTS            |
                                  UNDNAME_NO_SPECIAL_SYMS         |
                                  UNDNAME_NAME_ONLY
                                );
            if (!i) {
                return NULL;
            }
        } else {
            strcpy( s, &StringTable[Symbol->n_offset] );
        }
    }

    return s;
}
