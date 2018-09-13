/*++


Copyright (c) 1991  Microsoft Corporation

Module Name:

    RegClass.c

Abstract:

    This module contains routines to manipulate class registration
    registry keys for the win32 registry apis.  These routines are called
    from several of the functions for manipulating registry, including
    the functions that open, enumerate, create, and delete keys.

Author:

    Adam P. Edwards     (adamed)  14-Nov-1997

Key Functions:

    OpenCombinedClassesRoot
    BaseRegGetKeySemantics
    BaseRegOpenClassKey
    BaseRegOpenClassKeyFromLocation
    BaseRegGetUserAndMachineClass

Notes:

****************************************************
    PLEASE READ THIS IF YOU ARE NEW TO THIS CODE!!!!
****************************************************

    Starting with NT5, the HKEY_CLASSES_ROOT key is per-user
    instead of per-machine -- previously, HKCR was an alias for
    HKLM\Software\Classes.

    The per-user HKCR combines machine classes stored it the
    traditional HKLM\Software\Classes location with classes
    stored in HKCU\Software\Classes.

    Certain keys, such as CLSID, will have subkeys that come
    from both the machine and user locations.  When there is a conflict
    in key names, the user oriented key overrides the other one --
    only the user key is seen in that case.

    Here are the key ideas for this implementation:

    1. The changes for this module only affect keys under
       HKEY_CLASSES_ROOT. Only the Local registry
       implementation supports HKCR so all the changes are
       local only, they do not exist in the
       remote rpc registry server.

    2. We parse each key under HKCR as

         <prefix>\<intermediate>\<special>[\<classreg>]\[<lastelement>]

       where <prefix> is one of the forms

         \Registry\Machine\Software\Classes
         \Registry\User\<sid>\Software\Classes
         \Registry\User\<sid>_Classes

       <intermediate> can be a subpath of arbitrary length

       <special> is a certain list of keys, shown below in the
         gSpecialSubtrees table, e.g. IID, CLSID.

       <classreg> is any subkey of <special>.  <lastelement> is
         the remainder of the path.

    3. In order to quickly distinguish keys in HKCR from keys not in HKCR, we
       use tag bits on each registry handle that we return from an open or create
       if the key is under HKCR. When the HKCR predefined handle is opened,
       we set a tag on its handle index -- any children open or created with a
       parent key whose tag is set like this will inherit the tag. There are other
       tags, such as those for local and remote regkeys, already in use prior
       to the implementation of per user class registration in NT5. Please see
       the header file for more information on how to interpret the tags.

    4. The special keys have the following properties which differentiate
       them from standard registry keys:

       a. The children of a special key come from both HKLM\Software\Classes
          and HKCU\Software\Classes.  Thus, since CLSID is a special key,
          if HKLM\Software\Classes\CLSID\Key1 exists and
          HKCU\Software\Classes\CLSID\Key2 exists, one would find the keys
          Key1 and Key2 under HKCR\CLSID.
       b. If the same key exists in both the user and machine locations, only
          the user version of the key is seen under HKCR.

     5. To create the illusion described above, the code for several api's
        had to be modified:

        a. RegOpenKeyEx -- for HKCR subkeys, this api was modified to look
           for the key to open first in the user part of the registry,
           then the machine part if the user version did not exist. All
           keys opened with HKCR as an ancestor get a bit set in the handle
           index.

        b. RegCreateKeyEx -- modified in a fashion similar to RegOpenKeyEx.
        c. RegDeleteKey  -- modified to find key to delete in fashion similar
           to RegOpenKeyEx.

        d. RegEnumKeyEx -- extensive changes for HKCR. Previously this api was
           simply a wrapper to the kernel version. This is insufficient now
           because the kernel knows nothing of our hkcr scheme.  See regecls.*
           for details.

        e. RegQueryInfoKey -- changes related to RegEnumKeyEx changes -- see
           regecls.*, regqkey.c.

    It should be noted that HKCU\Software\Classes is not the true
    location of the user-only class data.  If it were, all the class
    data would be in ntuser.dat, which roams with the user.  Since
    class data can get very large, installation of a few apps
    would cause HKCU (ntuser.dat) to grow from a manageable size
    to many megabytes.  Since user-specific class data comes from
    the directory, it does not need to roam and therefore it was
    separated from HKCU (ntuser.dat) and stored in another hive
    mounted under HKEY_USERS.

    It is still desirable to allow access to this hive through
    HKCU\Software\Classes, so we use some trickery (symlinks) to
    make it seem as if the user class data exists there.

**************************
    IMPORTANT ASSUMPTIONS:
**************************

    BUGBUG: This code assumes that all special keys exist in both
    HKEY_LOCAL_MACHINE\Software\Classes and HKEY_CURRENT_USER\Software\Classes.
    The code may break if this is not true.

--*/


#ifdef LOCAL

#include <rpc.h>
#include <string.h>
#include <wchar.h>
#include "regrpc.h"
#include "localreg.h"
#include "regclass.h"
#include "regecls.h"
#include <malloc.h>


NTSTATUS QueryKeyInfo(
    HKEY hKey,
    PKEY_FULL_INFORMATION* ppKeyFullInfo,
    ULONG BufferLength,
    BOOL fClass,
    USHORT MaxClassLength);

extern HKEY HKEY_RestrictedSite;

BOOL            gbCombinedClasses = TRUE;

PKEY_VALUE_PARTIAL_INFORMATION gpNameSpaceKeyInfo = NULL;

#if defined(_REGCLASS_MALLOC_INSTRUMENTED_)

RTL_CRITICAL_SECTION gRegClassHeapCritSect;
DWORD                gcbAllocated = 0;
DWORD                gcAllocs = 0;
DWORD                gcbMaxAllocated = 0;
DWORD                gcMaxAllocs = 0;
PVOID                gpvAllocs;

#endif // defined(_REGCLASS_MALLOC_INSTRUMENTED_)

UNICODE_STRING gMachineClassesName = {
    REG_MACHINE_CLASSES_HIVE_NAMELEN,
    REG_MACHINE_CLASSES_HIVE_NAMELEN,
    REG_MACHINE_CLASSES_HIVE_NAME};



error_status_t
OpenCombinedClassesRoot(
     IN REGSAM samDesired,
    OUT HANDLE * phKey
    )
