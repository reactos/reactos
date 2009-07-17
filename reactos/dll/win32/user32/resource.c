/*
 * USER resource functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "winnls.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wownt32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(resource);
WINE_DECLARE_DEBUG_CHANNEL(accel);

/* this is the 8 byte accel struct used in Win32 resources (internal only) */
typedef struct
{
    WORD   fVirt;
    WORD   key;
    WORD   cmd;
    WORD   pad;
} PE_ACCEL, *LPPE_ACCEL;

/**********************************************************************
 *			LoadAccelerators	[USER.177]
 */
HACCEL16 WINAPI LoadAccelerators16(HINSTANCE16 instance, LPCSTR lpTableName)
{
    HRSRC16	hRsrc;

    TRACE_(accel)("%04x %s\n", instance, debugstr_a(lpTableName) );

    if (!(hRsrc = FindResource16( instance, lpTableName, (LPSTR)RT_ACCELERATOR ))) {
      WARN_(accel)("couldn't find accelerator table resource\n");
      return 0;
    }

    TRACE_(accel)("returning HACCEL 0x%x\n", hRsrc);
    return LoadResource16(instance,hRsrc);
}

/**********************************************************************
 *			LoadAcceleratorsW	(USER32.@)
 * The image layout seems to look like this (not 100% sure):
 * 00:	WORD	type		type of accelerator
 * 02:	WORD	event
 * 04:	WORD	IDval
 * 06:	WORD	pad		(to DWORD boundary)
 */
HACCEL WINAPI LoadAcceleratorsW(HINSTANCE instance,LPCWSTR lpTableName)
{
    HRSRC hRsrc;
    HACCEL hMem;
    HACCEL16 hRetval=0;
    DWORD size;

    if (HIWORD(lpTableName))
        TRACE_(accel)("%p '%s'\n", instance, (const char *)( lpTableName ) );
    else
        TRACE_(accel)("%p 0x%04x\n", instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResourceW( instance, lpTableName, (LPWSTR)RT_ACCELERATOR )))
    {
      WARN_(accel)("couldn't find accelerator table resource\n");
    } else {
      hMem = LoadResource( instance, hRsrc );
      size = SizeofResource( instance, hRsrc );
      if(size>=sizeof(PE_ACCEL))
      {
	LPPE_ACCEL accel_table = (LPPE_ACCEL) hMem;
	LPACCEL16 accel16;
	int i,nrofaccells = size/sizeof(PE_ACCEL);

	hRetval = GlobalAlloc16(0,sizeof(ACCEL16)*nrofaccells);
	accel16 = (LPACCEL16)GlobalLock16(hRetval);
	for (i=0;i<nrofaccells;i++) {
          accel16[i].fVirt = accel_table[i].fVirt & 0x7f;
          accel16[i].key = accel_table[i].key;
          if( !(accel16[i].fVirt & FVIRTKEY) )
            accel16[i].key &= 0x00ff;
          accel16[i].cmd = accel_table[i].cmd;
	}
	accel16[i-1].fVirt |= 0x80;
      }
    }
    TRACE_(accel)("returning HACCEL %p\n", hRsrc);
    return HACCEL_32(hRetval);
}

/***********************************************************************
 *		LoadAcceleratorsA   (USER32.@)
 */
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE instance,LPCSTR lpTableName)
{
    INT len;
    LPWSTR uni;
    HACCEL result = 0;

    if (!HIWORD(lpTableName)) return LoadAcceleratorsW( instance, (LPCWSTR)lpTableName );

    len = MultiByteToWideChar( CP_ACP, 0, lpTableName, -1, NULL, 0 );
    if ((uni = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, lpTableName, -1, uni, len );
        result = LoadAcceleratorsW(instance,uni);
        HeapFree( GetProcessHeap(), 0, uni);
    }
    return result;
}

/**********************************************************************
 *             CopyAcceleratorTableA   (USER32.@)
 */
INT WINAPI CopyAcceleratorTableA(HACCEL src, LPACCEL dst, INT entries)
{
  return CopyAcceleratorTableW(src, dst, entries);
}

/**********************************************************************
 *             CopyAcceleratorTableW   (USER32.@)
 *
 * By mortene@pvv.org 980321
 */
