/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    module.c

Abstract:

    This module contains the Win32 Module Management APIs

Author:

    Steve Wood (stevewo) 24-Sep-1990

Revision History:

--*/

#include "basedll.h"
#pragma hdrstop

#include <winuserp.h>


extern BOOLEAN gDoDllRedirection; // defined and initialized in baseinit.c

PVOID
BasepMapModuleHandle(
    IN HMODULE hModule OPTIONAL,
    IN BOOLEAN bResourcesOnly
    )
{
    if (ARGUMENT_PRESENT( hModule )) {
        if ((ULONG_PTR)hModule & 0x00000001) {
            if (bResourcesOnly) {
                return( (PVOID)hModule );
                }
            else {
                return( NULL );
                }
            }
        else {
            return( (PVOID)hModule );
            }
        }
    else {
        return( (PVOID)NtCurrentPeb()->ImageBaseAddress);
        }
}

HMODULE
LoadLibraryA(
    LPCSTR lpLibFileName
    )
{
    //
    // The specification for twain_32.dll says that this
    // DLL is supposed to be installed in %windir%. Some
    // apps put a twain_32.dll in the system32 directory
    // and all the apps using this dll will blow up.
    //

    if (_strcmpi(lpLibFileName, "twain_32.dll") == 0) {

        LPSTR pszBuffer;

        pszBuffer = RtlAllocateHeap(RtlProcessHeap(),
                                    MAKE_TAG( TMP_TAG ),
                                    MAX_PATH * sizeof(char));

        if (pszBuffer != NULL) {

            HMODULE hMod;

            GetWindowsDirectoryA(pszBuffer, MAX_PATH);

            strcat(pszBuffer, "\\twain_32.dll");

            hMod = LoadLibraryA(pszBuffer);

            RtlFreeHeap(RtlProcessHeap(), 0, pszBuffer);

            if (hMod != NULL) {
                return hMod;
                }
            }
        }

    return LoadLibraryExA( lpLibFileName, NULL, 0 );
}

HMODULE
LoadLibraryW(
    LPCWSTR lpwLibFileName
    )
{
    return LoadLibraryExW( lpwLibFileName, NULL, 0 );
}

HMODULE
LoadLibraryExA(
    LPCSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    )

/*++

Routine Description:

    ANSI thunk to LoadLibraryExW

--*/

{

    PUNICODE_STRING Unicode;

    Unicode = Basep8BitStringToStaticUnicodeString( lpLibFileName );
    if (Unicode == NULL) {
        return NULL;
    }

    return LoadLibraryExW(Unicode->Buffer,hFile,dwFlags);
}


NTSTATUS
BasepLoadLibraryAsDataFile(
    IN PWSTR DllPath OPTIONAL,
    IN PUNICODE_STRING DllName,
    OUT PVOID *DllHandle
    )

{
    WCHAR FullPath[ MAX_PATH ];
    PWSTR FilePart;
    HANDLE FileHandle;
    HANDLE MappingHandle;
    LPVOID DllBase;
    PIMAGE_NT_HEADERS NtHeaders;
    PTEB Teb;

    Teb = NtCurrentTeb();

    *DllHandle = NULL;
    if (!SearchPathW( DllPath,
                      DllName->Buffer,
                      L".DLL",
                      MAX_PATH,
                      FullPath,
                      &FilePart
                    )
       ) {
        return Teb->LastStatusValue;
        }

    FileHandle = CreateFileW( FullPath,
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_DELETE,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL
                            );

    if (FileHandle == INVALID_HANDLE_VALUE) {
        return Teb->LastStatusValue;
        }

    MappingHandle = CreateFileMappingW( FileHandle,
                                        NULL,
                                        PAGE_READONLY,
                                        0,
                                        0,
                                        NULL
                                      );
    CloseHandle( FileHandle );
    if (MappingHandle == NULL) {
        return Teb->LastStatusValue;
        }

    DllBase = MapViewOfFileEx( MappingHandle,
                               FILE_MAP_READ,
                               0,
                               0,
                               0,
                               NULL
                             );
    CloseHandle( MappingHandle );
    if (DllBase == NULL) {
        return Teb->LastStatusValue;
        }

    NtHeaders = RtlImageNtHeader( DllBase );
    if (NtHeaders == NULL) {
        UnmapViewOfFile( DllBase );
        return STATUS_INVALID_IMAGE_FORMAT;
        }


    *DllHandle = (HANDLE)((ULONG_PTR)DllBase | 0x00000001);
    LdrLoadAlternateResourceModule(*DllHandle, FullPath);

    return STATUS_SUCCESS;
}

#define DLL_EXTENSION L".DLL"

NTSTATUS
ComputeRedirectedDllName(
    IN  PUNICODE_STRING DllName,
    OUT PUNICODE_STRING NewDllName
    )
    /*++

Routine Description:

    This function computes a fully qualified path to a DLL name. It takes
    the path of the current process and the base name from DllName and
    puts these together. DllName can have '\' or '/' as separator.

Arguments:

    DllName - Points to a string that names the library file. This can be
        a fully qualified name or just a base name. We will parse for the base
        name (the portion after the last '\' or '/' char. Caller guarantees that
        DllName->Buffer is not a NULL pointer!

    NewDllName - Has fully qualified path based on GetModuleFileNameW(NULL...)
        and the base name from above.

Return Value:

    NTSTATUS: Currently: STATUS_NO_MEMORY or STATUS_SUCCESS.

--*/

{
    LPWSTR p, pp1, pp2, pp3 ;
    USHORT Size1, Size2 ;
    WCHAR FullNameBuffer[MAX_PATH] ;

    Size1 = (USHORT) GetModuleFileNameW(NULL, FullNameBuffer, sizeof(FullNameBuffer)>>1);

    // Find the end of the EXE path (start of its base-name) in pp1.

    p = FullNameBuffer + Size1 - 1; // point to last character of this name
    while (p != FullNameBuffer) {
        if (*p == (WCHAR)'\\') {
            pp1 = p + 1;
            break;
        }
        p-- ;
    }

    // Find the basename portion of the DLL to be loaded in pp2 and the
    // last '.' character if present in the basename.

    pp2 = DllName->Buffer;
    pp3 = NULL ;
    if (DllName->Length) {
        p = DllName->Buffer + (DllName->Length>>1) - 1 ; // point to last char
        while (p != DllName->Buffer) {
            if (*p == (WCHAR) '.') {
                if (!pp3) {
                    pp3 = p ;
                    }
                }
            else {
                if ((*p == (WCHAR) '\\') || (*p == (WCHAR) '/')) {
                    pp2 = p + 1;
                    break;
                    }
                }
            p--;
        }
    }
    // Create a fully qualified path to the DLL name (using pp1 and pp2)

    // Size1 = Number of bytes (not including NULL or EXE/process folder)
    Size1 = (USHORT)((pp1 - FullNameBuffer) << 1) ;

    // Size2 = Number of bytes in base DLL name (including trailing null char)
    Size2 = (USHORT)(DllName->MaximumLength - ((pp2 - DllName->Buffer) << 1)) ;

    // Just allocate 8 bytes more in case we decide to tack on the L".DLL" to
    // end of this path

    p = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), Size1+Size2+sizeof(DLL_EXTENSION)-sizeof(WCHAR));

    if (!p) {
        return STATUS_NO_MEMORY ;
        }

    RtlCopyMemory(p, FullNameBuffer, Size1);
    RtlCopyMemory(p+(Size1>>1), pp2, Size2) ;

    if (!pp3) {
            // If there is no '.' in the basename add the ".DLL" to it.
            //
            //  The -1 will work just as well as - sizeof(WCHAR) as we are dividing by
            // sizeof(WCHAR) and it will be rounded down correctly as Size1 and Size2 are
            // even. The -1 could be more optimal than subtracting sizeof(WCHAR)
            //
            RtlCopyMemory(p+((Size1+Size2-1)>>1), DLL_EXTENSION, sizeof(DLL_EXTENSION)) ;

            Size1  = Size1 + sizeof(DLL_EXTENSION) - sizeof(WCHAR) ; // Mark base name as being 8 bytes bigger.
        }
    else {
        // There was '.'. If this is the last character, strip it out.
        //
        if (*(pp3+1) == UNICODE_NULL) {
            *pp3 = UNICODE_NULL ;
            }
        }

    NewDllName->Buffer = p ;
    NewDllName->MaximumLength = Size1 + Size2 ;
    NewDllName->Length = NewDllName->MaximumLength - sizeof(WCHAR) ;

    return STATUS_SUCCESS ;
}

HMODULE
LoadLibraryExW(
    LPCWSTR lpwLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    )

/*++

Routine Description:

    This function loads the library module contained in the specified
    file and retrieves a handle to the loaded module.

    It is important to note that module handles are NOT global, in that
    a LoadLibrary call by one application does not produce a handle that
    another application can use, say in calling GetProcAddress.  The
    other application would need to do its own call to LoadLibrary for
    the module before calling GetProcAddress.  Module handles will have
    the same 32-bit value across all processes, but a module handle is
    only valid in the context of a process after the module has been
    loaded into that process, either as a result of an explicit call to
    LoadLibrary or as a result of an implicit call caused by a loadtime
    dynamic link to an entry point in the module.

    The library file name does not need to specify an extension.  If one
    is not specified, then the default library file extension, .DLL, is
    used (note that this is different than Win16. Under Win16 specifying
    no extension would not cause ".DLL" to be appended to the name. To get
    Win16 behavior, if the module name has no extension, the caller must
    supply a trailing ".").

    The library file name does not need to specify a directory path.  If
    one is specified, then the specified file must exist.  If a path is
    not specified, this function will look for the library file in using
    the Windows search path:

      - The current process image file directory

      - The current directory

      - The Windows system directory

      - The Windows directory

      - The directories listed in the PATH environment variable

    The first directory searched is the directory that contains the
    image file that was used to create the current process (see
    CreateProcess).  This allows private dynamic link library files
    associated with an application to be found without having to add the
    application's installed directory to the PATH environment variable.

    The image file loader optimizes the search by remembering for each
    loaded library module that unqualified module name that was searched
    for when a module was loaded into the current process the first
    time.  This unqualified name has nothing to do with the module name
    that is stored within the library module itself, as specified by the
    NAME keyword in the .DEF file.  This is a change from the Windows
    3.1 behavior, where the search was optimized by comparing to the
    name within the library module itself, which could lead to confusing
    result if the internal name differed from the external file name.

    Once a fully qualified path name to a library module file is
    obtained, a search is made to see if that library module file has
    been loaded into the current process.  The search is case
    insensitive and includes the full path name of each library module
    file.  If a match is found for the library module file, then it has
    already been loaded into the current process, so this function just
    increments the reference count for the module and returns the module
    handle for that library.

    Otherwise, this is the first time the specified module has been
    loaded for the current process, so the library module's DLL Instance
    Initialization entry point will be called.  See the Task Management
    section for a description of the DLL Instance Initialization entry
    point.

    Fine Point: If DLL re-direction is enabled for the app./process requesting
    this load, if we find a DLL in the app. folder (with same base name),
    we load that file (ignoring any path qualifications passed in).

Arguments:

    lpwLibFileName - Points to a string that names the library file.  The
    string must be a null-terminated unicode string.

    hFile - optional file handle, that if specified, while be used to
        create the mapping object for the module.

    dwFlags - flags that specify optional behavior.  Valid flags are:

        DONT_RESOLVE_DLL_REFERENCES - loads the library but does not
            attempt to resolve any of its DLL references nor does it
            attempt to call its initialization procedure.

Return Value:

    The return value identifies the loaded module if the function is
    successful.  A return value of NULL indicates an error and extended
    error status is available using the GetLastError function.

--*/

