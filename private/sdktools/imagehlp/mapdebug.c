#include <private.h>
#include <symbols.h>

#ifdef MAP_DEBUG_TEST
#undef MemAlloc
#undef MemFree

PVOID
MemAlloc(ULONG_PTR AllocSize)
{
    return LocalAlloc(LPTR, AllocSize);
}

VOID
MemFree(PVOID MemPtr)
{
    LocalFree(MemPtr);
}

#endif

// these are used only by ...
//  GetSymbolFileFromServer()
//  CloseSymbolServer()

HINSTANCE              ghSrv = 0;
CHAR                   gszSrvName[_MAX_PATH * 2];
LPSTR                  gszSrvParams = NULL; 
PSYMBOLSERVERPROC      gfnSymbolServer = NULL;
PSYMBOLSERVERCLOSEPROC gfnSymbolServerClose = NULL;

BOOL
ProcessImageDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD
    );


BOOL
IsImageMachineType64(
    DWORD MachineType
    )
{
   switch(MachineType) {
   case IMAGE_FILE_MACHINE_AXP64:
   case IMAGE_FILE_MACHINE_IA64:
       return TRUE;
   default:
       return FALSE;
   }
} 

ULONG
ReadImageData(
    IN  HANDLE  hprocess,
    IN  ULONG64 ul,
    IN  ULONG64 addr,
    OUT LPVOID  buffer,
    IN  ULONG   size
    )
{
    ULONG bytesread;

    if (hprocess) {

        ULONG64 base = ul;

        BOOL rc;

        rc = ReadInProcMemory(hprocess, 
                              base + addr, 
                              buffer, 
                              size, 
                              &bytesread);
        
        if (!rc || (bytesread < (ULONG)size)) 
            return 0;
    
    } else {

        PCHAR p = (PCHAR)ul + addr;

        memcpy(buffer, p, size);
    } 

    return size;
}

PVOID
MapItRO(
      HANDLE FileHandle
      )
{
    PVOID MappedBase = NULL;

    if (FileHandle) {

        HANDLE MappingHandle = CreateFileMapping( FileHandle, NULL, PAGE_READONLY, 0, 0, NULL );
        if (MappingHandle) {
            MappedBase = MapViewOfFile( MappingHandle, FILE_MAP_READ, 0, 0, 0 );
            CloseHandle(MappingHandle);
        }
    }

    return MappedBase;
}


void
CloseSymbolServer(
    VOID
    )
{
    if (!ghSrv) 
        return;

    if (gfnSymbolServerClose)
        gfnSymbolServerClose();

    FreeLibrary(ghSrv);

    ghSrv = 0;
    *gszSrvName = 0;
    gszSrvParams = NULL;
    gfnSymbolServer = NULL;
    gfnSymbolServerClose = NULL;
}


BOOL
GetSymbolFileFromServer(
    IN  LPCSTR ServerInfo, 
    IN  LPCSTR FileName, 
    IN  DWORD  num1,
    IN  DWORD  num2,
    IN  DWORD  num3,
    OUT LPSTR FilePath
    )
{
    BOOL rc;
    CHAR *params;
    
    // initialize server, if needed

    if (!ghSrv) {
        ghSrv = INVALID_HANDLE_VALUE;
        strcpy(gszSrvName, &ServerInfo[7]);
        if (!*gszSrvName) 
            return FALSE;
        // BUGBUG: gszSrvParams is no longer needed because it is 
        // now implemented in the local variable, 'params'
        // Still, we need to zero out this location to get gszSrvName.
        gszSrvParams = strchr(gszSrvName, '*');
        if (!gszSrvParams ) 
            return FALSE;
        *gszSrvParams++ = '\0';
        ghSrv = LoadLibrary(gszSrvName);
        if (ghSrv) {
            gfnSymbolServer = (PSYMBOLSERVERPROC)GetProcAddress(ghSrv, "SymbolServer");
            if (!gfnSymbolServer) {
                FreeLibrary(ghSrv);
                ghSrv = INVALID_HANDLE_VALUE;
            }
            gfnSymbolServerClose = (PSYMBOLSERVERCLOSEPROC)GetProcAddress(ghSrv, "SymbolServerClose");
        } else {
            ghSrv = INVALID_HANDLE_VALUE;
        }
    }

    // bail, if we have no valid server

    if (ghSrv == INVALID_HANDLE_VALUE) {
        DPRINTF(NULL, "SymSrv load failure: %s\n", gszSrvName);
        return FALSE;
    }

    params = strchr(ServerInfo, '*');
    if (!params)
        return FALSE;
    params = strchr(params+1, '*');
    if (!params)
        return FALSE;
    rc = gfnSymbolServer(params+1, FileName, num1, num2, num3, FilePath);

    if (!*FilePath) {
        DPRINTF(NULL, 
                "SymSrv: %s not in %s, (0x%x 0x%x 0x%x)\n",
                FileName,
                params,
                num1,
                num2,
                num3);
    }

    return rc;
}


__inline
EC
SetPDBError(
    EC ccode,
    EC ncode
    )
{
    if (ncode == EC_OK)
        return ncode;

    if (ccode != EC_NOT_FOUND)
        return ccode;

    return ncode;
}

