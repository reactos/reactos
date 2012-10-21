
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

VOID BaseCleanupDefineDosDevice(VOID);

CSR_API(BaseSrvCreateProcess);
CSR_API(BaseSrvCreateThread);
CSR_API(BaseSrvGetTempFile);
CSR_API(BaseSrvExitProcess);
CSR_API(BaseSrvGetProcessShutdownParam);
CSR_API(BaseSrvSetProcessShutdownParam);
CSR_API(BaseSrvSoundSentryNotification);
CSR_API(BaseSrvDefineDosDevice);

#endif // __BASESRV_H__

/* EOF */
