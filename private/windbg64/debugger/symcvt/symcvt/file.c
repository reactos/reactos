/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    file.c

Abstract:

    This module handles all file i/o for SYMCVT.  This includes the
    mapping of all files and establishing all file pointers for the
    mapped file(s).

Author:

    Wesley A. Witt (wesw) 19-April-1993

Environment:

    Win32, User Mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "symcvt.h"


static BOOL CalculateOutputFilePointers( PIMAGEPOINTERS pi, PIMAGEPOINTERS po );
static BOOL CalculateInputFilePointers( PIMAGEPOINTERS p );



BOOL
MapInputFile (
              PPOINTERS   p,
              HANDLE      hFile,
              char *      fname
              )
/*++

Routine Description:

    Maps the input file specified by the fname argument and saves the
    file handle & file pointer in the POINTERS structure.


Arguments:

    p        - Supplies pointer to a POINTERS structure (see cofftocv.h)
    hFile    - OPTIONAL Supplies handle for file if already open
    fname    - Supplies ascii string for the file name

Return Value:

    TRUE     - file mapped ok
    FALSE    - file could not be mapped

--*/

{
    BOOL        rVal = TRUE;

    memset( p, 0, sizeof(POINTERS) );

    strcpy( p->iptrs.szName, fname );

    if (hFile != NULL) {

        p->iptrs.hFile = hFile;
        p->iptrs.CloseFile = FALSE;

    } else {

        p->iptrs.hFile = CreateFile( p->iptrs.szName,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL );
        p->iptrs.CloseFile = TRUE;
    }

    if (p->iptrs.hFile == INVALID_HANDLE_VALUE) {

        rVal = FALSE;

    } else {

        p->iptrs.fsize = GetFileSize( p->iptrs.hFile, NULL );
        p->iptrs.hMap = CreateFileMapping( p->iptrs.hFile,
                                           NULL,
                                           PAGE_READONLY,
                                           0,
                                           0,
                                           NULL
                                         );

        if (p->iptrs.hMap == NULL) {

            p->iptrs.hMap = NULL;
            rVal = FALSE;

        } else {

            p->iptrs.fptr = MapViewOfFile( p->iptrs.hMap,
                                           FILE_MAP_READ,
                                           0, 0, 0 );
            if (p->iptrs.fptr == NULL) {
                CloseHandle( p->iptrs.hMap );
                p->iptrs.hMap = NULL;
                rVal = FALSE;
            }
        }
    }

    if (!hFile && p->iptrs.hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(p->iptrs.hFile);
        p->iptrs.hFile = NULL;
    }

    return rVal;
}                               /* MapInputFile() */



BOOL
UnMapInputFile (
    PPOINTERS p
    )
/*++

Routine Description:

    Unmaps the input file specified by the fname argument and then
    closes the file.


Arguments:

    p        - pointer to a POINTERS structure (see cofftocv.h)

Return Value:

    TRUE     - file mapped ok
    FALSE    - file could not be mapped

--*/

{
    if ( p->iptrs.fptr ) {
        UnmapViewOfFile( p->iptrs.fptr );
        p->iptrs.fptr = NULL;
    }
    if ( p->iptrs.hMap ) {
        CloseHandle( p->iptrs.hMap );
        p->iptrs.hMap = NULL;
    }
    if (p->iptrs.hFile != NULL) {
        if (p->iptrs.CloseFile) {
            CloseHandle( p->iptrs.hFile );
        }
        p->iptrs.hFile = NULL;
    }
    return TRUE;
}                               /* UnMapInputFile() */


BOOL
FillInSeparateImagePointers(
                            PIMAGEPOINTERS      p
                            )
/*++

Routine Description:

    This routine will go through the exe file and fill in the
    pointers needed relative to the separate debug information files

Arguments:

    p  - Supplies the structure to fill in

Return Value:

    TRUE if successful and FALSE otherwise.

--*/

