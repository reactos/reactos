/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
InitClassImpl(VOID)
{
   return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupClassImpl(VOID)
{
   return(STATUS_SUCCESS);
}


__inline VOID FASTCALL 
ClassDerefObject(PWNDCLASS_OBJECT Class)
{
   ASSERT(Class->refs >= 1);
   Class->refs--;   
}


__inline VOID FASTCALL 
ClassRefObject(PWNDCLASS_OBJECT Class)
{
   ASSERT(Class->refs >= 0);
   Class->refs++;
}


VOID FASTCALL DestroyClass(PWNDCLASS_OBJECT Class)
{
#if defined(DBG) || defined(KDBG)
   if ( Class->refs != 0 )
   {
      WCHAR AtomName[256];
      ULONG AtomNameLen = sizeof(AtomName);
      RtlQueryAtomInAtomTable ( gAtomTable, Class->Atom,
         NULL, NULL, AtomName, &AtomNameLen );
      DPRINT1("DestroyClass(): can't delete class = '%ws', b/c refs = %lu\n", AtomName, Class->refs );
   }
#endif
   ASSERT(Class->refs == 0);
   
   RemoveEntryList(&Class->ListEntry);
   if (Class->hMenu)
      UserDestroyMenu(Class->hMenu);
   RtlDeleteAtomFromAtomTable(gAtomTable, Class->Atom);
   ExFreePool(Class);
}


/* clean all process classes. all process windows must cleaned first!! */
void FASTCALL DestroyProcessClasses(PW32PROCESS Process )
{
   PWNDCLASS_OBJECT Class;

   while (!IsListEmpty(&Process->ClassList))
   {
      Class = CONTAINING_RECORD(RemoveHeadList(&Process->ClassList), WNDCLASS_OBJECT, ListEntry);
      DestroyClass(Class);
   }
}




PWNDCLASS_OBJECT FASTCALL
ClassGetClassByAtom(RTL_ATOM Atom, HINSTANCE hInstance)
{
   PWNDCLASS_OBJECT Class;
   PW32PROCESS Process = PsGetCurrentProcessWin32Process();

   LIST_FOR_EACH(Class, &Process->ClassList, WNDCLASS_OBJECT, ListEntry)
   {
      if (Class->Atom != Atom) continue;

      if (!hInstance || Class->Global || Class->hInstance == hInstance) return Class;
   }
   
   return NULL;
}


PWNDCLASS_OBJECT FASTCALL
ClassGetClassByName(LPCWSTR ClassName, HINSTANCE hInstance)
{
   NTSTATUS Status;
   RTL_ATOM Atom;

   if (!ClassName || !PsGetCurrentThreadWin32Thread()->Desktop)
      return FALSE;

   Status = RtlLookupAtomInAtomTable(
               gAtomTable,
               (LPWSTR)ClassName,
               &Atom);

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Failed to lookup class atom (ClassName '%S')!\n", ClassName);
      return FALSE;
   }

   return ClassGetClassByAtom(Atom, hInstance);
}


PWNDCLASS_OBJECT FASTCALL
ClassGetClassByNameOrAtom(LPCWSTR ClassNameOrAtom, HINSTANCE hInstance)
{
   if (!ClassNameOrAtom) return NULL;
   
   if (IS_ATOM(ClassNameOrAtom))
      return ClassGetClassByAtom((RTL_ATOM)((ULONG_PTR)ClassNameOrAtom), hInstance);
   else
      return ClassGetClassByName(ClassNameOrAtom, hInstance);
}