PDB *
LocatePdb(
    char *szPDB,
    ULONG PdbAge,
    ULONG PdbSignature,
    char *SymbolPath,
    char *szImageExt,
    BOOL  fImagePathPassed
    )
{
    PDB  *pdb = NULL;
    EC    ec;
    char  szError[cbErrMax] = "";
    char  szPDBSansPath[_MAX_FNAME];
    char  szPDBExt[_MAX_EXT];
    char  szPDBLocal[_MAX_PATH];
    char  szDbgPath[PDB_MAX_PATH];
    char *SemiColon;
    DWORD pass;
    EC    ecode = EC_NOT_FOUND;
    BOOL  symsrv = TRUE;
    char  szPDBName[_MAX_PATH];

    // SymbolPath is a semicolon delimited path (reference path first)

    strcpy (szPDBLocal, szPDB);
    _splitpath(szPDBLocal, NULL, NULL, szPDBSansPath, szPDBExt);

    do {
        SemiColon = strchr(SymbolPath, ';');

        if (SemiColon) {
            *SemiColon = '\0';
        }
 
        if (fImagePathPassed) {
            pass = 2;
            fImagePathPassed = 0;;
        } else {
            pass = 0;
        }
 
        if (SymbolPath) {
do_again:
            if (!_strnicmp(SymbolPath, "SYMSRV*", 7)) {
                
                *szPDBLocal = 0;
                sprintf(szPDBName, "%s%s", szPDBSansPath, ".pdb");
                if (symsrv) {
                    GetSymbolFileFromServer(SymbolPath, 
                                            szPDBName, 
                                            PdbSignature,
                                            PdbAge,
                                            0,
                                            szPDBLocal);
                    symsrv = FALSE;
                }
            
            } else {
            
                strcpy(szPDBLocal, SymbolPath);
                EnsureTrailingBackslash(szPDBLocal);
                
                // search order is ...
                //
                //   %dir%\symbols\%ext%\%file%
                //   %dir%\%ext%\%file%
                //   %dir%\%file%
                
                switch (pass)
                {
                case 0:
                    strcat(szPDBLocal, "symbols");
                    EnsureTrailingBackslash(szPDBLocal);
                    // pass through
                case 1:
                    strcat(szPDBLocal, szImageExt);
                    // pass through
                default:
                    EnsureTrailingBackslash(szPDBLocal);
                    break;
                }
    
                strcat(szPDBLocal, szPDBSansPath);
                strcat(szPDBLocal, szPDBExt);
            }

            if (*szPDBLocal) {

                DPRINTF(NULL, "LocatePDB-> Looking for %s... ", szPDBLocal);
                PDBOpenValidate(szPDBLocal, NULL, "r", PdbSignature, PdbAge, &ec, szError, &pdb);
                ecode = SetPDBError(ecode, ec);
                if (pdb) {
                    assert(ec == EC_OK);
                    break;
                } else {
                    if (ec == EC_INVALID_SIG || ec == EC_INVALID_AGE) {
                        EPRINTF(NULL, "unmatched pdb ");
                        if (PdbSignature == 0 && PdbAge == 0) {
                            if (PDBOpen(szError, "r", 0, &ec, szError, &pdb)) {
                                break;
                            }
                        }
                        EPRINTF(NULL, "\n");
                    } else if (ec == EC_NOT_FOUND) {
                        EPRINTF(NULL, "file not found\n");
                    } else {
                        EPRINTF(NULL, "pdb error 0x%x\n", ec);
                    }
                    
                    if (pass < 2) {
                        pass++;
                        goto do_again;
                    }
                }
            }
        }

        if (SemiColon) {
            *SemiColon = ';';
             SemiColon++;
             symsrv = TRUE;
        }

        SymbolPath = SemiColon;
    } while (SemiColon);

    if (!pdb) {
        strcpy(szPDBLocal, szPDB);
        DPRINTF(NULL, "LocatePDB-> Looking for %s... ", szPDBLocal);
        PDBOpenValidate(szPDBLocal, NULL, "r", PdbSignature, PdbAge, &ec, szError, &pdb);
        if (!pdb) {
            if (ec == EC_INVALID_SIG || ec == EC_INVALID_AGE) {
                EPRINTF(NULL, "unmatched pdb ");
                if (PdbSignature == 0 && PdbAge == 0) {
                    PDBOpen(szError, "r", 0, &ec, szError, &pdb);                    
                }
                if (!pdb)
                    EPRINTF(NULL, "\n");
            } else if (ec == EC_NOT_FOUND) {
                EPRINTF(NULL, "file not found\n");
            } else {
                EPRINTF(NULL, "pdb error 0x%x\n", ec);
            }
        }                                                                     
    }

    if (pdb) {
        EPRINTF(NULL, "OK\n");
        // Store the name of the PDB we actually opened for later reference.
        strcpy(szPDB, szPDBLocal);
        SetLastError(NO_ERROR);
    }


    return pdb;
}


BOOL
ProcessOldStyleCodeView(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    OMFSignature    *omfSig;
    OMFDirHeader    *omfDirHdr;
    OMFDirEntry     *omfDirEntry;
    OMFSegMap       *omfSegMap;
    OMFSegMapDesc   *omfSegMapDesc;
    DWORD            i, j, k, SectionSize;
    DWORD            SectionStart;
    PIMAGE_SECTION_HEADER   Section;

    if (pIDD->cOmapFrom) {
        // If there's omap, we need to generate the original section map

        omfSig = (OMFSignature *)pIDD->pMappedCv;
        omfDirHdr = (OMFDirHeader*) ((PCHAR)pIDD->pMappedCv + (DWORD)omfSig->filepos);
        omfDirEntry = (OMFDirEntry*) ((PCHAR)omfDirHdr + sizeof(OMFDirHeader));

        for (i=0; i<omfDirHdr->cDir; i++,omfDirEntry++) {
            if (omfDirEntry->SubSection == sstSegMap) {

                omfSegMap = (OMFSegMap*) ((PCHAR)pIDD->pMappedCv + omfDirEntry->lfo);

                omfSegMapDesc = (OMFSegMapDesc*)&omfSegMap->rgDesc[0];

                SectionStart = *(DWORD *)pIDD->pOmapFrom;

                Section = (PIMAGE_SECTION_HEADER) MemAlloc(omfSegMap->cSeg * sizeof(IMAGE_SECTION_HEADER));

                if (Section) {
                    for (j=0, k=0; j < omfSegMap->cSeg; j++) {
                        if (omfSegMapDesc[j].frame) {
                            // The linker sets the frame field to the actual section header number.  Zero is
                            // used to track absolute symbols that don't exist in a real sections.

                            Section[k].Misc.VirtualSize =
                                SectionSize = omfSegMapDesc[j].cbSeg;
                            Section[k].VirtualAddress =
                                SectionStart =
                                    SectionStart + ((SectionSize + (pIDD->ImageAlign-1)) & ~(pIDD->ImageAlign-1));
                            k++;
                        }
                    }

                    pIDD->pOriginalSections = Section;
                    pIDD->cOriginalSections = k;
                }
            }
        }
    }

    return TRUE;
}