/*++

Routine Description:

    Attempts to open the the HKEY_CLASSES_ROOT predefined handle.

Arguments:

    ServerName - Not used.
    samDesired - This access mask describes the desired security access
                 for the key.
    phKey - Returns a handle to the key \REGISTRY\MACHINE\SOFTWARE\CLASSES.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    OBJECT_ATTRIBUTES       Obja;
    NTSTATUS                Status;
    UNICODE_STRING          UsersHive;
    UNICODE_STRING          UsersMergedHive;

    // first try for a per-user HKCR
    RtlFormatCurrentUserKeyPath( &UsersHive );

    UsersMergedHive.MaximumLength = UsersHive.MaximumLength +
        REG_USER_HIVE_CLASSES_SUFFIXLEN + REG_CHAR_SIZE;

    //
    // alloca does not return NULL on failure, it throws an exception,
    // so return value is not checked.
    //
    UsersMergedHive.Buffer = alloca(UsersMergedHive.MaximumLength);

    RtlCopyUnicodeString(&UsersMergedHive, &UsersHive );

    // add the _Merged_Classes suffix
    Status = RtlAppendUnicodeToString( &UsersMergedHive, REG_USER_HIVE_CLASSES_SUFFIX);

    ASSERT(NT_SUCCESS(Status));

    //
    // Initialize the OBJECT_ATTRIBUTES structure so that it creates
    // (opens) the key "\HKU\<sid>_Merged_Classes" with a Security
    // Descriptor that allows everyone complete access.
    //

    InitializeObjectAttributes(
        &Obja,
        &UsersMergedHive,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(
                phKey,
                samDesired, // MAXIMUM_ALLOWED,
                &Obja
                );

    RtlFreeUnicodeString( &UsersHive );

    //
    // This key is the ancestor of all keys in HKCR, so
    // we must mark its handle so that its origin in HKCR
    // is propagated to all children opened with this handle
    //
    if (NT_SUCCESS(Status)) {

        *phKey = REG_CLASS_SET_SPECIAL_KEY(*phKey);

    }

    return Status;
}



NTSTATUS BaseRegGetKeySemantics(
    HKEY            hkParent,
    PUNICODE_STRING pSubKey,
    SKeySemantics*  pKeySemantics)
/*++

Routine Description:

    This function parses a key in HKEY_CLASSES_ROOT.  It is used to determine if a given key
    is a class registration unit key, as well as other syntactic / semantic information about
    the key.  It sets the value of pfIsClsRegKey to TRUE if it is, FALSE if not.

    The key in question is defined by the (hkParent, pSubKey) pair.

    Definitions for terms such as Prefix, Special Key, and class registration can
    be found at the top of this module.

Arguments:

    hkParent      - parent portion of key
    pSubKey       - child portion of key
    pKeySemantics - pointer to struct containing key semantic information -- the
                    following members of this structure are affected:

                    _fUser: TRUE if this key is rooted in HKEY_USERS, FALSE if not
                    _fMachine: TRUE if this key is rooted in HKLM, FALSE if not
                    _fCombinedClasses: TRUE if this key is rooted in HKEY_USERS\\<Sid>_Classes
                    _fClassRegistration: TRUE if this key is a class registration unit
                    _fClassRegParent: TRUE if this key is the parent of a class registration unit
                    _ichKeyStart: index to start of a class reg after the prefix -- this is after
                                 the pathsep which follows the prefix
                    _cbPrefixLen: Length (in bytes) of prefix from start of full path
                    _cbSpecialKey: Length (in bytes) of the name of the special key -- this is
                                 not from the start of the full path, just that key name only. It
                                 includes an initial pathsep.
                    _cbClassRegKey: length of class reg key name (not from start of full path).
                                 Includes an initial pathsep.
                    _cbFullPath: size of buffer structure pointed to by _pFullPath.  On return,
                                 this member is set to the number of bytes written to _pFullPath
                                 by the function, or the required number of bytes if the buffer
                                 passed in was too small
                    _pFullPath: KEY_NAME_INFORMATION structure containing the full pathname
                                of the registry key defined by (hkParent, pSubKey). This pathname
                                is null terminated.

Returns:
    
    NT_SUCCESS If the function completed successfully.  If the buffer pointed to by
    pKeySemantics->_pFullPath is not large enough to hold the name of the key, the
    function returns STATUS_BUFFER_TOO_SMALL and the required size in bytes is
    written to pKeySemantics->_cbFullPath.  The caller may then reallocate the buffer
    and call this function again.  All other errors return the appropriate NTSTATUS
    failure code.

Notes:

    After calling this function and getting a successful return status, the pKeySemantics
    structure should be freed by calling BaseRegReleaseKeySemantics

--*/
{
    NTSTATUS                 Status;
    UNICODE_STRING           NameInfo;
    PKEY_NAME_INFORMATION    pNameInfo;

    USHORT                   ichClassesKeyNameEnd;
    USHORT                   ichSpecialKeyNameEnd;
    USHORT                   cbName;
    ULONG                    cbObjInfo;
    WCHAR*                   szClassRegKeyEnd;

    //
    // Save in params
    //
    cbObjInfo = pKeySemantics->_cbFullPath  - REG_CHAR_SIZE; // subtract one for trailing \0
    pNameInfo = pKeySemantics->_pFullPath;

    //
    // reset out params
    //
    memset(&(pKeySemantics->_pFullPath), 0, sizeof(*(pKeySemantics->_pFullPath->Name)));
    memset(pKeySemantics, 0, sizeof(*pKeySemantics));

    //
    // restore in params
    //
    pKeySemantics->_pFullPath = pNameInfo;
    pKeySemantics->_cbFullPath = cbObjInfo;

    //
    // Get full name of key -- first, we need to find the path
    // for the registry key hkParent
    //
    if (!hkParent) {

        //
        // If no key name was specified, the full path is simply the subkey name
        //
        pKeySemantics->_cbFullPath = REG_CHAR_SIZE;
        (pKeySemantics->_pFullPath->Name)[0] = L'\0';
        pKeySemantics->_pFullPath->NameLength = 0;
        pKeySemantics->_cbFullPath = sizeof(*(pKeySemantics->_pFullPath));

    } else {

        Status = NtQueryKey(
            hkParent,
            KeyNameInformation,
            pKeySemantics->_pFullPath,
            cbObjInfo,
            &pKeySemantics->_cbFullPath);

        if (STATUS_KEY_DELETED == Status) {
            Status = STATUS_SUCCESS;
        }

        //
        // Kernel set the _cbFullPath member to the necessary size -- tack
        // on the length of the subkey too
        //

        //
        // BUGBUG: fix this for long key names -- reassign cbObjInfo
        //
        pKeySemantics->_cbFullPath += pSubKey->Length + REG_CHAR_SIZE * 2;

        //
        // The retrieval of the object's name information may have succeeded,
        // but we still need to append the subkey, so verify that enough
        // space is left
        //
        if (NT_SUCCESS(Status) && (cbObjInfo < pKeySemantics->_cbFullPath)) {
            Status = STATUS_BUFFER_OVERFLOW;
        }

        if (!NT_SUCCESS(Status)) {

            //
            // Retry by allocating a new buffer if the kernel thought the
            // supplied buffer was too small.  Add extra padding
            // because we may need to add a null terminator and pathsep later
            //
            if (STATUS_BUFFER_OVERFLOW == Status) {

                //
                // The _cbFullPath member was to the required length in the
                // call to NtQueryKey above and includes extra padding
                // for appending more characters
                //
                pNameInfo = (PKEY_NAME_INFORMATION) RegClassHeapAlloc(
                    pKeySemantics->_cbFullPath);

                if (!pNameInfo) {
                    return STATUS_NO_MEMORY;
                }

                cbObjInfo = pKeySemantics->_cbFullPath;

                //
                // Retry -- we should have a large enough buffer now
                //
                Status = NtQueryKey(
                    hkParent,
                    KeyNameInformation,
                    pNameInfo,
                    cbObjInfo,
                    &pKeySemantics->_cbFullPath);

                if (STATUS_KEY_DELETED == Status) {
                    Status = STATUS_SUCCESS;
                }
            }

            if (!NT_SUCCESS(Status)) {

                //
                // We allocated heap for the second query, but since it failed,
                // we need to free the allocated memory.
                //
                if (pNameInfo != pKeySemantics->_pFullPath) {
                    RegClassHeapFree(pNameInfo);
                }

                return Status;
            }
        }
    }

    //
    // If this isn't set, we know a non-registry key handle was passed in since
    // all registry handles have a path associated with them, whereas other types
    // of handles may not
    //
    if (!(pNameInfo->Name)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // We will always return success after this point, so it's
    // ok to set the full path member of the structure now. Make
    // sure we set the flag indicating that we had to allocate
    // memory to store the name if that was indeed the case
    //
    if (pNameInfo != pKeySemantics->_pFullPath) {
        pKeySemantics->_fAllocedNameBuf = TRUE;
    }

    pKeySemantics->_pFullPath = pNameInfo;

    //
    // Now that we know the name of the parent key, we can concatenate it
    // with the pSubKey parameter
    //

    //
    // First we need to add a trailing pathsep and NULL terminate it
    //
    pNameInfo->Name[pNameInfo->NameLength / 2] = L'\\';
    pNameInfo->Name[pNameInfo->NameLength / 2 + 1] = L'\0';

    //
    // Get a unicode string so we can perform string operations
    //
    RtlInitUnicodeString(&NameInfo, pNameInfo->Name);

    //
    // Adjust the length to reflect the unicode string -- right
    // now it inlcudes the length of the Length member of the
    // KEY_NAME_INFORMATION structure -- we just want the length
    // of the string
    //
    pNameInfo->NameLength = NameInfo.Length;

    //
    // Now add space to the string for the subkey and slash
    //
    NameInfo.MaximumLength += pSubKey->Length + REG_CHAR_SIZE;

    //
    // append the subkey to the parent key
    //

    //
    // We made sure the buffer was big enough, so the only way this will
    // fail is if pSubKey is invalid, which will cause an
    // access violation, so no need no test
    //
    Status = RtlAppendUnicodeStringToString(&NameInfo, pSubKey);

    ASSERT(NT_SUCCESS(Status));

    //
    // if the key name isn't at least as long as the shortest
    // classes hive name, leave.
    // This assumes that
    // HKU\\Sid_Classes is shorter than
    // HKU\\Sid\\Software\\Classes
    //
    if (NameInfo.Length < REG_CLASSES_HIVE_MIN_NAMELEN) {
        return STATUS_SUCCESS;
    }

    //
    // remove any terminating pathsep
    //
    if (NameInfo.Buffer[NameInfo.Length / 2 - 1] == L'\\') {
        NameInfo.Length-= sizeof(L'\\');
    }

    //
    // We're done getting the name of the key, save its length
    // for the caller
    //
    pNameInfo->NameLength = NameInfo.Length;

    //
    // cache the name length
    //
    cbName = (USHORT) pNameInfo->NameLength;

    //
    // null terminate the name
    //
    pNameInfo->Name[cbName / REG_CHAR_SIZE] = L'\0';

    if (REG_CLASS_IS_SPECIAL_KEY(hkParent)) {
        pKeySemantics->_fCombinedClasses = TRUE;
    }

    //
    // First, see if we're even in the correct hive -- we can check
    // certain characters in the path to avoid doing extra string compares
    //
    switch (pNameInfo->Name[REG_CLASSES_FIRST_DISTINCT_ICH])
    {
    case L'M':
    case L'm':
        //
        // check if we're in the machine hive
        //
        NameInfo.Length = REG_MACHINE_CLASSES_HIVE_NAMELEN;

        //
        // Compare prefix with the name for the machine classes key
        // Set machine flag if comparison returns equality.
        //
        if (RtlEqualUnicodeString(
                &NameInfo,
                &gMachineClassesName,
                TRUE) != 0) {

            NameInfo.Length = cbName;
            ichClassesKeyNameEnd = REG_MACHINE_CLASSES_HIVE_NAMECCH;

            pKeySemantics->_fMachine = TRUE;

            break;
        }

        return STATUS_SUCCESS;

    case L'U':
    case L'u':
        //
        // check if we're in the users hive
        //
        {
            //
            // This will try to find the user prefix -- it fails
            // if we're not in the user hive and returns a zero-length
            // prefix. Set the flag if it succeeds.
            //
            ichClassesKeyNameEnd = BaseRegGetUserPrefixLength(
                &NameInfo);

            if (!ichClassesKeyNameEnd) {
                return STATUS_SUCCESS;
            }

            pKeySemantics->_fUser = TRUE;

            break;
        }

        //
        // this isn't a class registration because it isn't in any of the
        // correct trees
        //
        return STATUS_SUCCESS;

    default:

        //
        // the appropriate characters weren't in the key name, so
        // this can't be a class registration
        //
        return STATUS_SUCCESS;
    }

    //
    // At this point, we've found the prefix. The next part of the key
    // is the special key -- we look for that now.
    //
    pKeySemantics->_cbPrefixLen = ichClassesKeyNameEnd * REG_CHAR_SIZE;
    pKeySemantics->_ichKeyStart = ichClassesKeyNameEnd;

    //
    // the start of the special key
    // is the character right after the end of the prefix
    //
    if (pKeySemantics->_cbPrefixLen < pNameInfo->NameLength) {
        pKeySemantics->_ichKeyStart++;
    }

    //
    // search for a special subkey of the classes hive --
    // this will return the index in the full path of the end
    // of the special key name.
    //
    ichSpecialKeyNameEnd = BaseRegCchSpecialKeyLen(
        &NameInfo,
        ichClassesKeyNameEnd,
        pKeySemantics);

    //
    // if we find that the entire key is a special key, we're done --
    // there's nothing after it in this case so there's no more to
    // parse
    //
    if (pKeySemantics->_fClassRegParent) {
        return STATUS_SUCCESS;
    }

    //
    // at this point, we know the key itself is a class registration
    //
    pKeySemantics->_fClassRegistration = TRUE;

    pKeySemantics->_cbClassRegKey = (USHORT) pNameInfo->NameLength -
        (pKeySemantics->_cbPrefixLen + pKeySemantics->_cbSpecialKey + REG_CHAR_SIZE);

    return STATUS_SUCCESS;
}


void BaseRegReleaseKeySemantics(SKeySemantics* pKeySemantics)
/*++
Routine Description:

    This function frees resources associated with an SKeySemantics object

Arguments:

    pKeySemantics - pointer to SKeySemantics object whose resources should
        be freed

Return Value:

    None

--*/
{
    if (pKeySemantics->_fAllocedNameBuf) {
        RegClassHeapFree(pKeySemantics->_pFullPath);
    }
}



NTSTATUS BaseRegOpenClassKey(
    IN HKEY hKey,
    IN PUNICODE_STRING lpSubKey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult)
/*++
Routine Description:

    This function is used to retry opening a class registration key.

Arguments:

    hKey - Supplies a handle to an open key.  The lpSubKey pathname
        parameter is relative to this key handle.

    lpSubKey - Supplies the downward key path to the key to open.
        lpSubKey is always relative to the key specified by hKey.

    dwOptions -- reserved.

    samDesired -- This access mask describes the desired security access
        for the key.

    phkResult -- Returns the handle to the newly opened key.

Return Value:

    Returns STATUS_SUCCESS if a key was successfully opened, otherwise it
        returns an NTSTATUS error code

    Note:

    The key must be a class registration key in order to be opened

--*/
{
    BYTE                rgNameInfoBuf[REG_MAX_CLASSKEY_LEN + REG_CHAR_SIZE];
    SKeySemantics       keyinfo;
    NTSTATUS            Status;

    //
    // Set up the buffer that will hold the name of the key
    //
    keyinfo._pFullPath = (PKEY_NAME_INFORMATION) rgNameInfoBuf;
    keyinfo._cbFullPath = sizeof(rgNameInfoBuf);

    //
    // get information about this key
    //
    Status = BaseRegGetKeySemantics(hKey, lpSubKey, &keyinfo);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Use the information above to look in both user and machine
    // hives for the key to be opened
    //
    Status =  BaseRegOpenClassKeyFromLocation(
        &keyinfo,
        hKey,
        lpSubKey,
        samDesired,
        LOCATION_BOTH,
        phkResult);

    BaseRegReleaseKeySemantics(&keyinfo);

    return Status;
}



NTSTATUS BaseRegOpenClassKeyFromLocation(
    SKeySemantics*  pKeyInfo,
    HKEY            hKey,
    PUNICODE_STRING lpSubKey,
    REGSAM          samDesired,
    DWORD           dwLocation,
    HKEY*           phkResult)
/*++
Routine Description:

    This function will try to open a class registration key that has no link
    in the combined classes hive -- it does this by attempting to open the
    class registration in the machine hive.  If it succeeds, it also creates
    a link to the key in the combined classes hive

Arguments:

    pKeyInfo -- structure supplying information about a key

    hKey -- Supplies a handle to an open key.  The lpSubKey pathname
        parameter is relative to this key handle.

    lpSubKey -- Supplies the downward key path to the key to open.
        lpSubKey is always relative to the key specified by hKey.

    samDesired -- This access mask describes the desired security access
        for the key.

    phkResult -- Returns the handle to the newly opened key.  If NULL,
        no open key handle is returned.

    dwLocation -- set of flags that specify where to look for the key.
        If LOCATION_MACHINE is specified, the function looks in machine.
        If LOCATION_USER is specified, the function looks in user.  Both
        flags may be specified simultaneously, so that it will look in both
        places, or LOCATION_BOTH may be specified for this purpose.  If
        the function looks in both places, an existing key in the user hive
        takes precedence over one in the machine hive.

Return Value:

    Returns STATUS_SUCCESS if a key was successfully opened, otherwise it
        returns an NTSTATUS error code

    Note:

--*/
{
    WCHAR*              FullPathBuf;
    USHORT              NewPathLen;
    UNICODE_STRING      ClassRegkey;
    UNICODE_STRING      ClassRegSubkey;
    OBJECT_ATTRIBUTES   Obja;
    NTSTATUS            Status;
    USHORT              PrefixLen;

    //
    // Init locals
    //
    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    NewPathLen = (USHORT) pKeyInfo->_pFullPath->NameLength + REG_CLASSES_SUBTREE_PADDING;

    //
    // Allocate space for the remapped key -- note that if alloca
    // fails, it throws an exception, so we don't check for NULL return value
    //
    FullPathBuf = (WCHAR*) RegClassHeapAlloc(NewPathLen);

    if (!FullPathBuf) {
        return STATUS_NO_MEMORY;
    }

    //
    // Set up a unicode string to use this buffer
    //
    ClassRegkey.MaximumLength = NewPathLen;
    ClassRegkey.Buffer = FullPathBuf;

    ASSERT((dwLocation == LOCATION_USER) || (dwLocation == LOCATION_MACHINE) ||
           (dwLocation == LOCATION_BOTH));

    //
    // Opening the entire key is a two step process.  First, open
    // the class registration portion -- we need to do that from
    // either the machine or user location.  The second step
    // is to open everything after the class registration using the
    // key obtained in the first step.
    //

    //
    // Below we try to find a user or machine version of the
    // class registration
    //

    if (LOCATION_USER & dwLocation) {
        //
        // Try the user location -- first, move the key name to
        // the user hive's namespace
        //
        Status = BaseRegTranslateToUserClassKey(
            pKeyInfo,
            &ClassRegkey,
            &PrefixLen);

        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }

        //
        // now try opening the key with the new HKCU string
        //
        InitializeObjectAttributes(
            &Obja,
            &ClassRegkey,
            OBJ_CASE_INSENSITIVE,
            NULL, // using absolute path, no hkey
            NULL);

        Status = NtOpenKey(
            phkResult,
            MAXIMUM_ALLOWED,
            &Obja);
    }

    //
    // Only try machine if we failed to open user key above
    // (or didn't even try to open it)
    //
    if ((LOCATION_MACHINE & dwLocation) && !NT_SUCCESS(Status)) {

        //
        // Now try HKLM -- translate the key to the machine
        // namespace
        //
        Status = BaseRegTranslateToMachineClassKey(
            pKeyInfo,
            &ClassRegkey,
            &PrefixLen);

        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }

        //
        // now try opening the key with the new HKLM string
        //
        InitializeObjectAttributes(
            &Obja,
            &ClassRegkey,
            OBJ_CASE_INSENSITIVE,
            NULL, // using absolute path, no hkey
            NULL);

        Status = NtOpenKey(
            phkResult,
            MAXIMUM_ALLOWED,
            &Obja);

        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }
    }

    //
    // mark this key as a class key from HKCR
    //
    if (NT_SUCCESS(Status)) {
        *phkResult = REG_CLASS_SET_SPECIAL_KEY(*phkResult);
    }

