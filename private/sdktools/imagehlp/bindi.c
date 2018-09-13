/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    bindi.c

Abstract:
    Implementation for the BindImage API

Author:

Revision History:

--*/

#include <private.h>

typedef struct _BOUND_FORWARDER_REFS {
    struct _BOUND_FORWARDER_REFS *Next;
    ULONG TimeDateStamp;
    LPSTR ModuleName;
} BOUND_FORWARDER_REFS, *PBOUND_FORWARDER_REFS;

typedef struct _IMPORT_DESCRIPTOR {
    struct _IMPORT_DESCRIPTOR *Next;
    LPSTR ModuleName;
    ULONG TimeDateStamp;
    USHORT NumberOfModuleForwarderRefs;
    PBOUND_FORWARDER_REFS Forwarders;
} IMPORT_DESCRIPTOR, *PIMPORT_DESCRIPTOR;

typedef struct _BINDP_PARAMETERS {
    DWORD Flags;
    BOOLEAN fNoUpdate;
    BOOLEAN fNewImports;
    LPSTR ImageName;
    LPSTR DllPath;
    LPSTR SymbolPath;
    PIMAGEHLP_STATUS_ROUTINE StatusRoutine;
} BINDP_PARAMETERS, *PBINDP_PARAMETERS;

BOOL
BindpLookupThunk(
    PBINDP_PARAMETERS Parms,
    PIMAGE_THUNK_DATA ThunkName,
    PLOADED_IMAGE Image,
    PIMAGE_THUNK_DATA SnappedThunks,
    PIMAGE_THUNK_DATA FunctionAddress,
    PLOADED_IMAGE Dll,
    PIMAGE_EXPORT_DIRECTORY Exports,
    PIMPORT_DESCRIPTOR NewImport,
    LPSTR DllPath,
    PULONG *ForwarderChain
    );

PVOID
BindpRvaToVa(
    PBINDP_PARAMETERS Parms,
    ULONG Rva,
    PLOADED_IMAGE Image
    );

VOID
BindpWalkAndProcessImports(
    PBINDP_PARAMETERS Parms,
    PLOADED_IMAGE Image,
    LPSTR DllPath,
    PBOOL ImageModified
    );

BOOL
BindImage(
    IN LPSTR ImageName,
    IN LPSTR DllPath,
    IN LPSTR SymbolPath
    )
{
    return BindImageEx( 0,
                        ImageName,
                        DllPath,
                        SymbolPath,
                        NULL
                      );
}

UCHAR BindpCapturedModuleNames[4096];
LPSTR BindpEndCapturedModuleNames;

LPSTR
BindpCaptureImportModuleName(
    LPSTR DllName
    )
{
    LPSTR s;

    s = (LPSTR) BindpCapturedModuleNames;
    if (BindpEndCapturedModuleNames == NULL) {
        *s = '\0';
        BindpEndCapturedModuleNames = s;
        }

    while (*s) {
        if (!_stricmp(s, DllName)) {
            return s;
            }

        s += strlen(s)+1;
        }

    strcpy(s, DllName);
    BindpEndCapturedModuleNames = s + strlen(s) + 1;
    *BindpEndCapturedModuleNames = '\0';
    return s;
}

PIMPORT_DESCRIPTOR
BindpAddImportDescriptor(
    PBINDP_PARAMETERS Parms,
    PIMPORT_DESCRIPTOR *NewImportDescriptor,
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor,
    LPSTR ModuleName,
    PLOADED_IMAGE Dll
    )
{
    PIMPORT_DESCRIPTOR p, *pp;

    if (!Parms->fNewImports) {
        return NULL;
        }

    pp = NewImportDescriptor;
    while (p = *pp) {
        if (!_stricmp( p->ModuleName, ModuleName )) {
            return p;
            }

        pp = &p->Next;
        }

    p = (PIMPORT_DESCRIPTOR) MemAlloc( sizeof( *p ) );
    if (p != NULL) {
        if (Dll != NULL) {
            p->TimeDateStamp = ((PIMAGE_NT_HEADERS32)Dll->FileHeader)->FileHeader.TimeDateStamp;
            }
        p->ModuleName = BindpCaptureImportModuleName( ModuleName );
        *pp = p;
        }
    else
    if (Parms->StatusRoutine != NULL) {
        (Parms->StatusRoutine)( BindOutOfMemory, NULL, NULL, 0, sizeof( *p ) );
        }

    return p;
}