BOOL
ProcessPdbDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    PDB    *pPdb;
    Dbg    *pDbg;
    DBI    *pDbi;
    GSI    *pGsi;
    int     DebugCount;
    void   *DebugData;
    PCHAR   szLocalSymbolPath = NULL;
    DWORD   cpathlen = 0;
    CHAR    szExt[_MAX_EXT] = {0};
    BOOL    fImagePathPassed = FALSE;

    if (pIDD->SymbolPath) 
        cpathlen = strlen(pIDD->SymbolPath);
    szLocalSymbolPath = MemAlloc(cpathlen + strlen(pIDD->PdbReferencePath) + 2);
    if (!szLocalSymbolPath) {
        return FALSE;
    }

    strcpy(szLocalSymbolPath, pIDD->PdbReferencePath);
    if (*szLocalSymbolPath) {            
        if (pIDD->SymbolPath)                             
            strcat(szLocalSymbolPath, ";");
        fImagePathPassed = TRUE;
    }
    if (pIDD->SymbolPath)                           
        strcat(szLocalSymbolPath, pIDD->SymbolPath);
    
    if (*pIDD->ImageFilePath) {
        _splitpath(pIDD->ImageFilePath, NULL, NULL, NULL, szExt);
    } else if (*pIDD->ImageName) {
        _splitpath(pIDD->ImageName, NULL, NULL, NULL, szExt);
    }

    // if we have no valid filename, then this must be an executable

    if (!*szExt)
        strcpy(szExt, ".exe");
        
    // go ahead and get it
    
    pPdb = LocatePdb(pIDD->PdbFileName, pIDD->PdbAge, pIDD->PdbSignature, szLocalSymbolPath, &szExt[1], fImagePathPassed);
    MemFree(szLocalSymbolPath);
    if (!pPdb) {
        return FALSE;
    }

    if (!PDBOpenDBI( pPdb, "r", "", &pDbi )) {
        PDBClose( pPdb );
        return FALSE;
    }

    if (!DBIOpenPublics( pDbi, &pGsi)) {
        DBIClose( pDbi );
        PDBClose( pPdb );
        return FALSE;
    }

    // Read in the omap.

    if (DBIOpenDbg(pDbi, dbgtypeOmapFromSrc, &pDbg)) {
        DebugCount = DbgQuerySize(pDbg);
        if (DebugCount) {
            DebugData = MemAlloc( DebugCount * sizeof(OMAP) );

            if (DbgQueryNext((Dbg *) pDbg, DebugCount, DebugData)) {
                pIDD->cOmapFrom = DebugCount;
                pIDD->pOmapFrom = DebugData;
            }
        }
        DbgClose(pDbg);
    }

    if (DBIOpenDbg(pDbi, dbgtypeOmapToSrc, &pDbg)) {
        DebugCount = DbgQuerySize(pDbg);
        if (DebugCount) {
            DebugData = MemAlloc( DebugCount * sizeof(OMAP) );

            if (DbgQueryNext((Dbg *) pDbg, DebugCount, DebugData)) {
                pIDD->cOmapTo = DebugCount;
                pIDD->pOmapTo = DebugData;
            }
        }

        DbgClose(pDbg);
    }

    // Read in the fpo (if it exists)

    if (DBIOpenDbg(pDbi, dbgtypeFPO, &pDbg)) {
        DebugCount = DbgQuerySize(pDbg);
        if (DebugCount) {
            DebugData = MemAlloc( DebugCount * sizeof(FPO_DATA) );

            if (DbgQueryNext((Dbg *) pDbg, DebugCount, DebugData)) {
                pIDD->cFpo = DebugCount;
                pIDD->pFpo = DebugData;
            }
        }
        DbgClose(pDbg);
    }

    // Read in the Pdata - BUGBUG: Using largest size for now
#if 0
    if (DBIOpenDbg(pDbi, dbgtypeException, &pDbg)) {
        DebugCount = DbgQuerySize(pDbg);
        if (DebugCount) {
            DebugData = MemAlloc( DebugCount * sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) );

            if (DbgQueryNext((Dbg *) pDbg, DebugCount, DebugData)) {
                pIDD->NumberOfPdataFunctionEntries = DebugCount;
                pIDD->pImageFunction = DebugData;
            }
        }
        DbgClose(pDbg);
    }
#endif

    if (!pIDD->pCurrentSections) {
        if (DBIOpenDbg(pDbi, dbgtypeSectionHdr, &pDbg)) {
            DebugCount = DbgQuerySize(pDbg);
            if (DebugCount) {
                DebugData = MemAlloc( DebugCount * sizeof(IMAGE_SECTION_HEADER) );
    
                if (DbgQueryNext((Dbg *) pDbg, DebugCount, DebugData)) {
                    pIDD->cCurrentSections = DebugCount;
                    pIDD->pCurrentSections = DebugData;
                }
            }
            DbgClose(pDbg);
        }
    }

    if (pIDD->cOmapFrom) {
        // If the image has omap, we need the original section headers.
        OMFSegMap *  omfSegMap = NULL;

        // Create the sec map from the CV symbolic.

        if (DBIQuerySecMap(pDbi, NULL, &DebugCount) &&
            (omfSegMap = (OMFSegMap *) MemAlloc (DebugCount)) &&
            DBIQuerySecMap(pDbi, (char *)omfSegMap, &DebugCount))
        {
            OMFSegMapDesc          *omfSegMapDesc = (OMFSegMapDesc*)&omfSegMap->rgDesc[0];
            DWORD                   j = 0, k=0, SectionSize = 0;
            DWORD                   SectionStart = *(DWORD *)pIDD->pOmapFrom;
            PIMAGE_SECTION_HEADER   Section = (PIMAGE_SECTION_HEADER) MemAlloc(omfSegMap->cSeg * sizeof(IMAGE_SECTION_HEADER));

            for (j=0; j < omfSegMap->cSeg; j++) {
                if (omfSegMapDesc[j].frame) {
                    // The linker sets the frame field to the actual section header number.  Zero is
                    // used to track absolute symbols that don't exist in a real sections.

                    Section[k].VirtualAddress =
                        SectionStart =
                            SectionStart + ((SectionSize + (pIDD->ImageAlign-1)) & ~(pIDD->ImageAlign-1));
                    Section[k].Misc.VirtualSize =
                        SectionSize = omfSegMapDesc[j].cbSeg;
                    k++;
                }
            }

            pIDD->pOriginalSections = Section;
            pIDD->cOriginalSections = k;

            MemFree(omfSegMap);
        }
    }

    pIDD->pPdb = pPdb;
    pIDD->pDbi = pDbi;
    pIDD->pGsi = pGsi;
    return TRUE;
}

__inline
DWORD
IsDataInSection (PIMAGE_SECTION_HEADER Section,
                 PIMAGE_DATA_DIRECTORY Data
                 )
{
    DWORD RealDataOffset;
    if ((Data->VirtualAddress >= Section->VirtualAddress) &&
        ((Data->VirtualAddress + Data->Size) <= (Section->VirtualAddress + Section->SizeOfRawData))) {
        RealDataOffset = (DWORD)(Data->VirtualAddress -
                                 Section->VirtualAddress +
                                 Section->PointerToRawData);
    } else {
        RealDataOffset = 0;
    }
    return RealDataOffset;
}

__inline
DWORD
SectionContains (
    HANDLE hp,
    PIMAGE_SECTION_HEADER pSH,
    PIMAGE_DATA_DIRECTORY ddir
    )
{
    DWORD rva = 0;

    if (!ddir->VirtualAddress)
        return 0;

    if (ddir->VirtualAddress >= pSH->VirtualAddress) {
        if ((ddir->VirtualAddress + ddir->Size) <= (pSH->VirtualAddress + pSH->SizeOfRawData)) {
            rva = ddir->VirtualAddress;
            if (!hp) 
                rva = rva - pSH->VirtualAddress + pSH->PointerToRawData;
        }
    }

    return rva;
}

