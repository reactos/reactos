/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    group32.c

Abstract:

    This module contains routines for reading/writing 32-bit Windows/NT
    groups that are stored in the registry.

Author:

    Steve Wood (stevewo) 22-Feb-1993

Revision History:

--*/

#include "advapi.h"
#include <stdio.h>

#include <winbasep.h>
#include "win31io.h"

#define BMR_ICON    1
#define BMR_DEVDEP  0

#define ROUND_UP( X, A ) (((ULONG_PTR)(X) + (A) - 1) & ~((A) - 1))
#define PAGE_SIZE 4096
#define PAGE_NUMBER( A ) ((DWORD)(A) / PAGE_SIZE)

typedef int (WINAPI *PGETSYSTEMMETRICS)(
    int nIndex
    );
PGETSYSTEMMETRICS lpGetSystemMetrics;

ULONG
QueryNumberOfPersonalGroupNames(
    HANDLE CurrentUser,
    PHANDLE GroupNamesKey,
    PHANDLE SettingsKey
    )
{
    NTSTATUS Status;
    UNICODE_STRING KeyName, ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR ValueNameBuffer[ 16 ];
    KEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ResultLength, NumberOfPersonalGroups;

    RtlInitUnicodeString( &KeyName, L"SoftWare\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager\\UNICODE Groups" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                CurrentUser,
                                NULL
                              );
    Status = NtOpenKey( GroupNamesKey,
                        STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY,
                        &ObjectAttributes
                      );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return 0;
        }

    RtlInitUnicodeString( &KeyName, L"SoftWare\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager\\Settings" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                CurrentUser,
                                NULL
                              );
    Status = NtOpenKey( SettingsKey,
                        STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY,
                        &ObjectAttributes
                      );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        NtClose( *GroupNamesKey );
        return 0;
        }

    for (NumberOfPersonalGroups=1; ; NumberOfPersonalGroups++) {
        swprintf( ValueNameBuffer, L"Group%d", NumberOfPersonalGroups );
        RtlInitUnicodeString( &ValueName, ValueNameBuffer );
        Status = NtQueryValueKey( *GroupNamesKey,
                                  &ValueName,
                                  KeyValuePartialInformation,
                                  (PVOID)&KeyValueInformation,
                                  sizeof( KeyValueInformation ),
                                  &ResultLength
                                );
        if (Status == STATUS_BUFFER_OVERFLOW) {
            Status = STATUS_SUCCESS;
            }

        if (!NT_SUCCESS( Status ) ||
            KeyValueInformation.Type != REG_SZ
           ) {
            break;
            }
        }

    return NumberOfPersonalGroups - 1;
}

BOOL
NewPersonalGroupName(
    HANDLE GroupNamesKey,
    PWSTR GroupName,
    ULONG GroupNumber
    )
{
    NTSTATUS Status;
    UNICODE_STRING ValueName;
    WCHAR ValueNameBuffer[ 16 ];

    if (GroupNumber >= CGROUPSMAX) {
        SetLastError( ERROR_TOO_MANY_NAMES );
        return FALSE;
        }

    swprintf( ValueNameBuffer, L"Group%d", GroupNumber );
    RtlInitUnicodeString( &ValueName, ValueNameBuffer );
    Status = NtSetValueKey( GroupNamesKey,
                            &ValueName,
                            0,
                            REG_SZ,
                            GroupName,
                            (wcslen( GroupName ) + 1) * sizeof( WCHAR )
                          );

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return FALSE;
        }
    else {
        return TRUE;
        }
}

BOOL
DoesExistGroup(
    HANDLE GroupsKey,
    PWSTR GroupName
    )
{
    NTSTATUS Status;
    UNICODE_STRING KeyName, ValueName;
    HANDLE Key;
    OBJECT_ATTRIBUTES ObjectAttributes;
    KEY_VALUE_FULL_INFORMATION KeyValueInformation;
    ULONG ResultLength;

    RtlInitUnicodeString( &KeyName, GroupName );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                GroupsKey,
                                NULL
                              );
    Status = NtOpenKey( &Key,
                        GENERIC_READ,
                        &ObjectAttributes
                      );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return FALSE;
        }

    RtlInitUnicodeString( &ValueName, L"" );
    Status = NtQueryValueKey( Key,
                              &ValueName,
                              KeyValueFullInformation,
                              &KeyValueInformation,
                              sizeof( KeyValueInformation ),
                              &ResultLength
                            );
    NtClose( Key );
    if (!NT_SUCCESS( Status ) && Status != STATUS_BUFFER_OVERFLOW) {
        BaseSetLastNTError( Status );
        return FALSE;
        }
    else
    if (KeyValueInformation.Type != REG_BINARY ||
        KeyValueInformation.DataLength > 0xFFFF
       ) {
        return FALSE;
        }
    else {
        return TRUE;
        }
}

