/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS/Win32 Base enviroment Subsystem Server
 * FILE:            subsystems/win/basesrv/basesrv.h
 * PURPOSE:         Main header - Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __BASESRV_H__
#define __BASESRV_H__

#pragma once

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* CSRSS Header */
#include <csr/csrsrv.h>

/* BASE Headers */
#include <win/basemsg.h>
#include <win/base.h>


extern HANDLE BaseSrvHeap;
extern HANDLE BaseSrvSharedHeap;
extern PBASE_STATIC_SERVER_DATA BaseStaticServerData;

/* dosdev.c */
VOID BaseInitDefineDosDevice(VOID);
VOID BaseCleanupDefineDosDevice(VOID);

CSR_API(BaseSrvDefineDosDevice);

/* proc.c */
CSR_API(BaseSrvGetTempFile);
CSR_API(BaseSrvCreateProcess);
CSR_API(BaseSrvCreateThread);
CSR_API(BaseSrvExitProcess);
CSR_API(BaseSrvGetProcessShutdownParam);
CSR_API(BaseSrvSetProcessShutdownParam);

/* sndsntry.c */
CSR_API(BaseSrvSoundSentryNotification);

#endif // __BASESRV_H__

/* EOF */