typedef struct NB10I                   // NB10 debug info
{
    DWORD   nb10;                      // NB10
    DWORD   off;                       // offset, always 0
    DWORD   sig;
    DWORD   age;
} NB10I;

void
RetrievePdbInfo(
    PIMGHLP_DEBUG_DATA pIDD,
    CHAR const *szReference
    )
{
    NB10I *pNb10 = (NB10I *)pIDD->pMappedCv;
    CHAR szRefDrive[_MAX_DRIVE];
    CHAR szRefPath[_MAX_DIR];

    if (pIDD->PdbSignature) {
        return;
    }

    if (pNb10->nb10 != '01BN') {
        return;
    }

    pIDD->PdbAge = pNb10->age;
    pIDD->PdbSignature = pNb10->sig;
    strcpy(pIDD->PdbFileName, (PCHAR)pNb10 + sizeof(NB10I));
    _splitpath(szReference, szRefDrive, szRefPath, NULL, NULL);
    _makepath(pIDD->PdbReferencePath, szRefDrive, szRefPath, NULL, NULL);
    if (strlen(szRefPath) > 1) {
        // Chop off trailing backslash.
        pIDD->PdbReferencePath[strlen(pIDD->PdbReferencePath)-1] = '\0';
    } else {
        // No path.  Put on at least a dot "."
        strcpy(pIDD->PdbReferencePath, ".");
    }
    return;
}

BOOL
FakePdbName(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    CHAR szName[_MAX_FNAME];
    
    if (pIDD->PdbSignature) {
        return FALSE;
    }

    if (!pIDD->ImageName)
        return FALSE;

    _splitpath(pIDD->ImageName, NULL, NULL, szName, NULL);
    if (!*szName)
        return FALSE;

    strcpy(pIDD->PdbFileName, szName);
    strcat(pIDD->PdbFileName, ".pdb");

    return TRUE;
}

BOOL
FindDebugInfoFileExCallback(
    HANDLE FileHandle,
    PSTR FileName,
    PVOID CallerData
    )
{
    PIMGHLP_DEBUG_DATA pIDD;
    PIMAGE_SEPARATE_DEBUG_HEADER DbgHeader;
    PVOID FileMap;
    BOOL  rc;

    rc = TRUE;

    if (!CallerData)
        return TRUE;

    pIDD = (PIMGHLP_DEBUG_DATA)CallerData;

    FileMap = MapItRO(FileHandle);

    if (!FileMap) {
        return FALSE;
    }

    DbgHeader = (PIMAGE_SEPARATE_DEBUG_HEADER)FileMap;

    // Only support .dbg files for X86 and Alpha (32 bit).

    if ((DbgHeader->Signature != IMAGE_SEPARATE_DEBUG_SIGNATURE) ||
        ((DbgHeader->Machine != IMAGE_FILE_MACHINE_I386) &&
         (DbgHeader->Machine != IMAGE_FILE_MACHINE_ALPHA)))
    {
        rc = FALSE;
        goto cleanup;
    }

    // ignore checksums, they are bogus
    rc = (pIDD->TimeDateStamp == DbgHeader->TimeDateStamp) ? TRUE : FALSE;

cleanup:
    if (FileMap)
        UnmapViewOfFile(FileMap);

    return rc;
}