{
    int                         li;
    int                         numDebugDirs;
    PIMAGE_DEBUG_DIRECTORY      pDebugDir;
    PIMAGE_COFF_SYMBOLS_HEADER  pCoffHdr;

    p->sectionHdrs = (PIMAGE_SECTION_HEADER)
      (p->fptr + sizeof(IMAGE_SEPARATE_DEBUG_HEADER));

    numDebugDirs = p->sepHdr->DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);

    if (numDebugDirs == 0) {
        return FALSE;
    }

    /*
     *  For each debug directory, determine the debug directory type
     *  and cache any information about them.
     */

    pDebugDir = (PIMAGE_DEBUG_DIRECTORY)
      (p->fptr + sizeof(IMAGE_SEPARATE_DEBUG_HEADER) +
       p->sepHdr->NumberOfSections * sizeof(IMAGE_SECTION_HEADER) +
       p->sepHdr->ExportedNamesSize);

    for (li=0; li < numDebugDirs; li++, pDebugDir++) {
        if (((int) pDebugDir->Type) > p->cDebugDir) {
            p->cDebugDir += 10;
            p->rgDebugDir = LocalReAlloc((char *) p->rgDebugDir,
                                    p->cDebugDir * sizeof(p->rgDebugDir[0]),
                                    LMEM_ZEROINIT);
        }

        p->rgDebugDir[pDebugDir->Type] = pDebugDir;
    }

    if (p->rgDebugDir[IMAGE_DEBUG_TYPE_COFF] != NULL) {
        pCoffHdr = (PIMAGE_COFF_SYMBOLS_HEADER) (p->fptr +
          p->rgDebugDir[IMAGE_DEBUG_TYPE_COFF]->PointerToRawData);
        p->AllSymbols = (PIMAGE_SYMBOL)
          ((char *) pCoffHdr + pCoffHdr->LvaToFirstSymbol);
        p->stringTable = pCoffHdr->NumberOfSymbols * IMAGE_SIZEOF_SYMBOL +
          (char *) p->AllSymbols;
        p->numberOfSymbols = pCoffHdr->NumberOfSymbols;
    }
    p->numberOfSections = p->sepHdr->NumberOfSections;

    return TRUE;
}                               /* FillInSeparateImagePointers() */



BOOL
CalculateNtImagePointers(
    PIMAGEPOINTERS p
    )