PGROUP_DEF
LoadGroup(
    HANDLE GroupsKey,
    PWSTR GroupName
    )
{
    PGROUP_DEF Group;
    NTSTATUS Status;
    UNICODE_STRING KeyName, ValueName;
    HANDLE Key;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PVOID Base;
    KEY_VALUE_FULL_INFORMATION KeyValueInformation;
    ULONG ResultLength;

    RtlInitUnicodeString( &KeyName, GroupName );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                GroupsKey,
                                NULL
                              );
    Status = NtOpenKey( &Key,
                        GENERIC_READ,
                        &ObjectAttributes
                      );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return NULL;
        }

    RtlInitUnicodeString( &ValueName, L"" );
    Status = NtQueryValueKey( Key,
                              &ValueName,
                              KeyValueFullInformation,
                              &KeyValueInformation,
                              sizeof( KeyValueInformation ),
                              &ResultLength
                            );
    if (!NT_SUCCESS( Status )) {
        if (Status == STATUS_BUFFER_OVERFLOW) {
            Status = STATUS_SUCCESS;
            }
        else {
            NtClose( Key );
            BaseSetLastNTError( Status );
            return NULL;
            }
        }

    if (KeyValueInformation.Type != REG_BINARY ||
        KeyValueInformation.DataLength > 0xFFFF
       ) {
        NtClose( Key );
        return NULL;
        }

    Base = VirtualAlloc( NULL, 0xFFFF, MEM_RESERVE, PAGE_READWRITE );
    if (Base == NULL ||
        !VirtualAlloc( Base, KeyValueInformation.DataLength, MEM_COMMIT, PAGE_READWRITE )
       ) {
        if (Base != NULL) {
            VirtualFree( Base, 0, MEM_RELEASE );
            }

        NtClose( Key );
        return NULL;
        }

    Status = NtQueryValueKey( Key,
                              &ValueName,
                              KeyValueFullInformation,
                              &KeyValueInformation,
                              KeyValueInformation.DataLength,
                              &ResultLength
                            );
    NtClose( Key );
    if (!NT_SUCCESS( Status )) {
        VirtualFree( Base, 0, MEM_RELEASE );
        return NULL;
        }
    else {
        Group = (PGROUP_DEF)((PCHAR)Base + KeyValueInformation.DataOffset);

        //
        // Set total size of group
        //
        Group->wReserved = (WORD)(ResultLength - KeyValueInformation.DataOffset);
        RtlMoveMemory( Base, Group, Group->wReserved );
        return (PGROUP_DEF)Base;
        }
}


BOOL
SaveGroup(
    HANDLE GroupsKey,
    PWSTR GroupName,
    PGROUP_DEF Group
    )
{
    NTSTATUS Status;
    UNICODE_STRING KeyName, ValueName;
    HANDLE Key;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG CreateDisposition;
    LONG ValueLength;


    RtlInitUnicodeString( &KeyName, GroupName );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                GroupsKey,
                                NULL
                              );
    Status = NtCreateKey( &Key,
                          STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY,
                          &ObjectAttributes,
                          0,
                          NULL,
                          0,
                          &CreateDisposition
                        );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return FALSE;
        }

    ValueLength = (LONG)((ULONG)Group->wReserved);
    Group->wReserved = 0;
    Group->wCheckSum = (WORD)-ValueLength;

    RtlInitUnicodeString( &ValueName, L"" );
    Status = NtSetValueKey( Key,
                            &ValueName,
                            0,
                            REG_BINARY,
                            Group,
                            ValueLength
                          );

    Group->wReserved = (WORD)ValueLength;
    Group->wCheckSum = 0;
    NtClose( Key );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }
    else {
        return TRUE;
        }
}



BOOL
DeleteGroup(
    HANDLE GroupsKey,
    PWSTR GroupName
    )
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    HANDLE Key;
    OBJECT_ATTRIBUTES ObjectAttributes;


    RtlInitUnicodeString( &KeyName, GroupName );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                GroupsKey,
                                NULL
                              );
    Status = NtOpenKey( &Key,
                        STANDARD_RIGHTS_WRITE |
                          DELETE |
                          KEY_QUERY_VALUE |
                          KEY_ENUMERATE_SUB_KEYS |
                          KEY_SET_VALUE |
                          KEY_CREATE_SUB_KEY,
                        &ObjectAttributes
                      );
    if (!NT_SUCCESS( Status )) {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            return TRUE;
            }
        else {
            BaseSetLastNTError( Status );
            return FALSE;
            }
        }

    Status = NtDeleteKey( Key );
    NtClose( Key );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }
    else {
        return TRUE;
        }
}


BOOL
UnloadGroup(
    PGROUP_DEF Group
    )
{
    return VirtualFree( Group, 0, MEM_RELEASE );
}