INT WINAPI CopyAcceleratorTableW(HACCEL src, LPACCEL dst,
				     INT entries)
{
  int i,xsize;
  LPACCEL16 accel = (LPACCEL16)GlobalLock16(HACCEL_16(src));
  BOOL done = FALSE;

  /* Do parameter checking to avoid the explosions and the screaming
     as far as possible. */
  if((dst && (entries < 1)) || (src == NULL) || !accel) {
    WARN_(accel)("Application sent invalid parameters (%p %p %d).\n",
         src, dst, entries);
    return 0;
  }
  xsize = GlobalSize16(HACCEL_16(src))/sizeof(ACCEL16);
  if (xsize<entries) entries=xsize;

  i=0;
  while(!done) {
    /* Spit out some debugging information. */
    TRACE_(accel)("accel %d: type 0x%02x, event '%c', IDval 0x%04x.\n",
	  i, accel[i].fVirt, accel[i].key, accel[i].cmd);

    /* Copy data to the destination structure array (if dst == NULL,
       we're just supposed to count the number of entries). */
    if(dst) {
      dst[i].fVirt = accel[i].fVirt&0x7f;
      dst[i].key = accel[i].key;
      dst[i].cmd = accel[i].cmd;

      /* Check if we've reached the end of the application supplied
         accelerator table. */
      if(i+1 == entries)
	done = TRUE;
    }

    /* The highest order bit seems to mark the end of the accelerator
       resource table, but not always. Use GlobalSize() check too. */
    if((accel[i].fVirt & 0x80) != 0) done = TRUE;

    i++;
  }

  return i;
}

/*********************************************************************
 *                    CreateAcceleratorTableA   (USER32.@)
 *
 * By mortene@pvv.org 980321
 */
HACCEL WINAPI CreateAcceleratorTableA(LPACCEL lpaccel, INT cEntries)
{
  HACCEL	hAccel;
  LPACCEL16	accel;
  int		i;

  /* Do parameter checking just in case someone's trying to be
     funny. */
  if(cEntries < 1) {
    WARN_(accel)("Application sent invalid parameters (%p %d).\n",
	 lpaccel, cEntries);
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
  }

  /* Allocate memory and copy the table. */
  hAccel = HACCEL_32(GlobalAlloc16(0,cEntries*sizeof(ACCEL16)));

  TRACE_(accel)("handle %p\n", hAccel);
  if(!hAccel) {
    ERR_(accel)("Out of memory.\n");
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }
  accel = GlobalLock16(HACCEL_16(hAccel));
  for (i=0;i<cEntries;i++) {
    accel[i].fVirt = lpaccel[i].fVirt&0x7f;
    accel[i].key = lpaccel[i].key;
    if( !(accel[i].fVirt & FVIRTKEY) )
      accel[i].key &= 0x00ff;
    accel[i].cmd = lpaccel[i].cmd;
  }
  /* Set the end-of-table terminator. */
  accel[cEntries-1].fVirt |= 0x80;

  TRACE_(accel)("Allocated accelerator handle %p with %d entries\n", hAccel,cEntries);
  return hAccel;
}

/*********************************************************************
 *                    CreateAcceleratorTableW   (USER32.@)
 *
 *
 */
HACCEL WINAPI CreateAcceleratorTableW(LPACCEL lpaccel, INT cEntries)
{
  HACCEL	hAccel;
  LPACCEL16	accel;
  int		i;
  char		ckey;

  /* Do parameter checking just in case someone's trying to be
     funny. */
  if(cEntries < 1) {
    WARN_(accel)("Application sent invalid parameters (%p %d).\n",
	 lpaccel, cEntries);
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
  }

  /* Allocate memory and copy the table. */
  hAccel = HACCEL_32(GlobalAlloc16(0,cEntries*sizeof(ACCEL16)));

  TRACE_(accel)("handle %p\n", hAccel);
  if(!hAccel) {
    ERR_(accel)("Out of memory.\n");
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }
  accel = GlobalLock16(HACCEL_16(hAccel));


  for (i=0;i<cEntries;i++) {
       accel[i].fVirt = lpaccel[i].fVirt&0x7f;
       if( !(accel[i].fVirt & FVIRTKEY) ) {
	  ckey = (char) lpaccel[i].key;
         if(!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &ckey, 1, &accel[i].key, 1))
            WARN_(accel)("Error converting ASCII accelerator table to Unicode\n");
       }
       else
         accel[i].key = lpaccel[i].key;
       accel[i].cmd = lpaccel[i].cmd;
  }

  /* Set the end-of-table terminator. */
  accel[cEntries-1].fVirt |= 0x80;

  TRACE_(accel)("Allocated accelerator handle %p\n", hAccel);
  return hAccel;
}