static
BOOL FASTCALL
IntRegisterClass(
   CONST WNDCLASSEXW *lpwcx,
   DWORD Flags,
   WNDPROC wpExtra,
   PUNICODE_STRING MenuName,
   RTL_ATOM Atom,
   HMENU hMenu)
{
   PWNDCLASS_OBJECT Class;
   ULONG  objectSize;
   BOOL Global;

   ASSERT(lpwcx);
   ASSERT(Atom);
   ASSERT(lpwcx->hInstance);

   Global = (Flags & REGISTERCLASS_SYSTEM) || (lpwcx->style & CS_GLOBALCLASS);

   /* Check for double registration of the class. */
   Class = ClassGetClassByAtom(Atom, lpwcx->hInstance);
   if (Class && Global == Class->Global)
   {
      /* can max have one class of each type (global/local) */
      SetLastWin32Error(ERROR_CLASS_ALREADY_EXISTS);
      return(FALSE);
   }

   objectSize = sizeof(WNDCLASS_OBJECT) + lpwcx->cbClsExtra;
   
   //FIXME: allocate in session heap (or possibly desktop heap)
   Class = ExAllocatePool(PagedPool, objectSize);
   if (!Class)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return(FALSE);
   }
   RtlZeroMemory(Class, objectSize);

   Class->cbSize = lpwcx->cbSize;
   Class->style = lpwcx->style;
   Class->cbClsExtra = lpwcx->cbClsExtra;
   Class->cbWndExtra = lpwcx->cbWndExtra;
   Class->hInstance = lpwcx->hInstance;
   Class->hIcon = lpwcx->hIcon;
   Class->hCursor = lpwcx->hCursor;
   Class->hMenu = hMenu;
   Class->hbrBackground = lpwcx->hbrBackground;
   Class->Unicode = !(Flags & REGISTERCLASS_ANSI);
   Class->Global = Global;
   Class->hIconSm = lpwcx->hIconSm;
   Class->Atom = Atom;
   
   if (MenuName->Length == 0)
   {
      Class->lpszMenuName.Length =
         Class->lpszMenuName.MaximumLength = 0;
      Class->lpszMenuName.Buffer = MenuName->Buffer;
   }
   else
   {
      Class->lpszMenuName.Length =
         Class->lpszMenuName.MaximumLength = MenuName->MaximumLength;
      Class->lpszMenuName.Buffer = ExAllocatePoolWithTag(PagedPool, Class->lpszMenuName.MaximumLength, TAG_STRING);
      
      if (Class->lpszMenuName.Buffer == NULL) 
      {
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         return(FALSE);
      }
      
      RtlCopyUnicodeString(&Class->lpszMenuName, MenuName);
   }
   
   if (wpExtra == NULL)
   {
      if (Flags & REGISTERCLASS_ANSI)
      {
         Class->lpfnWndProcA = lpwcx->lpfnWndProc;
         Class->lpfnWndProcW = (WNDPROC)IntAddWndProcHandle(lpwcx->lpfnWndProc,FALSE);
      }
      else
      {
         Class->lpfnWndProcW = lpwcx->lpfnWndProc;
         Class->lpfnWndProcA = (WNDPROC)IntAddWndProcHandle(lpwcx->lpfnWndProc,TRUE);
      }
   }
   else
   {
      if (Flags & REGISTERCLASS_ANSI)
      {
         Class->lpfnWndProcA = lpwcx->lpfnWndProc;
         Class->lpfnWndProcW = wpExtra;
      }
      else
      {
         Class->lpfnWndProcW = lpwcx->lpfnWndProc;
         Class->lpfnWndProcA = wpExtra;
      }
   }
   
  
   
   /* Extra class data */
   if (Class->cbClsExtra)
      Class->ExtraData = (PCHAR)(Class + 1);

   if (Global)
   {
      /* global classes go last (incl. system classes) */
      InsertTailList(&PsGetCurrentProcessWin32Process()->ClassList, &Class->ListEntry);
   }
   else
   {
      /* local classes have priority so we put them first */
      InsertHeadList(&PsGetCurrentProcessWin32Process()->ClassList, &Class->ListEntry);
   }
   
   return TRUE;
}


