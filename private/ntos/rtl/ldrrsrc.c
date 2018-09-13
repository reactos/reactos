/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ldrrsrc.c

Abstract:

    Loader API calls for accessing resource sections.

Author:

    Steve Wood (stevewo) 16-Sep-1991

Revision History:

--*/

#include "ntrtlp.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,LdrAccessResource)
#pragma alloc_text(PAGE,LdrpAccessResourceData)
#pragma alloc_text(PAGE,LdrFindEntryForAddress)
#pragma alloc_text(PAGE,LdrFindResource_U)
#pragma alloc_text(PAGE,LdrFindResourceDirectory_U)
#pragma alloc_text(PAGE,LdrpCompareResourceNames_U)
#pragma alloc_text(PAGE,LdrpSearchResourceSection_U)
#pragma alloc_text(PAGE,LdrEnumResources)
#endif

#ifndef NTOS_KERNEL_RUNTIME

PALT_RESOURCE_MODULE AlternateResourceModules;
ULONG AlternateResourceModuleCount;
ULONG AltResMemBlockCount;
LANGID UILangId, InstallLangId;

#define  MEMBLOCKSIZE 16
#define  RESMODSIZE sizeof(ALT_RESOURCE_MODULE)

#endif

NTSTATUS
LdrAccessResource(
    IN PVOID DllHandle,
    IN PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    OUT PVOID *Address OPTIONAL,
    OUT PULONG Size OPTIONAL
    )

/*++

Routine Description:

    This function locates the address of the specified resource in the
    specified DLL and returns its address.

Arguments:

    DllHandle - Supplies a handle to the image file that the resource is
        contained in.

    ResourceDataEntry - Supplies a pointer to the resource data entry in
        the resource data section of the image file specified by the
        DllHandle parameter.  This pointer should have been one returned
        by the LdrFindResource function.

    Address - Optional pointer to a variable that will receive the
        address of the resource specified by the first two parameters.

    Size - Optional pointer to a variable that will receive the size of
        the resource specified by the first two parameters.

Return Value:

    TBD

--*/

{
    RTL_PAGED_CODE();

    return LdrpAccessResourceData(
      DllHandle,
      ResourceDataEntry,
      Address,
      Size
      );
}


NTSTATUS
LdrpAccessResourceData(
    IN PVOID DllHandle,
    IN PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    OUT PVOID *Address OPTIONAL,
    OUT PULONG Size OPTIONAL
    )

/*++

Routine Description:

    This function returns the data necessary to actually examine the
    contents of a particular resource.

Arguments:

    DllHandle - Supplies a handle to the image file that the resource is
        contained in.

    ResourceDataEntry - Supplies a pointer to the resource data entry in
   the resource data directory of the image file specified by the
        DllHandle parameter.  This pointer should have been one returned
        by the LdrFindResource function.

    Address - Optional pointer to a variable that will receive the
        address of the resource specified by the first two parameters.

    Size - Optional pointer to a variable that will receive the size of
        the resource specified by the first two parameters.


Return Value:

    TBD

--*/