cleanup:

    RegClassHeapFree(FullPathBuf);

    return Status;
}



NTSTATUS BaseRegConstructUserClassPrefix(
    SKeySemantics*  pKeyInfo,
    PUNICODE_STRING pUserClassPrefix)
/*++
Routine Description:

    This function creates a prefix for a class key that is in the user hive

Arguments:

    pKeyInfo         - pointer to struct containing key semantic information
    pUserClassPrefix - out param for the constructed prefix

    Returns: NT_SUCCESS If the function completed successfully.

    Notes:

--*/
{
    UNICODE_STRING UserKey;
    NTSTATUS       Status;

    //
    // The prefix looks like <sid>_Classes
    //

    //
    // First obtain the sid
    //
    if (pKeyInfo->_fUser) {

        UNICODE_STRING SidString;

        //
        // construct a string that contains the user's sid
        //
        KeySemanticsGetSid(pKeyInfo, &SidString);

        RtlInitUnicodeString(&UserKey, REG_USER_HIVE_NAME);

        //
        // create a string that starts with the HKU prefix
        //
        RtlCopyUnicodeString(pUserClassPrefix, &UserKey);

        //
        // append the sid to the user prefix
        //
        Status = RtlAppendUnicodeStringToString(pUserClassPrefix, &SidString);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

    } else {

        UNICODE_STRING          UsersHive;

        //
        // This will only happen if a special key has been deleted from
        // the user hive
        //
        Status = RtlFormatCurrentUserKeyPath( &UsersHive );

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        RtlCopyUnicodeString(pUserClassPrefix, &UsersHive );

        RtlFreeUnicodeString(&UsersHive);
    }

    //
    // Append the suffix to the sid
    //
    return RtlAppendUnicodeToString(pUserClassPrefix, REG_USER_HIVE_CLASSES_SUFFIX);
}


