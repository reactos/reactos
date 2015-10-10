/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS user32.dll
 * FILE:            user/user32/windows/prop.c
 * PURPOSE:         Window Property
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#define ATOM_BUFFER_SIZE 256

/* INTERNAL FUNCTIONS ********************************************************/

HANDLE
FASTCALL
IntGetProp(HWND hWnd, ATOM Atom, BOOLEAN SystemProp)
{
  PLIST_ENTRY ListEntry, temp;
  PPROPERTY Property;
  PWND pWnd;
  int i;
  WORD SystemFlag = SystemProp ? PROPERTY_FLAG_SYSTEM : 0;

  pWnd = ValidateHwnd(hWnd);
  if (!pWnd) return NULL;

  ListEntry = SharedPtrToUser(pWnd->PropListHead.Flink);
  for (i = 0; i < pWnd->PropListItems; i++ )
  {
      Property = CONTAINING_RECORD(ListEntry, PROPERTY, PropListEntry);
      if (Property->Atom == Atom &&
          (Property->fs & PROPERTY_FLAG_SYSTEM) == SystemFlag)
      {
         return(Property);
      }
      temp = ListEntry->Flink;
      ListEntry = SharedPtrToUser(temp);
  }
  return NULL;
}

HANDLE
FASTCALL
UserGetProp(HWND hWnd, ATOM Atom, BOOLEAN SystemProp)
{
  PPROPERTY Prop;
  Prop = IntGetProp(hWnd, Atom, SystemProp);
  return Prop ? Prop->Data : NULL;
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
int WINAPI
EnumPropsA(HWND hWnd, PROPENUMPROCA lpEnumFunc)
{
  PPROPLISTITEM pli, i;
  NTSTATUS Status;
  DWORD Count;
  int ret = -1;

  if(!lpEnumFunc)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return ret;
  }

  Status = NtUserBuildPropList(hWnd, NULL, 0, &Count);
  if(!NT_SUCCESS(Status))
  {
    if(Status == STATUS_INVALID_HANDLE)
      SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    else
      SetLastError(RtlNtStatusToDosError(Status));
    return ret;
  }

  if(Count > 0)
  {
    pli = RtlAllocateHeap(GetProcessHeap(), 0, Count);
    if (pli == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return -1;
    }

    Status = NtUserBuildPropList(hWnd, (LPVOID)pli, Count, &Count);
    if(!NT_SUCCESS(Status))
    {
      RtlFreeHeap(GetProcessHeap(), 0, pli);
      if(Status == STATUS_INVALID_HANDLE)
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
      else
        SetLastError(RtlNtStatusToDosError(Status));
      return ret;
    }

    i = pli;
    for(; Count > 0; Count--, i++)
    {
      char str[ATOM_BUFFER_SIZE];

      if(!GlobalGetAtomNameA(i->Atom, str, ATOM_BUFFER_SIZE))
        continue;

      ret = lpEnumFunc(hWnd, str, i->Data);
      if(!ret)
        break;
    }

    RtlFreeHeap(GetProcessHeap(), 0, pli);
  }

  return ret;
}


/*
 * @implemented
 */
int WINAPI
EnumPropsExA(HWND hWnd, PROPENUMPROCEXA lpEnumFunc, LPARAM lParam)
{
  PPROPLISTITEM pli, i;
  NTSTATUS Status;
  DWORD Count;
  int ret = -1;

  if(!lpEnumFunc)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return ret;
  }

  Status = NtUserBuildPropList(hWnd, NULL, 0, &Count);
  if(!NT_SUCCESS(Status))
  {
    if(Status == STATUS_INVALID_HANDLE)
      SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    else
      SetLastError(RtlNtStatusToDosError(Status));
    return ret;
  }

  if(Count > 0)
  {
    pli = RtlAllocateHeap(GetProcessHeap(), 0, Count);
    if (pli == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return -1;
    }

    Status = NtUserBuildPropList(hWnd, (LPVOID)pli, Count, &Count);
    if(!NT_SUCCESS(Status))
    {
      RtlFreeHeap(GetProcessHeap(), 0, pli);
      if(Status == STATUS_INVALID_HANDLE)
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
      else
        SetLastError(RtlNtStatusToDosError(Status));
      return ret;
    }

    i = pli;
    for(; Count > 0; Count--, i++)
    {
      char str[ATOM_BUFFER_SIZE];

      if(!GlobalGetAtomNameA(i->Atom, str, ATOM_BUFFER_SIZE))
        continue;

      ret = lpEnumFunc(hWnd, str, i->Data, lParam);
      if(!ret)
        break;
    }

    RtlFreeHeap(GetProcessHeap(), 0, pli);
  }

  return ret;
}


/*
 * @implemented
 */
int WINAPI
EnumPropsExW(HWND hWnd, PROPENUMPROCEXW lpEnumFunc, LPARAM lParam)
{
  PPROPLISTITEM pli, i;
  NTSTATUS Status;
  DWORD Count;
  int ret = -1;

  if(!lpEnumFunc)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return ret;
  }

  Status = NtUserBuildPropList(hWnd, NULL, 0, &Count);
  if(!NT_SUCCESS(Status))
  {
    if(Status == STATUS_INVALID_HANDLE)
      SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    else
      SetLastError(RtlNtStatusToDosError(Status));
    return ret;
  }

  if(Count > 0)
  {
    pli = RtlAllocateHeap(GetProcessHeap(), 0, Count);
    if (pli == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return -1;
    }

    Status = NtUserBuildPropList(hWnd, (LPVOID)pli, Count, &Count);
    if(!NT_SUCCESS(Status))
    {
      RtlFreeHeap(GetProcessHeap(), 0, pli);
      if(Status == STATUS_INVALID_HANDLE)
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
      else
        SetLastError(RtlNtStatusToDosError(Status));
      return ret;
    }

    i = pli;
    for(; Count > 0; Count--, i++)
    {
      WCHAR str[ATOM_BUFFER_SIZE];

      if(!GlobalGetAtomNameW(i->Atom, str, ATOM_BUFFER_SIZE))
        continue;

      ret = lpEnumFunc(hWnd, str, i->Data, lParam);
      if(!ret)
        break;
    }

    RtlFreeHeap(GetProcessHeap(), 0, pli);
  }

  return ret;
}