BOOL
ProcessDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD,
    DWORD datasrc
    )
{
    BOOL                         status;
    ULONG                        cb;
    IMAGE_DOS_HEADER             dh;
    IMAGE_NT_HEADERS32           nh32;
    IMAGE_NT_HEADERS64           nh64;
    PIMAGE_ROM_OPTIONAL_HEADER   rom;
    IMAGE_SEPARATE_DEBUG_HEADER  sdh;
    PIMAGE_FILE_HEADER           fh;
    PIMAGE_DEBUG_MISC            md;
    ULONG                        ddva;
    ULONG                        shva;
    ULONG                        nSections;
    PIMAGE_SECTION_HEADER        psh;
    IMAGE_DEBUG_DIRECTORY        dd;
    PIMAGE_DATA_DIRECTORY        datadir;
    PCHAR                        pCV;
    ULONG                        i;
    int                          nDebugDirs = 0;
    HANDLE                       hp;
    ULONG64                      base;

    DWORD                        rva;
    PCHAR                        filepath;
    IMAGE_EXPORT_DIRECTORY       expdir;
    DWORD                        fsize;
    BOOL                         rc;
    USHORT                       filetype;

    // setup pointers for grabing data

    switch (datasrc) {
    case dsInProc:
        hp = pIDD->hProcess;
        base = pIDD->InProcImageBase;
        fsize = 0;
        filepath = pIDD->ImageFilePath;
        break;
    case dsImage:
        hp = NULL;
        // BUGBUG: localize this!
        pIDD->ImageMap = MapItRO(pIDD->ImageFileHandle);
        base = (ULONG64)pIDD->ImageMap;
        fsize = GetFileSize(pIDD->ImageFileHandle, NULL);
        filepath = pIDD->ImageFilePath;
        break;
    case dsDbg:
        hp = NULL;
        // BUGBUG: localize this!
        pIDD->DbgFileMap = MapItRO(pIDD->DbgFileHandle);
        base = (ULONG64)pIDD->DbgFileMap;
        fsize = GetFileSize(pIDD->DbgFileHandle, NULL);
        filepath = pIDD->DbgFilePath;
        break;
    default:
        return FALSE;
    }

    // some initialization
    pIDD->fNeedImage = FALSE;
    rc = FALSE;

    __try {

        // test the file type
    
        status = ReadImageData(hp, base, 0, &filetype, sizeof(filetype));
        if (!status)
            return FALSE;
        if (filetype == IMAGE_DOS_SIGNATURE)
            goto image;
        if (filetype == IMAGE_SEPARATE_DEBUG_SIGNATURE)
            goto dbg;
        return FALSE;
        
dbg:
    
        // grab the dbg header
    
        status = ReadImageData(hp, base, 0, &sdh, sizeof(sdh));
        if (!status)
            return FALSE;
        
        // Only support .dbg files for X86 and Alpha (32 bit).
    
        if ((sdh.Machine != IMAGE_FILE_MACHINE_I386) 
            && (sdh.Machine != IMAGE_FILE_MACHINE_ALPHA))
        {
            UnmapViewOfFile(pIDD->DbgFileMap);
            pIDD->DbgFileMap = 0;
            return FALSE;
        }
    
        pIDD->ImageAlign = sdh.SectionAlignment;
        pIDD->CheckSum = sdh.CheckSum;
        pIDD->Machine = sdh.Machine;
        pIDD->TimeDateStamp = sdh.TimeDateStamp;
        pIDD->Characteristics = sdh.Characteristics;
        if (!pIDD->ImageBaseFromImage) {
            pIDD->ImageBaseFromImage = sdh.ImageBase;
        }
    
        if (!pIDD->SizeOfImage) {
            pIDD->SizeOfImage = sdh.SizeOfImage;
        }
    
        nSections = sdh.NumberOfSections;
        psh = (PIMAGE_SECTION_HEADER) MemAlloc(nSections * sizeof(IMAGE_SECTION_HEADER));
        status = ReadImageData(hp, 
                               base, 
                               sizeof(IMAGE_SEPARATE_DEBUG_HEADER), 
                               psh, 
                               nSections * sizeof(IMAGE_SECTION_HEADER));
        if (!status) 
            goto debugdirs;
        
        pIDD->pCurrentSections  = (PCHAR)psh;
        pIDD->cCurrentSections  = nSections;
//        pIDD->ExportedNamesSize = sdh.ExportedNamesSize;
        
        if (sdh.DebugDirectorySize) {
            nDebugDirs = (int)(sdh.DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY));
            ddva = sizeof(IMAGE_SEPARATE_DEBUG_HEADER) 
                   + (sdh.NumberOfSections * sizeof(IMAGE_SECTION_HEADER)) 
                   + sdh.ExportedNamesSize;
        }
    
        goto debugdirs;
    
image:
    
        // grab the dos header
    
        status = ReadImageData(hp, base, 0, &dh, sizeof(dh));
        if (!status)
            return FALSE;
    
        // grab the pe header
    
        status = ReadImageData(hp, base, dh.e_lfanew, &nh32, sizeof(nh32));
        if (!status)
            return FALSE;
    
        // read header info
    
        if (nh32.Signature != IMAGE_NT_SIGNATURE) {
            
            // if header is not NT sig, this is a ROM image
    
            rom = (PIMAGE_ROM_OPTIONAL_HEADER)&nh32.OptionalHeader;
            if (rom->Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC) {
                fh = &nh32.FileHeader;
                pIDD->fROM = TRUE;
                pIDD->iohMagic = rom->Magic;
    
                pIDD->ImageBaseFromImage = rom->BaseOfCode;
                pIDD->SizeOfImage = rom->SizeOfCode;
                pIDD->CheckSum = 0;
            } else {
                return FALSE;
            }
        
        } else {
    
            // otherwise, get info from appropriate header type for 32 or 64 bit
    
            if (IsImageMachineType64(nh32.FileHeader.Machine)) {
    
                // Reread the header as a 64bit header.
                status = ReadImageData(hp, base, dh.e_lfanew, &nh64, sizeof(nh64));
                if (!status)
                    return FALSE;
    
                fh = &nh64.FileHeader;
                datadir = nh64.OptionalHeader.DataDirectory;
                shva = dh.e_lfanew + sizeof(nh64);
                pIDD->iohMagic = nh64.OptionalHeader.Magic;
                pIDD->fPE64 = TRUE;       // seems to be unused
    
                // BUGBUG: get rid of this mapping!
                if (datasrc == dsImage) {
                    pIDD->ImageBaseFromImage = nh64.OptionalHeader.ImageBase;
                    pIDD->ImageAlign = nh64.OptionalHeader.SectionAlignment;
                    pIDD->CheckSum = nh64.OptionalHeader.CheckSum;
                }
                pIDD->SizeOfImage = nh64.OptionalHeader.SizeOfImage;
            }   
            else {
                fh = &nh32.FileHeader;
                datadir = nh32.OptionalHeader.DataDirectory;
                shva = dh.e_lfanew + sizeof(nh32);
                pIDD->iohMagic = nh32.OptionalHeader.Magic;
                
                // BUGBUG: get rid of this mapping!
                if (datasrc == dsImage) {
                    pIDD->ImageBaseFromImage = nh32.OptionalHeader.ImageBase;
                    pIDD->ImageAlign = nh32.OptionalHeader.SectionAlignment;
                    pIDD->CheckSum = nh32.OptionalHeader.CheckSum;
                }
                pIDD->SizeOfImage = nh32.OptionalHeader.SizeOfImage;
            }
        }
    
        // read the section headers
    
        nSections = fh->NumberOfSections;
        psh = (PIMAGE_SECTION_HEADER) MemAlloc(nSections * sizeof(IMAGE_SECTION_HEADER));
        status = ReadImageData(hp, base, shva, psh, nSections * sizeof(IMAGE_SECTION_HEADER));
        if (!status) 
            goto debugdirs;
        
        // store off info to return struct
    
        pIDD->pCurrentSections = (PCHAR)psh;
        pIDD->cCurrentSections = nSections;
        pIDD->Machine = fh->Machine;
        pIDD->TimeDateStamp = fh->TimeDateStamp;
        pIDD->Characteristics = fh->Characteristics;
    
        if (pIDD->fROM)
            goto debugdirs;
    
        // get information from the sections
        
        for (i = 0; i < nSections; i++, psh++) {
            DWORD offset;
    
            if (offset = SectionContains(hp, psh, &datadir[IMAGE_DIRECTORY_ENTRY_EXPORT]))
            {
                status = ReadImageData(hp, base, offset, &expdir, sizeof(expdir));
                memcpy(&pIDD->expdir, &expdir, sizeof(expdir));
                pIDD->pMappedExportDirectory = (PCHAR)&pIDD->expdir;
//              pIDD->ExportedNamesSize = psh->SizeOfRawData;
                pIDD->dsExports = datasrc;
            }
            
            if (offset = SectionContains(hp, psh, &datadir[IMAGE_DIRECTORY_ENTRY_DEBUG]))
            {
                ddva = offset;
                nDebugDirs = datadir[IMAGE_DIRECTORY_ENTRY_DEBUG].Size / sizeof(IMAGE_DEBUG_DIRECTORY);
            }
        }
    
debugdirs:
        
        rc = TRUE;

        // copy the virtual addr of the debug directories over for MapDebugInformation

        if (datasrc == dsImage) {
            pIDD->ddva = ddva;
            pIDD->cdd  = nDebugDirs;
        }
    
        // read the debug directories

        while (nDebugDirs) {
            
            status = ReadImageData(hp, base, (ULONG_PTR)ddva, &dd, sizeof(dd));
            if (!status) 
                break;
            if (!dd.SizeOfData) 
                goto nextdebugdir;
    
            // these debug directories are processed both in-proc and from file
    
            switch (dd.Type) 
            {
            case IMAGE_DEBUG_TYPE_CODEVIEW:
                // get info on pdb file
                if (hp && dd.AddressOfRawData) {
                    // in-proc image               
                    if (!(pCV = MemAlloc(dd.SizeOfData)))
                        break;
                    status = ReadImageData(hp, base, dd.AddressOfRawData, pCV, dd.SizeOfData);
                    if (!status) {
                        MemFree(pCV);
                        break;
                    }
                } else {
                    // file-base image
                    if (dd.PointerToRawData >= fsize) 
                        break;
                    pCV = (PCHAR)base + dd.PointerToRawData;
                    pIDD->fCvMapped = TRUE;
                }
                pIDD->pMappedCv = (PCHAR)pCV;
                pIDD->cMappedCv = dd.SizeOfData;
                pIDD->dsCV = datasrc;
                RetrievePdbInfo(pIDD, filepath);
                break;
            
            case IMAGE_DEBUG_TYPE_MISC:
                // on stripped files, find the dbg file
                // on dbg file, find the original file name
                if (dd.PointerToRawData < fsize) {
                    md = (PIMAGE_DEBUG_MISC)((PCHAR)base + dd.PointerToRawData);
                    if (md->DataType != IMAGE_DEBUG_MISC_EXENAME) 
                        break;
                    if (datasrc == dsDbg) {
                        if (!*pIDD->OriginalImageFileName)
                            strncpy(pIDD->OriginalImageFileName, md->Data, sizeof(pIDD->OriginalImageFileName));
                        break;
                    }
                    if (fh->Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
                        strncpy(pIDD->OriginalDbgFileName, md->Data, sizeof(pIDD->OriginalDbgFileName));
                    } else {
                        strncpy(pIDD->OriginalImageFileName, md->Data, sizeof(pIDD->OriginalImageFileName));
                    }
                }
                break;
            
            case IMAGE_DEBUG_TYPE_COFF:
                if (dd.PointerToRawData < fsize) {
//                  pIDD->fNeedImage = TRUE;
                    pIDD->pMappedCoff = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cMappedCoff = dd.SizeOfData;
                    pIDD->fCoffMapped = TRUE;
                    pIDD->dsCoff = datasrc;
                } else {
                    pIDD->fNeedImage = TRUE;
                }
                break;
#ifdef INPROC_SUPPORT
            case IMAGE_DEBUG_TYPE_FPO:
                if (dd.PointerToRawData < fsize) {
                    pIDD->pFpo = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cFpo = dd.SizeOfData / SIZEOF_RFPO_DATA;
                    pIDD->fFpoMapped = TRUE;
                    pIDD->dsFPO = datasrc;
                } else {
                    DPRINTF(NULL, "found fpo in-process\n");
                }
                break;
            case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                if (dd.PointerToRawData < fsize) {
                    pIDD->pOmapTo = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cOmapTo = dd.SizeOfData / sizeof(OMAP);
                    pIDD->fOmapToMapped = TRUE;
                    pIDD->dsOmapTo = datasrc;
                } else {
                    DPRINTF(NULL, "found found omap-to in-process\n");
                }
                break;

            case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                if (dd.PointerToRawData < fsize) {
                    pIDD->pOmapFrom = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cOmapFrom = dd.SizeOfData / sizeof(OMAP);
                    pIDD->fOmapFromMapped = TRUE;
                    pIDD->dsOmapFrom = datasrc;
                } else {
                    DPRINTF(NULL, "found omap-from in-process\n");
                }
                break;
#endif
            }
    
            // these debug directories are only processed for disk-based images
    
            if (dd.PointerToRawData < fsize) {
    
                switch (dd.Type) 
                {
                case IMAGE_DEBUG_TYPE_FPO:
                    pIDD->pFpo = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cFpo = dd.SizeOfData / SIZEOF_RFPO_DATA;
                    pIDD->fFpoMapped = TRUE;
                    pIDD->dsFPO = datasrc;
                    break;
    
                case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                    pIDD->pOmapTo = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cOmapTo = dd.SizeOfData / sizeof(OMAP);
                    pIDD->fOmapToMapped = TRUE;
                    pIDD->dsOmapTo = datasrc;
                    break;
    
                case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                    pIDD->pOmapFrom = (PCHAR)base + dd.PointerToRawData;
                    pIDD->cOmapFrom = dd.SizeOfData / sizeof(OMAP);
                    pIDD->fOmapFromMapped = TRUE;
                    pIDD->dsOmapFrom = datasrc;
                    break;

                case IMAGE_DEBUG_TYPE_EXCEPTION:
                    pIDD->dsExceptions = datasrc;
                    break;
                }
            }
    
nextdebugdir:

            ddva += sizeof(IMAGE_DEBUG_DIRECTORY);
            nDebugDirs--;
        }
    
    } __except (EXCEPTION_EXECUTE_HANDLER) {

          // We might have gotten enough information
          // to be okay.  So don't indicate error.
    }

    return rc;
}