NTSTATUS BaseRegTranslateToMachineClassKey(
    SKeySemantics*  pKeyInfo,
    PUNICODE_STRING pMachineClassKey,
    USHORT*         pPrefixLen)
/*++
Routine Description:

    This function translates a class key rooted in HKCR to the machine hive

Arguments:

    pKeyInfo - pointer to struct containing key semantic information -- the
    pMachineClassKey - out param for result of translation
    pPrefixLen - out param for length of the prefix of the resulting translation

    Returns: NT_SUCCESS If the function completed successfully.

    Notes:

--*/
{
    UNICODE_STRING MachineKey;
    UNICODE_STRING ClassSubkey;

    RtlInitUnicodeString(&MachineKey, REG_MACHINE_CLASSES_HIVE_NAME);

    //
    // get the unique class key portion
    //
    KeySemanticsRemovePrefix(pKeyInfo, &ClassSubkey, REMOVEPREFIX_KEEP_INITIAL_PATHSEP);

    //
    // create a string that starts with the HKLM prefix and has the
    // desired class registration key as a subkey
    //
    RtlCopyUnicodeString(pMachineClassKey, &MachineKey);

    *pPrefixLen = REG_MACHINE_CLASSES_HIVE_NAMELEN;

    return RtlAppendUnicodeStringToString(pMachineClassKey, &ClassSubkey);
}


