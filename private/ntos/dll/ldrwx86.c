/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrwx86.c

Abstract:

    This module implements the wx86 specific ldr functions.

Author:

    13-Jan-1995 Jonle , created

Revision History:

    15-Oct-1998 CBiks   Modified the code that throws the architecture
                        mismatch exception so the exception is only
                        thrown for NT 3,0 and lower executables.  This was
                        changed to make the Wx86 loader behave like the
                        real loader, which does not throw this exception.

                        Also added a call to the cleanup function when
                        LdrpWx86LoadDll() fails.  There were cases where the
                        CPU failed to initialize but the Wx86 global pointers
                        were not cleared and pointed to a invalid memory because
                        wx86.dll was unloaded.
--*/

#include "ntos.h"
#include "ldrp.h"

#define PAGE_SIZE_X86   (0x1000)

#if defined (WX86)

BOOLEAN (*Wx86ProcessInit)(PVOID, BOOLEAN) = NULL;
BOOLEAN (*Wx86DllMapNotify)(PVOID, BOOLEAN) = NULL;
BOOLEAN (*Wx86DllEntryPoint)(PDLL_INIT_ROUTINE, PVOID, ULONG, PCONTEXT) = NULL;
ULONG (*Wx86ProcessStartRoutine)(VOID) = NULL;
ULONG (*Wx86ThreadStartRoutine)(PVOID) = NULL;
BOOLEAN (*Wx86KnownDllName)(PUNICODE_STRING, PUNICODE_STRING) = NULL;
BOOLEAN (*Wx86KnownNativeDll)(PUNICODE_STRING) = NULL;
BOOLEAN (*Wx86KnownRedistDll)(PUNICODE_STRING) = NULL;

BOOLEAN Wx86OnTheFly=FALSE;
ULONG Wx86ProviderUnloadCount = 0;

WCHAR Wx86Dir[]=L"\\sys32x86";
UNICODE_STRING Wx86SystemDir={0,0,NULL};
UNICODE_STRING NtSystemRoot={0,0,NULL};

#if defined(BUILD_WOW6432)
    //
    // We want to accept only Alpha32 images as native.  USER_SHARED_DATA
    // will only include AXP64.
    //
    #define IsNativeMachineType(m)                  \
         ((m) == IMAGE_FILE_MACHINE_ALPHA)
#else
    #define IsNativeMachineType(m)                  \
        ((m) >= USER_SHARED_DATA->ImageNumberLow && \
         (m) <= USER_SHARED_DATA->ImageNumberHigh)
#endif


//
// Wx86 plugin support
//

typedef struct _Wx86Plugin {
  LIST_ENTRY Links;                         // plugin list links
  PVOID DllBase;                            // plugin dll base
  ULONG Count;                              // number of providers
  PVOID Provider[WX86PLUGIN_MAXPROVIDER];   // dll base of providers for this plugin
} WX86PLUGIN, *PWX86PLUGIN;                 // plugin list entry

LIST_ENTRY Wx86PluginList= {                // plugin list head
    &Wx86PluginList,
    &Wx86PluginList
    };

// Prototypes for plugin provider interfaces

typedef BOOLEAN (*WX86IDENTIFYPLUGIN)(PVOID PluginDll,
                                      WCHAR *FullFileName,
                                      BOOLEAN NativeToX86
                                      );

typedef BOOLEAN (*WX86THUNKEXPORT)(PVOID PluginDll,
                                   PCHAR ExportName,
                                   ULONG Ordinal,
                                   PVOID ExportAddress,
                                   PVOID *ExportThunk,
                                   BOOLEAN NativeToX86
                                   ); 

BOOLEAN
DllHasExports(
    PVOID DllBase
    )
{
   ULONG ExportSize;
   PIMAGE_EXPORT_DIRECTORY ExportDir;


   ExportDir = RtlImageDirectoryEntryToData(DllBase,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_EXPORT,
                                            &ExportSize
                                            );

   return ExportDir && ExportSize &&
          (ExportDir->NumberOfFunctions || ExportDir->NumberOfNames);

}

BOOLEAN
DllNameMatchesLdrEntry(
     PUNICODE_STRING BaseDllName,
     PUNICODE_STRING FullDllName,
     PLDR_DATA_TABLE_ENTRY LdrEntry,
     BOOLEAN ImporterX86
     )
/*++

Routine Description:

    Verifies that the LdrEntry matches the specifed dll.

Arguments:

    BaseDllName  - Unicode string describing Base Name of the Dll.

    FullDllName  - Unicode string describing full path Name of the Dll.
                   Set FullDllName length to zero for no full path matching.

    LdrEntry     - loader information for dll found by basename compare.

    ImporterX86  - TRUE if Importer is X86.


Return Value:

    TRUE if any of the following conditions are met.
       - FullDllName is same as LdrEntry FullDllName.
       - Machine Type is the same.
       - x86 importer AND LdrEntry is a Wx86 Risc thunk dll.

--*/

{
    USHORT MachineType;
    BOOLEAN FullNameMatches = FALSE;
    PIMAGE_NT_HEADERS NtHeaders;


    //
    // The Base name must match.
    //

    if (!RtlEqualUnicodeString(BaseDllName, &LdrEntry->BaseDllName, TRUE)) {
        return FALSE;
        }


    if (!FullDllName->Length ||
        (FullDllName->Length &&
         RtlEqualUnicodeString(FullDllName, &LdrEntry->FullDllName, TRUE)))
       {
        FullNameMatches = TRUE;
        }

    //
    // if we are not checking Machine Type, return based
    // on FullName matching.
    // LDRP_WX86_IGNORE_MACHINETYPE is used for images with no exports
    // LDRP_WX86_PLUGIN is used when there's a plug provider that will
    // dynamically generate thunks when GetProcAddress() is called.
    //

    if ((LdrEntry->Flags & LDRP_WX86_IGNORE_MACHINETYPE) ||
        (LdrEntry->Flags & LDRP_WX86_PLUGIN) ) {
        return FullNameMatches;
        }

    NtHeaders = RtlImageNtHeader(LdrEntry->DllBase);
    MachineType = NtHeaders->FileHeader.Machine;

    if (ImporterX86) {
        if (MachineType == IMAGE_FILE_MACHINE_I386) {
            return FullNameMatches;
            }

            //
            // Allow cross platform linking for x86 to risc Wx86 thunk
            // dlls. All risc Wx86 Thunk dlls are marked as Wx86 Thunk dlls
            // in the ntheader.
            //
        if (FullNameMatches) {
            return (NtHeaders->OptionalHeader.DllCharacteristics
                    & IMAGE_DLLCHARACTERISTICS_X86_THUNK) != 0;
            }

            //
            // The full name doesn't match, we can still allow matches
            // for loads which were redirected from system32 to wx86
            // system dir (See LdrpWx86MapDll).
            //
        else {

            UNICODE_STRING PathPart;

            PathPart = LdrEntry->FullDllName;
            PathPart.Length = LdrEntry->FullDllName.Length - LdrEntry->BaseDllName.Length - sizeof(WCHAR);
            if (!RtlEqualUnicodeString(&PathPart, &Wx86SystemDir, TRUE)) {
                return FALSE;
                }

            PathPart = *FullDllName;
            PathPart.Length = FullDllName->Length - BaseDllName->Length - sizeof(WCHAR);
            if (!RtlEqualUnicodeString(&PathPart, &LdrpKnownDllPath, TRUE)) {
                return FALSE;
                }

            RtlCopyUnicodeString(FullDllName, &LdrEntry->FullDllName);

            return TRUE;
            }

        }



    //
    // Importer is Risc.
    //

    if (IsNativeMachineType(MachineType))
      {
        return FullNameMatches;
        }

    return FALSE;
}



BOOLEAN
SearchWx86Dll(
    IN  PWSTR DllPath,
    IN  PUNICODE_STRING BaseName,
    OUT PUNICODE_STRING FileName,
    OUT PWSTR *pNextDllPath
    )

/*++

Routine Description:

    Search the path for a dll, based on Wx86 altered search path rules.

Arguments:

    DllPath - search path to use.

    BaseName - Name of dll to search for.

    FileName - addr of Unicode string to fill in the found dll path name.

    pNextDllPath - addr to fill in next path component to be searched.

Return Value:

--*/

{
    PWCHAR pwch;
    ULONG Length;

    //
    // formulate the name for each path component,
    // and see if it exists.
    //

    Length = BaseName->Length + 2*sizeof(WCHAR);

    do {
        pwch = FileName->Buffer;

        //
        // copy up till next semicolon
        //
        FileName->Length = 0;

        while (*DllPath) {
            if (FileName->MaximumLength <= FileName->Length + Length) {
                return FALSE;
                }


            if (*DllPath == (WCHAR)';') {
                DllPath++;
                break;
                }

            *pwch++ = *DllPath++;
            FileName->Length += sizeof(WCHAR);
            }


        //
        //  if we got a path component, append the basename
        //  and return if it exists.
        //

        if (FileName->Length) {
            if (*(pwch -1) !=  L'\\') {
                *pwch = L'\\';
                FileName->Length += sizeof(WCHAR);
                }
            }

        RtlAppendUnicodeStringToString(FileName, BaseName);

        if (RtlDoesFileExists_U(FileName->Buffer)) {
            *pNextDllPath = DllPath;
            return TRUE;
            }

      } while (*DllPath);

    *pNextDllPath = DllPath;

    return FALSE;
}



BOOLEAN
LdrpWx86DllMapNotify(
     PVOID DllBase,
     BOOLEAN Mapped
     )
/*++

Routine Description:

    Invoked by the nt loader immediately after an x86 Dll is Mapped or
    unmapped from memory. This routine is not called at a point where
    it is safe to load other dlls. That work should be deferred till
    as late as possible before the x86 code is actually going to be executed.

Arguments:

    DllBase - Base address of the DLL

    Mapped - TRUE if the DLL is being mapped, FALSE if it is being unmapped.

Return Value:

    FALSE on failure, TRUE on success.

--*/
{
    return Wx86DllMapNotify && (*Wx86DllMapNotify)(DllBase, Mapped);
}




NTSTATUS
LdrpWx86MapDll(
    IN PWSTR DllPath OPTIONAL,
    IN PULONG DllCharacteristics OPTIONAL,
    IN BOOLEAN Wx86KnownDll,
    IN BOOLEAN StaticLink,
    OUT PUNICODE_STRING DllName,
    OUT PLDR_DATA_TABLE_ENTRY *pEntry,
    OUT ULONG_PTR *pViewSize,
    OUT HANDLE *pSection
    )
/*++

Routine Description:

    Resolves dll name, creates image section and maps image into memory.


Arguments:

    DllPath - Supplies the DLL search path.

    DllCharacteristics - Supplies an optional DLL characteristics flag,
        that if specified is used to match against the dll being loaded.
        (IMAGE_FILE_HEADER Characteristics)

    Wx86KnownDll - if true, Importer is x86.

    StaticLink - TRUE, if static link and not dynamic.

    DllName - Name of Dll to map.

    pEntry    - returns filled LdrEntry allocated off of the process heap.

    pViewSize - returns the View Size of mapped image.

    pSection  - returns the section handle.



Return Value:

    Status

--*/