__inline
BOOL
ProcessImageDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    return ProcessDebugInfo(pIDD, dsImage);
}


__inline
BOOL
ProcessInProcDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    return ProcessDebugInfo(pIDD, dsInProc);
}


__inline
BOOL
ProcessDbgDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    return ProcessDebugInfo(pIDD, dsDbg);
}


BOOL
FigureOutImageName(
    PIMGHLP_DEBUG_DATA pIDD
    )
/*
    We got here because we didn't get the image name passed in from the original call
    to ImgHlpFindDebugInfo AND we were unable to find the MISC data with the name
    Have to figure it out.

    A couple of options here.  First, if the DLL bit is set, try looking for Export
    table.  If found, IMAGE_EXPORT_DIRECTORY->Name is a rva pointer to the dll name.
    If it's not found, see if there's a OriginalDbgFileName.
      If so see if the format is <ext>\<filename>.dbg.
        If there's more than one backslash or no backslash, punt and label it with a .dll
        extension.
        Otherwise a splitpath will do the trick.
    If there's no DbgFilePath, see if there's a PDB name.  The same rules apply there as
    for .dbg files.  Worst case, you s/b able to get the base name and just stick on a
    If this all fails, label it as mod<base address>.
    If the DLL bit is not set, assume an exe and tag it with that extension.  The base name
    can be retrieved from DbgFilePath, PdbFilePath, or use just APP.
*/
{
    // Quick hack to get Dr. Watson going.

    CHAR szName[_MAX_FNAME];
    CHAR szExt[_MAX_FNAME];

    if (pIDD->OriginalDbgFileName[0]) {
        _splitpath(pIDD->OriginalDbgFileName, NULL, NULL, szName, NULL);
        strcpy(pIDD->OriginalImageFileName, szName);
        strcat(pIDD->OriginalImageFileName, pIDD->Characteristics & IMAGE_FILE_DLL ? ".dll" : ".exe");
    } else if (pIDD->ImageName) {
        _splitpath(pIDD->ImageName, NULL, NULL, szName, szExt);
        strcpy(pIDD->OriginalImageFileName, szName);
        if (*szExt) {
            strcat(pIDD->OriginalImageFileName, szExt);
        }
    } else if (pIDD->PdbFileName[0]) {
        _splitpath(pIDD->PdbFileName, NULL, NULL, szName, NULL);
        strcpy(pIDD->OriginalImageFileName, szName);
        strcat(pIDD->OriginalImageFileName, pIDD->Characteristics & IMAGE_FILE_DLL ? ".dll" : ".exe");
    } else {
        sprintf(pIDD->OriginalImageFileName, "MOD%p", pIDD->InProcImageBase);
    }


    return TRUE;
}