/*++

Routine Description:

    This function reads an NT image and its associated COFF headers
    and file pointers and build a set of pointers into the mapped image.
    The pointers are all relative to the image's mapped file pointer
    and allow direct access to the necessary data.

Arguments:

    p        - pointer to a IMAGEPOINTERS structure (see cofftocv.h)

Return Value:

    TRUE     - pointers were created
    FALSE    - pointers could not be created

--*/
{
    PIMAGE_DEBUG_DIRECTORY      debugDir;
    PIMAGE_SECTION_HEADER       sh;
    DWORD                       i, li, rva, numDebugDirs;
    PIMAGE_FILE_HEADER          pFileHdr;
    PIMAGE_OPTIONAL_HEADER      pOptHdr;
    DWORD                       offDebugInfo;

    __try {
        /*
         *      Based on wheither or not we find the dos (MZ) header
         *      at the beginning of the file, attempt to get a pointer
         *      to where the PE header is suppose to be.
         */

        p->dosHdr = (PIMAGE_DOS_HEADER) p->fptr;
        if (p->dosHdr->e_magic == IMAGE_DOS_SIGNATURE) {
            p->ntHdr = (PIMAGE_NT_HEADERS)
              ((DWORD)p->dosHdr->e_lfanew + p->fptr);
            p->fRomImage = FALSE;
        } else if (p->dosHdr->e_magic == IMAGE_SEPARATE_DEBUG_SIGNATURE) {
            p->sepHdr = (PIMAGE_SEPARATE_DEBUG_HEADER) p->fptr;
            p->dosHdr = NULL;
            p->fRomImage = FALSE;
            p->cDebugDir = 10;
            p->rgDebugDir = LocalAlloc(LPTR, sizeof(IMAGE_DEBUG_DIRECTORY) * 10 );
            return FillInSeparateImagePointers(p);
        } else {
            p->romHdr = (PIMAGE_ROM_HEADERS) p->fptr;
            if (p->romHdr->FileHeader.SizeOfOptionalHeader ==
                                          IMAGE_SIZEOF_ROM_OPTIONAL_HEADER &&
                p->romHdr->OptionalHeader.Magic ==
                                          IMAGE_ROM_OPTIONAL_HDR_MAGIC) {
                //
                // its a rom image
                //
                p->fRomImage = TRUE;
                p->ntHdr = NULL;
                p->dosHdr = NULL;
            } else {
                p->fRomImage = FALSE;
                p->ntHdr = (PIMAGE_NT_HEADERS) p->fptr;
                p->dosHdr = NULL;
                p->romHdr = NULL;
            }
        }

        /*
         *  What comes next must be a PE header.  If not then pop out
         */

        if ( p->ntHdr ) {
            if ( p->dosHdr && (DWORD)p->dosHdr->e_lfanew > (DWORD)p->fsize ) {
                return FALSE;
            }

            if ( p->ntHdr->Signature != IMAGE_NT_SIGNATURE ) {
                return FALSE;
            }

            /*
             *  We did find a PE header so start setting pointers to various
             *      structures in the exe file.
             */

            pFileHdr = p->fileHdr = &p->ntHdr->FileHeader;
            pOptHdr = p->optHdr = &p->ntHdr->OptionalHeader;
        } else if (p->romHdr) {
            pFileHdr = p->fileHdr = &p->romHdr->FileHeader;
            pOptHdr = (PIMAGE_OPTIONAL_HEADER) &p->romHdr->OptionalHeader;
            p->optHdr = (PIMAGE_OPTIONAL_HEADER) &p->romHdr->OptionalHeader;
        } else {
            return FALSE;
        }

        if (!(pFileHdr->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
            return FALSE;
        }

        if (pFileHdr->Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
            return(FALSE);
        }

        /*
         *  If they exists then get a pointer to the symbol table and
         *      the string table
         */

        if (pFileHdr->PointerToSymbolTable) {
            p->AllSymbols = (PIMAGE_SYMBOL)
                              (pFileHdr->PointerToSymbolTable + p->fptr);
            p->stringTable = (LPSTR)((ULONG)p->AllSymbols +
                           (IMAGE_SIZEOF_SYMBOL * pFileHdr->NumberOfSymbols));
            p->numberOfSymbols = pFileHdr->NumberOfSymbols;
        }

        p->numberOfSections = pFileHdr->NumberOfSections;
        p->cDebugDir = 10;
        p->rgDebugDir = LocalAlloc(LPTR, sizeof(IMAGE_DEBUG_DIRECTORY) * 10 );

        if (p->romHdr) {

            sh = p->sectionHdrs = (PIMAGE_SECTION_HEADER) (p->romHdr+1);

            debugDir = 0;

            for (i=0; i<pFileHdr->NumberOfSections; i++, sh++) {
                if (!strcmp(sh->Name, ".rdata")) {
                    debugDir = (PIMAGE_DEBUG_DIRECTORY)(sh->PointerToRawData + p->fptr);
                }

                if (strncmp(sh->Name,".debug",8)==0) {
                    p->debugSection = sh;
                }
            }

            if (debugDir) {
                do {
                    if ((int)debugDir->Type > p->cDebugDir) {
                        p->cDebugDir += 10;
                        p->rgDebugDir = LocalReAlloc((char *) p->rgDebugDir, p->cDebugDir * sizeof(p->rgDebugDir[0]), LMEM_ZEROINIT);
                    }
                    p->rgDebugDir[debugDir->Type] = debugDir;
                    debugDir++;
                } while (debugDir->Type != 0);
            }
        } else {

            /*
             *  Locate the debug directory
             */

            rva =
              pOptHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

            numDebugDirs =
              pOptHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                sizeof(IMAGE_DEBUG_DIRECTORY);

            if (numDebugDirs == 0) {
                return FALSE;
            }

            sh = p->sectionHdrs = IMAGE_FIRST_SECTION( p->ntHdr );

            /*
             * Find the section the debug directory is in.
             */

            for (i=0; i<pFileHdr->NumberOfSections; i++, sh++) {
                if (rva >= sh->VirtualAddress &&
                    rva < sh->VirtualAddress+sh->SizeOfRawData) {
                    break;
                }
            }

            /*
             *   For each debug directory, determine the debug directory
             *      type and cache any information about them.
             */

            debugDir = (PIMAGE_DEBUG_DIRECTORY) ( rva - sh->VirtualAddress +
                                                 sh->PointerToRawData +
                                                 p->fptr );

            for (li=0; li<numDebugDirs; li++, debugDir++) {
                if (((int) debugDir->Type) > p->cDebugDir) {
                    p->cDebugDir += 10;
                    p->rgDebugDir = LocalReAlloc((char *) p->rgDebugDir,
                                            p->cDebugDir * sizeof(p->rgDebugDir[0]),
                                            LMEM_ZEROINIT);
                }
                p->rgDebugDir[debugDir->Type] = debugDir;
                offDebugInfo = debugDir->AddressOfRawData;
            }

            /*
             *  Check to see if the debug information is mapped and if
             *      there is a section called .debug
             */

            sh = p->sectionHdrs = IMAGE_FIRST_SECTION( p->ntHdr );

            for (i=0; i<pFileHdr->NumberOfSections; i++, sh++) {
                if ((offDebugInfo >= sh->VirtualAddress) &&
                    (offDebugInfo < sh->VirtualAddress+sh->SizeOfRawData)) {
                    p->debugSection = sh;
                    break;
                }
            }
        }

        return TRUE;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }
}                               /* CalcuateNtImagePointers() */
