/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Thomas Weidenmueller <w3seek@reactos.com>
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>
#define TRACE DPRINT
#define WARN DPRINT1
#define ERR DPRINT1

PCLS SystemClassList = NULL;
BOOL RegisteredSysClasses = FALSE;

static struct
{
    int FnId;
    int ClsId;
}  FnidToiCls[] =
{
 { FNID_BUTTON,    ICLS_BUTTON},
 { FNID_EDIT,      ICLS_EDIT}, 
 { FNID_STATIC,    ICLS_STATIC},
 { FNID_LISTBOX,   ICLS_LISTBOX},
 { FNID_SCROLLBAR, ICLS_SCROLLBAR},
 { FNID_COMBOBOX,  ICLS_COMBOBOX},
 { FNID_MDICLIENT, ICLS_MDICLIENT},
 { FNID_COMBOLBOX, ICLS_COMBOLBOX},
 { FNID_DIALOG,    ICLS_DIALOG},  
 { FNID_MENU,      ICLS_MENU},
 { FNID_ICONTITLE, ICLS_ICONTITLE}
};

static 
BOOL
FASTCALL
LockupFnIdToiCls(int FnId, int *iCls )
{
  int i;
  
  for ( i = 0; i < (sizeof(FnidToiCls)/2)/sizeof(int); i++)
  {
     if (FnidToiCls[i].FnId == FnId)
     {
        *iCls = FnidToiCls[i].ClsId;
        return TRUE;
     }
  }
  return FALSE;
}

/* WINDOWCLASS ***************************************************************/

static VOID
IntFreeClassMenuName(IN OUT PCLS Class)
{
    /* free the menu name, if it was changed and allocated */
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
    /* there shouldn't be any clones anymore */
    ASSERT(Class->cWndReferenceCount == 0);
    ASSERT(Class->pclsClone == NULL);

    if (Class->pclsBase == Class)
    {
        PCALLPROC CallProc, NextCallProc;

        /* Destroy allocated callproc handles */
        CallProc = Class->spcpdFirst;
        while (CallProc != NULL)
        {
            NextCallProc = CallProc->Next;

            CallProc->Next = NULL;
            DestroyCallProc(NULL,
                            CallProc);

            CallProc = NextCallProc;
        }

        if (Class->pdce)
        {
           DceFreeClassDCE(((PDCE)Class->pdce)->hDC);
           Class->pdce = NULL;
        }

        IntFreeClassMenuName(Class);
    }

    /* free the structure */
    if (Class->rpdeskParent != NULL)
    {
        DesktopHeapFree(Class->rpdeskParent,
                        Class);
    }
    else
    {
        UserHeapFree(Class);
    }
}


/* clean all process classes. all process windows must cleaned first!! */
void FASTCALL DestroyProcessClasses(PW32PROCESS Process )
{
    PCLS Class;
    PPROCESSINFO pi = (PPROCESSINFO)Process;
     
    if (pi != NULL)
    {
        /* free all local classes */
        Class = pi->pclsPrivateList;
        while (Class != NULL)
        {
            pi->pclsPrivateList = Class->pclsNext;

            ASSERT(Class->pclsBase == Class);
            IntDestroyClass(Class);

            Class = pi->pclsPrivateList;
        }

        /* free all global classes */
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
    PWSTR AtomName;
    NTSTATUS Status;

    if (ClassName->Length != 0)
    {
        /* FIXME - Don't limit to 64 characters! use SEH when allocating memory! */
        if (ClassName->Length / sizeof(WCHAR) >= sizeof(szBuf) / sizeof(szBuf[0]))
        {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return (RTL_ATOM)0;
        }

        RtlCopyMemory(szBuf,
                      ClassName->Buffer,
                      ClassName->Length);
        szBuf[ClassName->Length / sizeof(WCHAR)] = UNICODE_NULL;
        AtomName = szBuf;
    }
    else
        AtomName = ClassName->Buffer;

  /* If an Atom, need to verify, if it was already added to the global atom list. */
    if (IS_ATOM(AtomName))
    {
       *pAtom = (RTL_ATOM)((ULONG_PTR)AtomName);
       Status = RtlQueryAtomInAtomTable( gAtomTable,
                                         *pAtom,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL);

       if (NT_SUCCESS(Status)) return TRUE;
    }

    Status = RtlAddAtomToAtomTable(gAtomTable,
                                   AtomName,
                                   pAtom);

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }

    return TRUE;
}

static NTSTATUS
IntDeregisterClassAtom(IN RTL_ATOM Atom)
{
    return RtlDeleteAtomFromAtomTable(gAtomTable,
                                      Atom);
}

PCALLPROC
UserFindCallProc(IN PCLS Class,
                 IN WNDPROC WndProc,
                 IN BOOL bUnicode)
{
    PCALLPROC CallProc;

    CallProc = Class->spcpdFirst;
    while (CallProc != NULL)
    {
        if (CallProc->WndProc == WndProc &&
            CallProc->Unicode == (UINT)bUnicode)
        {
            return CallProc;
        }

        CallProc = CallProc->Next;
    }

    return NULL;
}

VOID
UserAddCallProcToClass(IN OUT PCLS Class,
                       IN PCALLPROC CallProc)
{
    PCLS BaseClass;

    ASSERT(CallProc->Next == NULL);

    BaseClass = Class->pclsBase;
    ASSERT(CallProc->Next == NULL);
    CallProc->Next = BaseClass->spcpdFirst;
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

    /* update the base class first */
    Class = Class->pclsBase;

    if (!IntRegisterClassAtom(ClassName,
                              &Atom))
    {
        return FALSE;
    }

    IntDeregisterClassAtom(Class->atomClassName);

    Class->atomClassName = Atom;

    /* update the clones */
    Class = Class->pclsClone;
    while (Class != NULL)
    {
        Class->atomClassName = Atom;

        Class = Class->pclsNext;
    }

    return TRUE;
}

