/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            include/reactos/subsys/win/vdm.h
 * PURPOSE:         Public definitions for the Virtual Dos Machine
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 */

#ifndef _VDM_H
#define _VDM_H

#pragma once

/* CONSTANTS & TYPES **********************************************************/

typedef enum _VDM_ENTRY_CODE
{
    VdmEntryUndo,
    VdmEntryUpdateProcess,
    VdmEntryUpdateControlCHandler
} VDM_ENTRY_CODE;

//
// Undo States
//
#define VDM_UNDO_PARTIAL    0x01
#define VDM_UNDO_FULL       0x02
#define VDM_UNDO_REUSE      0x04
#define VDM_UNDO_COMPLETED  0x08

//
// Binary Types to share with VDM
//
#define BINARY_TYPE_EXE     0x01
#define BINARY_TYPE_COM     0x02
#define BINARY_TYPE_PIF     0x03
#define BINARY_TYPE_DOS     0x10
#define BINARY_TYPE_SEPARATE_WOW 0x20
#define BINARY_TYPE_WOW     0x40
#define BINARY_TYPE_WOW_EX  0x80

//
// VDM States
//
#define VDM_NOT_LOADED      0x01
#define VDM_NOT_READY       0x02
#define VDM_READY           0x04

//
// VDM Flags
//
#define VDM_FLAG_FIRST_TASK     0x01
#define VDM_FLAG_WOW            0x02
#define VDM_FLAG_DOS            0x04
#define VDM_FLAG_RETRY          0x08
#define VDM_INC_REENTER_COUNT   0x10
#define VDM_DEC_REENTER_COUNT   0x20
#define VDM_FLAG_NESTED_TASK    0x40
#define VDM_FLAG_DONT_WAIT      0x80
#define VDM_GET_FIRST_COMMAND   0x100
#define VDM_GET_ENVIRONMENT     0x400
#define VDM_FLAG_SEPARATE_WOW   0x800
#define VDM_LIST_WOW_PROCESSES  0x1000
#define VDM_LIST_WOW_TASKS      0x4000
#define VDM_ADD_WOW_TASK        0x8000

typedef struct
{
    ULONG TaskId;
    ULONG CreationFlags;
    ULONG ExitCode;
    ULONG CodePage;
    HANDLE StdIn;
    HANDLE StdOut;
    HANDLE StdErr;
    LPSTR CmdLine;
    LPSTR AppName;
    LPSTR PifFile;
    LPSTR CurDirectory;
    LPSTR Env;
    ULONG EnvLen;
    STARTUPINFOA StartupInfo;
    LPSTR Desktop;
    ULONG DesktopLen;
    LPSTR Title;
    ULONG TitleLen;
    LPVOID Reserved;
    ULONG ReservedLen;
    USHORT CmdLen;
    USHORT AppLen;
    USHORT PifLen;
    USHORT CurDirectoryLen;
    USHORT VDMState;
    USHORT CurrentDrive;
    BOOLEAN ComingFromBat;
} VDM_COMMAND_INFO, *PVDM_COMMAND_INFO;


/* FUNCTION PROTOTYPES ********************************************************/

BOOL
WINAPI
GetNextVDMCommand(
    IN OUT PVDM_COMMAND_INFO CommandData OPTIONAL
);

VOID
WINAPI
ExitVDM(
    IN BOOL IsWow,
    IN ULONG iWowTask
);

#endif // _VDM_H

/* EOF */