NTSTATUS BaseRegTranslateToUserClassKey(
    SKeySemantics*  pKeyInfo,
    PUNICODE_STRING pUserClassKey,
    USHORT*         pPrefixLen)
/*++
Routine Description:

    This function translates a class key rooted in HKCR to the user hive

Arguments:

    pKeyInfo - pointer to struct containing key semantic information -- the
    pUserClassKey - out param for result of translation
    pPrefixLen - out param for length of the prefix of the resulting translation

    Returns: NT_SUCCESS If the function completed successfully.

    Notes:

--*/
{
    UNICODE_STRING ClassSubkey;
    NTSTATUS       Status;

    //
    // get the unique class key portion
    //
    KeySemanticsRemovePrefix(pKeyInfo, &ClassSubkey, REMOVEPREFIX_KEEP_INITIAL_PATHSEP);

    if (!NT_SUCCESS(Status = BaseRegConstructUserClassPrefix(
        pKeyInfo,
        pUserClassKey))) {
        return Status;
    }

    *pPrefixLen = pUserClassKey->Length;

    //
    // finally, append the class key
    //
    return RtlAppendUnicodeStringToString(pUserClassKey, &ClassSubkey);
}



USHORT BaseRegGetUserPrefixLength(PUNICODE_STRING pFullPath)
/*++
Routine Description:

    This function is used to determine the length of the prefix
    \\Registry\\User\\<Sid>\\Software\Classes or \\Registry\\User\\\<Sid>_classes

Arguments:

    pFullPath          - full path of the registry, rooted at \\Registry

Return Value:

    Returns the length of the prefix (which must be nonzero), 0 if unsuccessful

--*/
{
    UNICODE_STRING UserHive;
    UNICODE_STRING FullPath;
    USHORT         ich;
    USHORT         ichMax;

    FullPath = *pFullPath;

    //
    // set ourselves up to look for the user hive portion
    // of the prefix
    //
    RtlInitUnicodeString(&UserHive, REG_USER_HIVE_NAME);

    if (FullPath.Length <= UserHive.Length) {
        return 0;
    }

    FullPath.Length = UserHive.Length;

    //
    // check for the user hive prefix, leave if not found
    //
    if (!RtlEqualUnicodeString(&UserHive, &FullPath, TRUE)) {
        return 0;
    }

    ichMax = pFullPath->Length / REG_CHAR_SIZE;

    //
    // before looking for the classes subtree, we must skip past
    // the user's sid -- the prefix is in the form
    // \\Registry\\User\\<sid>\\Software\\Classes or
    // \\Registyr\\User\\<sid>_Classes
    //
    for (ich = REG_USER_HIVE_NAMECCH + 1; ich < ichMax; ich++)
    {
        //
        // if we find a pathsep, we cannot be in the combined
        // classes hive or the user classes hive
        //
        if (pFullPath->Buffer[ich] == L'\\') {
            return 0;
        }

        //
        // if we find the underscore character, we are in the combined
        // classes hive or the user classes hive -- i.e. the prefix looks like
        // \\Registry\\User\\<sid>_Classes
        // -- use the underscore to distinguish from other cases
        //
        if (pFullPath->Buffer[ich] == L'_') {

            UNICODE_STRING Suffix;

            RtlInitUnicodeString(&Suffix, REG_USER_HIVE_CLASSES_SUFFIX);

            FullPath.Length = Suffix.Length;
            FullPath.Buffer = &(pFullPath->Buffer[ich]);

            // look for the user classes suffix in the user hive
            if (RtlEqualUnicodeString(&FullPath, &Suffix, TRUE)) {

                return ich + REG_USER_HIVE_CLASSES_SUFFIXCCH;
            }

            return 0;
        }
    }

    return 0;
}