BOOL
ExtendGroup(
    PGROUP_DEF Group,
    BOOL AppendToGroup,
    DWORD cb
    )
{
    PBYTE Start, Commit, End;

    if (((DWORD)Group->wReserved + cb) > 0xFFFF) {
        return FALSE;
        }

    Start = (PBYTE)Group + Group->cbGroup;
    End = Start + cb;

    if (PAGE_NUMBER( Group->wReserved ) != PAGE_NUMBER( Group->wReserved + cb )) {
        Commit = (PBYTE)ROUND_UP( (PBYTE)Group + Group->wReserved, PAGE_SIZE );
        if (!VirtualAlloc( Commit, ROUND_UP( cb, PAGE_SIZE ), MEM_COMMIT, PAGE_READWRITE )) {
            return FALSE;
            }
        }

    if (!AppendToGroup) {
        memmove( End, Start, Group->wReserved - Group->cbGroup );
        }

    Group->wReserved += (WORD)cb;
    return TRUE;
}

WORD
AddDataToGroup(
    PGROUP_DEF Group,
    PBYTE Data,
    DWORD cb
    )
{
    WORD Offset;

    if (cb == 0) {
        cb = strlen( Data ) + 1;
        }
    cb = (DWORD)ROUND_UP( cb, sizeof( DWORD ) );

    if (!ExtendGroup( Group, FALSE, cb )) {
        return 0;
        }

    if (((DWORD)Group->cbGroup + cb) > 0xFFFF) {
        return 0;
        }

    Offset = Group->cbGroup;
    Group->cbGroup += (WORD)cb;

    if (Data != NULL) {
        memmove( (PBYTE)Group + Offset, Data, cb );
        }
    else {
        memset( (PBYTE)Group + Offset, 0, cb );
        }

    return Offset;
}

BOOL
AddTagToGroup(
    PGROUP_DEF Group,
    WORD wID,
    WORD wItem,
    WORD cb,
    PBYTE rgb
    )
{
    WORD Offset;
    PTAG_DEF Tag;

    cb = (WORD)(ROUND_UP( cb, sizeof( DWORD ) ));

    Offset = Group->wReserved;
    if (!ExtendGroup( Group, TRUE, FIELD_OFFSET( TAG_DEF, rgb[ 0 ] ) + cb )) {
        return FALSE;
        }

    Tag = (PTAG_DEF)PTR( Group, Offset );
    Tag->wID = wID;
    Tag->dummy1 = 0;
    Tag->wItem = (int)wItem;
    Tag->dummy2 = 0;
    if (cb != 0 || rgb != NULL) {
        Tag->cb = (WORD)(cb + FIELD_OFFSET( TAG_DEF, rgb[ 0 ] ));
        memmove( &Tag->rgb[ 0 ], rgb, cb );
        }
    else {
        Tag->cb = 0;
        }

    return TRUE;
}


ULONG MonoChromePalette[] = {
    0x00000000, 0x00ffffff
};

BITMAPINFOHEADER DefaultQuestionIconBIH = {
    0x00000028,
    0x00000020,
    0x00000040,
    0x0001,
    0x0004,
    0x00000000,
    0x00000280,
    0x00000000,
    0x00000000,
    0xFFFFFFFF,
    0x00000000
};

