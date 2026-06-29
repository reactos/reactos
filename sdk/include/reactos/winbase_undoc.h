/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     Dual-licensed:
 *              LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 *              MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Undocumented Base API definitions, absent from winbase.h
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *
 * REMARK: This header is based on the following files from the official
 * Windows 10.0.10240.0 PSDK, a copy of which can be found at:
 * - https://github.com/tpn/winsdk-10/blob/master/Include/10.0.10240.0/um/minwin/winbasep.h
 * - https://github.com/tpn/winsdk-10/blob/master/Include/10.0.10240.0/um/minwin/wbasek.h
 */

#ifndef _WINBASE_UNDOC_H
#define _WINBASE_UNDOC_H

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Undocumented flags for CreateProcess
 */
#if (WINVER >= 0x400)
#define STARTF_HASSHELLDATA     0x00000400 // As seen in um/minwin/winbasep.h
#define STARTF_SHELLPRIVATE     STARTF_HASSHELLDATA // ReactOS-specific name
#endif /* (WINVER >= 0x400) */
#if (WINVER >= 0x0A00)
#define STARTF_TITLEISLOCALALLOCED  0x00004000
#endif /* (WINVER >= 0x0A00) */
#define STARTF_INHERITDESKTOP   0x40000000
#define STARTF_SCREENSAVER      0x80000000

#ifdef __cplusplus
}
#endif

#endif /* _WINBASE_UNDOC_H */