{
    LPWSTR TrimmedDllName;
    LPWSTR AllocatedPath;
    NTSTATUS Status;
    HMODULE hModule;
    UNICODE_STRING DllName_U, AppPathDllName_U;
    UNICODE_STRING AllocatedPath_U;
    ULONG DllCharacteristics;
    extern PLDR_DATA_TABLE_ENTRY BasepExeLdrEntry;

    TrimmedDllName = NULL;
    DllCharacteristics = 0;
    if (dwFlags & DONT_RESOLVE_DLL_REFERENCES) {
        DllCharacteristics |= IMAGE_FILE_EXECUTABLE_IMAGE;
        }

    RtlInitUnicodeString(&DllName_U, lpwLibFileName);

    //
    // Quick check to see if dll being loaded is the main exe. For some reason
    // hook stuff tends to do this and this is worst path through the loader
    //

    if ( !(dwFlags & LOAD_LIBRARY_AS_DATAFILE) && BasepExeLdrEntry && (DllName_U.Length == BasepExeLdrEntry->FullDllName.Length) ){
        if ( RtlEqualUnicodeString(&DllName_U,&BasepExeLdrEntry->FullDllName,TRUE) ) {
            return (HMODULE)BasepExeLdrEntry->DllBase;
            }
        }

    //
    // check to see if there are trailing spaces in the dll name (Win95 compat)
    //
    if ( DllName_U.Length && DllName_U.Buffer[(DllName_U.Length-1)>>1] == (WCHAR)' ') {
        TrimmedDllName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), DllName_U.MaximumLength);
        if ( !TrimmedDllName ) {
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return NULL;
            }
        RtlCopyMemory(TrimmedDllName,DllName_U.Buffer,DllName_U.MaximumLength);
        DllName_U.Buffer = TrimmedDllName;
        while (DllName_U.Length && DllName_U.Buffer[(DllName_U.Length-1)>>1] == (WCHAR)' ') {
            DllName_U.Buffer[(DllName_U.Length-1)>>1] = UNICODE_NULL;
            DllName_U.Length -= sizeof(WCHAR);
            DllName_U.MaximumLength -= sizeof(WCHAR);
            }
        }

    //
    // If DLL redirection is on for this application, we check to see if the DLL requested
    // (without path qualification) exists in the app. (EXE) folder. If so, we load that.
    // Else we fall back to regular search logic.
    //

    if (gDoDllRedirection && DllName_U.Length) {
        Status = ComputeRedirectedDllName(&DllName_U, &AppPathDllName_U) ;
        if(!NT_SUCCESS(Status)) {
            if ( TrimmedDllName ) {
                RtlFreeHeap(RtlProcessHeap(), 0, TrimmedDllName);
            }

            BaseSetLastNTError(Status);
            return NULL;
            }

        if (RtlDoesFileExists_U(AppPathDllName_U.Buffer)) {
            DllName_U.Buffer = AppPathDllName_U.Buffer ;
            DllName_U.MaximumLength = AppPathDllName_U.MaximumLength ;
            DllName_U.Length = AppPathDllName_U.Length;
            }
        }

    //
    // Determine the path that the program was created from
    //

    AllocatedPath = BaseComputeProcessDllPath(
                        dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH ? DllName_U.Buffer : NULL,
                        GetEnvironmentStringsW()
                        );
    if ( !AllocatedPath ) {
        Status = STATUS_NO_MEMORY;
        if ( TrimmedDllName ) {
            RtlFreeHeap(RtlProcessHeap(), 0, TrimmedDllName);
            }
        goto bail;
        }
    RtlInitUnicodeString(&AllocatedPath_U, AllocatedPath);
    try {
        if (dwFlags & LOAD_LIBRARY_AS_DATAFILE) {
#ifdef WX86
            // LdrGetDllHandle clears UseKnownWx86Dll, but the value is
            // needed again by LdrLoadDll.
            BOOLEAN Wx86KnownDll = NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll;
#endif
            Status = LdrGetDllHandle(
                        AllocatedPath_U.Buffer,
                        NULL,
                        &DllName_U,
                        (PVOID *)&hModule
                        );
            if (NT_SUCCESS( Status )) {
#ifdef WX86
                NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = Wx86KnownDll;
#endif
                goto alreadyLoaded;
                }

            Status = BasepLoadLibraryAsDataFile( AllocatedPath_U.Buffer,
                                                 &DllName_U,
                                                 (PVOID *)&hModule
                                               );
            }
        else {
alreadyLoaded:
            Status = LdrLoadDll(
                        AllocatedPath_U.Buffer,
                        &DllCharacteristics,
                        &DllName_U,
                        (PVOID *)&hModule
                        );
            }
        if ( TrimmedDllName ) {
            RtlFreeHeap(RtlProcessHeap(), 0, TrimmedDllName);
            TrimmedDllName = NULL;
            }
        RtlFreeHeap(RtlProcessHeap(), 0, AllocatedPath);
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        if ( TrimmedDllName ) {
            RtlFreeHeap(RtlProcessHeap(), 0, TrimmedDllName);
            }
        RtlFreeHeap(RtlProcessHeap(), 0, AllocatedPath);
        }
bail:
    if (gDoDllRedirection) {
        // We would have bailed had we not been able to allocate this buffer in re-direction case.
        RtlFreeHeap(RtlProcessHeap(), 0, AppPathDllName_U.Buffer);
        }

    if (!NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
        }
    else {
        return hModule;
        }
}

BOOL
FreeLibrary(
    HMODULE hLibModule
    )

/*++

Routine Description:

    This function decreases the reference count of the loaded library
    module by one.  The reference count is maintain for each process.

    When the reference count for the specified library module is
    decremented to zero, the library module's DLL Instance Termination
    entry point is called.  This will allow a library module a chance to
    cleanup resources that we allocated on behalf of the current
    process.  See the Task Management section for a description of the
    DLL Instance Termination entry point.  Finally, after the
    termination entry point returns, the library module is removed from
    the address space of the current process.

    If more than one process has loaded a library module, then the
    library module will remain in use until all process's that loaded
    the module have called FreeLibrary to unload the library.

Arguments:

    hLibModule - Identifies the loaded library module.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using the GetLastError function.

--*/

{
    NTSTATUS Status;


    if ( (ULONG_PTR)hLibModule & 0x00000001 ) {

        if ( RtlImageNtHeader((PVOID)((ULONG_PTR)hLibModule & ~0x00000001)) ) {

            Status = NtUnmapViewOfSection( NtCurrentProcess(),
                                           (PVOID)((ULONG_PTR)hLibModule & ~0x00000001)
                                         );
            LdrUnloadAlternateResourceModule(hLibModule);

            }
        else {
            Status = STATUS_INVALID_IMAGE_FORMAT;
            }
        }
    else {
        Status = LdrUnloadDll( (PVOID)hLibModule );
        }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        return TRUE;
        }
}

VOID
WINAPI
FreeLibraryAndExitThread(
    HMODULE hLibModule,
    DWORD dwExitCode
    )

/*++

Routine Description:

    This function decreases the reference count of the loaded library
    module by one, and then calls ExitThread.

    The purpose of this function is to allow threads that are created
    within a dll, and execute within that DLL an opportunity to safely
    unload the DLL and to exit.

    When the reference count for the specified library module is
    decremented to zero, the library module's DLL Instance Termination
    entry point is called.  This will allow a library module a chance to
    cleanup resources that we allocated on behalf of the current
    process.  See the Task Management section for a description of the
    DLL Instance Termination entry point.  Finally, after the
    termination entry point returns, the library module is removed from
    the address space of the current process.

    If more than one process has loaded a library module, then the
    library module will remain in use until all process's that loaded
    the module have called FreeLibrary to unload the library.

Arguments:

    hLibModule - Identifies the loaded library module.

    dwExitCode - Supplies the exit code for the thread

Return Value:

    This function never returns. invalid hLibModule values are silently ignored

--*/

{
    if ( (ULONG_PTR)hLibModule & 0x00000001 ) {
        NtUnmapViewOfSection( NtCurrentProcess(),
                                       (PVOID)((ULONG_PTR)hLibModule & ~0x00000001)
                                     );
        }
    else {
        LdrUnloadDll( (PVOID)hLibModule );
        }

    ExitThread(dwExitCode);
}

BOOL
WINAPI
DisableThreadLibraryCalls(
    HMODULE hLibModule
    )

/*++

Routine Description:

    This function disables DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications
    for the DLL specified by hLibModule. Attempting to call this on a DLL with
    inuse static thread local storage is an error.

Arguments:

    hLibModule - Identifies the loaded library module.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using the GetLastError function.

--*/

{
    NTSTATUS Status;
    BOOL rv;

    rv = TRUE;
    Status = LdrDisableThreadCalloutsForDll(hLibModule);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        rv = FALSE;
        }
    return rv;
}


DWORD
WINAPI
GetModuleFileNameW(
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    )

/*++

Routine Description:

    This function retrieves the full pathname of the executable file
    from which the specified module was loaded.  The function copies the
    null-terminated filename into the buffer pointed to by the
    lpFilename parameter.

Routine Description:

    hModule - Identifies the module whose executable file name is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        current process.

    lpFilename - Points to the buffer that is to receive the filename.

    nSize - Specifies the maximum number of characters to copy.  If the
        filename is longer than the maximum number of characters
        specified by the nSize parameter, it is truncated.

Return Value:

    The return value specifies the actual length of the string copied to
    the buffer.  A return value of zero indicates an error and extended
    error status is available using the GetLastError function.

Arguments:

--*/

{
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head,Next;
    DWORD ReturnCode;
    PVOID DllHandle = BasepMapModuleHandle( hModule, FALSE );
    PUNICODE_STRING Ustr;

    ReturnCode = 0;
    nSize = nSize << 1;

    //
    // If we are looking at the current image, then check for name
    // redirection
    //

    if ( !ARGUMENT_PRESENT( hModule )) {
        if ( RtlGetPerThreadCurdir() && RtlGetPerThreadCurdir()->ImageName ) {
            Ustr = RtlGetPerThreadCurdir()->ImageName;
            ReturnCode = (DWORD)Ustr->MaximumLength;
            if ( nSize < ReturnCode ) {
                ReturnCode = nSize;
                }
            RtlMoveMemory(
                lpFilename,
                Ustr->Buffer,
                ReturnCode
                );
            if ( ReturnCode == (DWORD)Ustr->MaximumLength ) {
                ReturnCode -= sizeof(UNICODE_NULL);
                }
            return ReturnCode/2;
            }
        }
    try {
        RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
        Next = Head->Flink;

        while ( Next != Head ) {
            Entry = CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InLoadOrderLinks);
            if ( DllHandle == (PVOID)Entry->DllBase ) {
                ReturnCode = (DWORD)Entry->FullDllName.MaximumLength;
                if ( nSize < ReturnCode ) {
                    ReturnCode = nSize;
                    }
                try {
                    RtlMoveMemory(
                        lpFilename,
                        Entry->FullDllName.Buffer,
                        ReturnCode
                        );
                    if ( ReturnCode == (DWORD)Entry->FullDllName.MaximumLength ) {
                        ReturnCode -= sizeof(UNICODE_NULL);
                        }
                    goto finally_exit;
                    }
                except (EXCEPTION_EXECUTE_HANDLER) {
                    BaseSetLastNTError(GetExceptionCode());
                    return 0;
                    }
                }
            Next = Next->Flink;
            }
finally_exit:;
        }
    finally {
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        }
    return ReturnCode/2;
}

