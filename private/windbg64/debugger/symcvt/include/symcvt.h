/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    symcvt.h

Abstract:

    This file contains all of the type definitions and prototypes
    necessary to access the symcvt library.

Author:

    Wesley A. Witt (wesw) 19-April-1993

Environment:

    Win32, User Mode

--*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagPTRINFO {
    DWORD                       size;
    DWORD                       count;
    PUCHAR                      ptr;
} PTRINFO, *PPTRINFO;

typedef struct tagIMAGEPOINTERS {
    char                        szName[MAX_PATH];
    HANDLE                      hFile;
    BOOL                        CloseFile;      // TRUE if symcvt opened file
    HANDLE                      hMap;
    DWORD                       fsize;
    PUCHAR                      fptr;
    BOOLEAN                     fRomImage;
    PIMAGE_DOS_HEADER           dosHdr;
    PIMAGE_NT_HEADERS           ntHdr;
    PIMAGE_ROM_HEADERS          romHdr;
    PIMAGE_FILE_HEADER          fileHdr;
    PIMAGE_OPTIONAL_HEADER      optHdr;
    PIMAGE_SEPARATE_DEBUG_HEADER sepHdr;
    int                         cDebugDir;
    PIMAGE_DEBUG_DIRECTORY *    rgDebugDir;
    PIMAGE_SECTION_HEADER       sectionHdrs;
    PIMAGE_SECTION_HEADER       debugSection;
    PIMAGE_SYMBOL               AllSymbols;
    PUCHAR                      stringTable;
    int                         numberOfSymbols;
    int                         numberOfSections;
    PCHAR *                     rgpbDebugSave;
} IMAGEPOINTERS, *PIMAGEPOINTERS;

#define COFF_DIR(x)             ((x)->rgDebugDir[IMAGE_DEBUG_TYPE_COFF])
#define CV_DIR(x)               ((x)->rgDebugDir[IMAGE_DEBUG_TYPE_CODEVIEW])

typedef struct _MODULEINFO {
    DWORD               iMod;
    DWORD               cb;
    DWORD               SrcModule;
    LPSTR               name;
} MODULEINFO, *LPMODULEINFO;

typedef struct tagPOINTERS {
    IMAGEPOINTERS               iptrs;         // input file pointers
    IMAGEPOINTERS               optrs;         // output file pointers
    PTRINFO                     pCvStart;      // start of cv info
    PUCHAR                      pCvCurr;       // current cv pointer
    PTRINFO                     pCvModules;    // module information
    PTRINFO                     pCvSrcModules; // source module information
    PTRINFO                     pCvPublics;    // publics information
    PTRINFO                     pCvSegName;    // segment names
    PTRINFO                     pCvSegMap;     // segment map
    PTRINFO                     pCvSymHash;    // symbol hash table
    PTRINFO                     pCvAddrSort;   // address sort table
    LPMODULEINFO                pMi;
    DWORD                       modcnt;
} POINTERS, *PPOINTERS;

typedef  char *  (* CONVERTPROC) (HANDLE, char *);

BOOL MapInputFile ( PPOINTERS p, HANDLE hFile, char *fname);
BOOL UnMapInputFile ( PPOINTERS p );
BOOL CalculateNtImagePointers( PIMAGEPOINTERS p );

#define align(_n)       ((4 - (( (DWORD)_n ) % 4 )) & 3)

#define ValidateHeap()

#ifdef __cplusplus
} // extern "C" {
#endif
