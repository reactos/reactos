/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    regdat16.c

Abstract:

    This module contains code for reading the Windows 3.1 registry
    file (REG.DAT)

Author:

    Steve Wood (stevewo) 22-Feb-1993

Revision History:

--*/

#include "advapi.h"
#include <stdio.h>

#include <winbasep.h>
#include "win31io.h"

// #define SHOW_TREE_MIGRATION

#define MAX_LEVELS 64

PREG_HEADER16
LoadRegistry16(
    PWSTR RegistryFileName
    )
{
    HANDLE File, Mapping;
    LPVOID Base;

    File = CreateFileW( RegistryFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                      );

    if (File == INVALID_HANDLE_VALUE) {
        return NULL;
        }


    Mapping = CreateFileMapping( File,
                                 NULL,
                                 PAGE_WRITECOPY,
                                 0,
                                 0,
                                 NULL
                               );
    CloseHandle( File );
    if (Mapping == NULL) {
        return NULL;
        }


    Base = MapViewOfFile( Mapping,
                          FILE_MAP_COPY,
                          0,
                          0,
                          0
                        );
    CloseHandle( Mapping );
    return (PREG_HEADER16)Base;
}


BOOL
UnloadRegistry16(
    PREG_HEADER16 Registry
    )
{
    return UnmapViewOfFile( (LPVOID)Registry );
}


typedef struct _REG16_PATH_SUBST {
    ULONG cbOldValue;
    LPSTR OldValue;
    LPSTR NewValue;
} REG16_PATH_SUBST, *PREG16_PATH_SUBST;

REG16_PATH_SUBST Reg16PathSubstitutes[] = {
    {10, "mciole.dll",   "mciole16.dll"},
    {11, "mplayer.exe",  "mplay32.exe" },
    {12, "packager.exe", "packgr32.exe"},
    {12, "soundrec.exe", "sndrec32.exe"},
    {0, NULL, NULL}
};

typedef struct _REG16_WALK_STATE {
    HANDLE KeyHandle;
    WORD NodeIndex;
    WORD Reserved;
} REG16_WALK_STATE, *PREG16_WALK_STATE;

char *szBlanks = "                                                                                 ";