ULONG FASTCALL
IntGetClassLong(PWINDOW_OBJECT Window, ULONG Offset, BOOL Ansi)
{
   LONG Ret;

   if ((int)Offset >= 0)
   {
      DPRINT("GetClassLong(%x, %d)\n", Window->hSelf, Offset);
      if ((Offset + sizeof(LONG)) > Window->Class->cbClsExtra)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return 0;
      }
      Ret = *((LONG *)(Window->Class->ExtraData + Offset));
      DPRINT("Result: %x\n", Ret);
      return Ret;
   }

   switch (Offset)
   {
      case GCL_CBWNDEXTRA:
         Ret = Window->Class->cbWndExtra;
         break;
      case GCL_CBCLSEXTRA:
         Ret = Window->Class->cbClsExtra;
         break;
      case GCL_HBRBACKGROUND:
         Ret = (ULONG)Window->Class->hbrBackground;
         break;
      case GCL_HCURSOR:
         Ret = (ULONG)Window->Class->hCursor;
         break;
      case GCL_HICON:
         Ret = (ULONG)Window->Class->hIcon;
         break;
      case GCL_HICONSM:
         Ret = (ULONG)Window->Class->hIconSm;
         break;
      case GCL_HMODULE:
         Ret = (ULONG)Window->Class->hInstance;
         break;
      case GCL_MENUNAME:
         Ret = (ULONG)Window->Class->lpszMenuName.Buffer;
         break;
      case GCL_STYLE:
         Ret = Window->Class->style;
         break;
      case GCL_WNDPROC:
         if (Ansi)
         {
            Ret = (ULONG)Window->Class->lpfnWndProcA;
         }
         else
         {
            Ret = (ULONG)Window->Class->lpfnWndProcW;
         }
         break;
      case GCW_ATOM:
         Ret = Window->Class->Atom;
         break;
      default:
         Ret = 0;
         break;
   }
   return(Ret);
}

static
void FASTCALL
co_IntSetClassLong(PWINDOW_OBJECT Window, ULONG Offset, LONG dwNewLong, BOOL Ansi)
{
   ASSERT_REFS_CO(Window);

   if ((int)Offset >= 0)
   {
      DPRINT("SetClassLong(%x, %d, %x)\n", Window->hSelf, Offset, dwNewLong);
      if ((Offset + sizeof(LONG)) > Window->Class->cbClsExtra)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return;
      }
      *((LONG *)(Window->Class->ExtraData + Offset)) = dwNewLong;
      return;
   }

   switch (Offset)
   {
      case GCL_CBWNDEXTRA:
         Window->Class->cbWndExtra = dwNewLong;
         break;
      case GCL_CBCLSEXTRA:
         Window->Class->cbClsExtra = dwNewLong;
         break;
      case GCL_HBRBACKGROUND:
         Window->Class->hbrBackground = (HBRUSH)dwNewLong;
         break;
      case GCL_HCURSOR:
         Window->Class->hCursor = (HCURSOR)dwNewLong;
         break;
      case GCL_HICON:
         Window->Class->hIcon = (HICON)dwNewLong;

         if (!IntGetOwner(Window) && !IntGetParent(Window))
         {
            co_IntShellHookNotify(HSHELL_REDRAW, (LPARAM) Window->hSelf);
         }
         break;
      case GCL_HICONSM:
         Window->Class->hIconSm = (HICON)dwNewLong;
         break;
      case GCL_HMODULE:
         Window->Class->hInstance = (HINSTANCE)dwNewLong;
         break;
      case GCL_MENUNAME:
         if (Window->Class->lpszMenuName.MaximumLength)
            RtlFreeUnicodeString(&Window->Class->lpszMenuName);
         if (!IS_INTRESOURCE(dwNewLong))
         {
            Window->Class->lpszMenuName.Length =
               Window->Class->lpszMenuName.MaximumLength = ((PUNICODE_STRING)dwNewLong)->MaximumLength;
            Window->Class->lpszMenuName.Buffer = ExAllocatePoolWithTag(PagedPool, Window->Class->lpszMenuName.MaximumLength, TAG_STRING);
            RtlCopyUnicodeString(&Window->Class->lpszMenuName, (PUNICODE_STRING)dwNewLong);
         }
         else
         {
            Window->Class->lpszMenuName.Length =
               Window->Class->lpszMenuName.MaximumLength = 0;
            Window->Class->lpszMenuName.Buffer = (LPWSTR)dwNewLong;
         }
         break;
      case GCL_STYLE:
         Window->Class->style = dwNewLong;
         break;
      case GCL_WNDPROC:
         if (Ansi)
         {
            Window->Class->lpfnWndProcA = (WNDPROC)dwNewLong;
            Window->Class->lpfnWndProcW = (WNDPROC) IntAddWndProcHandle((WNDPROC)dwNewLong,FALSE);
            Window->Class->Unicode = FALSE;
         }
         else
         {
            Window->Class->lpfnWndProcW = (WNDPROC)dwNewLong;
            Window->Class->lpfnWndProcA = (WNDPROC) IntAddWndProcHandle((WNDPROC)dwNewLong,TRUE);
            Window->Class->Unicode = TRUE;
         }
         break;
   }
}
/* SYSCALLS *****************************************************************/