{
    NTSTATUS st;
    NTSTATUS stMapSection;
    PWCHAR pwch;
    PVOID  ViewBase = NULL;
    PTEB Teb = NtCurrentTeb();
    PIMAGE_NT_HEADERS NtHeaders;
    PVOID ArbitraryUserPointer;
    PLDR_DATA_TABLE_ENTRY Entry;
    BOOLEAN Wx86DirOverride=FALSE;
    BOOLEAN Wx86DirUndone=FALSE;
    BOOLEAN ContainsNoExports = FALSE;
    UNICODE_STRING NameUnicode;
    UNICODE_STRING FreeUnicode;
    UNICODE_STRING FullNameUnicode;
    UNICODE_STRING BaseNameUnicode;
    UNICODE_STRING FullName;
    HANDLE MismatchSection;
    WCHAR FullNameBuffer[((DOS_MAX_PATH_LENGTH*2+20)+sizeof(UNICODE_NULL))/2];
    BOOLEAN Wx86Plugin;
    BOOLEAN MismatchEncountered;
    UNICODE_STRING ThunkDllName;
    SECTION_IMAGE_INFORMATION SectionInfo;

    UNICODE_STRING  LocalDllName;
    PUNICODE_STRING pLocalDllName;
    PWSTR           DllSearchPath;
    PUNICODE_STRING ForcedDllName = NULL;
    PUNICODE_STRING ForcedDllPath = NULL;

    MismatchEncountered = FALSE;
    FullName.Buffer = NULL;
    Wx86Plugin = FALSE;
    Entry = NULL;

    FullNameUnicode.Buffer = FullNameBuffer;
    FullNameUnicode.MaximumLength = sizeof(FullNameBuffer);
    FullNameUnicode.Length = 0;


    //
    // If DllPath is not supplied, use the loader's default path
    //

    if (!DllPath) {
        DllPath = LdrpDefaultPath.Buffer;
        }

    //
    // DllPath can be ignored if:
    //    1) DllPath is the null string
    // or 2) DllName contains a hard coded path
    //

    if (!*DllPath ||
        RtlDetermineDosPathNameType_U(DllName->Buffer) != RtlPathTypeRelative)
      {
        DllPath = NULL;
        }



    //
    // Alloc a chunk of memory to use in constructing the full
    // dll name from the path and file name. Note that because
    // a path component may contain relative references, it may
    // exceed MAX_PATH.
    //

    FreeUnicode.Length = 0;
    FreeUnicode.MaximumLength = (DOS_MAX_PATH_LENGTH*2+20) + sizeof(UNICODE_NULL);
    if (DllPath) {
        FreeUnicode.MaximumLength += wcslen(DllPath) * sizeof(WCHAR);
        }

    FreeUnicode.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                         MAKE_TAG( TEMP_TAG ),
                                         FreeUnicode.MaximumLength
                                         );
    if (!FreeUnicode.Buffer) {
        return STATUS_NO_MEMORY;
        }


    *pSection = NULL;
    MismatchSection = NULL;

    pLocalDllName = &LocalDllName;
    DllSearchPath = DllPath;

    //
    // If we are looking for an x86 dll and it's base name is one of the
    // Wx86 thunk dlls, map the Wx86 thunk dll instead
    //
    //
    // X86 importers: force Wx86 system32 path before NtSystem32 path.
    //

    LocalDllName.Buffer = wcsrchr(DllName->Buffer, L'\\');       
    LocalDllName.Buffer = LocalDllName.Buffer ?  &LocalDllName.Buffer[1] : DllName->Buffer;
    
    LocalDllName.Length = wcslen(LocalDllName.Buffer) * sizeof(WCHAR);
    LocalDllName.MaximumLength = LocalDllName.Length + sizeof(WCHAR);

    pLocalDllName = DllName;

    if (Wx86KnownDll) {
        // Looking for x86 dll ...

        if (Wx86KnownDllName(&LocalDllName, &ThunkDllName)) {
            // It's a thunked dll. Redirect to system32\<thunk dll>
            ForcedDllPath = &LdrpKnownDllPath;
            ForcedDllName = &ThunkDllName;       // use the thunked dll
        
        } else if (Wx86KnownRedistDll(&LocalDllName)) {
            // It's a redistributed x86 dll. Redirect to sys32x86\<dll name>
            ForcedDllPath = &Wx86SystemDir;
            ForcedDllName = &LocalDllName;       // use the input dll name
        
        }
    }

    // if ForcedDllPath is not NULL, disable path searching and retries.
    
    if (ForcedDllPath) {
        if (ShowSnaps) {
            DbgPrint("LDRWX86: %s %wZ - force load from %wZ\\%wZ\n",
                    Wx86KnownDll? "x86" : "native", DllName, ForcedDllPath, ForcedDllName );
        }

        RtlCopyUnicodeString(&FreeUnicode, ForcedDllPath);
        FreeUnicode.Buffer[FreeUnicode.Length>>1] = L'\\';
        FreeUnicode.Length += sizeof(WCHAR);
        RtlAppendUnicodeStringToString(&FreeUnicode, ForcedDllName);
        pLocalDllName = &FreeUnicode;
        DllSearchPath = NULL;               // disable path searching
        Wx86DirOverride = TRUE;             // don't retry the load
    }

    // Loop as long as there are more directories in DllSearchPath
    // or at least once if path searching is disabled.
    
    while (TRUE) {

        // Find the next occurance of the dll in the DllPath directories.
        // If no DllPath then use DllName as provided

        if (DllSearchPath) {
            if (!SearchWx86Dll(DllSearchPath,
                               pLocalDllName,
                               &FreeUnicode,
                               &DllSearchPath
                               ))
              {
               st = STATUS_DLL_NOT_FOUND;
               break;
               }

            pwch = FreeUnicode.Buffer;
            }
        else {
            pwch = pLocalDllName->Buffer;
            }

        //
        // Setup FullNameUnicode, BaseNameUnicode strings
        //

        FullNameUnicode.Length = (USHORT)RtlGetFullPathName_U(
                                               pwch,
                                               FullNameUnicode.MaximumLength,
                                               FullNameUnicode.Buffer,
                                               &BaseNameUnicode.Buffer
                                               );

        if (!FullNameUnicode.Length ||
            FullNameUnicode.Length >= FullNameUnicode.MaximumLength)
          {
            st = STATUS_OBJECT_PATH_SYNTAX_BAD;
            break;
            }


        BaseNameUnicode.Length = FullNameUnicode.Length -
                                 (USHORT)((ULONG_PTR)BaseNameUnicode.Buffer -
                                          (ULONG_PTR)FullNameUnicode.Buffer);

        BaseNameUnicode.MaximumLength = BaseNameUnicode.Length + sizeof(WCHAR);

        if (DllSearchPath && Wx86KnownDll && !Wx86DirOverride) {
            NameUnicode = FullNameUnicode;
            NameUnicode.Length -= BaseNameUnicode.Length + sizeof(WCHAR);
            if (RtlEqualUnicodeString(&NameUnicode, &LdrpKnownDllPath, TRUE)) {
                RtlCopyUnicodeString(&FreeUnicode, &Wx86SystemDir);
                FreeUnicode.Buffer[FreeUnicode.Length >> 1] = L'\\';
                FreeUnicode.Length += sizeof(WCHAR);
                pwch = &FullNameUnicode.Buffer[FreeUnicode.Length >> 1];
                RtlAppendUnicodeStringToString(&FreeUnicode, &BaseNameUnicode);
                Wx86DirOverride = TRUE;

                if (RtlDoesFileExists_U(FreeUnicode.Buffer)) {
                    RtlCopyUnicodeString(&FullNameUnicode, &FreeUnicode);
                    BaseNameUnicode.Buffer = pwch;
                    }
                else {
                    Wx86DirUndone = TRUE;
                    }
                }
            }


RetryWx86SystemDir:



        //
        // Create the image section.
        //

        if (!RtlDosPathNameToNtPathName_U(FullNameUnicode.Buffer,
                                          &NameUnicode,
                                          NULL,
                                          NULL
                                          ))
           {
            st = STATUS_OBJECT_PATH_SYNTAX_BAD;
            break;
            }

        if (ShowSnaps) {
            DbgPrint("LDR: Loading (%s) %wZ\n",
                     StaticLink ? "STATIC" : "DYNAMIC",
                     &FullNameUnicode
                     );
            }

        st = LdrpCreateDllSection(&NameUnicode,
                                  NULL,
                                  pLocalDllName,
                                  DllCharacteristics,
                                  pSection
                                  );

        RtlFreeHeap(RtlProcessHeap(), 0, NameUnicode.Buffer);

        if (!NT_SUCCESS(st)) {
            break;
            }

        //
        // Query the section info to discover its attributes
        //
        st = NtQuerySection(*pSection,
                            SectionImageInformation,
                            &SectionInfo,
                            sizeof(SectionInfo),
                            NULL);
        if (!NT_SUCCESS(st)) {
            break;
        }


            //
            // MachineType is native type, allow:
            // - if risc importer
            // - if Wx86 thunk dlls
            // - if image contains no exports,
            //      since Wx86 thunk dll not required (richedt32.dll).
            //

        if (IsNativeMachineType(SectionInfo.Machine)) {

            if (!Wx86KnownDll ||
                (SectionInfo.DllCharacteristics
                   & IMAGE_DLLCHARACTERISTICS_X86_THUNK)) {
                break;
                }


            if (!SectionInfo.ImageContainsCode) {
                ContainsNoExports = TRUE;
                break;
                }

            }


            //
            // Machine Type is not native, allow:
            // - if x86 importer, and machine type is x86
            // - if image doesn't contain code,
            //      since its probably a resource\data dll only.
            //

        else {

            if (SectionInfo.Machine == IMAGE_FILE_MACHINE_I386) {
                if (Wx86KnownDll) {
                    break;
                    }
                }


            if (!SectionInfo.ImageContainsCode) {
                ContainsNoExports = TRUE;
                break;
                }
            }


        //
        // Failure because of an image machine mismatch.
        // Save the mapped dll information for plugin processing or a hard
        // error in case we can't find an image with matching machine type.
        //

        if (!MismatchEncountered) {
            FullName.MaximumLength = FullNameUnicode.MaximumLength;
            FullName.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                              MAKE_TAG(TEMP_TAG),
                                              FullName.MaximumLength
                                              );
            if (!FullName.Buffer) {
                st = STATUS_NO_MEMORY;
                break;
                }
            st = NtDuplicateObject(NtCurrentProcess(),
                                   *pSection,
                                   NtCurrentProcess(),
                                   &MismatchSection,
                                   0,
                                   FALSE,
                                   DUPLICATE_SAME_ACCESS);
            if (!NT_SUCCESS(st)) {
                break;
                }
            RtlCopyUnicodeString(&FullName, &FullNameUnicode);
            st = STATUS_INVALID_IMAGE_FORMAT;
            MismatchEncountered = TRUE;
            }

        NtClose(*pSection);
        *pSection = NULL;

        //
        // If we previously overid system32 with wx86 sys dir
        // undo the override by retrying with system32.
        //

        if (DllSearchPath) {
            if (Wx86DirOverride && !Wx86DirUndone) {
                RtlCopyUnicodeString(&FullNameUnicode, &LdrpKnownDllPath);
                FullNameUnicode.Buffer[FullNameUnicode.Length >> 1] = L'\\';
                FullNameUnicode.Length += sizeof(WCHAR);
                pwch = &FullNameUnicode.Buffer[FullNameUnicode.Length >> 1];
                RtlAppendUnicodeStringToString(&FullNameUnicode, &BaseNameUnicode);
                BaseNameUnicode.Buffer = pwch;
                Wx86DirUndone = TRUE;
                goto RetryWx86SystemDir;
                }
            }

        //
        // if x86 Importer, with hardcoded path to system32, retry with
        // the Wx86 system directory. This is because some apps, erroneously
        // derive the system32 path by appending system32 to WinDir, instead
        // of calling GetSystemDir().
        //

        else if (Wx86KnownDll && !Wx86DirOverride) {
            NameUnicode = FullNameUnicode;
            NameUnicode.Length -= BaseNameUnicode.Length + sizeof(WCHAR);
            if (RtlEqualUnicodeString(&NameUnicode, &LdrpKnownDllPath, TRUE)) {
                RtlCopyUnicodeString(&FreeUnicode, &BaseNameUnicode);
                RtlCopyUnicodeString(&FullNameUnicode, &Wx86SystemDir);
                FullNameUnicode.Buffer[FullNameUnicode.Length >> 1] = L'\\';
                FullNameUnicode.Length += sizeof(WCHAR);
                BaseNameUnicode.Buffer = &FullNameUnicode.Buffer[FullNameUnicode.Length >> 1];
                RtlAppendUnicodeStringToString(&FullNameUnicode, &FreeUnicode);
                Wx86DirUndone = Wx86DirOverride = TRUE;
                goto RetryWx86SystemDir;
                }
            }



        //
        // Try further down the path, for a matching machine type
        // if no more path to search, we fail.
        //

        if (!DllSearchPath || !*DllSearchPath) {
            break;
            }

        } // while (TRUE)



    //
    // Cleanup the temporary allocated buffers.
    //

    if (FreeUnicode.Buffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, FreeUnicode.Buffer);
        }

    if (MismatchEncountered && !StaticLink) {
        if (NT_SUCCESS(st)) {
            //
            // Mismatch was encountered, but a DLL further along the path
            // did match.  Cleanup the mismatch stuff.
            //
            NtClose(MismatchSection);
            MismatchSection = NULL;
            MismatchEncountered = FALSE;
        } else {
            //
            // No matching DLL found.  Revert back to the image that gave the
            // image type mismatch.
            //
            if (pSection) NtClose(*pSection);
            *pSection = MismatchSection;
            MismatchSection = NULL;
            st = STATUS_SUCCESS;
        }
    }

    if (NT_SUCCESS(st)) {
        //
        // Map the section into memory
        //
        *pViewSize = 0;
        ViewBase = NULL;
        ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
        Teb->NtTib.ArbitraryUserPointer = FullNameUnicode.Buffer;
        st = NtMapViewOfSection(*pSection,
                                NtCurrentProcess(),
                                &ViewBase,
                                0L,
                                0L,
                                NULL,
                                pViewSize,
                                ViewShare,
                                0L,
                                PAGE_READWRITE
                                );
        // Save this return code because it may be STATUS_IMAGE_NOT_AT_BASE.
        // This status must be returned so the dll will be relocated by the caller.
        stMapSection = st;
        Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;
        if (!NT_SUCCESS(st)) {
            goto LWMDGiveUp;
            }

        NtHeaders = RtlImageNtHeader(ViewBase);
        if (NtHeaders == NULL) {
            st = STATUS_INVALID_IMAGE_FORMAT;
            goto LWMDGiveUp;
            }

#if defined (_ALPHA_)
        //
        // Fix up non alpha compatible images
        //

        if (NtHeaders->OptionalHeader.SectionAlignment < PAGE_SIZE) {
            NTSTATUS    formatStatus = LdrpWx86FormatVirtualImage((PIMAGE_NT_HEADERS32)NtHeaders, ViewBase);
            if (!NT_SUCCESS(formatStatus)) {
                st = formatStatus;            
                goto LWMDGiveUp;
            }
        }
#endif
    }

    if (MismatchEncountered) {

#if defined (_ALPHA_)

       if (!StaticLink) {

           // Encountered a mismatch image type and didn't find another
           // that matches.  Attempt to find a plugin provider dll that
           // can thunk this interface.
           // If there are no errors in plugin processing:
           //      st = STATUS_SUCCESS if plugin was thunked
           //      st = STATUS_IMAGE_MACHINE_TYPE_MISMATCH if not thunked
           // Otherwise st will contain the error from the plugin code

           PLDR_DATA_TABLE_ENTRY Temp;

           // temporarily insert the image in the loaded-module-list

           Temp = LdrpAllocateDataTableEntry(ViewBase);
           if (!Temp) {
               st = STATUS_NO_MEMORY;
           } else {
               Temp->Flags = 0;
               Temp->LoadCount = 0;
               Temp->FullDllName = FullNameUnicode;
               Temp->BaseDllName = BaseNameUnicode;
               Temp->EntryPoint = LdrpFetchAddressOfEntryPoint(Temp->DllBase);
               LdrpInsertMemoryTableEntry(Temp);

               // Determine if this dll can be thunked using a plugin

               st = Wx86IdentifyPlugin(ViewBase, &FullNameUnicode );

               // remove the image from the loaded-module-list.
               // It will be added again in the main-line path.

               RemoveEntryList(&Temp->InLoadOrderLinks);
               RemoveEntryList(&Temp->InMemoryOrderLinks);
               RemoveEntryList(&Temp->HashLinks);
               RtlFreeHeap(RtlProcessHeap(), 0, Temp);
               Temp = NULL;

               if (ShowSnaps) {
                   PCHAR Action;

                   if (st == STATUS_SUCCESS) {
                       Action = "Loaded";
                   } else if (st == STATUS_IMAGE_MACHINE_TYPE_MISMATCH) {
                       Action = "Unsupported";
                   } else {
                       Action = "Failed";
                   }

                   DbgPrint("LDRWx86: Plugin: %wZ %s.\n",
                       &FullNameUnicode, Action
                       );
               }
               
               // If plug provider supports this dll flag it and restore
               // status to the return code from original NtMapViewOfSection.
               // Howver, don't revert to STATUS_IMAGE_MACHINE_TYPE_MISMATCH -
               // that's what we just fixed up.
               
               if (st == STATUS_SUCCESS) {
                   Wx86Plugin = TRUE;
                   if (stMapSection != STATUS_IMAGE_MACHINE_TYPE_MISMATCH) {
                   st = stMapSection;
                   }
               }
           }
       }
#endif
       if (st == STATUS_IMAGE_MACHINE_TYPE_MISMATCH) {

           // Encountered a mismatch image type.
           // Raise a Hard Error for the machine mismatch.

           if (ShowSnaps) {
               DbgPrint("Wx86 image type mismatch loading %ws (expected %s)\n",
                       FullName.Buffer,
                       Wx86KnownDll? "x86" : "RISC"
                       );
               }

           if ( NtHeaders->OptionalHeader.MajorSubsystemVersion <= 3 ) {

               ULONG_PTR ErrorParameters[2];
               ULONG ErrorResponse;

               ErrorResponse = ResponseOk;

               ErrorParameters[0] = (ULONG_PTR)&FullName;

               NtRaiseHardError(STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE,
                                1,
                                1,
                                ErrorParameters,
                                OptionOk,
                                &ErrorResponse
                                );

           }

           st = STATUS_INVALID_IMAGE_FORMAT;
       }
    }

    //
    // if we were successfull,
    // allocate and fill FullDllName, BaseDllName for the caller.
    //

    if (NT_SUCCESS(st)) {
        PUNICODE_STRING Unicode;


        if (st == STATUS_IMAGE_MACHINE_TYPE_MISMATCH) {
            st = NtHeaders->OptionalHeader.ImageBase == (ULONG_PTR)ViewBase
                    ? STATUS_SUCCESS : STATUS_IMAGE_NOT_AT_BASE;
            }

        *pEntry = Entry = LdrpAllocateDataTableEntry(ViewBase);
        if (!Entry) {
            st = STATUS_NO_MEMORY;
            goto LWMDGiveUp;
            }

        //
        // Fil in loader entry
        //

        Entry->Flags = StaticLink ? LDRP_STATIC_LINK : 0;
        if (ContainsNoExports) {
            Entry->Flags |= LDRP_WX86_IGNORE_MACHINETYPE;
            }
        if (Wx86Plugin) {
            Entry->Flags |= LDRP_WX86_PLUGIN;
        }

        Entry->LoadCount = 0;
        Entry->EntryPoint = LdrpFetchAddressOfEntryPoint(ViewBase);
        Entry->FullDllName.Buffer = NULL;
        Entry->BaseDllName.Buffer = NULL;


        //
        // Copy in the full dll name
        //

        Unicode = &Entry->FullDllName;
        Unicode->Length = FullNameUnicode.Length;
        Unicode->MaximumLength = Unicode->Length + sizeof(UNICODE_NULL);
        Unicode->Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                          MAKE_TAG( LDR_TAG ),
                                          Unicode->MaximumLength
                                          );
        if (!Unicode->Buffer) {
            st = STATUS_NO_MEMORY;
            goto LWMDGiveUp;
            }

        RtlCopyMemory(Unicode->Buffer,
                      FullNameUnicode.Buffer,
                      Unicode->MaximumLength
                      );


        //
        // Copy in the basename
        //

        Unicode = &Entry->BaseDllName;
        Unicode->Length = BaseNameUnicode.Length;
        Unicode->MaximumLength = Unicode->Length + sizeof(UNICODE_NULL);
        Unicode->Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                              MAKE_TAG( LDR_TAG ),
                                              Unicode->MaximumLength
                                              );

        if (Unicode->Buffer) {
            RtlCopyMemory(Unicode->Buffer,
                          BaseNameUnicode.Buffer,
                          Unicode->MaximumLength
                          );
            }
        else {
            st = STATUS_NO_MEMORY;
            }


        }


