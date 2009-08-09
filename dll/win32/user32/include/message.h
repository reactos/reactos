/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/message.h
 * PURPOSE:     Message management definitions
 */

#ifndef LIB_USER32_INCLUDE_MESSAGE_H
#define LIB_USER32_INCLUDE_MESSAGE_H

BOOL FASTCALL MessageInit(VOID);
VOID FASTCALL MessageCleanup(VOID);

#define WM_ALTTABACTIVE         0x0029
#define WM_SETVISIBLE           0x0009

#endif /* LIB_USER32_INCLUDE_MESSAGE_H */
