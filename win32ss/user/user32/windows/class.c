/*
 * PROJECT:         ReactOS user32.dll
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            win32ss/user/user32/windows/class.c
 * PURPOSE:         Window classes
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define USE_VERSIONED_CLASSES

/* From rtl/actctx.c and must match! */
struct strsection_header
{
    DWORD magic;
    ULONG size;
    DWORD unk1[3];
    ULONG count;
    ULONG index_offset;
    DWORD unk2[2];
    ULONG global_offset;
    ULONG global_len;
};

struct wndclass_redirect_data
{
    ULONG size;
    DWORD res;
    ULONG name_len;
    ULONG name_offset;  /* versioned name offset */
    ULONG module_len;
    ULONG module_offset;/* container name offset */
};

//
// Use wine hack to process extended context classes.
//
/***********************************************************************
 *           is_comctl32_class
 */
LPCWSTR is_comctl32_class( const WCHAR *name )
{
    static const WCHAR classesW[][20] =
    {
        {'C','o','m','b','o','B','o','x','E','x','3','2',0},
        {'m','s','c','t','l','s','_','h','o','t','k','e','y','3','2',0},
        {'m','s','c','t','l','s','_','p','r','o','g','r','e','s','s','3','2',0},
        {'m','s','c','t','l','s','_','s','t','a','t','u','s','b','a','r','3','2',0},
        {'m','s','c','t','l','s','_','t','r','a','c','k','b','a','r','3','2',0},
        {'m','s','c','t','l','s','_','u','p','d','o','w','n','3','2',0},
        {'N','a','t','i','v','e','F','o','n','t','C','t','l',0},
        {'R','e','B','a','r','W','i','n','d','o','w','3','2',0},
        {'S','y','s','A','n','i','m','a','t','e','3','2',0},
        {'S','y','s','D','a','t','e','T','i','m','e','P','i','c','k','3','2',0},
        {'S','y','s','H','e','a','d','e','r','3','2',0},
        {'S','y','s','I','P','A','d','d','r','e','s','s','3','2',0},
        {'S','y','s','L','i','s','t','V','i','e','w','3','2',0},
        {'S','y','s','M','o','n','t','h','C','a','l','3','2',0},
        {'S','y','s','P','a','g','e','r',0},
        {'S','y','s','T','a','b','C','o','n','t','r','o','l','3','2',0},
        {'S','y','s','T','r','e','e','V','i','e','w','3','2',0},
        {'T','o','o','l','b','a','r','W','i','n','d','o','w','3','2',0},
        {'t','o','o','l','t','i','p','s','_','c','l','a','s','s','3','2',0},
    };

    int min = 0, max = (sizeof(classesW) / sizeof(classesW[0])) - 1;

    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        if (!(res = strcmpiW( name, classesW[pos] ))) return classesW[pos];
        if (res < 0) max = pos - 1;
        else min = pos + 1;
    }
    return NULL;
}

LPCWSTR
FASTCALL
ClassNameToVersion(
  const void* lpszClass,
  LPCWSTR lpszMenuName,
  LPCWSTR *plpLibFileName,
  HANDLE *pContext,
  BOOL bAnsi)
{
    LPCWSTR VersionedClass = NULL;
#ifdef USE_VERSIONED_CLASSES
    NTSTATUS Status;
#endif
    UNICODE_STRING SectionName;
    WCHAR SectionNameBuf[MAX_PATH] = {0};
    ACTCTX_SECTION_KEYED_DATA KeyedData = { sizeof(KeyedData) };

    if(!lpszClass)
    {
        ERR("Null class given !\n");
        return NULL;
    }

    if (IS_ATOM(lpszClass))
    {
        RtlInitEmptyUnicodeString(&SectionName, SectionNameBuf, sizeof(SectionNameBuf));
        if(!NtUserGetAtomName(LOWORD((DWORD_PTR)lpszClass), &SectionName))
        {
            ERR("Couldn't get atom name for atom %x !\n", LOWORD((DWORD_PTR)lpszClass));
            return NULL;
        }
        SectionName.Length = (USHORT)wcslen(SectionNameBuf) * sizeof(WCHAR);
        TRACE("ClassNameToVersion got name %wZ from atom\n", &SectionName);
    }
    else
    {
        if (bAnsi)
        {
            ANSI_STRING AnsiString;
            RtlInitAnsiString(&AnsiString, lpszClass);
            RtlInitEmptyUnicodeString(&SectionName, SectionNameBuf, sizeof(SectionNameBuf));
            RtlAnsiStringToUnicodeString(&SectionName, &AnsiString, FALSE);
        }
        else
        {
            RtlInitUnicodeString(&SectionName, lpszClass);
        }
    }
#ifdef USE_VERSIONED_CLASSES
    Status = RtlFindActivationContextSectionString( FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX,
                                                    NULL,
                                                    ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
                                                   &SectionName,
                                                   &KeyedData );

    if (NT_SUCCESS(Status) && KeyedData.ulDataFormatVersion == 1)
    {
        struct strsection_header *SectionHeader = KeyedData.lpSectionBase;

        /* Find activation context */
        if(SectionHeader && SectionHeader->count > 0)
        {
            struct wndclass_redirect_data *WindowRedirectionData = KeyedData.lpData;
            if(WindowRedirectionData && WindowRedirectionData->module_len)
            {
                LPCWSTR lpLibFileName;

                VersionedClass = (WCHAR*)((BYTE*)WindowRedirectionData + WindowRedirectionData->name_offset);
                lpLibFileName = (WCHAR*)((BYTE*)KeyedData.lpSectionBase + WindowRedirectionData->module_offset);
                TRACE("Returning VersionedClass=%S, plpLibFileName=%S for class %S\n", VersionedClass, lpLibFileName, SectionName.Buffer);

                if (pContext) *pContext = KeyedData.hActCtx;
                if (plpLibFileName) *plpLibFileName = lpLibFileName;

            }
        }
    }

    if (KeyedData.hActCtx)
        RtlReleaseActivationContext(KeyedData.hActCtx);
#endif

#ifndef DEFAULT_ACTIVATION_CONTEXTS_SUPPORTED
    /* This block is a hack! */
    if (!VersionedClass)
    {
        /*
         * In windows the default activation context always contains comctl32v5
         * In reactos we don't have a default activation context so we
         * mimic wine here.
         */
        VersionedClass = is_comctl32_class(SectionName.Buffer);
        if (VersionedClass)
        {
            if (pContext) *pContext = 0;
            if (plpLibFileName) *plpLibFileName = L"comctl32";
        }
    }
#endif

    /*
     * The returned strings are pointers in the activation context and
     * will get freed when the activation context gets freed
     */
    return VersionedClass;
}

