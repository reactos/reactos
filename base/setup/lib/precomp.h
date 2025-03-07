/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Precompiled header
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
 */

/* C Headers */
#include <stdio.h>
#include <stdlib.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>

#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>

#include <ntstrsafe.h>

/* Filesystem headers */
#include <reactos/rosioctl.h>   // For extra partition IDs


#ifndef SPLIBAPI
#define SPLIBAPI
#endif

//
///* Internal Headers */
//#include "interface/consup.h"
//#include "inffile.h"
//#include "inicache.h"
//#include "progress.h"
//#ifdef __REACTOS__
//#include "infros.h"
//#include "filequeue.h"
//#endif

//#include "registry.h"
//#include "fslist.h"
//#include "partlist.h"
//#include "cabinet.h"
//#include "filesup.h"
//#include "genlist.h"

extern HANDLE ProcessHeap;

#include "errorcode.h"
#include "utils/linklist.h"