USHORT BaseRegCchSpecialKeyLen(
    PUNICODE_STRING pFullPath,
    USHORT          ichSpecialKeyStart,
    SKeySemantics*  pKeySemantics)
/*++
Routine Description:

    This function is used to determine the length of a special subkey contained
    on the pSpecialKey parameter.  If the entire pFullPath is a special key,
    a flag in pKeySemantics will be set to TRUE

Arguments:

    pFullPath          - full path of the registry, rooted at \\Registry
    ichSpecialKeyStart - index in the full path of the start of the special key path
    pKeySemantics      - pointer to structure which stores semantics information about a key

Return Value:

    Returns the length of the special key if there is a special key in the pSpecialKey
        path, 0 if there is none

    Notes:

    This function depends on the gSpecialSubtree array being a *sorted* list of special
        key names.

--*/
{
    WCHAR* wszSpecialKey;
    USHORT ichSpecialKeyLen;

    ASSERT(pFullPath->Length / REG_CHAR_SIZE >= ichSpecialKeyStart);

    //
    // For hkcr itself, there is no ancestor -- detect this special
    // case and return
    //
    if (pFullPath->Length / REG_CHAR_SIZE == ichSpecialKeyStart) {
        pKeySemantics->_fClassRegParent = TRUE;
        return ichSpecialKeyStart;
    }

    //
    // The special key is now just the parent of this key -- find
    // the immediate ancestor of this key
    //
    wszSpecialKey = wcsrchr(&(pFullPath->Buffer[ichSpecialKeyStart]), L'\\');

    ASSERT(wszSpecialKey);

    //
    // The length of the special key is the difference
    // between the '\' at the end of the special key and the start
    // of the string
    //
    ichSpecialKeyLen = (USHORT)(wszSpecialKey - pFullPath->Buffer);

    //
    // Store the length of the special key name by itself as well
    //
    pKeySemantics->_cbSpecialKey = ichSpecialKeyLen - ichSpecialKeyStart;

    return ichSpecialKeyLen;
}


NTSTATUS BaseRegOpenClassKeyRoot(
    SKeySemantics*  pKeyInfo,
    PHKEY           phkClassRoot,
    PUNICODE_STRING pClassKeyPath,
    BOOL            fMachine)
/*++
Routine Description:

    This function will try to open the class root key appropriate to
    a given key being opened from HKEY_CLASSES_ROOT. The key opened is either
    HKEY_USERS\<Sid>_Classes or HKLM\Software\Classes. If the key exists
    in the user portion, then that the user key will be opened.  Otherwise,
    the machine key is returned. It also returns the unicode string
    subkey name used to open the key specified in
    pKeyInfo relative to the class root key returned in phkClassRoot.

Arguments:

    pKeyInfo -- structure supplying information about a key

    phkClassRoot -- out param for the class root key result of the function

    pClassKeyPath -- Supplies the downward key path to the key to open.
        pClassKeyPath is always relative to the key specified by hKey.

    pfMachine -- out param flag that indicates that whether or not
        this key was opened in the machine hive.

Return Value:

    Returns STATUS_SUCCESS if a key was successfully deleted, otherwise it
        returns an NTSTATUS error code

    Note:

--*/
{
    NTSTATUS Status;
    USHORT   PrefixLen;
    UNICODE_STRING NewFullPath;

    //
    // Allocate space for a full path -- note that
    // we don't check the return value since alloca throws
    // an exception if it fails
    //
    NewFullPath.Buffer = alloca(pClassKeyPath->MaximumLength);
    NewFullPath.MaximumLength = pClassKeyPath->MaximumLength;

    //
    // Translate to appropriate location
    //
    if (fMachine) {

        Status = BaseRegTranslateToMachineClassKey(
            pKeyInfo,
            &NewFullPath,
            &PrefixLen);
    } else {

        Status = BaseRegTranslateToUserClassKey(
            pKeyInfo,
            &NewFullPath,
            &PrefixLen);
    }

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Open the prefix
    //
    {
        UNICODE_STRING RootKey;
        OBJECT_ATTRIBUTES Obja;

        RootKey.Buffer = NewFullPath.Buffer;

        //
        // Calculate the length of the prefix
        //
        RootKey.Length = PrefixLen;

        //
        // now, get ready to open it
        //
        InitializeObjectAttributes(&Obja,
                                   &RootKey,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL, // full path, no hkey
                                   NULL);

        Status =  NtOpenKey(
            phkClassRoot,
            MAXIMUM_ALLOWED,
            &Obja);
    }

    if (NT_SUCCESS(Status)) {

        //
        // Skip past the prefix
        //
        NewFullPath.Buffer += (PrefixLen / REG_CHAR_SIZE);
        NewFullPath.Length -= PrefixLen;

        if (L'\\' == NewFullPath.Buffer[0]) {
            NewFullPath.Length -= REG_CHAR_SIZE;
            NewFullPath.Buffer ++;
        }

        //
        // Copy everything after the prefix
        //
        RtlCopyUnicodeString(pClassKeyPath, &NewFullPath);
    }

    return Status;
}


NTSTATUS
BaseRegMapClassRegistrationKey(
    HKEY              hKey,
    PUNICODE_STRING   pSubKey,
    SKeySemantics*    pKeyInfo,
    PUNICODE_STRING   pDestSubKey,
    BOOL*             pfRetryOnAccessDenied,
    PHKEY             phkDestResult,
    PUNICODE_STRING*  ppSubKeyResult)