DWORD
GetModuleFileNameA(
    HMODULE hModule,
    LPSTR lpFilename,
    DWORD nSize
    )
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    DWORD ReturnCode;

    UnicodeString.Buffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), nSize*2);
    if ( !UnicodeString.Buffer ) {
        BaseSetLastNTError( STATUS_NO_MEMORY );
        return 0;
        }
    ReturnCode = GetModuleFileNameW(hModule, UnicodeString.Buffer, nSize);
    UnicodeString.Length = UnicodeString.MaximumLength = (USHORT)ReturnCode*2;
    UnicodeString.MaximumLength++;
    UnicodeString.MaximumLength++;

    if (ReturnCode) {
    Status = BasepUnicodeStringTo8BitString(&AnsiString, &UnicodeString, TRUE);
    if (!NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        RtlFreeUnicodeString(&UnicodeString);
        return 0;
        }

    ReturnCode = min( nSize, AnsiString.Length );

    RtlMoveMemory(
        lpFilename,
        AnsiString.Buffer,
        nSize <= ReturnCode ? nSize : ReturnCode + 1
        );

    RtlFreeAnsiString(&AnsiString);
    }

    RtlFreeUnicodeString(&UnicodeString);
    return ReturnCode;
}

HMODULE
GetModuleHandleA(
    LPCSTR lpModuleName
    )

/*++

Routine Description:

    ANSI thunk to GetModuleHandleW

--*/

{
    PUNICODE_STRING Unicode;

    if ( !ARGUMENT_PRESENT(lpModuleName) ) {
        return( (HMODULE)(PVOID)NtCurrentPeb()->ImageBaseAddress );
        }

    Unicode = Basep8BitStringToStaticUnicodeString( lpModuleName );
    if (Unicode == NULL) {
        return NULL;
    }

    return GetModuleHandleW(Unicode->Buffer);
}

HMODULE
WINAPI
GetModuleHandleForUnicodeString(
    IN PUNICODE_STRING ModuleName
    )
/*++

Routine Description:

    This function is the helper routine for GetModuleHandleW. See that for
    more details on return value, etc.

Arguments:

    ModuleName - Points to counted unicode string that names the library file.
        Caller guarantees that ModuleName->Buffer is not NULL.

Return Value:

    See GetModuleHandleW for this.

--*/

{
    LPWSTR AllocatedPath;
    NTSTATUS Status;
    HMODULE hModule;
#ifdef WX86
    BOOLEAN Wx86KnownDll;
#endif

#ifdef WX86
    // LdrGetDllHandle clears UseKnownWx86Dll, but the value is needed again
    // for the second LdrGetDllHandle call.
    Wx86KnownDll = NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll;
#endif
    Status = LdrGetDllHandle(
                (PWSTR)1,
                NULL,
                ModuleName,
                (PVOID *)&hModule
                );
    if ( NT_SUCCESS(Status) ) {
        return hModule;
        }

    //
    // Determine the path that the program was created from
    //

    AllocatedPath = BaseComputeProcessDllPath(NULL,GetEnvironmentStringsW());
    if (!AllocatedPath) {
        Status = STATUS_NO_MEMORY;
        goto bail;
        }
#ifdef WX86
    NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = Wx86KnownDll;
#endif

    try {

        Status = LdrGetDllHandle(
                    AllocatedPath,
                    NULL,
                    ModuleName,
                    (PVOID *)&hModule
                    );
        RtlFreeHeap(RtlProcessHeap(), 0, AllocatedPath);
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        RtlFreeHeap(RtlProcessHeap(), 0, AllocatedPath);
        }

bail:
    if (!NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
        }
    else {
        return hModule;
        }
}

HMODULE
WINAPI
GetModuleHandleW(
    LPCWSTR lpwModuleName
    )

/*++

Routine Description:

    This function returns the handle of a module that is loaded into the
    context of the calling process.

    In a multi-thread environment, this function is not reliable, since
    while one thread is calling this function and getting back a module
    handle, another thread in the same process could be calling
    FreeLibrary for the same module, therefore invalidating the returned
    module handle for the first thread.

Arguments:

    lpwModuleName - Points to a string that names the library file.  The
    string must be a null-terminated unicode string.  If this
        parameter is NULL, then the handle for the current image file is
        returned.

Return Value:

    The return value is a module handle.  A return value of NULL
    indicates either that the module has not been loaded into the
    context of the current process or an error occured.  The exact
    reason is available using the GetLastError function.

--*/

{
    NTSTATUS Status;
    HMODULE hModule;
    UNICODE_STRING DllName_U, AppPathDllName_U;

    if (!ARGUMENT_PRESENT(lpwModuleName)) {
        return( (HMODULE)(PVOID)NtCurrentPeb()->ImageBaseAddress );
        }

    RtlInitUnicodeString(&DllName_U, lpwModuleName);

    if (gDoDllRedirection) {
        Status = ComputeRedirectedDllName(&DllName_U, &AppPathDllName_U) ;
        if(!NT_SUCCESS(Status)) {
            BaseSetLastNTError(Status);
            return NULL;
        }

        hModule = GetModuleHandleForUnicodeString(&AppPathDllName_U) ;

        RtlFreeHeap(RtlProcessHeap(), 0, AppPathDllName_U.Buffer);

        if (hModule != NULL) {
            return hModule;
            }
        // Didn't find any re-directed DLL with this name loaded. Now we can just check for the
        // original name passed in.
    }

    return GetModuleHandleForUnicodeString(&DllName_U) ;
}


FARPROC
GetProcAddress(
    HMODULE hModule,
    LPCSTR lpProcName
    )

/*++

Routine Description:

    This function retrieves the memory address of the function whose
    name is pointed to by the lpProcName parameter.  The GetProcAddress
    function searches for the function in the module specified by the
    hModule parameter, or in the module associated with the current
    process if hModule is NULL.  The function must be an exported
    function; the module's definition file must contain an appropriate
    EXPORTS line for the function.

    If the lpProcName parameter is an ordinal value and a function with
    the specified ordinal does not exist in the module, GetProcAddress
    can still return a non-NULL value.  In cases where the function may
    not exist, specify the function by name rather than ordinal value.

    Only use GetProcAddress to retrieve addresses of exported functions
    that belong to library modules.

    The spelling of the function name (pointed to by lpProcName) must be
    identical to the spelling as it appears in the source library's
    definition (.DEF) file.  The function can be renamed in the
    definition file.  Case sensitive matching is used???

Arguments:

    hModule - Identifies the module whose executable file contains the
        function.  A value of NULL references the module handle
        associated with the image file that was used to create the
        current process.


    lpProcName - Points to the function name, or contains the ordinal
        value of the function.  If it is an ordinal value, the value
        must be in the low-order word and zero must be in the high-order
        word.  The string must be a null-terminated character string.

Return Value:

    The return value points to the function's entry point if the
    function is successful.  A return value of NULL indicates an error
    and extended error status is available using the GetLastError function.


--*/

{
    NTSTATUS Status;
    PVOID ProcedureAddress;
    STRING ProcedureName;

    if ( (ULONG_PTR)lpProcName > 0xffff ) {
        RtlInitString(&ProcedureName,lpProcName);
        Status = LdrGetProcedureAddress(
                        BasepMapModuleHandle( hModule, FALSE ),
                        &ProcedureName,
                        0L,
                        &ProcedureAddress
                        );
        }
    else {
        Status = LdrGetProcedureAddress(
                        BasepMapModuleHandle( hModule, FALSE ),
                        NULL,
                        PtrToUlong((PVOID)lpProcName),
                        &ProcedureAddress
                        );
        }
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
        }
    else {
        if ( ProcedureAddress == BasepMapModuleHandle( hModule, FALSE ) ) {
            if ( (ULONG_PTR)lpProcName > 0xffff ) {
                Status = STATUS_ENTRYPOINT_NOT_FOUND;
                }
            else {
                Status = STATUS_ORDINAL_NOT_FOUND;
                }
            BaseSetLastNTError(Status);
            return NULL;
            }
        else {
            return (FARPROC)ProcedureAddress;
            }
        }
}

DWORD
GetVersion(
    VOID
    )

/*++

Routine Description:

    This function returns the current version number of Windows.

Arguments:

    None.

Return Value:

    The return value specifies the major and minor version numbers of
    Windows.  The high-order word specifies the minor version (revision)
    number; the low-order word specifies the major version number.

--*/

{
    PPEB Peb;

    Peb = NtCurrentPeb();

    return (((Peb->OSPlatformId ^ 0x2) << 30) |
            (Peb->OSBuildNumber << 16) |
            (Peb->OSMinorVersion << 8) |
             Peb->OSMajorVersion
           );
}

WINBASEAPI
BOOL
WINAPI
GetVersionExA(
    LPOSVERSIONINFOA lpVersionInformation
    )
{
    OSVERSIONINFOEXW VersionInformationU;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    if (lpVersionInformation->dwOSVersionInfoSize != sizeof( OSVERSIONINFOEXA ) &&
        lpVersionInformation->dwOSVersionInfoSize != sizeof( *lpVersionInformation )
       ) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
        }

    VersionInformationU.dwOSVersionInfoSize = sizeof( VersionInformationU );
    if (GetVersionExW( (LPOSVERSIONINFOW)&VersionInformationU )) {
        lpVersionInformation->dwMajorVersion = VersionInformationU.dwMajorVersion;
        lpVersionInformation->dwMinorVersion = VersionInformationU.dwMinorVersion;
        lpVersionInformation->dwBuildNumber  = VersionInformationU.dwBuildNumber;
        lpVersionInformation->dwPlatformId   = VersionInformationU.dwPlatformId;
        if (lpVersionInformation->dwOSVersionInfoSize == sizeof( OSVERSIONINFOEXA )) {
            ((POSVERSIONINFOEXA)lpVersionInformation)->wServicePackMajor = VersionInformationU.wServicePackMajor;
            ((POSVERSIONINFOEXA)lpVersionInformation)->wServicePackMinor = VersionInformationU.wServicePackMinor;
            ((POSVERSIONINFOEXA)lpVersionInformation)->wSuiteMask = VersionInformationU.wSuiteMask;
            ((POSVERSIONINFOEXA)lpVersionInformation)->wProductType = VersionInformationU.wProductType;
            ((POSVERSIONINFOEXA)lpVersionInformation)->wReserved = VersionInformationU.wReserved;
            }

        AnsiString.Buffer = lpVersionInformation->szCSDVersion;
        AnsiString.Length = 0;
        AnsiString.MaximumLength = sizeof( lpVersionInformation->szCSDVersion );

        RtlInitUnicodeString( &UnicodeString, VersionInformationU.szCSDVersion );
        Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                               &UnicodeString,
                                               FALSE
                                             );
        if (NT_SUCCESS( Status )) {
            return TRUE;
            }
        else {
            return FALSE;
            }
        }
    else {
        return FALSE;
        }
}

WINBASEAPI
BOOL
WINAPI
GetVersionExW(
    LPOSVERSIONINFOW lpVersionInformation
    )
{
    PPEB Peb;
	NTSTATUS Status;

    if (lpVersionInformation->dwOSVersionInfoSize != sizeof( OSVERSIONINFOEXW ) &&
        lpVersionInformation->dwOSVersionInfoSize != sizeof( *lpVersionInformation )
       ) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
        }
    Status = RtlGetVersion(lpVersionInformation);
    if (Status == STATUS_SUCCESS) {
        Peb = NtCurrentPeb();
        if (Peb->CSDVersion.Buffer)
    		wcscpy( lpVersionInformation->szCSDVersion, Peb->CSDVersion.Buffer );
    	if (lpVersionInformation->dwOSVersionInfoSize ==
											sizeof( OSVERSIONINFOEXW))
			((POSVERSIONINFOEXW)lpVersionInformation)->wReserved =
										(UCHAR)BaseRCNumber;
		return TRUE;
	} else {
		return FALSE;
	}
}