LWMDGiveUp:

    // cleanup items saved from mismatched images

    if (MismatchSection) {
        NtClose(MismatchSection);
    }

    if (FullName.Buffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, FullName.Buffer);
    }

    //
    // If failure, cleanup mapview and section.
    //

    if (!NT_SUCCESS(st)) {

        if (ViewBase) {
            NtUnmapViewOfSection( NtCurrentProcess(), ViewBase);
            }

        if (*pSection) {
            NtClose(*pSection);
            }

        if (Entry) {
            if (Entry->FullDllName.Buffer) {
                RtlFreeHeap(RtlProcessHeap(), 0, Entry->FullDllName.Buffer);
                }

            if (Entry->BaseDllName.Buffer) {
                RtlFreeHeap(RtlProcessHeap(), 0, Entry->BaseDllName.Buffer);
                }

            RtlFreeHeap(RtlProcessHeap(), 0, Entry);

            *pEntry = NULL;

            }

        }
    return st;
}




PLDR_DATA_TABLE_ENTRY
LdrpWx86CheckForLoadedDll(
    IN PWSTR DllPath OPTIONAL,
    IN PUNICODE_STRING DllName,
    IN BOOLEAN Wx86KnownDll,
    OUT PUNICODE_STRING FullDllName
    )
/*++

Routine Description:

    Checks for loaded dlls, ensuring that duplicate module
    base names are resolved correctly

Arguments:

    DllPath      - optional search path used to locate the DLL.

    DllName      - Name of Dll

    Wx86KnownDll - if true, Importer is x86.

    FullDllName  - buffer to receive full path name,
                   assumes STATIC_UNICODE_BUFFER_LENGTH

Return Value:

    LdrEntry for dllname if found, otherwise NULL.

--*/
{
    NTSTATUS Status;
    int Index, Length;
    PWCHAR pwch;
    PLIST_ENTRY Head, Next;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    BOOLEAN HardCodedPath= FALSE;
    UNICODE_STRING BaseDllName;
    UNICODE_STRING FreeUnicode;
    UNICODE_STRING ThunkDllName;
    UNICODE_STRING LocalDllName;
    PUNICODE_STRING ForcedDllPath;
    PUNICODE_STRING ForcedDllName;

    //
    // If DllPath is not supplied, use the loader's default path
    //

    if (!DllPath) {
        DllPath = LdrpDefaultPath.Buffer;
        }

    //
    // DllPath can be ignored if:
    //    1) DllPath is unusable (=1)
    // or 2) DllPath is the null string
    // or 3) DllName contains a hard coded path
    //

    if ((UINT_PTR)DllPath == 1 || !*DllPath ||
        RtlDetermineDosPathNameType_U(DllName->Buffer) != RtlPathTypeRelative)
      {
        DllPath = NULL;
        }

    //
    // Alloc a chunk of memory to use in constructing the full
    // dll name from the path and file name. Note that because
    // a path component may contain relative references, it may
    // exceed MAX_PATH.
    //

    FreeUnicode.Length = 0;
    FreeUnicode.MaximumLength = (DOS_MAX_PATH_LENGTH*2+20)*sizeof(UNICODE_NULL);
    if (DllPath) {
        FreeUnicode.MaximumLength = wcslen(DllPath) * sizeof(WCHAR);
        }

    FreeUnicode.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                         MAKE_TAG( TEMP_TAG ),
                                         FreeUnicode.MaximumLength
                                         );
    if (!FreeUnicode.Buffer) {
        return NULL;
        }
 
    // Checking for known dlls before searching for the file improves performance
    // This code was moved from the do loop.  
    FullDllName->Length = 0;
    BaseDllName = *DllName;
   
    LocalDllName.Buffer = wcsrchr(BaseDllName.Buffer, L'\\');       
    LocalDllName.Buffer = LocalDllName.Buffer ?  &LocalDllName.Buffer[1] : BaseDllName.Buffer;

    LocalDllName.Length = wcslen(LocalDllName.Buffer) * sizeof(WCHAR);
    LocalDllName.MaximumLength = LocalDllName.Length + sizeof(WCHAR);
    ForcedDllPath = NULL;
    ForcedDllName = NULL;

    if (Wx86KnownDll) {                         // if the call is from x86 apps
        // Looking for x86 dll ...
        
        if (Wx86KnownDllName(&LocalDllName, &ThunkDllName)) {
            // It's a thunked dll. Redirect to system32\<thunk dll>
            ForcedDllPath = &LdrpKnownDllPath;
            ForcedDllName = &ThunkDllName;       // use the thunked dll
        
        } else if (Wx86KnownRedistDll(&LocalDllName)) {
            // It's a redistributed x86 dll. Redirect to sys32x86\<dll name>
            ForcedDllPath = &Wx86SystemDir;
            ForcedDllName = &LocalDllName;       // use the input dll name

        }
                
    } else if (Wx86KnownNativeDll(&LocalDllName)) {
       // It's a native known dll. Redirect to system32\<dll name>
       ForcedDllPath = &LdrpKnownDllPath;
       ForcedDllName = &LocalDllName;
    }

    // If ForcedDllPath is not null, then we either need to find the dll at the
    // predetermined path or we return failed.

    if (ForcedDllPath) {
        RtlCopyUnicodeString(FullDllName, ForcedDllPath );
        FullDllName->Buffer[FullDllName->Length/2] = L'\\';
        FullDllName->Length += sizeof(WCHAR);
        BaseDllName.Buffer = &FullDllName->Buffer[FullDllName->Length/2];
        BaseDllName.Length = ForcedDllName->Length;
        BaseDllName.MaximumLength = BaseDllName.Length + sizeof(WCHAR);
        RtlAppendUnicodeStringToString(FullDllName, ForcedDllName);
        
        Index = LDRP_COMPUTE_HASH_INDEX(BaseDllName.Buffer[0]);
        Head = &LdrpHashTable[Index];
        Next = Head->Flink;
        while ( Next != Head ) {
            LdrEntry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, HashLinks);
            if (DllNameMatchesLdrEntry(&BaseDllName,
                                       FullDllName,
                                       LdrEntry,
                                       Wx86KnownDll
                                       ))
            {
                goto FoundMatch;
            }
            Next = Next->Flink;
            // not a Wx86 Known dll or Wx86 Known dll is not loaded.
        }
      
        return NULL;
    }
    
    //
    // Search the DllPath, and verify that fullpath and machine type match.
    //

    do {
        if (DllPath) {
            if (!SearchWx86Dll(DllPath,
                               DllName,
                               &FreeUnicode,
                               &DllPath
                               ))
            {
               FullDllName->Length = 0;
               FullDllName->Buffer[0] = L'\0';
               BaseDllName = *DllName;

               Index = LDRP_COMPUTE_HASH_INDEX(BaseDllName.Buffer[0]);
               Head = &LdrpHashTable[Index];
               Next = Head->Flink;
               while ( Next != Head ) {
                   LdrEntry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, HashLinks);
                   if (DllNameMatchesLdrEntry(&BaseDllName,
                                              FullDllName,
                                              LdrEntry,
                                              Wx86KnownDll
                                              ))
                     {
                       goto FoundMatch;
                       }
                   Next = Next->Flink;
                   }

               break;
               }

            pwch = FreeUnicode.Buffer;
            }
        else {
            pwch = DllName->Buffer;
            }

       //
       // Form the fullpathname
       //

       FullDllName->Length = 0;
       Length = RtlGetFullPathName_U(pwch,
                                     FullDllName->MaximumLength,
                                     FullDllName->Buffer,
                                     &pwch  // receives address of file name portion
                                     );

       if (Length && Length < FullDllName->MaximumLength) {
           UNICODE_STRING PathPart;

           // Setup BaseDllName as the file name portion of FullDllName

           FullDllName->Length = (USHORT)Length;
           RtlInitUnicodeString(&BaseDllName, pwch);

           // Setup PathPart as the path portion of FullDllName

           PathPart = *FullDllName;
           PathPart.Length = (USHORT)((ULONG_PTR)BaseDllName.Buffer  -
                                      (ULONG_PTR)FullDllName->Buffer -
                                      sizeof(WCHAR)
                                      );
           //
           // Search Loader HashTable by BaseName.
           // For each matching basename, verify the full path and machine type.
           //

           Index = LDRP_COMPUTE_HASH_INDEX(BaseDllName.Buffer[0]);
           Head = &LdrpHashTable[Index];
           Next = Head->Flink;
           while ( Next != Head ) {
               LdrEntry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, HashLinks);
               if (DllNameMatchesLdrEntry(&BaseDllName,
                                          FullDllName,
                                          LdrEntry,
                                          Wx86KnownDll
                                          ))
                 {
                   goto FoundMatch;
                   }
               Next = Next->Flink;
               }


           //
           // TBD: need to add code from LdrpCheckForLoadedDll
        //
        // no names matched. This might be a long short name mismatch or
        // any kind of alias pathname. Deal with this by opening and mapping
        // full dll name and then repeat the scan this time checking for
        // timedatestamp matches
        //
           //

           }


    } while (DllPath && *DllPath);

    LdrEntry = NULL;