static WNDPROC
IntGetClassWndProc(IN PCLS Class,
                   IN PPROCESSINFO pi,
                   IN BOOL Ansi)
{
    ASSERT(UserIsEnteredExclusive() == TRUE);

    if (Class->System)
    {
        return (Ansi ? Class->WndProcExtra : Class->lpfnWndProc);
    }
    else
    {
        if (!Ansi == Class->Unicode)
        {
            return Class->lpfnWndProc;
        }
        else
        {
            PCLS BaseClass;

            /* make sure the call procedures are located on the desktop
               of the base class! */
            BaseClass = Class->pclsBase;
            Class = BaseClass;

            if (Class->CallProc != NULL)
            {
                return GetCallProcHandle(Class->CallProc);
            }
            else
            {
                PCALLPROC NewCallProc;

                if (pi == NULL)
                    return NULL;

                NewCallProc = UserFindCallProc(Class,
                                               Class->lpfnWndProc,
                                               Class->Unicode);
                if (NewCallProc == NULL)
                {
                    NewCallProc = CreateCallProc(NULL,
                                                 Class->lpfnWndProc,
                                                 Class->Unicode,
                                                 pi);
                    if (NewCallProc == NULL)
                    {
                        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
                        return NULL;
                    }

                    UserAddCallProcToClass(Class,
                                           NewCallProc);
                }

                Class->CallProc = NewCallProc;

                /* update the clones */
                Class = Class->pclsClone;
                while (Class != NULL)
                {
                    Class->CallProc = NewCallProc;

                    Class = Class->pclsNext;
                }

                return GetCallProcHandle(NewCallProc);
            }
        }
    }
}