{
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory;
    ULONG ResourceSize;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG_PTR VirtualAddressOffset;
    PIMAGE_SECTION_HEADER NtSection;

    RTL_PAGED_CODE();

#ifndef NTOS_KERNEL_RUNTIME
    ResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
        RtlImageDirectoryEntryToData(DllHandle,
                                     TRUE,
                                     IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                     &ResourceSize
                                     );
    if (!ResourceDirectory) {
        return( STATUS_RESOURCE_DATA_NOT_FOUND );
    }

    if ((ULONG_PTR)ResourceDataEntry < (ULONG_PTR) ResourceDirectory ){
        DllHandle = LdrLoadAlternateResourceModule (DllHandle, NULL);
    } else{
        NtHeaders = RtlImageNtHeader(
                        (PVOID)((ULONG_PTR)DllHandle & ~0x00000001)
                        );
        if (NtHeaders) {
            // Find the bounds of the image so we can see if this resource entry is in an alternate
            // resource dll.

            ULONG_PTR ImageStart = (ULONG_PTR)DllHandle & ~0x00000001;
            SIZE_T ImageSize = 0;

            if ((ULONG_PTR)DllHandle & 0x00000001) {
                // mapped as datafile.  Ask mm for the size
                NTSTATUS Status;
                MEMORY_BASIC_INFORMATION MemInfo;

                Status = NtQueryVirtualMemory(
                            NtCurrentProcess(),
                            (PVOID) ImageStart,
                            MemoryBasicInformation,
                            (PVOID)&MemInfo,
                            sizeof(MemInfo),
                            NULL
                            );

                if ( !NT_SUCCESS(Status) ) {
                    ImageSize = 0;
                } else {
                    ImageSize = MemInfo.RegionSize;
                }
            } else {
                ImageSize = ((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.SizeOfImage;
            }

            if (!(((ULONG_PTR)ResourceDataEntry >= ImageStart) && ((ULONG_PTR)ResourceDataEntry < (ImageStart + ImageSize)))) {
                // Doesn't fall within the specified image.  Must be an alternate dll.
                DllHandle = LdrLoadAlternateResourceModule (DllHandle, NULL);
            }
        }
    }

    if (!DllHandle){
        return ( STATUS_RESOURCE_DATA_NOT_FOUND );
    }

#endif
    try {
        ResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
            RtlImageDirectoryEntryToData(DllHandle,
                                         TRUE,
                                         IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                         &ResourceSize
                                         );
        if (!ResourceDirectory) {
            return( STATUS_RESOURCE_DATA_NOT_FOUND );
            }

        if ((ULONG_PTR)DllHandle & 0x00000001) {
            ULONG ResourceRVA;
            DllHandle = (PVOID)((ULONG_PTR)DllHandle & ~0x00000001);
            NtHeaders = RtlImageNtHeader( DllHandle );
            if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
                ResourceRVA=((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_RESOURCE ].VirtualAddress;
            } else if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
                ResourceRVA=((PIMAGE_NT_HEADERS64)NtHeaders)->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_RESOURCE ].VirtualAddress;
            } else {
                ResourceRVA = 0;
            }

            if (!ResourceRVA) {
                return( STATUS_RESOURCE_DATA_NOT_FOUND );
                }

            VirtualAddressOffset = (ULONG_PTR)DllHandle + ResourceRVA - (ULONG_PTR)ResourceDirectory;

            //
            // Now, we must check to see if the resource is not in the
            // same section as the resource table.  If it's in .rsrc1,
            // we've got to adjust the RVA in the ResourceDataEntry
            // to point to the correct place in the non-VA data file.
            //
            NtSection= RtlSectionTableFromVirtualAddress( NtHeaders, DllHandle, ResourceRVA);

            if (!NtSection) {
                return( STATUS_RESOURCE_DATA_NOT_FOUND );
                }

            if ( ResourceDataEntry->OffsetToData > NtSection->Misc.VirtualSize ) {
                ULONG rva;

                rva = NtSection->VirtualAddress;
                NtSection= RtlSectionTableFromVirtualAddress(NtHeaders,
                                                             DllHandle,
                                                             ResourceDataEntry->OffsetToData
                                                             );
                VirtualAddressOffset +=
                        ((ULONG_PTR)NtSection->VirtualAddress - rva) -
                        ((ULONG_PTR)RtlAddressInSectionTable ( NtHeaders, DllHandle, NtSection->VirtualAddress ) - (ULONG_PTR)ResourceDirectory);
                }
            }
        else {
            VirtualAddressOffset = 0;
            }

        if (ARGUMENT_PRESENT( Address )) {
            *Address = (PVOID)( (PCHAR)DllHandle +
                                (ResourceDataEntry->OffsetToData - VirtualAddressOffset)
                              );
            }

        if (ARGUMENT_PRESENT( Size )) {
            *Size = ResourceDataEntry->Size;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
        }

    return( STATUS_SUCCESS );
}


NTSTATUS
LdrFindEntryForAddress(
    IN PVOID Address,
    OUT PLDR_DATA_TABLE_ENTRY *TableEntry
    )
/*++

Routine Description:

    This function returns the load data table entry that describes the virtual
    address range that contains the passed virtual address.

Arguments:

    Address - Supplies a 32-bit virtual address.

    TableEntry - Supplies a pointer to the variable that will receive the
        address of the loader data table entry.


Return Value:

    Status

--*/
{
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY Head, Next;
    PLDR_DATA_TABLE_ENTRY Entry;
    PIMAGE_NT_HEADERS NtHeaders;
    PVOID ImageBase;
    PVOID EndOfImage;

    Ldr = NtCurrentPeb()->Ldr;
    if (Ldr == NULL) {
        return( STATUS_NO_MORE_ENTRIES );
        }

    Head = &Ldr->InMemoryOrderModuleList;
    Next = Head->Flink;
    while ( Next != Head ) {
        Entry = CONTAINING_RECORD( Next, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks );

        NtHeaders = RtlImageNtHeader( Entry->DllBase );
        if (NtHeaders != NULL) {
            ImageBase = (PVOID)Entry->DllBase;

            EndOfImage = (PVOID)
                ((ULONG_PTR)ImageBase + NtHeaders->OptionalHeader.SizeOfImage);

            if ((ULONG_PTR)Address >= (ULONG_PTR)ImageBase && (ULONG_PTR)Address < (ULONG_PTR)EndOfImage) {
                *TableEntry = Entry;
                return( STATUS_SUCCESS );
                }
            }

        Next = Next->Flink;
        }

    return( STATUS_NO_MORE_ENTRIES );
}


NTSTATUS
LdrFindResource_U(
    IN PVOID DllHandle,
    IN PULONG_PTR ResourceIdPath,
    IN ULONG ResourceIdPathLength,
    OUT PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
    )

/*++

Routine Description:

    This function locates the address of the specified resource in the
    specified DLL and returns its address.

Arguments:

    DllHandle - Supplies a handle to the image file that the resource is
        contained in.

    ResourceIdPath - Supplies a pointer to an array of 32-bit resource
        identifiers.  Each identifier is either an integer or a pointer
        to a STRING structure that specifies a resource name.  The array
        is used to traverse the directory structure contained in the
        resource section in the image file specified by the DllHandle
        parameter.

    ResourceIdPathLength - Supplies the number of elements in the
        ResourceIdPath array.

    ResourceDataEntry - Supplies a pointer to a variable that will
        receive the address of the resource data entry in the resource
        data section of the image file specified by the DllHandle
        parameter.

Return Value:

    TBD

--*/

{
    RTL_PAGED_CODE();

    return LdrpSearchResourceSection_U(
      DllHandle,
      ResourceIdPath,
      ResourceIdPathLength,
      FALSE,                // Look for a leaf node
      FALSE,
      (PVOID *)ResourceDataEntry
      );
}


NTSTATUS
LdrFindResourceDirectory_U(
    IN PVOID DllHandle,
    IN PULONG_PTR ResourceIdPath,
    IN ULONG ResourceIdPathLength,
    OUT PIMAGE_RESOURCE_DIRECTORY *ResourceDirectory
    )

/*++

Routine Description:

    This function locates the address of the specified resource directory in
    specified DLL and returns its address.

Arguments:

    DllHandle - Supplies a handle to the image file that the resource
        directory is contained in.

    ResourceIdPath - Supplies a pointer to an array of 32-bit resource
        identifiers.  Each identifier is either an integer or a pointer
        to a STRING structure that specifies a resource name.  The array
        is used to traverse the directory structure contained in the
        resource section in the image file specified by the DllHandle
        parameter.

    ResourceIdPathLength - Supplies the number of elements in the
        ResourceIdPath array.

    ResourceDirectory - Supplies a pointer to a variable that will
        receive the address of the resource directory specified by
        ResourceIdPath in the resource data section of the image file
        the DllHandle parameter.

Return Value:

    TBD

--*/

{
    RTL_PAGED_CODE();

    return LdrpSearchResourceSection_U(
      DllHandle,
      ResourceIdPath,
      ResourceIdPathLength,
      TRUE,                 // Look for a directory node
      FALSE,
      (PVOID *)ResourceDirectory
      );
}


LONG
LdrpCompareResourceNames_U(
    IN ULONG_PTR ResourceName,
    IN PIMAGE_RESOURCE_DIRECTORY ResourceDirectory,
    IN PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry
    )
{
    LONG li;
    PIMAGE_RESOURCE_DIR_STRING_U ResourceNameString;

    if (ResourceName & LDR_RESOURCE_ID_NAME_MASK) {
        if (!ResourceDirectoryEntry->NameIsString) {
            return( -1 );
            }

        ResourceNameString = (PIMAGE_RESOURCE_DIR_STRING_U)
            ((PCHAR)ResourceDirectory + ResourceDirectoryEntry->NameOffset);

        li = wcsncmp( (LPWSTR)ResourceName,
            ResourceNameString->NameString,
            ResourceNameString->Length
          );

        if (!li && wcslen((PWSTR)ResourceName) != ResourceNameString->Length) {
       return( 1 );
       }

   return(li);
        }
    else {
        if (ResourceDirectoryEntry->NameIsString) {
            return( 1 );
            }

        return( (ULONG)(ResourceName - ResourceDirectoryEntry->Name) );
        }
}


#define  USE_FIRSTAVAILABLE_LANGID   (0xFFFFFFFF & ~LDR_RESOURCE_ID_NAME_MASK)

NTSTATUS
LdrpSearchResourceSection_U(
    IN PVOID DllHandle,
    IN PULONG_PTR ResourceIdPath,
    IN ULONG ResourceIdPathLength,
    IN BOOLEAN FindDirectoryEntry,
    IN BOOLEAN ExactLangMatchOnly,
    OUT PVOID *ResourceDirectoryOrData
    )

/*++

Routine Description:

    This function locates the address of the specified resource in the
    specified DLL and returns its address.

Arguments:

    DllHandle - Supplies a handle to the image file that the resource is
        contained in.

    ResourceIdPath - Supplies a pointer to an array of 32-bit resource
        identifiers.  Each identifier is either an integer or a pointer
        to a null terminated string (PSZ) that specifies a resource
        name.  The array is used to traverse the directory structure
        contained in the resource section in the image file specified by
        the DllHandle parameter.

    ResourceIdPathLength - Supplies the number of elements in the
        ResourceIdPath array.

    FindDirectoryEntry - Supplies a boolean that is TRUE if caller is
        searching for a resource directory, otherwise the caller is
        searching for a resource data entry.

    ExactLangMatchOnly - Supplies a boolean that is TRUE if caller is
        searching for a resource with, and only with, the language id
        specified in ResourceIdPath, otherwise the caller wants the routine
        to come up with default when specified langid is not found.

    ResourceDirectoryOrData - Supplies a pointer to a variable that will
        receive the address of the resource directory or data entry in
        the resource data section of the image file specified by the
        DllHandle parameter.

Return Value:

    TBD

--*/

{
    NTSTATUS Status;
    PIMAGE_RESOURCE_DIRECTORY LanguageResourceDirectory, ResourceDirectory, TopResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirEntLow;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirEntMiddle;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirEntHigh;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceEntry;
    USHORT n, half;
    LONG dir;
    ULONG size;
    ULONG_PTR ResourceIdRetry;
    ULONG RetryCount;
    LANGID NewLangId;
    PULONG_PTR IdPath = ResourceIdPath;
    ULONG IdPathLength = ResourceIdPathLength;
    BOOLEAN fIsNeutral = FALSE;
    LANGID GivenLanguage;
#ifndef NTOS_KERNEL_RUNTIME
    LCID DefaultThreadLocale, DefaultSystemLocale;
    PVOID AltResourceDllHandle = NULL;
    ULONG_PTR UIResourceIdPath[3];
#endif

    RTL_PAGED_CODE();

    try {
        TopResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
            RtlImageDirectoryEntryToData(DllHandle,
                                         TRUE,
                                         IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                         &size
                                         );
        if (!TopResourceDirectory) {
            return( STATUS_RESOURCE_DATA_NOT_FOUND );
            }

        ResourceDirectory = TopResourceDirectory;
        ResourceIdRetry = USE_FIRSTAVAILABLE_LANGID;
        RetryCount = 0;
        ResourceEntry = NULL;
        LanguageResourceDirectory = NULL;
        while (ResourceDirectory != NULL && ResourceIdPathLength--) {
            //
            // If search path includes a language id, then attempt to
            // match the following language ids in this order:
            //
            //   (0)  use given language id
            //   (1)  use primary language of given language id
            //   (2)  use id 0  (neutral resource)
            //   (3)  use default UI language id
            //
            // If the PRIMARY language id is ZERO, then ALSO attempt to
            // match the following language ids in this order:
            //
            //   (4)  use lang id of TEB if different from user locale
            //   (5)  use UI lang from exe resource
            //   (6)  use primary UI lang from exe resource
            //   (7)  use Install Language
            //   (8)  use lang id from user's locale id
            //   (9)  use primary language of user's locale id
            //   (10) use lang id from system default locale id
            //   (11) use primary language of system default locale id
            //   (12) use US English lang id
            //   (13) use any lang id that matches requested info
            //
            if (ResourceIdPathLength == 0 && IdPathLength == 3) {
                LanguageResourceDirectory = ResourceDirectory;
                }

            if (LanguageResourceDirectory != NULL) {
                GivenLanguage = (LANGID)IdPath[ 2 ];
                fIsNeutral = (PRIMARYLANGID( GivenLanguage ) == LANG_NEUTRAL);
TryNextLangId:
                switch( RetryCount++ ) {
#ifdef NTOS_KERNEL_RUNTIME
                    case 0:     // Use given language id
                        NewLangId = GivenLanguage;
                        break;

                    case 1:     // Use primary language of given language id
                        NewLangId = PRIMARYLANGID( GivenLanguage );
                        break;

                    case 2:     // Use id 0  (neutral resource)
                        NewLangId = 0;
                        break;

                    case 3:     // Use user's default UI language
                        NewLangId = (LANGID)ResourceIdRetry;
                        break;

                    case 4:     // Use native UI language
                        if ( !fIsNeutral ) {
                            // Stop looking - Not in the neutral case
                            goto ReturnFailure;
                            break;
                        }
                        NewLangId = PsInstallUILanguageId;
                        break;

                    case 5:     // Use default system locale
                        NewLangId = LANGIDFROMLCID(PsDefaultSystemLocaleId);
                        break;

                    case 6:
                        // Use US English language
                        NewLangId = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
                        break;

                    case 7:     // Take any lang id that matches
                        NewLangId = USE_FIRSTAVAILABLE_LANGID;
                        break;

#else
                    case 0:     // Use given language id
                        NewLangId = GivenLanguage;
                        break;

                    case 1:     // Use primary language of given language id
                        if ( ExactLangMatchOnly) {
                            //
                            //  Did not find an exact language match.
                            //  Stop looking.
                            //
                            goto ReturnFailure;
                        }
                        NewLangId = PRIMARYLANGID( GivenLanguage );
                        break;

                    case 2:     // Use id 0  (neutral resource)
                        NewLangId = 0;
                        break;

                    case 3:     // Use user's default UI language

                        if (!UILangId){
                            Status = NtQueryDefaultUILanguage( &UILangId );
                            if (!NT_SUCCESS( Status )) {
                                //
                                // Failed reading key.  Skip this lookup.
                                //
                                NewLangId = (LANGID)ResourceIdRetry;
                                break;
                            }
                        }
                        NewLangId = UILangId;
                        //
                        // Arabic/Hebrew MUI files may contain resources with LANG ID different than 401/40d.
                        // e.g. Comdlg32.dll has two sets of Arabic/Hebrew resources one mirrored (401/40d)
                        // and one flipped (801/80d).
                        //
                        if( !fIsNeutral &&
                            ((PRIMARYLANGID (GivenLanguage) == LANG_ARABIC) || (PRIMARYLANGID (GivenLanguage) == LANG_HEBREW)) &&
                            (PRIMARYLANGID (GivenLanguage) == PRIMARYLANGID (NewLangId))
                          ) {
                            NewLangId = GivenLanguage;
                        }

                        if (fIsNeutral || GivenLanguage == NewLangId){
                            //
                            //  Load alternate resource dll.
                            //
                            AltResourceDllHandle=LdrLoadAlternateResourceModule(
                                                    DllHandle,
                                                    NULL);

                            if (!AltResourceDllHandle){
                                //
                                //  Alternate resource dll not available.
                                //  Skip this lookup.
                                //
                                NewLangId = (LANGID)ResourceIdRetry;
                                break;

                            }

                            //
                            //  Map to alternate resource dll and search
                            //  it instead.
                            //

                            UIResourceIdPath[0]=IdPath[0];
                            UIResourceIdPath[1]=IdPath[1];
                            UIResourceIdPath[2]=NewLangId;

                            Status = LdrpSearchResourceSection_U(
                                        AltResourceDllHandle,
                                        UIResourceIdPath,
                                        3,
                                        FindDirectoryEntry,
                                        TRUE,
                                        (PVOID *)ResourceDirectoryOrData
                                        );

                            if (NT_SUCCESS(Status)){
                                //
                                // We sucessfully found alternate resource,
                                // return it.
                                //
                                return Status;
                            }


                        }
                        //
                        //  Caller does not want alternate resource, or
                        //  alternate resource not found.
                        //
                        NewLangId = (LANGID)ResourceIdRetry;
                        break;


                    case 4:     // Use langid of ThreadLocale if different from user locale
                        if ( !fIsNeutral ) {
                            // Stop looking - Not in the neutral case
                            goto ReturnFailure;
                            break;
                        }

                        //
                        // Try the thread locale language-id if it is different from
                        // user locale.
                        //
                        if (NtCurrentTeb()){
                            Status = NtQueryDefaultLocale(
                                        TRUE,
                                        &DefaultThreadLocale
                                        );
                            if (NT_SUCCESS( Status ) &&
                                DefaultThreadLocale !=
                                NtCurrentTeb()->CurrentLocale) {
                                //
                                // Thread locale is different from
                                // default locale.
                                //
                                NewLangId = LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale);
                                break;
                            }
                        }

                        NewLangId = (LANGID)ResourceIdRetry;
                        break;


                    case 5:   // UI language from the executable resource

                        if (!UILangId){
                            NewLangId = (LANGID)ResourceIdRetry;
                        } else {
                            NewLangId = UILangId;
                        }
                        break;

                    case 6:   // Parimary lang of UI language from the executable resource

                        if (!UILangId){
                            NewLangId = (LANGID)ResourceIdRetry;
                        } else {
                            NewLangId = PRIMARYLANGID( (LANGID) UILangId );
                        }
                        break;

                    case 7:   // Use install -native- language
                        //
                        // Thread locale is the same as the user locale, then let's
                        // try loading the native (install) ui language resources.
                        //
                        if (!InstallLangId){
                            Status = NtQueryInstallUILanguage(&InstallLangId);
                            if (!NT_SUCCESS( Status )) {
                                //
                                // Failed reading key.  Skip this lookup.
                                //
                                NewLangId = (LANGID)ResourceIdRetry;
                                break;

                            }
                        }

                        NewLangId = InstallLangId;
                        break;

                    case 8:     // Use lang id from locale in TEB
                        if (SUBLANGID( GivenLanguage ) == SUBLANG_SYS_DEFAULT) {
                            // Skip over all USER locale options
                            DefaultThreadLocale = 0;
                            RetryCount += 2;
                            break;
                        }

                        if (NtCurrentTeb() != NULL) {
                            NewLangId = LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale);
                        }
                        break;

                    case 9:     // Use User's default locale
                        Status = NtQueryDefaultLocale( TRUE, &DefaultThreadLocale );
                        if (NT_SUCCESS( Status )) {
                            NewLangId = LANGIDFROMLCID(DefaultThreadLocale);
                            break;
                            }

                        RetryCount++;
                        break;

                    case 10:     // Use primary language of User's default locale
                        NewLangId = PRIMARYLANGID( (LANGID)ResourceIdRetry );
                        break;

                    case 11:     // Use System default locale
                        Status = NtQueryDefaultLocale( FALSE, &DefaultSystemLocale );
                        if (!NT_SUCCESS( Status )) {
                            RetryCount++;
                            break;
                        }
                        if (DefaultSystemLocale != DefaultThreadLocale) {
                            NewLangId = LANGIDFROMLCID(DefaultSystemLocale);
                            break;
                        }

                        RetryCount += 2;
                        // fall through

                    case 13:     // Use US English language
                        NewLangId = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
                        break;

                    case 12:     // Use primary language of System default locale
                        NewLangId = PRIMARYLANGID( (LANGID)ResourceIdRetry );
                        break;

                    case 14:     // Take any lang id that matches
                        NewLangId = USE_FIRSTAVAILABLE_LANGID;
                        break;
#endif
                    default:    // No lang ids to match
                        goto ReturnFailure;
                        break;
                }

                //
                // If looking for a specific language id and same as the
                // one we just looked up, then skip it.
                //
                if (NewLangId != USE_FIRSTAVAILABLE_LANGID &&
                    NewLangId == ResourceIdRetry
                   ) {
                    goto TryNextLangId;
                    }

                //
                // Try this new language Id
                //
                ResourceIdRetry = (ULONG_PTR)NewLangId;
                ResourceIdPath = &ResourceIdRetry;
                ResourceDirectory = LanguageResourceDirectory;
                }

            n = ResourceDirectory->NumberOfNamedEntries;
            ResourceDirEntLow = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResourceDirectory+1);
            if (!(*ResourceIdPath & LDR_RESOURCE_ID_NAME_MASK)) {
                ResourceDirEntLow += n;
                n = ResourceDirectory->NumberOfIdEntries;
                }

            if (!n) {
                ResourceDirectory = NULL;
                goto NotFound;
                }

            if (LanguageResourceDirectory != NULL &&
                *ResourceIdPath == USE_FIRSTAVAILABLE_LANGID
               ) {
                ResourceDirectory = NULL;
                ResourceIdRetry = ResourceDirEntLow->Name;
                ResourceEntry = (PIMAGE_RESOURCE_DATA_ENTRY)
                    ((PCHAR)TopResourceDirectory +
                            ResourceDirEntLow->OffsetToData
                    );

                break;
                }

            ResourceDirectory = NULL;
            ResourceDirEntHigh = ResourceDirEntLow + n - 1;
            while (ResourceDirEntLow <= ResourceDirEntHigh) {
                if ((half = (n >> 1)) != 0) {
                    ResourceDirEntMiddle = ResourceDirEntLow;
                    if (*(PUCHAR)&n & 1) {
                        ResourceDirEntMiddle += half;
                        }
                    else {
                        ResourceDirEntMiddle += half - 1;
                        }
                    dir = LdrpCompareResourceNames_U( *ResourceIdPath,
                                                      TopResourceDirectory,
                                                      ResourceDirEntMiddle
                                                    );
                    if (!dir) {
                        if (ResourceDirEntMiddle->DataIsDirectory) {
                            ResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
                    ((PCHAR)TopResourceDirectory +
                                    ResourceDirEntMiddle->OffsetToDirectory
                                );
                            }
                        else {
                            ResourceDirectory = NULL;
                            ResourceEntry = (PIMAGE_RESOURCE_DATA_ENTRY)
                                ((PCHAR)TopResourceDirectory +
                  ResourceDirEntMiddle->OffsetToData
                                );
                            }

                        break;
                        }
                    else {
                        if (dir < 0) {
                            ResourceDirEntHigh = ResourceDirEntMiddle - 1;
                            if (*(PUCHAR)&n & 1) {
                                n = half;
                                }
                            else {
                                n = half - 1;
                                }
                            }
                        else {
                            ResourceDirEntLow = ResourceDirEntMiddle + 1;
                            n = half;
                            }
                        }
                    }
                else {
                    if (n != 0) {
                        dir = LdrpCompareResourceNames_U( *ResourceIdPath,
                          TopResourceDirectory,
                                                          ResourceDirEntLow
                                                        );
                        if (!dir) {
                            if (ResourceDirEntLow->DataIsDirectory) {
                                ResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
                                    ((PCHAR)TopResourceDirectory +
                                        ResourceDirEntLow->OffsetToDirectory
                                    );
                                }
                            else {
                                ResourceEntry = (PIMAGE_RESOURCE_DATA_ENTRY)
                                    ((PCHAR)TopResourceDirectory +
                      ResourceDirEntLow->OffsetToData
                                    );
                                }
                            }
                        }

                    break;
                    }
                }

            ResourceIdPath++;
            }

        if (ResourceEntry != NULL && !FindDirectoryEntry) {
            *ResourceDirectoryOrData = (PVOID)ResourceEntry;
            Status = STATUS_SUCCESS;
            }
        else
        if (ResourceDirectory != NULL && FindDirectoryEntry) {
            *ResourceDirectoryOrData = (PVOID)ResourceDirectory;
            Status = STATUS_SUCCESS;
            }
        else {
NotFound:
            switch( IdPathLength - ResourceIdPathLength) {
                case 3:     Status = STATUS_RESOURCE_LANG_NOT_FOUND; break;
                case 2:     Status = STATUS_RESOURCE_NAME_NOT_FOUND; break;
                case 1:     Status = STATUS_RESOURCE_TYPE_NOT_FOUND; break;
                default:    Status = STATUS_INVALID_PARAMETER; break;
                }
            }

        if (Status == STATUS_RESOURCE_LANG_NOT_FOUND &&
            LanguageResourceDirectory != NULL
           ) {
            ResourceEntry = NULL;
            goto TryNextLangId;
ReturnFailure: ;
            Status = STATUS_RESOURCE_LANG_NOT_FOUND;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    return Status;
}