BOOL
FindExecutableImageExCallback(
    HANDLE FileHandle,
    PSTR FileName,
    PVOID CallerData
    )
{
    PIMGHLP_DEBUG_DATA pIDD;
    PIMAGE_FILE_HEADER FileHeader = NULL;
    PVOID ImageMap = NULL;
    BOOL rc;

    if (!CallerData)
        return TRUE;
    
    pIDD = (PIMGHLP_DEBUG_DATA)CallerData;
    if (!pIDD->TimeDateStamp)
        return TRUE;

    // Crack the image and let's see what we're working with
    ImageMap = MapItRO(FileHandle);

    // Check the first word.  We're either looking at a normal PE32/PE64 image, or it's
    // a ROM image (no DOS stub) or it's a random file.
    switch (*(PUSHORT)ImageMap) {
        case IMAGE_FILE_MACHINE_I386:
            // Must be an X86 ROM image (ie: ntldr)
            FileHeader = &((PIMAGE_ROM_HEADERS)ImageMap)->FileHeader;

            // Make sure
            if (!(FileHeader->SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32) &&
                pIDD->iohMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC))
            {
                FileHeader = NULL;
            }
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
        case IMAGE_FILE_MACHINE_IA64:
            // Should be an Alpha/IA64 ROM image (ie: osloader.exe)
            FileHeader = &((PIMAGE_ROM_HEADERS)ImageMap)->FileHeader;

            // Make sure
            if (!(FileHeader->SizeOfOptionalHeader == sizeof(IMAGE_ROM_OPTIONAL_HEADER) &&
                 pIDD->iohMagic == IMAGE_ROM_OPTIONAL_HDR_MAGIC))
            {
                FileHeader = NULL;
            } 
            break;

        case IMAGE_DOS_SIGNATURE:
            {
                PIMAGE_NT_HEADERS NtHeaders = ImageNtHeader(ImageMap);
                if (NtHeaders) {
                    FileHeader = &NtHeaders->FileHeader;
                }
            }
            break;

        default:
            break;
    }

    // default return is a match

    rc = TRUE;

    // compare timestamps

    if (FileHeader && FileHeader->TimeDateStamp != pIDD->TimeDateStamp)
        rc = FALSE;

    // cleanup

    if (ImageMap)
        UnmapViewOfFile(ImageMap);

    return rc;
}


PIMGHLP_DEBUG_DATA
ImgHlpFindDebugInfo(
    HANDLE  hProcess,
    HANDLE  FileHandle,
    LPSTR   FileName,
    LPSTR   SymbolPath,
    ULONG64 ImageBase,
    ULONG   dwFlags
    )