ULONG DefaultQuestionIconBits[] = {
     0x00000000, 0x00000000, 0x00000000, 0x00000000
   , 0x00000000, 0x33000000, 0x00000030, 0x00000000
   , 0x00000000, 0xbb080000, 0x00000083, 0x00000000
   , 0x00000000, 0xff0b0000, 0x000000b3, 0x00000000
   , 0x00000000, 0xff0b0000, 0x000000b3, 0x00000000
   , 0x00000000, 0xbb000000, 0x00000080, 0x00000000
   , 0x00000000, 0x00000000, 0x00000000, 0x00000000
   , 0x00000000, 0x00000000, 0x00000000, 0x00000000
   , 0x00000000, 0x00000000, 0x00000000, 0x00000000
   , 0x00000000, 0x33000000, 0x00000030, 0x00000000
   , 0x00000000, 0xbb080000, 0x00000083, 0x00000000
   , 0x00000000, 0xbb0b0000, 0x000000b3, 0x00000000
   , 0x00000000, 0xff0b0000, 0x000000b3, 0x00000000
   , 0x00000000, 0xff0b0000, 0x000030b8, 0x00000000
   , 0x00000000, 0xbf000000, 0x000083bb, 0x00000000
   , 0x00000000, 0xbf000000, 0x0033b8fb, 0x00000000
   , 0x00000000, 0x0b000000, 0x33b8bbbf, 0x00000000
   , 0x00000000, 0x00000000, 0xb3bbfbbb, 0x00000030
   , 0x00000000, 0x00000000, 0xbbbbbb00, 0x00000030
   , 0x00000000, 0x00000000, 0xbbbb0000, 0x00000083
   , 0x00000000, 0x00000000, 0xbb0b0000, 0x000000b3
   , 0x00000000, 0x00003033, 0xbb0b0000, 0x000000b3
   , 0x08000000, 0x000083bb, 0xbb0b0000, 0x000000b3
   , 0x0b000000, 0x0000b3bb, 0xbb0b0000, 0x000000b3
   , 0x0b000000, 0x0030b8ff, 0xbb3b0000, 0x000000b3
   , 0x0b000000, 0x0083bbff, 0xbb8b0300, 0x000000b3
   , 0x00000000, 0x33b8fbbf, 0xbbbb3833, 0x00000080
   , 0x00000000, 0xbbbbffbb, 0xb8bbbbbb, 0x00000030
   , 0x00000000, 0xbbffbf0b, 0x83bbbbbb, 0x00000000
   , 0x00000000, 0xffbf0b00, 0x00b8bbff, 0x00000000
   , 0x00000000, 0xbb0b0000, 0x0000b8bb, 0x00000000
   , 0x00000000, 0x00000000, 0x00000000, 0x00000000
   , 0xff7ffcff, 0xff3ff8ff, 0xff1ff0ff, 0xff1ff0ff
   , 0xff1ff0ff, 0xff3ff8ff, 0xff7ffcff, 0xffffffff
   , 0xff7ffcff, 0xff3ff8ff, 0xff1ff0ff, 0xff1ff0ff
   , 0xff1ff0ff, 0xff0ff0ff, 0xff03f8ff, 0xff00f8ff
   , 0x7f00fcff, 0x3f00feff, 0x3f00ffff, 0x1fc0ffff
   , 0x1ff01fff, 0x1ff00ffe, 0x1ff007fc, 0x1ff007fc
   , 0x1fe003fc, 0x1f0000fc, 0x3f0000fe, 0x3f0000fe
   , 0x7f0000ff, 0xff0080ff, 0xff03e0ff, 0xff0ff8ff
};


BOOL
ConvertIconBits(
    PCURSORSHAPE_16 pIconHeader
    );

BOOL
ConvertIconBits(
    PCURSORSHAPE_16 pIconHeader
    )
{
    PBYTE Src;
    UINT cbScanLine;
    UINT nScanLine;
    UINT i, j, k;
    PBYTE Plane0;
    PBYTE Plane1;
    PBYTE Plane2;
    PBYTE Plane3;
    PBYTE p;
    BYTE Color0, Color1, Color2, Color3, FourColor, FourColorPlane[ 32 * 4 * 4 ];

    Src = (PBYTE)(pIconHeader + 1) + (pIconHeader->cbWidth * pIconHeader->cy);
    cbScanLine = (((pIconHeader->cx * pIconHeader->BitsPixel + 31) & ~31) / 8);
    nScanLine = pIconHeader->cy;

    if (pIconHeader->Planes != 4) {
        return FALSE;
        }
    else
    if (pIconHeader->BitsPixel != 1) {
        return FALSE;
        }
    else
    if (pIconHeader->cx != pIconHeader->cy) {
        return FALSE;
        }
    else
    if (nScanLine != 32) {
        return FALSE;
        }
    else
    if (cbScanLine != 4) {
        return FALSE;
        }

    Plane0 = (PBYTE)Src;
    Plane1 = Plane0 + cbScanLine;
    Plane2 = Plane1 + cbScanLine;
    Plane3 = Plane2 + cbScanLine;
    p = &FourColorPlane[ 0 ];
    j = nScanLine;
    while (j--) {
        k = cbScanLine;
        while (k--) {
            Color0 = *Plane0++;
            Color1 = *Plane1++;
            Color2 = *Plane2++;
            Color3 = *Plane3++;
            i = 4;
            while (i--) {
                FourColor = 0;
                if (Color0 & 0x80) {
                    FourColor |= 0x10;
                    }
                if (Color1 & 0x80) {
                    FourColor |= 0x20;
                    }
                if (Color2 & 0x80) {
                    FourColor |= 0x40;
                    }
                if (Color3 & 0x80) {
                    FourColor |= 0x80;
                    }
                if (Color0 & 0x40) {
                    FourColor |= 0x01;
                    }
                if (Color1 & 0x40) {
                    FourColor |= 0x02;
                    }
                if (Color2 & 0x40) {
                    FourColor |= 0x04;
                    }
                if (Color3 & 0x40) {
                    FourColor |= 0x08;
                    }

                Color0 <<= 2;
                Color1 <<= 2;
                Color2 <<= 2;
                Color3 <<= 2;

                *p++ = FourColor;
                }
            }

        Plane0 += 3 * cbScanLine;
        Plane1 += 3 * cbScanLine;
        Plane2 += 3 * cbScanLine;
        Plane3 += 3 * cbScanLine;
        }

    memmove( Src, &FourColorPlane[ 0 ], sizeof( FourColorPlane ) );
    pIconHeader->BitsPixel = 4;
    pIconHeader->Planes = 1;
    pIconHeader->cbWidth = cbScanLine * 4;
    return TRUE;
}