PUCHAR
BindpAddForwarderReference(
    PBINDP_PARAMETERS Parms,
    LPSTR ImageName,
    LPSTR ImportName,
    PIMPORT_DESCRIPTOR NewImportDescriptor,
    LPSTR DllPath,
    PUCHAR ForwarderString,
    PBOOL BoundForwarder
    )
{
    CHAR DllName[ MAX_PATH ];
    PUCHAR s;
    PLOADED_IMAGE Dll;
    ULONG cb;
    USHORT OrdinalNumber;
    USHORT HintIndex;
    ULONG ExportSize;
    PIMAGE_EXPORT_DIRECTORY Exports;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG FunctionTableBase;
    LPSTR NameTableName;
    ULONG64 ForwardedAddress;
    PBOUND_FORWARDER_REFS p, *pp;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;
    PIMAGE_FILE_HEADER FileHeader;

    *BoundForwarder = FALSE;
BindAnotherForwarder:
    s = ForwarderString;
    while (*s && *s != '.') {
        s++;
        }
    if (*s != '.') {
        return ForwarderString;
        }
    cb = (ULONG) (s - ForwarderString);
    if (cb >= MAX_PATH) {
        return ForwarderString;
        }
    strncpy( DllName, (LPSTR) ForwarderString, cb );
    DllName[ cb ] = '\0';
    strcat( DllName, ".DLL" );

    Dll = ImageLoad( DllName, DllPath );
    if (!Dll) {
        return ForwarderString;
        }
    s += 1;

    Exports = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData(
                                         (PVOID)Dll->MappedAddress,
                                         FALSE,
                                         IMAGE_DIRECTORY_ENTRY_EXPORT,
                                         &ExportSize
                                         );
    if (!Exports) {
        return ForwarderString;
    }

    FileHeader = &((PIMAGE_NT_HEADERS32)Dll->FileHeader)->FileHeader;
    OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)Dll->FileHeader,
                                 &OptionalHeader32,
                                 &OptionalHeader64);

    if ( *s == '#' ) {
        // Binding for ordinal forwarders

        OrdinalNumber = (atoi((PCHAR)s + 1)) - (USHORT)Exports->Base;

        if (OrdinalNumber >= Exports->NumberOfFunctions) {
            return ForwarderString;
        }
    } else {
        // Regular binding for named forwarders

        OrdinalNumber = 0xFFFF;
    }

    NameTableBase = (PULONG) BindpRvaToVa( Parms, Exports->AddressOfNames, Dll );
    NameOrdinalTableBase = (PUSHORT) BindpRvaToVa( Parms, Exports->AddressOfNameOrdinals, Dll );
    FunctionTableBase = (PULONG) BindpRvaToVa( Parms, Exports->AddressOfFunctions, Dll );

    if (OrdinalNumber == 0xFFFF) {
        for ( HintIndex = 0; HintIndex < Exports->NumberOfNames; HintIndex++){
            NameTableName = (LPSTR) BindpRvaToVa( Parms, NameTableBase[HintIndex], Dll );
            if ( NameTableName ) {
                OrdinalNumber = NameOrdinalTableBase[HintIndex];

                if (!strcmp((PCHAR)s, NameTableName)) {
                    break;
                }
            }
        }

        if (HintIndex >= Exports->NumberOfNames) {
            return ForwarderString;
        }
    }

    do {
       ForwardedAddress = FunctionTableBase[OrdinalNumber] +
           OPTIONALHEADER(ImageBase);

       pp = &NewImportDescriptor->Forwarders;
       while (p = *pp) {
           if (!_stricmp(DllName, p->ModuleName)) {
               break;
           }

           pp = &p->Next;
       }

       if (p == NULL) {
           p = (PBOUND_FORWARDER_REFS) MemAlloc( sizeof( *p ) );
           if (p == NULL) {
               if (Parms->StatusRoutine != NULL) {
                   (Parms->StatusRoutine)( BindOutOfMemory, NULL, NULL, 0, sizeof( *p ) );
               }

               break;
           }

           p->ModuleName = BindpCaptureImportModuleName( DllName );
           *pp = p;
           NewImportDescriptor->NumberOfModuleForwarderRefs += 1;
       }

       p->TimeDateStamp = FileHeader->TimeDateStamp;
       if (Parms->StatusRoutine != NULL)
       {
           (Parms->StatusRoutine)( BindForwarder,
                                   ImageName,
                                   ImportName,
                                   (ULONG_PTR)ForwardedAddress,    //BUGBUG
                                   (ULONG_PTR)ForwarderString
                                 );
       }

       Exports = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData(
                                            (PVOID)Dll->MappedAddress,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_EXPORT,
                                            &ExportSize
                                            );

       Exports = (PIMAGE_EXPORT_DIRECTORY) ((ULONG_PTR)Exports -
                     (ULONG_PTR) Dll->MappedAddress +
                     OPTIONALHEADER(ImageBase));

       if ((ForwardedAddress >= (ULONG_PTR)Exports) &&
           (ForwardedAddress <= ((ULONG_PTR)Exports + ExportSize)))
       {
           ForwarderString = BindpRvaToVa(Parms,
                                          FunctionTableBase[OrdinalNumber],
                                          Dll);
           goto BindAnotherForwarder;
       } else {
           ForwarderString = (PUCHAR)ForwardedAddress;
           *BoundForwarder = TRUE;
           break;
       }
    }
    while (0);

    return ForwarderString;
}

PIMAGE_BOUND_IMPORT_DESCRIPTOR
BindpCreateNewImportSection(
    PBINDP_PARAMETERS Parms,
    PIMPORT_DESCRIPTOR *NewImportDescriptor,
    PULONG NewImportsSize
    )
{
    ULONG cbString, cbStruct;
    PIMPORT_DESCRIPTOR p, *pp;
    PBOUND_FORWARDER_REFS p1, *pp1;
    LPSTR CapturedStrings;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR NewImports, NewImport;
    PIMAGE_BOUND_FORWARDER_REF NewForwarder;


    *NewImportsSize = 0;
    cbString = 0;
    cbStruct = 0;
    pp = NewImportDescriptor;
    while (p = *pp) {
        cbStruct += sizeof( IMAGE_BOUND_IMPORT_DESCRIPTOR );
        pp1 = &p->Forwarders;
        while (p1 = *pp1) {
            cbStruct += sizeof( IMAGE_BOUND_FORWARDER_REF );
            pp1 = &p1->Next;
            }

        pp = &p->Next;
        }
    if (cbStruct == 0) {
        BindpEndCapturedModuleNames = NULL;
        return NULL;
        }
    cbStruct += sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR);    // Room for terminating zero entry
    cbString = (ULONG) (BindpEndCapturedModuleNames - (LPSTR) BindpCapturedModuleNames);
    BindpEndCapturedModuleNames = NULL;
    *NewImportsSize = cbStruct+((cbString + sizeof(ULONG) - 1) & ~(sizeof(ULONG)-1));
    NewImports = (PIMAGE_BOUND_IMPORT_DESCRIPTOR) MemAlloc( *NewImportsSize );
    if (NewImports != NULL) {
        CapturedStrings = (LPSTR)NewImports + cbStruct;
        memcpy(CapturedStrings, BindpCapturedModuleNames, cbString);

        NewImport = NewImports;
        pp = NewImportDescriptor;
        while (p = *pp) {
            NewImport->TimeDateStamp = p->TimeDateStamp;
            NewImport->OffsetModuleName = (USHORT)(cbStruct + (p->ModuleName - (LPSTR) BindpCapturedModuleNames));
            NewImport->NumberOfModuleForwarderRefs = p->NumberOfModuleForwarderRefs;

            NewForwarder = (PIMAGE_BOUND_FORWARDER_REF)(NewImport+1);
            pp1 = &p->Forwarders;
            while (p1 = *pp1) {
                NewForwarder->TimeDateStamp = p1->TimeDateStamp;
                NewForwarder->OffsetModuleName = (USHORT)(cbStruct + (p1->ModuleName - (LPSTR) BindpCapturedModuleNames));
                NewForwarder += 1;
                pp1 = &p1->Next;
                }
            NewImport = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)NewForwarder;

            pp = &p->Next;
            }
        }
    else
    if (Parms->StatusRoutine != NULL) {
        (Parms->StatusRoutine)( BindOutOfMemory, NULL, NULL, 0, *NewImportsSize );
        }

    pp = NewImportDescriptor;
    while ((p = *pp) != NULL) {
        *pp = p->Next;
        pp1 = &p->Forwarders;
        while ((p1 = *pp1) != NULL) {
            *pp1 = p1->Next;
            MemFree(p1);
            }

        MemFree(p);
        }

    return NewImports;
}