/******************************************************************************
 * DestroyAcceleratorTable [USER32.@]
 * Destroys an accelerator table
 *
 * NOTES
 *    By mortene@pvv.org 980321
 *
 * PARAMS
 *    handle [I] Handle to accelerator table
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DestroyAcceleratorTable( HACCEL handle )
{
    if( !handle )
        return FALSE;
    return !GlobalFree16(HACCEL_16(handle));
}

/**********************************************************************
 *     LoadString   (USER.176)
 */
INT16 WINAPI LoadString16( HINSTANCE16 instance, UINT16 resource_id,
                           LPSTR buffer, INT16 buflen )
{
    HGLOBAL16 hmem;
    HRSRC16 hrsrc;
    unsigned char *p;
    int string_num;
    int i;

    TRACE("inst=%04x id=%04x buff=%p len=%d\n",
          instance, resource_id, buffer, buflen);

    hrsrc = FindResource16( instance, MAKEINTRESOURCEA((resource_id>>4)+1), (LPSTR)RT_STRING );
    if (!hrsrc) return 0;
    hmem = LoadResource16( instance, hrsrc );
    if (!hmem) return 0;

    p = LockResource16(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;

    TRACE("strlen = %d\n", (int)*p );

    if (buffer == NULL) return *p;
    i = min(buflen - 1, *p);
    if (i > 0) {
	memcpy(buffer, p + 1, i);
	buffer[i] = '\0';
    } else {
	if (buflen > 1) {
	    buffer[0] = '\0';
	    return 0;
	}
	WARN("Don't know why caller gave buflen=%d *p=%d trying to obtain string '%s'\n", buflen, *p, p + 1);
    }
    FreeResource16( hmem );

    TRACE("'%s' loaded !\n", buffer);
    return i;
}

/**********************************************************************
 *	LoadStringW		(USER32.@)
 */
INT WINAPI LoadStringW( HINSTANCE instance, UINT resource_id,
                            LPWSTR buffer, INT buflen )
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    TRACE("instance = %p, id = %04x, buffer = %p, length = %d\n",
          instance, resource_id, buffer, buflen);

    if(buffer == NULL)
        return 0;

    /* Use loword (incremented by 1) as resourceid */
    hrsrc = FindResourceW( instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1),
                           (LPWSTR)RT_STRING );
    if (!hrsrc) return 0;
    hmem = LoadResource( instance, hrsrc );
    if (!hmem) return 0;

    p = LockResource(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;

    TRACE("strlen = %d\n", (int)*p );

    /*if buflen == 0, then return a read-only pointer to the resource itself in buffer
    it is assumed that buffer is actually a (LPWSTR *) */
    if(buflen == 0)
    {
        *((LPWSTR *)buffer) = p + 1;
        return *p;
    }

    i = min(buflen - 1, *p);
    if (i > 0) {
	memcpy(buffer, p + 1, i * sizeof (WCHAR));
        buffer[i] = 0;
    } else {
	if (buflen > 1) {
            buffer[0] = 0;
	    return 0;
	}
    }

    TRACE("%s loaded !\n", debugstr_w(buffer));
    return i;
}

/**********************************************************************
 *	LoadStringA	(USER32.@)
 */
INT WINAPI LoadStringA( HINSTANCE instance, UINT resource_id, LPSTR buffer, INT buflen )
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    DWORD retval = 0;

    TRACE("instance = %p, id = %04x, buffer = %p, length = %d\n",
          instance, resource_id, buffer, buflen);

    if (!buflen) return -1;

    /* Use loword (incremented by 1) as resourceid */
    if ((hrsrc = FindResourceW( instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1),
                                (LPWSTR)RT_STRING )) &&
        (hmem = LoadResource( instance, hrsrc )))
    {
        const WCHAR *p = LockResource(hmem);
        unsigned int id = resource_id & 0x000f;

        while (id--) p += *p + 1;

        RtlUnicodeToMultiByteN( buffer, buflen - 1, &retval, p + 1, *p * sizeof(WCHAR) );
    }
    buffer[retval] = 0;
    TRACE("returning %s\n", debugstr_a(buffer));
    return retval;
}

/**********************************************************************
 *	GetGuiResources	(USER32.@)
 */
DWORD WINAPI GetGuiResources( HANDLE hProcess, DWORD uiFlags )
{
    static BOOL warn = TRUE;

    if (warn) {
        FIXME("(%p,%x): stub\n",hProcess,uiFlags);
       warn = FALSE;
    }

    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}