VOID
CopyIconBits(
    PBYTE Dst,
    PBYTE Src,
    UINT cbScanLine,
    UINT nScanLine
    );

VOID
CopyIconBits(
    PBYTE Dst,
    PBYTE Src,
    UINT cbScanLine,
    UINT nScanLine
    )
{
    Src += (cbScanLine * nScanLine);
    while (nScanLine--) {
        Src -= cbScanLine;
        memcpy( Dst, Src, cbScanLine );
        Dst += cbScanLine;
        }
}

typedef struct _WIN31_GROUP_ITEM_DESC {
    LPSTR GroupName;
    LPSTR ItemName;
} WIN31_GROUP_ITEM_DESC, *PWIN31_GROUP_ITEM_DESC;

WIN31_GROUP_ITEM_DESC GroupItemsToIgnore[] = {
    {"Main", "File Manager"},
    {NULL, "Control Panel"},
    {NULL, "Print Manager"},
    {NULL, "ClipBook Viewer"},
    {NULL, "MS-DOS Prompt"},
    {NULL, "Windows Setup"},
    {NULL, "Read Me"},
    {"Accessories", "Paintbrush"},
    {NULL, "Write"},
    {NULL, "Terminal"},
    {NULL, "Notepad"},
    {NULL, "Recorder"},
    {NULL, "Clock"},
    {NULL, "Object Packager"},
    {NULL, "Media Player"},
    {NULL, "Sound Recorder"},
    {"Network", NULL},
    {NULL, NULL}
};