/*
   Given:
     ImageFileHandle - Map the thing.  The only time FileHandle s/b non-null
                       is if we're given an image handle.  If this is not
                       true, ignore the handle.
    !ImageFileHandle - Use the filename and search for first the image name,
                       then the .dbg file, and finally a .pdb file.

    dwFlags:           NO_PE64_IMAGES - Return failure if only image is PE64.
                                        Used to implement MapDebugInformation()

*/
{
    PIMGHLP_DEBUG_DATA pIDD;

    // No File handle and   no file name.  Bail

    if (!FileHandle && (!FileName || !*FileName)) {
        return NULL;
    }

    SetLastError(NO_ERROR);

    pIDD = MemAlloc(sizeof(IMGHLP_DEBUG_DATA));
    if (!pIDD) {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }
    ZeroMemory(pIDD, sizeof(IMGHLP_DEBUG_DATA));

    __try {

        // store off parameters

        pIDD->InProcImageBase = ImageBase;
        pIDD->hProcess = hProcess;

        if (FileName)
            lstrcpy(pIDD->ImageName, FileName);

        if (SymbolPath) {
            pIDD->SymbolPath = MemAlloc(strlen(SymbolPath) + 1);
            if (pIDD->SymbolPath) {
                strcpy(pIDD->SymbolPath, SymbolPath);
            }
        }
        
        // if we have a base pointer into process memory.  See what we can get here.
        if (pIDD->InProcImageBase) {
            pIDD->fInProcHeader = ProcessInProcDebugInfo(pIDD);
        } 
        
        // find disk-based image

        if (FileHandle) {
            // if passed a handle, save it
            if (!DuplicateHandle(
                                 GetCurrentProcess(),
                                 FileHandle,
                                 GetCurrentProcess(),
                                 &pIDD->ImageFileHandle,
                                 GENERIC_READ,
                                 FALSE,
                                 DUPLICATE_SAME_ACCESS
                                ))
            {
                return NULL;
            }
            if (FileName) {
                strcpy(pIDD->ImageFilePath, FileName);
            }

        } else if (FileName && *FileName && !pIDD->fInProcHeader)
        {
            // otherwise use the file name to open the disk image 
            // only if we didn't have access to in-proc headers
            pIDD->ImageFileHandle = FindExecutableImageEx(FileName, 
                                                          SymbolPath, 
                                                          pIDD->ImageFilePath, 
                                                          FindExecutableImageExCallback, 
                                                          pIDD);
        }

        // if we have a file handle.  See what we can get here.
        if (pIDD->ImageFileHandle) { 
            if (!pIDD->DbgFileHandle && !*pIDD->PdbFileName) {
                ProcessImageDebugInfo(pIDD);
            }
        }

        // search for pdb, if indicated or if we have found no image info, so far
        if (!pIDD->Characteristics || (pIDD->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)) {
            pIDD->DbgFileHandle = fnFindDebugInfoFileEx(
                                        (*pIDD->OriginalDbgFileName) ? pIDD->OriginalDbgFileName : pIDD->ImageName,
                                        pIDD->SymbolPath,
                                        pIDD->DbgFilePath,
                                        FindDebugInfoFileExCallback,
                                        pIDD,
                                        fdifRECURSIVE
                                        );
        }

        // if we have a .dbg file.  See what we can get from it.
        if (pIDD->DbgFileHandle) {
            ProcessDbgDebugInfo(pIDD);
        }

        // check one more time to see if information we have aquired
        // indicates we need the image.
        if (FileName && *FileName && pIDD->fNeedImage) {
            pIDD->ImageFileHandle = FindExecutableImageEx(FileName, 
                                                          SymbolPath, 
                                                          pIDD->ImageFilePath, 
                                                          FindExecutableImageExCallback, 
                                                          pIDD);
            if (pIDD->ImageFileHandle) { 
                    ProcessImageDebugInfo(pIDD);
            }
        }

        // if there's a pdb.  Pull what we can from there.
        if (*pIDD->PdbFileName) {
            ProcessPdbDebugInfo(pIDD);
        
        // otherwise, if old codeview, pull from there
        } else if (pIDD->pMappedCv) {
            ProcessOldStyleCodeView(pIDD);
        
        // otherwise if we couldn't read from the image info, look for PDB anyway
        } else if (!pIDD->ImageFileHandle && !pIDD->DbgFileHandle) {
            if (FakePdbName(pIDD)) {
                ProcessPdbDebugInfo(pIDD);
            }
        } 

        if (!pIDD->OriginalImageFileName[0]) {
            FigureOutImageName(pIDD);
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (pIDD) {
            ImgHlpReleaseDebugInfo(pIDD, IMGHLP_FREE_ALL);
            pIDD = NULL;
        }
    }

    return pIDD;
}


void
ImgHlpReleaseDebugInfo(
    PIMGHLP_DEBUG_DATA pIDD,
    DWORD              dwFlags
    )
{
    if (!pIDD)
        return;

    if (pIDD->ImageMap) {
        UnmapViewOfFile(pIDD->ImageMap);
    }

    if (pIDD->ImageFileHandle) {
        CloseHandle(pIDD->ImageFileHandle);
    }

    if (pIDD->DbgFileMap) {
        UnmapViewOfFile(pIDD->DbgFileMap);
    }

    if (pIDD->DbgFileHandle) {
        CloseHandle(pIDD->DbgFileHandle);
    }

    if ((dwFlags & IMGHLP_FREE_FPO) &&
        pIDD->pFpo &&
        !pIDD->fFpoMapped
       )
    {
        MemFree(pIDD->pFpo);
    }

    if ((dwFlags & IMGHLP_FREE_PDATA) &&
        pIDD->pImageFunction &&
        !pIDD->fImageFunctionMapped
       )
    {
        MemFree(pIDD->pImageFunction);
    }

    if ((dwFlags & IMGHLP_FREE_PDATA) &&
        pIDD->pMappedCoff &&
        !pIDD->fCoffMapped
       )
    {
        MemFree(pIDD->pMappedCoff);
    }

    if ((dwFlags & IMGHLP_FREE_PDATA) &&
        pIDD->pMappedCv &&
        !pIDD->fCvMapped
       )
    {
        MemFree(pIDD->pMappedCv);
    }

    if ((dwFlags & IMGHLP_FREE_OMAPT) &&
        pIDD->pOmapTo &&
        !pIDD->fOmapToMapped
       )
    {
        MemFree(pIDD->pOmapTo);
    }

    if ((dwFlags & IMGHLP_FREE_OMAPF) &&
        pIDD->pOmapFrom &&
        !pIDD->fOmapFromMapped
       )
    {
        MemFree(pIDD->pOmapFrom);
    }

    if ((dwFlags & IMGHLP_FREE_OSECT) &&
        pIDD->pOriginalSections
       )
    {
        MemFree(pIDD->pOriginalSections);
    }

    if ((dwFlags & IMGHLP_FREE_CSECT) &&
        pIDD->pCurrentSections &&
        !pIDD->fCurrentSectionsMapped
       )
    {
        MemFree(pIDD->pCurrentSections);
    }

    if (dwFlags & IMGHLP_FREE_PDB) {
        if (pIDD->pGsi) {
            GSIClose(pIDD->pGsi);
        }
        if (pIDD->pDbi) {
            DBIClose(pIDD->pDbi);
        }
        if (pIDD->pPdb) {
            PDBClose(pIDD->pPdb);
        }
    }

    if (pIDD->SymbolPath) {
        MemFree(pIDD->SymbolPath);
    }

    MemFree(pIDD);

    return;
}



#ifdef MAP_DEBUG_TEST

void
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    CHAR szSymPath[4096];
    PIMGHLP_DEBUG_DATA pDebugInfo;

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test1");
    pDebugInfo = ImgHlpFindDebugInfo(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test1\\ntoskrnl.exe", szSymPath, 0x1000000, 0);
    ImgHlpReleaseDebugInfo(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test2");
    pDebugInfo = ImgHlpFindDebugInfo(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test2\\ntoskrnl.exe", szSymPath, 0x1000000, 0);
    ImgHlpReleaseDebugInfo(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test3");
    pDebugInfo = ImgHlpFindDebugInfo(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test3\\ntoskrnl.exe", szSymPath, 0x1000000, 0);
    ImgHlpReleaseDebugInfo(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test4");
    pDebugInfo = ImgHlpFindDebugInfo(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test4\\ntoskrnl.exe", szSymPath, 0x1000000, 0);
    ImgHlpReleaseDebugInfo(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test5");
    pDebugInfo = ImgHlpFindDebugInfo(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test5\\ntdll.dll", szSymPath, 0x1000000, 0);
    ImgHlpReleaseDebugInfo(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test6");
    pDebugInfo = ImgHlpFindDebugInfo(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test6\\ntdll.dll", szSymPath, 0x1000000, 0);
    ImgHlpReleaseDebugInfo(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test7");
    pDebugInfo = ImgHlpFindDebugInfo(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test7\\osloader.exe", szSymPath, 0x1000000, 0);
    ImgHlpReleaseDebugInfo(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test8");
    pDebugInfo = ImgHlpFindDebugInfo(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test8\\osloader.exe", szSymPath, 0x1000000, 0);
    ImgHlpReleaseDebugInfo(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test9");
    pDebugInfo = ImgHlpFindDebugInfo(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test9\\msvcrt.dll", szSymPath, 0x1000000, 0);
    ImgHlpReleaseDebugInfo(pDebugInfo, IMGHLP_FREE_ALL);

    strcpy(szSymPath, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test10");
    pDebugInfo = ImgHlpFindDebugInfo(NULL, NULL, "h:\\nt\\private\\sdktools\\imagehlp\\test\\test10\\msvcrt.dll", szSymPath, 0x1000000, 0);
    ImgHlpReleaseDebugInfo(pDebugInfo, IMGHLP_FREE_ALL);
}
#endif