BOOL
CreateRegistryClassesFromRegistry16(
    HANDLE SoftwareRoot,
    PREG_HEADER16 Registry
    )
{
    PREG_KEY16 KeyNode;
    PREG_STRING16 KeyNameString, KeyValueString;
    PREG_NODE16 NodeTable;
    PCH StringTable;
    int i;
    DWORD Level;
    REG16_WALK_STATE State[ MAX_LEVELS ];
    ANSI_STRING AnsiString;
    UNICODE_STRING KeyName;
    UNICODE_STRING KeyValue;
    UNICODE_STRING ValueName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG CreateDisposition;
    PREG16_PATH_SUBST PathSubst;

    RtlInitUnicodeString( &ValueName, NULL );

    NodeTable = (PREG_NODE16)((PBYTE)Registry + Registry->dwNodeTable);
    StringTable = (PCH)Registry + Registry->dwStringValue;

    i = NodeTable[ 0 ].key.iChild;
    Level = 0;
    while (i != 0) {
        KeyNode = &NodeTable[ i ].key;
        KeyNameString = &NodeTable[ KeyNode->iKey ].str;
        AnsiString.Length = KeyNameString->cb;
        AnsiString.MaximumLength = AnsiString.Length;
        AnsiString.Buffer = StringTable + KeyNameString->irgb;
        if (Level == 0 && *AnsiString.Buffer == '.') {
            AnsiString.Buffer += 1;
            AnsiString.Length -= 1;
            }
        RtlAnsiStringToUnicodeString( &KeyName, &AnsiString, TRUE );

#ifdef SHOW_TREE_MIGRATION
        DbgPrint( "%.*s%wZ", Level * 4, szBlanks, &KeyName );
#endif

        InitializeObjectAttributes( &ObjectAttributes,
                                    &KeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    Level == 0 ? SoftwareRoot : State[ Level-1 ].KeyHandle,
                                    NULL
                                  );
        Status = NtCreateKey( &State[ Level ].KeyHandle,
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

        RtlFreeUnicodeString( &KeyName );
        if (!NT_SUCCESS( Status )) {
#ifdef SHOW_TREE_MIGRATION
            DbgPrint( " *** CreateKey failed with Status == %x\n", Status );
#endif
            BaseSetLastNTError( Status );
            break;
            }
#ifdef SHOW_TREE_MIGRATION
        DbgPrint( "%s\n", CreateDisposition == REG_CREATED_NEW_KEY ? " *** NEW ***" : "" );
#endif

        if (KeyNode->iValue != 0) {
            ULONG cb;
            LPSTR s, Src, Dst;

            KeyValueString = &NodeTable[ KeyNode->iValue ].str;
            cb = KeyValueString->cb;
            Src = StringTable + KeyValueString->irgb;
            Dst = RtlAllocateHeap( RtlProcessHeap(), 0, 2*(cb+1) );
            if (Dst != NULL) {
                AnsiString.Length = 0;
                AnsiString.Buffer = Dst;
                while (cb) {
                    PathSubst = &Reg16PathSubstitutes[ 0 ];
                    while (PathSubst->OldValue) {
                        if (cb >= PathSubst->cbOldValue &&
                            !_strnicmp( Src, PathSubst->OldValue, PathSubst->cbOldValue )
                           ) {
                            *Dst = '\0';
                            while (Dst > AnsiString.Buffer) {
                                if (Dst[ -1 ] <= ' ') {
                                    break;
                                    }
                                else {
                                    Dst -= 1;
                                    }
                                }

                            s = PathSubst->NewValue;
#ifdef SHOW_TREE_MIGRATION
                            DbgPrint( " Found '%s%s' changed to '%s' ", Dst, PathSubst->OldValue, PathSubst->NewValue );
#else
                            KdPrint(( "ADVAPI: Found '%s%s' changed to '%s'\n", Dst, PathSubst->OldValue, PathSubst->NewValue ));
#endif
                            Src += PathSubst->cbOldValue;
                            cb -= PathSubst->cbOldValue;
                            while (*Dst = *s++) {
                                Dst += 1;
                                }

                            break;
                            }
                        else {
                            PathSubst += 1;
                            }
                        }

                    if (PathSubst->OldValue == NULL) {
                        *Dst++ = *Src++;
                        cb -= 1;
                        }
                    }
                *Dst = '\0';

                AnsiString.Length = (USHORT)(Dst - AnsiString.Buffer);
                AnsiString.MaximumLength = (USHORT)(AnsiString.Length + 1);
                RtlAnsiStringToUnicodeString( &KeyValue, &AnsiString, TRUE );
                RtlFreeHeap( RtlProcessHeap(), 0, AnsiString.Buffer );

#ifdef SHOW_TREE_MIGRATION
                DbgPrint( "%.*s= (%u, %u) %wZ", (Level+1) * 4, szBlanks, KeyValueString->cb, cb, &KeyValue );
#endif

                Status = NtSetValueKey( State[ Level ].KeyHandle,
                                        &ValueName,
                                        0,
                                        REG_SZ,
                                        KeyValue.Buffer,
                                        KeyValue.Length + sizeof( UNICODE_NULL )
                                      );
                RtlFreeUnicodeString( &KeyValue );

                if (!NT_SUCCESS( Status )) {
#ifdef SHOW_TREE_MIGRATION
                    DbgPrint( " *** SetValueKey failed with Status == %x\n", Status );
#endif
                    BaseSetLastNTError( Status );
                    break;
                    }
#ifdef SHOW_TREE_MIGRATION
                DbgPrint( "\n" );
#endif
                }
            }

        if (KeyNode->iChild != 0) {
            State[ Level++ ].NodeIndex = KeyNode->iNext;
            State[ Level ].KeyHandle = NULL;
            i = KeyNode->iChild;
            }
        else {
            NtClose( State[ Level ].KeyHandle );
            if (KeyNode->iNext != 0) {
                i = KeyNode->iNext;
                }
            else {
                while (Level != 0) {
                    Level -= 1;
                    NtClose( State[ Level ].KeyHandle );

                    if (i = State[ Level ].NodeIndex) {
                        break;
                        }
                    }
                }
            }
        }

    if (Level == 0) {
        return TRUE;
        }


    while (Level != 0 && (i = State[ --Level ].NodeIndex) == 0) {
        NtClose( State[ Level ].KeyHandle );
        }

    return FALSE;
}


#if DBG
char *Blanks = "                                                                                 ";

BOOL
DumpRegistry16(
    PREG_HEADER16 Registry
    )
{
    PREG_KEY16 KeyNode;
    PREG_STRING16 StringNode, KeyNameString, KeyValueString;
    PREG_NODE16 NodeTable;
    PCH StringTable, String, KeyName, KeyValue;
    int i, j;
    DWORD Level;
    WORD LevelNode[ MAX_LEVELS ];

    NodeTable = (PREG_NODE16)((PBYTE)Registry + Registry->dwNodeTable);
    StringTable = (PCH)Registry + Registry->dwStringValue;

    DbgPrint( "Windows 3.1 Registry data at %08x\n", Registry );
    DbgPrint( "    dwMagic:       %08x\n", Registry->dwMagic );
    DbgPrint( "    dwVersion:     %08x\n", Registry->dwVersion );
    DbgPrint( "    dwHdrSize:     %08x\n", Registry->dwHdrSize );
    DbgPrint( "    dwNodeTable:   %08x\n", Registry->dwNodeTable );
    DbgPrint( "    dwNTSize:      %08x\n", Registry->dwNTSize );
    DbgPrint( "    dwStringValue: %08x\n", Registry->dwStringValue );
    DbgPrint( "    dwSVSize:      %08x\n", Registry->dwSVSize );
    DbgPrint( "    nHash:         %04x\n", Registry->nHash );
    DbgPrint( "    iFirstFree:    %04x\n", Registry->iFirstFree );

    i = NodeTable[ 0 ].key.iChild;
    Level = 0;
    while (i != 0) {
        KeyNode = &NodeTable[ i ].key;
        KeyNameString = &NodeTable[ KeyNode->iKey ].str;
        KeyName = StringTable + KeyNameString->irgb;
        DbgPrint( "%.*s%.*s\n",
                  Level * 4,
                  Blanks,
                  KeyNameString->cb,
                  KeyName
                );
        if (KeyNode->iValue != 0) {
            KeyValueString = &NodeTable[ KeyNode->iValue ].str;
            KeyValue = StringTable + KeyValueString->irgb;
            DbgPrint( "%.*s= %.*s\n",
                      (Level+1) * 4,
                      Blanks,
                      KeyValueString->cb,
                      KeyValue
                    );
            }

        if (KeyNode->iChild != 0) {
            LevelNode[ Level++ ] = KeyNode->iNext;
            i = KeyNode->iChild;
            }
        else
        if (KeyNode->iNext != 0) {
            i = KeyNode->iNext;
            }
        else {
            while (Level != 0 && (i = LevelNode[ --Level ]) == 0) {
                ;
                }
            }
        }

    return TRUE;
}

#endif // DBG
