/*
 * PROJECT:     ReactOS Client/Server Runtime SubSystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CSR Client Library - Main Header
 * COPYRIGHT:   Copyright 2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#ifndef _CSRLIB_H_
#define _CSRLIB_H_

/* INCLUDES ******************************************************************/

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
//#include <windef.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

/* CSRSS Headers */
#include <csr.h>

/* GLOBALS ********************************************************************/

extern HANDLE CsrApiPort;
extern HANDLE CsrPortHeap;

#endif /* _CSRLIB_H_ */

/* EOF */
