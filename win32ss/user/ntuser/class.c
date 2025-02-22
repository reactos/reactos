/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Window classes
 * FILE:             win32ss/user/ntuser/class.c
 * PROGRAMER:        Thomas Weidenmueller <w3seek@reactos.com>
 */

#include <win32k.h>
#include <unaligned.h>

DBG_DEFAULT_CHANNEL(UserClass);

static PWSTR ControlsList[] =
{
  L"Button",
  L"Edit",
  L"Static",
  L"ListBox",
  L"ScrollBar",
  L"ComboBox",
  L"MDIClient",
  L"ComboLBox",
  L"DDEMLEvent",
  L"DDEMLMom",
  L"DMGClass",
  L"DDEMLAnsiClient",
  L"DDEMLUnicodeClient",
  L"DDEMLAnsiServer",
  L"DDEMLUnicodeServer",
  L"IME",
  L"Ghost",
};

static NTSTATUS IntDeregisterClassAtom(IN RTL_ATOM Atom);

REGISTER_SYSCLASS DefaultServerClasses[] =
{
  { ((PWSTR)WC_DESKTOP),
    CS_GLOBALCLASS|CS_DBLCLKS,
    NULL, // Use User32 procs
    sizeof(ULONG)*2,
    (HICON)OCR_NORMAL,
    (HBRUSH)(COLOR_BACKGROUND),
    FNID_DESKTOP,
    ICLS_DESKTOP
  },
  { ((PWSTR)WC_SWITCH),
    CS_VREDRAW|CS_HREDRAW|CS_SAVEBITS,
    NULL, // Use User32 procs
    sizeof(LONG_PTR), // See user32_apitest GetClassInfo, 0: Pointer to ALTTABINFO
    (HICON)OCR_NORMAL,
    NULL,
    FNID_SWITCH,
    ICLS_SWITCH
  },
  { ((PWSTR)WC_MENU),
    CS_DBLCLKS|CS_SAVEBITS|CS_DROPSHADOW,
    NULL, // Use User32 procs
    16, // See user32_apitest GetClassInfo, PopupMenuWndProcW
    (HICON)OCR_NORMAL,
    (HBRUSH)(COLOR_MENU + 1),
    FNID_MENU,
    ICLS_MENU
  },
  { L"ScrollBar",
    CS_DBLCLKS|CS_VREDRAW|CS_HREDRAW|CS_PARENTDC,
    NULL, // Use User32 procs
    sizeof(SBWND)-sizeof(WND),
    (HICON)OCR_NORMAL,
    NULL,
    FNID_SCROLLBAR,
    ICLS_SCROLLBAR
  },
#if 0
  { ((PWSTR)((ULONG_PTR)(WORD)(0x8006))), // Tooltips
    CS_PARENTDC|CS_DBLCLKS,
    NULL, // Use User32 procs
    0,
    (HICON)OCR_NORMAL,
    0,
    FNID_TOOLTIPS,
    ICLS_TOOLTIPS
  },
#endif
  { ((PWSTR)WC_ICONTITLE), // IconTitle is here for now...
    0,
    NULL, // Use User32 procs
    0,
    (HICON)OCR_NORMAL,
    0,
    FNID_ICONTITLE,
    ICLS_ICONTITLE
  },
  { L"Message",
    CS_GLOBALCLASS,
    NULL, // Use User32 procs
    0,
    (HICON)OCR_NORMAL,
    NULL,
    FNID_MESSAGEWND,
    ICLS_HWNDMESSAGE
  }
};

static struct
{
    int FnId;
    int ClsId;
}  FnidToiCls[] =
{ /* Function Ids to Class indexes. */
 { FNID_SCROLLBAR,  ICLS_SCROLLBAR},
 { FNID_ICONTITLE,  ICLS_ICONTITLE},
 { FNID_MENU,       ICLS_MENU},
 { FNID_DESKTOP,    ICLS_DESKTOP},
 { FNID_SWITCH,     ICLS_SWITCH},
 { FNID_MESSAGEWND, ICLS_HWNDMESSAGE},
 { FNID_BUTTON,     ICLS_BUTTON},
 { FNID_COMBOBOX,   ICLS_COMBOBOX},
 { FNID_COMBOLBOX,  ICLS_COMBOLBOX},
 { FNID_DIALOG,     ICLS_DIALOG},
 { FNID_EDIT,       ICLS_EDIT},
 { FNID_LISTBOX,    ICLS_LISTBOX},
 { FNID_MDICLIENT,  ICLS_MDICLIENT},
 { FNID_STATIC,     ICLS_STATIC},
 { FNID_IME,        ICLS_IME},
 { FNID_GHOST,      ICLS_GHOST},
 { FNID_TOOLTIPS,   ICLS_TOOLTIPS}
};

BOOL
FASTCALL
LookupFnIdToiCls(int FnId, int *iCls )
{
  int i;

  for ( i = 0; i < ARRAYSIZE(FnidToiCls); i++)
  {
     if (FnidToiCls[i].FnId == FnId)
     {
        if (iCls) *iCls = FnidToiCls[i].ClsId;
        return TRUE;
     }
  }
  if (iCls) *iCls = 0;
  return FALSE;
}

_Must_inspect_result_
NTSTATUS
NTAPI
ProbeAndCaptureUnicodeStringOrAtom(
    _Out_ _When_(return>=0, _At_(pustrOut->Buffer, _Post_ _Notnull_)) PUNICODE_STRING pustrOut,
    __in_data_source(USER_MODE) _In_ PUNICODE_STRING pustrUnsafe)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ustrCopy;

    /* Default to NULL */
    RtlInitEmptyUnicodeString(pustrOut, NULL, 0);

    _SEH2_TRY
    {
        ProbeForRead(pustrUnsafe, sizeof(UNICODE_STRING), 1);

        ustrCopy = *pustrUnsafe;

        /* Validate the string */
        if ((ustrCopy.Length & 1) || (ustrCopy.Buffer == NULL))
        {
            /* This is not legal */
            _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
        }

        /* Check if this is an atom */
        if (IS_ATOM(ustrCopy.Buffer))
        {
            /* Copy the atom, length is 0 */
            pustrOut->MaximumLength = pustrOut->Length = 0;
            pustrOut->Buffer = ustrCopy.Buffer;
        }
        else
        {
            /* Get the length, maximum length includes zero termination */
            pustrOut->Length = ustrCopy.Length;
            pustrOut->MaximumLength = pustrOut->Length + sizeof(WCHAR);

            /* Allocate a buffer */
            pustrOut->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                     pustrOut->MaximumLength,
                                                     TAG_STRING);
            if (!pustrOut->Buffer)
            {
                _SEH2_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
            }

            /* Copy the string and zero terminate it */
            ProbeForRead(ustrCopy.Buffer, pustrOut->Length, 1);
            RtlCopyMemory(pustrOut->Buffer, ustrCopy.Buffer, pustrOut->Length);
            pustrOut->Buffer[pustrOut->Length / sizeof(WCHAR)] = L'\0';
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Check if we already allocated a buffer */
        if (pustrOut->Buffer)
        {
            /* Free the buffer */
            ExFreePoolWithTag(pustrOut->Buffer, TAG_STRING);
            Status = _SEH2_GetExceptionCode();
        }
    }
    _SEH2_END;

    return Status;
}

/* WINDOWCLASS ***************************************************************/

static VOID
IntFreeClassMenuName(IN OUT PCLS Class)
{
    /* Free the menu name, if it was changed and allocated */
    if (Class->lpszClientUnicodeMenuName != NULL && Class->MenuNameIsString)
    {
        UserHeapFree(Class->lpszClientUnicodeMenuName);
        Class->lpszClientUnicodeMenuName = NULL;
        Class->lpszClientAnsiMenuName = NULL;
    }
}

static VOID
IntDestroyClass(IN OUT PCLS Class)
{
    PDESKTOP pDesk;

    /* There shouldn't be any clones anymore */
    ASSERT(Class->cWndReferenceCount == 0);
    ASSERT(Class->pclsClone == NULL);

    if (Class->pclsBase == Class)
    {
        PCALLPROCDATA CallProc, NextCallProc;

        /* Destroy allocated callproc handles */
        CallProc = Class->spcpdFirst;
        while (CallProc != NULL)
        {
            NextCallProc = CallProc->spcpdNext;

            CallProc->spcpdNext = NULL;
            DestroyCallProc(CallProc);

            CallProc = NextCallProc;
        }

        // Fixes running the static test then run class test issue.
        // Some applications do not use UnregisterClass before exiting.
        // Keep from reusing the same atom with case insensitive
        // comparisons, remove registration of the atom if not zeroed.
        if (Class->atomClassName)
            IntDeregisterClassAtom(Class->atomClassName);
        // Dereference non-versioned class name
        if (Class->atomNVClassName)
            IntDeregisterClassAtom(Class->atomNVClassName);

        if (Class->pdce)
        {
           DceFreeClassDCE(Class->pdce);
           Class->pdce = NULL;
        }

        IntFreeClassMenuName(Class);
    }

    if (Class->spicn)
        UserDereferenceObject(Class->spicn);
    if (Class->spcur)
        UserDereferenceObject(Class->spcur);
    if (Class->spicnSm)
    {
        UserDereferenceObject(Class->spicnSm);
        /* Destroy the icon if we own it */
        if ((Class->CSF_flags & CSF_CACHEDSMICON)
                && !(UserObjectInDestroy(UserHMGetHandle(Class->spicnSm))))
            IntDestroyCurIconObject(Class->spicnSm);
    }

    pDesk = Class->rpdeskParent;
    Class->rpdeskParent = NULL;

    /* Free the structure */
    if (pDesk != NULL)
    {
        DesktopHeapFree(pDesk, Class);
    }
    else
    {
        UserHeapFree(Class);
    }
}


/* Clean all process classes. all process windows must cleaned first!! */
void FASTCALL DestroyProcessClasses(PPROCESSINFO Process )
{
    PCLS Class;
    PPROCESSINFO pi = (PPROCESSINFO)Process;

    if (pi != NULL)
    {
        /* Free all local classes */
        Class = pi->pclsPrivateList;
        while (Class != NULL)
        {
            pi->pclsPrivateList = Class->pclsNext;

            ASSERT(Class->pclsBase == Class);
            IntDestroyClass(Class);

            Class = pi->pclsPrivateList;
        }

        /* Free all global classes */
        Class = pi->pclsPublicList;
        while (Class != NULL)
        {
            pi->pclsPublicList = Class->pclsNext;

            ASSERT(Class->pclsBase == Class);
            IntDestroyClass(Class);

            Class = pi->pclsPublicList;
        }
    }
}