FoundMatch:
    RtlFreeHeap(RtlProcessHeap(), 0, FreeUnicode.Buffer);
    return LdrEntry;
}





VOID
LdrpWx86DllProcessDetach(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    )
/*++

Routine Description:

    Handles process detach for LdrUnload

Arguments:

    InitRoutine     - address of i386 dll entry point
    DllBase         - standard dll entry point parameters


Return Value:

    SUCCESS or reason

--*/
{
    PIMAGE_NT_HEADERS NtHeader;
    PDLL_INIT_ROUTINE InitRoutine;


    //
    // check for all x86dlls unloaded
    //

    NtHeader = RtlImageNtHeader(LdrDataTableEntry->DllBase);
    InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;
    if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED)) {
        if (ShowSnaps) {
            DbgPrint("WX86LDR: Calling deinit %lx\n", InitRoutine);
            }

        if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
            (Wx86DllEntryPoint)(InitRoutine,
                                LdrDataTableEntry->DllBase,
                                DLL_PROCESS_DETACH,
                                NULL
                                );

            }
        else {
            LdrpCallInitRoutine(InitRoutine,
                                LdrDataTableEntry->DllBase,
                                DLL_PROCESS_DETACH,
                                NULL);
            }

        }
}









NTSTATUS
LdrpRunWx86DllEntryPoint(
    IN PDLL_INIT_ROUTINE InitRoutine,
    OUT BOOLEAN *pInitStatus,
    IN PVOID DllBase,
    IN ULONG Reason,
    IN PCONTEXT Context
    )
/*++

Routine Description:

    Invokes the i386 emulator (wx86.dll) to run dll entry points.

Arguments:

    InitRoutine     - address of i386 dll entry point

    pInitStatus     - receives return code from the InitRoutine

    DllBase         - standard dll entry point parameters
    Reason
    Context


Return Value:

    SUCCESS or reason

--*/

