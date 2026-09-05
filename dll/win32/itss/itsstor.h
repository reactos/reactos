/*
 *    ITSS Storage implementation
 *
 * Copyright 2004 Mike McCormack
 *
 *  see http://bonedaddy.net/pabs3/hhm/#chmspec
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_ITS_STORAGE_PRIVATE__
#define __WINE_ITS_STORAGE_PRIVATE__

extern HRESULT ITSS_StgOpenStorage( 
    const WCHAR* pwcsName,
    IStorage* pstgPriority,
    DWORD grfMode,
    SNB snbExclude,
    DWORD reserved,
    IStorage** ppstgOpen);

extern HRESULT ITS_IParseDisplayName_create(IUnknown *pUnkOuter, LPVOID *ppObj);

extern HRESULT ITSProtocol_create(IUnknown *pUnkOuter, LPVOID *ppobj);

extern LONG dll_count;
static inline void ITSS_LockModule(void) { InterlockedIncrement(&dll_count); }
static inline void ITSS_UnlockModule(void) { InterlockedDecrement(&dll_count); }

#endif /* __WINE_ITS_STORAGE_PRIVATE__ */