/*++
Routine Description:

    This function will map a key from HKEY_CLASSES_ROOT into either the
    user hive or machine hive.  The remapped key is returned in the
    (*phkDestResult, *ppSubKeyResult) pair.

Arguments:

    hKey    -- root of key to remap

    pSubKey -- Supplies the downward key path to the key to remap.
        pSubKey is always relative to the key specified by hKey.

    pKeyInfo -- structure supplying information about a key

    pDestSubKey -- unicode string in which to store result data
        if the key gets remapped

    pfRetryOnAccessDenied -- out param flag to set indicating whether
        failure to open the remapped key because of access denied should
        be retried

    phkDestResult -- out param for root of remapped key

    ppSubKeyResult -- out param for remainder of path of remapped key

Return Value:

    Returns STATUS_SUCCESS if a key was successfully deleted, otherwise it
        returns an NTSTATUS error code

    Note:

--*/
{
    BOOL           fMachine;
    UNICODE_STRING ClassKeyPath;
    NTSTATUS       Status;

    //
    // by default, use machine
    //
    fMachine = TRUE;

    //
    // Do not do this in the sandbox
    //
    if (NULL == HKEY_RestrictedSite) {

        HKEY           hkUser;

        //
        // Check for existence of the key in the
        // user hive
        //
        Status =  BaseRegOpenClassKeyFromLocation(
            pKeyInfo,
            hKey,
            pSubKey,
            MAXIMUM_ALLOWED,
            LOCATION_USER,
            &hkUser);

        if (!NT_SUCCESS(Status)) {

            //
            // a not found error is fine -- this just means that
            // neither key exists already -- in this case we
            // choose to use machine
            //
            if (STATUS_OBJECT_NAME_NOT_FOUND != Status) {
                return Status;
            }

        } else {

            //
            // The user key exists, we choose it over
            // the machine key
            //
            fMachine = FALSE;

            NtClose(hkUser);
        }

        //
        // Get a buffer for the new path
        //
        ClassKeyPath.Buffer = (WCHAR*) RegClassHeapAlloc(
            ClassKeyPath.MaximumLength = ((USHORT) pKeyInfo->_pFullPath->NameLength +
            REG_CLASSES_SUBTREE_PADDING));

        if (!(ClassKeyPath.Buffer)) {
            return STATUS_NO_MEMORY;
        }

        //
        // Remap the key
        //
        Status = BaseRegOpenClassKeyRoot(
            pKeyInfo,
            phkDestResult,
            &ClassKeyPath,
            fMachine);

        if (!NT_SUCCESS(Status)) {

            RegClassHeapFree(ClassKeyPath.Buffer);

            return Status;
        }

        //
        // If the remapped key is in the machine hive, set the flag so that
        // retries are not permitted.
        //
        if (*pfRetryOnAccessDenied && !fMachine) {
            *pfRetryOnAccessDenied = FALSE;
        }

        //
        // phkDestResult, the root portion of the remapped key, was set above.
        // now set the subkey portion and leave
        //
        *pDestSubKey = ClassKeyPath;
        *ppSubKeyResult = pDestSubKey;

        return STATUS_SUCCESS;
    }

    *ppSubKeyResult = pSubKey;
    *phkDestResult = hKey;

    return STATUS_SUCCESS;
}


NTSTATUS BaseRegGetUserAndMachineClass(
    SKeySemantics* pKeySemantics,
    HKEY           hKey,
    REGSAM         samDesired,
    PHKEY          phkMachine,
    PHKEY          phkUser)
/*++
Routine Description:

    This function will return kernel objects corresponding to the user
    and machine components of a given kernel object.  

Arguments:

    pKeySemantics -- supplies information about hKey.  This is optional --
        if the caller does not supply it, the function will query for the information.
        This is an optimization for callers that already have this info
        and can save us the time of

    hKey -- key for which to open user and machine versions

    samDesired -- security access mask for one of the returned keys -- see
        note below for important info on this
    
    phkMachine -- out param for machine version of key

    phkUser -- out param for user version of key

Return Value:

    Returns STATUS_SUCCESS if a key was successfully deleted, otherwise it
        returns an NTSTATUS error code

Notes:

***VERY IMPORTANT!!!***
    
    One of the two returned keys will alias hKey -- this way we only open
    one object (one trip to the kernel) instead of two.  This means that the caller
    should not blindly call NtClose on the two returned objects -- a == comparison
    between one of the keys and hKey should be made to determine if it that key is
    the alias -- if it is, you should *NOT* call NtClose on it because otherwise the
    owner of hKey will call NtClose on the same handle value after your call to close
    that handle which will cause an exception.  You *should* close the handle that does not
    alias hKey -- if you don't you'll get a handle leak.

    Another important note -- only the new key (non-aliased) will have the access mask
    specified in samDesired -- the aliased key is just hKey, so it has the same access
    mask.  If you want to ensure the correct access on that key, you'll need to explicitly
    duplicate or open that key with the correct access.

--*/
{
    NTSTATUS       Status;
    SKeySemantics  keyinfo;
    SKeySemantics* pKeyInfo;
    UNICODE_STRING EmptyString = {0, 0, 0};
    BYTE           rgNameBuf[REG_MAX_CLASSKEY_LEN + REG_CHAR_SIZE + sizeof(KEY_NAME_INFORMATION)];
    DWORD          dwLocation;
    PHKEY          phkNew;

    //
    // Clear out parameters
    //
    *phkMachine = NULL;
    *phkUser = NULL;

    //
    // Try to use caller supplied key information
    //
    if (pKeySemantics) {
        pKeyInfo = pKeySemantics;
    } else {

        //
        // Set buffer to store info about this key
        //
        keyinfo._pFullPath = (PKEY_NAME_INFORMATION) rgNameBuf;
        keyinfo._cbFullPath = sizeof(rgNameBuf);

        //
        // get information about this key
        //
        Status = BaseRegGetKeySemantics(hKey, &EmptyString, &keyinfo);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        pKeyInfo = &keyinfo;
    }
        
    if (pKeyInfo->_fMachine) {

        *phkMachine = hKey;
        dwLocation = LOCATION_USER;
        phkNew = phkUser;

    } else {

        *phkUser = hKey;
        dwLocation = LOCATION_MACHINE;
        phkNew = phkMachine;

    }

    (void) BaseRegOpenClassKeyFromLocation(
        pKeyInfo,
        hKey,
        &EmptyString,
        MAXIMUM_ALLOWED,
        dwLocation,
        phkNew);

    if (!pKeySemantics) {
        BaseRegReleaseKeySemantics(&keyinfo);
    }

    return STATUS_SUCCESS;
}