{
    PIMAGE_NT_HEADERS NtHeader = NULL;
    BOOLEAN InitStatus;
    PWX86TIB Wx86Tib;

    NtHeader = RtlImageNtHeader(DllBase);
    if (NtHeader && NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {

        InitStatus  =  (Wx86DllEntryPoint)(InitRoutine,
                                           DllBase,
                                           Reason,
                                           Context
                                           );

        if (pInitStatus) {
            *pInitStatus = InitStatus;
            }

        return STATUS_SUCCESS;

        }

    return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
}



NTSTATUS
LoaderWx86Unload(
    VOID
    )
/*++

Routine Description:

   When Wx86.dll process detach routine executes, Wx86.dll invokes this routine
   so that the loader can cleanup Wx86 specific stuff.


Arguments:

   none

Return Value:

    SUCCESS or reason

--*/

{
     Wx86ProcessInit = NULL;
     Wx86DllMapNotify = NULL;
     Wx86DllEntryPoint = NULL;
     Wx86ProcessStartRoutine= NULL;
     Wx86ThreadStartRoutine= NULL;
     Wx86KnownDllName = NULL;
     Wx86KnownNativeDll = NULL;
     Wx86KnownRedistDll = NULL;
     Wx86OnTheFly=FALSE;


     if (Wx86SystemDir.Buffer) {
         RtlFreeHeap(RtlProcessHeap(), 0, Wx86SystemDir.Buffer);
         Wx86SystemDir.Buffer = NULL;
         }

     return STATUS_SUCCESS;

}






NTSTATUS
LdrpLoadWx86Dll(
    PCONTEXT Context
    )
/*++

Routine Description:

   Loads in the i386 emulator (wx86.dll) and performs process initialization
   for wx86 specific ldr code.

Arguments:

   Context, initial process context,
   if hibit set this is ofly init after process initialzation is complete
   (Wx86 on the fly), and the PCONTEXT == Wx86DllHandle | 0x80000000

Return Value:

   SUCCESS or reason

--*/
{
    NTSTATUS st;
    ULONG Length;
    PVOID DllHandle;
    ANSI_STRING ProcName;
    UNICODE_STRING DllName;
    WCHAR Buffer[STATIC_UNICODE_BUFFER_LENGTH];

    //
    // Retrieve the dll handle for the ofly case
    //

    DllHandle = (PVOID)((UINT_PTR)Context & ~0x80000000);

    if (DllHandle != Context) {
        Context = NULL;
        }
    else {
        DllHandle = NULL;
        }

    //
    // initialize Wx86SystemDir
    //

    RtlInitUnicodeString( &NtSystemRoot, USER_SHARED_DATA->NtSystemRoot );
    Wx86SystemDir.MaximumLength = NtSystemRoot.Length + sizeof(Wx86Dir);
    Wx86SystemDir.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                           MAKE_TAG( LDR_TAG ),
                                           Wx86SystemDir.MaximumLength
                                           );
    if (!Wx86SystemDir.Buffer) {
        st = STATUS_NO_MEMORY;
        goto LWx86DllError;
        }

    RtlCopyUnicodeString(&Wx86SystemDir, &NtSystemRoot);
    st = RtlAppendUnicodeToString(&Wx86SystemDir, Wx86Dir);
    if (!NT_SUCCESS(st)) {
        goto LWx86DllError;
        }



    //
    // Load Wx86.dll, wintdll.dll. This must be done before the app binary is
    // snapped to ensure wx86.dll is ready for emulation.
    //

    if (!DllHandle) {
        DllName.Buffer = Buffer;
        DllName.MaximumLength = sizeof(Buffer);
        RtlCopyUnicodeString(&DllName, &LdrpKnownDllPath);
        DllName.Buffer[DllName.Length / sizeof(WCHAR)] = L'\\';
        DllName.Length += sizeof(WCHAR);
        RtlAppendUnicodeToString(&DllName, L"wx86.dll");

        st = LdrpLoadDll(NULL, NULL, &DllName, &DllHandle, TRUE);
        if (!NT_SUCCESS(st)) {
            goto LWx86DllError;
            }
        }

    //
    // Get fn address from Wx86.dll
    //

    RtlInitAnsiString (&ProcName,"Wx86KnownDllName");
    st = LdrGetProcedureAddress(DllHandle,
                                &ProcName,
                                0,
                                (PVOID *)&Wx86KnownDllName
                                );
    if (!NT_SUCCESS(st)) {
        goto LWx86DllError;
        }


    RtlInitAnsiString (&ProcName,"Wx86KnownNativeDll");
    st = LdrGetProcedureAddress(DllHandle,
                                &ProcName,
                                0,
                                (PVOID *)&Wx86KnownNativeDll
                                );
    if (!NT_SUCCESS(st)) {
        goto LWx86DllError;
        }

    RtlInitAnsiString (&ProcName,"Wx86KnownRedistDll");
    st = LdrGetProcedureAddress(DllHandle,
                                &ProcName,
                                0,
                                (PVOID *)&Wx86KnownRedistDll
                                );
    if (!NT_SUCCESS(st)) {
        goto LWx86DllError;
        }

    RtlInitAnsiString (&ProcName,"RunWx86DllEntryPoint");
    st = LdrGetProcedureAddress(DllHandle,
                                &ProcName,
                                0,
                                (PVOID *)&Wx86DllEntryPoint
                                );
    if (!NT_SUCCESS(st)) {
        goto LWx86DllError;
        }

    RtlInitAnsiString (&ProcName,"Wx86ThreadStartRoutine");
    st = LdrGetProcedureAddress(DllHandle,
                                &ProcName,
                                0,
                                (PVOID *)&Wx86ThreadStartRoutine
                                );
    if (!NT_SUCCESS(st)) {
        goto LWx86DllError;
        }

    RtlInitAnsiString (&ProcName,"Wx86ProcessStartRoutine");
    st = LdrGetProcedureAddress(DllHandle,
                                &ProcName,
                                0,
                                (PVOID *)&Wx86ProcessStartRoutine
                                );
    if (!NT_SUCCESS(st)) {
        goto LWx86DllError;
        }

    RtlInitAnsiString (&ProcName,"Wx86DllMapNotify");
    st = LdrGetProcedureAddress(DllHandle,
                                &ProcName,
                                0,
                                (PVOID *)&Wx86DllMapNotify
                                );
    if (!NT_SUCCESS(st)) {
        goto LWx86DllError;
        }


    RtlInitAnsiString (&ProcName,"Wx86ProcessInit");
    st = LdrGetProcedureAddress(DllHandle,
                                &ProcName,
                                0,
                                (PVOID *)&Wx86ProcessInit
                                );
    if (!NT_SUCCESS(st)) {
        goto LWx86DllError;
        }


    if (Context) {
        st = LdrpInitWx86(NtCurrentTeb()->Vdm, Context, FALSE);
        if (!NT_SUCCESS(st)) {
            goto LWx86DllError;
            }
        }
    else {
        Wx86OnTheFly=TRUE;
        }



    if (!(*Wx86ProcessInit)(LoaderWx86Unload, Wx86OnTheFly)) {
        st = STATUS_ENTRYPOINT_NOT_FOUND;
        }


LWx86DllError:

    if (!NT_SUCCESS(st)) {
        // If the load failed make sure we clean-up.
        LoaderWx86Unload();
        }

    return st;
}



NTSTATUS
LdrpInitWx86(
    PWX86TIB Wx86Tib,
    PCONTEXT Context,
    BOOLEAN NewThread
    )
/*++

Routine Description:

    Per thread wx86 specific initialization.

Arguments:

Return Value:

    SUCCESS or reason

--*/
{
    PTEB Teb;
    MEMORY_BASIC_INFORMATION MemBasicInfo;

    if (Wx86Tib != Wx86CurrentTib()) {
        return STATUS_APP_INIT_FAILURE;
        }

    if (ShowSnaps) {
        DbgPrint("LDRWX86: %x Pc %x Base %x Limit %x DeallocationStack %x\n",
                  Wx86Tib,
                  Wx86Tib->InitialPc,
                  Wx86Tib->StackBase,
                  Wx86Tib->StackLimit,
                  Wx86Tib->DeallocationStack
                  );
        }


    if (Wx86Tib->EmulateInitialPc) {
        Wx86Tib->EmulateInitialPc = FALSE;

        if (NewThread) {

#if defined(_MIPS_)
            Context->XIntA0 = (LONG)Wx86ThreadStartRoutine;
#elif defined(_ALPHA_)
            Context->IntA0 = (ULONG_PTR)Wx86ThreadStartRoutine;
#elif defined(_PPC_)
            Context->Gpr3  = (ULONG)Wx86ThreadStartRoutine;
#elif defined(_IA64_)
            Context->IntS0 = Context->StIIP = (ULONG_PTR)Wx86ThreadStartRoutine;
#else
#error Need to set instruction pointer to Wx86ThreadStartRoutine
#endif
            }
        else {

#if defined(_MIPS_)
            Context->XIntA1 = (LONG)Wx86ProcessStartRoutine;
#elif defined(_ALPHA_)
            Context->IntA0 = (ULONG_PTR)Wx86ProcessStartRoutine;
#elif defined(_PPC_)
            Context->Gpr3  = (ULONG)Wx86ProcessStartRoutine;
#elif defined(_IA64_)
            Context->IntS0 = Context->StIIP = (ULONG_PTR)Wx86ProcessStartRoutine;
#else
#error Need to set instruction pointer to Wx86ProcessStartRoutine
#endif

            }

        }


    return STATUS_SUCCESS;
}
#endif



#if defined (_ALPHA_) || defined(BUILD_WOW6432)


   // From mi\mi.h:
#define MI_ROUND_TO_SIZE(LENGTH,ALIGNMENT)     \
                    (((LENGTH) + ((ALIGNMENT) - 1)) & ~((ALIGNMENT) - 1))

NTSTATUS
Wx86SetRelocatedSharedProtection (
    IN PVOID Base,
    IN BOOLEAN Reset
    )

/*++

Routine Description:


    This function loops thru the images sections/objects, setting
    all relocated shared sections/objects marked r/o to r/w. It also resets the
    original section/object protections.

Arguments:

    Base - Base of image.

    Reset - If TRUE, reset section/object protection to original
            protection described by the section/object headers.
            If FALSE, then set all sections/objects to r/w.

Return Value:

    SUCCESS or reason NtProtectVirtualMemory failed.

--*/

{
    HANDLE CurrentProcessHandle;
    SIZE_T RegionSize;
    ULONG NewProtect, OldProtect;
    PVOID VirtualAddress;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER SectionHeader;
    NTSTATUS st;
    ULONG NumberOfSharedDataPages;
    SIZE_T NumberOfNativePagesForImage;

    CurrentProcessHandle = NtCurrentProcess();

    NtHeaders = RtlImageNtHeader(Base);

    SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeaders + sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    NumberOfSharedDataPages = 0;
    NumberOfNativePagesForImage =
        NATIVE_BYTES_TO_PAGES (NtHeaders->OptionalHeader.SizeOfImage);

    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++, SectionHeader++) {
        if ((SectionHeader->Characteristics & IMAGE_SCN_MEM_SHARED) && 
            (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
             (SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE))) {
            RegionSize = SectionHeader->SizeOfRawData;
            VirtualAddress = (PVOID)((ULONG_PTR)Base + 
                                    ((NumberOfNativePagesForImage + NumberOfSharedDataPages) << NATIVE_PAGE_SHIFT));
            NumberOfNativePagesForImage +=  MI_ROUND_TO_SIZE (RegionSize, NATIVE_PAGE_SIZE) >> NATIVE_PAGE_SHIFT;

            if (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE)) {
                //
                // Object isn't writeable, so change it.
                //
                if (Reset) {
                    if (SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
                        NewProtect = PAGE_EXECUTE;
                    } 
                    else {
                        NewProtect = PAGE_READONLY;
                    }
                    NewProtect |= (SectionHeader->Characteristics & IMAGE_SCN_MEM_NOT_CACHED) ? PAGE_NOCACHE : 0;
                } 
                else {
                    NewProtect = PAGE_READWRITE;
                }

                st = NtProtectVirtualMemory(CurrentProcessHandle, &VirtualAddress,
                                            &RegionSize, NewProtect, &OldProtect);

                if (!NT_SUCCESS(st)) {
                    return st;
                }
            }
        }
    }

    if (Reset) {
        NtFlushInstructionCache(NtCurrentProcess(), NULL, 0);
    }

    return STATUS_SUCCESS;
}


PIMAGE_BASE_RELOCATION LdrpWx86ProcessRelocationBlock(
    IN ULONG_PTR VA,
    IN PUCHAR ImageBase,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN ULONG Diff,
    IN ULONG_PTR SectionStartVA,
    IN ULONG_PTR SectionEndVA);

NTSTATUS 
FixupBlockList(
    IN PUCHAR ImageBase);

VOID 
FixupSectionHeader(
    IN PUCHAR ImageBase);