NTSTATUS
LdrEnumResources(
    IN PVOID DllHandle,
    IN PULONG_PTR ResourceIdPath,
    IN ULONG ResourceIdPathLength,
    IN OUT PULONG NumberOfResources,
    OUT PLDR_ENUM_RESOURCE_ENTRY Resources OPTIONAL
    )
{
    NTSTATUS Status;
    PIMAGE_RESOURCE_DIRECTORY TopResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY TypeResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY NameResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY LangResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY TypeResourceDirectoryEntry;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY NameResourceDirectoryEntry;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY LangResourceDirectoryEntry;
    ULONG TypeDirectoryIndex, NumberOfTypeDirectoryEntries;
    ULONG NameDirectoryIndex, NumberOfNameDirectoryEntries;
    ULONG LangDirectoryIndex, NumberOfLangDirectoryEntries;
    BOOLEAN ScanTypeDirectory;
    BOOLEAN ScanNameDirectory;
    BOOLEAN ReturnThisResource;
    PIMAGE_RESOURCE_DIR_STRING_U ResourceNameString;
    ULONG_PTR TypeResourceNameOrId;
    ULONG_PTR NameResourceNameOrId;
    ULONG_PTR LangResourceNameOrId;
    PLDR_ENUM_RESOURCE_ENTRY ResourceInfo;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    ULONG ResourceIndex, MaxResourceIndex;
    ULONG Size;

    ResourceIndex = 0;
    if (!ARGUMENT_PRESENT( Resources )) {
        MaxResourceIndex = 0;
        }
    else {
        MaxResourceIndex = *NumberOfResources;
        }
    *NumberOfResources = 0;

    TopResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
        RtlImageDirectoryEntryToData( DllHandle,
                                      TRUE,
                                      IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                      &Size
                                    );
    if (!TopResourceDirectory) {
        return STATUS_RESOURCE_DATA_NOT_FOUND;
        }

    TypeResourceDirectory = TopResourceDirectory;
    TypeResourceDirectoryEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(TypeResourceDirectory+1);
    NumberOfTypeDirectoryEntries = TypeResourceDirectory->NumberOfNamedEntries +
                                   TypeResourceDirectory->NumberOfIdEntries;
    TypeDirectoryIndex = 0;
    Status = STATUS_SUCCESS;
    for (TypeDirectoryIndex=0;
         TypeDirectoryIndex<NumberOfTypeDirectoryEntries;
         TypeDirectoryIndex++, TypeResourceDirectoryEntry++
        ) {
        if (ResourceIdPathLength > 0) {
            ScanTypeDirectory = LdrpCompareResourceNames_U( ResourceIdPath[ 0 ],
                                                            TopResourceDirectory,
                                                            TypeResourceDirectoryEntry
                                                          ) == 0;
            }
        else {
            ScanTypeDirectory = TRUE;
            }
        if (ScanTypeDirectory) {
            if (!TypeResourceDirectoryEntry->DataIsDirectory) {
                return STATUS_INVALID_IMAGE_FORMAT;
                }
            if (TypeResourceDirectoryEntry->NameIsString) {
                ResourceNameString = (PIMAGE_RESOURCE_DIR_STRING_U)
                    ((PCHAR)TopResourceDirectory + TypeResourceDirectoryEntry->NameOffset);

                TypeResourceNameOrId = (ULONG_PTR)ResourceNameString;
                }
            else {
                TypeResourceNameOrId = (ULONG_PTR)TypeResourceDirectoryEntry->Id;
                }

            NameResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
                ((PCHAR)TopResourceDirectory + TypeResourceDirectoryEntry->OffsetToDirectory);
            NameResourceDirectoryEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(NameResourceDirectory+1);
            NumberOfNameDirectoryEntries = NameResourceDirectory->NumberOfNamedEntries +
                                           NameResourceDirectory->NumberOfIdEntries;
            NameDirectoryIndex = 0;
            for (NameDirectoryIndex=0;
                 NameDirectoryIndex<NumberOfNameDirectoryEntries;
                 NameDirectoryIndex++, NameResourceDirectoryEntry++
                ) {
                if (ResourceIdPathLength > 1) {
                    ScanNameDirectory = LdrpCompareResourceNames_U( ResourceIdPath[ 1 ],
                                                                    TopResourceDirectory,
                                                                    NameResourceDirectoryEntry
                                                                  ) == 0;
                    }
                else {
                    ScanNameDirectory = TRUE;
                    }
                if (ScanNameDirectory) {
                    if (!NameResourceDirectoryEntry->DataIsDirectory) {
                        return STATUS_INVALID_IMAGE_FORMAT;
                        }

                    if (NameResourceDirectoryEntry->NameIsString) {
                        ResourceNameString = (PIMAGE_RESOURCE_DIR_STRING_U)
                            ((PCHAR)TopResourceDirectory + NameResourceDirectoryEntry->NameOffset);

                        NameResourceNameOrId = (ULONG_PTR)ResourceNameString;
                        }
                    else {
                        NameResourceNameOrId = (ULONG_PTR)NameResourceDirectoryEntry->Id;
                        }

                    LangResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
                        ((PCHAR)TopResourceDirectory + NameResourceDirectoryEntry->OffsetToDirectory);

                    LangResourceDirectoryEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(LangResourceDirectory+1);
                    NumberOfLangDirectoryEntries = LangResourceDirectory->NumberOfNamedEntries +
                                                   LangResourceDirectory->NumberOfIdEntries;
                    LangDirectoryIndex = 0;
                    for (LangDirectoryIndex=0;
                         LangDirectoryIndex<NumberOfLangDirectoryEntries;
                         LangDirectoryIndex++, LangResourceDirectoryEntry++
                        ) {
                        if (ResourceIdPathLength > 2) {
                            ReturnThisResource = LdrpCompareResourceNames_U( ResourceIdPath[ 2 ],
                                                                             TopResourceDirectory,
                                                                             LangResourceDirectoryEntry
                                                                           ) == 0;
                            }
                        else {
                            ReturnThisResource = TRUE;
                            }
                        if (ReturnThisResource) {
                            if (LangResourceDirectoryEntry->DataIsDirectory) {
                                return STATUS_INVALID_IMAGE_FORMAT;
                                }

                            if (LangResourceDirectoryEntry->NameIsString) {
                                ResourceNameString = (PIMAGE_RESOURCE_DIR_STRING_U)
                                    ((PCHAR)TopResourceDirectory + LangResourceDirectoryEntry->NameOffset);

                                LangResourceNameOrId = (ULONG_PTR)ResourceNameString;
                                }
                            else {
                                LangResourceNameOrId = (ULONG_PTR)LangResourceDirectoryEntry->Id;
                                }

                            ResourceDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)
                                    ((PCHAR)TopResourceDirectory + LangResourceDirectoryEntry->OffsetToData);

                            ResourceInfo = &Resources[ ResourceIndex++ ];
                            if (ResourceIndex <= MaxResourceIndex) {
                                ResourceInfo->Path[ 0 ].NameOrId = TypeResourceNameOrId;
                                ResourceInfo->Path[ 1 ].NameOrId = NameResourceNameOrId;
                                ResourceInfo->Path[ 2 ].NameOrId = LangResourceNameOrId;
                                ResourceInfo->Data = (PVOID)((ULONG_PTR)DllHandle + ResourceDataEntry->OffsetToData);
                                ResourceInfo->Size = ResourceDataEntry->Size;
                                ResourceInfo->Reserved = 0;
                                }
                            else {
                                Status = STATUS_INFO_LENGTH_MISMATCH;
                                }
                            }
                        }
                    }
                }
            }
        }

    *NumberOfResources = ResourceIndex;
    return Status;
}