//
// Ref: http://yvs-it.blogspot.com/2010/04/initcommoncontrolsex.html
//
BOOL
FASTCALL
VersionRegisterClass(
  PCWSTR pszClass,
  LPCWSTR lpLibFileName,
  HANDLE Contex,
  HMODULE * phLibModule)
{
    BOOL Ret = FALSE;
    HMODULE hLibModule = NULL;
    PREGISTERCLASSNAMEW pRegisterClassNameW;
    UNICODE_STRING ClassName;
    WCHAR ClassNameBuf[MAX_PATH] = {0};
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame = { sizeof(Frame), 1 };

    ERR("VersionRegisterClass: Attempting to call RegisterClassNameW in %S.\n", lpLibFileName);

    RtlActivateActivationContextUnsafeFast(&Frame, Contex);

    _SEH2_TRY
    {
        hLibModule = LoadLibraryW(lpLibFileName);
        if (hLibModule)
        {
            if ((pRegisterClassNameW = (void*)GetProcAddress(hLibModule, "RegisterClassNameW")))
            {
                if (IS_ATOM(pszClass))
                {
                    RtlInitEmptyUnicodeString(&ClassName, ClassNameBuf, sizeof(ClassNameBuf));
                    if (!NtUserGetAtomName(LOWORD((DWORD_PTR)pszClass), &ClassName))
                    {
                        ERR("Error while verifying ATOM\n");
                        _SEH2_YIELD(goto Error_Exit);
                    }
                    pszClass = ClassName.Buffer;
                }
                Ret = pRegisterClassNameW(pszClass);
            }
            else
            {
                WARN("No RegisterClassNameW PROC\n");
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
       ERR("Got exception while trying to call RegisterClassNameW!\n");
    }
    _SEH2_END

Error_Exit:
    if (Ret || !hLibModule)
    {
        if (phLibModule) *phLibModule = hLibModule;
    }
    else
    {
        DWORD dwLastError = GetLastError();
        FreeLibrary(hLibModule);
        SetLastError(dwLastError);
    }

    RtlDeactivateActivationContextUnsafeFast(&Frame);
    return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetClassInfoExA(
  HINSTANCE hInstance,
  LPCSTR lpszClass,
  LPWNDCLASSEXA lpwcx)
{
    UNICODE_STRING ClassName = {0};
    LPCSTR pszMenuName;
    HMODULE hLibModule = NULL;
    DWORD dwLastError;
    BOOL Ret, ClassFound = FALSE, ConvertedString = FALSE;
    LPCWSTR lpszClsVersion;
    HANDLE pCtx = NULL;
    LPCWSTR lpLibFileName = NULL;

    TRACE("%p class/atom: %s/%04x %p\n", hInstance,
        IS_ATOM(lpszClass) ? NULL : lpszClass,
        IS_ATOM(lpszClass) ? lpszClass : 0,
        lpwcx);

    if (!lpwcx)
    {
        SetLastError(ERROR_NOACCESS);
        return FALSE;
    }

    if (hInstance == User32Instance)
    {
        hInstance = NULL;
    }

    if (lpszClass == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lpszClsVersion = ClassNameToVersion(lpszClass, NULL, &lpLibFileName, &pCtx, TRUE);
    if (lpszClsVersion)
    {
        RtlInitUnicodeString(&ClassName, lpszClsVersion);
    }
    else if (IS_ATOM(lpszClass))
    {
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpszClass);
    }
    else
    {
        ConvertedString = TRUE;
        if (!RtlCreateUnicodeStringFromAsciiz(&ClassName, lpszClass))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    if (!RegisterDefaultClasses)
    {
        TRACE("RegisterSystemControls\n");
        RegisterSystemControls();
    }

    for(;;)
    {
        Ret = NtUserGetClassInfo(hInstance,
                                 &ClassName,
                                 (LPWNDCLASSEXW)lpwcx,
                                 (LPWSTR *)&pszMenuName,
                                 TRUE);
        if (Ret) break;
        if (!lpLibFileName) break;
        if (!ClassFound)
        {
            dwLastError = GetLastError();
            if ( dwLastError == ERROR_CANNOT_FIND_WND_CLASS ||
                 dwLastError == ERROR_CLASS_DOES_NOT_EXIST )
            {
                ClassFound = VersionRegisterClass(ClassName.Buffer, lpLibFileName, pCtx, &hLibModule);
                if (ClassFound) continue;
            }
        }
        if (hLibModule)
        {
            dwLastError = GetLastError();
            FreeLibrary(hLibModule);
            SetLastError(dwLastError);
            hLibModule = 0;
        }
        break;
    }

    if (Ret)
    {
        lpwcx->lpszClassName = lpszClass;
//       lpwcx->lpszMenuName  = pszMenuName;
    }

    if (ConvertedString)
    {
        RtlFreeUnicodeString(&ClassName);
    }

    return Ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetClassInfoExW(
  HINSTANCE hInstance,
  LPCWSTR lpszClass,
  LPWNDCLASSEXW lpwcx)
{
    UNICODE_STRING ClassName = {0};
    LPWSTR pszMenuName;
    HMODULE hLibModule = NULL;
    DWORD dwLastError;
    BOOL Ret, ClassFound = FALSE;
    LPCWSTR lpszClsVersion;
    HANDLE pCtx = NULL;
    LPCWSTR lpLibFileName = NULL;

    TRACE("%p class/atom: %S/%04x %p\n", hInstance,
        IS_ATOM(lpszClass) ? NULL : lpszClass,
        IS_ATOM(lpszClass) ? lpszClass : 0,
        lpwcx);

    /* From wine, for speed only, ReactOS supports the correct return in
     * Win32k. cbSize is ignored.
     */
    if (!lpwcx)
    {
       SetLastError( ERROR_NOACCESS );
       return FALSE;
    }

    if (hInstance == User32Instance)
    {
        hInstance = NULL;
    }

    if (lpszClass == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lpszClsVersion = ClassNameToVersion(lpszClass, NULL, &lpLibFileName, &pCtx, FALSE);
    if (lpszClsVersion)
    {
        RtlInitUnicodeString(&ClassName, lpszClsVersion);
    }
    else if (IS_ATOM(lpszClass))
    {
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpszClass);
    }
    else
    {
        RtlInitUnicodeString(&ClassName, lpszClass);
    }

    if (!RegisterDefaultClasses)
    {
       TRACE("RegisterSystemControls\n");
       RegisterSystemControls();
    }

    for(;;)
    {
        Ret = NtUserGetClassInfo( hInstance,
                                 &ClassName,
                                  lpwcx,
                                 &pszMenuName,
                                  FALSE);
        if (Ret) break;
        if (!lpLibFileName) break;
        if (!ClassFound)
        {
            dwLastError = GetLastError();
            if ( dwLastError == ERROR_CANNOT_FIND_WND_CLASS ||
                 dwLastError == ERROR_CLASS_DOES_NOT_EXIST )
            {
                ClassFound = VersionRegisterClass(ClassName.Buffer, lpLibFileName, pCtx, &hLibModule);
                if (ClassFound) continue;
            }
        }
        if (hLibModule)
        {
            dwLastError = GetLastError();
            FreeLibrary(hLibModule);
            SetLastError(dwLastError);
            hLibModule = 0;
        }
        break;
    }

    if (Ret)
    {
        lpwcx->lpszClassName = lpszClass;
//       lpwcx->lpszMenuName  = pszMenuName;
    }
    return Ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetClassInfoA(
  HINSTANCE hInstance,
  LPCSTR lpClassName,
  LPWNDCLASSA lpWndClass)
{
    WNDCLASSEXA wcex;
    BOOL retval;

    retval = GetClassInfoExA(hInstance, lpClassName, &wcex);
    if (retval)
    {
        lpWndClass->style         = wcex.style;
        lpWndClass->lpfnWndProc   = wcex.lpfnWndProc;
        lpWndClass->cbClsExtra    = wcex.cbClsExtra;
        lpWndClass->cbWndExtra    = wcex.cbWndExtra;
        lpWndClass->hInstance     = wcex.hInstance;
        lpWndClass->hIcon         = wcex.hIcon;
        lpWndClass->hCursor       = wcex.hCursor;
        lpWndClass->hbrBackground = wcex.hbrBackground;
        lpWndClass->lpszMenuName  = wcex.lpszMenuName;
        lpWndClass->lpszClassName = wcex.lpszClassName;
    }

    return retval;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetClassInfoW(
  HINSTANCE hInstance,
  LPCWSTR lpClassName,
  LPWNDCLASSW lpWndClass)
{
    WNDCLASSEXW wcex;
    BOOL retval;

    retval = GetClassInfoExW(hInstance, lpClassName, &wcex);
    if (retval)
    {
        lpWndClass->style         = wcex.style;
        lpWndClass->lpfnWndProc   = wcex.lpfnWndProc;
        lpWndClass->cbClsExtra    = wcex.cbClsExtra;
        lpWndClass->cbWndExtra    = wcex.cbWndExtra;
        lpWndClass->hInstance     = wcex.hInstance;
        lpWndClass->hIcon         = wcex.hIcon;
        lpWndClass->hCursor       = wcex.hCursor;
        lpWndClass->hbrBackground = wcex.hbrBackground;
        lpWndClass->lpszMenuName  = wcex.lpszMenuName;
        lpWndClass->lpszClassName = wcex.lpszClassName;
    }
    return retval;
}

//
// Based on find_winproc... Fixes many whine tests......
//
ULONG_PTR FASTCALL
IntGetClsWndProc(PWND pWnd, PCLS Class, BOOL Ansi)
{
    INT i;
    ULONG_PTR gcpd, Ret = 0;
 // If server side, sweep through proc list and return the client side proc.
    if (Class->CSF_flags & CSF_SERVERSIDEPROC)
    {  // Always scan through the list due to wine class "deftest".
        for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
        {
            if (GETPFNSERVER(i) == Class->lpfnWndProc)
            {
                if (Ansi)
                    Ret = (ULONG_PTR)GETPFNCLIENTA(i);
                else
                    Ret = (ULONG_PTR)GETPFNCLIENTW(i);
            }
        }
         return Ret;
    }
    // Set return proc.
    Ret = (ULONG_PTR)Class->lpfnWndProc;
    // Return the proc if one of the FnId default class type.
    if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_BUTTON)
    {
        if (Ansi)
        { // If match return the right proc by type.
            if (GETPFNCLIENTW(Class->fnid) == Class->lpfnWndProc)
                Ret = (ULONG_PTR)GETPFNCLIENTA(Class->fnid);
        }
        else
        {
            if (GETPFNCLIENTA(Class->fnid) == Class->lpfnWndProc)
               Ret = (ULONG_PTR)GETPFNCLIENTW(Class->fnid);
        }
    }
    // Return on change or Ansi/Unicode proc equal.
    if ( Ret != (ULONG_PTR)Class->lpfnWndProc ||
         Ansi == !!(Class->CSF_flags & CSF_ANSIPROC) )
        return Ret;

    /* We have an Ansi and Unicode swap! If Ansi create Unicode proc handle.
       This will force CallWindowProc to deal with it. */
    gcpd = NtUserGetCPD( UserHMGetHandle(pWnd),
                         (Ansi ? UserGetCPDA2U : UserGetCPDU2A )|UserGetCPDWndtoCls,
                         Ret);

    return (gcpd ? gcpd : Ret);
}

//
// Based on IntGetClsWndProc
//
WNDPROC FASTCALL
IntGetWndProc(PWND pWnd, BOOL Ansi)
{
    INT i;
    WNDPROC gcpd, Ret = 0;
    PCLS Class = DesktopPtrToUser(pWnd->pcls);

    if (!Class) return Ret;

    if (pWnd->state & WNDS_SERVERSIDEWINDOWPROC)
    {
        for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
        {
            if (GETPFNSERVER(i) == pWnd->lpfnWndProc)
            {
                if (Ansi)
                    Ret = GETPFNCLIENTA(i);
                else
                    Ret = GETPFNCLIENTW(i);
            }
        }
        return Ret;
    }
    // Wine Class tests:
    /*  Edit controls are special - they return a wndproc handle when
        GetWindowLongPtr is called with a different A/W.
        On the other hand there is no W->A->W conversion so this control
        is treated specially.
     */
    if (Class->fnid == FNID_EDIT)
        Ret = pWnd->lpfnWndProc;
    else
    {
        // Set return proc.
        Ret = pWnd->lpfnWndProc;

        if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_BUTTON)
        {
            if (Ansi)
            {
                if (GETPFNCLIENTW(Class->fnid) == pWnd->lpfnWndProc)
                    Ret = GETPFNCLIENTA(Class->fnid);
            }
            else
            {
                if (GETPFNCLIENTA(Class->fnid) == pWnd->lpfnWndProc)
                    Ret = GETPFNCLIENTW(Class->fnid);
            }
        }
        // Return on the change.
        if ( Ret != pWnd->lpfnWndProc)
            return Ret;
    }

    if ( Ansi == !!(pWnd->state & WNDS_ANSIWINDOWPROC) )
        return Ret;

    gcpd = (WNDPROC)NtUserGetCPD( UserHMGetHandle(pWnd),
                                  (Ansi ? UserGetCPDA2U : UserGetCPDU2A )|UserGetCPDWindow,
                                  (ULONG_PTR)Ret);

    return (gcpd ? gcpd : Ret);
}

static ULONG_PTR FASTCALL
IntGetClassLongA(PWND Wnd, PCLS Class, int nIndex)
{
    ULONG_PTR Ret = 0;

    if (nIndex >= 0)
    {
        if (nIndex + sizeof(ULONG_PTR) < nIndex ||
            nIndex + sizeof(ULONG_PTR) > Class->cbclsExtra)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
        }
        else
            Ret = *(PULONG_PTR)((ULONG_PTR)(Class + 1) + nIndex);
    }
    else
    {
        switch (nIndex)
        {
            case GCL_CBWNDEXTRA:
                Ret = (ULONG_PTR)Class->cbwndExtra;
                break;

            case GCL_CBCLSEXTRA:
                Ret = (ULONG_PTR)Class->cbclsExtra;
                break;

            case GCL_HBRBACKGROUND:
                Ret = (ULONG_PTR)Class->hbrBackground;
                if (Ret != 0 && Ret < 0x4000)
                    Ret = (ULONG_PTR)GetSysColorBrush((ULONG)Ret - 1);
                break;

            case GCL_HMODULE:
                //ERR("Cls 0x%x GCL_HMODULE 0x%x\n", Wnd->pcls, Class->hModule);
                Ret = (ULONG_PTR)Class->hModule;
                break;

            case GCL_MENUNAME:
                Ret = (ULONG_PTR)Class->lpszClientAnsiMenuName;
                break;

            case GCL_STYLE:
                Ret = (ULONG_PTR)Class->style;
                break;

            case GCW_ATOM:
                Ret = (ULONG_PTR)Class->atomNVClassName;
                break;

            case GCLP_HCURSOR:
                Ret = Class->spcur ? (ULONG_PTR)((PPROCMARKHEAD)SharedPtrToUser(Class->spcur))->h : 0;
                break;

            case GCLP_HICON:
                Ret = Class->spicn ? (ULONG_PTR)((PPROCMARKHEAD)SharedPtrToUser(Class->spicn))->h : 0;
                break;

            case GCLP_HICONSM:
                Ret = Class->spicnSm ? (ULONG_PTR)((PPROCMARKHEAD)SharedPtrToUser(Class->spicnSm))->h : 0;
                break;

            case GCLP_WNDPROC:
                Ret = IntGetClsWndProc(Wnd, Class, TRUE);
                break;

            default:
                SetLastError(ERROR_INVALID_INDEX);
                break;
        }
    }

    return Ret;
}

static ULONG_PTR FASTCALL
IntGetClassLongW (PWND Wnd, PCLS Class, int nIndex)
{
    ULONG_PTR Ret = 0;

    if (nIndex >= 0)
    {
        if (nIndex + sizeof(ULONG_PTR) < nIndex ||
            nIndex + sizeof(ULONG_PTR) > Class->cbclsExtra)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
        }
        else
            Ret = *(PULONG_PTR)((ULONG_PTR)(Class + 1) + nIndex);
    }
    else
    {
        switch (nIndex)
        {
            case GCL_CBWNDEXTRA:
                Ret = (ULONG_PTR)Class->cbwndExtra;
                break;

            case GCL_CBCLSEXTRA:
                Ret = (ULONG_PTR)Class->cbclsExtra;
                break;

            case GCLP_HBRBACKGROUND:
                Ret = (ULONG_PTR)Class->hbrBackground;
                if (Ret != 0 && Ret < 0x4000)
                    Ret = (ULONG_PTR)GetSysColorBrush((ULONG)Ret - 1);
                break;

            case GCL_HMODULE:
                Ret = (ULONG_PTR)Class->hModule;
                break;

            case GCLP_MENUNAME:
                Ret = (ULONG_PTR)Class->lpszClientUnicodeMenuName;
                break;

            case GCL_STYLE:
                Ret = (ULONG_PTR)Class->style;
                break;

            case GCW_ATOM:
                Ret = (ULONG_PTR)Class->atomNVClassName;
                break;

            case GCLP_HCURSOR:
                Ret = Class->spcur ? (ULONG_PTR)((PPROCMARKHEAD)SharedPtrToUser(Class->spcur))->h : 0;
                break;

            case GCLP_HICON:
                Ret = Class->spicn ? (ULONG_PTR)((PPROCMARKHEAD)SharedPtrToUser(Class->spicn))->h : 0;
                break;

            case GCLP_HICONSM:
                Ret = Class->spicnSm ? (ULONG_PTR)((PPROCMARKHEAD)SharedPtrToUser(Class->spicnSm))->h : 0;
                break;

            case GCLP_WNDPROC:
                Ret = IntGetClsWndProc(Wnd, Class, FALSE);
                break;

            default:
                SetLastError(ERROR_INVALID_INDEX);
                break;
        }
    }

    return Ret;
}

/*
 * @implemented
 */
DWORD WINAPI
GetClassLongA(HWND hWnd, int nIndex)
{
    PWND Wnd;
    PCLS Class;
    ULONG_PTR Ret = 0;

    TRACE("%p %d\n", hWnd, nIndex);

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return 0;

    _SEH2_TRY
    {
        Class = DesktopPtrToUser(Wnd->pcls);
        if (Class != NULL)
        {
#ifdef _WIN64
            switch (nIndex)
            {
                case GCLP_HBRBACKGROUND:
                case GCLP_HCURSOR:
                case GCLP_HICON:
                case GCLP_HICONSM:
                case GCLP_HMODULE:
                case GCLP_MENUNAME:
                case GCLP_WNDPROC:
                    SetLastError(ERROR_INVALID_INDEX);
                    break;

                default:
                    Ret = IntGetClassLongA(Wnd, Class, nIndex);
                    break;
            }
#else
            Ret = IntGetClassLongA(Wnd, Class, nIndex);
#endif
        }
        else
        {
            WARN("Invalid class for hwnd 0x%p!\n", hWnd);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = 0;
    }
    _SEH2_END;

    return (DWORD)Ret;
}

/*
 * @implemented
 */
DWORD WINAPI
GetClassLongW ( HWND hWnd, int nIndex )
{
    PWND Wnd;
    PCLS Class;
    ULONG_PTR Ret = 0;

    TRACE("%p %d\n", hWnd, nIndex);

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return 0;

    _SEH2_TRY
    {
        Class = DesktopPtrToUser(Wnd->pcls);
        if (Class != NULL)
        {
#ifdef _WIN64
            switch (nIndex)
            {
                case GCLP_HBRBACKGROUND:
                case GCLP_HCURSOR:
                case GCLP_HICON:
                case GCLP_HICONSM:
                case GCLP_HMODULE:
                case GCLP_MENUNAME:
                case GCLP_WNDPROC:
                    SetLastError(ERROR_INVALID_INDEX);
                    break;

                default:
                    Ret = IntGetClassLongW(Wnd, Class, nIndex);
                    break;
            }
#else
            Ret = IntGetClassLongW(Wnd, Class, nIndex);
#endif
        }
        else
        {
            WARN("Invalid class for hwnd 0x%p!\n", hWnd);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = 0;
    }
    _SEH2_END;

    return (DWORD)Ret;
}

#ifdef _WIN64
/*
 * @implemented
 */
ULONG_PTR
WINAPI
GetClassLongPtrA(HWND hWnd,
                 INT nIndex)
{
    PWND Wnd;
    PCLS Class;
    ULONG_PTR Ret = 0;

    TRACE("%p %d\n", hWnd, nIndex);

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return 0;

    _SEH2_TRY
    {
        Class = DesktopPtrToUser(Wnd->pcls);
        if (Class != NULL)
        {
            Ret = IntGetClassLongA(Wnd, Class, nIndex);
        }
        else
        {
            WARN("Invalid class for hwnd 0x%p!\n", hWnd);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = 0;
    }
    _SEH2_END;

    return Ret;
}

/*
 * @implemented
 */
ULONG_PTR
WINAPI
GetClassLongPtrW(HWND hWnd,
                 INT nIndex)
{
    PWND Wnd;
    PCLS Class;
    ULONG_PTR Ret = 0;

    TRACE("%p %d\n", hWnd, nIndex);

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return 0;

    _SEH2_TRY
    {
        Class = DesktopPtrToUser(Wnd->pcls);
        if (Class != NULL)
        {
            Ret = IntGetClassLongW(Wnd, Class, nIndex);
        }
        else
        {
            WARN("Invalid class for hwnd 0x%p!\n", hWnd);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = 0;
    }
    _SEH2_END;

    return Ret;
}
#endif


/*
 * @implemented
 */
int WINAPI
GetClassNameA(
  HWND hWnd,
  LPSTR lpClassName,
  int nMaxCount)
{
    WCHAR tmpbuf[MAX_ATOM_LEN + 1];
    int len;

    if (nMaxCount <= 0) return 0;
    if (!GetClassNameW( hWnd, tmpbuf, sizeof(tmpbuf)/sizeof(WCHAR) )) return 0;
    RtlUnicodeToMultiByteN( lpClassName, nMaxCount - 1, (PULONG)&len, tmpbuf, strlenW(tmpbuf) * sizeof(WCHAR) );
    lpClassName[len] = 0;

    TRACE("%p class/atom: %s/%04x %x\n", hWnd,
        IS_ATOM(lpClassName) ? NULL : lpClassName,
        IS_ATOM(lpClassName) ? lpClassName : 0,
        nMaxCount);

    return len;
}


/*
 * @implemented
 */
int
WINAPI
GetClassNameW(
  HWND hWnd,
  LPWSTR lpClassName,
  int nMaxCount)
{
    UNICODE_STRING ClassName;
    int Result;

    RtlInitEmptyUnicodeString(&ClassName,
                              lpClassName,
                              nMaxCount * sizeof(WCHAR));

    Result = NtUserGetClassName(hWnd,
                                FALSE,
                                &ClassName);

    TRACE("%p class/atom: %S/%04x %x\n", hWnd,
        IS_ATOM(lpClassName) ? NULL : lpClassName,
        IS_ATOM(lpClassName) ? lpClassName : 0,
        nMaxCount);

    return Result;
}


/*
 * @implemented
 */
WORD
WINAPI
GetClassWord(
  HWND hwnd,
  int offset)
{
    PWND Wnd;
    PCLS class;
    WORD retvalue = 0;

    if (offset < 0) return GetClassLongA( hwnd, offset );

    Wnd = ValidateHwnd(hwnd);
    if (!Wnd)
        return 0;

    class = DesktopPtrToUser(Wnd->pcls);
    if (class == NULL) return 0;

    if (offset <= class->cbclsExtra - sizeof(WORD))
        memcpy( &retvalue, (char *)(class + 1) + offset, sizeof(retvalue) );
    else
        SetLastError( ERROR_INVALID_INDEX );

    return retvalue;
}


LONG_PTR IntGetWindowLong( HWND hwnd, INT offset, UINT size, BOOL unicode )
{
    LONG_PTR retvalue = 0;
    WND *wndPtr;

    if (offset == GWLP_HWNDPARENT)
    {
        HWND parent = GetAncestor( hwnd, GA_PARENT );
        if (parent == GetDesktopWindow()) parent = GetWindow( hwnd, GW_OWNER );
        return (ULONG_PTR)parent;
    }

    if (!(wndPtr = ValidateHwnd( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    if (offset >= 0 && wndPtr->fnid != FNID_DESKTOP)
    {
        if (offset > (int)(wndPtr->cbwndExtra - size))
        {
            WARN("Invalid offset %d\n", offset );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        retvalue = *((LONG_PTR *)((PCHAR)(wndPtr + 1) + offset));

        /* WINE: special case for dialog window procedure */
        //if ((offset == DWLP_DLGPROC) && (size == sizeof(LONG_PTR)) && (wndPtr->flags & WIN_ISDIALOG))
        //    retvalue = (LONG_PTR)IntGetWndProc( (WNDPROC)retvalue, unicode );
        return retvalue;
    }

    switch(offset)
    {
    case GWLP_USERDATA:  retvalue = wndPtr->dwUserData; break;
    case GWL_STYLE:      retvalue = wndPtr->style; break;
    case GWL_EXSTYLE:    retvalue = wndPtr->ExStyle; break;
    case GWLP_ID:        retvalue = wndPtr->IDMenu; break;
    case GWLP_HINSTANCE: retvalue = (ULONG_PTR)wndPtr->hModule; break;
#if 0
    /* -1 is an undocumented case which returns WW* */
    /* source: http://www.geoffchappell.com/studies/windows/win32/user32/structs/wnd/index.htm*/
    case -1:             retvalue = (ULONG_PTR)&wndPtr->ww; break;
#else
    /* We don't have a WW but WND already contains the same fields in the right order, */
    /* so we can return a pointer to its first field */
    case -1:             retvalue = (ULONG_PTR)&wndPtr->state; break;
#endif
    case GWLP_WNDPROC:
    {
        if (!TestWindowProcess(wndPtr))
        {
            SetLastError(ERROR_ACCESS_DENIED);
            retvalue = 0;
            ERR("Outside Access and Denied!\n");
            break;
        }
        retvalue = (ULONG_PTR)IntGetWndProc(wndPtr, !unicode);
        break;
    }
    default:
        WARN("Unknown offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        break;
    }
    return retvalue;

}
/*
 * @implemented
 */
LONG
WINAPI
GetWindowLongA ( HWND hWnd, int nIndex )
{
    return IntGetWindowLong( hWnd, nIndex, sizeof(LONG), FALSE );
}

/*
 * @implemented
 */
LONG
WINAPI
GetWindowLongW(HWND hWnd, int nIndex)
{
    return IntGetWindowLong( hWnd, nIndex, sizeof(LONG), TRUE );
}

#ifdef _WIN64
/*
 * @implemented
 */
LONG_PTR
WINAPI
GetWindowLongPtrA(HWND hWnd,
                  INT nIndex)
{
    return IntGetWindowLong( hWnd, nIndex, sizeof(LONG_PTR), FALSE );
}

/*
 * @implemented
 */
LONG_PTR
WINAPI
GetWindowLongPtrW(HWND hWnd,
                  INT nIndex)
{
    return IntGetWindowLong( hWnd, nIndex, sizeof(LONG_PTR), TRUE );

}
#endif // _WIN64

/*
 * @implemented
 */
WORD
WINAPI
GetWindowWord(HWND hWnd, int nIndex)
{
    switch(nIndex)
    {
    case GWLP_ID:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        break;
    default:
        if (nIndex < 0)
        {
            WARN("Invalid offset %d\n", nIndex );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        break;
    }
    return IntGetWindowLong( hWnd, nIndex, sizeof(WORD), FALSE );
}

/*
 * @implemented
 */
UINT
WINAPI
RealGetWindowClassW(
  HWND  hwnd,
  LPWSTR pszType,
  UINT  cchType)
{
    UNICODE_STRING ClassName;

    RtlInitEmptyUnicodeString(&ClassName,
                              pszType,
                              cchType * sizeof(WCHAR));

    return NtUserGetClassName(hwnd,TRUE,&ClassName);
}


/*
 * @implemented
 */
UINT
WINAPI
RealGetWindowClassA(
  HWND  hwnd,
  LPSTR pszType,
  UINT  cchType)
{
    WCHAR tmpbuf[MAX_ATOM_LEN + 1];
    UINT len;

    if ((INT)cchType <= 0) return 0;
    if (!RealGetWindowClassW( hwnd, tmpbuf, sizeof(tmpbuf)/sizeof(WCHAR) )) return 0;
    RtlUnicodeToMultiByteN( pszType, cchType - 1, (PULONG)&len, tmpbuf, strlenW(tmpbuf) * sizeof(WCHAR) );
    pszType[len] = 0;
    return len;
}

/*
 * Create a small icon based on a standard icon
 */
#if 0 // Keep vintage code from revision 18764 by GvG!
static HICON
CreateSmallIcon(HICON StdIcon)
{
    HICON SmallIcon = NULL;
    ICONINFO StdInfo;
    int SmallIconWidth;
    int SmallIconHeight;
    BITMAP StdBitmapInfo;
    HDC hSourceDc = NULL;
    HDC hDestDc = NULL;
    ICONINFO SmallInfo;
    HBITMAP OldSourceBitmap = NULL;
    HBITMAP OldDestBitmap = NULL;

    SmallInfo.hbmColor = NULL;
    SmallInfo.hbmMask = NULL;

    /* We need something to work with... */
    if (NULL == StdIcon)
    {
        goto cleanup;
    }

    SmallIconWidth = GetSystemMetrics(SM_CXSMICON);
    SmallIconHeight = GetSystemMetrics(SM_CYSMICON);
    if (! GetIconInfo(StdIcon, &StdInfo))
    {
        ERR("Failed to get icon info for icon 0x%x\n", StdIcon);
        goto cleanup;
    }
   if (! GetObjectW(StdInfo.hbmMask, sizeof(BITMAP), &StdBitmapInfo))
    {
        ERR("Failed to get bitmap info for icon 0x%x bitmap 0x%x\n",
                StdIcon, StdInfo.hbmColor);
        goto cleanup;
    }
    if (StdBitmapInfo.bmWidth == SmallIconWidth &&
        StdBitmapInfo.bmHeight == SmallIconHeight)
    {
        /* Icon already has the correct dimensions */
        return StdIcon;
    }

    hSourceDc = CreateCompatibleDC(NULL);
    if (NULL == hSourceDc)
    {
        ERR("Failed to create source DC\n");
        goto cleanup;
    }
    hDestDc = CreateCompatibleDC(NULL);
    if (NULL == hDestDc)
    {
        ERR("Failed to create dest DC\n");
        goto cleanup;
    }

    OldSourceBitmap = SelectObject(hSourceDc, StdInfo.hbmColor);
    if (NULL == OldSourceBitmap)
    {
        ERR("Failed to select source color bitmap\n");
        goto cleanup;
    }
    SmallInfo.hbmColor = CreateCompatibleBitmap(hSourceDc, SmallIconWidth,
                                                SmallIconHeight);
    if (NULL == SmallInfo.hbmColor)
    {
        ERR("Failed to create color bitmap\n");
        goto cleanup;
    }
    OldDestBitmap = SelectObject(hDestDc, SmallInfo.hbmColor);
    if (NULL == OldDestBitmap)
    {
        ERR("Failed to select dest color bitmap\n");
        goto cleanup;
    }
    if (! StretchBlt(hDestDc, 0, 0, SmallIconWidth, SmallIconHeight,
                     hSourceDc, 0, 0, StdBitmapInfo.bmWidth,
                     StdBitmapInfo.bmHeight, SRCCOPY))
    {
        ERR("Failed to stretch color bitmap\n");
        goto cleanup;
    }

    if (NULL == SelectObject(hSourceDc, StdInfo.hbmMask))
    {
        ERR("Failed to select source mask bitmap\n");
        goto cleanup;
    }
    SmallInfo.hbmMask = CreateCompatibleBitmap(hSourceDc, SmallIconWidth, SmallIconHeight);
    if (NULL == SmallInfo.hbmMask)
    {
        ERR("Failed to create mask bitmap\n");
        goto cleanup;
    }
    if (NULL == SelectObject(hDestDc, SmallInfo.hbmMask))
    {
        ERR("Failed to select dest mask bitmap\n");
        goto cleanup;
    }
    if (! StretchBlt(hDestDc, 0, 0, SmallIconWidth, SmallIconHeight,
                     hSourceDc, 0, 0, StdBitmapInfo.bmWidth,
                     StdBitmapInfo.bmHeight, SRCCOPY))
    {
        ERR("Failed to stretch mask bitmap\n");
        goto cleanup;
    }

    SmallInfo.fIcon = TRUE;
    SmallInfo.xHotspot = SmallIconWidth / 2;
    SmallInfo.yHotspot = SmallIconHeight / 2;
    SmallIcon = CreateIconIndirect(&SmallInfo);
    if (NULL == SmallIcon)
    {
        ERR("Failed to create icon\n");
        goto cleanup;
    }

cleanup:
    if (NULL != SmallInfo.hbmMask)
    {
        DeleteObject(SmallInfo.hbmMask);
    }
    if (NULL != OldDestBitmap)
    {
        SelectObject(hDestDc, OldDestBitmap);
    }
    if (NULL != SmallInfo.hbmColor)
    {
        DeleteObject(SmallInfo.hbmColor);
    }
    if (NULL != hDestDc)
    {
        DeleteDC(hDestDc);
    }
    if (NULL != OldSourceBitmap)
    {
        SelectObject(hSourceDc, OldSourceBitmap);
    }
    if (NULL != hSourceDc)
    {
        DeleteDC(hSourceDc);
    }

    return SmallIcon;
}
#endif

ATOM WINAPI
RegisterClassExWOWW(WNDCLASSEXW *lpwcx,
                    LPDWORD pdwWowData,
                    WORD fnID,
                    DWORD dwFlags,
                    BOOL ChkRegCls)
{
    ATOM Atom;
    WNDCLASSEXW WndClass;
    UNICODE_STRING ClassName;
    UNICODE_STRING ClassVersion;
    UNICODE_STRING MenuName = {0};
    CLSMENUNAME clsMenuName;
    ANSI_STRING AnsiMenuName;
    LPCWSTR lpszClsVersion;

    if (lpwcx == NULL || lpwcx->cbSize != sizeof(WNDCLASSEXW) ||
        lpwcx->cbClsExtra < 0 || lpwcx->cbWndExtra < 0 ||
        lpwcx->lpszClassName == NULL)
    {
        TRACE("RegisterClassExWOWW Invalid Parameter Error!\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (ChkRegCls)
    {
        if (!RegisterDefaultClasses) RegisterSystemControls();
    }
    /*
     * On real Windows this looks more like:
     *    if (lpwcx->hInstance == User32Instance &&
     *        *(PULONG)((ULONG_PTR)NtCurrentTeb() + 0x6D4) & 0x400)
     * But since I have no idea what the magic field in the
     * TEB structure means, I rather decided to omit that.
     * -- Filip Navara

        GetWin32ClientInfo()->dwExpWinVer & (WINVER == 0x400)
     */
    if (lpwcx->hInstance == User32Instance)
    {
        TRACE("RegisterClassExWOWW User32Instance!\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    /* Yes, this is correct. We should modify the passed structure. */
    if (lpwcx->hInstance == NULL)
       ((WNDCLASSEXW*)lpwcx)->hInstance = GetModuleHandleW(NULL);

    RtlCopyMemory(&WndClass, lpwcx, sizeof(WNDCLASSEXW));
/*
    if (NULL == WndClass.hIconSm)
    {
        WndClass.hIconSm = CreateSmallIcon(WndClass.hIcon);
    }
*/
    RtlInitEmptyAnsiString(&AnsiMenuName, NULL, 0);
    if (WndClass.lpszMenuName != NULL)
    {
        if (!IS_INTRESOURCE(WndClass.lpszMenuName))
        {
            if (WndClass.lpszMenuName[0])
            {
               RtlInitUnicodeString(&MenuName, WndClass.lpszMenuName);
               RtlUnicodeStringToAnsiString( &AnsiMenuName, &MenuName, TRUE);
            }
        }
        else
        {
            MenuName.Buffer = (LPWSTR)WndClass.lpszMenuName;
             AnsiMenuName.Buffer = (PCHAR)WndClass.lpszMenuName;
        }
    }

    if (IS_ATOM(WndClass.lpszClassName))
    {
        ClassName.Length =
        ClassName.MaximumLength = 0;
        ClassName.Buffer = (LPWSTR)WndClass.lpszClassName;
    }
    else
    {
        RtlInitUnicodeString(&ClassName, WndClass.lpszClassName);
    }

    ClassVersion = ClassName;
    if (fnID == 0)
    {
        lpszClsVersion = ClassNameToVersion(lpwcx->lpszClassName, NULL, NULL, NULL, FALSE);
        if (lpszClsVersion)
        {
            RtlInitUnicodeString(&ClassVersion, lpszClsVersion);
        }
    }

    clsMenuName.pszClientAnsiMenuName = AnsiMenuName.Buffer;
    clsMenuName.pwszClientUnicodeMenuName = MenuName.Buffer;
    clsMenuName.pusMenuName = &MenuName;

    Atom = NtUserRegisterClassExWOW( &WndClass,
                                     &ClassName,
                                     &ClassVersion,
                                     &clsMenuName,
                                     fnID,
                                     dwFlags,
                                     pdwWowData);

    TRACE("atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
           Atom, lpwcx->lpfnWndProc, lpwcx->hInstance, lpwcx->hbrBackground,
           lpwcx->style, lpwcx->cbClsExtra, lpwcx->cbWndExtra, WndClass);

    return Atom;
}

/*
 * @implemented
 */
ATOM WINAPI
RegisterClassExA(CONST WNDCLASSEXA *lpwcx)
{
    RTL_ATOM Atom;
    WNDCLASSEXW WndClass;
    WCHAR mname[MAX_BUFFER_LEN];
    WCHAR cname[MAX_BUFFER_LEN];

    RtlCopyMemory(&WndClass, lpwcx, sizeof(WNDCLASSEXA));

    if (WndClass.lpszMenuName != NULL)
    {
        if (!IS_INTRESOURCE(WndClass.lpszMenuName))
        {
            if (WndClass.lpszMenuName[0])
            {
                if (!MultiByteToWideChar( CP_ACP, 0, lpwcx->lpszMenuName, -1, mname, MAX_ATOM_LEN + 1 )) return 0;

                WndClass.lpszMenuName = mname;
            }
        }
    }

    if (!IS_ATOM(WndClass.lpszClassName))
    {
        if (!MultiByteToWideChar( CP_ACP, 0, lpwcx->lpszClassName, -1, cname, MAX_ATOM_LEN + 1 )) return 0;

        WndClass.lpszClassName = cname;
    }

    Atom = RegisterClassExWOWW( &WndClass,
                                0,
                                0,
                                CSF_ANSIPROC,
                                TRUE);

    TRACE("A atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d class=%p\n",
           Atom, lpwcx->lpfnWndProc, lpwcx->hInstance, lpwcx->hbrBackground,
           lpwcx->style, lpwcx->cbClsExtra, lpwcx->cbWndExtra, WndClass);

    return (ATOM)Atom;
}

/*
 * @implemented
 */
ATOM WINAPI
RegisterClassExW(CONST WNDCLASSEXW *lpwcx)
{
    ATOM Atom;

    Atom = RegisterClassExWOWW( (WNDCLASSEXW *)lpwcx, 0, 0, 0, TRUE);

    TRACE("W atom=%04x wndproc=%p hinst=%p bg=%p style=%08x clsExt=%d winExt=%d\n",
          Atom, lpwcx->lpfnWndProc, lpwcx->hInstance, lpwcx->hbrBackground,
          lpwcx->style, lpwcx->cbClsExtra, lpwcx->cbWndExtra);

    return Atom;
}

/*
 * @implemented
 */
ATOM WINAPI
RegisterClassA(CONST WNDCLASSA *lpWndClass)
{
    WNDCLASSEXA Class;

    if (lpWndClass == NULL)
        return 0;

    /* These MUST be copied manually, since on 64 bit architectures the
       alignment of the members is different between the 2 structs! */
    Class.style = lpWndClass->style;
    Class.lpfnWndProc = lpWndClass->lpfnWndProc;
    Class.cbClsExtra = lpWndClass->cbClsExtra;
    Class.cbWndExtra = lpWndClass->cbWndExtra;
    Class.hInstance = lpWndClass->hInstance;
    Class.hIcon = lpWndClass->hIcon;
    Class.hCursor = lpWndClass->hCursor;
    Class.hbrBackground = lpWndClass->hbrBackground;
    Class.lpszMenuName = lpWndClass->lpszMenuName;
    Class.lpszClassName = lpWndClass->lpszClassName;

    Class.cbSize = sizeof(WNDCLASSEXA);
    Class.hIconSm = NULL;

    return RegisterClassExA(&Class);
}

/*
 * @implemented
 */
ATOM WINAPI
RegisterClassW(CONST WNDCLASSW *lpWndClass)
{
    WNDCLASSEXW Class;

    if (lpWndClass == NULL)
        return 0;

    /* These MUST be copied manually, since on 64 bit architectures the
       alignment of the members is different between the 2 structs! */
    Class.style = lpWndClass->style;
    Class.lpfnWndProc = lpWndClass->lpfnWndProc;
    Class.cbClsExtra = lpWndClass->cbClsExtra;
    Class.cbWndExtra = lpWndClass->cbWndExtra;
    Class.hInstance = lpWndClass->hInstance;
    Class.hIcon = lpWndClass->hIcon;
    Class.hCursor = lpWndClass->hCursor;
    Class.hbrBackground = lpWndClass->hbrBackground;
    Class.lpszMenuName = lpWndClass->lpszMenuName;
    Class.lpszClassName = lpWndClass->lpszClassName;

    Class.cbSize = sizeof(WNDCLASSEXW);
    Class.hIconSm = NULL;

    return RegisterClassExW(&Class);
}

/*
 * @implemented
 */
DWORD
WINAPI
SetClassLongA (HWND hWnd,
               int nIndex,
               LONG dwNewLong)
{
    PSTR lpStr = (PSTR)(ULONG_PTR)dwNewLong;
    UNICODE_STRING Value = {0};
    BOOL Allocated = FALSE;
    DWORD Ret;

    /* FIXME - portability!!!! */

    if (nIndex == GCL_MENUNAME && lpStr != NULL)
    {
        if (!IS_INTRESOURCE(lpStr))
        {
            if (!RtlCreateUnicodeStringFromAsciiz(&Value,
                                                  lpStr))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }

            Allocated = TRUE;
        }
        else
            Value.Buffer = (PWSTR)lpStr;

        dwNewLong = (LONG_PTR)&Value;
    }
    else if (nIndex == GCW_ATOM && lpStr != NULL)
    {
        if (!IS_ATOM(lpStr))
        {
            if (!RtlCreateUnicodeStringFromAsciiz(&Value,
                                                  lpStr))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }

            Allocated = TRUE;
        }
        else
            Value.Buffer = (PWSTR)lpStr;

        dwNewLong = (LONG_PTR)&Value;
    }

    Ret = (DWORD)NtUserSetClassLong(hWnd,
                                    nIndex,
                                    dwNewLong,
                                    TRUE);

    if (Allocated)
    {
        RtlFreeUnicodeString(&Value);
    }

    return Ret;
}


/*
 * @implemented
 */
DWORD
WINAPI
SetClassLongW(HWND hWnd,
              int nIndex,
              LONG dwNewLong)
{
    PWSTR lpStr = (PWSTR)(ULONG_PTR)dwNewLong;
    UNICODE_STRING Value = {0};

    TRACE("%p %d %lx\n", hWnd, nIndex, dwNewLong);

    /* FIXME - portability!!!! */

    if (nIndex == GCL_MENUNAME && lpStr != NULL)
    {
        if (!IS_INTRESOURCE(lpStr))
        {
            RtlInitUnicodeString(&Value,
                                 lpStr);
        }
        else
            Value.Buffer = lpStr;

        dwNewLong = (LONG_PTR)&Value;
    }
    else if (nIndex == GCW_ATOM && lpStr != NULL)
    {
        if (!IS_ATOM(lpStr))
        {
            RtlInitUnicodeString(&Value,
                                 lpStr);
        }
        else
            Value.Buffer = lpStr;

        dwNewLong = (LONG_PTR)&Value;
    }

    return (DWORD)NtUserSetClassLong(hWnd,
                                     nIndex,
                                     dwNewLong,
                                     FALSE);
}

#ifdef _WIN64
/*
 * @unimplemented
 */
ULONG_PTR
WINAPI
SetClassLongPtrA(HWND hWnd,
                 INT nIndex,
                 LONG_PTR dwNewLong)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
ULONG_PTR
WINAPI
SetClassLongPtrW(HWND hWnd,
                 INT nIndex,
                 LONG_PTR dwNewLong)
{
    UNIMPLEMENTED;
    return 0;
}
#endif // _WIN64

/*
 * @implemented
 */
WORD
WINAPI
SetClassWord(
  HWND hWnd,
  int nIndex,
  WORD wNewWord)
/*
 * NOTE: Obsoleted in 32-bit windows
 */
{
    if ((nIndex < 0) && (nIndex != GCW_ATOM))
        return 0;

    return (WORD) SetClassLongW ( hWnd, nIndex, wNewWord );
}

/*
 * @implemented
 */
WORD
WINAPI
SetWindowWord ( HWND hWnd,int nIndex,WORD wNewWord )
{
    switch(nIndex)
    {
    case GWLP_ID:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        break;
    default:
        if (nIndex < 0)
        {
            WARN("Invalid offset %d\n", nIndex );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        break;
    }
    return NtUserSetWindowLong( hWnd, nIndex, wNewWord, FALSE );
}

/*
 * @implemented
 */
LONG
WINAPI
DECLSPEC_HOTPATCH
SetWindowLongA(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
    return NtUserSetWindowLong(hWnd, nIndex, dwNewLong, TRUE);
}

/*
 * @implemented
 */
LONG
WINAPI
SetWindowLongW(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
    return NtUserSetWindowLong(hWnd, nIndex, dwNewLong, FALSE);
}

#ifdef _WIN64
/*
 * @implemented
 */
LONG_PTR
WINAPI
SetWindowLongPtrA(HWND hWnd,
                  INT nIndex,
                  LONG_PTR dwNewLong)
{
    return NtUserSetWindowLongPtr(hWnd, nIndex, dwNewLong, TRUE);
}

/*
 * @implemented
 */
LONG_PTR
WINAPI
SetWindowLongPtrW(HWND hWnd,
                  INT nIndex,
                  LONG_PTR dwNewLong)
{
    return NtUserSetWindowLongPtr(hWnd, nIndex, dwNewLong, FALSE);
}
#endif

/*
 * @implemented
 */
BOOL
WINAPI
UnregisterClassA(
  LPCSTR lpClassName,
  HINSTANCE hInstance)
{
    UNICODE_STRING ClassName = {0};
    BOOL Ret;
    LPCWSTR lpszClsVersion;
    BOOL ConvertedString = FALSE;

    TRACE("class/atom: %s/%04x %p\n",
        IS_ATOM(lpClassName) ? NULL : lpClassName,
        IS_ATOM(lpClassName) ? lpClassName : 0,
        hInstance);

    lpszClsVersion = ClassNameToVersion(lpClassName, NULL, NULL, NULL, TRUE);
    if (lpszClsVersion)
    {
        RtlInitUnicodeString(&ClassName, lpszClsVersion);
    }
    else if (!IS_ATOM(lpClassName))
    {
        ConvertedString = TRUE;
        if (!RtlCreateUnicodeStringFromAsciiz(&ClassName, lpClassName))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }
    else
    {
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpClassName);
    }

    Ret = NtUserUnregisterClass(&ClassName, hInstance, 0);

    if (ConvertedString)
        RtlFreeUnicodeString(&ClassName);

    return Ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
UnregisterClassW(
  LPCWSTR lpClassName,
  HINSTANCE hInstance)
{
    UNICODE_STRING ClassName = {0};
    LPCWSTR lpszClsVersion;

    TRACE("class/atom: %S/%04x %p\n",
        IS_ATOM(lpClassName) ? NULL : lpClassName,
        IS_ATOM(lpClassName) ? lpClassName : 0,
        hInstance);

    lpszClsVersion = ClassNameToVersion(lpClassName, NULL, NULL, NULL, FALSE);
    if (lpszClsVersion)
    {
        RtlInitUnicodeString(&ClassName, lpszClsVersion);
    }
    else if (!IS_ATOM(lpClassName))
    {
        RtlInitUnicodeString(&ClassName, lpClassName);
    }
    else
    {
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpClassName);
    }

    return NtUserUnregisterClass(&ClassName, hInstance, 0);
}

/* EOF */