static WNDPROC
IntSetClassWndProc(IN OUT PCLS Class,
                   IN WNDPROC WndProc,
                   IN BOOL Ansi)
{
    WNDPROC Ret;

    if (Class->System)
    {
        DPRINT1("Attempted to change window procedure of system window class 0x%p!\n", Class->atomClassName);
        SetLastWin32Error(ERROR_ACCESS_DENIED);
        return NULL;
    }

    /* update the base class first */
    Class = Class->pclsBase;

    /* resolve any callproc handle if possible */
    if (IsCallProcHandle(WndProc))
    {
        WNDPROC_INFO wpInfo;

        if (UserGetCallProcInfo((HANDLE)WndProc,
                                &wpInfo))
        {
            WndProc = wpInfo.WindowProc;
            /* FIXME - what if wpInfo.IsUnicode doesn't match Ansi? */
        }
    }

    Ret = IntGetClassWndProc(Class,
                             GetW32ProcessInfo(),
                             Ansi);
    if (Ret == NULL)
    {
        return NULL;
    }

    /* update the class info */
    Class->Unicode = !Ansi;
    Class->lpfnWndProc = WndProc;

    /* update the clones */
    Class = Class->pclsClone;
    while (Class != NULL)
    {
        Class->Unicode = !Ansi;
        Class->lpfnWndProc = WndProc;

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
        /* it is most likely that a window is created on the same
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
            /* simply clone the class */
            RtlCopyMemory(Class,
                          BaseClass,
                          ClassSize);

            /* update some pointers and link the class */
            Class->rpdeskParent = Desktop;
            Class->cWndReferenceCount = 0;

            if (BaseClass->rpdeskParent == NULL)
            {
                /* we don't really need the base class on the shared
                   heap anymore, delete it so the only class left is
                   the clone we just created, which now serves as the
                   new base class */
                ASSERT(BaseClass->pclsClone == NULL);
                ASSERT(Class->pclsClone == NULL);
                Class->pclsBase = Class;
                Class->pclsNext = BaseClass->pclsNext;

                /* replace the base class */
                (void)InterlockedExchangePointer((PVOID*)ClassLink,
                                                 Class);

                /* destroy the obsolete copy on the shared heap */
                BaseClass->pclsBase = NULL;
                BaseClass->pclsClone = NULL;
                IntDestroyClass(BaseClass);
            }
            else
            {
                /* link in the clone */
                Class->pclsClone = NULL;
                Class->pclsBase = BaseClass;
                Class->pclsNext = BaseClass->pclsClone;
                (void)InterlockedExchangePointer(&BaseClass->pclsClone,
                                                 Class);
            }
        }
        else
        {
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
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

    Class = IntGetClassForDesktop(BaseClass,
                                  ClassLink,
                                  Desktop);
    if (Class != NULL)
    {
        Class->cWndReferenceCount++;
    }

    return Class;
}

static VOID
IntMakeCloneBaseClass(IN OUT PCLS Class,
                      IN OUT PCLS *BaseClassLink,
                      IN OUT PCLS *CloneLink)
{
    PCLS Clone, BaseClass;

    ASSERT(Class->pclsBase != Class);
    ASSERT(Class->pclsBase->pclsClone != NULL);
    ASSERT(Class->rpdeskParent != NULL);
    ASSERT(Class->cWndReferenceCount != 0);
    ASSERT(Class->pclsBase->rpdeskParent != NULL);
    ASSERT(Class->pclsBase->cWndReferenceCount == 0);

    /* unlink the clone */
    *CloneLink = Class->pclsNext;
    Class->pclsClone = Class->pclsBase->pclsClone;

    BaseClass = Class->pclsBase;

    /* update the class information to make it a base class */
    Class->pclsBase = Class;
    Class->pclsNext = (*BaseClassLink)->pclsNext;

    /* update all clones */
    Clone = Class->pclsClone;
    while (Clone != NULL)
    {
        ASSERT(Clone->pclsClone == NULL);
        Clone->pclsBase = Class;

        if (!Class->System)
            Clone->CallProc = Class->CallProc;

        Clone = Clone->pclsNext;
    }

    /* link in the new base class */
    (void)InterlockedExchangePointer((PVOID*)BaseClassLink,
                                     Class);
}

VOID
IntDereferenceClass(IN OUT PCLS Class,
                    IN PDESKTOPINFO Desktop,
                    IN PPROCESSINFO pi)
{
    PCLS *PrevLink, BaseClass, CurrentClass;

    BaseClass = Class->pclsBase;

    if (--Class->cWndReferenceCount == 0)
    {
        if (BaseClass == Class)
        {
            ASSERT(Class->pclsBase == Class);

            /* check if there are clones of the class on other desktops,
               link the first clone in if possible. If there are no clones
               then leave the class on the desktop heap. It will get moved
               to the shared heap when the thread detaches. */
            if (BaseClass->pclsClone != NULL)
            {
                if (BaseClass->System)
                    PrevLink = &SystemClassList;
                else if (BaseClass->Global)
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

                /* make the first clone become the new base class */
                IntMakeCloneBaseClass(BaseClass->pclsClone,
                                      PrevLink,
                                      &BaseClass->pclsClone);

                /* destroy the class, there's still another clone of the class
                   that now serves as a base class. Make sure we don't destruct
                   resources shared by all classes (Base = NULL)! */
                BaseClass->pclsBase = NULL;
                BaseClass->pclsClone = NULL;
                IntDestroyClass(BaseClass);
            }
        }
        else
        {
            /* locate the cloned class and unlink it */
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

            /* the class was just a clone, we don't need it anymore */
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

    /* allocate the new base class on the shared heap */
    NewClass = UserHeapAlloc(ClassSize);
    if (NewClass != NULL)
    {
        RtlCopyMemory(NewClass,
                      Class,
                      ClassSize);

        NewClass->rpdeskParent = NULL;
        NewClass->pclsBase = NewClass;

        /* replace the class in the list */
        (void)InterlockedExchangePointer((PVOID*)*ClassLinkPtr,
                                         NewClass);
        *ClassLinkPtr = &NewClass->pclsNext;

        /* free the obsolete class on the desktop heap */
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
            /* there shouldn't be any clones around anymore! */
            ASSERT(Class->pclsClone == NULL);

            /* FIXME - If process is terminating, don't move the class but rather destroy it! */
            /* FIXME - We could move the class to another desktop heap if there's still desktops
                       mapped into the process... */

            /* move the class to the shared heap */
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
                    /* unlink the base class */
                    (void)InterlockedExchangePointer((PVOID*)Link,
                                                     Class->pclsNext);

                    /* we can free the old base class now */
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
    if (pi == NULL)
        return TRUE;

    /* check all local classes */
    IntCheckDesktopClasses(Desktop,
                           &pi->pclsPrivateList,
                           FreeOnFailure,
                           &Ret);

    /* check all global classes */
    IntCheckDesktopClasses(Desktop,
                           &pi->pclsPublicList,
                           FreeOnFailure,
                           &Ret);

    /* check all system classes */
    IntCheckDesktopClasses(Desktop,
                           &SystemClassList,
                           FreeOnFailure,
                           &Ret);

    if (!Ret)
    {
        DPRINT1("Failed to move process classes from desktop 0x%p to the shared heap!\n", Desktop);
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    }

    return Ret;
}

PCLS
FASTCALL
IntCreateClass(IN CONST WNDCLASSEXW* lpwcx,
               IN PUNICODE_STRING ClassName,
               IN PUNICODE_STRING MenuName,
               IN WNDPROC wpExtra,
               IN DWORD dwFlags,
               IN PDESKTOP Desktop,
               IN PPROCESSINFO pi)
{
    SIZE_T ClassSize;
    PCLS Class = NULL;
    RTL_ATOM Atom;
    PWSTR pszMenuName = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("lpwcx=%p ClassName=%wZ MenuName=%wZ wpExtra=%p dwFlags=%08x Desktop=%p pi=%p\n",
        lpwcx, ClassName, MenuName, wpExtra, dwFlags, Desktop, pi);

    if (!IntRegisterClassAtom(ClassName,
                              &Atom))
    {
        DPRINT1("Failed to register class atom!\n");
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
        /* FIXME - the class was created before being connected
                   to a desktop. It is possible for the desktop window,
                   but should it be allowed for any other case? */
        Class = UserHeapAlloc(ClassSize);
    }

    if (Class != NULL)
    {
        RtlZeroMemory(Class,
                      ClassSize);

        Class->rpdeskParent = Desktop;
        Class->pclsBase = Class;
        Class->atomClassName = Atom;

        Class->CSF_flags = dwFlags;

        if (dwFlags & CSF_SYSTEMCLASS)
        {
            dwFlags &= ~CSF_ANSIPROC;
            Class->WndProcExtra = wpExtra;
            Class->System = TRUE;
        }

        _SEH2_TRY
        {
            PWSTR pszMenuNameBuffer = pszMenuName;

            /* need to protect with SEH since accessing the WNDCLASSEX structure
               and string buffers might raise an exception! We don't want to
               leak memory... */
            Class->lpfnWndProc = lpwcx->lpfnWndProc;
            Class->style = lpwcx->style;
            Class->cbclsExtra = lpwcx->cbClsExtra;
            Class->cbwndExtra = lpwcx->cbWndExtra;
            Class->hModule = lpwcx->hInstance;
            Class->hIcon = lpwcx->hIcon; /* FIXME */
            Class->hIconSm = lpwcx->hIconSm; /* FIXME */
            Class->hCursor = lpwcx->hCursor; /* FIXME */
            Class->hbrBackground = lpwcx->hbrBackground;


            /* make a copy of the string */
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

            /* save an ansi copy of the string */
            if (pszMenuNameBuffer != NULL)
            {
                ANSI_STRING AnsiString;

                Class->lpszClientAnsiMenuName = (PSTR)pszMenuNameBuffer;
                AnsiString.MaximumLength = RtlUnicodeStringToAnsiSize(MenuName);
                AnsiString.Buffer = Class->lpszClientAnsiMenuName;
                Status = RtlUnicodeStringToAnsiString(&AnsiString,
                                                      MenuName,
                                                      FALSE);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to convert unicode menu name to ansi!\n");

                    /* life would've been much prettier if ntoskrnl exported RtlRaiseStatus()... */
                    _SEH2_LEAVE;
                }
            }
            else
                Class->lpszClientAnsiMenuName = (PSTR)MenuName->Buffer;

            /* Save kernel use menu name and ansi class name */
            Class->lpszMenuName = Class->lpszClientUnicodeMenuName; // Fixme!
            //Class->lpszAnsiClassName = Fixme!

            if (!(dwFlags & CSF_ANSIPROC))
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
            DPRINT1("Failed creating the class: 0x%x\n", Status);

            SetLastNtError(Status);

            if (pszMenuName != NULL)
                UserHeapFree(pszMenuName);

            DesktopHeapFree(Desktop,
                            Class);
            Class = NULL;

            IntDeregisterClassAtom(Atom);
        }
    }
    else
    {
NoMem:
        DPRINT1("Failed to allocate class on Desktop 0x%p\n", Desktop);

        if (pszMenuName != NULL)
            UserHeapFree(pszMenuName);

        IntDeregisterClassAtom(Atom);

        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    }

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
            !Class->Destroying)
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

BOOL
IntGetAtomFromStringOrAtom(IN PUNICODE_STRING ClassName,
                           OUT RTL_ATOM *Atom)
{
    BOOL Ret = FALSE;

    if (ClassName->Length != 0)
    {
        WCHAR szBuf[65];
        PWSTR AtomName;
        NTSTATUS Status;

        /* NOTE: Caller has to protect the call with SEH! */

        if (ClassName->Length != 0)
        {
            /* FIXME - Don't limit to 64 characters! use SEH when allocating memory! */
            if (ClassName->Length / sizeof(WCHAR) >= sizeof(szBuf) / sizeof(szBuf[0]))
            {
                SetLastWin32Error(ERROR_INVALID_PARAMETER);
                return (RTL_ATOM)0;
            }

            /* We need to make a local copy of the class name! The caller could
               modify the buffer and we could overflow in RtlLookupAtomInAtomTable.
               We're protected by SEH, but the ranges that might be accessed were
               not probed... */
            RtlCopyMemory(szBuf,
                          ClassName->Buffer,
                          ClassName->Length);
            szBuf[ClassName->Length / sizeof(WCHAR)] = UNICODE_NULL;
            AtomName = szBuf;
        }
        else
            AtomName = ClassName->Buffer;

        /* lookup the atom */
        Status = RtlLookupAtomInAtomTable(gAtomTable,
                                          AtomName,
                                          Atom);
        if (NT_SUCCESS(Status))
        {
            Ret = TRUE;
        }
        else
        {
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                SetLastWin32Error(ERROR_CANNOT_FIND_WND_CLASS);
            }
            else
            {
                SetLastNtError(Status);
            }
        }
    }
    else
    {
        ASSERT(IS_ATOM(ClassName->Buffer));
        *Atom = (RTL_ATOM)((ULONG_PTR)ClassName->Buffer);
        Ret = TRUE;
    }

    return Ret;
}

RTL_ATOM
IntGetClassAtom(IN PUNICODE_STRING ClassName,
                IN HINSTANCE hInstance  OPTIONAL,
                IN PPROCESSINFO pi  OPTIONAL,
                OUT PCLS *BaseClass  OPTIONAL,
                OUT PCLS **Link  OPTIONAL)
{
    RTL_ATOM Atom = (RTL_ATOM)0;

    ASSERT(BaseClass != NULL);

    if (IntGetAtomFromStringOrAtom(ClassName,
                                   &Atom) &&
        Atom != (RTL_ATOM)0)
    {
        PCLS Class;

        /* attempt to locate the class object */

        ASSERT(pi != NULL);

        /* Step 1: try to find an exact match of locally registered classes */
        Class = IntFindClass(Atom,
                             hInstance,
                             &pi->pclsPrivateList,
                             Link);
        if (Class != NULL)
        {
            goto FoundClass;
        }

        /* Step 2: try to find any globally registered class. The hInstance
                   is not relevant for global classes */
        Class = IntFindClass(Atom,
                             NULL,
                             &pi->pclsPublicList,
                             Link);
        if (Class != NULL)
        {
            goto FoundClass;
        }

        /* Step 3: try to find any local class registered by user32 */
        Class = IntFindClass(Atom,
                             hModClient,
                             &pi->pclsPrivateList,
                             Link);
        if (Class != NULL)
        {
            goto FoundClass;
        }

        /* Step 4: try to find any global class registered by user32 */
        Class = IntFindClass(Atom,
                             hModClient,
                             &pi->pclsPublicList,
                             Link);
        if (Class != NULL)
        {
            goto FoundClass;
        }

        /* Step 5: try to find a system class */
        Class = IntFindClass(Atom,
                             NULL,
                             &SystemClassList,
                             Link);
        if (Class == NULL)
        {
            SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
            return (RTL_ATOM)0;
        }

FoundClass:
        *BaseClass = Class;
    }

    return Atom;
}

RTL_ATOM
UserRegisterClass(IN CONST WNDCLASSEXW* lpwcx,
                  IN PUNICODE_STRING ClassName,
                  IN PUNICODE_STRING MenuName,
                  IN WNDPROC wpExtra,
                  IN DWORD dwFlags)
{
    PTHREADINFO pti;
    PW32THREADINFO ti;
    PPROCESSINFO pi;
    PCLS Class;
    RTL_ATOM ClassAtom;
    RTL_ATOM Ret = (RTL_ATOM)0;

    /* NOTE: Accessing the buffers in ClassName and MenuName may raise exceptions! */

    pti = PsGetCurrentThreadWin32Thread();
    ti = GetW32ThreadInfo();
    if (ti == NULL || !RegisteredSysClasses)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return (RTL_ATOM)0;
    }

    pi = ti->ppi;

    /* try to find a previously registered class */
    ClassAtom = IntGetClassAtom(ClassName,
                                lpwcx->hInstance,
                                pi,
                                &Class,
                                NULL);
    if (ClassAtom != (RTL_ATOM)0)
    {
        if (lpwcx->style & CS_GLOBALCLASS)
        {
            // global classes shall not have same names as system classes
            if (Class->Global || Class->System)
            {
                DPRINT("Class 0x%p does already exist!\n", ClassAtom);
                SetLastWin32Error(ERROR_CLASS_ALREADY_EXISTS);
                return (RTL_ATOM)0;
            }
        }
        else if ( !Class->Global && !Class->System)
        {
            // local class already exists
            DPRINT("Class 0x%p does already exist!\n", ClassAtom);
            SetLastWin32Error(ERROR_CLASS_ALREADY_EXISTS);
            return (RTL_ATOM)0;
        }
    }

    Class = IntCreateClass(lpwcx,
                           ClassName,
                           MenuName,
                           wpExtra,
                           dwFlags,
                           pti->Desktop,
                           pi);

    if (Class != NULL)
    {
        PCLS *List;

        /* Register the class */
        if (Class->System)
            List = &SystemClassList;
        else if (Class->Global)
            List = &pi->pclsPublicList;
        else
            List = &pi->pclsPrivateList;

        Class->pclsNext = *List;
        (void)InterlockedExchangePointer((PVOID*)List,
                                         Class);

        Ret = Class->atomClassName;
    }

    return Ret;
}

BOOL
UserUnregisterClass(IN PUNICODE_STRING ClassName,
                    IN HINSTANCE hInstance)
{
    PCLS *Link;
    PPROCESSINFO pi;
    RTL_ATOM ClassAtom;
    PCLS Class;

    pi = GetW32ProcessInfo();
    if (pi == NULL)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    TRACE("UserUnregisterClass(%wZ)\n", ClassName);

    /* NOTE: Accessing the buffer in ClassName may raise an exception! */
    ClassAtom = IntGetClassAtom(ClassName,
                                hInstance,
                                pi,
                                &Class,
                                &Link);
    if (ClassAtom == (RTL_ATOM)0)
    {
        return FALSE;
    }

    ASSERT(Class != NULL);

    if (Class->cWndReferenceCount != 0 ||
        Class->pclsClone != NULL)
    {
        SetLastWin32Error(ERROR_CLASS_HAS_WINDOWS);
        return FALSE;
    }

    /* must be a base class! */
    ASSERT(Class->pclsBase == Class);

    /* unlink the class */
    *Link = Class->pclsNext;

    if (NT_SUCCESS(IntDeregisterClassAtom(Class->atomClassName)))
    {
        /* finally free the resources */
        IntDestroyClass(Class);
        return TRUE;
    }
	return FALSE;
}

INT
UserGetClassName(IN PCLS Class,
                 IN OUT PUNICODE_STRING ClassName,
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

            /* limit the size of the static buffer on the stack to the
               size of the buffer provided by the caller */
            if (BufLen / sizeof(WCHAR) > AnsiClassName->MaximumLength)
            {
                BufLen = AnsiClassName->MaximumLength * sizeof(WCHAR);
            }

            /* find out how big the buffer needs to be */
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
                    /* the buffer required exceeds the ansi buffer provided,
                       pretend like we're using the ansi buffer and limit the
                       size to the buffer size provided */
                    BufLen = AnsiClassName->MaximumLength * sizeof(WCHAR);
                }

                /* allocate a temporary buffer that can hold the unicode class name */
                szTemp = ExAllocatePool(PagedPool,
                                        BufLen);
                if (szTemp == NULL)
                {
                    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
                    _SEH2_LEAVE;
                }

                /* query the class name */
                Status = RtlQueryAtomInAtomTable(gAtomTable,
                                                 Class->atomClassName,
                                                 NULL,
                                                 NULL,
                                                 szTemp,
                                                 &BufLen);
            }
            else
                szTemp = szStaticTemp;

            if (NT_SUCCESS(Status))
            {
                /* convert the atom name to ansi */

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
        else /* !Ansi */
        {
            BufLen = ClassName->MaximumLength;

            /* query the atom name */
            Status = RtlQueryAtomInAtomTable(gAtomTable,
                                             Class->atomClassName,
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
        ExFreePool(szTemp);
    }

    return Ret;
}

ULONG_PTR
UserGetClassLongPtr(IN PCLS Class,
                    IN INT Index,
                    IN BOOL Ansi)
{
    ULONG_PTR Ret = 0;

    if (Index >= 0)
    {
        PULONG_PTR Data;

        TRACE("GetClassLong(%d)\n", Index);
        if (Index + sizeof(ULONG_PTR) < Index ||
            Index + sizeof(ULONG_PTR) > Class->cbclsExtra)
        {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return 0;
        }

        Data = (PULONG_PTR)((ULONG_PTR)(Class + 1) + Index);

        /* FIXME - Data might be a unaligned pointer! Might be a problem on
                   certain architectures, maybe using RtlCopyMemory is a
                   better choice for those architectures! */

        TRACE("Result: %x\n", Ret);
        return *Data;
    }

    switch (Index)
    {
        case GCL_CBWNDEXTRA:
            Ret = (ULONG_PTR)Class->cbwndExtra;
            break;

        case GCL_CBCLSEXTRA:
            Ret = (ULONG_PTR)Class->cbclsExtra;
            break;

        case GCLP_HBRBACKGROUND:
            Ret = (ULONG_PTR)Class->hbrBackground;
            break;

        case GCLP_HCURSOR:
            /* FIXME - get handle from pointer to CURSOR object */
            Ret = (ULONG_PTR)Class->hCursor;
            break;

        case GCLP_HICON:
            /* FIXME - get handle from pointer to ICON object */
            Ret = (ULONG_PTR)Class->hIcon;
            break;

        case GCLP_HICONSM:
            /* FIXME - get handle from pointer to ICON object */
            Ret = (ULONG_PTR)Class->hIconSm;
            break;

        case GCLP_HMODULE:
            Ret = (ULONG_PTR)Class->hModule;
            break;

        case GCLP_MENUNAME:
            /* NOTE: Returns pointer in kernel heap! */
            if (Ansi)
                Ret = (ULONG_PTR)Class->lpszClientAnsiMenuName;
            else
                Ret = (ULONG_PTR)Class->lpszClientUnicodeMenuName;
            break;

        case GCL_STYLE:
            Ret = (ULONG_PTR)Class->style;
            break;

        case GCLP_WNDPROC:
            Ret = (ULONG_PTR)IntGetClassWndProc(Class,
                                                GetW32ProcessInfo(),
                                                Ansi);
            break;

        case GCW_ATOM:
            Ret = (ULONG_PTR)Class->atomClassName;
            break;

        default:
            SetLastWin32Error(ERROR_INVALID_INDEX);
            break;
    }

    return Ret;
}

static BOOL
IntSetClassMenuName(IN PCLS Class,
                    IN PUNICODE_STRING MenuName)
{
    BOOL Ret = FALSE;

    /* change the base class first */
    Class = Class->pclsBase;

    if (MenuName->Length != 0)
    {
        ANSI_STRING AnsiString;
        PWSTR strBufW;

        AnsiString.MaximumLength = RtlUnicodeStringToAnsiSize(MenuName);

        strBufW = UserHeapAlloc(MenuName->Length + sizeof(UNICODE_NULL) +
                                AnsiString.MaximumLength);
        if (strBufW != NULL)
        {
            _SEH2_TRY
            {
                NTSTATUS Status;

                /* copy the unicode string */
                RtlCopyMemory(strBufW,
                              MenuName->Buffer,
                              MenuName->Length);
                strBufW[MenuName->Length / sizeof(WCHAR)] = UNICODE_NULL;

                /* create an ansi copy of the string */
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
                /* update the base class */
                IntFreeClassMenuName(Class);
                Class->lpszClientUnicodeMenuName = strBufW;
                Class->lpszClientAnsiMenuName = AnsiString.Buffer;
                Class->MenuNameIsString = TRUE;

                /* update the clones */
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
                DPRINT1("Failed to copy class menu name!\n");
                UserHeapFree(strBufW);
            }
        }
        else
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    }
    else
    {
        ASSERT(IS_INTRESOURCE(MenuName->Buffer));

        /* update the base class */
        IntFreeClassMenuName(Class);
        Class->lpszClientUnicodeMenuName = MenuName->Buffer;
        Class->lpszClientAnsiMenuName = (PSTR)MenuName->Buffer;
        Class->MenuNameIsString = FALSE;

        /* update the clones */
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

ULONG_PTR
UserSetClassLongPtr(IN PCLS Class,
                    IN INT Index,
                    IN ULONG_PTR NewLong,
                    IN BOOL Ansi)
{
    ULONG_PTR Ret = 0;

    /* NOTE: For GCLP_MENUNAME and GCW_ATOM this function may raise an exception! */

    /* change the information in the base class first, then update the clones */
    Class = Class->pclsBase;

    if (Index >= 0)
    {
        PULONG_PTR Data;

        TRACE("SetClassLong(%d, %x)\n", Index, NewLong);

        if (Index + sizeof(ULONG_PTR) < Index ||
            Index + sizeof(ULONG_PTR) > Class->cbclsExtra)
        {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return 0;
        }

        Data = (PULONG_PTR)((ULONG_PTR)(Class + 1) + Index);

        /* FIXME - Data might be a unaligned pointer! Might be a problem on
                   certain architectures, maybe using RtlCopyMemory is a
                   better choice for those architectures! */
        Ret = *Data;
        *Data = NewLong;

        /* update the clones */
        Class = Class->pclsClone;
        while (Class != NULL)
        {
            *(PULONG_PTR)((ULONG_PTR)(Class + 1) + Index) = NewLong;
            Class = Class->pclsNext;
        }

        return Ret;
    }

    switch (Index)
    {
        case GCL_CBWNDEXTRA:
            Ret = (ULONG_PTR)Class->cbwndExtra;
            Class->cbwndExtra = (INT)NewLong;

            /* update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                Class->cbwndExtra = (INT)NewLong;
                Class = Class->pclsNext;
            }

            break;

        case GCL_CBCLSEXTRA:
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            break;

        case GCLP_HBRBACKGROUND:
            Ret = (ULONG_PTR)Class->hbrBackground;
            Class->hbrBackground = (HBRUSH)NewLong;

            /* update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                Class->hbrBackground = (HBRUSH)NewLong;
                Class = Class->pclsNext;
            }
            break;

        case GCLP_HCURSOR:
            /* FIXME - get handle from pointer to CURSOR object */
            Ret = (ULONG_PTR)Class->hCursor;
            Class->hCursor = (HANDLE)NewLong;

            /* update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                Class->hCursor = (HANDLE)NewLong;
                Class = Class->pclsNext;
            }
            break;

        case GCLP_HICON:
            /* FIXME - get handle from pointer to ICON object */
            Ret = (ULONG_PTR)Class->hIcon;
            Class->hIcon = (HANDLE)NewLong;

            /* update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                Class->hIcon = (HANDLE)NewLong;
                Class = Class->pclsNext;
            }
            break;

        case GCLP_HICONSM:
            /* FIXME - get handle from pointer to ICON object */
            Ret = (ULONG_PTR)Class->hIconSm;
            Class->hIconSm = (HANDLE)NewLong;

            /* update the clones */
            Class = Class->pclsClone;
            while (Class != NULL)
            {
                Class->hIconSm = (HANDLE)NewLong;
                Class = Class->pclsNext;
            }
            break;

        case GCLP_HMODULE:
            Ret = (ULONG_PTR)Class->hModule;
            Class->hModule = (HINSTANCE)NewLong;

           /* update the clones */
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
                DPRINT1("Setting the class menu name failed!\n");
            }

            /* FIXME - really return NULL? Wine does so... */
            break;
        }

        case GCL_STYLE:
            Ret = (ULONG_PTR)Class->style;
            Class->style = (UINT)NewLong;

            /* FIXME - what if the CS_GLOBALCLASS style is changed? should we
                       move the class to the appropriate list? For now, we save
                       the original value in Class->Global, so we can always
                       locate the appropriate list */

           /* update the clones */
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

            Ret = (ULONG_PTR)Class->atomClassName;
            if (!IntSetClassAtom(Class,
                                 Value))
            {
                Ret = 0;
            }
            break;
        }

        default:
            SetLastWin32Error(ERROR_INVALID_INDEX);
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
    PPROCESSINFO pi;

    lpwcx->style = Class->style;

    pi = GetW32ProcessInfo();
    lpwcx->lpfnWndProc = IntGetClassWndProc(Class,
                                            pi,
                                            Ansi);

    lpwcx->cbClsExtra = Class->cbclsExtra;
    lpwcx->cbWndExtra = Class->cbwndExtra;
    lpwcx->hIcon = Class->hIcon; /* FIXME - get handle from pointer */
    lpwcx->hCursor = Class->hCursor; /* FIXME - get handle from pointer */
    lpwcx->hbrBackground = Class->hbrBackground;
/*
    FIXME!
    Cls: lpszMenuName and lpszAnsiClassName should be used by kernel space.
    lpszClientXxxMenuName should already be mapped to user space.
 */
    if (Ansi)
       ((PWNDCLASSEXA)lpwcx)->lpszMenuName = Class->lpszClientAnsiMenuName;
    else
       lpwcx->lpszMenuName = Class->lpszClientUnicodeMenuName;

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

    /* FIXME - return the string? Okay! This is performed in User32!*/
    //lpwcx->lpszClassName = (LPCWSTR)((ULONG_PTR)Class->atomClassName);

    lpwcx->hIconSm = Class->hIconSm; /* FIXME - get handle from pointer */

    return TRUE;
}

BOOL
UserRegisterSystemClasses(IN ULONG Count,
                          IN PREGISTER_SYSCLASS SystemClasses)
{
    /* NOTE: This routine may raise exceptions! */
    UINT i;
    UNICODE_STRING ClassName, MenuName;
    PPROCESSINFO pi = GetW32ProcessInfo();
    WNDCLASSEXW wc;
    PCLS Class;
    BOOL Ret = TRUE;

    if ( hModClient == NULL)
        return FALSE;
    /* Init System Classes once only*/
    if (RegisteredSysClasses)
       return TRUE;

    RtlZeroMemory(&MenuName, sizeof(MenuName));

    for (i = 0; i != Count; i++)
    {
        ClassName = ProbeForReadUnicodeString(&SystemClasses[i].ClassName);
        if (ClassName.Length != 0)
        {
            ProbeForRead(ClassName.Buffer,
                         ClassName.Length,
                         sizeof(WCHAR));
        }

        wc.cbSize = sizeof(wc);
        wc.style = SystemClasses[i].Style;
        wc.lpfnWndProc = SystemClasses[i].ProcW;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = SystemClasses[i].ExtraBytes;
        wc.hInstance = hModClient;
        wc.hIcon = NULL;
        wc.hCursor = SystemClasses[i].hCursor;
        wc.hbrBackground = SystemClasses[i].hBrush;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = ClassName.Buffer;
        wc.hIconSm = NULL;

        Class = IntCreateClass(&wc,
                               &ClassName,
                               &MenuName,
                               SystemClasses[i].ProcA,
                               CSF_SYSTEMCLASS,
                               NULL,
                               pi);
        if (Class != NULL)
        {
            int iCls;

            Class->fnid = SystemClasses[i].ClassId;
            if (LockupFnIdToiCls(Class->fnid, &iCls))
            {
                gpsi->atomSysClass[iCls] = Class->atomClassName;
            }

            ASSERT(Class->System);
            Class->pclsNext = SystemClassList;
            (void)InterlockedExchangePointer((PVOID*)&SystemClassList,
                                             Class);
        }
        else
        {
            WARN("!!! Registering system class failed!\n");
            Ret = FALSE;
        }
    }

    if (Ret)
        RegisteredSysClasses = TRUE;
    return Ret;
}

/* SYSCALLS *****************************************************************/

RTL_ATOM
APIENTRY
NtUserRegisterClassExWOW(
    WNDCLASSEXW* lpwcx,
    PUNICODE_STRING ClassName,
    PUNICODE_STRING ClsNVersion,
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
    UNICODE_STRING CapturedName = {0}, CapturedMenuName = {0};
    RTL_ATOM Ret = (RTL_ATOM)0;
    WNDPROC wpExtra = NULL;

    if (Flags & ~(CSF_ANSIPROC))
    {
        SetLastWin32Error(ERROR_INVALID_FLAGS);
        return Ret;
    }

    UserEnterExclusive();

    _SEH2_TRY
    {
        /* Probe the parameters and basic parameter checks */
        if (ProbeForReadUint(&lpwcx->cbSize) != sizeof(WNDCLASSEXW))
        {
            goto InvalidParameter;
        }

        ProbeForRead(lpwcx,
                     sizeof(WNDCLASSEXW),
                     sizeof(ULONG));
        RtlCopyMemory(&CapturedClassInfo,
                      lpwcx,
                      sizeof(WNDCLASSEXW));

        CapturedName = ProbeForReadUnicodeString(ClassName);

        ProbeForRead(pClassMenuName,
                     sizeof(CLSMENUNAME),
                     1);

        CapturedMenuName = ProbeForReadUnicodeString(pClassMenuName->pusMenuName);

        if ( CapturedName.Length & 1 ||
             CapturedMenuName.Length & 1 ||
             CapturedClassInfo.cbClsExtra < 0 ||
             CapturedClassInfo.cbClsExtra +
                CapturedName.Length +
                CapturedMenuName.Length +
                sizeof(CLS) < CapturedClassInfo.cbClsExtra ||
             CapturedClassInfo.cbWndExtra < 0 ||
             CapturedClassInfo.hInstance == NULL)
        {
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
InvalidParameter:
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            _SEH2_LEAVE;
        }
        /* Register the class */
        Ret = UserRegisterClass(&CapturedClassInfo,
                                &CapturedName,
                                &CapturedMenuName,
                                wpExtra,
                                Flags);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    UserLeave();

    return Ret;
}

ULONG_PTR APIENTRY
NtUserGetClassLong(IN HWND hWnd,
                   IN INT Offset,
                   IN BOOL Ansi)
{
    PWINDOW_OBJECT Window;
    ULONG_PTR Ret = 0;

    if (Offset != GCLP_WNDPROC)
    {
        UserEnterShared();
    }
    else
    {
        UserEnterExclusive();
    }

    Window = UserGetWindowObject(hWnd);
    if (Window != NULL)
    {
        Ret = UserGetClassLongPtr(Window->Wnd->pcls,
                                  Offset,
                                  Ansi);

        if (Ret != 0 && Offset == GCLP_MENUNAME && Window->Wnd->pcls->MenuNameIsString)
        {
            Ret = (ULONG_PTR)UserHeapAddressToUser((PVOID)Ret);
        }
    }

    UserLeave();

    return Ret;
}



ULONG_PTR APIENTRY
NtUserSetClassLong(HWND hWnd,
                   INT Offset,
                   ULONG_PTR dwNewLong,
                   BOOL Ansi)
{
    PPROCESSINFO pi;
    PWINDOW_OBJECT Window;
    ULONG_PTR Ret = 0;

    UserEnterExclusive();

    pi = GetW32ProcessInfo();
    if (pi == NULL)
        goto Cleanup;

    Window = UserGetWindowObject(hWnd);
    if (Window != NULL)
    {
        if (Window->ti->ppi != pi)
        {
            SetLastWin32Error(ERROR_ACCESS_DENIED);
            goto Cleanup;
        }

        _SEH2_TRY
        {
            UNICODE_STRING Value;

            /* probe the parameters */
            if (Offset == GCW_ATOM || Offset == GCLP_MENUNAME)
            {
                Value = ProbeForReadUnicodeString((PUNICODE_STRING)dwNewLong);
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
                        SetLastWin32Error(ERROR_INVALID_PARAMETER);
                        _SEH2_LEAVE;
                    }
                }

                dwNewLong = (ULONG_PTR)&Value;
            }

            Ret = UserSetClassLongPtr(Window->Wnd->pcls,
                                      Offset,
                                      dwNewLong,
                                      Ansi);
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

BOOL APIENTRY
NtUserUnregisterClass(IN PUNICODE_STRING ClassNameOrAtom,
                      IN HINSTANCE hInstance,
                      OUT PCLSMENUNAME pClassMenuName)
{
    UNICODE_STRING CapturedClassName;
    BOOL Ret = FALSE;

    UserEnterExclusive();

    _SEH2_TRY
    {
        /* probe the paramters */
        CapturedClassName = ProbeForReadUnicodeString(ClassNameOrAtom);
        if (CapturedClassName.Length & 1)
        {
            goto InvalidParameter;
        }

        if (CapturedClassName.Length != 0)
        {
            ProbeForRead(CapturedClassName.Buffer,
                         CapturedClassName.Length,
                         sizeof(WCHAR));
        }
        else
        {
            if (!IS_ATOM(CapturedClassName.Buffer))
            {
InvalidParameter:
                SetLastWin32Error(ERROR_INVALID_PARAMETER);
                _SEH2_LEAVE;
            }
        }

        /* unregister the class */
        Ret = UserUnregisterClass(&CapturedClassName,
                                  hInstance);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    UserLeave();
    return Ret;
}

/* NOTE: for system classes hInstance is not NULL here, but User32Instance */
BOOL APIENTRY
NtUserGetClassInfo(
   HINSTANCE hInstance,
   PUNICODE_STRING ClassName,
   LPWNDCLASSEXW lpWndClassEx,
   LPWSTR *ppszMenuName,
   BOOL Ansi)
{
   UNICODE_STRING CapturedClassName, SafeClassName;
   WNDCLASSEXW Safewcexw;
   PCLS Class;
   RTL_ATOM ClassAtom = 0;
   PPROCESSINFO pi;
   BOOL Ret = TRUE;

   /* NOTE: need exclusive lock because getting the wndproc might require the
            creation of a call procedure handle */
   UserEnterExclusive();

   pi = GetW32ProcessInfo();
   if (pi == NULL)
   {
      ERR("GetW32ProcessInfo() returned NULL!\n");
      goto Cleanup;
   }

   _SEH2_TRY
   {
      /* probe the paramters */
      CapturedClassName = ProbeForReadUnicodeString(ClassName);

      if (CapturedClassName.Length == 0)
         TRACE("hInst %p atom %04X lpWndClassEx %p Ansi %d\n", hInstance, CapturedClassName.Buffer, lpWndClassEx, Ansi);
      else
         TRACE("hInst %p class %wZ lpWndClassEx %p Ansi %d\n", hInstance, &CapturedClassName, lpWndClassEx, Ansi);

      if (CapturedClassName.Length & 1)
      {
         goto InvalidParameter;
      }

      if (CapturedClassName.Length != 0)
      {
         ProbeForRead( CapturedClassName.Buffer,
                       CapturedClassName.Length,
                       sizeof(WCHAR));

         RtlInitUnicodeString( &SafeClassName, CapturedClassName.Buffer);

         SafeClassName.Buffer = ExAllocatePoolWithTag( PagedPool,
                                                       SafeClassName.MaximumLength,
                                                       TAG_STRING);
         RtlCopyMemory( SafeClassName.Buffer,
                        CapturedClassName.Buffer,
                        SafeClassName.MaximumLength);
      }
      else
      {
         if (!IS_ATOM(CapturedClassName.Buffer))
         {
            ERR("NtUserGetClassInfo() got ClassName instead of Atom!\n");
            goto InvalidParameter;
         }

         SafeClassName.Buffer = CapturedClassName.Buffer;
         SafeClassName.Length = 0;
         SafeClassName.MaximumLength = 0;
      }

      if (ProbeForReadUint(&lpWndClassEx->cbSize) != sizeof(WNDCLASSEXW))
      {
InvalidParameter:
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         Ret = FALSE;
         _SEH2_LEAVE;
      }

      ProbeForWrite( lpWndClassEx, sizeof(WNDCLASSEXW), sizeof(ULONG));

      RtlCopyMemory( &Safewcexw, lpWndClassEx, sizeof(WNDCLASSEXW));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      Ret = FALSE;
   }
   _SEH2_END;

   if (Ret)
   {
      ClassAtom = IntGetClassAtom( &SafeClassName,
                                    hInstance,
                                    pi,
                                   &Class,
                                    NULL);
      if (ClassAtom != (RTL_ATOM)0)
      {
         if (hInstance == NULL) hInstance = hModClient;

         Ret = UserGetClassInfo( Class,
                                &Safewcexw,
                                 Ansi,
                                 hInstance);
      }
      else
      {
         SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
         Ret = FALSE;
      }
   }
   _SEH2_TRY
   {
      if (Ret)
      {
         /* Emulate Function. */
         if (ppszMenuName) *ppszMenuName = (LPWSTR)Safewcexw.lpszMenuName;

         RtlCopyMemory(lpWndClassEx, &Safewcexw, sizeof(WNDCLASSEXW));

         // From Wine:
         /* We must return the atom of the class here instead of just TRUE. */
         /* Undocumented behavior! Return the class atom as a BOOL! */
         Ret = (BOOL)ClassAtom;
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      Ret = FALSE;
   }
   _SEH2_END;

Cleanup:
   if (SafeClassName.Length) ExFreePoolWithTag(SafeClassName.Buffer, TAG_STRING);
   UserLeave();
   return Ret;
}



INT APIENTRY
NtUserGetClassName (IN HWND hWnd,
                    OUT PUNICODE_STRING ClassName,
                    IN BOOL Ansi)
{
    PWINDOW_OBJECT Window;
    UNICODE_STRING CapturedClassName;
    INT Ret = 0;

    UserEnterShared();

    Window = UserGetWindowObject(hWnd);
    if (Window != NULL)
    {
        _SEH2_TRY
        {
            ProbeForWriteUnicodeString(ClassName);
            CapturedClassName = *ClassName;

            /* get the class name */
            Ret = UserGetClassName(Window->Wnd->pcls,
                                   &CapturedClassName,
                                   Ansi);

            if (Ret != 0)
            {
                /* update the Length field */
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
PCLS APIENTRY
NtUserGetWOWClass(HINSTANCE hInstance,
                  PUNICODE_STRING ClassName)
{
  UNICODE_STRING SafeClassName;
  PPROCESSINFO pi;
  PCLS Class = NULL;
  RTL_ATOM ClassAtom = 0;
  BOOL Hit = FALSE;

  UserEnterExclusive();

  pi = GetW32ProcessInfo();
  if (pi == NULL)
  {
     ERR("GetW32ProcessInfo() returned NULL!\n");
     goto Cleanup;
  }

  _SEH2_TRY
  {
     if (ClassName->Length != 0)
     {
        ProbeForRead( ClassName->Buffer,
                      ClassName->Length,
                      sizeof(WCHAR));

        RtlInitUnicodeString( &SafeClassName, ClassName->Buffer);

        SafeClassName.Buffer = ExAllocatePoolWithTag( PagedPool,
                                                      SafeClassName.MaximumLength,
                                                      TAG_STRING);
        RtlCopyMemory( SafeClassName.Buffer,
                       ClassName->Buffer,
                       SafeClassName.MaximumLength);
     }
     else
     {
        if (!IS_ATOM(ClassName->Buffer))
        {
           ERR("NtUserGetWOWClass() got ClassName instead of Atom!\n");
           Hit = TRUE;
        }
        else
        {
           SafeClassName.Buffer = ClassName->Buffer;
           SafeClassName.Length = 0;
           SafeClassName.MaximumLength = 0;
        }
     }
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
     SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
     Hit = TRUE;
  }
  _SEH2_END;

  if (!Hit)
  {
     ClassAtom = IntGetClassAtom( &SafeClassName,
                                   hInstance,
                                   pi,
                                  &Class,
                                   NULL);
     if (!ClassAtom)
     {
        SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
     }
  }

Cleanup:
  if (SafeClassName.Length) ExFreePoolWithTag(SafeClassName.Buffer, TAG_STRING);
  UserLeave();
//
// Don't forget to use DesktopPtrToUser( ? ) with return pointer in user space.
//
  return Class;
}

/* EOF */