BOOL
BindpExpandImageFileHeaders(
    PBINDP_PARAMETERS Parms,
    PLOADED_IMAGE Dll,
    ULONG NewSizeOfHeaders
    )
{
    HANDLE hMappedFile;
    LPVOID lpMappedAddress;
    DWORD dwFileSizeLow, dwOldFileSize;
    DWORD dwFileSizeHigh;
    DWORD dwSizeDelta;
    PIMAGE_SECTION_HEADER Section;
    ULONG SectionNumber;
    PIMAGE_DEBUG_DIRECTORY DebugDirectories;
    ULONG DebugDirectoriesSize;
    ULONG OldSizeOfHeaders;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;
    PIMAGE_FILE_HEADER FileHeader;

    dwFileSizeLow = GetFileSize( Dll->hFile, &dwFileSizeHigh );
    if (dwFileSizeLow == 0xFFFFFFFF || dwFileSizeHigh != 0) {
        return FALSE;
    }

    FileHeader = &((PIMAGE_NT_HEADERS32)Dll->FileHeader)->FileHeader;
    OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)Dll->FileHeader,
                                 &OptionalHeader32,
                                 &OptionalHeader64);

    OldSizeOfHeaders = OPTIONALHEADER(SizeOfHeaders);
    dwOldFileSize = dwFileSizeLow;
    dwSizeDelta = NewSizeOfHeaders - OldSizeOfHeaders;
    dwFileSizeLow += dwSizeDelta;

    hMappedFile = CreateFileMapping(Dll->hFile,
                                    NULL,
                                    PAGE_READWRITE,
                                    dwFileSizeHigh,
                                    dwFileSizeLow,
                                    NULL
                                   );
    if (!hMappedFile) {
        return FALSE;
    }


    FlushViewOfFile(Dll->MappedAddress, Dll->SizeOfImage);
    UnmapViewOfFile(Dll->MappedAddress);
    lpMappedAddress = MapViewOfFileEx(hMappedFile,
                                      FILE_MAP_WRITE,
                                      0,
                                      0,
                                      0,
                                      Dll->MappedAddress
                                     );
    if (!lpMappedAddress) {
        lpMappedAddress = MapViewOfFileEx(hMappedFile,
                                          FILE_MAP_WRITE,
                                          0,
                                          0,
                                          0,
                                          0
                                         );
    }

    CloseHandle(hMappedFile);

    if (lpMappedAddress != Dll->MappedAddress) {
        Dll->MappedAddress = (PUCHAR) lpMappedAddress;
        CalculateImagePtrs(Dll);
        FileHeader = &((PIMAGE_NT_HEADERS32)Dll->FileHeader)->FileHeader;
        OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)Dll->FileHeader,
                                     &OptionalHeader32,
                                     &OptionalHeader64);
    }

    if (Dll->SizeOfImage != dwFileSizeLow) {
        Dll->SizeOfImage = dwFileSizeLow;
    }

    DebugDirectories = (PIMAGE_DEBUG_DIRECTORY)ImageDirectoryEntryToData(
                                            (PVOID)Dll->MappedAddress,
                                            FALSE,
                                            IMAGE_DIRECTORY_ENTRY_DEBUG,
                                            &DebugDirectoriesSize
                                            );

    if (DebugDirectoryIsUseful(DebugDirectories, DebugDirectoriesSize)) {
        while (DebugDirectoriesSize != 0) {
            DebugDirectories->PointerToRawData += dwSizeDelta;
            DebugDirectories += 1;
            DebugDirectoriesSize -= sizeof( *DebugDirectories );
        }
    }

    OPTIONALHEADER_LV(SizeOfHeaders) = NewSizeOfHeaders;
    if (FileHeader->PointerToSymbolTable != 0) {
        // Only adjust if it's already set

        FileHeader->PointerToSymbolTable += dwSizeDelta;
    }
    Section = Dll->Sections;
    for (SectionNumber=0; SectionNumber<FileHeader->NumberOfSections; SectionNumber++) {
        if (Section->PointerToRawData != 0) {
            Section->PointerToRawData += dwSizeDelta;
        }
        if (Section->PointerToRelocations != 0) {
            Section->PointerToRelocations += dwSizeDelta;
        }
        if (Section->PointerToLinenumbers != 0) {
            Section->PointerToLinenumbers += dwSizeDelta;
        }
        Section += 1;
    }

    memmove((LPSTR)lpMappedAddress + NewSizeOfHeaders,
            (LPSTR)lpMappedAddress + OldSizeOfHeaders,
            dwOldFileSize - OldSizeOfHeaders
           );

    if (Parms->StatusRoutine != NULL) {
        (Parms->StatusRoutine)( BindExpandFileHeaders, Dll->ModuleName, NULL, 0, NewSizeOfHeaders );
    }

    return TRUE;
}