NTSTATUS
LdrpWx86FormatVirtualImage(
    IN PIMAGE_NT_HEADERS32 NtHeaders,
    IN PVOID DllBase
    )
{
   PIMAGE_SECTION_HEADER SectionTable, Section, LastSection, FirstSection;
   ULONG VirtualImageSize;
   PUCHAR NextVirtualAddress, SrcVirtualAddress, DestVirtualAddress;
   PUCHAR ImageBase= DllBase;
   LONG Size;
   ULONG NumberOfSharedDataPages;
   ULONG NumberOfNativePagesForImage;
   ULONG NumberOfExtraPagesForImage;
   ULONG_PTR PreferredImageBase;
   BOOLEAN ImageHasRelocatedSharedSection = FALSE;
   ULONG SubSectionSize;

   NTSTATUS st = Wx86SetRelocatedSharedProtection(DllBase, FALSE);
   if (!NT_SUCCESS(st)) {
       DbgPrint("Wx86SetRelocatedSharedProtection failed with return status %x\n", st);
       Wx86SetRelocatedSharedProtection(DllBase, TRUE);
       return st;
   }

   //
   // Copy each section from its raw file address to its virtual address
   //

   SectionTable = IMAGE_FIRST_SECTION(NtHeaders);
   LastSection = SectionTable + NtHeaders->FileHeader.NumberOfSections;

   if (SectionTable->PointerToRawData == SectionTable->VirtualAddress) {
       // If the first section does not need to be moved then we exclude it
       // from condideration in passes 1 and 2
       FirstSection = SectionTable + 1;
       }
   else {
       FirstSection = SectionTable;
       }

   //
   // First pass starts at the top and works down moving up each section that
   // is to be moved up.
   //
   Section = FirstSection;
   while (Section < LastSection) {
       SrcVirtualAddress = ImageBase + Section->PointerToRawData;
       DestVirtualAddress = Section->VirtualAddress + ImageBase;

       if (DestVirtualAddress > SrcVirtualAddress) {
           // Section needs to be moved down
           break;
           }

       // Section needs to be moved up
      if (Section->SizeOfRawData != 0) {
          if (Section->PointerToRawData != 0) {
              RtlMoveMemory(DestVirtualAddress,
                     SrcVirtualAddress,
                     Section->SizeOfRawData);
              }
          }
      else {
          Section->PointerToRawData = 0;
          }

       Section++;
       }

   //
   // Second pass is from the end of the image and work backwards since src and
   // dst overlap
   //
   Section = --LastSection;
   NextVirtualAddress = ImageBase + NtHeaders->OptionalHeader.SizeOfImage;

   while (Section >= FirstSection) {
       SrcVirtualAddress = ImageBase + Section->PointerToRawData;
       DestVirtualAddress = Section->VirtualAddress + ImageBase;

       //
       // Compute the subsection size.  The mm is really flexible here...
       // it will allow a SizeOfRawData that far exceeds the virtual size,
       // so we can't trust that.  If that happens, just use the page-aligned
       // virtual size, since that is all that the mm will map in.
       //
       SubSectionSize = Section->SizeOfRawData;
       if (Section->Misc.VirtualSize &&
           SubSectionSize > MI_ROUND_TO_SIZE(Section->Misc.VirtualSize, PAGE_SIZE_X86)) {
          SubSectionSize = MI_ROUND_TO_SIZE(Section->Misc.VirtualSize, PAGE_SIZE_X86);
       }

      //
      // ensure Virtual section doesn't overlap the next section
      //
      if (DestVirtualAddress + SubSectionSize > NextVirtualAddress) {
          Wx86SetRelocatedSharedProtection(DllBase, TRUE);
          return STATUS_INVALID_IMAGE_FORMAT;
          }

       if (DestVirtualAddress < SrcVirtualAddress) {
           // Section needs to be moved up
           break;
           }

       // Section needs to be moved down
      if (Section->SizeOfRawData != 0) {
          if (Section->PointerToRawData != 0) {
              RtlMoveMemory(DestVirtualAddress,
                     SrcVirtualAddress,
                     SubSectionSize);
              }
          }
      else {
          Section->PointerToRawData = 0;
          }

       NextVirtualAddress = DestVirtualAddress;
       Section--;
       }

   //
   // Third pass is for zeroing out any memory left between the end of a
   // section and the end of the page. We'll do this from end to top
   //
   Section = LastSection;
   NextVirtualAddress = ImageBase + NtHeaders->OptionalHeader.SizeOfImage;

   NumberOfSharedDataPages = 0;  
   while (Section >= SectionTable) {
       DestVirtualAddress = Section->VirtualAddress + ImageBase;

      //
      // Shared Data sections cannot be shared, because of
      // page misalignment, and are treated as Exec- Copy on Write.
      //
       if ((Section->Characteristics & IMAGE_SCN_MEM_SHARED) && 
           (!(Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
            (Section->Characteristics & IMAGE_SCN_MEM_WRITE))) {
          ImageHasRelocatedSharedSection = TRUE;
#if 0
          DbgPrint("Unsuported IMAGE_SCN_MEM_SHARED %x\n",
                   Section->Characteristics
                   );
#endif
      }

      //
      // If section was empty zero it out
      //
      if (Section->SizeOfRawData != 0) {
          if (Section->PointerToRawData == 0) {
              RtlZeroMemory(DestVirtualAddress,
                            Section->SizeOfRawData
                            );
              }
          }

      //
      // Zero out remaining bytes up to the next section
      //
      RtlZeroMemory(DestVirtualAddress + Section->SizeOfRawData,
                    (ULONG)(NextVirtualAddress - DestVirtualAddress - Section->SizeOfRawData)
                    );

       NextVirtualAddress = DestVirtualAddress;
       Section--;
       }

   // Pass 4: if the dll has any shared sections, change the shared data
   // references to point to additional shared pages at the end of the image.
   //
   // Note that our fixups are applied assuming that the dll is loaded at
   // its preferred base; if it is loaded at some other address, it will
   // be relocated again along will al other addresses.


   if (!ImageHasRelocatedSharedSection) {
       goto LdrwWx86FormatVirtualImageDone;
   }

   st = FixupBlockList(DllBase);   
   if (!NT_SUCCESS(st)) {
       Wx86SetRelocatedSharedProtection(DllBase, TRUE);
       return st;
   }

   NumberOfNativePagesForImage =
        NATIVE_BYTES_TO_PAGES (NtHeaders->OptionalHeader.SizeOfImage);
   NumberOfExtraPagesForImage = 0;

   // Account for raw data that extends beyond SizeOfImage

   for (Section = SectionTable; Section <= LastSection; Section++)
   {
       ULONG EndOfSection;
       ULONG ExtraPages;
       
       EndOfSection = Section->PointerToRawData + Section->SizeOfRawData;
       
       if (EndOfSection > NtHeaders->OptionalHeader.SizeOfImage) {
           
           ExtraPages = NATIVE_BYTES_TO_PAGES(EndOfSection - NtHeaders->OptionalHeader.SizeOfImage);
           if (ExtraPages > NumberOfExtraPagesForImage) {
               NumberOfExtraPagesForImage = ExtraPages;
           }
       }
   }

   PreferredImageBase = NtHeaders->OptionalHeader.ImageBase;

   NumberOfNativePagesForImage += NumberOfExtraPagesForImage;
   NumberOfSharedDataPages = 0;
   for (Section = SectionTable; Section <= LastSection; Section++)
   {
        ULONG bFirst = 1;

        if ((Section->Characteristics & IMAGE_SCN_MEM_SHARED) && 
            (!(Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
             (Section->Characteristics & IMAGE_SCN_MEM_WRITE))) 
        {
            PIMAGE_BASE_RELOCATION NextBlock;
            PUSHORT NextOffset;
            ULONG TotalBytes;
            ULONG SizeOfBlock;
            ULONG_PTR VA;
            ULONG_PTR SectionStartVA;
            ULONG_PTR SectionEndVA;
            ULONG SectionVirtualSize;
            ULONG Diff;

            SectionVirtualSize = Section->Misc.VirtualSize;
            if (SectionVirtualSize == 0)
            {
                SectionVirtualSize = Section->SizeOfRawData;
            }

            SectionStartVA = PreferredImageBase + Section->VirtualAddress;
            SectionEndVA = SectionStartVA + SectionVirtualSize;


            NextBlock = RtlImageDirectoryEntryToData(DllBase, TRUE,
                                        IMAGE_DIRECTORY_ENTRY_BASERELOC,
                                        &TotalBytes);
            if (!NextBlock || !TotalBytes)
            {
                // Note that if this fails, it should fail in the very
                // first iteration and no fixups would have been performed

                if (!bFirst)
                {
                    // Trouble
                    if (ShowSnaps)
                    {
                        DbgPrint("LdrpWx86FormatVirtualImage: failure "
                        "after relocating some sections for image at %x\n",
                                DllBase);
                    }
                    Wx86SetRelocatedSharedProtection(DllBase, TRUE);
                    return STATUS_INVALID_IMAGE_FORMAT;
                }

                if (ShowSnaps)
                {
                    DbgPrint("LdrpWx86FormatVirtualImage: No fixup info "
                                "for image at %x; private sections will be "
                                "used for shared data sections.\n",
                            DllBase);
                }
                break;
            }

            bFirst = 0;

            Diff = (NumberOfNativePagesForImage +
                                NumberOfSharedDataPages) << NATIVE_PAGE_SHIFT;
            Diff -= (ULONG) (SectionStartVA - PreferredImageBase);

            if (ShowSnaps)
            {
                DbgPrint("LdrpWx86FormatVirtualImage: Relocating shared "
                         "data for shared data section 0x%x of image "
                         "at %x by 0x%lx bytes\n",
                         Section - SectionTable + 1, DllBase, Diff);
            }

            while (TotalBytes)
            {
                SizeOfBlock = NextBlock->SizeOfBlock;
                TotalBytes -= SizeOfBlock;
                SizeOfBlock -= sizeof(IMAGE_BASE_RELOCATION);
                SizeOfBlock /= sizeof(USHORT);
                NextOffset = (PUSHORT) ((PCHAR)NextBlock +
                                        sizeof(IMAGE_BASE_RELOCATION));
                VA = (ULONG_PTR) DllBase + NextBlock->VirtualAddress;

                NextBlock = LdrpWx86ProcessRelocationBlock(VA, DllBase, SizeOfBlock,
                                                        NextOffset,
                                                        Diff,
                                                        SectionStartVA,
                                                        SectionEndVA);
                if (NextBlock == NULL)
                {
                    // Trouble
                    if (ShowSnaps)
                    {
                        DbgPrint("LdrpWx86FormatVirtualImage: failure "
                        "after relocating some sections for image at %x; "
                        "Relocation information invalid\n",
                                DllBase);
                    }
                    Wx86SetRelocatedSharedProtection(DllBase, TRUE);
                    return STATUS_INVALID_IMAGE_FORMAT;
                }
            }
            NumberOfSharedDataPages += MI_ROUND_TO_SIZE (SectionVirtualSize,
                                                        NATIVE_PAGE_SIZE) >>
                                                        NATIVE_PAGE_SHIFT;

        }
   }

LdrwWx86FormatVirtualImageDone:
   //
   // Zero out first section's Raw Data up to its VirtualAddress
   //
   if (SectionTable->PointerToRawData != 0) {
       DestVirtualAddress = SectionTable->PointerToRawData + ImageBase;
       Size = (LONG)(NextVirtualAddress - DestVirtualAddress);
       if (Size > 0) {
           RtlZeroMemory(DestVirtualAddress,
                     (ULONG)Size
                     );
           }
   }

   Wx86SetRelocatedSharedProtection(DllBase, TRUE);
   return STATUS_SUCCESS;

}


////////////////////////////////////////////////////

ULONG
LdrpWx86RelocatedFixupDiff(
    IN PUCHAR ImageBase,
    IN ULONG  Offset
    )
{
   PIMAGE_SECTION_HEADER SectionHeader;
   ULONG i;
   ULONG NumberOfSharedDataPages;
   ULONG NumberOfNativePagesForImage;
   PIMAGE_NT_HEADERS32 NtHeaders = (PIMAGE_NT_HEADERS32)RtlImageNtHeader(ImageBase);
   ULONG Diff = 0;
   ULONG_PTR FixupAddr = (ULONG_PTR)(ImageBase + Offset);

   SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeaders + sizeof(ULONG) +
                    sizeof(IMAGE_FILE_HEADER) +
                    NtHeaders->FileHeader.SizeOfOptionalHeader
                    );

   NumberOfNativePagesForImage =
        NATIVE_BYTES_TO_PAGES (NtHeaders->OptionalHeader.SizeOfImage);
   NumberOfSharedDataPages = 0;

   for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++, SectionHeader++) 
   {
       ULONG_PTR SectionStartVA;
       ULONG_PTR SectionEndVA;
       ULONG SectionVirtualSize;

       SectionVirtualSize = SectionHeader->Misc.VirtualSize;
       if (SectionVirtualSize == 0) {
           SectionVirtualSize = SectionHeader->SizeOfRawData;
       }

       SectionStartVA = (ULONG_PTR)ImageBase + SectionHeader->VirtualAddress;
       SectionEndVA = SectionStartVA + SectionVirtualSize;

       if (((ULONG_PTR)FixupAddr >= SectionStartVA) && ((ULONG_PTR)FixupAddr <= SectionEndVA)) {
           if ((SectionHeader->Characteristics & IMAGE_SCN_MEM_SHARED) && 
               (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
                (SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE))) {
               Diff = (NumberOfNativePagesForImage +
                       NumberOfSharedDataPages) << NATIVE_PAGE_SHIFT;
               Diff -= (ULONG)SectionHeader->VirtualAddress;
           }
           break;
       }
 
       if ((SectionHeader->Characteristics & IMAGE_SCN_MEM_SHARED) && 
           (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
            (SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE))) {
           NumberOfSharedDataPages += MI_ROUND_TO_SIZE (SectionVirtualSize,
                                                        NATIVE_PAGE_SIZE) >>
                                                        NATIVE_PAGE_SHIFT;
       }
   }

   return Diff;
}