NTSTATUS GetFixedKeyInfo(
    HKEY     hkUser,
    HKEY     hkMachine,
    LPDWORD  pdwUserValues,
    LPDWORD  pdwMachineValues,
    LPDWORD  pdwUserMaxDataLen,
    LPDWORD  pdwMachineMaxDataLen,
    LPDWORD  pdwMaxValueNameLen)
{

    NTSTATUS             Status;
    DWORD                cUserValues;
    DWORD                cMachineValues;
    KEY_FULL_INFORMATION KeyInfo;
    DWORD                dwRead;
    DWORD                cbMaxNameLen;
    DWORD                cbUserMaxDataLen;
    DWORD                cbMachineMaxDataLen;

    //
    // Init locals
    //
    cUserValues = 0;
    cMachineValues = 0;
    cbMaxNameLen = 0;
    cbUserMaxDataLen = 0;
    cbMachineMaxDataLen = 0;

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // Init out params
    //
    if (pdwUserValues) {
        *pdwUserValues = 0;
    }

    if (pdwMachineValues) {
        *pdwMachineValues = 0;
    }

    if (pdwMaxValueNameLen) {
        *pdwMaxValueNameLen = 0;
    }

    if (pdwUserMaxDataLen) {
        *pdwUserMaxDataLen = 0;
    }

    if (pdwMachineMaxDataLen) {
        *pdwMachineMaxDataLen = 0;
    }

    //
    // Get user information
    //
    if (hkUser) {

        Status = NtQueryKey(
            hkUser,
            KeyFullInformation,
            &KeyInfo,
            sizeof(KeyInfo),
            &dwRead);

        //
        // We expect this error because we didn't allocate any space
        // for the name or class -- that's ok since it fills in the fixed
        // part of the structure anyway, which is all we care about
        //
        if (STATUS_BUFFER_OVERFLOW == Status) {
            Status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        cUserValues = KeyInfo.Values;
        cbMaxNameLen = KeyInfo.MaxValueNameLen;
        cbUserMaxDataLen = KeyInfo.MaxValueDataLen;
    }

    //
    // Get machine information
    //
    if (hkMachine) {

        Status = NtQueryKey(
            hkMachine,
            KeyFullInformation,
            &KeyInfo,
            sizeof(KeyInfo),
            &dwRead);

        if (STATUS_BUFFER_OVERFLOW == Status) {
            Status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        cMachineValues = KeyInfo.Values;
        cbMachineMaxDataLen = KeyInfo.MaxValueDataLen;

        if (KeyInfo.MaxValueNameLen > cbMaxNameLen) {
            cbMaxNameLen = KeyInfo.MaxValueNameLen;
        }
     }

    if (pdwUserValues) {
        *pdwUserValues = cUserValues;
    }

    if (pdwMachineValues) {
        *pdwMachineValues = cMachineValues;
    }

    if (pdwMaxValueNameLen) {
        *pdwMaxValueNameLen = cbMaxNameLen;
    }

    if (pdwUserMaxDataLen) {
        *pdwUserMaxDataLen = cbUserMaxDataLen;
    }

    if (pdwMachineMaxDataLen) {
        *pdwMachineMaxDataLen = cbMachineMaxDataLen;
    }

    return Status;
}


#ifdef CLASSES_RETRY_ON_ACCESS_DENIED


NTSTATUS
BaseRegMapClassOnAccessDenied(
    SKeySemantics*    pKeySemantics,
    PHKEY             phkDest,
    PUNICODE_STRING   pDestSubKey,
    BOOL*             pfRetryOnAccessDenied)
/*++
Routine Description:

    This function will remap a key to the user hive when an access denied
    error is encountered creating it in the machine hive

Arguments:

    pKeySemantics -- structure supplying information about a key

    phkDest -- out param for root of remapped key

    pDestSubKey -- out param for remainder of path of remapped key

    pfRetryOnAccessDenied -- in / out param.  If true, we can
      remap it.  On return this value indicates whether or not
      we can do another retry

Return Value:

    Returns STATUS_SUCCESS if a key was successfully deleted, otherwise it
        returns an NTSTATUS error code

    Note:

--*/
{
    NTSTATUS Status;

    Status = STATUS_ACCESS_DENIED;

    if (pKeySemantics->_fCombinedClasses &&
        *pfRetryOnAccessDenied) {

        USHORT         PrefixLen;
        UNICODE_STRING NewFullPath;

        //
        // Close the original key -- we don't need it anymore.
        //
        NtClose(*phkDest);
        *phkDest = NULL;

        //
        // No more retries permitted for this key
        //
        *pfRetryOnAccessDenied = FALSE;

        //
        // Get space for the new path -- we will free this below.
        // We avoid using alloca because of stack overflows
        //
        NewFullPath.MaximumLength = pKeySemantics->_pFullPath->NameLength + REG_CLASSES_SUBTREE_PADDING;

        NewFullPath.Buffer = RegClassHeapAllocate(NewFullPath.MaximumLength);

        if (!(NewFullPath.Buffer)) {
            return STATUS_NO_MEMORY;
        }

        //
        // Translate this key to the user hive
        //
        Status = BaseRegTranslateToUserClassKey(
            pKeySemantics,
            &NewFullPath,
            &PrefixLen);

        if (NT_SUCCESS(Status)) {

            UNICODE_STRING    Prefix;
            OBJECT_ATTRIBUTES Obja;

            //
            // Allocate space for the new key name to give back to the caller
            //
            pDestSubKey->MaximumLength = NewFullPath.MaximumLength - PrefixLen + 1;
            pDestSubKey->Buffer = (WCHAR*) RegClassHeapAlloc(pDestSubKey->MaximumLength);

            if (!(pDestSubKey->Buffer)) {
                Status = STATUS_NO_MEMORY;
                goto cleanup;
            }

            //
            // Make a string which strips off every thing after the prefix --
            // we will open up to the prefix
            //
            Prefix.Buffer = NewFullPath.Buffer;
            Prefix.Length = PrefixLen;

            //
            // Move our full path past the prefix
            //
            NewFullPath.Buffer += PrefixLen / REG_CHAR_SIZE;

            //
            // Copy everything after the prefix to the subkey path
            // that we're returning to the caller
            //
            RtlCopyUnicodeString(pDestSubKey, &NewFullPath);

            //
            // Now open the root for the caller
            //
            InitializeObjectAttributes(&Obja,
                                       &Prefix,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL, // full path, no hkey
                                       NULL);

            Status = NtOpenKey(
                phkDest,
                MAXIMUM_ALLOWED,
                &Obja);
        }
    }

cleanup:

    //
    // Free the buffer we allocated above
    //
    RegClassHeapFree(NewFullPath.Buffer);

    return Status;
}

#endif // CLASSES_RETRY_ON_ACCESS_DENIED


#if defined(_REGCLASS_MALLOC_INSTRUMENTED_)

BOOL InitializeInstrumentedRegClassHeap()
{
    NTSTATUS Status;

    Status = RtlInitializeCriticalSection(
                    &(gRegClassHeapCritSect));

    return NT_SUCCESS(Status);
}

BOOL CleanupInstrumentedRegClassHeap()
{
    NTSTATUS Status;

    Status = RtlDeleteCriticalSection(
                    &(gRegClassHeapCritSect));

    DbgPrint("WINREG: Instrumented memory data for process id 0x%x\n", NtCurrentTeb()->ClientId.UniqueProcess);
    DbgPrint("WINREG: Classes Heap Maximum Allocated: 0x%x\n", gcbMaxAllocated);
    DbgPrint("WINREG: Classes Heap Maximum Outstanding Allocs: 0x%x\n", gcMaxAllocs);

    if (gcbAllocated || gcAllocs) {

        DbgPrint("WINREG: Classes Heap ERROR!\n");
        DbgPrint("WINREG: Classes Heap not completely freed!\n");
        DbgPrint("WINREG: Classes Heap Leaked 0x%x bytes\n", gcbAllocated);
        DbgPrint("WINREG: Classes Heap Outstanding Allocs: 0x%x\n", gcAllocs);

        DbgBreakPoint();
    } else {
        DbgPrint("WINREG: Classes Heap is OK.\n");
    }

    return NT_SUCCESS(Status);
}


#endif // defined(_REGCLASS_MALLOC_INSTRUMENTED_)

#endif // LOCAL