/*
 * @implemented
 */
int WINAPI
EnumPropsW(HWND hWnd, PROPENUMPROCW lpEnumFunc)
{
  PPROPLISTITEM pli, i;
  NTSTATUS Status;
  DWORD Count;
  int ret = -1;

  if(!lpEnumFunc)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return ret;
  }

  Status = NtUserBuildPropList(hWnd, NULL, 0, &Count);
  if(!NT_SUCCESS(Status))
  {
    if(Status == STATUS_INVALID_HANDLE)
      SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    else
      SetLastError(RtlNtStatusToDosError(Status));
    return ret;
  }

  if(Count > 0)
  {
    pli = RtlAllocateHeap(GetProcessHeap(), 0, Count);
    if (pli == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return -1;
    }

    Status = NtUserBuildPropList(hWnd, (LPVOID)pli, Count, &Count);
    if(!NT_SUCCESS(Status))
    {
      RtlFreeHeap(GetProcessHeap(), 0, pli);
      if(Status == STATUS_INVALID_HANDLE)
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
      else
        SetLastError(RtlNtStatusToDosError(Status));
      return ret;
    }

    i = pli;
    for(; Count > 0; Count--, i++)
    {
      WCHAR str[ATOM_BUFFER_SIZE];

      if(!GlobalGetAtomNameW(i->Atom, str, ATOM_BUFFER_SIZE))
        continue;

      ret = lpEnumFunc(hWnd, str, i->Data);
      if(!ret)
        break;
    }

    RtlFreeHeap(GetProcessHeap(), 0, pli);
  }

  return ret;
}


/*
 * @implemented
 */
HANDLE WINAPI
GetPropA(HWND hWnd, LPCSTR lpString)
{
  PWSTR lpWString;
  UNICODE_STRING UString;
  HANDLE Ret;
  if (HIWORD(lpString))
    {
      RtlCreateUnicodeStringFromAsciiz(&UString, (LPSTR)lpString);
      lpWString = UString.Buffer;
      if (lpWString == NULL)
	{
	  return(FALSE);
	}
      Ret = GetPropW(hWnd, lpWString);
      RtlFreeUnicodeString(&UString);
    }
  else
    {
      Ret = GetPropW(hWnd, (LPWSTR)lpString);
    }
  return(Ret);
}


/*
 * @implemented
 */
HANDLE WINAPI
GetPropW(HWND hWnd, LPCWSTR lpString)
{
  ATOM Atom;
  HANDLE Data = NULL;
  PPROPERTY Prop;
  if (HIWORD(lpString))
  {
     Atom = GlobalFindAtomW(lpString);
  }
  else
  {
     Atom = LOWORD((DWORD_PTR)lpString);
  }
  Prop = IntGetProp(hWnd, Atom, FALSE);
  if (Prop != NULL) Data = Prop->Data;
  return Data;
}


/*
 * @implemented
 */
HANDLE WINAPI
RemovePropA(HWND hWnd, LPCSTR lpString)
{
  PWSTR lpWString;
  UNICODE_STRING UString;
  HANDLE Ret;

  if (HIWORD(lpString))
    {
      RtlCreateUnicodeStringFromAsciiz(&UString, (LPSTR)lpString);
      lpWString = UString.Buffer;
      if (lpWString == NULL)
	{
	  return(FALSE);
	}
      Ret = RemovePropW(hWnd, lpWString);
      RtlFreeUnicodeString(&UString);
    }
  else
    {
      Ret = RemovePropW(hWnd, (LPCWSTR)lpString);
    }
  return(Ret);
}


/*
 * @implemented
 */
HANDLE WINAPI
RemovePropW(HWND hWnd,
	    LPCWSTR lpString)
{
  ATOM Atom;
  if (HIWORD(lpString))
    {
      Atom = GlobalFindAtomW(lpString);
    }
  else
    {
      Atom = LOWORD((DWORD_PTR)lpString);
    }
  return(NtUserRemoveProp(hWnd, Atom));
}


/*
 * @implemented
 */
BOOL WINAPI
SetPropA(HWND hWnd, LPCSTR lpString, HANDLE hData)
{
  PWSTR lpWString;
  UNICODE_STRING UString;
  BOOL Ret;

  if (HIWORD(lpString))
    {
      RtlCreateUnicodeStringFromAsciiz(&UString, (LPSTR)lpString);
      lpWString = UString.Buffer;
      if (lpWString == NULL)
	{
	  return(FALSE);
	}
      Ret = SetPropW(hWnd, lpWString, hData);
      RtlFreeUnicodeString(&UString);
    }
  else
    {
      Ret = SetPropW(hWnd, (LPWSTR)lpString, hData);
    }
  return(Ret);
}


/*
 * @implemented
 */
BOOL WINAPI
SetPropW(HWND hWnd, LPCWSTR lpString, HANDLE hData)
{
  ATOM Atom;
  if (HIWORD(lpString))
    {
      Atom = GlobalAddAtomW(lpString);
    }
  else
    {
      Atom = LOWORD((DWORD_PTR)lpString);
    }

  return(NtUserSetProp(hWnd, Atom, hData));
}