PGROUP_DEF
CreateGroupFromGroup16(
    LPSTR GroupName,
    PGROUP_DEF16 Group16
    )
{
    PWIN31_GROUP_ITEM_DESC p1, ItemsToIgnore;
    PGROUP_DEF Group;
    PITEM_DEF Item;
    PITEM_DEF16 Item16;
    PTAG_DEF16 Tag16;
    DWORD cb;
    PBYTE p;
    LPSTR s;
    UINT i;
    PCURSORSHAPE_16 pIconHeader;
    PBITMAPINFOHEADER pbmi;
    BOOL bUserDefaultIcon, bItemConvertedOkay;
    int imagesize, colorsize, masksize, bmisize;

    ItemsToIgnore = NULL;
    p1 = GroupItemsToIgnore;
    while (p1->GroupName || p1->ItemName) {
        if (p1->GroupName &&
            !_stricmp( p1->GroupName, GroupName )
           ) {
            if (p1->ItemName == NULL) {
                return (PGROUP_DEF)-1;
                }

            ItemsToIgnore = p1;
            break;
            }

        p1 += 1;
        }

    Group = VirtualAlloc( NULL, 0xFFFF, MEM_RESERVE, PAGE_READWRITE );
    if (Group == NULL) {
        return NULL;
        }

    if (!VirtualAlloc( Group,
                       cb = FIELD_OFFSET( GROUP_DEF, rgiItems[ 0 ] ),
                       MEM_COMMIT,
                       PAGE_READWRITE
                     )
       ) {
        VirtualFree( Group, 0, MEM_RELEASE );
        return NULL;
        }
    cb = (DWORD)ROUND_UP( cb, sizeof( DWORD ) );
    Group->wReserved = (WORD)cb;
    Group->cbGroup = (WORD)cb;
    Group->cItems = (Group16->cItems + NSLOTS - 1) & ~(NSLOTS-1);
    AddDataToGroup( Group, NULL, (Group->cItems * sizeof( Group->rgiItems[ 0 ] )) );
    Group->pName = AddDataToGroup( Group,
                                   GroupName,
                                   0
                                 );
    Group->dwMagic = GROUP_MAGIC;
    Group->wCheckSum = 0;           /* adjusted later... */
    Group->nCmdShow = SW_SHOWMINIMIZED;     // Group16->nCmdShow;
    Group->wIconFormat = Group16->wIconFormat;

    if (lpGetSystemMetrics == NULL) {
        lpGetSystemMetrics = (PGETSYSTEMMETRICS)GetProcAddress( LoadLibrary( "user32.dll" ),
                                                                "GetSystemMetrics"
                                                              );
        if (lpGetSystemMetrics == NULL) {
            lpGetSystemMetrics = (PGETSYSTEMMETRICS)-1;
            }
        }

    if (lpGetSystemMetrics != (PGETSYSTEMMETRICS)-1) {
        Group->cxIcon = (WORD)(*lpGetSystemMetrics)(SM_CXICON);
        Group->cyIcon = (WORD)(*lpGetSystemMetrics)(SM_CYICON);
        }

    Group->ptMin.x = (LONG)Group16->ptMin.x;
    Group->ptMin.y = (LONG)Group16->ptMin.y;

    Group->rcNormal.left = (int)Group16->rcNormal.Left;
    Group->rcNormal.top = (int)Group16->rcNormal.Top;
    Group->rcNormal.right = (int)Group16->rcNormal.Right;
    Group->rcNormal.bottom = (int)Group16->rcNormal.Bottom;


    for (i=0; i<Group16->cItems; i++) {
        if (Group16->rgiItems[ i ] == 0) {
            continue;
            }

        Item16 = ITEM16( Group16, i );
        if (p1 = ItemsToIgnore) {
            s = PTR( Group16, Item16->pName );
            do {
                if (!_stricmp( p1->ItemName, s )) {
                    s = NULL;
                    break;
                    }
                p1 += 1;
                }
            while (p1->GroupName == NULL && p1->ItemName != NULL);

            if (s == NULL) {
                continue;
                }
            }

        Group->rgiItems[ i ] = AddDataToGroup( Group, NULL, sizeof( ITEM_DEF ) );
        if (Group->rgiItems[ i ] == 0) {
            break;
            }

        Item = ITEM( Group, i );
        Item->pt.x = (LONG)Item16->pt.x;
        Item->pt.y = (LONG)Item16->pt.y;
        Item->idIcon = Item16->iIcon;
        Item->indexIcon = Item16->iIcon;
        Item->wIconVer = 3;

        bUserDefaultIcon = FALSE;
        pIconHeader = (PCURSORSHAPE_16)PTR( Group16, Item16->pHeader );
        if (pIconHeader->Planes != 1) {
            if (!ConvertIconBits( pIconHeader )) {
                KdPrint(("WIN31IO: Invalid 16-bit item icon at %08x\n", pIconHeader ));
                bUserDefaultIcon = TRUE;
                }
            }
        else
        if (pIconHeader->BitsPixel != 1 &&
            pIconHeader->BitsPixel != 4 &&
            pIconHeader->BitsPixel != 8
           ) {
            bUserDefaultIcon = TRUE;
            }

        if (!bUserDefaultIcon) {
            bmisize = sizeof( BITMAPINFOHEADER );
            imagesize = Item16->cbXORPlane;
            masksize = Item16->cbANDPlane;

            if (pIconHeader->BitsPixel == 1) {
                colorsize = sizeof( MonoChromePalette );
                }
            else {
                colorsize = 0;
                }

            Item->cbIconRes = (WORD)(bmisize +
                                     colorsize +
                                     imagesize +
                                     masksize
                                    );

            Item->cbIconRes = (WORD)ROUND_UP( Item->cbIconRes, sizeof( DWORD ) );
            Item->pIconRes = AddDataToGroup( Group, NULL, Item->cbIconRes );
            if (Item->pIconRes != 0) {
                p = PTR( Group, Item->pIconRes );
                pbmi = (PBITMAPINFOHEADER)p;

                pbmi->biSize = bmisize;
                pbmi->biWidth = pIconHeader->cx;
                pbmi->biHeight = pIconHeader->cy * 2;
                pbmi->biPlanes = pIconHeader->Planes;
                pbmi->biBitCount = pIconHeader->BitsPixel;
                pbmi->biCompression = BI_RGB;
                pbmi->biSizeImage = imagesize;
                pbmi->biXPelsPerMeter = 0;
                pbmi->biYPelsPerMeter = 0;
                pbmi->biClrImportant = 0;

                if (colorsize != 0) {
                    memcpy( p + bmisize, MonoChromePalette, colorsize );
                    pbmi->biClrUsed = 0;
                    }
                else {
                    pbmi->biClrUsed = (DWORD)-1;
                    }

                CopyIconBits( p + bmisize + colorsize,
                              (PBYTE)PTR( Group16, Item16->pXORPlane ),
                              (((pIconHeader->cx * pIconHeader->BitsPixel + 31) & ~31) / 8),
                              pIconHeader->cy
                            );

                CopyIconBits( p + bmisize + colorsize + imagesize,
                              (PBYTE)PTR( Group16, Item16->pANDPlane ),
                              (((pIconHeader->cx + 31) & ~31) / 8),
                              pIconHeader->cy
                            );
                }
            }
        else {
            bmisize = sizeof( DefaultQuestionIconBIH );
            imagesize = sizeof( DefaultQuestionIconBits );
            Item->cbIconRes = bmisize + imagesize;
            Item->cbIconRes = (WORD)ROUND_UP( Item->cbIconRes, sizeof( DWORD ) );
            Item->pIconRes = AddDataToGroup( Group, NULL, Item->cbIconRes );
            if (Item->pIconRes != 0) {
                p = PTR( Group, Item->pIconRes );
                memcpy( p, &DefaultQuestionIconBIH, bmisize );
                memcpy( p + bmisize, &DefaultQuestionIconBits, imagesize );
                }
            }

        bItemConvertedOkay = FALSE;
        if (Item->pIconRes != 0) {
            Item->pName = AddDataToGroup( Group,
                                          PTR( Group16, Item16->pName ),
                                          0
                                        );
            if (Item->pName != 0) {
                Item->pCommand = AddDataToGroup( Group,
                                                 PTR( Group16, Item16->pCommand ),
                                                 0
                                               );
                if (Item->pCommand != 0) {
                    Item->pIconPath = AddDataToGroup( Group,
                                                      PTR( Group16, Item16->pIconPath ),
                                                      0
                                                    );
                    if (Item->pIconPath != 0) {
                        bItemConvertedOkay = TRUE;
                        }
                    }
                }
            }

        if (!bItemConvertedOkay) {
            break;
            }
        }

    Tag16 = (PTAG_DEF16)((PBYTE)Group16 + Group16->cbGroup);
    if (bItemConvertedOkay &&
        Tag16->wID == ID_MAGIC && Tag16->wItem == ID_LASTTAG &&
        *(UNALIGNED DWORD *)&Tag16->rgb[ 0 ] == PMTAG_MAGIC
       ) {
        while (Tag16->wID != ID_LASTTAG) {
            if (!AddTagToGroup( Group,
                                Tag16->wID,
                                Tag16->wItem,
                                (WORD)(Tag16->cb - FIELD_OFFSET( TAG_DEF16, rgb[ 0 ] )),
                                &Tag16->rgb[ 0 ]
                              )
               ) {
                bItemConvertedOkay = FALSE;
                break;
                }

            Tag16 = (PTAG_DEF16)((PBYTE)Tag16 + Tag16->cb);
            }

        if (bItemConvertedOkay) {
            if (!AddTagToGroup( Group,
                                ID_LASTTAG,
                                0xFFFF,
                                0,
                                NULL
                              )
               ) {
                bItemConvertedOkay = FALSE;
                }
            }
        }

    if (!bItemConvertedOkay) {
        UnloadGroup( Group );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
        }
    else {
        return Group;
        }
}