NTSTATUS 
FixupBlockList(
    IN PUCHAR ImageBase)
{
   PIMAGE_BASE_RELOCATION NextBlock;
   PUSHORT NextOffset;
   ULONG TotalBytes;
   ULONG SizeOfBlock;

   NTSTATUS st;

   NextBlock = RtlImageDirectoryEntryToData(ImageBase, TRUE,
                                            IMAGE_DIRECTORY_ENTRY_BASERELOC,
                                            &TotalBytes);

   if (!NextBlock || !TotalBytes) {
       if (ShowSnaps) {
           DbgPrint("LdrpWx86FixupBlockList: No fixup info "
                    "for image at %x; private sections will be "
                    "used for shared data sections.\n",
                    ImageBase);
       }
       return STATUS_SUCCESS;
   }


   while (TotalBytes) {
       SizeOfBlock = NextBlock->SizeOfBlock;
       TotalBytes -= SizeOfBlock;
       SizeOfBlock -= sizeof(IMAGE_BASE_RELOCATION);
       SizeOfBlock /= sizeof(USHORT);
       NextOffset = (PUSHORT) ((PCHAR)NextBlock +
                               sizeof(IMAGE_BASE_RELOCATION));
       
       NextBlock->VirtualAddress += LdrpWx86RelocatedFixupDiff(ImageBase, NextBlock->VirtualAddress);

       while (SizeOfBlock--) {
           switch ((*NextOffset) >> 12) {
               case IMAGE_REL_BASED_HIGHLOW :
               case IMAGE_REL_BASED_HIGH :
               case IMAGE_REL_BASED_LOW :
                   break;

               case IMAGE_REL_BASED_HIGHADJ :
                   ++NextOffset;
                   --SizeOfBlock;
                   break;

               case IMAGE_REL_BASED_IA64_IMM64:
               case IMAGE_REL_BASED_DIR64:
               case IMAGE_REL_BASED_MIPS_JMPADDR :
               case IMAGE_REL_BASED_ABSOLUTE :
               case IMAGE_REL_BASED_SECTION :
               case IMAGE_REL_BASED_REL32 :
                   break;

               case IMAGE_REL_BASED_HIGH3ADJ :
                   ++NextOffset;
                   --SizeOfBlock;
                   ++NextOffset;
                   --SizeOfBlock;
                   break;

               default :
                   return STATUS_INVALID_IMAGE_FORMAT;
           }
           ++NextOffset;
       }

       NextBlock = (PIMAGE_BASE_RELOCATION)NextOffset;

       if (NextBlock == NULL) {
           // Trouble
           if (ShowSnaps) {
               DbgPrint("LdrpWx86FixupBlockList: failure "
                        "after relocating some sections for image at %x; "
                        "Relocation information invalid\n",
                        ImageBase);
           }
           return STATUS_INVALID_IMAGE_FORMAT;
      }
   }

   return STATUS_SUCCESS;
}


BOOLEAN
LdrpWx86DllHasRelocatedSharedSection(
    IN PUCHAR ImageBase)
{
   PIMAGE_SECTION_HEADER SectionHeader;
   ULONG i;
   PIMAGE_NT_HEADERS32 NtHeaders = (PIMAGE_NT_HEADERS32)RtlImageNtHeader(ImageBase);

   SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeaders + sizeof(ULONG) +
                    sizeof(IMAGE_FILE_HEADER) +
                    NtHeaders->FileHeader.SizeOfOptionalHeader
                    );

   for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++, SectionHeader++) 
   {
       if ((SectionHeader->Characteristics & IMAGE_SCN_MEM_SHARED) && 
           (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
            (SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE))) {
           return TRUE;
       }
   }

   return FALSE;
}


////////////////////////////////////////////////

// Following fn is adapted from rtl\ldrreloc.c; it should be updated when
// that function changes. Eliminated 64 bit address relocations.
//
// Note: Instead of calling this routine, we could call
//     LdrpProcessRelocationBlock(VA, 1, NextOffset, Diff)
//
// but we should do that only if the address to be relocated is between
// SectionStartVA and SectionEndVA. So we would have to replicate all the
// code in the switch stmt below that computes the address of the data item -
// which is pretty much the entire function. So we chose to replicate the
// function as it was and change it to make the test.

PIMAGE_BASE_RELOCATION LdrpWx86ProcessRelocationBlock(
    IN ULONG_PTR VA,
    IN PUCHAR ImageBase,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN ULONG Diff,
    IN ULONG_PTR SectionStartVA,
    IN ULONG_PTR SectionEndVA)
{
    PUCHAR FixupVA;
    USHORT Offset;
    LONG Temp;
    ULONG_PTR DataVA;


    while (SizeOfBlock--) {

       Offset = *NextOffset & (USHORT)0xfff;
       FixupVA = (PUCHAR)(VA + Offset);
       //
       // Apply the fixups.
       //

       switch ((*NextOffset) >> 12) {

            case IMAGE_REL_BASED_HIGHLOW :
                //
                // HighLow - (32-bits) relocate the high and low half
                //      of an address.
                //
                Temp = *(LONG UNALIGNED *)FixupVA;
                DataVA = (ULONG_PTR) Temp;
                if (DataVA >= SectionStartVA && DataVA <= SectionEndVA)
                {
                    Temp += (ULONG) Diff;
                    *(LONG UNALIGNED *)FixupVA = Temp;
                }

                break;

            case IMAGE_REL_BASED_HIGH :
                //
                // High - (16-bits) relocate the high half of an address.
                //
                Temp = *(PUSHORT)FixupVA << 16;
                DataVA = (ULONG_PTR) Temp;
                if (DataVA >= SectionStartVA && DataVA <= SectionEndVA)
                {
                    Temp += (ULONG) Diff;
                    *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);
                }
                break;

            case IMAGE_REL_BASED_HIGHADJ :
                //
                // Adjust high - (16-bits) relocate the high half of an
                //      address and adjust for sign extension of low half.
                //
                Temp = *(PUSHORT)FixupVA << 16;
                ++NextOffset;
                --SizeOfBlock;
                Temp += (LONG)(*(PSHORT)NextOffset);
                DataVA = (ULONG_PTR) Temp;
                if (DataVA >= SectionStartVA && DataVA <= SectionEndVA)
                {
                    Temp += (ULONG) Diff;
                    Temp += 0x8000;
                    *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);
                }
                break;

            case IMAGE_REL_BASED_LOW :
                //
                // Low - (16-bit) relocate the low half of an address.
                //
                Temp = *(PSHORT)FixupVA;
                DataVA = (ULONG_PTR) Temp;
                if (DataVA >= SectionStartVA && DataVA <= SectionEndVA)
                {
                    Temp += (ULONG) Diff;
                    *(PUSHORT)FixupVA = (USHORT)Temp;
                }
                break;

            case IMAGE_REL_BASED_IA64_IMM64:

                //
                // Align it to bundle address before fixing up the
                // 64-bit immediate value of the movl instruction.
                //

                // No need to support

                break;

            case IMAGE_REL_BASED_DIR64:

                //
                // Update 32-bit address
                //

                // No need to support

                break;

            case IMAGE_REL_BASED_MIPS_JMPADDR :
                //
                // JumpAddress - (32-bits) relocate a MIPS jump address.
                //

                // No need to support
                break;

            case IMAGE_REL_BASED_ABSOLUTE :
                //
                // Absolute - no fixup required.
                //
                break;

            case IMAGE_REL_BASED_SECTION :
                //
                // Section Relative reloc.  Ignore for now.
                //
                break;

            case IMAGE_REL_BASED_REL32 :
                //
                // Relative intrasection. Ignore for now.
                //
                break;

           case IMAGE_REL_BASED_HIGH3ADJ :
               //
               // Similar to HIGHADJ except this is the third word.
               //  Adjust low half of high dword of an address and adjust for
               //   sign extension of the low dword.
               //

               // No need to support
                ++NextOffset;
                --SizeOfBlock;
                ++NextOffset;
                --SizeOfBlock;

               break;

            default :
                //
                // Illegal - illegal relocation type.
                //

                return (PIMAGE_BASE_RELOCATION)NULL;
       }
       ++NextOffset;
    }
    return (PIMAGE_BASE_RELOCATION)NextOffset;
}

#endif  // ALPHA or BUILD_WOW6432

#if defined (_ALPHA_) && defined (WX86)
NTSTATUS
Wx86IdentifyPlugin(
    IN PVOID DllBase,
    IN PUNICODE_STRING FullDllName
    )