BOOL
BindImageEx(
    IN DWORD Flags,
    IN LPSTR ImageName,
    IN LPSTR DllPath,
    IN LPSTR SymbolPath,
    IN PIMAGEHLP_STATUS_ROUTINE StatusRoutine
    )
{
    BINDP_PARAMETERS Parms;
    LOADED_IMAGE LoadedImageBuffer;
    PLOADED_IMAGE LoadedImage;
    ULONG CheckSum;
    ULONG HeaderSum;
    BOOL fSymbolsAlreadySplit, fRC;
    SYSTEMTIME SystemTime;
    FILETIME LastWriteTime;
    BOOL ImageModified;
    DWORD OldChecksum;
    CHAR DebugFileName[ MAX_PATH ];
    CHAR DebugFilePath[ MAX_PATH ];
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;
    PIMAGE_FILE_HEADER FileHeader;

    Parms.Flags         = Flags;
    if (Flags & BIND_NO_BOUND_IMPORTS) {
        Parms.fNewImports = FALSE;
    } else {
        Parms.fNewImports = TRUE;
    }
    if (Flags & BIND_NO_UPDATE) {
        Parms.fNoUpdate = TRUE;
    } else {
        Parms.fNoUpdate = FALSE;
    }
    Parms.ImageName     = ImageName;
    Parms.DllPath       = DllPath;
    Parms.SymbolPath    = SymbolPath;
    Parms.StatusRoutine = StatusRoutine;

    fRC = FALSE;            // Assume we'll fail to bind

    __try {

        // Map and load the image

        LoadedImage = &LoadedImageBuffer;
        memset( LoadedImage, 0, sizeof( *LoadedImage ) );
        if (MapAndLoad( ImageName, DllPath, LoadedImage, TRUE, Parms.fNoUpdate )) {
            LoadedImage->ModuleName = ImageName;

            //
            // Now locate and walk through and process the images imports
            //
            if (LoadedImage->FileHeader != NULL &&
                ((Flags & BIND_ALL_IMAGES) || (!LoadedImage->fSystemImage)) ) {

                FileHeader = &((PIMAGE_NT_HEADERS32)LoadedImage->FileHeader)->FileHeader;
                OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)LoadedImage->FileHeader,
                                             &OptionalHeader32,
                                             &OptionalHeader64);

                if (OPTIONALHEADER(DllCharacteristics) & IMAGE_DLLCHARACTERISTICS_NO_BIND) {
                    goto NoBind;
                }

                {
                    DWORD dwCertificateSize;
                    PVOID pCertificates = ImageDirectoryEntryToData(
                                                        LoadedImage->MappedAddress,
                                                        FALSE,
                                                        IMAGE_DIRECTORY_ENTRY_SECURITY,
                                                        &dwCertificateSize
                                                        );

                    if (pCertificates || dwCertificateSize) {
                        goto NoBind;
                    }
                }


                BindpWalkAndProcessImports(
                                &Parms,
                                LoadedImage,
                                DllPath,
                                &ImageModified
                                );

                //
                // If the file is being updated, then recompute the checksum.
                // and update image and possibly stripped symbol file.
                //

                if (!Parms.fNoUpdate && ImageModified &&
                    (LoadedImage->hFile != INVALID_HANDLE_VALUE)) {
                    if ( (FileHeader->Characteristics & IMAGE_FILE_DEBUG_STRIPPED) &&
                         (SymbolPath != NULL) ) {
                        PIMAGE_DEBUG_DIRECTORY DebugDirectories;
                        ULONG DebugDirectoriesSize;
                        PIMAGE_DEBUG_MISC MiscDebug;

                        fSymbolsAlreadySplit = TRUE;
                        strcpy( DebugFileName, ImageName );
                        DebugDirectories = (PIMAGE_DEBUG_DIRECTORY)ImageDirectoryEntryToData(
                                                                LoadedImage->MappedAddress,
                                                                FALSE,
                                                                IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                                &DebugDirectoriesSize
                                                                );
                        if (DebugDirectoryIsUseful(DebugDirectories, DebugDirectoriesSize)) {
                            while (DebugDirectoriesSize != 0) {
                                if (DebugDirectories->Type == IMAGE_DEBUG_TYPE_MISC) {
                                    MiscDebug = (PIMAGE_DEBUG_MISC)
                                        ((PCHAR)LoadedImage->MappedAddress +
                                         DebugDirectories->PointerToRawData
                                        );
                                    strcpy( DebugFileName, (PCHAR) MiscDebug->Data );
                                    break;
                                } else {
                                    DebugDirectories += 1;
                                    DebugDirectoriesSize -= sizeof( *DebugDirectories );
                                }
                            }
                        }
                    } else {
                        fSymbolsAlreadySplit = FALSE;
                    }

                    OldChecksum = OPTIONALHEADER(CheckSum);
                    CheckSumMappedFile(
                                (PVOID)LoadedImage->MappedAddress,
                                GetFileSize(LoadedImage->hFile, NULL),
                                &HeaderSum,
                                &CheckSum
                                );

                    OPTIONALHEADER_LV(CheckSum) = CheckSum;
                    FlushViewOfFile(LoadedImage->MappedAddress, LoadedImage->SizeOfImage);

                    if (fSymbolsAlreadySplit) {
                        if ( UpdateDebugInfoFileEx(ImageName,
                                                   SymbolPath,
                                                   DebugFilePath,
                                                   (PIMAGE_NT_HEADERS32)(LoadedImage->FileHeader),
                                                   OldChecksum)) {
                            if (GetLastError() == ERROR_INVALID_DATA) {
                                if (Parms.StatusRoutine != NULL) {
                                    (Parms.StatusRoutine)( BindMismatchedSymbols,
                                                           LoadedImage->ModuleName,
                                                           NULL,
                                                           0,
                                                           (ULONG_PTR)DebugFileName
                                                         );
                                }
                            }
                        } else {
                            if (Parms.StatusRoutine != NULL) {
                                (Parms.StatusRoutine)( BindSymbolsNotUpdated,
                                                       LoadedImage->ModuleName,
                                                       NULL,
                                                       0,
                                                       (ULONG_PTR)DebugFileName
                                                     );
                            }
                        }
                    }

                    GetSystemTime(&SystemTime);
                    if (SystemTimeToFileTime( &SystemTime, &LastWriteTime )) {
                        SetFileTime( LoadedImage->hFile, NULL, NULL, &LastWriteTime );
                    }
                }
            }

NoBind:
            UnmapViewOfFile( LoadedImage->MappedAddress );
            if (LoadedImage->hFile != INVALID_HANDLE_VALUE) {
                CloseHandle( LoadedImage->hFile );
            }

            fRC = TRUE;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // Nothing to do...
    }

    if (!(Flags & BIND_CACHE_IMPORT_DLLS)) {
        UnloadAllImages();
    }

    return (fRC);
}