#ifndef NTOS_KERNEL_RUNTIME

BOOLEAN
LdrAlternateResourcesEnabled(
    VOID
    )

/*++

Routine Description:

    This function determines if the althernate resources are enabled.

Arguments:

    None.

Return Value:

    True - Alternate Resource enabled.
    False - Alternate Resource not enabled.

--*/

{
    NTSTATUS Status;

    if (!UILangId){
        Status = NtQueryDefaultUILanguage( &UILangId );

        if (!NT_SUCCESS( Status )) {
            //
            //  Failed to get UI LangID.  AltResource not enabled.
            //
            return FALSE;
            }
        }

    if (!InstallLangId){
        Status = NtQueryInstallUILanguage( &InstallLangId);

        if (!NT_SUCCESS( Status )) {
            //
            //  Failed to get Intall LangID.  AltResource not enabled.
            //
            return FALSE;
            }
        }

    if (UILangId == InstallLangId) {
        //
        //  UI Lang matches Installed Lang. AltResource not enabled.
        //
        return FALSE;
        }

    return TRUE;
}

PVOID
LdrGetAlternateResourceModuleHandle(
    IN PVOID Module
    )
/*++

Routine Description:

    This function gets the alternate resource module from the table
    containing the handle.

Arguments:

    Module - Module of which alternate resource module needs to loaded.

Return Value:

   Handle of the alternate resource module.

--*/