static BOOL
IntRegisterClassAtom(IN PUNICODE_STRING ClassName,
                     OUT RTL_ATOM *pAtom)
{
    WCHAR szBuf[65];
    PWSTR AtomName = szBuf;
    NTSTATUS Status;

    if (ClassName->Length != 0)
    {
        if (ClassName->Length + sizeof(UNICODE_NULL) > sizeof(szBuf))
        {
            AtomName = ExAllocatePoolWithTag(PagedPool,
                                             ClassName->Length + sizeof(UNICODE_NULL),
                                             TAG_USTR);

            if (AtomName == NULL)
            {
                EngSetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }
        }

        _SEH2_TRY
        {
            RtlCopyMemory(AtomName,
                          ClassName->Buffer,
                          ClassName->Length);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            if (AtomName != szBuf)
                ExFreePoolWithTag(AtomName, TAG_USTR);
            SetLastNtError(_SEH2_GetExceptionCode());
            _SEH2_YIELD(return FALSE);
        }
        _SEH2_END;
        AtomName[ClassName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        ASSERT(IS_ATOM(ClassName->Buffer));
        AtomName = ClassName->Buffer;
    }

    Status = RtlAddAtomToAtomTable(gAtomTable,
                                   AtomName,
                                   pAtom);

    if (AtomName != ClassName->Buffer && AtomName != szBuf)
        ExFreePoolWithTag(AtomName, TAG_USTR);


    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL FASTCALL
RegisterControlAtoms(VOID)
{
    RTL_ATOM Atom;
    UNICODE_STRING ClassName;
    INT i = 0;

    while ( i < ICLS_DESKTOP)
    {
       RtlInitUnicodeString(&ClassName, ControlsList[i]);
       if (IntRegisterClassAtom(&ClassName, &Atom))
       {
          gpsi->atomSysClass[i] = Atom;
          TRACE("Reg Control Atom %ls: 0x%x\n", ControlsList[i], Atom);
       }
       i++;
    }
    return TRUE;
}

static NTSTATUS
IntDeregisterClassAtom(IN RTL_ATOM Atom)
{
    return RtlDeleteAtomFromAtomTable(gAtomTable,
                                      Atom);
}

VOID
UserAddCallProcToClass(IN OUT PCLS Class,
                       IN PCALLPROCDATA CallProc)
{
    PCLS BaseClass;

    ASSERT(CallProc->spcpdNext == NULL);

    BaseClass = Class->pclsBase;
    ASSERT(CallProc->spcpdNext == NULL);
    CallProc->spcpdNext = BaseClass->spcpdFirst;
    BaseClass->spcpdFirst = CallProc;

    /* Update all clones */
    Class = Class->pclsClone;
    while (Class != NULL)
    {
        Class->spcpdFirst = BaseClass->spcpdFirst;
        Class = Class->pclsNext;
    }
}

static BOOL
IntSetClassAtom(IN OUT PCLS Class,
                IN PUNICODE_STRING ClassName)
{
    RTL_ATOM Atom = (RTL_ATOM)0;

    /* Update the base class first */
    Class = Class->pclsBase;
    if (ClassName->Length > 0)
    {
        if (!IntRegisterClassAtom(ClassName,
                                  &Atom))
        {
            ERR("RegisterClassAtom failed ! %x\n", EngGetLastError());
            return FALSE;
        }
    }
    else
    {
        if (IS_ATOM(ClassName->Buffer))
        {
            Atom = (ATOM)((ULONG_PTR)ClassName->Buffer & 0xffff); // XXX: are we missing refcount here ?
        }
        else
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    IntDeregisterClassAtom(Class->atomNVClassName);

    Class->atomNVClassName = Atom;

    /* Update the clones */
    Class = Class->pclsClone;
    while (Class != NULL)
    {
        Class->atomNVClassName = Atom;

        Class = Class->pclsNext;
    }

    return TRUE;
}

//
// Same as User32:IntGetClsWndProc.
//
WNDPROC FASTCALL
IntGetClassWndProc(PCLS Class, BOOL Ansi)
{
  INT i;
  WNDPROC gcpd = NULL, Ret = NULL;

  if (Class->CSF_flags & CSF_SERVERSIDEPROC)
  {
     for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
     {
         if (GETPFNSERVER(i) == Class->lpfnWndProc)
         {
            if (Ansi)
               Ret = GETPFNCLIENTA(i);
            else
               Ret = GETPFNCLIENTW(i);
         }
     }
     return Ret;
  }
  Ret = Class->lpfnWndProc;

  if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_BUTTON)
  {
     if (Ansi)
     {
        if (GETPFNCLIENTW(Class->fnid) == Class->lpfnWndProc)
           Ret = GETPFNCLIENTA(Class->fnid);
     }
     else
     {
        if (GETPFNCLIENTA(Class->fnid) == Class->lpfnWndProc)
           Ret = GETPFNCLIENTW(Class->fnid);
     }
  }

  if ( Ret != Class->lpfnWndProc ||
       Ansi == !!(Class->CSF_flags & CSF_ANSIPROC) )
     return Ret;

  gcpd = (WNDPROC)UserGetCPD( Class,
                       (Ansi ? UserGetCPDA2U : UserGetCPDU2A )|UserGetCPDClass,
                       (ULONG_PTR)Ret);

  return (gcpd ? gcpd : Ret);
}


static
WNDPROC FASTCALL
IntSetClassWndProc(IN OUT PCLS Class,
                   IN WNDPROC WndProc,
                   IN BOOL Ansi)
{
   INT i;
   PCALLPROCDATA pcpd;
   WNDPROC Ret, chWndProc;

   Ret = IntGetClassWndProc(Class, Ansi);

   // If Server Side, downgrade to Client Side.
   if (Class->CSF_flags & CSF_SERVERSIDEPROC)
   {
      if (Ansi) Class->CSF_flags |= CSF_ANSIPROC;
      Class->CSF_flags &= ~CSF_SERVERSIDEPROC;
      Class->Unicode = !Ansi;
   }

   if (!WndProc) WndProc = Class->lpfnWndProc;

   chWndProc = WndProc;

   // Check if CallProc handle and retrieve previous call proc address and set.
   if (IsCallProcHandle(WndProc))
   {
      pcpd = UserGetObject(gHandleTable, WndProc, TYPE_CALLPROC);
      if (pcpd) chWndProc = pcpd->pfnClientPrevious;
   }

   Class->lpfnWndProc = chWndProc;

   // Clear test proc.
   chWndProc = NULL;

   // Switch from Client Side call to Server Side call if match. Ref: "deftest".
   for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
   {
       if (GETPFNCLIENTW(i) == Class->lpfnWndProc)
       {
          chWndProc = GETPFNSERVER(i);
          break;
       }
       if (GETPFNCLIENTA(i) == Class->lpfnWndProc)
       {
          chWndProc = GETPFNSERVER(i);
          break;
       }
   }
   // If match, set/reset to Server Side and clear ansi.
   if (chWndProc)
   {
      Class->lpfnWndProc = chWndProc;
      Class->Unicode = TRUE;
      Class->CSF_flags &= ~CSF_ANSIPROC;
      Class->CSF_flags |= CSF_SERVERSIDEPROC;
   }
   else
   {
      Class->Unicode = !Ansi;

      if (Ansi)
         Class->CSF_flags |= CSF_ANSIPROC;
      else
         Class->CSF_flags &= ~CSF_ANSIPROC;
   }

   /* Update the clones */
   chWndProc = Class->lpfnWndProc;

   Class = Class->pclsClone;
   while (Class != NULL)
   {
      Class->Unicode = !Ansi;
      Class->lpfnWndProc = chWndProc;

      Class = Class->pclsNext;
   }

   return Ret;
}

static PCLS
IntGetClassForDesktop(IN OUT PCLS BaseClass,
                      IN OUT PCLS *ClassLink,
                      IN PDESKTOP Desktop)
{
    SIZE_T ClassSize;
    PCLS Class;

    ASSERT(Desktop != NULL);
    ASSERT(BaseClass->pclsBase == BaseClass);

    if (BaseClass->rpdeskParent == Desktop)
    {
        /* It is most likely that a window is created on the same
           desktop as the window class. */

        return BaseClass;
    }

    if (BaseClass->rpdeskParent == NULL)
    {
        ASSERT(BaseClass->cWndReferenceCount == 0);
        ASSERT(BaseClass->pclsClone == NULL);

        /* Classes are also located in the shared heap when the class
           was created before the thread attached to a desktop. As soon
           as a window is created for such a class located on the shared
           heap, the class is cloned into the desktop heap on which the
           window is created. */
        Class = NULL;
    }
    else
    {
        /* The user is asking for a class object on a different desktop,
           try to find one! */
        Class = BaseClass->pclsClone;
        while (Class != NULL)
        {
            if (Class->rpdeskParent == Desktop)
            {
                ASSERT(Class->pclsBase == BaseClass);
                ASSERT(Class->pclsClone == NULL);
                break;
            }

            Class = Class->pclsNext;
        }
    }

    if (Class == NULL)
    {
        /* The window is created on a different desktop, we need to
           clone the class object to the desktop heap of the window! */
        ClassSize = sizeof(*BaseClass) + (SIZE_T)BaseClass->cbclsExtra;

        Class = DesktopHeapAlloc(Desktop,
                                 ClassSize);

        if (Class != NULL)
        {
            /* Simply clone the class */
            RtlCopyMemory( Class, BaseClass, ClassSize);

            /* Reference our objects */
            if (Class->spcur)
                UserReferenceObject(Class->spcur);
            if (Class->spicn)
                UserReferenceObject(Class->spicn);
            if (Class->spicnSm)
                UserReferenceObject(Class->spicnSm);

            TRACE("Clone Class 0x%p hM 0x%p\n %S\n",Class, Class->hModule, Class->lpszClientUnicodeMenuName);

            /* Restore module address if default user class Ref: Bug 4778 */
            if ( Class->hModule != hModClient &&
                 Class->fnid <= FNID_GHOST    &&
                 Class->fnid >= FNID_BUTTON )
            {
               Class->hModule = hModClient;
               TRACE("Clone Class 0x%p Reset hM 0x%p\n",Class, Class->hModule);
            }

            /* Update some pointers and link the class */
            Class->rpdeskParent = Desktop;
            Class->cWndReferenceCount = 0;

            if (BaseClass->rpdeskParent == NULL)
            {
                /* We don't really need the base class on the shared
                   heap anymore, delete it so the only class left is
                   the clone we just created, which now serves as the
                   new base class */
                ASSERT(BaseClass->pclsClone == NULL);
                ASSERT(Class->pclsClone == NULL);
                Class->pclsBase = Class;
                Class->pclsNext = BaseClass->pclsNext;

                /* Replace the base class */
                (void)InterlockedExchangePointer((PVOID*)ClassLink,
                                                 Class);

                /* Destroy the obsolete copy on the shared heap */
                BaseClass->pclsBase = NULL;
                BaseClass->pclsClone = NULL;
                IntDestroyClass(BaseClass);
            }
            else
            {
                /* Link in the clone */
                Class->pclsClone = NULL;
                Class->pclsBase = BaseClass;
                Class->pclsNext = BaseClass->pclsClone;
                (void)InterlockedExchangePointer((PVOID*)&BaseClass->pclsClone,
                                                 Class);
            }
        }
        else
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    return Class;
}

PCLS
IntReferenceClass(IN OUT PCLS BaseClass,
                  IN OUT PCLS *ClassLink,
                  IN PDESKTOP Desktop)
{
    PCLS Class;
    ASSERT(BaseClass->pclsBase == BaseClass);

    if (Desktop != NULL)
    {
        Class = IntGetClassForDesktop(BaseClass,
                                      ClassLink,
                                      Desktop);
    }
    else
    {
        Class = BaseClass;
    }

    if (Class != NULL)
    {
        Class->cWndReferenceCount++;
    }

    return Class;
}

static
VOID
IntMakeCloneBaseClass(IN OUT PCLS Class,
                      IN OUT PCLS *BaseClassLink,
                      IN OUT PCLS *CloneLink)
{
    PCLS Clone;

    ASSERT(Class->pclsBase != Class);
    ASSERT(Class->pclsBase->pclsClone != NULL);
    ASSERT(Class->rpdeskParent != NULL);
    ASSERT(Class->cWndReferenceCount != 0);
    ASSERT(Class->pclsBase->rpdeskParent != NULL);
    ASSERT(Class->pclsBase->cWndReferenceCount == 0);

    /* Unlink the clone */
    *CloneLink = Class->pclsNext;
    Class->pclsClone = Class->pclsBase->pclsClone;

    /* Update the class information to make it a base class */
    Class->pclsBase = Class;
    Class->pclsNext = (*BaseClassLink)->pclsNext;

    /* Update all clones */
    Clone = Class->pclsClone;
    while (Clone != NULL)
    {
        ASSERT(Clone->pclsClone == NULL);
        Clone->pclsBase = Class;

        Clone = Clone->pclsNext;
    }

    /* Link in the new base class */
    (void)InterlockedExchangePointer((PVOID*)BaseClassLink,
                                     Class);
}


VOID
IntDereferenceClass(IN OUT PCLS Class,
                    IN PDESKTOPINFO Desktop,
                    IN PPROCESSINFO pi)
{
    PCLS *PrevLink, BaseClass, CurrentClass;

    ASSERT(Class->cWndReferenceCount >= 1);

    BaseClass = Class->pclsBase;

    if (--Class->cWndReferenceCount == 0)
    {
        if (BaseClass == Class)
        {
            ASSERT(Class->pclsBase == Class);

            TRACE("IntDereferenceClass 0x%p\n", Class);
            /* Check if there are clones of the class on other desktops,
               link the first clone in if possible. If there are no clones
               then leave the class on the desktop heap. It will get moved
               to the shared heap when the thread detaches. */
            if (BaseClass->pclsClone != NULL)
            {
                if (BaseClass->Global)
                    PrevLink = &pi->pclsPublicList;
                else
                    PrevLink = &pi->pclsPrivateList;

                CurrentClass = *PrevLink;
                while (CurrentClass != BaseClass)
                {
                    ASSERT(CurrentClass != NULL);

                    PrevLink = &CurrentClass->pclsNext;
                    CurrentClass = CurrentClass->pclsNext;
                }

                ASSERT(*PrevLink == BaseClass);

                /* Make the first clone become the new base class */
                IntMakeCloneBaseClass(BaseClass->pclsClone,
                                      PrevLink,
                                      &BaseClass->pclsClone);

                /* Destroy the class, there's still another clone of the class
                   that now serves as a base class. Make sure we don't destruct
                   resources shared by all classes (Base = NULL)! */
                BaseClass->pclsBase = NULL;
                BaseClass->pclsClone = NULL;
                IntDestroyClass(BaseClass);
            }
        }
        else
        {
            TRACE("IntDereferenceClass1 0x%p\n", Class);

            /* Locate the cloned class and unlink it */
            PrevLink = &BaseClass->pclsClone;
            CurrentClass = BaseClass->pclsClone;
            while (CurrentClass != Class)
            {
                ASSERT(CurrentClass != NULL);

                PrevLink = &CurrentClass->pclsNext;
                CurrentClass = CurrentClass->pclsNext;
            }

            ASSERT(CurrentClass == Class);

            (void)InterlockedExchangePointer((PVOID*)PrevLink,
                                             Class->pclsNext);

            ASSERT(Class->pclsBase == BaseClass);
            ASSERT(Class->pclsClone == NULL);

            /* The class was just a clone, we don't need it anymore */
            IntDestroyClass(Class);
        }
    }
}

static BOOL
IntMoveClassToSharedHeap(IN OUT PCLS Class,
                         IN OUT PCLS **ClassLinkPtr)
{
    PCLS NewClass;
    SIZE_T ClassSize;

    ASSERT(Class->pclsBase == Class);
    ASSERT(Class->rpdeskParent != NULL);
    ASSERT(Class->cWndReferenceCount == 0);
    ASSERT(Class->pclsClone == NULL);

    ClassSize = sizeof(*Class) + (SIZE_T)Class->cbclsExtra;

    /* Allocate the new base class on the shared heap */
    NewClass = UserHeapAlloc(ClassSize);
    if (NewClass != NULL)
    {
        RtlCopyMemory(NewClass,
                      Class,
                      ClassSize);

        NewClass->rpdeskParent = NULL;
        NewClass->pclsBase = NewClass;

        if (NewClass->spcur)
            UserReferenceObject(NewClass->spcur);
        if (NewClass->spicn)
            UserReferenceObject(NewClass->spicn);
        if (NewClass->spicnSm)
            UserReferenceObject(NewClass->spicnSm);

        /* Replace the class in the list */
        (void)InterlockedExchangePointer((PVOID*)*ClassLinkPtr,
                                         NewClass);
        *ClassLinkPtr = &NewClass->pclsNext;

        /* Free the obsolete class on the desktop heap */
        Class->pclsBase = NULL;
        IntDestroyClass(Class);
        return TRUE;
    }

    return FALSE;
}

static VOID
IntCheckDesktopClasses(IN PDESKTOP Desktop,
                       IN OUT PCLS *ClassList,
                       IN BOOL FreeOnFailure,
                       OUT BOOL *Ret)
{
    PCLS Class, NextClass, *Link;

    /* NOTE: We only need to check base classes! When classes are no longer needed
             on a desktop, the clones will be freed automatically as soon as possible.
             However, we need to move base classes to the shared heap, as soon as
             the last desktop heap where a class is allocated on is about to be destroyed.
             If we didn't move the class to the shared heap, the class would become
             inaccessible! */

    ASSERT(Desktop != NULL);

    Link = ClassList;
    Class = *Link;
    while (Class != NULL)
    {
        NextClass = Class->pclsNext;

        ASSERT(Class->pclsBase == Class);

        if (Class->rpdeskParent == Desktop &&
            Class->cWndReferenceCount == 0)
        {
            /* There shouldn't be any clones around anymore! */
            ASSERT(Class->pclsClone == NULL);

            /* FIXME: If process is terminating, don't move the class but rather destroy it! */
            /* FIXME: We could move the class to another desktop heap if there's still desktops
                       mapped into the process... */

            /* Move the class to the shared heap */
            if (IntMoveClassToSharedHeap(Class,
                                         &Link))
            {
                ASSERT(*Link == NextClass);
            }
            else
            {
                ASSERT(NextClass == Class->pclsNext);

                if (FreeOnFailure)
                {
                    /* Unlink the base class */
                    (void)InterlockedExchangePointer((PVOID*)Link,
                                                     Class->pclsNext);

                    /* We can free the old base class now */
                    Class->pclsBase = NULL;
                    IntDestroyClass(Class);
                }
                else
                {
                    Link = &Class->pclsNext;
                    *Ret = FALSE;
                }
            }
        }
        else
            Link = &Class->pclsNext;

        Class = NextClass;
    }
}

BOOL
IntCheckProcessDesktopClasses(IN PDESKTOP Desktop,
                              IN BOOL FreeOnFailure)
{
    PPROCESSINFO pi;
    BOOL Ret = TRUE;

    pi = GetW32ProcessInfo();

    /* Check all local classes */
    IntCheckDesktopClasses(Desktop,
                           &pi->pclsPrivateList,
                           FreeOnFailure,
                           &Ret);

    /* Check all global classes */
    IntCheckDesktopClasses(Desktop,
                           &pi->pclsPublicList,
                           FreeOnFailure,
                           &Ret);
    if (!Ret)
    {
        ERR("Failed to move process classes from desktop 0x%p to the shared heap!\n", Desktop);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return Ret;
}

PCLS
FASTCALL
IntCreateClass(IN CONST WNDCLASSEXW* lpwcx,
               IN PUNICODE_STRING ClassName,
               IN PUNICODE_STRING ClassVersion,
               IN PUNICODE_STRING MenuName,
               IN DWORD fnID,
               IN DWORD dwFlags,
               IN PDESKTOP Desktop,
               IN PPROCESSINFO pi)
{
    SIZE_T ClassSize;
    PCLS Class = NULL;
    RTL_ATOM Atom, verAtom;
    WNDPROC WndProc;
    PWSTR pszMenuName = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("lpwcx=%p ClassName=%wZ MenuName=%wZ dwFlags=%08x Desktop=%p pi=%p\n",
        lpwcx, ClassName, MenuName, dwFlags, Desktop, pi);

    if (!IntRegisterClassAtom(ClassName,
                              &Atom))
    {
        ERR("Failed to register class atom!\n");
        return NULL;
    }

    if (!IntRegisterClassAtom(ClassVersion,
                              &verAtom))
    {
        ERR("Failed to register version class atom!\n");
        IntDeregisterClassAtom(Atom);
        return NULL;
    }

    ClassSize = sizeof(*Class) + lpwcx->cbClsExtra;
    if (MenuName->Length != 0)
    {
        pszMenuName = UserHeapAlloc(MenuName->Length + sizeof(UNICODE_NULL) +
                                    RtlUnicodeStringToAnsiSize(MenuName));
        if (pszMenuName == NULL)
            goto NoMem;
    }

    if (Desktop != NULL)
    {
        Class = DesktopHeapAlloc(Desktop,
                                 ClassSize);
    }
    else
    {
        /* FIXME: The class was created before being connected
                  to a desktop. It is possible for the desktop window,
                  but should it be allowed for any other case? */
        TRACE("This CLASS has no Desktop to heap from! Atom %u\n",Atom);
        Class = UserHeapAlloc(ClassSize);
    }

    if (Class != NULL)
    {
        int iCls = 0;

        RtlZeroMemory(Class, ClassSize);

        Class->rpdeskParent = Desktop;
        Class->pclsBase = Class;
        Class->atomClassName = verAtom;
        Class->atomNVClassName = Atom;
        Class->fnid = fnID;
        Class->CSF_flags = dwFlags;

        if (LookupFnIdToiCls(Class->fnid, &iCls) && gpsi->atomSysClass[iCls] == 0)
        {
            gpsi->atomSysClass[iCls] = Class->atomClassName;
        }

        _SEH2_TRY
        {
            PWSTR pszMenuNameBuffer = pszMenuName;

            /* Need to protect with SEH since accessing the WNDCLASSEX structure
               and string buffers might raise an exception! We don't want to
               leak memory... */
            // What?! If the user interface was written correctly this would not be an issue!
            Class->lpfnWndProc = lpwcx->lpfnWndProc;
            Class->style = lpwcx->style;
            Class->cbclsExtra = lpwcx->cbClsExtra;
            Class->cbwndExtra = lpwcx->cbWndExtra;
            Class->hModule = lpwcx->hInstance;
            Class->spicn = lpwcx->hIcon ? UserGetCurIconObject(lpwcx->hIcon) : NULL;
            Class->spcur = lpwcx->hCursor ? UserGetCurIconObject(lpwcx->hCursor) : NULL;
            Class->spicnSm = lpwcx->hIconSm ? UserGetCurIconObject(lpwcx->hIconSm) : NULL;
            ////
            Class->hbrBackground = lpwcx->hbrBackground;

            /* Make a copy of the string */
            if (pszMenuNameBuffer != NULL)
            {
                Class->MenuNameIsString = TRUE;

                Class->lpszClientUnicodeMenuName = pszMenuNameBuffer;
                RtlCopyMemory(Class->lpszClientUnicodeMenuName,
                              MenuName->Buffer,
                              MenuName->Length);
                Class->lpszClientUnicodeMenuName[MenuName->Length / sizeof(WCHAR)] = UNICODE_NULL;

                pszMenuNameBuffer += (MenuName->Length / sizeof(WCHAR)) + 1;
            }
            else
                Class->lpszClientUnicodeMenuName = MenuName->Buffer;

            /* Save an ANSI copy of the string */
            if (pszMenuNameBuffer != NULL)
            {
                ANSI_STRING AnsiString;

                Class->lpszClientAnsiMenuName = (PSTR)pszMenuNameBuffer;
                AnsiString.MaximumLength = (USHORT)RtlUnicodeStringToAnsiSize(MenuName);
                AnsiString.Buffer = Class->lpszClientAnsiMenuName;
                Status = RtlUnicodeStringToAnsiString(&AnsiString,
                                                      MenuName,
                                                      FALSE);
                if (!NT_SUCCESS(Status))
                {
                    ERR("Failed to convert unicode menu name to ansi!\n");

                    /* Life would've been much prettier if ntoskrnl exported RtlRaiseStatus()... */
                    _SEH2_LEAVE;
                }
            }
            else
                Class->lpszClientAnsiMenuName = (PSTR)MenuName->Buffer;

            /* Save kernel use menu name and ansi class name */
            Class->lpszMenuName = Class->lpszClientUnicodeMenuName; // FIXME!
            //Class->lpszAnsiClassName = FIXME

            /* Server Side overrides class calling type (A/W)!
               User32 whine test_builtinproc: "deftest"
                  built-in winproc - window A/W type automatically detected */
            if (!(Class->CSF_flags & CSF_SERVERSIDEPROC))
            {
               int i;
               WndProc = NULL;
          /* Due to the wine class "deftest" and most likely no FNID to reference
             from, sort through the Server Side list and compare proc addresses
             for match. This method will be used in related code.
           */
               for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
               { // Open ANSI or Unicode, just match, set and break.
                   if (GETPFNCLIENTW(i) == Class->lpfnWndProc)
                   {
                      WndProc = GETPFNSERVER(i);
                      break;
                   }
                   if (GETPFNCLIENTA(i) == Class->lpfnWndProc)
                   {
                      WndProc = GETPFNSERVER(i);
                      break;
                   }
               }
               if (WndProc)
               {  // If a hit, we are Server Side so set the right flags and proc.
                  Class->CSF_flags |= CSF_SERVERSIDEPROC;
                  Class->CSF_flags &= ~CSF_ANSIPROC;
                  Class->lpfnWndProc = WndProc;
               }
            }

            if (!(Class->CSF_flags & CSF_ANSIPROC))
                Class->Unicode = TRUE;

            if (Class->style & CS_GLOBALCLASS)
                Class->Global = TRUE;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            ERR("Failed creating the class: 0x%x\n", Status);

            SetLastNtError(Status);

            if (pszMenuName != NULL)
                UserHeapFree(pszMenuName);

            DesktopHeapFree(Desktop,
                            Class);
            Class = NULL;

            IntDeregisterClassAtom(verAtom);
            IntDeregisterClassAtom(Atom);
        }
    }
    else
    {
NoMem:
        ERR("Failed to allocate class on Desktop 0x%p\n", Desktop);

        if (pszMenuName != NULL)
            UserHeapFree(pszMenuName);

        IntDeregisterClassAtom(Atom);
        IntDeregisterClassAtom(verAtom);

        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    TRACE("Created class 0x%p with name %wZ and proc 0x%p for atom 0x%x and version atom 0x%x and hInstance 0x%p, global %u\n",
          Class, ClassName, Class ? Class->lpfnWndProc : NULL, Atom, verAtom,
          Class ? Class->hModule : NULL , Class ? Class->Global : 0);

    return Class;
}

static PCLS
IntFindClass(IN RTL_ATOM Atom,
             IN HINSTANCE hInstance,
             IN PCLS *ClassList,
             OUT PCLS **Link  OPTIONAL)
{
    PCLS Class, *PrevLink = ClassList;

    Class = *PrevLink;
    while (Class != NULL)
    {
        if (Class->atomClassName == Atom &&
            (hInstance == NULL || Class->hModule == hInstance) &&
            !(Class->CSF_flags & CSF_WOWDEFERDESTROY))
        {
            ASSERT(Class->pclsBase == Class);

            if (Link != NULL)
                *Link = PrevLink;
            break;
        }

        PrevLink = &Class->pclsNext;
        Class = Class->pclsNext;
    }

    return Class;
}

_Success_(return)
BOOL
NTAPI
IntGetAtomFromStringOrAtom(
    _In_ PUNICODE_STRING ClassName,
    _Out_ RTL_ATOM *Atom)
{
    BOOL Ret = FALSE;

    if (ClassName->Length != 0)
    {
        WCHAR szBuf[65];
        PWSTR AtomName = szBuf;
        NTSTATUS Status = STATUS_INVALID_PARAMETER;

        *Atom = 0;

        /* NOTE: Caller has to protect the call with SEH! */
        if (ClassName->Length + sizeof(UNICODE_NULL) > sizeof(szBuf))
        {
            AtomName = ExAllocatePoolWithTag(PagedPool,
                                             ClassName->Length + sizeof(UNICODE_NULL),
                                             TAG_USTR);
            if (AtomName == NULL)
            {
                EngSetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }
        }

        _SEH2_TRY
        {
            RtlCopyMemory(AtomName,
                          ClassName->Buffer,
                          ClassName->Length);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            if (AtomName != szBuf)
                ExFreePoolWithTag(AtomName, TAG_USTR);
            SetLastNtError(_SEH2_GetExceptionCode());
            _SEH2_YIELD(return FALSE);
        }
        _SEH2_END;
        AtomName[ClassName->Length / sizeof(WCHAR)] = UNICODE_NULL;

        /* Lookup the atom */
        Status = RtlLookupAtomInAtomTable(gAtomTable, AtomName, Atom);

        if (AtomName != szBuf)
            ExFreePoolWithTag(AtomName, TAG_USTR);

        if (NT_SUCCESS(Status))
        {
            Ret = TRUE;
        }
        else
        {
            if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            {
                SetLastNtError(Status);
            }
        }
    }
    else
    {
        if (ClassName->Buffer)
        {
            *Atom = (RTL_ATOM)((ULONG_PTR)ClassName->Buffer);
            Ret = TRUE;
        }
        else
        {
            *Atom = 0;
            EngSetLastError(ERROR_CLASS_DOES_NOT_EXIST);
            Ret = FALSE;
        }
    }

    return Ret;
}

RTL_ATOM
IntGetClassAtom(
    _In_ PUNICODE_STRING ClassName,
    IN HINSTANCE hInstance  OPTIONAL,
    IN PPROCESSINFO pi  OPTIONAL,
    OUT PCLS *BaseClass  OPTIONAL,
    OUT PCLS **Link  OPTIONAL)
{
    RTL_ATOM Atom = (RTL_ATOM)0;

    ASSERT(BaseClass != NULL);

    if (IntGetAtomFromStringOrAtom(ClassName, &Atom) &&
        Atom != (RTL_ATOM)0)
    {
        PCLS Class;

        /* Attempt to locate the class object */

        ASSERT(pi != NULL);

        /* Step 1: Try to find an exact match of locally registered classes */
        Class = IntFindClass(Atom,
                             hInstance,
                             &pi->pclsPrivateList,
                             Link);
        if (Class != NULL)
        {  TRACE("Step 1: 0x%p\n",Class );
            goto FoundClass;
        }

        /* Step 2: Try to find any globally registered class. The hInstance
                   is not relevant for global classes */
        Class = IntFindClass(Atom,
                             NULL,
                             &pi->pclsPublicList,
                             Link);
        if (Class != NULL)
        { TRACE("Step 2: 0x%p 0x%p\n",Class, Class->hModule);
            goto FoundClass;
        }

        /* Step 3: Try to find any local class registered by user32 */
        Class = IntFindClass(Atom,
                             hModClient,
                             &pi->pclsPrivateList,
                             Link);
        if (Class != NULL)
        { TRACE("Step 3: 0x%p\n",Class );
            goto FoundClass;
        }

        /* Step 4: Try to find any global class registered by user32 */
        Class = IntFindClass(Atom,
                             hModClient,
                             &pi->pclsPublicList,
                             Link);
        if (Class == NULL)
        {
            return (RTL_ATOM)0;
        }else{TRACE("Step 4: 0x%p\n",Class );}

FoundClass:
        *BaseClass = Class;
    }
    else
    {
        Atom = 0;
    }

    return Atom;
}

PCLS
IntGetAndReferenceClass(PUNICODE_STRING ClassName, HINSTANCE hInstance, BOOL bDesktopThread)
{
   PCLS *ClassLink, Class = NULL;
   RTL_ATOM ClassAtom;
   PTHREADINFO pti;

   if (bDesktopThread)
       pti = gptiDesktopThread;
   else
       pti = PsGetCurrentThreadWin32Thread();

   if ( !(pti->ppi->W32PF_flags & W32PF_CLASSESREGISTERED ))
   {
      UserRegisterSystemClasses();
   }

   /* Check the class. */

   TRACE("Finding Class %wZ for hInstance 0x%p\n", ClassName, hInstance);

   ClassAtom = IntGetClassAtom(ClassName,
                               hInstance,
                               pti->ppi,
                               &Class,
                               &ClassLink);

   if (ClassAtom == (RTL_ATOM)0)
   {
      if (IS_ATOM(ClassName->Buffer))
      {
         ERR("Class 0x%p not found\n", ClassName->Buffer);
      }
      else
      {
         ERR("Class \"%wZ\" not found\n", ClassName);
      }

      return NULL;
   }

   TRACE("Referencing Class 0x%p with atom 0x%x\n", Class, ClassAtom);
   Class = IntReferenceClass(Class,
                             ClassLink,
                             pti->rpdesk);
   if (Class == NULL)
   {
       ERR("Failed to reference window class!\n");
       return NULL;
   }

   return Class;
}

RTL_ATOM
UserRegisterClass(IN CONST WNDCLASSEXW* lpwcx,
                  IN PUNICODE_STRING ClassName,
                  IN PUNICODE_STRING ClassVersion,
                  IN PUNICODE_STRING MenuName,
                  IN DWORD fnID,
                  IN DWORD dwFlags)
{
    PTHREADINFO pti;
    PPROCESSINFO pi;
    PCLS Class;
    RTL_ATOM ClassAtom;
    RTL_ATOM Ret = (RTL_ATOM)0;

    /* NOTE: Accessing the buffers in ClassName and MenuName may raise exceptions! */

    pti = GetW32ThreadInfo();

    pi = pti->ppi;

    // Need only to test for two conditions not four....... Fix more whine tests....
    if ( IntGetAtomFromStringOrAtom( ClassVersion, &ClassAtom) &&
                                     ClassAtom != (RTL_ATOM)0 &&
                                    !(dwFlags & CSF_SERVERSIDEPROC) ) // Bypass Server Sides
    {
       Class = IntFindClass( ClassAtom,
                             lpwcx->hInstance,
                            &pi->pclsPrivateList,
                             NULL);

       if (Class != NULL && !Class->Global)
       {
          // Local class already exists
          TRACE("Local Class 0x%x does already exist!\n", ClassAtom);
          EngSetLastError(ERROR_CLASS_ALREADY_EXISTS);
          return (RTL_ATOM)0;
       }

       if (lpwcx->style & CS_GLOBALCLASS)
       {
          Class = IntFindClass( ClassAtom,
                                NULL,
                               &pi->pclsPublicList,
                                NULL);

          if (Class != NULL && Class->Global)
          {
             TRACE("Global Class 0x%x does already exist!\n", ClassAtom);
             EngSetLastError(ERROR_CLASS_ALREADY_EXISTS);
             return (RTL_ATOM)0;
          }
       }
    }

    Class = IntCreateClass(lpwcx,
                           ClassName,
                           ClassVersion,
                           MenuName,
                           fnID,
                           dwFlags,
                           pti->rpdesk,
                           pi);

    if (Class != NULL)
    {
        PCLS *List;

        /* Register the class */
        if (Class->Global)
            List = &pi->pclsPublicList;
        else
            List = &pi->pclsPrivateList;

        Class->pclsNext = *List;
        (void)InterlockedExchangePointer((PVOID*)List,
                                         Class);

        Ret = Class->atomNVClassName;
    }
    else
    {
       ERR("UserRegisterClass: Yes, that is right, you have no Class!\n");
    }

    return Ret;
}

BOOL
UserUnregisterClass(IN PUNICODE_STRING ClassName,
                    IN HINSTANCE hInstance,
                    OUT PCLSMENUNAME pClassMenuName)
{
    PCLS *Link;
    PPROCESSINFO pi;
    RTL_ATOM ClassAtom;
    PCLS Class;

    pi = GetW32ProcessInfo();

    TRACE("UserUnregisterClass(%wZ, 0x%p)\n", ClassName, hInstance);

    /* NOTE: Accessing the buffer in ClassName may raise an exception! */
    ClassAtom = IntGetClassAtom(ClassName,
                                hInstance,
                                pi,
                                &Class,
                                &Link);
    if (ClassAtom == (RTL_ATOM)0)
    {
        EngSetLastError(ERROR_CLASS_DOES_NOT_EXIST);
        TRACE("UserUnregisterClass: No Class found.\n");
        return FALSE;
    }

    ASSERT(Class != NULL);

    if (Class->cWndReferenceCount != 0 ||
        Class->pclsClone != NULL)
    {
        TRACE("UserUnregisterClass: Class has a Window. Ct %u : Clone 0x%p\n", Class->cWndReferenceCount, Class->pclsClone);
        EngSetLastError(ERROR_CLASS_HAS_WINDOWS);
        return FALSE;
    }

    /* Must be a base class! */
    ASSERT(Class->pclsBase == Class);

    /* Unlink the class */
    *Link = Class->pclsNext;

    if (NT_SUCCESS(IntDeregisterClassAtom(Class->atomClassName)))
    {
        TRACE("Class 0x%p\n", Class);
        TRACE("UserUnregisterClass: Good Exit!\n");
        Class->atomClassName = 0; // Don't let it linger...
        /* Finally free the resources */
        IntDestroyClass(Class);
        return TRUE;
    }
    ERR("UserUnregisterClass: Can not deregister Class Atom.\n");
    return FALSE;
}

INT
UserGetClassName(IN PCLS Class,
                 IN OUT PUNICODE_STRING ClassName,
                 IN RTL_ATOM Atom,
                 IN BOOL Ansi)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR szStaticTemp[32];
    PWSTR szTemp = NULL;
    ULONG BufLen = sizeof(szStaticTemp);
    INT Ret = 0;

    /* Note: Accessing the buffer in ClassName may raise an exception! */

    _SEH2_TRY
    {
        if (Ansi)
        {
            PANSI_STRING AnsiClassName = (PANSI_STRING)ClassName;
            UNICODE_STRING UnicodeClassName;

            /* Limit the size of the static buffer on the stack to the
               size of the buffer provided by the caller */
            if (BufLen / sizeof(WCHAR) > AnsiClassName->MaximumLength)
            {
                BufLen = AnsiClassName->MaximumLength * sizeof(WCHAR);
            }

            /* Find out how big the buffer needs to be */
            Status = RtlQueryAtomInAtomTable(gAtomTable,
                                             Class->atomClassName,
                                             NULL,
                                             NULL,
                                             szStaticTemp,
                                             &BufLen);
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                if (BufLen / sizeof(WCHAR) > AnsiClassName->MaximumLength)
                {
                    /* The buffer required exceeds the ansi buffer provided,
                       pretend like we're using the ansi buffer and limit the
                       size to the buffer size provided */
                    BufLen = AnsiClassName->MaximumLength * sizeof(WCHAR);
                }

                /* Allocate a temporary buffer that can hold the unicode class name */
                szTemp = ExAllocatePoolWithTag(PagedPool,
                                               BufLen,
                                               USERTAG_CLASS);
                if (szTemp == NULL)
                {
                    EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    _SEH2_LEAVE;
                }

                /* Query the class name */
                Status = RtlQueryAtomInAtomTable(gAtomTable,
                                                 Atom ? Atom : Class->atomNVClassName,
                                                 NULL,
                                                 NULL,
                                                 szTemp,
                                                 &BufLen);
            }
            else
                szTemp = szStaticTemp;

            if (NT_SUCCESS(Status))
            {
                /* Convert the atom name to ansi */

                RtlInitUnicodeString(&UnicodeClassName,
                                     szTemp);

                Status = RtlUnicodeStringToAnsiString(AnsiClassName,
                                                      &UnicodeClassName,
                                                      FALSE);
                if (!NT_SUCCESS(Status))
                {
                    SetLastNtError(Status);
                    _SEH2_LEAVE;
                }
            }

            Ret = BufLen / sizeof(WCHAR);
        }
        else /* !ANSI */
        {
            BufLen = ClassName->MaximumLength;

            /* Query the atom name */
            Status = RtlQueryAtomInAtomTable(gAtomTable,
                                             Atom ? Atom : Class->atomNVClassName,
                                             NULL,
                                             NULL,
                                             ClassName->Buffer,
                                             &BufLen);

            if (!NT_SUCCESS(Status))
            {
                SetLastNtError(Status);
                _SEH2_LEAVE;
            }

            Ret = BufLen / sizeof(WCHAR);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    if (Ansi && szTemp != NULL && szTemp != szStaticTemp)
    {
        ExFreePoolWithTag(szTemp, USERTAG_CLASS);
    }

    return Ret;
}

static BOOL
IntSetClassMenuName(IN PCLS Class,
                    IN PUNICODE_STRING MenuName)
{
    BOOL Ret = FALSE;

    /* Change the base class first */
    Class = Class->pclsBase;

    if (MenuName->Length != 0)
    {
        ANSI_STRING AnsiString;
        PWSTR strBufW;

        AnsiString.MaximumLength = (USHORT)RtlUnicodeStringToAnsiSize(MenuName);

        strBufW = UserHeapAlloc(MenuName->Length + sizeof(UNICODE_NULL) +
                                AnsiString.MaximumLength);
        if (strBufW != NULL)
        {
            _SEH2_TRY
            {
                NTSTATUS Status;

                /* Copy the unicode string */
                RtlCopyMemory(strBufW,
                              MenuName->Buffer,
                              MenuName->Length);
                strBufW[MenuName->Length / sizeof(WCHAR)] = UNICODE_NULL;

                /* Create an ANSI copy of the string */
                AnsiString.Buffer = (PSTR)(strBufW + (MenuName->Length / sizeof(WCHAR)) + 1);
                Status = RtlUnicodeStringToAnsiString(&AnsiString,
                                                      MenuName,
                                                      FALSE);
                if (!NT_SUCCESS(Status))
                {
                    SetLastNtError(Status);
                    _SEH2_LEAVE;
                }

                Ret = TRUE;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                SetLastNtError(_SEH2_GetExceptionCode());
            }
            _SEH2_END;

            if (Ret)
            {
                /* Update the base class */
                IntFreeClassMenuName(Class);
                Class->lpszClientUnicodeMenuName = strBufW;
                Class->lpszClientAnsiMenuName = AnsiString.Buffer;
                Class->MenuNameIsString = TRUE;

                /* Update the clones */
                Class = Class->pclsClone;
                while (Class != NULL)
                {
                    Class->lpszClientUnicodeMenuName = strBufW;
                    Class->lpszClientAnsiMenuName = AnsiString.Buffer;
                    Class->MenuNameIsString = TRUE;

                    Class = Class->pclsNext;
                }
            }
            else
            {
                ERR("Failed to copy class menu name!\n");
                UserHeapFree(strBufW);
            }
        }
        else
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    else
    {
        ASSERT(IS_INTRESOURCE(MenuName->Buffer));

        /* Update the base class */
        IntFreeClassMenuName(Class);
        Class->lpszClientUnicodeMenuName = MenuName->Buffer;
        Class->lpszClientAnsiMenuName = (PSTR)MenuName->Buffer;
        Class->MenuNameIsString = FALSE;

        /* Update the clones */
        Class = Class->pclsClone;
        while (Class != NULL)
        {
            Class->lpszClientUnicodeMenuName = MenuName->Buffer;
            Class->lpszClientAnsiMenuName = (PSTR)MenuName->Buffer;
            Class->MenuNameIsString = FALSE;

            Class = Class->pclsNext;
        }

        Ret = TRUE;
    }

    return Ret;
}

static inline
ULONG_PTR
IntGetSetClassLongPtr(PCLS Class, ULONG Index, ULONG_PTR NewValue, ULONG Size)
{
    PVOID Address = (PUCHAR)(&Class[1]) + Index;
    ULONG_PTR OldValue;

#ifdef _WIN64
    if (Size == sizeof(LONG))
    {
        /* Values might be unaligned */
        OldValue = ReadUnalignedU32(Address);
        WriteUnalignedU32(Address, NewValue);
    }
    else
#endif
    {
        /* Values might be unaligned */
        OldValue = ReadUnalignedUlongPtr(Address);
        WriteUnalignedUlongPtr(Address, NewValue);
    }

    return OldValue;
}

ULONG_PTR
UserSetClassLongPtr(IN PCLS Class,
                    IN INT Index,
                    IN ULONG_PTR NewLong,
                    IN BOOL Ansi,
                    IN ULONG Size)
{
    ULONG_PTR Ret = 0;

    /* NOTE: For GCLP_MENUNAME and GCW_ATOM this function may raise an exception! */

    /* Change the information in the base class first, then update the clones */
    Class = Class->pclsBase;

    if (Index >= 0)
    {
        TRACE("SetClassLong(%d, %x)\n", Index, NewLong);

        if (((ULONG)Index + Size) < (ULONG)Index ||
            ((ULONG)Index + Size) > (ULONG)Class->cbclsExtra)
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        Ret = IntGetSetClassLongPtr(Class, Index, NewLong, Size);

        /* Update the clones */
        Class = Class->pclsClone;
        while (Class != NULL)
        {
            IntGetSetClassLongPtr(Class, Index, NewLong, Size);
            Class = Class->pclsNext;
        }

        return Ret;
    }

    switch (Index)
    {
        case GCL_CBWNDEXTRA:
            Ret = (ULONG_PTR)Class->cbwndExtra;
            Class->cbwndExtra = (INT)NewLong;

            /* Update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                Class->cbwndExtra = (INT)NewLong;
                Class = Class->pclsNext;
            }

            break;

        case GCL_CBCLSEXTRA:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            break;

        case GCLP_HBRBACKGROUND:
            Ret = (ULONG_PTR)Class->hbrBackground;
            Class->hbrBackground = (HBRUSH)NewLong;

            /* Update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                Class->hbrBackground = (HBRUSH)NewLong;
                Class = Class->pclsNext;
            }
            break;

        case GCLP_HCURSOR:
        {
            PCURICON_OBJECT NewCursor = NULL;

            if (NewLong)
            {
                NewCursor = UserGetCurIconObject((HCURSOR)NewLong);
                if (!NewCursor)
                {
                    EngSetLastError(ERROR_INVALID_CURSOR_HANDLE);
                    return 0;
                }
            }

            if (Class->spcur)
            {
                Ret = (ULONG_PTR)UserHMGetHandle(Class->spcur);
                UserDereferenceObject(Class->spcur);
            }
            else
            {
                Ret = 0;
            }

            if (Ret == NewLong)
            {
                /* It's a nop */
                return Ret;
            }

            Class->spcur = NewCursor;

            /* Update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                if (Class->spcur)
                    UserDereferenceObject(Class->spcur);
                if (NewCursor)
                    UserReferenceObject(NewCursor);
                Class->spcur = NewCursor;
                Class = Class->pclsNext;
            }

            break;
        }

        // MSDN:
        // hIconSm, A handle to a small icon that is associated with the window class.
        // If this member is NULL, the system searches the icon resource specified by
        // the hIcon member for an icon of the appropriate size to use as the small icon.
        //
        case GCLP_HICON:
        {
            PCURICON_OBJECT NewIcon = NULL;
            PCURICON_OBJECT NewSmallIcon = NULL;

            if (NewLong)
            {
                NewIcon = UserGetCurIconObject((HCURSOR)NewLong);
                if (!NewIcon)
                {
                    EngSetLastError(ERROR_INVALID_ICON_HANDLE);
                    return 0;
                }
            }

            if (Class->spicn)
            {
                Ret = (ULONG_PTR)UserHMGetHandle(Class->spicn);
                UserDereferenceObject(Class->spicn);
            }
            else
            {
                Ret = 0;
            }

            if (Ret == NewLong)
            {
                /* It's a nop */
                return Ret;
            }

            if (Ret && (Class->CSF_flags & CSF_CACHEDSMICON))
            {
                /* We will change the small icon */
                UserDereferenceObject(Class->spicnSm);
                IntDestroyCurIconObject(Class->spicnSm);
                Class->spicnSm = NULL;
                Class->CSF_flags &= ~CSF_CACHEDSMICON;
            }

            if (NewLong && !Class->spicnSm)
            {
                /* Create the new small icon from the new large(?) one */
                HICON SmallIconHandle = NULL;
                if((NewIcon->CURSORF_flags & (CURSORF_LRSHARED | CURSORF_FROMRESOURCE))
                        == (CURSORF_LRSHARED | CURSORF_FROMRESOURCE))
                {
                    SmallIconHandle = co_IntCopyImage(
                        (HICON)NewLong,
                        IMAGE_ICON,
                        UserGetSystemMetrics( SM_CXSMICON ),
                        UserGetSystemMetrics( SM_CYSMICON ),
                        LR_COPYFROMRESOURCE);
                }
                if (!SmallIconHandle)
                {
                    /* Retry without copying from resource */
                    SmallIconHandle = co_IntCopyImage(
                        (HICON)NewLong,
                        IMAGE_ICON,
                        UserGetSystemMetrics( SM_CXSMICON ),
                        UserGetSystemMetrics( SM_CYSMICON ),
                        0);
                }
                if (SmallIconHandle)
                {
                    /* So use it */
                    NewSmallIcon = Class->spicnSm = UserGetCurIconObject(SmallIconHandle);
                    Class->CSF_flags |= CSF_CACHEDSMICON;
                }
            }

            Class->spicn = NewIcon;

            /* Update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                if (Class->spicn)
                    UserDereferenceObject(Class->spicn);
                if (NewIcon)
                    UserReferenceObject(NewIcon);
                Class->spicn = NewIcon;
                if (NewSmallIcon)
                {
                    if (Class->spicnSm)
                        UserDereferenceObject(Class->spicnSm);
                    UserReferenceObject(NewSmallIcon);
                    Class->spicnSm = NewSmallIcon;
                    Class->CSF_flags |= CSF_CACHEDSMICON;
                }
                Class = Class->pclsNext;
            }
            break;
        }

        case GCLP_HICONSM:
        {
            PCURICON_OBJECT NewSmallIcon = NULL;
            BOOLEAN NewIconFromCache = FALSE;

            if (NewLong)
            {
                NewSmallIcon = UserGetCurIconObject((HCURSOR)NewLong);
                if (!NewSmallIcon)
                {
                    EngSetLastError(ERROR_INVALID_ICON_HANDLE);
                    return 0;
                }
            }
            else
            {
                /* Create the new small icon from the large one */
                HICON SmallIconHandle = NULL;
                if((Class->spicn->CURSORF_flags & (CURSORF_LRSHARED | CURSORF_FROMRESOURCE))
                        == (CURSORF_LRSHARED | CURSORF_FROMRESOURCE))
                {
                    SmallIconHandle = co_IntCopyImage(
                        UserHMGetHandle(Class->spicn),
                        IMAGE_ICON,
                        UserGetSystemMetrics( SM_CXSMICON ),
                        UserGetSystemMetrics( SM_CYSMICON ),
                        LR_COPYFROMRESOURCE);
                }
                if (!SmallIconHandle)
                {
                    /* Retry without copying from resource */
                    SmallIconHandle = co_IntCopyImage(
                        UserHMGetHandle(Class->spicn),
                        IMAGE_ICON,
                        UserGetSystemMetrics( SM_CXSMICON ),
                        UserGetSystemMetrics( SM_CYSMICON ),
                        0);
                }
                if (SmallIconHandle)
                {
                    /* So use it */
                    NewSmallIcon = UserGetCurIconObject(SmallIconHandle);
                    NewIconFromCache = TRUE;
                }
                else
                {
                    ERR("Failed getting a small icon for the class.\n");
                }
            }

            if (Class->spicnSm)
            {
                if (Class->CSF_flags & CSF_CACHEDSMICON)
                {
                    /* We must destroy the icon if we own it */
                    IntDestroyCurIconObject(Class->spicnSm);
                    Ret = 0;
                }
                else
                {
                    Ret = (ULONG_PTR)UserHMGetHandle(Class->spicnSm);
                }
                UserDereferenceObject(Class->spicnSm);
            }
            else
            {
                Ret = 0;
            }

            if (NewIconFromCache)
                Class->CSF_flags |= CSF_CACHEDSMICON;
            else
                Class->CSF_flags &= ~CSF_CACHEDSMICON;
            Class->spicnSm = NewSmallIcon;

            /* Update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                if (Class->spicnSm)
                    UserDereferenceObject(Class->spicnSm);
                if (NewSmallIcon)
                    UserReferenceObject(NewSmallIcon);
                if (NewIconFromCache)
                    Class->CSF_flags |= CSF_CACHEDSMICON;
                else
                    Class->CSF_flags &= ~CSF_CACHEDSMICON;
                Class->spicnSm = NewSmallIcon;
                Class = Class->pclsNext;
            }
        }
        break;

        case GCLP_HMODULE:
            Ret = (ULONG_PTR)Class->hModule;
            Class->hModule = (HINSTANCE)NewLong;

           /* Update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                Class->hModule = (HINSTANCE)NewLong;
                Class = Class->pclsNext;
            }
            break;

        case GCLP_MENUNAME:
        {
            PUNICODE_STRING Value = (PUNICODE_STRING)NewLong;

            if (!IntSetClassMenuName(Class,
                                     Value))
            {
                ERR("Setting the class menu name failed!\n");
            }

            /* FIXME: Really return NULL? Wine does so... */
            break;
        }

        case GCL_STYLE:
            Ret = (ULONG_PTR)Class->style;
            Class->style = (UINT)NewLong;

            /* FIXME: What if the CS_GLOBALCLASS style is changed? should we
                      move the class to the appropriate list? For now, we save
                      the original value in Class->Global, so we can always
                      locate the appropriate list */

           /* Update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                Class->style = (UINT)NewLong;
                Class = Class->pclsNext;
            }
            break;

        case GCLP_WNDPROC:
            Ret = (ULONG_PTR)IntSetClassWndProc(Class,
                                                (WNDPROC)NewLong,
                                                Ansi);
            break;

        case GCW_ATOM:
        {
            PUNICODE_STRING Value = (PUNICODE_STRING)NewLong;

            Ret = (ULONG_PTR)Class->atomNVClassName;
            if (!IntSetClassAtom(Class,
                                 Value))
            {
                Ret = 0;
            }
            break;
        }

        default:
            EngSetLastError(ERROR_INVALID_INDEX);
            break;
    }

    return Ret;
}

static BOOL
UserGetClassInfo(IN PCLS Class,
                 OUT PWNDCLASSEXW lpwcx,
                 IN BOOL Ansi,
                 HINSTANCE hInstance)
{
    if (!Class) return FALSE;

    lpwcx->style = Class->style;

    // If fnId is set, clear the global bit. See wine class test check_style.
    if (Class->fnid)
       lpwcx->style &= ~CS_GLOBALCLASS;

    lpwcx->lpfnWndProc = IntGetClassWndProc(Class, Ansi);

    lpwcx->cbClsExtra = Class->cbclsExtra;
    lpwcx->cbWndExtra = Class->cbwndExtra;
    lpwcx->hIcon = Class->spicn ? UserHMGetHandle(Class->spicn) : NULL;
    lpwcx->hCursor = Class->spcur ? UserHMGetHandle(Class->spcur) : NULL;
    lpwcx->hIconSm = Class->spicnSm ? UserHMGetHandle(Class->spicnSm) : NULL;
    lpwcx->hbrBackground = Class->hbrBackground;

    /* Copy non-string to user first. */
    if (Ansi)
       ((PWNDCLASSEXA)lpwcx)->lpszMenuName = Class->lpszClientAnsiMenuName;
    else
       lpwcx->lpszMenuName = Class->lpszClientUnicodeMenuName;
/*
 *  FIXME: CLSMENUNAME has the answers! Copy the already made buffers from there!
 *  Cls: lpszMenuName and lpszAnsiClassName should be used by kernel space.
 *  lpszClientXxxMenuName should already be mapped to user space.
 */
    /* Copy string ptr to user. */
    if ( Class->lpszClientUnicodeMenuName != NULL &&
         Class->MenuNameIsString)
    {
       lpwcx->lpszMenuName = UserHeapAddressToUser(Ansi ?
                                 (PVOID)Class->lpszClientAnsiMenuName :
                                 (PVOID)Class->lpszClientUnicodeMenuName);
    }

    if (hInstance == hModClient)
        lpwcx->hInstance = NULL;
    else
        lpwcx->hInstance = hInstance;

    /* FIXME: Return the string? Okay! This is performed in User32! */
    //lpwcx->lpszClassName = (LPCWSTR)((ULONG_PTR)Class->atomClassName);

    return TRUE;
}

//
// Register System Classes....
//
BOOL
FASTCALL
UserRegisterSystemClasses(VOID)
{
    UINT i;
    UNICODE_STRING ClassName, MenuName;
    PPROCESSINFO ppi = GetW32ProcessInfo();
    WNDCLASSEXW wc;
    PCLS Class;
    BOOL Ret = TRUE;
    HBRUSH hBrush;
    DWORD Flags = 0;

    if (ppi->W32PF_flags & W32PF_CLASSESREGISTERED)
       return TRUE;

    if ( hModClient == NULL)
       return FALSE;

    RtlZeroMemory(&ClassName, sizeof(ClassName));
    RtlZeroMemory(&MenuName, sizeof(MenuName));

    for (i = 0; i != ARRAYSIZE(DefaultServerClasses); i++)
    {
        if (!IS_ATOM(DefaultServerClasses[i].ClassName))
        {
           RtlInitUnicodeString(&ClassName, DefaultServerClasses[i].ClassName);
        }
        else
        {
           ClassName.Buffer = DefaultServerClasses[i].ClassName;
           ClassName.Length = 0;
           ClassName.MaximumLength = 0;
        }

        wc.cbSize = sizeof(wc);
        wc.style = DefaultServerClasses[i].Style;

        Flags |= CSF_SERVERSIDEPROC;

        if (DefaultServerClasses[i].ProcW)
        {
           wc.lpfnWndProc = DefaultServerClasses[i].ProcW;
           wc.hInstance = hModuleWin;
        }
        else
        {
           wc.lpfnWndProc = GETPFNSERVER(DefaultServerClasses[i].fiId);
           wc.hInstance = hModClient;
        }

        wc.cbClsExtra = 0;
        wc.cbWndExtra = DefaultServerClasses[i].ExtraBytes;
        wc.hIcon = NULL;

        //// System Cursors should be initilized!!!
        wc.hCursor = NULL;
        if (DefaultServerClasses[i].hCursor == (HICON)OCR_NORMAL)
        {
            if (SYSTEMCUR(ARROW) == NULL)
            {
                ERR("SYSTEMCUR(ARROW) == NULL, should not happen!!\n");
            }
            else
            {
                wc.hCursor = UserHMGetHandle(SYSTEMCUR(ARROW));
            }
        }

        hBrush = DefaultServerClasses[i].hBrush;
        if (hBrush <= (HBRUSH)COLOR_MENUBAR)
        {
            hBrush = IntGetSysColorBrush(HandleToUlong(hBrush));
        }
        wc.hbrBackground = hBrush;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = ClassName.Buffer;
        wc.hIconSm = NULL;

        Class = IntCreateClass( &wc,
                                &ClassName,
                                &ClassName,
                                &MenuName,
                                 DefaultServerClasses[i].fiId,
                                 Flags,
                                 NULL,
                                 ppi);
        if (Class != NULL)
        {
            Class->pclsNext = ppi->pclsPublicList;
            (void)InterlockedExchangePointer((PVOID*)&ppi->pclsPublicList,
                                             Class);

            ppi->dwRegisteredClasses |= ICLASS_TO_MASK(DefaultServerClasses[i].iCls);
        }
        else
        {
            ERR("!!! Registering system class failed!\n");
            Ret = FALSE;
        }
    }
    if (Ret) ppi->W32PF_flags |= W32PF_CLASSESREGISTERED;
    return Ret;
}

/* SYSCALLS *****************************************************************/

RTL_ATOM
APIENTRY
NtUserRegisterClassExWOW(
    WNDCLASSEXW* lpwcx,
    PUNICODE_STRING ClassName,
    PUNICODE_STRING ClsVersion,
    PCLSMENUNAME pClassMenuName,
    DWORD fnID,
    DWORD Flags,
    LPDWORD pWow)
/*
 * FUNCTION:
 *   Registers a new class with the window manager
 * ARGUMENTS:
 *   lpwcx          = Win32 extended window class structure
 *   bUnicodeClass = Whether to send ANSI or unicode strings
 *                   to window procedures
 * RETURNS:
 *   Atom identifying the new class
 */
{
    WNDCLASSEXW CapturedClassInfo = {0};
    UNICODE_STRING CapturedName = {0}, CapturedMenuName = {0}, CapturedVersion = {0};
    RTL_ATOM Ret = (RTL_ATOM)0;
    PPROCESSINFO ppi = GetW32ProcessInfo();
    BOOL Exception = FALSE;

    if (Flags & ~(CSF_ANSIPROC))
    {
        ERR("NtUserRegisterClassExWOW Bad Flags!\n");
        EngSetLastError(ERROR_INVALID_FLAGS);
        return Ret;
    }

    UserEnterExclusive();

    TRACE("NtUserRegisterClassExWOW ClsN %wZ\n",ClassName);

    if ( !(ppi->W32PF_flags & W32PF_CLASSESREGISTERED ))
    {
       UserRegisterSystemClasses();
    }

    _SEH2_TRY
    {
        /* Probe the parameters and basic parameter checks */
        if (ProbeForReadUint(&lpwcx->cbSize) != sizeof(WNDCLASSEXW))
        {
            ERR("NtUserRegisterClassExWOW Wrong cbSize!\n");
            goto InvalidParameter;
        }

        ProbeForRead(lpwcx,
                     sizeof(WNDCLASSEXW),
                     sizeof(ULONG));
        RtlCopyMemory(&CapturedClassInfo,
                      lpwcx,
                      sizeof(WNDCLASSEXW));

        CapturedName = ProbeForReadUnicodeString(ClassName);
        CapturedVersion = ProbeForReadUnicodeString(ClsVersion);

        ProbeForRead(pClassMenuName,
                     sizeof(CLSMENUNAME),
                     1);

        CapturedMenuName = ProbeForReadUnicodeString(pClassMenuName->pusMenuName);

        if ( (CapturedName.Length & 1) ||
             (CapturedMenuName.Length & 1) ||
             (CapturedClassInfo.cbClsExtra < 0) ||
             ((CapturedClassInfo.cbClsExtra + CapturedName.Length +
              CapturedMenuName.Length +  sizeof(CLS))
                < (ULONG)CapturedClassInfo.cbClsExtra) ||
             (CapturedClassInfo.cbWndExtra < 0) ||
             (CapturedClassInfo.hInstance == NULL) )
        {
            ERR("NtUserRegisterClassExWOW Invalid Parameter Error!\n");
            goto InvalidParameter;
        }

        if (CapturedName.Length != 0)
        {
            ProbeForRead(CapturedName.Buffer,
                         CapturedName.Length,
                         sizeof(WCHAR));
        }
        else
        {
            if (!IS_ATOM(CapturedName.Buffer))
            {
                ERR("NtUserRegisterClassExWOW ClassName Error!\n");
                goto InvalidParameter;
            }
        }

        if (CapturedVersion.Length != 0)
        {
            ProbeForRead(CapturedVersion.Buffer,
                         CapturedVersion.Length,
                         sizeof(WCHAR));
        }
        else
        {
            if (!IS_ATOM(CapturedVersion.Buffer))
            {
                ERR("NtUserRegisterClassExWOW ClassName Error!\n");
                goto InvalidParameter;
            }
        }

        if (CapturedMenuName.Length != 0)
        {
            ProbeForRead(CapturedMenuName.Buffer,
                         CapturedMenuName.Length,
                         sizeof(WCHAR));
        }
        else if (CapturedMenuName.Buffer != NULL &&
                 !IS_INTRESOURCE(CapturedMenuName.Buffer))
        {
            ERR("NtUserRegisterClassExWOW MenuName Error!\n");
InvalidParameter:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            _SEH2_LEAVE;
        }

        if (IsCallProcHandle(lpwcx->lpfnWndProc))
        {  // Never seen this yet, but I'm sure it's a little haxxy trick!
           // If this pops up we know what todo!
           ERR("NtUserRegisterClassExWOW WndProc is CallProc!!\n");
        }

        TRACE("NtUserRegisterClassExWOW MnuN %wZ\n",&CapturedMenuName);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("NtUserRegisterClassExWOW Exception Error!\n");
        SetLastNtError(_SEH2_GetExceptionCode());
        Exception = TRUE;
    }
    _SEH2_END;

    if (!Exception)
    {
        /* Register the class */
        Ret = UserRegisterClass(&CapturedClassInfo,
                                &CapturedName,
                                &CapturedVersion,
                                &CapturedMenuName,
                                fnID,
                                Flags);
    }

    if (!Ret)
    {
       TRACE("NtUserRegisterClassExWOW Null Return!\n");
    }

    UserLeave();

    return Ret;
}

ULONG_PTR APIENTRY
IntNtUserSetClassLongPtr(HWND hWnd,
                   INT Offset,
                   ULONG_PTR dwNewLong,
                   BOOL Ansi,
                   ULONG Size)
{
    PPROCESSINFO pi;
    PWND Window;
    ULONG_PTR Ret = 0;

    UserEnterExclusive();

    pi = GetW32ProcessInfo();

    Window = UserGetWindowObject(hWnd);
    if (Window != NULL)
    {
        if (Window->head.pti->ppi != pi)
        {
            EngSetLastError(ERROR_ACCESS_DENIED);
            goto Cleanup;
        }

        _SEH2_TRY
        {
            UNICODE_STRING Value;

            /* Probe the parameters */
            if (Offset == GCW_ATOM || Offset == GCLP_MENUNAME)
            {
                /* FIXME: Resource ID can be passed directly without UNICODE_STRING ? */
                if (IS_ATOM(dwNewLong))
                {
                    Value.MaximumLength = 0;
                    Value.Length = 0;
                    Value.Buffer = (PWSTR)dwNewLong;
                }
                else
                {
                    Value = ProbeForReadUnicodeString((PUNICODE_STRING)dwNewLong);
                }

                if (Value.Length & 1)
                {
                    goto InvalidParameter;
                }

                if (Value.Length != 0)
                {
                    ProbeForRead(Value.Buffer,
                                 Value.Length,
                                 sizeof(WCHAR));
                }
                else
                {
                    if (Offset == GCW_ATOM && !IS_ATOM(Value.Buffer))
                    {
                        goto InvalidParameter;
                    }
                    else if (Offset == GCLP_MENUNAME && !IS_INTRESOURCE(Value.Buffer))
                    {
InvalidParameter:
                        EngSetLastError(ERROR_INVALID_PARAMETER);
                        _SEH2_LEAVE;
                    }
                }

                dwNewLong = (ULONG_PTR)&Value;
            }

            Ret = UserSetClassLongPtr(Window->pcls,
                                      Offset,
                                      dwNewLong,
                                      Ansi,
                                      Size);
            switch(Offset)
            {
               case GCLP_HICONSM:
               case GCLP_HICON:
               {
                  if (Ret && Ret != dwNewLong)
                     UserPaintCaption(Window, DC_ICON);
               }
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

Cleanup:
    UserLeave();

    return Ret;
}

ULONG_PTR
APIENTRY
NtUserSetClassLong(
    _In_ HWND hWnd,
    _In_ INT Offset,
    _In_ ULONG dwNewLong,
    _In_ BOOL Ansi)
{
    return IntNtUserSetClassLongPtr(hWnd, Offset, dwNewLong, Ansi, sizeof(LONG));
}

#ifdef _WIN64

ULONG_PTR
APIENTRY
NtUserSetClassLongPtr(
    _In_ HWND hWnd,
    _In_ INT Offset,
    _In_ ULONG_PTR dwNewLong,
    _In_ BOOL Ansi)
{
    return IntNtUserSetClassLongPtr(hWnd, Offset, dwNewLong, Ansi, sizeof(LONG_PTR));
}

#endif // _WIN64

WORD
APIENTRY
NtUserSetClassWord(
  HWND hWnd,
  INT nIndex,
  WORD wNewWord)
{
/*
 * NOTE: Obsoleted in 32-bit windows
 */
   return(0);
}

BOOL
APIENTRY
NtUserUnregisterClass(
    IN PUNICODE_STRING ClassNameOrAtom,
    IN HINSTANCE hInstance,
    OUT PCLSMENUNAME pClassMenuName)
{
    UNICODE_STRING SafeClassName;
    NTSTATUS Status;
    BOOL Ret;

    Status = ProbeAndCaptureUnicodeStringOrAtom(&SafeClassName, ClassNameOrAtom);
    if (!NT_SUCCESS(Status))
    {
        ERR("Error capturing the class name\n");
        SetLastNtError(Status);
        return FALSE;
    }

    UserEnterExclusive();

    /* Unregister the class */
    Ret = UserUnregisterClass(&SafeClassName, hInstance, NULL); // Null for now~

    UserLeave();

    if (SafeClassName.Buffer && !IS_ATOM(SafeClassName.Buffer))
        ExFreePoolWithTag(SafeClassName.Buffer, TAG_STRING);

    return Ret;
}


/* NOTE: For system classes hInstance is not NULL here, but User32Instance */
BOOL
APIENTRY
NtUserGetClassInfo(
   HINSTANCE hInstance,
   PUNICODE_STRING ClassName,
   LPWNDCLASSEXW lpWndClassEx,
   LPWSTR *ppszMenuName,
   BOOL bAnsi)
{
    UNICODE_STRING SafeClassName;
    WNDCLASSEXW Safewcexw;
    PCLS Class;
    RTL_ATOM ClassAtom = 0;
    PPROCESSINFO ppi;
    BOOL Ret = TRUE;
    NTSTATUS Status;

    _SEH2_TRY
    {
        ProbeForWrite( lpWndClassEx, sizeof(WNDCLASSEXW), sizeof(ULONG));
        RtlCopyMemory( &Safewcexw, lpWndClassEx, sizeof(WNDCLASSEXW));
        if (ppszMenuName)
        {
            ProbeForWrite(ppszMenuName, sizeof(*ppszMenuName), sizeof(PVOID));
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    Status = ProbeAndCaptureUnicodeStringOrAtom(&SafeClassName, ClassName);
    if (!NT_SUCCESS(Status))
    {
        ERR("Error capturing the class name\n");
        SetLastNtError(Status);
        return FALSE;
    }

    // If null instance use client.
    if (!hInstance) hInstance = hModClient;

    TRACE("GetClassInfo(%wZ, %p)\n", &SafeClassName, hInstance);

    /* NOTE: Need exclusive lock because getting the wndproc might require the
             creation of a call procedure handle */
    UserEnterExclusive();

    ppi = GetW32ProcessInfo();
    if (!(ppi->W32PF_flags & W32PF_CLASSESREGISTERED))
    {
        UserRegisterSystemClasses();
    }

    ClassAtom = IntGetClassAtom(&SafeClassName,
                                hInstance,
                                ppi,
                                &Class,
                                NULL);
    if (ClassAtom != (RTL_ATOM)0)
    {
        ClassAtom = Class->atomNVClassName;
        Ret = UserGetClassInfo(Class, &Safewcexw, bAnsi, hInstance);
    }
    else
    {
        EngSetLastError(ERROR_CLASS_DOES_NOT_EXIST);
        Ret = FALSE;
    }

    UserLeave();

    if (Ret)
    {
        _SEH2_TRY
        {
            /* Emulate Function. */
            if (ppszMenuName) *ppszMenuName = (LPWSTR)Safewcexw.lpszMenuName;

            RtlCopyMemory(lpWndClassEx, &Safewcexw, sizeof(WNDCLASSEXW));

            // From Wine:
            /* We must return the atom of the class here instead of just TRUE. */
            /* Undocumented behavior! Return the class atom as a BOOL! */
            Ret = (BOOL)ClassAtom;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            EngSetLastError(ERROR_CLASS_DOES_NOT_EXIST);
            Ret = FALSE;
        }
        _SEH2_END;
    }

    if (!IS_ATOM(SafeClassName.Buffer))
        ExFreePoolWithTag(SafeClassName.Buffer, TAG_STRING);

    return Ret;
}


INT APIENTRY
NtUserGetClassName (IN HWND hWnd,
                    IN BOOL Real,
                    OUT PUNICODE_STRING ClassName)
{
    PWND Window;
    UNICODE_STRING CapturedClassName;
    INT iCls, Ret = 0;
    RTL_ATOM Atom = 0;

    UserEnterShared();

    Window = UserGetWindowObject(hWnd);
    if (Window != NULL)
    {
        if (Real && Window->fnid && !(Window->fnid & FNID_DESTROY))
        {
           if (LookupFnIdToiCls(Window->fnid, &iCls))
           {
              Atom = gpsi->atomSysClass[iCls];
           }
        }

        _SEH2_TRY
        {
            ProbeForWriteUnicodeString(ClassName);
            CapturedClassName = *ClassName;
            if (CapturedClassName.Length != 0)
            {
                ProbeForRead(CapturedClassName.Buffer,
                             CapturedClassName.Length,
                             sizeof(WCHAR));
            }

            /* Get the class name */
            Ret = UserGetClassName(Window->pcls,
                                   &CapturedClassName,
                                   Atom,
                                   FALSE);

            if (Ret != 0)
            {
                /* Update the Length field */
                ClassName->Length = CapturedClassName.Length;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    UserLeave();

    return Ret;
}

/* Return Pointer to Class structure. */
PCLS
APIENTRY
NtUserGetWOWClass(
    HINSTANCE hInstance,
    PUNICODE_STRING ClassName)
{
    UNICODE_STRING SafeClassName;
    PPROCESSINFO pi;
    PCLS Class = NULL;
    RTL_ATOM ClassAtom = 0;
    NTSTATUS Status;

    Status = ProbeAndCaptureUnicodeStringOrAtom(&SafeClassName, ClassName);
    if (!NT_SUCCESS(Status))
    {
        ERR("Error capturing the class name\n");
        SetLastNtError(Status);
        return FALSE;
    }

    UserEnterExclusive();

    pi = GetW32ProcessInfo();

    ClassAtom = IntGetClassAtom(&SafeClassName,
                                hInstance,
                                pi,
                                &Class,
                                NULL);
    if (!ClassAtom)
    {
        EngSetLastError(ERROR_CLASS_DOES_NOT_EXIST);
    }


    if (SafeClassName.Buffer && !IS_ATOM(SafeClassName.Buffer))
        ExFreePoolWithTag(SafeClassName.Buffer, TAG_STRING);

    UserLeave();
//
// Don't forget to use DesktopPtrToUser( ? ) with return pointer in user space.
//
    return Class;
}

/* EOF */