BOOL
BindpLookupThunk(
    PBINDP_PARAMETERS Parms,
    PIMAGE_THUNK_DATA ThunkName,
    PLOADED_IMAGE Image,
    PIMAGE_THUNK_DATA SnappedThunks,
    PIMAGE_THUNK_DATA FunctionAddress,
    PLOADED_IMAGE Dll,
    PIMAGE_EXPORT_DIRECTORY Exports,
    PIMPORT_DESCRIPTOR NewImport,
    LPSTR DllPath,
    PULONG *ForwarderChain
    )
{
    BOOL Ordinal;
    USHORT OrdinalNumber;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG FunctionTableBase;
    PIMAGE_IMPORT_BY_NAME ImportName;
    USHORT HintIndex;
    LPSTR NameTableName;
    ULONG64 ExportsBase;
    ULONG ExportSize;
    UCHAR NameBuffer[ 32 ];
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;
    PIMAGE_OPTIONAL_HEADER32 DllOptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 DllOptionalHeader64 = NULL;

    NameTableBase = (PULONG) BindpRvaToVa( Parms, Exports->AddressOfNames, Dll );
    NameOrdinalTableBase = (PUSHORT) BindpRvaToVa( Parms, Exports->AddressOfNameOrdinals, Dll );
    FunctionTableBase = (PULONG) BindpRvaToVa( Parms, Exports->AddressOfFunctions, Dll );

    OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)Image->FileHeader,
                                 &OptionalHeader32,
                                 &OptionalHeader64);

    OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)Dll->FileHeader,
                                 &DllOptionalHeader32,
                                 &DllOptionalHeader64);
    //
    // Determine if snap is by name, or by ordinal
    //

    Ordinal = (BOOL)IMAGE_SNAP_BY_ORDINAL(ThunkName->u1.Ordinal);

    if (Ordinal) {
        UCHAR szOrdinal[8];
        OrdinalNumber = (USHORT)(IMAGE_ORDINAL(ThunkName->u1.Ordinal) - Exports->Base);
        if ( (ULONG)OrdinalNumber >= Exports->NumberOfFunctions ) {
            return FALSE;
            }
        ImportName = (PIMAGE_IMPORT_BY_NAME)NameBuffer;
        // Can't use sprintf w/o dragging in more CRT support than we want...  Must run on Win95.
        strcpy((PCHAR) ImportName->Name, "Ordinal");
        strcat((PCHAR) ImportName->Name, _ultoa((ULONG) OrdinalNumber, (LPSTR) szOrdinal, 16));
        }
    else {
        ImportName = (PIMAGE_IMPORT_BY_NAME)BindpRvaToVa(
                                                Parms,
                                                (ULONG)(ULONG_PTR)(ThunkName->u1.AddressOfData),
                                                Image
                                                );
        if (!ImportName) {
            return FALSE;
            }

        //
        // now check to see if the hint index is in range. If it
        // is, then check to see if it matches the function at
        // the hint. If all of this is true, then we can snap
        // by hint. Otherwise need to scan the name ordinal table
        //

        OrdinalNumber = (USHORT)(Exports->NumberOfFunctions+1);
        HintIndex = ImportName->Hint;
        if ((ULONG)HintIndex < Exports->NumberOfNames ) {
            NameTableName = (LPSTR) BindpRvaToVa( Parms, NameTableBase[HintIndex], Dll );
            if ( NameTableName ) {
                if ( !strcmp((PCHAR)ImportName->Name, NameTableName) ) {
                    OrdinalNumber = NameOrdinalTableBase[HintIndex];
                    }
                }
            }

        if ((ULONG)OrdinalNumber >= Exports->NumberOfFunctions) {
            for (HintIndex = 0; HintIndex < Exports->NumberOfNames; HintIndex++) {
                NameTableName = (LPSTR) BindpRvaToVa( Parms, NameTableBase[HintIndex], Dll );
                if (NameTableName) {
                    if (!strcmp( (PCHAR)ImportName->Name, NameTableName )) {
                        OrdinalNumber = NameOrdinalTableBase[HintIndex];
                        break;
                        }
                    }
                }

            if ((ULONG)OrdinalNumber >= Exports->NumberOfFunctions) {
                return FALSE;
                }
            }
        }

    (PULONG)(FunctionAddress->u1.Function) = (PULONG)(FunctionTableBase[OrdinalNumber] +
                                            (DllOptionalHeader32 ?
                                               DllOptionalHeader32->ImageBase :
                                               DllOptionalHeader64->ImageBase)
                                           );
    ExportsBase = (ULONG64)ImageDirectoryEntryToData(
                          (PVOID)Dll->MappedAddress,
                          TRUE,
                          IMAGE_DIRECTORY_ENTRY_EXPORT,
                          &ExportSize
                          ) - (ULONG_PTR)Dll->MappedAddress;
    ExportsBase += (DllOptionalHeader32 ?
                      DllOptionalHeader32->ImageBase :
                      DllOptionalHeader64->ImageBase);

    if ((ULONG64)FunctionAddress->u1.Function > (ULONG64)ExportsBase &&
        (ULONG64)FunctionAddress->u1.Function < ((ULONG64)ExportsBase + ExportSize)
       ) {
        BOOL BoundForwarder;

        BoundForwarder = FALSE;
        if (NewImport != NULL) {
            (PUCHAR)(FunctionAddress->u1.ForwarderString) = BindpAddForwarderReference(Parms,
                                           Image->ModuleName,
                                           (LPSTR) ImportName->Name,
                                           NewImport,
                                           DllPath,
                                           (PUCHAR) BindpRvaToVa( Parms, FunctionTableBase[OrdinalNumber], Dll ),
                                           &BoundForwarder
                                          );
            }

        if (!BoundForwarder) {
            **ForwarderChain = (ULONG) (FunctionAddress - SnappedThunks);
            *ForwarderChain = (ULONG *)&FunctionAddress->u1.Ordinal;

            if (Parms->StatusRoutine != NULL) {
                (Parms->StatusRoutine)( BindForwarderNOT,
                                        Image->ModuleName,
                                        Dll->ModuleName,
                                        (ULONG_PTR)FunctionAddress->u1.Function,
                                        (ULONG_PTR)(ImportName->Name)
                                      );
                }
            }
        }
    else {
        if (Parms->StatusRoutine != NULL) {
            (Parms->StatusRoutine)( BindImportProcedure,
                                    Image->ModuleName,
                                    Dll->ModuleName,
                                    (ULONG_PTR)FunctionAddress->u1.Function,
                                    (ULONG_PTR)(ImportName->Name)
                                  );
            }
        }

    return TRUE;
}