RTL_ATOM STDCALL
NtUserRegisterClassExWOW(
   CONST WNDCLASSEXW* lpwcx,
   PUNICODE_STRING ClassName,
   PUNICODE_STRING ClassNameCopy,//huhuhuhu???
   PUNICODE_STRING MenuName,
   WNDPROC wpExtra,
   DWORD Flags,
   DWORD Unknown7,
   HMENU hMenu)

/*
 * FUNCTION:
 *   Registers a new class with the window manager
 * ARGUMENTS:
 *   lpwcx          = Win32 extended window class structure
 *   bUnicodeClass = Whether to send ANSI or unicode strings
 *                   to window procedures
 *   wpExtra       = Extra window procedure, if this is not null, its used for the second window procedure for standard controls.
 * RETURNS:
 *   Atom identifying the new class
 */
{
   WNDCLASSEXW SafeClass;
   NTSTATUS Status;
   RTL_ATOM Atom;
   DECLARE_RETURN(RTL_ATOM);

   DPRINT("Enter NtUserRegisterClassExWOW\n");
   UserEnterExclusive();

   if (!lpwcx)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( (RTL_ATOM)0);
   }

   if (Flags & ~REGISTERCLASS_ALL)
   {
      SetLastWin32Error(ERROR_INVALID_FLAGS);
      RETURN( (RTL_ATOM)0);
   }

   Status = MmCopyFromCaller(&SafeClass, lpwcx, sizeof(WNDCLASSEXW));
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( (RTL_ATOM)0);
   }

   /* Deny negative sizes */
   if (lpwcx->cbClsExtra < 0 || lpwcx->cbWndExtra < 0)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( (RTL_ATOM)0);
   }

   if (!lpwcx->hInstance)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( (RTL_ATOM)0);
   }

   //FIXME: make ClassName ptr the atom, not buffer
   if (ClassName->Length > 0)
   {
      DPRINT("NtUserRegisterClassExWOW(%S)\n", ClassName->Buffer);
      /* FIXME - Safely copy/verify the buffer first!!! */
      Status = RtlAddAtomToAtomTable(gAtomTable,
                                     ClassName->Buffer,
                                     &Atom);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Failed adding class name (%S) to atom table\n",
                 ClassName->Buffer);
         SetLastNtError(Status);
         RETURN((RTL_ATOM)0);
      }
   }
   else
   {
      Atom = (RTL_ATOM)(ULONG)ClassName->Buffer;
   }

   if (!Atom)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(0);
   }

   if (!IntRegisterClass(&SafeClass, Flags, wpExtra, MenuName, Atom, hMenu))
   {
      if (ClassName->Length)
      {
         RtlDeleteAtomFromAtomTable(gAtomTable, Atom);
      }
      DPRINT("Failed creating window class object\n");
      RETURN((RTL_ATOM)0);
   }

   RETURN(Atom);

