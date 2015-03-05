/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window properties
 * FILE:             subsys/win32k/ntuser/prop.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserProp);

/* STATIC FUNCTIONS **********************************************************/

PPROPERTY FASTCALL
IntGetProp(PWND Window, ATOM Atom)
{
   PLIST_ENTRY ListEntry;
   PPROPERTY Property;
   UINT i;

   ListEntry = Window->PropListHead.Flink;

   for (i = 0; i < Window->PropListItems; i++ )
   {
      Property = CONTAINING_RECORD(ListEntry, PROPERTY, PropListEntry);

      if (ListEntry == NULL)
      {
          ERR("Corrupted (or uninitialized?) property list for window %p. Prop count %d. Atom %d.\n",
              Window, Window->PropListItems, Atom);
          return NULL;
      }

      if (Property->Atom == Atom)
      {
         return(Property);
      }
      ListEntry = ListEntry->Flink;
   }
   return(NULL);
}

HANDLE
FASTCALL
UserGetProp(PWND pWnd, ATOM Atom)
{
  PPROPERTY Prop;
  Prop = IntGetProp(pWnd, Atom);
  return Prop ? Prop->Data : NULL;
}

BOOL FASTCALL
IntRemoveProp(PWND Window, ATOM Atom)
{
   PPROPERTY Prop;
   Prop = IntGetProp(Window, Atom);

   if (Prop == NULL)
   {
      return FALSE;
   }
   RemoveEntryList(&Prop->PropListEntry);
   UserHeapFree(Prop);
   Window->PropListItems--;
   return TRUE;
}

BOOL FASTCALL
IntSetProp(PWND pWnd, ATOM Atom, HANDLE Data)
{
   PPROPERTY Prop;

   Prop = IntGetProp(pWnd, Atom);

   if (Prop == NULL)
   {
      Prop = UserHeapAlloc(sizeof(PROPERTY));
      if (Prop == NULL)
      {
         return FALSE;
      }
      Prop->Atom = Atom;
      InsertTailList(&pWnd->PropListHead, &Prop->PropListEntry);
      pWnd->PropListItems++;
   }

   Prop->Data = Data;
   return TRUE;
}

VOID FASTCALL
IntRemoveWindowProp(PWND Window)
{
   PLIST_ENTRY ListEntry;
   PPROPERTY Property;

   while (!IsListEmpty(&Window->PropListHead))
   {
       ListEntry = Window->PropListHead.Flink;
       Property = CONTAINING_RECORD(ListEntry, PROPERTY, PropListEntry);
       RemoveEntryList(&Property->PropListEntry);
       UserHeapFree(Property);
       Window->PropListItems--;
   }
   return;
}

/* FUNCTIONS *****************************************************************/

NTSTATUS APIENTRY
NtUserBuildPropList(HWND hWnd,
                    LPVOID Buffer,
                    DWORD BufferSize,
                    DWORD *Count)
{
   PWND Window;
   PPROPERTY Property;
   PLIST_ENTRY ListEntry;
   PROPLISTITEM listitem, *li;
   NTSTATUS Status;
   DWORD Cnt = 0;
   DECLARE_RETURN(NTSTATUS);

   TRACE("Enter NtUserBuildPropList\n");
   UserEnterShared();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( STATUS_INVALID_HANDLE);
   }

   if(Buffer)
   {
      if(!BufferSize || (BufferSize % sizeof(PROPLISTITEM) != 0))
      {
         RETURN( STATUS_INVALID_PARAMETER);
      }

      /* Copy list */
      li = (PROPLISTITEM *)Buffer;
      ListEntry = Window->PropListHead.Flink;
      while((BufferSize >= sizeof(PROPLISTITEM)) && (ListEntry != &Window->PropListHead))
      {
         Property = CONTAINING_RECORD(ListEntry, PROPERTY, PropListEntry);
         listitem.Atom = Property->Atom;
         listitem.Data = Property->Data;

         Status = MmCopyToCaller(li, &listitem, sizeof(PROPLISTITEM));
         if(!NT_SUCCESS(Status))
         {
            RETURN( Status);
         }

         BufferSize -= sizeof(PROPLISTITEM);
         Cnt++;
         li++;
         ListEntry = ListEntry->Flink;
      }

   }
   else
   {
      Cnt = Window->PropListItems * sizeof(PROPLISTITEM);
   }

   if(Count)
   {
      Status = MmCopyToCaller(Count, &Cnt, sizeof(DWORD));
      if(!NT_SUCCESS(Status))
      {
         RETURN( Status);
      }
   }

   RETURN( STATUS_SUCCESS);

CLEANUP:
   TRACE("Leave NtUserBuildPropList, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

HANDLE APIENTRY
NtUserRemoveProp(HWND hWnd, ATOM Atom)
{
   PWND Window;
   PPROPERTY Prop;
   HANDLE Data;
   DECLARE_RETURN(HANDLE);

   TRACE("Enter NtUserRemoveProp\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( NULL);
   }

   Prop = IntGetProp(Window, Atom);

   if (Prop == NULL)
   {
      RETURN(NULL);
   }
   Data = Prop->Data;
   RemoveEntryList(&Prop->PropListEntry);
   UserHeapFree(Prop);
   Window->PropListItems--;

   RETURN(Data);

CLEANUP:
   TRACE("Leave NtUserRemoveProp, ret=%p\n", _ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL APIENTRY
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data)
{
   PWND Window;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserSetProp\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   RETURN( IntSetProp(Window, Atom, Data));

CLEANUP:
   TRACE("Leave NtUserSetProp, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
