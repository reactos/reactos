/*
 * Copyright 1999, 2000 Peter Hunnisett
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_DPINIT_H
#define __WINE_DPINIT_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "dplay_global.h"

extern HRESULT DP_CreateInterface( REFIID riid, LPVOID* ppvObj );
extern HRESULT DPL_CreateInterface( REFIID riid, LPVOID* ppvObj );
extern HRESULT DPSP_CreateInterface( REFIID riid, LPVOID* ppvObj,
                                     IDirectPlay2Impl* dp );
extern HRESULT DPLSP_CreateInterface( REFIID riid, LPVOID* ppvObj,
                                      IDirectPlay2Impl* dp );


#endif
