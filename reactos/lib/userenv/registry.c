/* $Id: registry.c,v 1.2 2004/02/28 11:30:59 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/registry.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include <ntos.h>
#include <windows.h>
#include <string.h>

#include "internal.h"


/* FUNCTIONS ***************************************************************/

static BOOL
CopyKey (HKEY hDstKey,
	 HKEY hSrcKey)
{
  FILETIME LastWrite;
  DWORD dwSubKeys;
  DWORD dwValues;
  DWORD dwType;
  DWORD dwMaxSubKeyNameLength;
  DWORD dwSubKeyNameLength;
  DWORD dwMaxValueNameLength;
  DWORD dwValueNameLength;
  DWORD dwMaxValueLength;
  DWORD dwValueLength;
  DWORD dwDisposition;
  DWORD i;
  LPWSTR lpNameBuffer;
  LPBYTE lpDataBuffer;
  HKEY hDstSubKey;
  HKEY hSrcSubKey;

  DPRINT ("CopyKey() called \n");

  if (RegQueryInfoKey (hSrcKey,
		       NULL,
		       NULL,
		       NULL,
		       &dwSubKeys,
		       &dwMaxSubKeyNameLength,
		       NULL,
		       &dwValues,
		       &dwMaxValueNameLength,
		       &dwMaxValueLength,
		       NULL,
		       NULL))
    {
      DPRINT1("Error: %lu\n", GetLastError ());
      return FALSE;
    }

  DPRINT ("dwSubKeys %lu\n", dwSubKeys);
  DPRINT ("dwMaxSubKeyNameLength %lu\n", dwMaxSubKeyNameLength);
  DPRINT ("dwValues %lu\n", dwValues);
  DPRINT ("dwMaxValueNameLength %lu\n", dwMaxValueNameLength);
  DPRINT ("dwMaxValueLength %lu\n", dwMaxValueLength);

  /* Copy subkeys */
  if (dwSubKeys != 0)
    {
      lpNameBuffer = HeapAlloc (GetProcessHeap (),
				0,
				dwMaxSubKeyNameLength * sizeof(WCHAR));
      if (lpNameBuffer == NULL)
	{
	  DPRINT1("Buffer allocation failed\n");
	  return FALSE;
	}

      for (i = 0; i < dwSubKeys; i++)
	{
	  dwSubKeyNameLength = dwMaxSubKeyNameLength;
	  if (RegEnumKeyExW (hSrcKey,
			     i,
			     lpNameBuffer,
			     &dwSubKeyNameLength,
			     NULL,
			     NULL,
			     NULL,
			     &LastWrite))
	    {
	      DPRINT1("Subkey enumeration failed (Error %lu)\n", GetLastError());
	      HeapFree (GetProcessHeap (),
			0,
			lpNameBuffer);
	      return FALSE;
	    }

	  if (RegCreateKeyExW (hDstKey,
			       lpNameBuffer,
			       0,
			       NULL,
			       REG_OPTION_NON_VOLATILE,
			       KEY_WRITE,
			       NULL,
			       &hDstSubKey,
			       &dwDisposition))
	    {
	      DPRINT1("Subkey creation failed (Error %lu)\n", GetLastError());
	      HeapFree (GetProcessHeap (),
			0,
			lpNameBuffer);
	      return FALSE;
	    }

	  if (RegOpenKeyExW (hSrcKey,
			     lpNameBuffer,
			     0,
			     KEY_READ,
			     &hSrcSubKey))
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      RegCloseKey (hDstSubKey);
	      HeapFree (GetProcessHeap (),
			0,
			lpNameBuffer);
	      return FALSE;
	    }

	  if (!CopyKey (hDstSubKey,
			hSrcSubKey))
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      RegCloseKey (hSrcSubKey);
	      RegCloseKey (hDstSubKey);
	      HeapFree (GetProcessHeap (),
			0,
			lpNameBuffer);
	      return FALSE;
	    }

	  RegCloseKey (hSrcSubKey);
	  RegCloseKey (hDstSubKey);
	}

       HeapFree (GetProcessHeap (),
		 0,
		 lpNameBuffer);
    }

  /* Copy values */
  if (dwValues != 0)
    {
      lpNameBuffer = HeapAlloc (GetProcessHeap (),
				0,
				dwMaxValueNameLength * sizeof(WCHAR));
      if (lpNameBuffer == NULL)
	{
	  DPRINT1("Buffer allocation failed\n");
	  return FALSE;
	}

      lpDataBuffer = HeapAlloc (GetProcessHeap (),
				0,
				dwMaxValueLength);
      if (lpDataBuffer == NULL)
	{
	  DPRINT1("Buffer allocation failed\n");
	  HeapFree (GetProcessHeap (),
		    0,
		    lpNameBuffer);
	  return FALSE;
	}

      for (i = 0; i < dwValues; i++)
	{
	  dwValueNameLength = dwMaxValueNameLength;
	  dwValueLength = dwMaxValueLength;
	  if (RegEnumValueW (hSrcKey,
			     i,
			     lpNameBuffer,
			     &dwValueNameLength,
			     NULL,
			     &dwType,
			     lpDataBuffer,
			     &dwValueLength))
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      HeapFree (GetProcessHeap (),
			0,
			lpDataBuffer);
	      HeapFree (GetProcessHeap (),
			0,
			lpNameBuffer);
	      return FALSE;
	    }

	  if (RegSetValueEx (hDstKey,
			     lpNameBuffer,
			     0,
			     dwType,
			     lpDataBuffer,
			     dwValueLength))
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      HeapFree (GetProcessHeap (),
			0,
			lpDataBuffer);
	      HeapFree (GetProcessHeap (),
			0,
			lpNameBuffer);
	      return FALSE;
	    }
	}

      HeapFree (GetProcessHeap (),
		0,
		lpDataBuffer);

      HeapFree (GetProcessHeap (),
		0,
		lpNameBuffer);
    }

  DPRINT ("CopyKey() done \n");

  return TRUE;
}


BOOL
CreateUserHive (LPCWSTR lpKeyName)
{
  HKEY hDefaultKey;
  HKEY hUserKey;
  BOOL bResult;

  DPRINT ("CreateUserHive(%S) called\n", lpKeyName);


  if (RegOpenKeyExW (HKEY_USERS,
		     L".Default",
		     0,
		     KEY_READ,
		     &hDefaultKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  if (RegOpenKeyExW (HKEY_USERS,
		     lpKeyName,
		     0,
		     KEY_ALL_ACCESS,
		     &hUserKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hDefaultKey);
      return FALSE;
    }

  bResult = CopyKey (hUserKey,
		     hDefaultKey);

  RegCloseKey (hUserKey);
  RegCloseKey (hDefaultKey);

  DPRINT ("CreateUserHive() done\n");

  return bResult;
}

/* EOF */