WINBASEAPI
BOOL
WINAPI
VerifyVersionInfoW(
    IN LPOSVERSIONINFOEXW VersionInfo,
    IN DWORD TypeMask,
    IN DWORDLONG ConditionMask
    )

/*++

Routine Description:

    This function verifies a version condition.  Basically, this
    function lets an app query the system to see if the app is
    running on a specific version combination.


Arguments:

    VersionInfo     - a version structure containing the comparison data
    TypeMask        - a mask comtaining the data types to look at
    ConditionMask   - a mask containing conditionals for doing the comparisons


Return Value:

    TRUE  - the version condition exists
    FALSE - the version condition does NOT exists

--*/

{
    DWORD i;
    OSVERSIONINFOEXW CurrVersion;
    BOOL SuiteFound = FALSE;
	NTSTATUS Status;


    Status = RtlVerifyVersionInfo(VersionInfo, TypeMask, ConditionMask);
    if (Status == STATUS_INVALID_PARAMETER) {
        SetLastError( ERROR_BAD_ARGUMENTS );
        return FALSE;
    } else if (Status == STATUS_REVISION_MISMATCH) {
        SetLastError(ERROR_OLD_WIN_VERSION);
        return FALSE;
    }

    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
VerifyVersionInfoA(
    IN LPOSVERSIONINFOEXA VersionInfo,
    IN DWORD TypeMask,
    IN DWORDLONG ConditionMask
    )

/*++

Routine Description:

    This function verifies a version condition.  Basically, this
    function lets an app query the system to see if the app is
    running on a specific version combination.


Arguments:

    VersionInfo     - a version structure containing the comparison data
    TypeMask        - a mask comtaining the data types to look at
    ConditionMask   - a mask containing conditionals for doing the comparisons


Return Value:

    TRUE  - the version condition exists
    FALSE - the version condition does NOT exists

--*/

{
    OSVERSIONINFOEXW VersionInfoW;


    VersionInfoW.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    VersionInfoW.dwMajorVersion      = VersionInfo->dwMajorVersion;
    VersionInfoW.dwMinorVersion      = VersionInfo->dwMinorVersion;
    VersionInfoW.dwBuildNumber       = VersionInfo->dwBuildNumber;
    VersionInfoW.dwPlatformId        = VersionInfo->dwPlatformId;
    VersionInfoW.wServicePackMajor   = VersionInfo->wServicePackMajor;
    VersionInfoW.wServicePackMinor   = VersionInfo->wServicePackMinor;
    VersionInfoW.wSuiteMask          = VersionInfo->wSuiteMask;
    VersionInfoW.wProductType        = VersionInfo->wProductType;
    VersionInfoW.wReserved           = VersionInfo->wReserved;

    return VerifyVersionInfoW( &VersionInfoW, TypeMask, ConditionMask );
}

ULONG_PTR
BaseDllMapResourceIdA(
    LPCSTR lpId
    );

ULONG_PTR
BaseDllMapResourceIdW(
    LPCWSTR lpId
    );

VOID
BaseDllFreeResourceId(
    ULONG_PTR Id
    );

HRSRC
FindResourceA(
    HMODULE hModule,
    LPCSTR lpName,
    LPCSTR lpType
    )

/*++

Routine Description:

    This function determines the location of a resource in the specified
    resource file.  The lpName and lpType parameters define the resource
    name and type, respectively.

    If the high-order word of the lpName or lpType parameter is zero,
    the low-order word specifies the integer ID of the name or type of
    the given resource.  Otherwise, the parameters are pointers to
    null-terminated character strings.  If the first character of the
    string is a pound sign (#), the remaining characters represent a
    decimal number that specifies the integer ID of the resource's name
    or type.  For example, the string "#258" represents the integer ID
    258.

    To reduce the amount of memory required for the resources used by an
    application, applications should refer to their resources by integer
    ID instead of by name.

    An application must not call FindResource and the LoadResource
    function to load cursor, icon, or string resources.  Instead, it
    must load these resources by calling the following functions:

      - LoadCursor

      - LoadIcon

      - LoadString

    An application can call FindResource and LoadResource to load other
    predefined resource types.  However, it is recommended that the
    application load the corresponding resources by calling the
    following functions:

      - LoadAccelerators

      - LoadBitmap

      - LoadMenu

    The above six API calls are documented with the Graphical User
    Interface API specification.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource.  A value of NULL references the module handle
        associated with the image file that was used to create the
        current process.

    lpName - Points to a null-terminated character string that
        represents the name of the resource.

    lpType - Points to a null-terminated character string that
        represents the type name of the resource.  For predefined
        resource types, the lpType parameter should be one of the
        following values:

        RT_ACCELERATOR - Accelerator table

        RT_BITMAP - Bitmap resource

        RT_DIALOG - Dialog box

        RT_FONT - Font resource

        RT_FONTDIR - Font directory resource

        RT_MENU - Menu resource

        RT_RCDATA - User-defined resource (raw data)

Return Value:

    The return value identifies the named resource.  It is NULL if the
    requested resource cannot be found.

--*/


{
    NTSTATUS Status;
    ULONG_PTR IdPath[ 3 ];
    PVOID p;

    IdPath[ 0 ] = 0;
    IdPath[ 1 ] = 0;
    try {
        if ((IdPath[ 0 ] = BaseDllMapResourceIdA( lpType )) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else
        if ((IdPath[ 1 ] = BaseDllMapResourceIdA( lpName )) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else {
            IdPath[ 2 ] = 0;
            p = NULL;
            Status = LdrFindResource_U( BasepMapModuleHandle( hModule, TRUE ),
                                        IdPath,
                                        3,
                                        (PIMAGE_RESOURCE_DATA_ENTRY *)&p
                                      );
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    //
    // Free any strings allocated by BaseDllMapResourceIdA
    //
    BaseDllFreeResourceId( IdPath[ 0 ] );
    BaseDllFreeResourceId( IdPath[ 1 ] );

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( NULL );
        }
    else {
        return( (HRSRC)p );
        }
}

HRSRC
FindResourceExA(
    HMODULE hModule,
    LPCSTR lpType,
    LPCSTR lpName,
    WORD  wLanguage
    )

/*++

Routine Description:

    This function determines the location of a resource in the specified
    resource file.  The lpType, lpName and wLanguage parameters define
    the resource type, name and language respectively.

    If the high-order word of the lpType or lpName parameter
    is zero, the low-order word specifies the integer ID of the type, name
    or language of the given resource.  Otherwise, the parameters are pointers
    to null-terminated character strings.  If the first character of the
    string is a pound sign (#), the remaining characters represent a
    decimal number that specifies the integer ID of the resource's name
    or type.  For example, the string "#258" represents the integer ID
    258.

    If the wLanguage parameter is zero, then the current language
    associated with the calling thread will be used.

    To reduce the amount of memory required for the resources used by an
    application, applications should refer to their resources by integer
    ID instead of by name.

    An application must not call FindResource and the LoadResource
    function to load cursor, icon, or string resources.  Instead, it
    must load these resources by calling the following functions:

      - LoadCursor

      - LoadIcon

      - LoadString

    An application can call FindResource and LoadResource to load other
    predefined resource types.  However, it is recommended that the
    application load the corresponding resources by calling the
    following functions:

      - LoadAccelerators

      - LoadBitmap

      - LoadMenu

    The above six API calls are documented with the Graphical User
    Interface API specification.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource.  A value of NULL references the module handle
        associated with the image file that was used to create the
        current process.

    lpType - Points to a null-terminated character string that
        represents the type name of the resource.  For predefined
        resource types, the lpType parameter should be one of the
        following values:

        RT_ACCELERATOR - Accelerator table

        RT_BITMAP - Bitmap resource

        RT_DIALOG - Dialog box

        RT_FONT - Font resource

        RT_FONTDIR - Font directory resource

        RT_MENU - Menu resource

        RT_RCDATA - User-defined resource (raw data)

    lpName - Points to a null-terminated character string that
        represents the name of the resource.

    wLanguage -  represents the language of the resource.  If this parameter
        is zero then the current language associated with the calling
        thread is used.

Return Value:

    The return value identifies the named resource.  It is NULL if the
    requested resource cannot be found.

--*/


{
    NTSTATUS Status;
    ULONG_PTR IdPath[ 3 ];
    PVOID p;

    IdPath[ 0 ] = 0;
    IdPath[ 1 ] = 0;
    try {
        if ((IdPath[ 0 ] = BaseDllMapResourceIdA( lpType )) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else
        if ((IdPath[ 1 ] = BaseDllMapResourceIdA( lpName )) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else {
            IdPath[ 2 ] = (ULONG_PTR)wLanguage;
            p = NULL;
            Status = LdrFindResource_U( BasepMapModuleHandle( hModule, TRUE ),
                                        IdPath,
                                        3,
                                        (PIMAGE_RESOURCE_DATA_ENTRY *)&p
                                      );
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    //
    // Free any strings allocated by BaseDllMapResourceIdA
    //
    BaseDllFreeResourceId( IdPath[ 0 ] );
    BaseDllFreeResourceId( IdPath[ 1 ] );

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( NULL );
        }
    else {
        return( (HRSRC)p );
        }
}

HGLOBAL
LoadResource(
    HMODULE hModule,
    HRSRC hResInfo
    )

/*++

Routine Description:

    This function loads a resource identified by the hResInfo parameter
    from the executable file associated with the module specified by the
    hModule parameter.  The function loads the resource into memory only
    if it has not been previously loaded.  Otherwise, it retrieves a
    handle to the existing resource.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource.  A value of NULL references the module handle
        associated with the image file that was used to create the
        current process.

    hResInfo - Identifies the desired resource.  This handle is assumed
        to have been returned by the FindResource function.

Return Value:

    The return value identifies the global memory block that contains
    the data associated with the resource.  It is NULL if no such
    resource exists.

--*/

{
    NTSTATUS Status;
    PVOID p;

    try {
        Status = LdrAccessResource( BasepMapModuleHandle( hModule, TRUE ),
                                    (PIMAGE_RESOURCE_DATA_ENTRY)hResInfo,
                                    &p,
                                    (PULONG)NULL
                                  );
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( NULL );
        }
    else {
        return( (HGLOBAL)p );
        }
}

DWORD
SizeofResource(
    HMODULE hModule,
    HRSRC hResInfo
    )

/*++

Routine Description:

    This function supplies the size (in bytes) of the specified
    resource.

    The value returned may be larger than the actual resource due to
    alignment.  An application should not rely upon this value for the
    exact size of a resource.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource.  A value of NULL references the module handle
        associated with the image file that was used to create the
        current process.

    hResInfo - Identifies the desired resource.  This handle is assumed
        to have been returned by the FindResource function.

Return Value:

    The return value specifies the number of bytes in the resource.  It
    is zero if the resource cannot be found.

--*/

{
    NTSTATUS Status;
    ULONG cb;

    try {
        Status = LdrAccessResource( BasepMapModuleHandle( hModule, TRUE ),
                                    (PIMAGE_RESOURCE_DATA_ENTRY)hResInfo,
                                    (PVOID *)NULL,
                                    &cb
                                  );
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( 0 );
        }
    else {
        return( (DWORD)cb );
        }
}

#ifdef _X86_
BOOL
__stdcall
_ResourceCallEnumTypeRoutine(
    ENUMRESTYPEPROCA EnumProc,
    HMODULE hModule,
    LPSTR lpType,
    LONG lParam);
#else
#define _ResourceCallEnumTypeRoutine( EnumProc, hModule, lpType, lParam ) \
    (*EnumProc)(hModule, lpType, lParam)
#endif


BOOL
WINAPI
EnumResourceTypesA(
    HMODULE hModule,
    ENUMRESTYPEPROCA lpEnumFunc,
    LONG_PTR lParam
    )

/*++

Routine Description:

    This function enumerates all of the resource type names contained in
    a module.  It enumerates them by passing each type name to the callback
    function pointed to by the lpEnumFunc parameter.

    The EnumResourceTypes function continues to enumerate type names until
    called function returns FALSE or the last type name in the module has
    been enumerated.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource type names to be enumerated.  A value of NULL
        references the module handle associated with the image file that
        was used to create the current process.

    lpEnumFunc - Points to the callback function that will be called
        for each enumerated resource type name.

    lParam - Specifies the value to be passed to the callback function
        for the application's use.

Return Value:

    TRUE - All resource type names were enumerated.

    FALSE/NULL - The enumeration was terminated before all resource type
        names were enumerated.

Callback Function:

    BOOL
    EnumFunc(
        HMODULE hModule,
        LPSTR lpType,
        LONG lParam
        );

    Routine Description:

        EnumFunc is a placeholder for the application-supplied function name.

    Arguments:

        hModule - Identifies the module whose executable file contains the
            resource type names to be enumerated.  A value of NULL
            references the module handle associated with the image file that
            was used to create the current process.

        lpType - Points to a null-terminated character string that
            represents the type name of the resource.  For predefined
            resource types, the lpType parameter will be one of the
            following values:

            RT_ACCELERATOR - Accelerator table

            RT_BITMAP - Bitmap resource

            RT_DIALOG - Dialog box

            RT_FONT - Font resource

            RT_FONTDIR - Font directory resource

            RT_MENU - Menu resource

            RT_RCDATA - User-defined resource (raw data)

            RT_STRING - String table

            RT_MESSAGETABLE - Message table

            RT_CURSOR - Hardware dependent cursor resource

            RT_GROUP_CURSOR - Directory of cursor resources

            RT_ICON - Hardware dependent cursor resource

            RT_GROUP_ICON - Directory of icon resources

        lParam - Specifies the 32-bit arugment of the EnumResourceTypes
            function.

    Return Value:

        TRUE - Continue the enumeration.

        FALSE/NULL - Stop enumerating resource type names.

--*/

{
    BOOL Result;
    NTSTATUS Status;
    ULONG i;
    HANDLE DllHandle;
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory, TopResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry;
    PIMAGE_RESOURCE_DIR_STRING_U ResourceNameString;
    LPSTR lpType;
    LPSTR Buffer;
    ULONG BufferLength;
    ULONG Length;

    DllHandle = BasepMapModuleHandle( hModule, TRUE );
    TopResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
        RtlImageDirectoryEntryToData( (PVOID)DllHandle,
                                      TRUE,
                                      IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                      &i
                                     );
    if (!TopResourceDirectory) {
        BaseSetLastNTError( STATUS_RESOURCE_DATA_NOT_FOUND );
        return FALSE;
        }

    Status = LdrFindResourceDirectory_U( (PVOID)DllHandle,
                                         NULL,
                                         0,
                                         &ResourceDirectory
                                       );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return FALSE;
        }

    Buffer = NULL;
    BufferLength = 0;
    Result = TRUE;
    try {
        ResourceDirectoryEntry =
            (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResourceDirectory+1);
        for (i=0; i<ResourceDirectory->NumberOfNamedEntries; i++) {
            ResourceNameString = (PIMAGE_RESOURCE_DIR_STRING_U)
                ((PCHAR)TopResourceDirectory + ResourceDirectoryEntry->NameOffset);
            if ((ULONG)(ResourceNameString->Length+1) >= BufferLength) {
                if (Buffer) {
                    RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
                    Buffer = NULL;
                    }

                BufferLength = (ResourceNameString->Length + 64) & ~63;
                Buffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), BufferLength );
                }

            Status = RtlUnicodeToMultiByteN( Buffer,
                                             BufferLength - 1,
                                             &Length,
                                             ResourceNameString->NameString,
                                             ResourceNameString->Length * sizeof( WCHAR )
                                           );

            if (!NT_SUCCESS( Status )) {
                BaseSetLastNTError( Status );
                return FALSE;
            }

            Buffer[ Length ] = '\0';

            if (!_ResourceCallEnumTypeRoutine(lpEnumFunc, hModule, (LPSTR)Buffer, lParam )) {
                Result = FALSE;
                break;
                }

            ResourceDirectoryEntry++;
            }

        if (Result) {
            for (i=0; i<ResourceDirectory->NumberOfIdEntries; i++) {
                lpType = (LPSTR)ResourceDirectoryEntry->Id;
                if (!_ResourceCallEnumTypeRoutine(lpEnumFunc, hModule, lpType, lParam )) {
                    Result = FALSE;
                    break;
                    }

                ResourceDirectoryEntry++;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    if (Buffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
        }

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
        }
    else {
        return Result;
        }
}


#ifdef _X86_
BOOL
__stdcall
_ResourceCallEnumNameRoutine(
    ENUMRESNAMEPROCA EnumProc,
    HMODULE hModule,
    LPCSTR lpType,
    LPSTR lpName,
    LONG lParam);
#else
#define _ResourceCallEnumNameRoutine( EnumProc, hModule, lpType, lpName, lParam ) \
    (*EnumProc)(hModule, lpType, lpName, lParam)
#endif

BOOL
WINAPI
EnumResourceNamesA(
    HMODULE hModule,
    LPCSTR lpType,
    ENUMRESNAMEPROCA lpEnumFunc,
    LONG_PTR lParam
    )

/*++

Routine Description:

    This function enumerates all of the resource names for a specific
    resource type name contained in a module.  It enumerates them by
    passing each resource name and type name to the callback function
    pointed to by the lpEnumFunc parameter.

    The EnumResourceNames function continues to enumerate resource names
    until called function returns FALSE or the last resource name for the
    specified resource type name has been enumerated.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource names to be enumerated.  A value of NULL references the
        module handle associated with the image file that was used to
        create the current process.

    lpType - Points to a null-terminated character string that
        represents the type name of the resources whose names are to be
        enumerated.  For predefined resource types, the lpType parameter
        should be one of the following values:

        RT_ACCELERATOR - Accelerator table

        RT_BITMAP - Bitmap resource

        RT_DIALOG - Dialog box

        RT_FONT - Font resource

        RT_FONTDIR - Font directory resource

        RT_MENU - Menu resource

        RT_RCDATA - User-defined resource (raw data)

    lpEnumFunc - Points to the callback function that will be called
        for each enumerated resource name.

    lParam - Specifies the value to be passed to the callback function
        for the application's use.

Return Value:

    TRUE - All resource names were enumerated.

    FALSE/NULL - The enumeration was terminated before all resource
        names were enumerated.

Callback Function:

    BOOL
    EnumFunc(
        HMODULE hModule,
        LPSTR lpType,
        LPSTR lpName,
        LONG lParam
        );

    Routine Description:

        EnumFunc is a placeholder for the application-supplied function name.

    Arguments:

        hModule - Identifies the module whose executable file contains
            the resource names to be enumerated.  A value of NULL
            references the module handle associated with the image file
            that was used to create the current process.

        lpType - Points to a null-terminated character string that
            represents the type name of the resource being enumerated.
            For predefined resource types, the lpType parameter will be
            one of the following values:

            RT_ACCELERATOR - Accelerator table

            RT_BITMAP - Bitmap resource

            RT_DIALOG - Dialog box

            RT_FONT - Font resource

            RT_FONTDIR - Font directory resource

            RT_MENU - Menu resource

            RT_RCDATA - User-defined resource (raw data)

            RT_STRING - String table

            RT_MESSAGETABLE - Message table

            RT_CURSOR - Hardware dependent cursor resource

            RT_GROUP_CURSOR - Directory of cursor resources

            RT_ICON - Hardware dependent cursor resource

            RT_GROUP_ICON - Directory of icon resources

        lpName - Points to a null-terminated character string that
            represents the name of the resource being enumerated.

        lParam - Specifies the 32-bit arugment of the EnumResourceNames
            function.

    Return Value:

        TRUE - Continue the enumeration.

        FALSE/NULL - Stop enumerating resource names.

--*/

{
    BOOL Result;
    NTSTATUS Status;
    ULONG i;
    ULONG_PTR IdPath[ 1 ];
    HANDLE DllHandle;
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory, TopResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry;
    PIMAGE_RESOURCE_DIR_STRING_U ResourceNameString;
    LPSTR lpName;
    PCHAR Buffer;
    ULONG BufferLength;
    ULONG Length;

    if ((IdPath[ 0 ] = BaseDllMapResourceIdA( lpType )) == -1) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
        }

    DllHandle = BasepMapModuleHandle( hModule, TRUE );
    TopResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
        RtlImageDirectoryEntryToData( (PVOID)DllHandle,
                                      TRUE,
                                      IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                      &i
                                    );
    if (!TopResourceDirectory) {
        Status = STATUS_RESOURCE_DATA_NOT_FOUND;
        }
    else {
        Status = LdrFindResourceDirectory_U( (PVOID)DllHandle,
                                             IdPath,
                                             1,
                                             &ResourceDirectory
                                           );
        }

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        BaseDllFreeResourceId( IdPath[ 0 ] );
        return FALSE;
        }

    Buffer = NULL;
    BufferLength = 0;
    Result = TRUE;
    SetLastError( NO_ERROR );
    try {
        ResourceDirectoryEntry =
            (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResourceDirectory+1);
        for (i=0; i<ResourceDirectory->NumberOfNamedEntries; i++) {
            ResourceNameString = (PIMAGE_RESOURCE_DIR_STRING_U)
                ((PCHAR)TopResourceDirectory + ResourceDirectoryEntry->NameOffset);
            if ((ULONG)(ResourceNameString->Length+1) >= BufferLength) {
                if (Buffer) {
                    RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
                    Buffer = NULL;
                    }

                BufferLength = (ResourceNameString->Length + 64) & ~63;
                Buffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), BufferLength );
                }

            Status = RtlUnicodeToMultiByteN( Buffer,
                                             BufferLength - 1,
                                             &Length,
                                             ResourceNameString->NameString,
                                             ResourceNameString->Length * sizeof( WCHAR )
                                           );

            if (!NT_SUCCESS( Status )) {
                BaseSetLastNTError( Status );
                Result = FALSE;
                break;
                }

            Buffer[ Length ] = '\0';

            if (!_ResourceCallEnumNameRoutine(lpEnumFunc, hModule, lpType, (LPSTR)Buffer, lParam )) {
                Result = FALSE;
                break;
                }

            ResourceDirectoryEntry++;
            }

        if (Result) {
            for (i=0; i<ResourceDirectory->NumberOfIdEntries; i++) {
                lpName = (LPSTR)ResourceDirectoryEntry->Id;
                if (!_ResourceCallEnumNameRoutine(lpEnumFunc, hModule, lpType, lpName, lParam )) {
                    Result = FALSE;
                    break;
                    }

                ResourceDirectoryEntry++;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        Result = FALSE;
        }

    if (Buffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
        }

    //
    // Free any string allocated by BaseDllMapResourceIdA
    //
    BaseDllFreeResourceId( IdPath[ 0 ] );

    return Result;
}


#ifdef _X86_
BOOL
__stdcall
_ResourceCallEnumLangRoutine(
    ENUMRESLANGPROCA EnumProc,
    HMODULE hModule,
    LPCSTR lpType,
    LPCSTR lpName,
    WORD wLanguage,
    LONG lParam);
#else
#define _ResourceCallEnumLangRoutine( EnumProc, hModule, lpType, lpName, wLanguage, lParam ) \
    (*EnumProc)(hModule, lpType, lpName, wLanguage, lParam)
#endif

BOOL
WINAPI
EnumResourceLanguagesA(
    HMODULE hModule,
    LPCSTR lpType,
    LPCSTR lpName,
    ENUMRESLANGPROCA lpEnumFunc,
    LONG_PTR lParam
    )

/*++

Routine Description:

    This function enumerates all of the language specific resources
    contained in a module for a given resource type and name ID.  It
    enumerates them by passing each resource type, name and language to
    the callback function pointed to by the lpEnumFunc parameter.

    The EnumResourceLanguares function continues to enumerate resources
    until called function returns FALSE or the last resource for
    the specified language has been enumerated.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource names to be enumerated.  A value of NULL references the
        module handle associated with the image file that was used to
        create the current process.

    lpType - Points to a null-terminated character string that
        represents the type name of the resources whose names are to be
        enumerated.  For predefined resource types, the lpType parameter
        should be one of the following values:

        RT_ACCELERATOR - Accelerator table

        RT_BITMAP - Bitmap resource

        RT_DIALOG - Dialog box

        RT_FONT - Font resource

        RT_FONTDIR - Font directory resource

        RT_MENU - Menu resource

        RT_RCDATA - User-defined resource (raw data)

    lpName - Points to a null-terminated character string that
        represents the name of the resource being enumerated.

    lpEnumFunc - Points to the callback function that will be called
        for each enumerated resource name.

    lParam - Specifies the value to be passed to the callback function
        for the application's use.

Return Value:

    TRUE - All resource names were enumerated.

    FALSE/NULL - The enumeration was terminated before all resource
        names were enumerated.

Callback Function:

    BOOL
    EnumFunc(
        HMODULE hModule,
        LPSTR lpType,
        LPSTR lpName,
        WORD  wLanguage,
        LONG lParam
        );

    Routine Description:

        EnumFunc is a placeholder for the application-supplied function name.

    Arguments:

        hModule - Identifies the module whose executable file contains
            the resource names to be enumerated.  A value of NULL
            references the module handle associated with the image file
            that was used to create the current process.

        lpType - Points to a null-terminated character string that
            represents the type name of the resource being enumerated.
            For predefined resource types, the lpType parameter will be
            one of the following values:

            RT_ACCELERATOR - Accelerator table

            RT_BITMAP - Bitmap resource

            RT_DIALOG - Dialog box

            RT_FONT - Font resource

            RT_FONTDIR - Font directory resource

            RT_MENU - Menu resource

            RT_RCDATA - User-defined resource (raw data)

            RT_STRING - String table

            RT_MESSAGETABLE - Message table

            RT_CURSOR - Hardware dependent cursor resource

            RT_GROUP_CURSOR - Directory of cursor resources

            RT_ICON - Hardware dependent cursor resource

            RT_GROUP_ICON - Directory of icon resources

        lpName - Points to a null-terminated character string that
            represents the name of the resource being enumerated.

        wLanguage -  represents the language of the resource.

        lParam - Specifies the 32-bit arugment of the EnumResourceNames
            function.

    Return Value:

        TRUE - Continue the enumeration.

        FALSE/NULL - Stop enumerating resource names.

--*/

{
    BOOL Result;
    NTSTATUS Status;
    ULONG i;
    ULONG_PTR IdPath[ 2 ];
    HANDLE DllHandle;
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory, TopResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry;
    USHORT wLanguage;

    IdPath[ 1 ] = 0;
    if ((IdPath[ 0 ] = BaseDllMapResourceIdA( lpType )) == -1) {
        Status = STATUS_INVALID_PARAMETER;
        }
    else
    if ((IdPath[ 1 ] = BaseDllMapResourceIdA( lpName )) == -1) {
        Status = STATUS_INVALID_PARAMETER;
        }
    else {
        DllHandle = BasepMapModuleHandle( hModule, TRUE );
        TopResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
            RtlImageDirectoryEntryToData( (PVOID)DllHandle,
                                          TRUE,
                                          IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                          &i
                                        );
        if (!TopResourceDirectory) {
            Status = STATUS_RESOURCE_DATA_NOT_FOUND;
            }
        else {
            Status = LdrFindResourceDirectory_U( (PVOID)DllHandle,
                                                 IdPath,
                                                 2,
                                                 &ResourceDirectory
                                               );
            }
        }

    if (!NT_SUCCESS( Status )) {
        BaseDllFreeResourceId( IdPath[ 0 ] );
        BaseDllFreeResourceId( IdPath[ 1 ] );
        BaseSetLastNTError( Status );
        return FALSE;
        }

    Result = TRUE;
    SetLastError( NO_ERROR );
    try {
        ResourceDirectoryEntry =
            (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResourceDirectory+1);
        if (ResourceDirectory->NumberOfNamedEntries != 0) {
            BaseSetLastNTError( STATUS_INVALID_IMAGE_FORMAT );
            Result = FALSE;
            }
        else {
            for (i=0; i<ResourceDirectory->NumberOfIdEntries; i++) {
                wLanguage = ResourceDirectoryEntry->Id;
                if (!_ResourceCallEnumLangRoutine(lpEnumFunc, hModule, lpType, lpName, wLanguage, lParam )) {
                    Result = FALSE;
                    break;
                    }

                ResourceDirectoryEntry++;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        Result = FALSE;
        }

    //
    // Free any strings allocated by BaseDllMapResourceIdA
    //
    BaseDllFreeResourceId( IdPath[ 0 ] );
    BaseDllFreeResourceId( IdPath[ 1 ] );
    return Result;
}


BOOL
WINAPI
FreeResource(
    HGLOBAL hResData
    )
{
    //
    // Can't fail so return Win 3.x success code.
    //

    return FALSE;
}

LPVOID
WINAPI
LockResource(
    HGLOBAL hResData
    )
{
    return( (LPVOID)hResData );
}


HRSRC
FindResourceW(
    HMODULE hModule,
    LPCWSTR lpName,
    LPCWSTR lpType
    )

/*++

Routine Description:

    This function determines the location of a resource in the specified
    resource file.  The lpName and lpType parameters define the resource
    name and type, respectively.

    If the high-order word of the lpName or lpType parameter is zero,
    the low-order word specifies the integer ID of the name or type of
    the given resource.  Otherwise, the parameters are pointers to
    null-terminated character strings.  If the first character of the
    string is a pound sign (#), the remaining characters represent a
    decimal number that specifies the integer ID of the resource's name
    or type.  For example, the string "#258" represents the integer ID
    258.

    To reduce the amount of memory required for the resources used by an
    application, applications should refer to their resources by integer
    ID instead of by name.

    An application must not call FindResource and the LoadResource
    function to load cursor, icon, or string resources.  Instead, it
    must load these resources by calling the following functions:

      - LoadCursor

      - LoadIcon

      - LoadString

    An application can call FindResource and LoadResource to load other
    predefined resource types.  However, it is recommended that the
    application load the corresponding resources by calling the
    following functions:

      - LoadAccelerators

      - LoadBitmap

      - LoadMenu

    The above six API calls are documented with the Graphical User
    Interface API specification.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource.  A value of NULL references the module handle
        associated with the image file that was used to create the
        current process.

    lpName - Points to a null-terminated character string that
        represents the name of the resource.

    lpType - Points to a null-terminated character string that
        represents the type name of the resource.  For predefined
        resource types, the lpType parameter should be one of the
        following values:

        RT_ACCELERATOR - Accelerator table

        RT_BITMAP - Bitmap resource

        RT_DIALOG - Dialog box

        RT_FONT - Font resource

        RT_FONTDIR - Font directory resource

        RT_MENU - Menu resource

        RT_RCDATA - User-defined resource (raw data)

Return Value:

    The return value identifies the named resource.  It is NULL if the
    requested resource cannot be found.

--*/


{
    NTSTATUS Status;
    ULONG_PTR IdPath[ 3 ];
    PVOID p;

    IdPath[ 0 ] = 0;
    IdPath[ 1 ] = 0;
    try {
        if ((IdPath[ 0 ] = BaseDllMapResourceIdW( lpType )) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else
        if ((IdPath[ 1 ] = BaseDllMapResourceIdW( lpName )) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else {
            IdPath[ 2 ] = 0;
            p = NULL;
            Status = LdrFindResource_U( BasepMapModuleHandle( hModule, TRUE ),
                                        IdPath,
                                        3,
                                        (PIMAGE_RESOURCE_DATA_ENTRY *)&p
                                      );
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    //
    // Free any strings allocated by BaseDllMapResourceIdW
    //
    BaseDllFreeResourceId( IdPath[ 0 ] );
    BaseDllFreeResourceId( IdPath[ 1 ] );

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( NULL );
        }
    else {
        return( (HRSRC)p );
        }
}

HRSRC
FindResourceExW(
    HMODULE hModule,
    LPCWSTR lpType,
    LPCWSTR lpName,
    WORD  wLanguage
    )

/*++

Routine Description:

    This function determines the location of a resource in the specified
    resource file.  The lpType, lpName and wLanguage parameters define
    the resource type, name and language respectively.

    If the high-order word of the lpType or lpName parameter
    is zero, the low-order word specifies the integer ID of the type, name
    or language of the given resource.  Otherwise, the parameters are pointers
    to null-terminated character strings.  If the first character of the
    string is a pound sign (#), the remaining characters represent a
    decimal number that specifies the integer ID of the resource's name
    or type.  For example, the string "#258" represents the integer ID
    258.

    If the wLanguage parameter is zero, then the current language
    associated with the calling thread will be used.

    To reduce the amount of memory required for the resources used by an
    application, applications should refer to their resources by integer
    ID instead of by name.

    An application must not call FindResource and the LoadResource
    function to load cursor, icon, or string resources.  Instead, it
    must load these resources by calling the following functions:

      - LoadCursor

      - LoadIcon

      - LoadString

    An application can call FindResource and LoadResource to load other
    predefined resource types.  However, it is recommended that the
    application load the corresponding resources by calling the
    following functions:

      - LoadAccelerators

      - LoadBitmap

      - LoadMenu

    The above six API calls are documented with the Graphical User
    Interface API specification.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource.  A value of NULL references the module handle
        associated with the image file that was used to create the
        current process.

    lpType - Points to a null-terminated character string that
        represents the type name of the resource.  For predefined
        resource types, the lpType parameter should be one of the
        following values:

        RT_ACCELERATOR - Accelerator table

        RT_BITMAP - Bitmap resource

        RT_DIALOG - Dialog box

        RT_FONT - Font resource

        RT_FONTDIR - Font directory resource

        RT_MENU - Menu resource

        RT_RCDATA - User-defined resource (raw data)

    lpName - Points to a null-terminated character string that
        represents the name of the resource.

    wLanguage -  represents the language of the resource.  If this parameter
        is zero then the current language associated with the calling
        thread is used.

Return Value:

    The return value identifies the named resource.  It is NULL if the
    requested resource cannot be found.

--*/


{
    NTSTATUS Status;
    ULONG_PTR IdPath[ 3 ];
    PVOID p;

    IdPath[ 0 ] = 0;
    IdPath[ 1 ] = 0;
    try {
        if ((IdPath[ 0 ] = BaseDllMapResourceIdW( lpType )) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else
        if ((IdPath[ 1 ] = BaseDllMapResourceIdW( lpName )) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else {
            IdPath[ 2 ] = (ULONG_PTR)wLanguage;
            p = NULL;
            Status = LdrFindResource_U( BasepMapModuleHandle( hModule, TRUE ),
                                      IdPath,
                                      3,
                                      (PIMAGE_RESOURCE_DATA_ENTRY *)&p
                                    );
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    //
    // Free any strings allocated by BaseDllMapResourceIdW
    //

    BaseDllFreeResourceId( IdPath[ 0 ] );
    BaseDllFreeResourceId( IdPath[ 1 ] );

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( NULL );
        }
    else {
        return( (HRSRC)p );
        }
}


BOOL
APIENTRY
EnumResourceTypesW(
    HMODULE hModule,
    ENUMRESTYPEPROCW lpEnumFunc,
    LONG_PTR lParam
    )

/*++

Routine Description:

    This function enumerates all of the resource type names contained in
    a module.  It enumerates them by passing each type name to the callback
    function pointed to by the lpEnumFunc parameter.

    The EnumResourceTypes function continues to enumerate type names until
    called function returns FALSE or the last type name in the module has
    been enumerated.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource type names to be enumerated.  A value of NULL
        references the module handle associated with the image file that
        was used to create the current process.

    lpEnumFunc - Points to the callback function that will be called
        for each enumerated resource type name.

    lParam - Specifies the value to be passed to the callback function
        for the application's use.

Return Value:

    TRUE - All resource type names were enumerated.

    FALSE/NULL - The enumeration was terminated before all resource type
        names were enumerated.

Callback Function:

    BOOL
    EnumFunc(
        HMODULE hModule,
        LPWSTR lpType,
        LONG lParam
        );

    Routine Description:

        EnumFunc is a placeholder for the application-supplied function name.

    Arguments:

        hModule - Identifies the module whose executable file contains the
            resource type names to be enumerated.  A value of NULL
            references the module handle associated with the image file that
            was used to create the current process.

        lpType - Points to a null-terminated character string that
            represents the type name of the resource.  For predefined
            resource types, the lpType parameter will be one of the
            following values:

            RT_ACCELERATOR - Accelerator table

            RT_BITMAP - Bitmap resource

            RT_DIALOG - Dialog box

            RT_FONT - Font resource

            RT_FONTDIR - Font directory resource

            RT_MENU - Menu resource

            RT_RCDATA - User-defined resource (raw data)

            RT_STRING - String table

            RT_MESSAGETABLE - Message table

            RT_CURSOR - Hardware dependent cursor resource

            RT_GROUP_CURSOR - Directory of cursor resources

            RT_ICON - Hardware dependent cursor resource

            RT_GROUP_ICON - Directory of icon resources

        lParam - Specifies the 32-bit arugment of the EnumResourceTypes
            function.

    Return Value:

        TRUE - Continue the enumeration.

        FALSE/NULL - Stop enumerating resource type names.

--*/

{
    BOOL Result;
    NTSTATUS Status;
    ULONG i;
    HANDLE DllHandle;
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory, TopResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry;
    PIMAGE_RESOURCE_DIR_STRING_U ResourceNameString;
    LPWSTR lpType;
    LPWSTR Buffer;
    ULONG BufferLength;

    DllHandle = BasepMapModuleHandle( hModule, TRUE );
    TopResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
        RtlImageDirectoryEntryToData( (PVOID)DllHandle,
                                      TRUE,
                                      IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                      &i
                                     );
    if (!TopResourceDirectory) {
        BaseSetLastNTError( STATUS_RESOURCE_DATA_NOT_FOUND );
        return FALSE;
        }

    Status = LdrFindResourceDirectory_U( (PVOID)DllHandle,
                                         NULL,
                                         0,
                                         &ResourceDirectory
                                       );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return FALSE;
        }

    Buffer = NULL;
    BufferLength = 0;
    Result = TRUE;
    try {
        ResourceDirectoryEntry =
            (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResourceDirectory+1);
        for (i=0; i<ResourceDirectory->NumberOfNamedEntries; i++) {
            ResourceNameString = (PIMAGE_RESOURCE_DIR_STRING_U)
                ((PCHAR)TopResourceDirectory + ResourceDirectoryEntry->NameOffset);
            if ((ULONG)((ResourceNameString->Length+1) * sizeof( WCHAR )) >= BufferLength) {
                if (Buffer) {
                    RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
                    Buffer = NULL;
                    }

                BufferLength = ((ResourceNameString->Length * sizeof( WCHAR )) + 64) & ~63;
                Buffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), BufferLength );
                if (Buffer == NULL) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    Result = FALSE;
                    break;
                    }
                }
            RtlMoveMemory( Buffer,
                           ResourceNameString->NameString,
                           ResourceNameString->Length * sizeof( WCHAR )
                         );
            Buffer[ ResourceNameString->Length ] = UNICODE_NULL;

            if (!_ResourceCallEnumTypeRoutine((ENUMRESTYPEPROCA)lpEnumFunc, hModule, (LPSTR)Buffer, lParam )) {
                Result = FALSE;
                break;
                }

            ResourceDirectoryEntry++;
            }

        if (Result) {
            for (i=0; i<ResourceDirectory->NumberOfIdEntries; i++) {
                lpType = (LPWSTR)ResourceDirectoryEntry->Id;
                if (!_ResourceCallEnumTypeRoutine((ENUMRESTYPEPROCA)lpEnumFunc, hModule, (LPSTR)lpType, lParam )) {
                    Result = FALSE;
                    break;
                    }

                ResourceDirectoryEntry++;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    if (Buffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
        }

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FALSE );
        }
    else {
        return Result;
        }
}


BOOL
APIENTRY
EnumResourceNamesW(
    HMODULE hModule,
    LPCWSTR lpType,
    ENUMRESNAMEPROCW lpEnumFunc,
    LONG_PTR lParam
    )

/*++

Routine Description:

    This function enumerates all of the resource names for a specific
    resource type name contained in a module.  It enumerates them by
    passing each resource name and type name to the callback function
    pointed to by the lpEnumFunc parameter.

    The EnumResourceNames function continues to enumerate resource names
    until called function returns FALSE or the last resource name for the
    specified resource type name has been enumerated.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource names to be enumerated.  A value of NULL references the
        module handle associated with the image file that was used to
        create the current process.

    lpType - Points to a null-terminated character string that
        represents the type name of the resources whose names are to be
        enumerated.  For predefined resource types, the lpType parameter
        should be one of the following values:

        RT_ACCELERATOR - Accelerator table

        RT_BITMAP - Bitmap resource

        RT_DIALOG - Dialog box

        RT_FONT - Font resource

        RT_FONTDIR - Font directory resource

        RT_MENU - Menu resource

        RT_RCDATA - User-defined resource (raw data)

    lpEnumFunc - Points to the callback function that will be called
        for each enumerated resource name.

    lParam - Specifies the value to be passed to the callback function
        for the application's use.

Return Value:

    TRUE - All resource names were enumerated.

    FALSE/NULL - The enumeration was terminated before all resource
        names were enumerated.

Callback Function:

    BOOL
    EnumFunc(
        HMODULE hModule,
        LPWSTR lpType,
        LPWSTR lpName,
        LONG lParam
        );

    Routine Description:

        EnumFunc is a placeholder for the application-supplied function name.

    Arguments:

        hModule - Identifies the module whose executable file contains
            the resource names to be enumerated.  A value of NULL
            references the module handle associated with the image file
            that was used to create the current process.

        lpType - Points to a null-terminated character string that
            represents the type name of the resource being enumerated.
            For predefined resource types, the lpType parameter will be
            one of the following values:

            RT_ACCELERATOR - Accelerator table

            RT_BITMAP - Bitmap resource

            RT_DIALOG - Dialog box

            RT_FONT - Font resource

            RT_FONTDIR - Font directory resource

            RT_MENU - Menu resource

            RT_RCDATA - User-defined resource (raw data)

            RT_STRING - String table

            RT_MESSAGETABLE - Message table

            RT_CURSOR - Hardware dependent cursor resource

            RT_GROUP_CURSOR - Directory of cursor resources

            RT_ICON - Hardware dependent cursor resource

            RT_GROUP_ICON - Directory of icon resources

        lpName - Points to a null-terminated character string that
            represents the name of the resource being enumerated.

        lParam - Specifies the 32-bit arugment of the EnumResourceNames
            function.

    Return Value:

        TRUE - Continue the enumeration.

        FALSE/NULL - Stop enumerating resource names.

--*/

{
    BOOL Result;
    NTSTATUS Status;
    ULONG i;
    ULONG_PTR IdPath[ 1 ];
    HANDLE DllHandle;
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory, TopResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry;
    PIMAGE_RESOURCE_DIR_STRING_U ResourceNameString;
    LPWSTR lpName;
    LPWSTR Buffer;
    ULONG BufferLength;

    if ((IdPath[ 0 ] = BaseDllMapResourceIdW( lpType )) == -1) {
        Status = STATUS_INVALID_PARAMETER;
        }
    else {
        DllHandle = BasepMapModuleHandle( hModule, TRUE );
        TopResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
            RtlImageDirectoryEntryToData( (PVOID)DllHandle,
                                          TRUE,
                                          IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                          &i
                                        );
        if (!TopResourceDirectory) {
            Status = STATUS_RESOURCE_DATA_NOT_FOUND;
            }
        else {
            Status = LdrFindResourceDirectory_U( (PVOID)DllHandle,
                                                 IdPath,
                                                 1,
                                                 &ResourceDirectory
                                               );
            }
        }

    if (!NT_SUCCESS( Status )) {
        BaseDllFreeResourceId( IdPath[ 0 ] );
        BaseSetLastNTError( Status );
        return FALSE;
        }

    Buffer = NULL;
    BufferLength = 0;
    Result = TRUE;
    try {
        ResourceDirectoryEntry =
            (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResourceDirectory+1);
        for (i=0; i<ResourceDirectory->NumberOfNamedEntries; i++) {
            ResourceNameString = (PIMAGE_RESOURCE_DIR_STRING_U)
                ((PCHAR)TopResourceDirectory + ResourceDirectoryEntry->NameOffset);
            if ((ULONG)((ResourceNameString->Length+1) * sizeof( WCHAR )) >= BufferLength) {
                if (Buffer) {
                    RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
                    Buffer = NULL;
                    }

                BufferLength = ((ResourceNameString->Length * sizeof( WCHAR )) + 64) & ~63;
                Buffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), BufferLength );
                if (Buffer == NULL) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    Result = FALSE;
                    break;
                    }
                }
            RtlMoveMemory( Buffer,
                           ResourceNameString->NameString,
                           ResourceNameString->Length * sizeof( WCHAR )
                         );
            Buffer[ ResourceNameString->Length ] = UNICODE_NULL;

            if (!_ResourceCallEnumNameRoutine((ENUMRESNAMEPROCA)lpEnumFunc, hModule, (LPSTR)lpType, (LPSTR)Buffer, lParam )) {
                Result = FALSE;
                break;
                }

            ResourceDirectoryEntry++;
            }

        if (Result) {
            for (i=0; i<ResourceDirectory->NumberOfIdEntries; i++) {
                lpName = (LPWSTR)ResourceDirectoryEntry->Id;
                if (!_ResourceCallEnumNameRoutine((ENUMRESNAMEPROCA)lpEnumFunc, hModule, (LPSTR)lpType, (LPSTR)lpName, lParam )) {
                    Result = FALSE;
                    break;
                    }

                ResourceDirectoryEntry++;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        Result = FALSE;
        }

    if (Buffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
        }

    //
    // Free any string allocated by BaseDllMapResourceIdW
    //
    BaseDllFreeResourceId( IdPath[ 0 ] );

    return Result;
}


BOOL
APIENTRY
EnumResourceLanguagesW(
    HMODULE hModule,
    LPCWSTR lpType,
    LPCWSTR lpName,
    ENUMRESLANGPROCW lpEnumFunc,
    LONG_PTR lParam
    )

/*++

Routine Description:

    This function enumerates all of the language specific resources
    contained in a module for a given resource type and name ID.  It
    enumerates them by passing each resource type, name and language to
    the callback function pointed to by the lpEnumFunc parameter.

    The EnumResourceLanguares function continues to enumerate resources
    until called function returns FALSE or the last resource for
    the specified language has been enumerated.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource names to be enumerated.  A value of NULL references the
        module handle associated with the image file that was used to
        create the current process.

    lpType - Points to a null-terminated character string that
        represents the type name of the resources whose names are to be
        enumerated.  For predefined resource types, the lpType parameter
        should be one of the following values:

        RT_ACCELERATOR - Accelerator table

        RT_BITMAP - Bitmap resource

        RT_DIALOG - Dialog box

        RT_FONT - Font resource

        RT_FONTDIR - Font directory resource

        RT_MENU - Menu resource

        RT_RCDATA - User-defined resource (raw data)

    lpName - Points to a null-terminated character string that
        represents the name of the resource being enumerated.

    lpEnumFunc - Points to the callback function that will be called
        for each enumerated resource name.

    lParam - Specifies the value to be passed to the callback function
        for the application's use.

Return Value:

    TRUE - All resource names were enumerated.

    FALSE/NULL - The enumeration was terminated before all resource
        names were enumerated.

Callback Function:

    BOOL
    EnumFunc(
        HMODULE hModule,
        LPWSTR lpType,
        LPWSTR lpName,
        WORD  wLanguage,
        LONG lParam
        );

    Routine Description:

        EnumFunc is a placeholder for the application-supplied function name.

    Arguments:

        hModule - Identifies the module whose executable file contains
            the resource names to be enumerated.  A value of NULL
            references the module handle associated with the image file
            that was used to create the current process.

        lpType - Points to a null-terminated character string that
            represents the type name of the resource being enumerated.
            For predefined resource types, the lpType parameter will be
            one of the following values:

            RT_ACCELERATOR - Accelerator table

            RT_BITMAP - Bitmap resource

            RT_DIALOG - Dialog box

            RT_FONT - Font resource

            RT_FONTDIR - Font directory resource

            RT_MENU - Menu resource

            RT_RCDATA - User-defined resource (raw data)

            RT_STRING - String table

            RT_MESSAGETABLE - Message table

            RT_CURSOR - Hardware dependent cursor resource

            RT_GROUP_CURSOR - Directory of cursor resources

            RT_ICON - Hardware dependent cursor resource

            RT_GROUP_ICON - Directory of icon resources

        lpName - Points to a null-terminated character string that
            represents the name of the resource being enumerated.

        wLanguage -  represents the language of the resource.

        lParam - Specifies the 32-bit arugment of the EnumResourceNames
            function.

    Return Value:

        TRUE - Continue the enumeration.

        FALSE/NULL - Stop enumerating resource names.

--*/

{
    BOOL Result;
    NTSTATUS Status;
    ULONG i;
    ULONG_PTR IdPath[ 2 ];
    HANDLE DllHandle;
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory, TopResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry;
    USHORT wLanguage;

    IdPath[ 1 ] = 0;
    if ((IdPath[ 0 ] = BaseDllMapResourceIdW( lpType )) == -1) {
        Status = STATUS_INVALID_PARAMETER;
        }
    else
    if ((IdPath[ 1 ] = BaseDllMapResourceIdW( lpName )) == -1) {
        Status = STATUS_INVALID_PARAMETER;
        }
    else {
        DllHandle = BasepMapModuleHandle( hModule, TRUE );
        TopResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
            RtlImageDirectoryEntryToData( (PVOID)DllHandle,
                                          TRUE,
                                          IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                          &i
                                        );
        if (!TopResourceDirectory) {
            Status = STATUS_RESOURCE_DATA_NOT_FOUND;
            }
        else {
            Status = LdrFindResourceDirectory_U( (PVOID)DllHandle,
                                                 IdPath,
                                                 2,
                                                 &ResourceDirectory
                                               );
            }
        }

    if (!NT_SUCCESS( Status )) {
        BaseDllFreeResourceId( IdPath[ 0 ] );
        BaseDllFreeResourceId( IdPath[ 1 ] );
        BaseSetLastNTError( Status );
        return FALSE;
        }

    Result = TRUE;
    try {
        ResourceDirectoryEntry =
            (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResourceDirectory+1);
        if (ResourceDirectory->NumberOfNamedEntries != 0) {
            BaseSetLastNTError( STATUS_INVALID_IMAGE_FORMAT );
            Result = FALSE;
            }
        else {
            for (i=0; i<ResourceDirectory->NumberOfIdEntries; i++) {
                wLanguage = ResourceDirectoryEntry->Id;
                if (!_ResourceCallEnumLangRoutine((ENUMRESLANGPROCA)lpEnumFunc, hModule, (LPSTR)lpType, (LPSTR)lpName, wLanguage, lParam )) {
                    Result = FALSE;
                    break;
                    }

                ResourceDirectoryEntry++;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        Result = FALSE;
        }

    //
    // Free any strings allocated by BaseDllMapResourceIdW
    //
    BaseDllFreeResourceId( IdPath[ 0 ] );
    BaseDllFreeResourceId( IdPath[ 1 ] );

    return Result;
}


ULONG_PTR
BaseDllMapResourceIdA(
    LPCSTR lpId
    )
{
    NTSTATUS Status;
    ULONG_PTR Id;
    ULONG ulId;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    PWSTR s;

    try {
        if ((ULONG_PTR)lpId >= LDR_RESOURCE_ID_NAME_MINVAL) {

            if (*lpId == '#') {
                Status = RtlCharToInteger( lpId+1, 10, &ulId );
                Id = ulId;
                if (!NT_SUCCESS( Status ) || Id & LDR_RESOURCE_ID_NAME_MASK) {
                    if (NT_SUCCESS( Status )) {
                        Status = STATUS_INVALID_PARAMETER;
                        }
                    BaseSetLastNTError( Status );
                    Id = (ULONG)-1;
                    }
                }
            else {
                RtlInitAnsiString( &AnsiString, lpId );
                Status = RtlAnsiStringToUnicodeString( &UnicodeString,
                                                       &AnsiString,
                                                       TRUE
                                                     );
                if (!NT_SUCCESS( Status )){
                    BaseSetLastNTError( Status );
                    Id = (ULONG_PTR)-1;
                    }
                else {
                    s = UnicodeString.Buffer;
                    while (*s != UNICODE_NULL) {
                        *s = RtlUpcaseUnicodeChar( *s );
                        s++;
                        }

                    Id = (ULONG_PTR)UnicodeString.Buffer;
                    }
                }
            }
        else {
            Id = (ULONG_PTR)lpId;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        Id =  (ULONG_PTR)-1;
        }
    return Id;
}

ULONG_PTR
BaseDllMapResourceIdW(
    LPCWSTR lpId
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    ULONG_PTR Id;
    ULONG ulId;
    PWSTR s;

    try {
        if ((ULONG_PTR)lpId >= LDR_RESOURCE_ID_NAME_MINVAL) {
            if (*lpId == '#') {
                RtlInitUnicodeString( &UnicodeString, lpId+1 );
                Status = RtlUnicodeStringToInteger( &UnicodeString, 10, &ulId );
                Id = ulId;
                if (!NT_SUCCESS( Status ) || Id > LDR_RESOURCE_ID_NAME_MASK) {
                    if (NT_SUCCESS( Status )) {
                        Status = STATUS_INVALID_PARAMETER;
                        }
                    BaseSetLastNTError( Status );
                    Id = (ULONG_PTR)-1;
                    }
                }
            else {
                s = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), (wcslen( lpId ) + 1) * sizeof( WCHAR ) );
                if (s == NULL) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    Id = (ULONG_PTR)-1;
                    }
                else {
                    Id = (ULONG_PTR)s;

                    while (*lpId != UNICODE_NULL) {
                            *s++ = RtlUpcaseUnicodeChar( *lpId++ );
                            }

                    *s = UNICODE_NULL;
                    }
                }
            }
        else {
            Id = (ULONG_PTR)lpId;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        Id =  (ULONG_PTR)-1;
        }

    return Id;
}


VOID
BaseDllFreeResourceId(
    ULONG_PTR Id
    )
{
    UNICODE_STRING UnicodeString;

    if (Id >= LDR_RESOURCE_ID_NAME_MINVAL && Id != -1) {
        UnicodeString.Buffer = (PWSTR)Id;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = 0;
        RtlFreeUnicodeString( &UnicodeString );
        }
}


INT_PTR ReturnMem16Data(
	DWORD dwReserved1,
	DWORD dwReserved2,
	DWORD dwReserved3
	)
{
// Since there's _currently_ no other app we know that this will be useful for, we can
// always return "our" value.

    // Elmo's Preschool Deluxe is looking for free physical or virtual mem.  Give it a
    // Number it will be happy with.
    // Incoming params from Elmo's (in case they're needed one day):
    // dwReserved1 will be 0
    // dwReserved2 will be 1 or 2 (physical/virtual)
    // dwReserved3 will be 0
    return 0x2000;
}


BOOL
UTRegister(
    HMODULE hInst32,
    LPSTR lpszDll16,
    LPSTR lpszInitFunc,
    LPSTR lpszThunkFunc,
    FARPROC *ppfnThunk32Func,
    FARPROC Callback,
    PVOID lpvData
    )
{

	//
    // This function is supposed to return an error code.  VOID happens to work because
    // the stub would just leave EAX alone.  If something happens and EAX starts getting
    // zero'ed out, it'll cause problems and return type here should be changed to int
    // and success should return NON-ZERO.   - bjm 09/98
    //

	// Sure, we could have checked this on a compat bit instead, but the ISV is the
	// Children's Television Workshop people and if they do this silliness in any
	// of their other apps, we'll get those fixed "for free".
    if ( 0 == lstrcmpi( lpszDll16, (LPCSTR)"mem16.dll" ) &&
         0 == lstrcmpi( lpszThunkFunc, (LPCSTR)"GetMemory" ) )
	{
	    //
	    // Elmo's Preschool Deluxe calls to a 16bit dll they ship just
	    // to get physical and virtual mem.  Let's give 'em a pointer to our routine that
	    // will give it numbers that makes it happy.
	    //
	    *ppfnThunk32Func = ReturnMem16Data;
       return(TRUE);
	}

    // Stub this function for King's Quest and Bodyworks 5.0 and other random Win 95 apps.
    return(FALSE);
}


VOID
UTUnRegister(
    HMODULE hInst32
    )
{
    // Stub this function for King's Quest and Bodyworks 5.0 and other random Win 95 apps.
    return;
}