PVOID
BindpRvaToVa(
    PBINDP_PARAMETERS Parms,
    ULONG Rva,
    PLOADED_IMAGE Image
    )
{
    PVOID Va;

    Va = ImageRvaToVa( Image->FileHeader,
                       Image->MappedAddress,
                       Rva,
                       &Image->LastRvaSection
                     );
    if (!Va && Parms->StatusRoutine != NULL) {
        (Parms->StatusRoutine)( BindRvaToVaFailed,
                                Image->ModuleName,
                                NULL,
                                (ULONG)Rva,
                                0
                              );
        }

    return Va;
}

VOID
SetIdataToRo(
    PLOADED_IMAGE Image
    )
{
    PIMAGE_SECTION_HEADER Section;
    ULONG i;

    for(Section = Image->Sections,i=0; i<Image->NumberOfSections; i++,Section++) {
        if (!_stricmp((PCHAR) Section->Name, ".idata")) {
            if (Section->Characteristics & IMAGE_SCN_MEM_WRITE) {
                Section->Characteristics &= ~IMAGE_SCN_MEM_WRITE;
                Section->Characteristics |= IMAGE_SCN_MEM_READ;
                }

            break;
            }
        }
}

VOID
BindpWalkAndProcessImports(
    PBINDP_PARAMETERS Parms,
    PLOADED_IMAGE Image,
    LPSTR DllPath,
    PBOOL ImageModified
    )
{

    ULONG  ForwarderChainHead;
    PULONG ForwarderChain;
    ULONG ImportSize;
    ULONG ExportSize;
    PIMPORT_DESCRIPTOR NewImportDescriptorHead, NewImportDescriptor;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR PrevNewImports, NewImports;
    ULONG PrevNewImportsSize, NewImportsSize;
    PIMAGE_IMPORT_DESCRIPTOR Imports;
    PIMAGE_EXPORT_DIRECTORY Exports;
    LPSTR ImportModule;
    PLOADED_IMAGE Dll;
    PIMAGE_THUNK_DATA tname,tsnap;
    PIMAGE_THUNK_DATA ThunkNames;
    PIMAGE_THUNK_DATA SnappedThunks;
    PIMAGE_IMPORT_BY_NAME ImportName;
    ULONG NumberOfThunks;
    ULONG i, cb;
    BOOL Ordinal, BindThunkFailed, NoErrors;
    USHORT OrdinalNumber;
    UCHAR NameBuffer[ 32 ];
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;
    PIMAGE_FILE_HEADER FileHeader;

    *ImageModified = FALSE;

    //
    // Locate the import array for this image/dll
    //

    NewImportDescriptorHead = NULL;
    Imports = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(
                                            (PVOID)Image->MappedAddress,
                                            FALSE,
                                            IMAGE_DIRECTORY_ENTRY_IMPORT,
                                            &ImportSize
                                            );
    if (Imports == NULL) {
        //
        // Nothing to bind if no imports
        //

        return;
    }

    FileHeader = &((PIMAGE_NT_HEADERS32)Image->FileHeader)->FileHeader;
    OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)Image->FileHeader,
                                 &OptionalHeader32,
                                 &OptionalHeader64);

    PrevNewImports = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(
                                                (PVOID)Image->MappedAddress,
                                                FALSE,
                                                IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
                                                &PrevNewImportsSize
                                                );

    // If the user asked for an old style bind and there are new style bind records
    // already in the image, zero them out first.  This is the fix the problem where
    // you bind on NT (creating new import descriptors), boot Win95 and bind there
    // (creating old bind format), and then reboot to NT (the loader will only check
    // the BOUND_IMPORT array.

    if (PrevNewImports &&
        (Parms->fNewImports == FALSE) &&
        (Parms->fNoUpdate == FALSE ))
    {
        OPTIONALHEADER_LV(DataDirectory[ IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ].VirtualAddress) = 0;
        OPTIONALHEADER_LV(DataDirectory[ IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ].Size) = 0;
        PrevNewImports = 0;
        PrevNewImportsSize = 0;
        *ImageModified = TRUE;
    }

    //
    // For each import record
    //

    for(;Imports;Imports++) {
        if ( !Imports->Name ) {
            break;
        }

        //
        // Locate the module being imported and load the dll
        //

        ImportModule = (LPSTR)BindpRvaToVa( Parms, Imports->Name, Image );

        if (ImportModule) {
            Dll = ImageLoad( ImportModule, DllPath );
            if (!Dll) {
                if (Parms->StatusRoutine != NULL) {
                    (Parms->StatusRoutine)( BindImportModuleFailed,
                                            Image->ModuleName,
                                            ImportModule,
                                            0,
                                            0
                                          );
                }
                //
                // Unless specifically told not to, generate the new style
                // import descriptor.
                //

                BindpAddImportDescriptor(Parms,
                                         &NewImportDescriptorHead,
                                         Imports,
                                         ImportModule,
                                         Dll
                                        );
                continue;
            }

            if (Parms->StatusRoutine != NULL) {
                (Parms->StatusRoutine)( BindImportModule,
                                        Image->ModuleName,
                                        ImportModule,
                                        0,
                                        0
                                      );
            }
            //
            // If we can load the DLL, locate the export section and
            // start snapping the thunks
            //

            Exports = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData(
                                                    (PVOID)Dll->MappedAddress,
                                                    FALSE,
                                                    IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                    &ExportSize
                                                    );
            if ( !Exports ) {
                continue;
            }

            //
            // assert that the export directory addresses can be translated
            //

            if ( !BindpRvaToVa( Parms, Exports->AddressOfNames, Dll ) ) {
                continue;
            }

            if ( !BindpRvaToVa( Parms, Exports->AddressOfNameOrdinals, Dll ) ) {
                continue;
            }

            if ( !BindpRvaToVa( Parms, Exports->AddressOfFunctions, Dll ) ) {
                continue;
            }

            //
            // For old style bind, bypass the bind if it's already bound.
            // New style binds s/b looked up in PrevNewImport.
            //

            if ( Parms->fNewImports == FALSE &&
                 Imports->TimeDateStamp &&
                 Imports->TimeDateStamp == FileHeader->TimeDateStamp ) {
                    continue;
            }

            //
            // Now we need to size our thunk table and
            // allocate a buffer to hold snapped thunks. This is
            // done instead of writting to the mapped view so that
            // thunks are only updated if we find all the entry points
            //

            ThunkNames = (PIMAGE_THUNK_DATA) BindpRvaToVa( Parms, Imports->OriginalFirstThunk, Image );

            if (!ThunkNames || ThunkNames->u1.Function == 0) {
                //
                // Skip this one if no thunks or first thunk is the terminating null thunk
                //
                continue;
            }

            //
            // Unless specifically told not to, generate the new style
            // import descriptor.
            //

            NewImportDescriptor = BindpAddImportDescriptor(Parms,
                                                           &NewImportDescriptorHead,
                                                           Imports,
                                                           ImportModule,
                                                           Dll
                                                          );
            NumberOfThunks = 0;
            tname = ThunkNames;
            while (tname->u1.AddressOfData) {
                NumberOfThunks++;
                tname++;
            }
            SnappedThunks = (PIMAGE_THUNK_DATA) MemAlloc( NumberOfThunks*sizeof(*SnappedThunks) );
            if ( !SnappedThunks ) {
                continue;
            }

            tname = ThunkNames;
            tsnap = SnappedThunks;
            NoErrors = TRUE;
            ForwarderChainHead = (ULONG)-1;
            ForwarderChain = &ForwarderChainHead;
            for(i=0;i<NumberOfThunks;i++) {
                BindThunkFailed = FALSE;
                __try {
                    if (!BindpLookupThunk( Parms,
                                           tname,
                                           Image,
                                           SnappedThunks,
                                           tsnap,
                                           Dll,
                                           Exports,
                                           NewImportDescriptor,
                                           DllPath,
                                           &ForwarderChain
                                         )
                       ) {
                        BindThunkFailed = TRUE;
                    }
                } __except ( EXCEPTION_EXECUTE_HANDLER ) {
                    BindThunkFailed = TRUE;
                }

                if (BindThunkFailed) {
                    if (NewImportDescriptor != NULL) {
                        NewImportDescriptor->TimeDateStamp = 0;
                    }

                    if (Parms->StatusRoutine != NULL) {
                        Ordinal = (BOOL)IMAGE_SNAP_BY_ORDINAL(tname->u1.Ordinal);
                        if (Ordinal) {
                            UCHAR szOrdinal[8];

                            OrdinalNumber = (USHORT)(IMAGE_ORDINAL(tname->u1.Ordinal) - Exports->Base);
                            ImportName = (PIMAGE_IMPORT_BY_NAME)NameBuffer;
                            // Can't use sprintf w/o dragging in more CRT support than we want...  Must run on Win95.
                            strcpy((PCHAR) ImportName->Name, "Ordinal");
                            strcat((PCHAR) ImportName->Name, _ultoa((ULONG) OrdinalNumber, (LPSTR)szOrdinal, 16));
                        }
                        else {
                            ImportName = (PIMAGE_IMPORT_BY_NAME)BindpRvaToVa(
                                                                    Parms,
                                                                    (ULONG)(ULONG_PTR)(tname->u1.AddressOfData),
                                                                    Image
                                                                    );
                        }

                        (Parms->StatusRoutine)( BindImportProcedureFailed,
                                                Image->ModuleName,
                                                Dll->ModuleName,
                                                (ULONG_PTR)tsnap->u1.Function,
                                                (ULONG_PTR)(ImportName->Name)
                                              );
                    }

                    break;
                }

                tname++;
                tsnap++;
            }

            tname = (PIMAGE_THUNK_DATA) BindpRvaToVa( Parms, Imports->FirstThunk, Image );
            if ( !tname ) {
                NoErrors = FALSE;
            }

            //
            // If we were able to locate all of the entrypoints in the
            // target dll, then copy the snapped thunks into the image,
            // update the time and date stamp, and flush the image to
            // disk
            //

            if ( NoErrors && Parms->fNoUpdate == FALSE ) {
                if (ForwarderChainHead != -1) {
                    *ImageModified = TRUE;
                    *ForwarderChain = -1;
                }
                if (Imports->ForwarderChain != ForwarderChainHead) {
                    Imports->ForwarderChain = ForwarderChainHead;
                    *ImageModified = TRUE;
                }
                cb = NumberOfThunks*sizeof(*SnappedThunks);
                if (memcmp(tname,SnappedThunks,cb)) {
                    MoveMemory(tname,SnappedThunks,cb);
                    *ImageModified = TRUE;
                }
                if (NewImportDescriptorHead == NULL) {
                    if (Imports->TimeDateStamp != FileHeader->TimeDateStamp) {
                        Imports->TimeDateStamp = FileHeader->TimeDateStamp;
                        *ImageModified = TRUE;
                    }
                }
                else
                if (Imports->TimeDateStamp != 0xFFFFFFFF) {
                    Imports->TimeDateStamp = 0xFFFFFFFF;
                    *ImageModified = TRUE;
                }
            }

            MemFree(SnappedThunks);
        }
    }

    NewImports = BindpCreateNewImportSection(Parms, &NewImportDescriptorHead, &NewImportsSize);
    if (PrevNewImportsSize != NewImportsSize ||
        memcmp( PrevNewImports, NewImports, NewImportsSize )
       ) {
        *ImageModified = TRUE;
    }

    if (!*ImageModified) {
        return;
    }

    if (Parms->StatusRoutine != NULL) {
        (Parms->StatusRoutine)( BindImageModified,
                                Image->ModuleName,
                                NULL,
                                0,
                                0
                              );
    }

    if (NewImports != NULL) {
        ULONG cbFreeFile, cbFreeHeaders, OffsetHeaderFreeSpace, cbFreeSpaceOnDisk;

        if (NoErrors && Parms->fNoUpdate == FALSE) {
            OPTIONALHEADER_LV(DataDirectory[ IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ].VirtualAddress) = 0;
            OPTIONALHEADER_LV(DataDirectory[ IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ].Size) = 0;
        }
        OffsetHeaderFreeSpace = GetImageUnusedHeaderBytes( Image, &cbFreeFile );
        cbFreeHeaders = Image->Sections->VirtualAddress -
                        OPTIONALHEADER(SizeOfHeaders) +
                        cbFreeFile;

        // FreeSpace on Disk may be larger that FreeHeaders in the headers (the linker
        // can start the first section on a page boundary already)

        cbFreeSpaceOnDisk = Image->Sections->PointerToRawData -
                            OPTIONALHEADER(SizeOfHeaders) +
                            cbFreeFile;

        if (NewImportsSize > cbFreeFile) {
            if (NewImportsSize > cbFreeHeaders) {
                if (Parms->StatusRoutine != NULL) {
                    (Parms->StatusRoutine)( BindNoRoomInImage,
                                            Image->ModuleName,
                                            NULL,
                                            0,
                                            0
                                          );
                }
                NoErrors = FALSE;
            }
            else
            if (NoErrors && (Parms->fNoUpdate == FALSE)) {
                if (NewImportsSize <= cbFreeSpaceOnDisk) {

                    // There's already space on disk.  Just adjust the header size.

                    OPTIONALHEADER_LV(SizeOfHeaders) =
                        (OPTIONALHEADER(SizeOfHeaders) -
                         cbFreeFile + NewImportsSize + (OPTIONALHEADER(FileAlignment)-1)
                        ) & ~(OPTIONALHEADER(FileAlignment)-1);

                } else  {

                    NoErrors = BindpExpandImageFileHeaders( Parms,
                                                            Image,
                                                           (OPTIONALHEADER(SizeOfHeaders) -
                                                             cbFreeFile +
                                                             NewImportsSize +
                                                             (OPTIONALHEADER(FileAlignment)-1)
                                                            ) &
                                                             ~(OPTIONALHEADER(FileAlignment)-1)
                                                          );
                    // Expand may have remapped the image.  Recalc the header ptrs.
                    FileHeader = &((PIMAGE_NT_HEADERS32)Image->FileHeader)->FileHeader;
                    OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)Image->FileHeader,
                                                 &OptionalHeader32,
                                                 &OptionalHeader64);

                }
            }
        }

        if (Parms->StatusRoutine != NULL) {
            (Parms->StatusRoutine)( BindImageComplete,
                                    Image->ModuleName,
                                    NULL,
                                    (ULONG_PTR)NewImports,
                                    NoErrors
                                  );
        }

        if (NoErrors && Parms->fNoUpdate == FALSE) {
            OPTIONALHEADER_LV(DataDirectory[ IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ].VirtualAddress) = OffsetHeaderFreeSpace;
            OPTIONALHEADER_LV(DataDirectory[ IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ].Size) = NewImportsSize;
            memcpy( (LPSTR)(Image->MappedAddress) + OffsetHeaderFreeSpace,
                    NewImports,
                    NewImportsSize
                  );
        }

        MemFree(NewImports);
    }

    if (NoErrors && Parms->fNoUpdate == FALSE) {
        SetIdataToRo( Image );
    }
}