{
    ULONG ModuleIndex;

    for (ModuleIndex = 0;
         ModuleIndex < AlternateResourceModuleCount;
         ModuleIndex++ ){
        if (AlternateResourceModules[ModuleIndex].ModuleBase ==
            Module){
            return AlternateResourceModules[ModuleIndex].AlternateModule;
        }
    }
    return NULL;
}

BOOLEAN
LdrpGetFileVersion(
    IN  PVOID      ImageBase,
    IN  LANGID     LangId,
    OUT PULONGLONG Version
    )

/*++

Routine Description:

    Get the version stamp out of the VS_FIXEDFILEINFO resource in a PE
    image.

Arguments:

    ImageBase - supplies the address in memory where the file is mapped in.

    Version - receives 64bit version number, or 0 if the file is not
        a PE image or has no version data.

Return Value:

    None.

--*/

{
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    NTSTATUS Status;
    ULONG_PTR IdPath[3];
    ULONG ResourceSize;


typedef struct tagVS_FIXEDFILEINFO
{
    LONG   dwSignature;            /* e.g. 0xfeef04bd */
    LONG   dwStrucVersion;         /* e.g. 0x00000042 = "0.42" */
    LONG   dwFileVersionMS;        /* e.g. 0x00030075 = "3.75" */
    LONG   dwFileVersionLS;        /* e.g. 0x00000031 = "0.31" */
    LONG   dwProductVersionMS;     /* e.g. 0x00030010 = "3.10" */
    LONG   dwProductVersionLS;     /* e.g. 0x00000031 = "0.31" */
    LONG   dwFileFlagsMask;        /* = 0x3F for version "0.42" */
    LONG   dwFileFlags;            /* e.g. VFF_DEBUG | VFF_PRERELEASE */
    LONG   dwFileOS;               /* e.g. VOS_DOS_WINDOWS16 */
    LONG   dwFileType;             /* e.g. VFT_DRIVER */
    LONG   dwFileSubtype;          /* e.g. VFT2_DRV_KEYBOARD */
    LONG   dwFileDateMS;           /* e.g. 0 */
    LONG   dwFileDateLS;           /* e.g. 0 */
} VS_FIXEDFILEINFO;

    struct {
        USHORT TotalSize;
        USHORT DataSize;
        USHORT Type;
        WCHAR Name[16];              // L"VS_VERSION_INFO" + unicode nul
        VS_FIXEDFILEINFO FixedFileInfo;
    } *Resource;

    *Version = 0;


    IdPath[0] = 16;
    IdPath[1] = 1;
    IdPath[2] = LangId;

    try {
        Status = LdrpSearchResourceSection_U(
                    ImageBase,
                    IdPath,
                    3,
                    FALSE,
                    TRUE,
                    &DataEntry);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_UNSUCCESSFUL;
    }

    if(!NT_SUCCESS(Status)) {
        return FALSE;
    }

    try {
        Status = LdrpAccessResourceData(
                    ImageBase,
                    DataEntry,
                    &Resource,
                    &ResourceSize);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_UNSUCCESSFUL;
    }

    if(!NT_SUCCESS(Status)) {
        return FALSE;
    }

    try {
        if((ResourceSize >= sizeof(*Resource))
            && !_wcsicmp(Resource->Name,L"VS_VERSION_INFO")) {

            *Version = ((ULONGLONG)Resource->FixedFileInfo.dwFileVersionMS << 32
)
                     | (ULONGLONG)Resource->FixedFileInfo.dwFileVersionLS;

        } else {
            DbgPrint(("LDR: Warning: invalid version resource\n"));
            return FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        DbgPrint(("LDR: Exception encountered processing bogus version resource\n"));
        return FALSE;
    }
    return TRUE;
}

BOOLEAN
LdrpVerifyAlternateResourceModule(
    IN PVOID Module,
    IN PVOID AlternateModule
    )

/*++

Routine Description:

    This function verifies if the alternate resource module has the same
    version of the base module.

Arguments:

    Module - The handle of the base module.
    AlternateModule - The handle of the alternate resource module

Return Value:

    TBD.

--*/

{
    ULONGLONG ModuleVersion;
    ULONGLONG AltModuleVersion;
    NTSTATUS Status;

    if (!UILangId){
        Status = NtQueryDefaultUILanguage( &UILangId);
        if (!NT_SUCCESS( Status )) {
            //
            //  Failed to get UI LangID.  AltResource not enabled.
            //
            return FALSE;
            }
        }

    if (!LdrpGetFileVersion(AlternateModule, UILangId, &AltModuleVersion)){
        return FALSE;
        }

    if (!InstallLangId){
        Status = NtQueryInstallUILanguage (&InstallLangId);
        if (!NT_SUCCESS( Status )) {
            //
            //  Failed to get Install LangID.  AltResource not enabled.
            //
            return FALSE;
            }
        }

    if (!LdrpGetFileVersion(Module, InstallLangId, &ModuleVersion)){
        return FALSE;
        }

    if (ModuleVersion == AltModuleVersion){
        return TRUE;
        }
    else{
        return FALSE;
        }
}

BOOLEAN
LdrpSetAlternateResourceModuleHandle(
    IN PVOID Module,
    IN PVOID AlternateModule
    )

/*++

Routine Description:

    This function records the handle of the base module and alternate
    resource module in an array.

Arguments:

    Module - The handle of the base module.
    AlternateModule - The handle of the alternate resource module

Return Value:

    TBD.

--*/

{
    PALT_RESOURCE_MODULE NewModules;

    if (AlternateResourceModules == NULL){
        //
        //  Allocate memory of initial size MEMBLOCKSIZE.
        //
        NewModules = RtlAllocateHeap(
                        RtlProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        RESMODSIZE * MEMBLOCKSIZE);
        if (!NewModules){
            return FALSE;
            }
        AlternateResourceModules = NewModules;
        AltResMemBlockCount = MEMBLOCKSIZE;
        }
    else
    if (AlternateResourceModuleCount >= AltResMemBlockCount ){
        //
        //  ReAllocate another chunk of memory.
        //
        NewModules = RtlReAllocateHeap(
                        RtlProcessHeap(),
                        0,
                        AlternateResourceModules,
                        (AltResMemBlockCount + MEMBLOCKSIZE) * RESMODSIZE
                        );

        if (!NewModules){
            return FALSE;
            }
        AlternateResourceModules = NewModules;
        AltResMemBlockCount += MEMBLOCKSIZE;
        }

    AlternateResourceModules[AlternateResourceModuleCount].ModuleBase = Module;
    AlternateResourceModules[AlternateResourceModuleCount].AlternateModule = AlternateModule;



    AlternateResourceModuleCount++;

    return TRUE;

}

PVOID
LdrLoadAlternateResourceModule(
    IN PVOID Module,
    IN LPCWSTR PathToAlternateModule OPTIONAL
    )

/*++

Routine Description:

    This function does the acutally loading into memory of the alternate
    resource module, or loads from the table if it was loaded before.

Arguments:

    Module - The handle of the base module.
    PathToAlternateModule - Optional path from which module is being loaded.

Return Value:

    Handle to the alternate resource module.

--*/

{
    PVOID AlternateModule, DllBase;
    PLDR_DATA_TABLE_ENTRY Entry;
    HANDLE FileHandle, MappingHandle;
    PIMAGE_NT_HEADERS NtHeaders;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING AltDllName;
    PVOID FreeBuffer;
    LPWSTR BaseDllName, p;
    WCHAR DllPathName[DOS_MAX_PATH_LENGTH];
    ULONG DllPathNameLength, BaseDllNameLength, CopyCount;
    ULONG Digit;
    int i, RetryCount;
    WCHAR AltModulePath[DOS_MAX_PATH_LENGTH];
    WCHAR AltModulePathMUI[DOS_MAX_PATH_LENGTH];
    WCHAR AltModulePathFallback[DOS_MAX_PATH_LENGTH];
    IO_STATUS_BLOCK IoStatusBlock;
    RTL_RELATIVE_NAME RelativeName;
    SIZE_T ViewSize;
    LARGE_INTEGER SectionOffset;
    WCHAR LangIdDir[6];
    UNICODE_STRING AltModulePathList[3];
    UNICODE_STRING NtSystemRoot;


    if (!LdrAlternateResourcesEnabled()) {
        return NULL;
        }

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    AlternateModule = LdrGetAlternateResourceModuleHandle(Module);
    if (AlternateModule == NO_ALTERNATE_RESOURCE_MODULE){
        //
        //  We tried to load this module before but failed. Don't try
        //  again in the future.
        //
        RtlLeaveCriticalSection(
            (PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        return NULL;
        }
    else if (AlternateModule > 0){
        //
        //  We found the previously loaded match
        //
        RtlLeaveCriticalSection(
            (PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        return AlternateModule;
        }

    if (ARGUMENT_PRESENT(PathToAlternateModule)){
        //
        //  Caller suplied path.
        //

        CopyCount = wcslen(PathToAlternateModule);

        for (p = (LPWSTR) PathToAlternateModule + CopyCount;
             p > PathToAlternateModule;
             p--){
            if (*(p-1) == L'\\'){
                break;
                }
            }

        if (p == PathToAlternateModule){
            goto error_exit;
            }

        DllPathNameLength = (ULONG)(p - PathToAlternateModule) * sizeof(WCHAR);

        RtlCopyMemory(
            DllPathName,
            PathToAlternateModule,
            DllPathNameLength
            );

        BaseDllName = p ;
        BaseDllNameLength = CopyCount * sizeof(WCHAR) - DllPathNameLength;

        }
    else{
        //
        //  Try to get full dll path from Ldr data table.
        //
        Status = LdrFindEntryForAddress(Module, &Entry);
        if (!NT_SUCCESS( Status )){
            goto error_exit;
            }

        DllPathNameLength = Entry->FullDllName.Length -
                            Entry->BaseDllName.Length;

        RtlCopyMemory(
            DllPathName,
            Entry->FullDllName.Buffer,
            DllPathNameLength);

        BaseDllName = Entry->BaseDllName.Buffer;
        BaseDllNameLength = Entry->BaseDllName.Length;
        }

    DllPathName[DllPathNameLength / sizeof(WCHAR)] = UNICODE_NULL;

    //
    //  Generate the langid directory like "0804\"
    //
    if (!UILangId){
        Status = NtQueryDefaultUILanguage( &UILangId );
        if (!NT_SUCCESS( Status )) {
            goto error_exit;
            }
        }

    CopyCount = 0;
    for (i = 12; i >= 0; i -= 4){
        Digit = ((UILangId >> i) & 0xF);
        if (Digit >= 10){
            LangIdDir[CopyCount++] = (WCHAR) (Digit - 10 + L'A');
            }
        else{
            LangIdDir[CopyCount++] = (WCHAR) (Digit + L'0');
            }
        }

    LangIdDir[CopyCount++] = L'\\';
    LangIdDir[CopyCount++] = UNICODE_NULL;

    //
    //  Generate the first path c:\winnt\system32\mui\0804\ntdll.dll.mui
    //
    AltModulePathList[1].Buffer = AltModulePath;
    AltModulePathList[1].Length = 0;
    AltModulePathList[1].MaximumLength = sizeof(AltModulePath);

    RtlAppendUnicodeToString(&AltModulePathList[1], DllPathName);
    RtlAppendUnicodeToString(&AltModulePathList[1], L"mui\\");
    RtlAppendUnicodeToString(&AltModulePathList[1], LangIdDir);
    RtlAppendUnicodeToString(&AltModulePathList[1], BaseDllName);

    //
    //  Generate the first path c:\winnt\system32\mui\0804\ntdll.dll
    //
    AltModulePathList[0].Buffer = AltModulePathMUI;
    AltModulePathList[0].Length = 0;
    AltModulePathList[0].MaximumLength = sizeof(AltModulePathMUI);

    RtlCopyUnicodeString(&AltModulePathList[0], &AltModulePathList[1]);
    RtlAppendUnicodeToString(&AltModulePathList[0], L".mui");

    //
    //  Generate path c:\winnt\mui\fallback\0804\foo.exe.mui
    //
    AltModulePathList[2].Buffer = AltModulePathFallback;
    AltModulePathList[2].Length = 0;
    AltModulePathList[2].MaximumLength = sizeof(AltModulePathFallback);

    RtlInitUnicodeString(&NtSystemRoot, USER_SHARED_DATA->NtSystemRoot);
    RtlAppendUnicodeStringToString(&AltModulePathList[2], &NtSystemRoot);
    RtlAppendUnicodeToString(&AltModulePathList[2], L"\\mui\\fallback\\");
    RtlAppendUnicodeToString(&AltModulePathList[2], LangIdDir);
    RtlAppendUnicodeToString(&AltModulePathList[2], BaseDllName);
    RtlAppendUnicodeToString(&AltModulePathList[2], L".mui");

    //
    //  Try name with .mui extesion first.
    //
    RetryCount = 0;
    while (RetryCount < sizeof(AltModulePathList)/sizeof(UNICODE_STRING)){
        if (!RtlDosPathNameToNtPathName_U(
                    AltModulePathList[RetryCount].Buffer,
                    &AltDllName,
                    NULL,
                    &RelativeName
                    )){
            goto error_exit;
            }

        FreeBuffer = AltDllName.Buffer;
        if ( RelativeName.RelativeName.Length ) {
            AltDllName = *(PUNICODE_STRING)&RelativeName.RelativeName;
            }
        else {
            RelativeName.ContainingDirectory = NULL;
            }

        InitializeObjectAttributes(
            &ObjectAttributes,
            &AltDllName,
            OBJ_CASE_INSENSITIVE,
            RelativeName.ContainingDirectory,
            NULL
            );


        Status = NtCreateFile(
                &FileHandle,
                (ACCESS_MASK) GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                0L,
                FILE_SHARE_READ | FILE_SHARE_DELETE,
                FILE_OPEN,
                0L,
                NULL,
                0L
                );

        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

        if (NT_SUCCESS( Status )){
            break;
            }
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND && RetryCount == 0) {
            //
            //  Error other than the file name with .mui not found.
            //  Most likely directory is missing.  Skip file name w/o .mui
            //  and goto fallback directory.
            //
            RetryCount++;
            }

        RetryCount++;
        }

    Status = NtCreateSection(
                &MappingHandle,
                STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                NULL,
                NULL,
                PAGE_WRITECOPY,
                SEC_COMMIT,
                FileHandle
                );

    if (!NT_SUCCESS( Status )){
        goto error_exit;
        }

    NtClose( FileHandle );

    SectionOffset.LowPart = 0;
    SectionOffset.HighPart = 0;
    ViewSize = 0;
    DllBase = NULL;

    Status = NtMapViewOfSection(
                MappingHandle,
                NtCurrentProcess(),
                &DllBase,
                0L,
                0L,
                &SectionOffset,
                &ViewSize,
                ViewShare,
                0L,
                PAGE_WRITECOPY
                );

    NtClose( MappingHandle );

    if (!NT_SUCCESS( Status )){
        goto error_exit;
        }

    NtHeaders = RtlImageNtHeader( DllBase );
    if (!NtHeaders) {
        NtUnmapViewOfSection(NtCurrentProcess(), (PVOID) DllBase);
        goto error_exit;
        }

    AlternateModule = (HANDLE)((ULONG_PTR)DllBase | 0x00000001);

    //
    //  Disable version check now to make testing with an earlier
    //  localized version easier.
    //
//    if(!LdrpVerifyAlternateResourceModule(Module, AlternateModule)){
//	 NtUnmapViewOfSection(NtCurrentProcess(), (PVOID) DllBase);
//	 goto error_exit;
//	 }


    LdrpSetAlternateResourceModuleHandle(Module, AlternateModule);
    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    return AlternateModule;

error_exit:
    LdrpSetAlternateResourceModuleHandle(Module, NO_ALTERNATE_RESOURCE_MODULE);
    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    return NULL;
}

BOOLEAN
LdrUnloadAlternateResourceModule(
    IN PVOID Module
    )

/*++

Routine Description:

    This function unmaps an alternate resource module from the process'
    address space and updates alternate resource module table.

Arguments:

    Module - handle of the base module.

Return Value:

    TBD.

--*/

{
    ULONG ModuleIndex;
    PALT_RESOURCE_MODULE AltModule;

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    if (AlternateResourceModuleCount == 0){
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        return TRUE;
        }

    for (ModuleIndex = AlternateResourceModuleCount;
         ModuleIndex > 0;
         ModuleIndex--){
        if (AlternateResourceModules[ModuleIndex-1].ModuleBase == Module){
            break;
        }
    }

    if (ModuleIndex == 0){
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        return FALSE;
        }
    //
    //  Adjust to the actual index
    //
    ModuleIndex --;

    AltModule = &AlternateResourceModules[ModuleIndex];
    if (AltModule->AlternateModule != NO_ALTERNATE_RESOURCE_MODULE) {
        NtUnmapViewOfSection(
            NtCurrentProcess(),
            (PVOID)((ULONG_PTR)AltModule->AlternateModule & ~0x00000001)
            );
    }

    __try {
        if (ModuleIndex != AlternateResourceModuleCount - 1){
            //
            //  Consolidate the array.  Skip this if unloaded item
            //  is the last element.
            //
            RtlMoveMemory(
                AltModule,
                AltModule + 1,
                (AlternateResourceModuleCount - ModuleIndex - 1) * RESMODSIZE
                );
            }
        }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        return FALSE;
        }

    AlternateResourceModuleCount --;

    if (AlternateResourceModuleCount == 0){
        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            AlternateResourceModules
            );
        AlternateResourceModules = NULL;
        AltResMemBlockCount = 0;
        }
    else
    if (AlternateResourceModuleCount < AltResMemBlockCount - MEMBLOCKSIZE){
        AltModule = RtlReAllocateHeap(
                        RtlProcessHeap(),
                        0,
                        AlternateResourceModules,
                        (AltResMemBlockCount - MEMBLOCKSIZE) * RESMODSIZE);

        if (!AltModule){
            RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
            return FALSE;
            }

        AlternateResourceModules = AltModule;
        AltResMemBlockCount -= MEMBLOCKSIZE;
        }
    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    return TRUE;
}


BOOLEAN
LdrFlushAlternateResourceModules(
    VOID
    )

/*++

Routine Description:

    This function unmaps all the alternate resouce modules for the
    process address space. This function would be used mainly by
    CSRSS, and any sub-systems that are permanent during logon and
    logoff.


Arguments:

    None

Return Value:

    TRUE  : Successful
    FALSE : Failed

--*/

{
    ULONG ModuleIndex;
    PALT_RESOURCE_MODULE AltModule;

    //
    // Grab the loader lock
    //
    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    if (AlternateResourceModuleCount > 0) {
        //
        // Let's unmap the alternate resource modules from the process
        // address space
        //
        for (ModuleIndex=0;
             ModuleIndex<AlternateResourceModuleCount;
             ModuleIndex++) {

            AltModule = &AlternateResourceModules[ModuleIndex];

            if (AltModule->AlternateModule != NO_ALTERNATE_RESOURCE_MODULE) {
                NtUnmapViewOfSection(NtCurrentProcess(),
                                     (PVOID)((ULONG_PTR)AltModule->AlternateModule & ~0x00000001));
                }
            }

        //
        // Cleanup alternate resource modules memory
        //
        RtlFreeHeap(RtlProcessHeap(), 0, AlternateResourceModules);
        AlternateResourceModules = NULL;
        AlternateResourceModuleCount = 0;
        AltResMemBlockCount = 0;

        }

    //
    // Re-Initialize the UI language for the current process,
    // and leave the LoaderLock
    //
    UILangId = 0;
    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    return TRUE;
}

#endif
