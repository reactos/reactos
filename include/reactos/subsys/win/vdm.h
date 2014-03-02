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

typedef struct
{
    ULONG TaskId;
    ULONG Unused;
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

#endif // _VDM_H

/* EOF */