CLEANUP:
   DPRINT("Leave NtUserRegisterClassExWOW, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



DWORD STDCALL
NtUserGetClassLong(HWND hWnd, DWORD Offset, BOOL Ansi)
{
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetClassLong\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(0);
   }

   RETURN(IntGetClassLong(Window, Offset, Ansi));

CLEANUP:
   DPRINT("Leave NtUserGetClassLong, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



DWORD STDCALL
NtUserSetClassLong(HWND hWnd,
                   DWORD Offset,
                   LONG dwNewLong,
                   BOOL Ansi)
{
   PWINDOW_OBJECT Window;
   LONG Ret;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserSetClassLong\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(0);
   }

   UserRefObjectCo(Window, &Ref);

   Ret = IntGetClassLong(Window, Offset, Ansi);
   co_IntSetClassLong(Window, Offset, dwNewLong, Ansi);

   UserDerefObjectCo(Window);

   RETURN(Ret);

CLEANUP:
   DPRINT("Leave NtUserSetClassLong, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD STDCALL
NtUserSetClassWord(DWORD Unknown0,
                   DWORD Unknown1,
                   DWORD Unknown2)
{
   UNIMPLEMENTED;
   return(0);
}

BOOL STDCALL
NtUserUnregisterClass(
   LPCWSTR ClassNameOrAtom,
   HINSTANCE hInstance, /* can be 0 */
   DWORD Unknown)
{
   PWNDCLASS_OBJECT Class;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserUnregisterClass(%S)\n", ClassNameOrAtom);
   UserEnterExclusive();

   if (!ClassNameOrAtom)
   {
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      RETURN(FALSE);
   }

   if (!(Class = ClassGetClassByNameOrAtom(ClassNameOrAtom, hInstance)))
   {
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      RETURN(FALSE);
   }

   if (Class->refs)
   {
      /* NOTE: the class will not be freed when its refs become 0 ie. no more
       * windows are using it. I dunno why that is but its how Windows does it (and Wine).
       * The class will hang around until the process exit. -Gunnar
       */
      SetLastWin32Error(ERROR_CLASS_HAS_WINDOWS);
      RETURN(FALSE);
   }

   DestroyClass(Class);
   
   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserUnregisterClass, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* NOTE: for system classes hInstance is not NULL here, but User32Instance */
DWORD STDCALL
NtUserGetClassInfo(
   HINSTANCE hInstance,
   LPCWSTR lpClassName,
   LPWNDCLASSEXW lpWndClassEx,
   BOOL Ansi,
   DWORD unknown3)
{
   PWNDCLASS_OBJECT Class;
   RTL_ATOM Atom;
   DECLARE_RETURN(DWORD);

   if (IS_ATOM(lpClassName))
      DPRINT("NtUserGetClassInfo - %x (%lx)\n", lpClassName, hInstance);
   else
      DPRINT("NtUserGetClassInfo - %S (%lx)\n", lpClassName, hInstance);

   UserEnterExclusive();

   if (!hInstance)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(0);
   }

   if (!(Class = ClassGetClassByNameOrAtom(lpClassName, hInstance)))
   {
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      RETURN(0);
   }

   lpWndClassEx->cbSize = sizeof(WNDCLASSEXW);
   lpWndClassEx->style = Class->style;
   if (Ansi)
      lpWndClassEx->lpfnWndProc = Class->lpfnWndProcA;
   else
      lpWndClassEx->lpfnWndProc = Class->lpfnWndProcW;
   lpWndClassEx->cbClsExtra = Class->cbClsExtra;
   lpWndClassEx->cbWndExtra = Class->cbWndExtra;
   /* This is not typo, we're really not going to use Class->hInstance here. */
   /* Well, i think its wrong so i changed it -Gunnar */
   lpWndClassEx->hInstance = Class->hInstance;
   lpWndClassEx->hIcon = Class->hIcon;
   lpWndClassEx->hCursor = Class->hCursor;
   lpWndClassEx->hbrBackground = Class->hbrBackground;
   if (Class->lpszMenuName.MaximumLength)
      RtlCopyUnicodeString((PUNICODE_STRING)lpWndClassEx->lpszMenuName, &Class->lpszMenuName);
   else
      lpWndClassEx->lpszMenuName = Class->lpszMenuName.Buffer;
   lpWndClassEx->lpszClassName = lpClassName;
   lpWndClassEx->hIconSm = Class->hIconSm;
   Atom = Class->Atom;

   RETURN(Atom);

CLEANUP:
   DPRINT("Leave NtUserGetClassInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



DWORD STDCALL
NtUserGetClassName (
   HWND hWnd,
   LPWSTR lpClassName,
   ULONG nMaxCount /* in TCHARS */
   )
{
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(DWORD);
   NTSTATUS Status;

   UserEnterShared();
   DPRINT("Enter NtUserGetClassName\n");   

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(0);
   }

   nMaxCount *= sizeof(WCHAR);
   
   //FIXME: wrap in SEH to protect lpClassName access
   Status = RtlQueryAtomInAtomTable(gAtomTable,
                                    Window->Class->Atom, NULL, NULL,
                                    lpClassName, &nMaxCount);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(0);
   }

   RETURN(nMaxCount / sizeof(WCHAR));

CLEANUP:
   DPRINT("Leave NtUserGetClassName, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD STDCALL
NtUserGetWOWClass(DWORD Unknown0,
                  DWORD Unknown1)
{
   UNIMPLEMENTED;
   return(0);
}


/* EOF */
