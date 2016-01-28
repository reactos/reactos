/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/include/settings.h
 * PURPOSE:         Public Console Settings Management Interface
 * PROGRAMMERS:     Johannes Anderwald
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#include "concfg/settings.h"

/* MACROS *********************************************************************/

// WARNING! Redefinitions of macros from concfg/settings.h

/*
 * BYTE Foreground = LOBYTE(Attributes) & 0x0F;
 * BYTE Background = (LOBYTE(Attributes) & 0xF0) >> 4;
 */
#define RGBFromAttrib(Console, Attribute)   ((Console)->Colors[(Attribute) & 0xF])
#define TextAttribFromAttrib(Attribute)     ( !((Attribute) & COMMON_LVB_REVERSE_VIDEO) ? (Attribute) & 0xF : ((Attribute) >> 4) & 0xF )
#define BkgdAttribFromAttrib(Attribute)     ( !((Attribute) & COMMON_LVB_REVERSE_VIDEO) ? ((Attribute) >> 4) & 0xF : (Attribute) & 0xF )
#define MakeAttrib(TextAttrib, BkgdAttrib)  (USHORT)((((BkgdAttrib) & 0xF) << 4) | ((TextAttrib) & 0xF))

/* FUNCTIONS ******************************************************************/

VOID ConSrvApplyUserSettings(IN PCONSOLE Console,
                             IN PCONSOLE_STATE_INFO ConsoleInfo);

/* EOF */