#if DBG

BOOL
DumpGroup(
    PWSTR GroupName,
    PGROUP_DEF Group
    )
{
    PBITMAPINFOHEADER Icon;
    PICON_HEADER16 Icon16;
    PITEM_DEF Item;
    PTAG_DEF Tag;
    PULONG p;
    int cb;
    UINT i;

    DbgPrint( "%ws - Group at %08x\n", GroupName, Group );
    DbgPrint( "     dwMagic:     %08x\n", Group->dwMagic );
    DbgPrint( "     wCheckSum:       %04x\n", Group->wCheckSum );
    DbgPrint( "     cbGroup:         %04x\n", Group->cbGroup );
    DbgPrint( "     nCmdShow:        %04x\n", Group->nCmdShow );
    DbgPrint( "     rcNormal:       [%08x,%08x,%08x,%08x]\n",
                      Group->rcNormal.left,
                      Group->rcNormal.top,
                      Group->rcNormal.right,
                      Group->rcNormal.bottom
            );
    DbgPrint( "     ptMin:          [%08x,%08x]\n", Group->ptMin.x, Group->ptMin.y );
    DbgPrint( "     pName:          [%04x] %s\n", Group->pName, Group->pName ? (PSZ)PTR( Group, Group->pName ) : "(null)" );
    DbgPrint( "     cxIcon:          %04x\n", Group->cxIcon );
    DbgPrint( "     cyIcon:          %04x\n", Group->cyIcon );
    DbgPrint( "     wIconFormat:     %04x\n", Group->wIconFormat );
    DbgPrint( "     wReserved:       %04x\n", Group->wReserved );
    DbgPrint( "     cItems:          %04x\n", Group->cItems );

    for (i=0; i<Group->cItems; i++) {
        DbgPrint( "     Item[ %02x ] at %04x\n", i, Group->rgiItems[ i ] );
        if (Group->rgiItems[ i ] != 0) {
            Item = ITEM( Group, i );
            DbgPrint( "         pt:      [%08x, %08x]\n",
                               Item->pt.x,
                               Item->pt.y
                    );
            DbgPrint( "         idIcon:   %04x\n", Item->idIcon );
            DbgPrint( "         wIconVer: %04x\n", Item->wIconVer );
            DbgPrint( "         cbIconRes:%04x\n", Item->cbIconRes );
            DbgPrint( "         indexIcon:%04x\n", Item->indexIcon );
            DbgPrint( "         dummy2:   %04x\n", Item->dummy2 );
            DbgPrint( "         pIconRes: %04x\n", Item->pIconRes );
            if (Item->wIconVer == 2) {
                Icon16 = (PICON_HEADER16)PTR( Group, Item->pIconRes );
                DbgPrint( "             xHot: %04x\n", Icon16->xHotSpot );
                DbgPrint( "             yHot: %04x\n", Icon16->yHotSpot );
                DbgPrint( "             cx:   %04x\n", Icon16->cx );
                DbgPrint( "             cy:   %04x\n", Icon16->cy );
                DbgPrint( "             cbWid:%04x\n", Icon16->cbWidth );
                DbgPrint( "             Plane:%04x\n", Icon16->Planes );
                DbgPrint( "             BPP:  %04x\n", Icon16->BitsPixel );
                p = (PULONG)(Icon16+1);
                cb = Item->cbIconRes - sizeof( *Icon16 );
                }
            else {
                Icon = (PBITMAPINFOHEADER)PTR( Group, Item->pIconRes );
                DbgPrint( "             biSize         :      %08x\n", Icon->biSize          );
                DbgPrint( "             biWidth        :      %08x\n", Icon->biWidth         );
                DbgPrint( "             biHeight       :      %08x\n", Icon->biHeight        );
                DbgPrint( "             biPlanes       :      %04x\n", Icon->biPlanes        );
                DbgPrint( "             biBitCount     :      %04x\n", Icon->biBitCount      );
                DbgPrint( "             biCompression  :      %08x\n", Icon->biCompression   );
                DbgPrint( "             biSizeImage    :      %08x\n", Icon->biSizeImage     );
                DbgPrint( "             biXPelsPerMeter:      %08x\n", Icon->biXPelsPerMeter );
                DbgPrint( "             biYPelsPerMeter:      %08x\n", Icon->biYPelsPerMeter );
                DbgPrint( "             biClrUsed      :      %08x\n", Icon->biClrUsed       );
                DbgPrint( "             biClrImportant :      %08x\n", Icon->biClrImportant  );
                p = (PULONG)(Icon+1);
                cb = Item->cbIconRes - sizeof( *Icon );
                }

            DbgPrint( "         dummy3:   %04x\n", Item->dummy3 );
            DbgPrint( "         pName:   [%04x] %s\n", Item->pName, PTR( Group, Item->pName ) );
            DbgPrint( "         pCommand:[%04x] %s\n", Item->pCommand, PTR( Group, Item->pCommand ) );
            DbgPrint( "         pIconPth:[%04x] %s\n", Item->pIconPath, PTR( Group, Item->pIconPath ) );
            DbgPrint( "         IconData: %04x bytes\n", cb );
            while (cb > 0) {
                DbgPrint( "             %08x", *p++ );
                cb -= sizeof( *p );
                if (cb >= sizeof( *p )) {
                    cb -= sizeof( *p );
                    DbgPrint( " %08x", *p++ );
                    if (cb >= sizeof( *p )) {
                        cb -= sizeof( *p );
                        DbgPrint( " %08x", *p++ );
                        if (cb >= sizeof( *p )) {
                            cb -= sizeof( *p );
                            DbgPrint( " %08x", *p++ );
                            }
                        }
                    }

                DbgPrint( "\n" );
                }
            }
        }

    Tag = (PTAG_DEF)((PBYTE)Group + Group->cbGroup);
    if (Tag->wID == ID_MAGIC && Tag->wItem == ID_LASTTAG &&
        *(LPDWORD)&Tag->rgb == PMTAG_MAGIC
       ) {
        while (Tag->wID != ID_LASTTAG) {
            DbgPrint( "     Tag at %04x\n", (PBYTE)Tag - (PBYTE)Group );
            DbgPrint( "         wID:      %04x\n", Tag->wID );
            DbgPrint( "         dummy1:   %04x\n", Tag->dummy1 );
            DbgPrint( "         wItem:    %08x\n", Tag->wItem );
            DbgPrint( "         cb:       %04x\n", Tag->cb );
            DbgPrint( "         dummy2:   %04x\n", Tag->dummy2 );
            switch( Tag->wID ) {
                case ID_MAGIC:
                    DbgPrint( "         rgb:      ID_MAGIC( %.4s )\n", Tag->rgb );
                    break;

                case ID_WRITERVERSION:
                    DbgPrint( "         rgb:      ID_WRITERVERSION( %s )\n", Tag->rgb );
                    break;

                case ID_APPLICATIONDIR:
                    DbgPrint( "         rgb:      ID_APPLICATIONDIR( %s )\n", Tag->rgb );
                    break;

                case ID_HOTKEY:
                    DbgPrint( "         rgb:      ID_HOTKEY( %04x )\n", *(LPWORD)Tag->rgb );
                    break;

                case ID_MINIMIZE:
                    DbgPrint( "         rgb:      ID_MINIMIZE()\n" );
                    break;

                case ID_LASTTAG:
                    DbgPrint( "         rgb:      ID_LASTTAG()\n" );
                    break;

                default:
                    DbgPrint( "         rgb:      unknown data format for this ID\n" );
                    break;
                }


            Tag = (PTAG_DEF)((PBYTE)Tag + Tag->cb);
            }
        }

    return TRUE;
}

#endif // DBG