/*++

Routine Description:

    Determine which (if any) plugin provider dlls support this plugin.

    All registered plugin provider dlls are loaded sequentially and given
    an opportunity to examine the the plugin dll. Provider dlls that support
    the plugin are stored in a WX86PLUGIN object and linked into Wx86PluginList.

    Note: This routine may be invoked before Wx86 has been loaded so the
    global values initialized by LdrpLoadWx86 cannot be used.

Return Value:

    STATUS_SUCCESS - The dll is supported by at least one plugin provider.

    STATUS_IMAGE_MACHINE_TYPE_MISMATCH - There are no plugin providers that support this dll.

    Other - failure in Wx86IdentifyPlugIn
--*/
{
    NTSTATUS st;
    PVOID Provider[WX86PLUGIN_MAXPROVIDER];
    ULONG Count;
    ULONG Length;
    ULONG Index;
    ULONG NumProviders;
    ULONG Disposition;
    HANDLE hProviderKey = NULL;
    HANDLE hProviderIdKey = NULL;
    USHORT ProviderLength;
    USHORT MachineType;
    BOOLEAN CanThunk;
    UNICODE_STRING KeyName;
    UNICODE_STRING DllName;
    UNICODE_STRING ProviderName;
    ANSI_STRING ProcName;
    OBJECT_ATTRIBUTES Obja;
    PKEY_VALUE_PARTIAL_INFORMATION ValPartInfo;
    PKEY_FULL_INFORMATION KeyFullInfo;
    PKEY_BASIC_INFORMATION KeyBasicInfo;
    WCHAR ProviderPath[DOS_MAX_PATH_LENGTH];
    WCHAR DataBuffer[STATIC_UNICODE_BUFFER_LENGTH];
    WX86IDENTIFYPLUGIN IdentifyPlugin = NULL;
    PIMAGE_NT_HEADERS NtHeaders;

    NtHeaders = RtlImageNtHeader(DllBase);
    if (NtHeaders == NULL) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }
    MachineType = NtHeaders->FileHeader.Machine;

    // Identify the plugin by trying each plugin provider in turn.
    // The absence of the Provider registry key is a switch to turn off plugin support.

    Count = 0;
    RtlInitUnicodeString ( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wx86\\Provider" );
    InitializeObjectAttributes (&Obja, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL );
    st = NtOpenKey (&hProviderKey, KEY_READ | KEY_WRITE, &Obja);
    if (st == STATUS_OBJECT_NAME_NOT_FOUND) {
        st = STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
        goto Wx86IdentifyDone;
    }
    if (NT_ERROR(st)) {
        goto Wx86IdentifyDone;
    }

    ValPartInfo = (PKEY_VALUE_PARTIAL_INFORMATION)DataBuffer;
    KeyFullInfo = (PKEY_FULL_INFORMATION)DataBuffer;
    st = NtQueryKey(hProviderKey,
                    KeyFullInformation,
                    KeyFullInfo,
                    sizeof(DataBuffer),
                    &Length
                    );
    if (NT_ERROR(st)) {
        goto Wx86IdentifyDone;
    }

    NumProviders = KeyFullInfo->SubKeys;

    // Initialize the base path of the provider dlls

    ProviderName.Buffer = ProviderPath;
    ProviderName.MaximumLength = sizeof(ProviderPath);
    ProviderName.Length = 0;
    RtlAppendUnicodeToString(&ProviderName, USER_SHARED_DATA->NtSystemRoot );
    if (NT_SUCCESS(st)) {
        st = RtlAppendUnicodeToString(&ProviderName, Wx86Dir);
    }
    if (NT_SUCCESS(st)) {
        st = RtlAppendUnicodeToString( &ProviderName, L"\\Provider\\");
    }
    if (NT_ERROR(st)) {
        goto Wx86IdentifyDone;
    }
    ProviderLength = ProviderName.Length;

    Index = 0;
    st = STATUS_SUCCESS;
    KeyBasicInfo = (PKEY_BASIC_INFORMATION)DataBuffer;
    do {
        st = NtEnumerateKey( hProviderKey, Index, KeyBasicInformation, DataBuffer, sizeof(DataBuffer)-2, &Length );
        if (st == STATUS_NO_MORE_ENTRIES) {
            st = STATUS_SUCCESS;
            break;
        }
        if (NT_ERROR(st)) {
            goto Wx86IdentifyDone;
        }

        // Get the name of the next plugin provider dll

        KeyBasicInfo->Name[KeyBasicInfo->NameLength/2] = UNICODE_NULL;
        RtlInitUnicodeString( &KeyName, KeyBasicInfo->Name );
        InitializeObjectAttributes (&Obja, &KeyName, OBJ_CASE_INSENSITIVE, hProviderKey, NULL );
        st = NtOpenKey (&hProviderIdKey, KEY_READ, &Obja);
        if (st == STATUS_OBJECT_NAME_NOT_FOUND) {
            goto Wx86IdentifyDone;
        }

        RtlInitUnicodeString ( &KeyName, L"DllName" );
        st = NtQueryValueKey( hProviderIdKey, &KeyName, KeyValuePartialInformation, DataBuffer, sizeof(DataBuffer)-2, &Length );
        if (st == STATUS_SUCCESS) {

            // Get the full provider dll path by appending the name to the base provider path

            *(PWCHAR)(&ValPartInfo->Data[ValPartInfo->DataLength]) = UNICODE_NULL;
            ProviderName.Length = ProviderLength;
            if (NT_SUCCESS(st)) {
                st = RtlAppendUnicodeToString( &ProviderName, (PWSTR)ValPartInfo->Data);
            }
            if (NT_ERROR(st)) {
                goto Wx86IdentifyDone;
            }

            // Get the name of an export this provider requires (if any). If the plugin dll doesn't
            // have this export then it's not possible for this provider to support it. This
            // is a quick check which excludes most false tests for plug providers.

            RtlInitUnicodeString ( &KeyName, L"Export" );
            st = NtQueryValueKey( hProviderIdKey, &KeyName, KeyValuePartialInformation, DataBuffer, sizeof(DataBuffer)-2, &Length );
            if (st == STATUS_SUCCESS) {
                ANSI_STRING ExportName;
                UNICODE_STRING UnicodeExportName;
                PVOID ExportAddress;
                CHAR ExportNameBuffer[64];

                ExportName.Buffer = ExportNameBuffer;
                ExportName.MaximumLength = sizeof(ExportNameBuffer);
                ExportName.Length = 0;

                *(PWCHAR)(&ValPartInfo->Data[ValPartInfo->DataLength]) = UNICODE_NULL;
                RtlInitUnicodeString( &UnicodeExportName, (PWCHAR)(ValPartInfo->Data) );
                st = RtlUnicodeStringToAnsiString( &ExportName, &UnicodeExportName, FALSE );
                if (NT_ERROR(st)) {
                    goto Wx86IdentifyDone;
                }
                st = LdrGetProcedureAddress( DllBase, &ExportName, 0, &ExportAddress );
                if (NT_ERROR(st)) {
                    st = STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
                    continue;
                }
            }

            // The required export exists so it's time to load the provider and do the full check
            // On error skip to the next provider

            st = LdrpLoadDll( NULL, NULL, &ProviderName, &Provider[Count], TRUE );

            if (NT_SUCCESS(st)) {
                RtlInitAnsiString(&ProcName,"Wx86IdentifyPlugin");
                st = LdrGetProcedureAddress( Provider[Count], &ProcName, 0, (PVOID *)&IdentifyPlugin);
                CanThunk = FALSE;
                if (NT_SUCCESS(st) && IdentifyPlugin) {
                    try {
                        CanThunk = IdentifyPlugin(DllBase,
                                                  FullDllName->Buffer,
                                                  MachineType == IMAGE_FILE_MACHINE_I386
                                                  );
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                    }
                }

                if (CanThunk) {
                    Count++;
                } else {
                    LdrUnloadDll( Provider[Count] );
                    Provider[Count] = NULL;
                }
            } // loaded provder dll
        }  // opened provider key
    } while ((++Index < NumProviders) && (Count < WX86PLUGIN_MAXPROVIDER));

    // Return success if we found at least on plugin provider. Allocate a WX86PLUGIN
    // list entry to keep track of which provders were loaded for this plugin

    if (Count) {
        PWX86PLUGIN Wx86Plugin;

        Wx86Plugin = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( LDR_TAG ),sizeof(WX86PLUGIN));
        if ( !Wx86Plugin ) {
            st = STATUS_NO_MEMORY;
            goto Wx86IdentifyDone;
            }
        Wx86Plugin->DllBase = DllBase;
        Wx86Plugin->Count = Count;
        RtlMoveMemory( Wx86Plugin->Provider, Provider, Count*sizeof(PVOID) );
        InsertTailList( &Wx86PluginList, &Wx86Plugin->Links );

        st = STATUS_SUCCESS;
    } else {
        st = STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
    }
    st = Count? STATUS_SUCCESS : STATUS_IMAGE_MACHINE_TYPE_MISMATCH;

Wx86IdentifyDone:

    // On error exit release any provider dlls that were loaded

    if (NT_ERROR(st)) {
        for (Index = 0; Index < Count; Index++) {
            LdrUnloadDll( Provider[Count] );
        }
    }

    if (hProviderKey) {
        NtClose(hProviderKey);
    }
    if (hProviderIdKey) {
        NtClose(hProviderIdKey);
    }

    return st;
}

NTSTATUS
Wx86ThunkPluginExport(
    IN PVOID DllBase,
    IN PCHAR ExportName,
    IN ULONG Ordinal,
    IN PVOID ExportAddress,
    OUT PVOID *ExportThunk
    )
/*++

Routine Description:

    This procedure is called during GetProcAddress() processing when
    the request if for a cross-architecure procedure address in an
    image which was loaded as a plugin dll (LDRP_WX86_PLUGIN flag).

    The providers associated with this plugin dll are called in order
    to provide a thunk for the specified plugin export. The thunk attempt
    stops when a provider successfully thunks the export.

Return Value:

    STATUS_SUCCESS - if export is successfully thunked

    STATUS_PROCEDURE_NOT_FOUND - if export cannot be thunked

    STATUS_INVALID_IMAGE_FORMAT - DllBase is invalid

--*/
{
    PLIST_ENTRY Head, Next;
    PWX86PLUGIN Wx86Plugin;
    ULONG Index;
    WX86THUNKEXPORT ThunkExport;
    ANSI_STRING ProcName;
    PIMAGE_NT_HEADERS NtHeaders;
    USHORT MachineType;
    BOOLEAN Thunked;
    NTSTATUS st;

    NtHeaders = RtlImageNtHeader(DllBase);
    if (NtHeaders == NULL) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }
    MachineType = NtHeaders->FileHeader.Machine;

    // Find the Wx86Plugin entry with a matching DllBase

    Head = &Wx86PluginList;
    Next = Head->Flink;
    while ( Next != Head ) {
        Wx86Plugin = CONTAINING_RECORD(Next, WX86PLUGIN, Links);
        if (Wx86Plugin->DllBase == DllBase) {
            break;
        }
        Next = Next->Flink;
    }

    // No Wx86Plugin entry for this Dll

    if (Next == Head) {
        return STATUS_PROCEDURE_NOT_FOUND;
    }

    // Get the address of the routine to thunk exports for each provider

    RtlInitAnsiString(&ProcName,"Wx86ThunkExport");
    for (Index = 0; Index < Wx86Plugin->Count; Index++) {
        st = LdrGetProcedureAddress(Wx86Plugin->Provider[Index],
                                    &ProcName,
                                    0,
                                    (PVOID *)&ThunkExport
                                    );
        if (NT_SUCCESS(st) && ThunkExport != NULL) {
            try {
                Thunked = ThunkExport(DllBase,
                                      ExportName,
                                      Ordinal,
                                      ExportAddress,
                                      ExportThunk,
                                      MachineType == IMAGE_FILE_MACHINE_I386
                                      );
            } except (EXCEPTION_EXECUTE_HANDLER) {
                Thunked = FALSE;
            }

            if (Thunked) {
                if (ShowSnaps) {
                    DbgPrint("LDRWx86: thunk export for %08X Ord=%04X Addr=%08X Name=%s Thunk=%08X\n",
                             DllBase, Ordinal, ExportAddress,
                             ExportName? ExportName : "<noname>", *ExportThunk );
                break;
                }
            }
        }
    }

    return st;
}

BOOLEAN
Wx86UnloadProviders(
    IN PVOID DllBase
    )
/*++

Routine Description:

    Handle unloading of plugin dlls.

    The DllBase passed in may have already been unloaded. However, it is used here
    only to find the associated plugin provider dll.

Return Value:

    FALSE on failure, TRUE on success.

--*/
{
    PLDR_DATA_TABLE_ENTRY PluginEntry;
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY Head, Next;
    PWX86PLUGIN Wx86Plugin;
    ULONG Index;

    // Prevent recursion - Only perform this at the top level.
    // This is relevant when being called from LdrUnloadDll

    if (Wx86ProviderUnloadCount == 0) {
        try {
            Wx86ProviderUnloadCount++;

            // Find the the entry for this plugin dll

    Head = &Wx86PluginList;
    Next = Head->Flink;
    while ( Next != Head ) {
        Wx86Plugin = CONTAINING_RECORD(Next, WX86PLUGIN, Links);
                if (Wx86Plugin->DllBase == DllBase) {
                    
                    // If the plugin dll is no longer mapped then unload the providers
                    // and free the Wx86PluginList entry

                    if (!LdrpCheckForLoadedDllHandle( DllBase, &PluginEntry )) {
            for (Index = 0; Index < Wx86Plugin->Count; Index++) {
                            Status = LdrUnloadDll( Wx86Plugin->Provider[Index] );
            }
            RemoveEntryList( &Wx86Plugin->Links );
            RtlFreeHeap(RtlProcessHeap(), 0, Wx86Plugin );
                        break;
                    }
                }
        Next = Next->Flink;
            }
        } finally {
            Wx86ProviderUnloadCount--;
        }
    }
    
    return NT_SUCCESS(Status);
}

#endif
