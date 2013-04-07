/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
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


/* Globals */
extern HANDLE BaseSrvHeap;
extern HANDLE BaseSrvSharedHeap;
extern PBASE_STATIC_SERVER_DATA BaseStaticServerData;

#endif // __BASESRV_H__

/* EOF */