DWORD
GetImageUnusedHeaderBytes(
    PLOADED_IMAGE LoadedImage,
    LPDWORD SizeUnusedHeaderBytes
    )
{
    DWORD OffsetFirstUnusedHeaderByte;
    DWORD i;
    DWORD OffsetHeader;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;
    PIMAGE_NT_HEADERS32 NtHeaders;

    NtHeaders = (PIMAGE_NT_HEADERS32)LoadedImage->FileHeader;

    //
    // this calculates an offset, not an address, so DWORD is correct
    //
    OffsetFirstUnusedHeaderByte = (DWORD)
       (((LPSTR)NtHeaders - (LPSTR)LoadedImage->MappedAddress) +
        (FIELD_OFFSET( IMAGE_NT_HEADERS32, OptionalHeader ) +
         NtHeaders->FileHeader.SizeOfOptionalHeader +
         (NtHeaders->FileHeader.NumberOfSections *
          sizeof(IMAGE_SECTION_HEADER)
         )
        )
       );

    OptionalHeadersFromNtHeaders(NtHeaders,
                                 &OptionalHeader32,
                                 &OptionalHeader64);

    for ( i=0; i<OPTIONALHEADER(NumberOfRvaAndSizes); i++ ) {
        OffsetHeader = OPTIONALHEADER(DataDirectory[i].VirtualAddress);
        if (OffsetHeader < OPTIONALHEADER(SizeOfHeaders)) {
            if (OffsetHeader >= OffsetFirstUnusedHeaderByte) {
                OffsetFirstUnusedHeaderByte = OffsetHeader +
                    OPTIONALHEADER(DataDirectory[i].Size);
                }
            }
        }

    *SizeUnusedHeaderBytes = OPTIONALHEADER(SizeOfHeaders) -
                             OffsetFirstUnusedHeaderByte;

    return OffsetFirstUnusedHeaderByte;
}
