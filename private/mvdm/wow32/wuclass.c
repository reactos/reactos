/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUCLASS.C
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wuclass.c);

extern HANDLE ghInstanceUser32;
extern WORD   gUser16hInstance;

/*++
    BOOL GetClassInfo(<hInstance>, <lpClassName>, <lpWndClass>)
    HANDLE <hInstance>;
    LPSTR <lpClassName>;
    LPWNDCLASS <lpWndClass>;

    The %GetClassInfo% function retrieves information about a window class. The
    <hInstance> parameter identifies the instance of the application that
    created the class, and the <lpClassName> parameter identifies the window
    class. If the function locates the specified window class, it copies the
    %WNDCLASS% data used to register the window class to the %WNDCLASS%
    structure pointed to by the <lpWndClass> parameter.

    <hInstance>
        Identifies the instance of the application that created the class. To
        retrieve information on classes defined by Windows (such as buttons or
        list boxes), set hInstance to NULL.

    <lpClassName>
        Points to a null-terminated string that contains the name of the
        class to find. If the high-order word of this parameter is NULL, the
        low-order word is assumed to be a value returned by the
        %MAKEINTRESOURCE% macro used when the class was created.

    <lpWndClass>
        Points to the %WNDCLASS% structure that will receive the class
        information.

    The return value is TRUE if the function found a matching class and
    successfully copied the data; the return value is FALSE if the function did
    not find a matching class.

    The %lpszClassName%, %lpszMenuName%, and %hInstance% members of the
    %WNDCLASS% structure are <not> set by this function. The menu name is
    not stored internally and cannot be returned. The class name is already
    known since it is passed to this function. The %GetClassInfo% function
    returns all other fields with the values used when the class was
    registered.
--*/

ULONG FASTCALL WU32GetClassInfo(PVDMFRAME pFrame)
{
    ULONG       ul;
    PSZ psz2,   pszClass;
    WNDCLASS    t3;
    register    PGETCLASSINFO16 parg16;
    WORD        w;
    HINSTANCE   hInst;
    PWC         pwc = NULL;
    PWNDCLASS16 pwc16;
    CHAR        szAtomName[WOWCLASS_ATOM_NAME];

    GETARGPTR(pFrame, sizeof(GETCLASSINFO16), parg16);
    GETPSZIDPTR(parg16->f2, psz2);

    if ( HIWORD(psz2) == 0 ) {
        pszClass = szAtomName;
        GetAtomName( (ATOM)psz2, pszClass, WOWCLASS_ATOM_NAME );
    } else {
        pszClass = psz2;
    }

    // map hInst user16 to hMod user32
    if(parg16->f1 == gUser16hInstance) {
        hInst = ghInstanceUser32;
    }
    else {
        hInst = HMODINST32(parg16->f1);
    }

    ul = GETBOOL16(GetClassInfo(hInst, pszClass, &t3));

    // This fine piece of hackery mimicks the difference between the class list 
    // search algorithms in Win3.1 & SUR.  Essentially SUR checks the private,
    // public, and global class lists while Win3.1 only checks the private and
    // global lists.  Note we are striving for Win3.1 compatibility here -- not
    // always the logical thing!  Finding an existing *stale* class breaks some
    // apps!  Bug #31269    a-craigj, GerardoB
    // Restrict this hack to PageMaker 50a for now
    if(CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_FAKECLASSINFOFAIL) {
    if(ul && hInst) {
        
        // if this class wasn't registered by this app, AND it's not a global
        // class, then it must have come from the public list and this app 
        // wouldn't know about it on Win3.1 -- so lie and say it doesn't exist!
        // Note: The high word is the *hModule* which is what Win3.1 AND NT save
        //       with the class internally (not the hInstance!)
        if((HIWORD(t3.hInstance) != HIWORD(hInst)) && !(t3.style & CS_GLOBALCLASS)) {
            WOW32WARNMSGF(0,("\nWOW:GetClassInfo force failure hack:\n   class = '%s'  wc.hInstance = %X  app_hInst = %X\n\n", pszClass, t3.hInstance, hInst));
            ul = 0;
        }
    }
    }

    if (ul) {

        //
        // If the class is a 'standard' class, replace the class proc
        // with a thunk proc
        //

        GETVDMPTR(parg16->f3, sizeof(WNDCLASS16), pwc16);
        STOREWORD(pwc16->style,          t3.style);
        if (!WOW32_stricmp(pszClass, "edit")) {
            STOREWORD(pwc16->style, (FETCHWORD(pwc16->style) & ~CS_GLOBALCLASS));
        }

        STOREDWORD(pwc16->vpfnWndProc,  0);

        // if this class was registered by WOW
        if (IsWOWProc (t3.lpfnWndProc)) {

            //Unmark the proc and restore the high bits from rpl field
            UnMarkWOWProc (t3.lpfnWndProc,pwc16->vpfnWndProc);

            STORESHORT(pwc16->cbClsExtra, t3.cbClsExtra );

        } else {
            pwc16->vpfnWndProc = GetThunkWindowProc((DWORD)t3.lpfnWndProc, pszClass, NULL, NULL);
            STORESHORT(pwc16->cbClsExtra, t3.cbClsExtra);
        }

#ifdef OLD_WAY
        if (parg16->f1 ||
            !(pwc16->vpfnWndProc = GetThunkWindowProc((DWORD)t3.lpfnWndProc, pszClass, NULL, NULL))) {

            pwc = FindClass16(pszClass, (HINST16)parg16->f1);
            STOREDWORD(pwc16->vpfnWndProc,   pwc->vpfnWndProc);
        }
#endif

        STORESHORT(pwc16->cbWndExtra,    t3.cbWndExtra);

        // Win3.1 copies the hInst passed in by the app into the WNDCLASS struct
        // unless hInst == NULL, in which case they copy user's hInst 
        if((!parg16->f1) || (t3.hInstance == ghInstanceUser32)) {
            w = gUser16hInstance;
        } else {
            w = VALIDHMOD(t3.hInstance);
            if(w != BOGUSGDT) {
                w = parg16->f1;
            }
        }
        STOREWORD(pwc16->hInstance, w);
        w = GETHICON16(t3.hIcon);        STOREWORD(pwc16->hIcon, w);
        w = GETHCURSOR16(t3.hCursor);    STOREWORD(pwc16->hCursor, w);
        w = ((ULONG)t3.hbrBackground > COLOR_ENDCOLORS) ?
                GETHBRUSH16(t3.hbrBackground) : (WORD)t3.hbrBackground;
        STOREWORD(pwc16->hbrBackground, w);

        // These are strange assignments.  We don't keep the class name or
        // menu name in 16-bit memory.  For class name, USER32 just returns
        // the value which is passed as the second parameter, which works
        // MOST of the time; we do the same.  For the menu name, USER32 just
        // returns the value which was passed when the class was registered.
        // There are some situations where these psz's might go out of scope
        // and no longer be valid when the application attempts to use them.
        // IF YOU EVER FIND AN APPLICATION WHICH GETS THE WRONG
        // THING AND IT FAILS BECAUSE OF IT, SOME NASTY HACKING WILL HAVE
        // TO BE DONE AND IT SHOULD BE DONE IN USER32 ALSO...
        // -BobDay
        //
        if ( pwc = FindClass16(pszClass, (HINST16)parg16->f1)) {
            STOREDWORD(pwc16->vpszMenuName,  pwc->vpszMenu);
        } else {
            STOREDWORD(pwc16->vpszMenuName, 0 );
        }

        STOREDWORD(pwc16->vpszClassName, parg16->f2);

        FLUSHVDMPTR(parg16->f3, sizeof(WNDCLASS16), pwc16);
        FREEVDMPTR(pwc16);
    }
    FREEPSZIDPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    LONG GetClassLong(<hwnd>, <nIndex>)
    HWND <hwnd>;
    int <nIndex>;

    The %GetClassLong% function retrieves the long value specified by the
    <nIndex> parameter from the %WNDCLASS% structure of the window specified by
    the <hwnd> parameter.

    <hwnd>
        Identifies the window.

    <nIndex>
        Specifies the byte offset of the value to be retrieved. It can also
        be the following value:

    GCL_WNDPROC
    Retrieves a long pointer to the window function.
    GCL_MENUNAME
    Retrieves a long pointer to the menu name.

    The return value specifies the value retrieved from the %WNDCLASS%
    structure.

    To access any extra four-byte values allocated when the window-class
    structure was created, use a positive byte offset as the index specified by
    the <nIndex> parameter. The first four-byte value in the extra space is at
    offset zero, the next four-byte value is at offset 4, and so on.
--*/

ULONG FASTCALL WU32GetClassLong(PVDMFRAME pFrame)
{
    ULONG ul;
    INT iOffset;
    HWND hwnd;
    register PWW pww;
    register PWC pwc;
    register PGETCLASSLONG16 parg16;

    GETARGPTR(pFrame, sizeof(GETCLASSLONG16), parg16);

    // Make sure Win32 didn't change offsets for GCL constants

#if (GCL_WNDPROC != (-24) || GCL_MENUNAME != (-8))
#error Win16/Win32 GCL constants differ
#endif

    // Make sure the 16-bit app is requesting allowable offsets

    iOffset = INT32(parg16->f2);
    WOW32ASSERT(iOffset >= 0 ||
        iOffset == GCL_WNDPROC ||
        iOffset == GCL_MENUNAME);

    hwnd = HWND32(parg16->f1);

    switch (iOffset) {
        case GCL_WNDPROC:
            {
                DWORD   dwProc32;

                dwProc32 = GetClassLong(hwnd, iOffset);

                if ( IsWOWProc (dwProc32)) {
                    if ( HIWORD(dwProc32) == WNDPROC_HANDLE ) {
                        //
                        // Class has a window proc which is really a handle
                        // to a proc.  This happens when there is some
                        // unicode to ansi transition or vice versa.
                        //
                        pww = FindPWW( hwnd);
                        if ( pww == NULL ) {
                            ul = 0;
                        } else {
                            ul = GetThunkWindowProc(dwProc32,NULL,pww,hwnd);
                        }
                    } else {
                        //
                        // Class already has a 16:16 address
                        //
                        //Unmark the proc and restore the high bits from rpl field
                        UnMarkWOWProc (dwProc32,ul);
                    }
                } else {
                    //
                    // Class has a 32-bit proc, return an allocated thunk
                    //
                    pww = FindPWW(hwnd);
                    if ( pww == NULL ) {
                        ul = 0;
                    } else {
                        ul = GetThunkWindowProc(dwProc32,NULL,pww,hwnd);
                    }
                }
            }
            break;

        case GCL_MENUNAME:
            if (pwc = FindPWC(hwnd)) {
                ul = pwc->vpszMenu;
            } else {
                ul = 0;
            }
            break;

        case GCL_CBCLSEXTRA:
            ul = GetClassLong(hwnd, GCL_CBCLSEXTRA);
            break;

        default:
            ul = GetClassLong(hwnd, iOffset);
            break;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    WORD GetClassWord(<hwnd>, <nIndex>)
    HWND <hwnd>;
    int <nIndex>;

    The %GetClassWord% function retrieves the word that is specified by the
    <nIndex> parameter from the %WNDCLASS% structure of the window specified by
    the <hwnd> parameter.

    <hwnd>
        Identifies the window.

    <nIndex>
        Specifies the byte offset of the value to be retrieved. It can also
        be one of the following values:

    GCL_CBCLSEXTRA
        Tells how many bytes of additional class information you have. For
        information on how to access this memory, see the following "Comments"
        section.

    GCL_CBWNDEXTRA
        Tells how many bytes of additional window information you have. For
        information on how to access this memory, see the following<>Comments
        section.

    GCL_HBRBACKGROUND
        Retrieves a handle to the background brush.

    GCL_HCURSOR
        Retrieves a handle to the cursor.

    GCL_HICON
        Retrieves a handle to the icon.

    GCL_HMODULE
        Retrieves a handle to the module.

    GCL_STYLE
        Retrieves the window-class style bits.

    The return value specifies the value retrieved from the %WNDCLASS%
    structure.

    To access any extra two-byte values allocated when the window-class
    structure was created, use a positive byte offset as the index specified by
    the <nIndex> parameter, starting at zero for the first two-byte value in the
    extra space, 2 for the next two-byte value and so on.
--*/

ULONG FASTCALL WU32GetClassWord(PVDMFRAME pFrame)
{
    ULONG  ul;
    HWND   hwnd;
    INT    iOffset;
    register PGETCLASSWORD16 parg16;

    GETARGPTR(pFrame, sizeof(GETCLASSWORD16), parg16);

    // Make sure Win32 didn't change offsets

#if (GCL_HBRBACKGROUND != (-10) || GCL_HCURSOR != (-12) || GCL_HICON != (-14) || GCL_HMODULE != (-16) || GCL_CBWNDEXTRA != (-18) || GCL_CBCLSEXTRA != (-20) || GCL_STYLE != (-26))
#error Win16/Win32 class-word constants differ
#endif

    // Make sure the 16-bit app is requesting allowable offsets
    // (It's just assertion code, so it doesn't have to be pretty! -JTP)

    iOffset = INT32(parg16->f2);
    WOW32ASSERT(iOffset >= 0 ||
        iOffset == GCL_HBRBACKGROUND ||
        iOffset == GCL_HCURSOR ||
        iOffset == GCL_HICON ||
        iOffset == GCL_HMODULE ||
        iOffset == GCL_CBWNDEXTRA ||
        iOffset == GCL_CBCLSEXTRA ||
        iOffset == GCL_STYLE ||
        iOffset == GCW_ATOM);

    hwnd = HWND32(parg16->f1);

    switch(iOffset) {
        case GCL_HBRBACKGROUND:
            ul = GetClassLong(hwnd, iOffset);
            if (ul > COLOR_ENDCOLORS)
                ul = GETHBRUSH16(ul);
            break;

        case GCL_HCURSOR:
            ul = GETHCURSOR16((HAND32)GetClassLong(hwnd, iOffset));
            break;

        case GCL_HICON:
            ul = GETHICON16((HAND32)GetClassLong(hwnd, iOffset));
            break;

        case GCL_HMODULE:
            ul = GetGCL_HMODULE(hwnd);
            break;

        case GCL_CBWNDEXTRA:
        case GCL_STYLE:
            ul = GetClassLong(hwnd, iOffset);
            break;

        case GCL_CBCLSEXTRA:
            ul = GetClassLong(hwnd, GCL_CBCLSEXTRA);

            break;


        default:
            ul = GetClassWord(hwnd, iOffset);
            break;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL RegisterClass(<lpWndClass>)
    LPWNDCLASS <lpWndClass>;

    The %RegisterClass% function registers a window class for subsequent use in
    calls to the %CreateWindow% function. The window class has the attributes
    defined by the contents of the structure pointed to by the <lpWndClass>
    parameter. If two classes with the same name are registered, the second
    attempt fails and the information for that class is ignored.

    <lpWndClass>
        Points to a %WNDCLASS% structure. The structure must be filled with
        the appropriate class attributes before being passed to the function.
        See the following "Comments" section for details.

    The return value specifies whether the window class is registered. It is
    TRUE if the class is registered. Otherwise, it is FALSE.

    The callback function must use the Pascal calling conventions and must be
    declared %FAR%.

    Callback Function:

    BOOL FAR PASCAL <WndProc>(<hwnd>, <wMsg>, <wParam>, <lParam>)
    HWND <hwnd>;
    WORD <wMsg>;
    WORD<wParam>;
    DWORD<lParam>;

    <WndProc> is a placeholder for the application-supplied function name. The
    actual name must be exported by including it in an %EXPORTS% statement in
    the application's module-definition file.

    <wMsg>
        Specifies the message number.

    <wParam>
        Specifies additional message-dependent information.

    <lParam>
        Specifies additional message-dependent information.

    The window function returns the result of the message processing. The
    possible return values depend on the actual message sent.
--*/

ULONG FASTCALL WU32RegisterClass(PVDMFRAME pFrame)
{
    ULONG ul;
    WNDCLASS t1;
    VPSZ    vpszMenu;
    PSZ pszMenu;
    PSZ pszClass;
    register PREGISTERCLASS16 parg16;
    CHAR    szAtomName[WOWCLASS_ATOM_NAME];
    WC      wc;

    GETARGPTR(pFrame, sizeof(REGISTERCLASS16), parg16);

    GETWNDCLASS16(parg16->vpWndClass, &t1);

    // Fix up the window words for apps that have hardcode values and did not
    // use the value from GetClassInfo when superclassing system proc.
    // Some items have been expanded from a WORD to a DWORD bug 22014
//    t1.cbWndExtra = (t1.cbWndExtra + 3) & ~3;

    vpszMenu = (VPSZ)t1.lpszMenuName;
    if (HIWORD(t1.lpszMenuName) != 0) {
        GETPSZPTR(t1.lpszMenuName, pszMenu);
        t1.lpszMenuName = pszMenu;
    }

    if (HIWORD(t1.lpszClassName) == 0) {
        pszClass = szAtomName;
        GetAtomName( (ATOM)t1.lpszClassName, pszClass, WOWCLASS_ATOM_NAME);
    } else {
        GETPSZPTR(t1.lpszClassName, pszClass);
    }

    t1.lpszClassName = pszClass;

    ul = 0;

    wc.vpszMenu = vpszMenu;
    wc.iClsExtra = 0;
    wc.hMod16 = WOWGetProcModule16((DWORD)t1.lpfnWndProc);

    // mark the proc as WOW proc and save the high bits in the RPL
    MarkWOWProc(t1.lpfnWndProc,t1.lpfnWndProc);

    // Validate hbrBackground, because apps can pass an invalid handle.
    // The GetGDI32 returns a non-null value even if h16 is invalid.
    //
    // if hbrBackground is not valid we set it to NULL if the apps
    // ExpWinVer is < 3.1. This behaviour is identical to WIN31 behaviour.
    //
    // We need to do this validation here because USER32 will fail
    // RegisterClass() if hbrBackground is not valid.
    //
    // known culprits: QuickCase:W (that comes with QcWin)
    //                 class "iconbutton".

    if ((DWORD)t1.hbrBackground > (DWORD)(COLOR_ENDCOLORS) &&
        GetObjectType(t1.hbrBackground) != OBJ_BRUSH) {
           if ((WORD)W32GetExpWinVer((HANDLE) LOWORD(t1.hInstance) ) < 0x030a)
                t1.hbrBackground = (HBRUSH)NULL;
    }

    ul = GETBOOL16((pfnOut.pfnRegisterClassWOWA)(&t1, (DWORD *)&wc));

    if (!ul) {
        LOGDEBUG(LOG_ALWAYS,("WOW: RegisterClass failed (\"%s\")\n", (LPSZ)pszClass));
        // WOW32ASSERT(ul);
    }

    FREEPSZPTR(pszClass);
    FREEPSZPTR(pszMenu);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    LONG SetClassLong(<hwnd>, <nIndex>, <dwNewLong>)
    HWND <hwnd>;
    int <nIndex>;
    DWORD <dwNewLong>;

    The %SetClassLong% function replaces the long value specified by the
    <nIndex> parameter in the %WNDCLASS% structure of the window specified by
    the <hwnd> parameter.

    <hwnd>
        Identifies the window.

    <nIndex>
        Specifies the byte offset of the word to be changed. It can also
        be one of the following values:

    GCL_MENUNAME
        Sets a new long pointer to the menu

    GCL_WNDPROC
        Sets a new long pointer to the window function.

    <dwNewLong>
        Specifies the replacement value.

    The return value specifies the previous value of the specified long
    integer.

    If the %SetClassLong% function and GCL_WNDPROC index are used to set a
    window function, the given function must have the window-function form and
    be exported in the module-definition file. See the %RegisterClass% function
    earlier in this chapter for details.

    Calling %SetClassLong% with the GCL_WNDPROC index creates a subclass of the
    window class that affects all windows subsequently created with the class.
    See Chapter 1, Window Manager Interface Functions, for more information on
    window subclassing. An application should not attempt to create a window
    subclass for standard Windows controls such as combo boxes and buttons.

    To access any extra two-byte values allocated when the window-class
    structure was created, use a positive byte offset as the index specified by
    the <nIndex> parameter, starting at zero for the first two-byte value in the
    extra space, 2 for the next two-byte value and so on.
--*/

ULONG FASTCALL WU32SetClassLong(PVDMFRAME pFrame)
{
    ULONG ul;
    INT iOffset;
    PSZ pszMenu;
    register PWC pwc;
    register PSETCLASSLONG16 parg16;

    GETARGPTR(pFrame, sizeof(SETCLASSLONG16), parg16);

    // Make sure Win32 didn't change offsets for GCL constants

#if (GCL_MENUNAME != (-8) || GCL_WNDPROC != (-24))
#error Win16/Win32 GCL constants differ
#endif

    // Make sure the 16-bit app is requesting allowable offsets

    iOffset = INT32(parg16->f2);

    WOW32ASSERT(iOffset >= 0 ||
                iOffset == GCL_WNDPROC ||
                iOffset == GCL_MENUNAME);

    ul = 0;

    switch (iOffset) {
        case GCL_WNDPROC:
            {
                DWORD   dwWndProc32Old;
                DWORD   dwWndProc32New;
                PWW     pww;

                // Look to see if the new 16:16 proc is a thunk for a 32-bit proc.
                dwWndProc32New = IsThunkWindowProc(LONG32(parg16->f3), NULL );

                if ( dwWndProc32New != 0 ) {
                    //
                    // They are attempting to set the window proc to an existing
                    // 16-bit thunk that is really just a thunk for a 32-bit
                    // routine.  We can just set it back to the 32-bit routine.
                    //
                    dwWndProc32Old = SetClassLong(HWND32(parg16->f1), GCL_WNDPROC, (LONG)dwWndProc32New);
                } else {
                    //
                    // They are attempting to set it to a real 16:16 proc.
                    //
                    LONG l;

                    l = LONG32(parg16->f3);

                    // mark the proc as WOW proc and save the high bits in the RPL
                    MarkWOWProc (l,l);

                    dwWndProc32Old = SetClassLong(HWND32(parg16->f1), GCL_WNDPROC, l);
                }

                if ( IsWOWProc (dwWndProc32Old)) {
                    if ( HIWORD(dwWndProc32Old) == WNDPROC_HANDLE ) {
                        //
                        // If the return value is a handle, then just thunk it.
                        //
                        pww = FindPWW(HWND32(parg16->f1));
                        if ( pww == NULL ) {
                            ul = 0;
                        } else {
                            ul = GetThunkWindowProc(dwWndProc32Old, NULL, pww, HWND32(parg16->f1));
                        }
                    } else {
                        //
                        // Previous proc was a 16:16 proc
                        // Unmark the proc and restore the high bits from rpl field

                        UnMarkWOWProc (dwWndProc32Old,ul);
                    }
                } else {
                    //
                    // Previous proc was a 32-bit proc, use an allocated thunk
                    //
                    pww = FindPWW(HWND32(parg16->f1));
                    if ( pww == NULL ) {
                        ul = 0;
                    } else {
                        ul = GetThunkWindowProc(dwWndProc32Old, NULL, pww, HWND32(parg16->f1));
                    }

                }
            }
            break;

        case GCL_MENUNAME:
            if (pwc = FindPWC(HWND32(parg16->f1))) {
                ul = pwc->vpszMenu;
                GETPSZPTR(parg16->f3, pszMenu);
                SETWC(HWND32(parg16->f1), GCL_WOWMENUNAME, parg16->f3);
                SetClassLong(HWND32(parg16->f1), GCL_MENUNAME, (LONG)pszMenu);
                FREEPSZPTR(pszMenu);
            }
            break;


        case GCL_CBCLSEXTRA:
            // apps shouldn't do this but of course some do!
            // (see GCW_CBCLSEXTRA notes in thunk for RegisterClass())
            WOW32WARNMSG(0, ("WOW:SetClassLong(): app changing cbClsExtra!"));

            // only allow this to be set by classes registered via WOW
            if(IsWOWProc (GetClassLong(HWND32(parg16->f1), GCL_WNDPROC))) {

                /*
                 * The hard stuff is now done in User.  FritzS
                 */

                ul = SetClassLong(HWND32(parg16->f1), iOffset, WORD32(parg16->f3));
                break;
            }
            else {
                ul = 0;  // no can do for non-WOW classes
            }
            break;

        default:
            ul = SetClassLong(HWND32(parg16->f1), iOffset, LONG32(parg16->f3));
            break;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    WORD SetClassWord(<hwnd>, <nIndex>, <wNewWord>)
    HWND <hwnd>;
    int <nIndex>;
    WORD <wNewWord>;

    The %SetClassWord% function replaces the word specified by the <nIndex>
    parameter in the %WNDCLASS% structure of the window specified by the <hwnd>
    parameter.

    <hwnd>
        Identifies the window.

    <nIndex>
        Specifies the byte offset of the word to be changed. It can also
        be one of the following values:

    GCL_CBCLSEXTRA
        Sets two new bytes of additional window-class data.

    GCL_CBWNDEXTRA
        Sets two new bytes of additional window-class data.

    GCL_HBRBACKGROUND
        Sets a new handle to a background brush.

    GCL_HCURSOR
        Sets a new handle to a cursor.

    GCL_HICON
        Sets a new handle to an icon.

    GCL_STYLE
        Sets a new style bit for the window class.

    <wNewWord>
        Specifies the replacement value.

    The return value specifies the previous value of the specified word.

    The %SetClassWord% function should be used with care. For example, it is
    possible to change the background color for a class by using %SetClassWord%,
    but this change does not cause all windows belonging to the class to be
    repainted immediately.

    To access any extra four-byte values allocated when the window-class
    structure was created, use a positive byte offset as the index specified by
    the <nIndex> parameter, starting at zero for the first four-byte value in
    the extra space, 4 for the next four-byte value and so on.
--*/

ULONG FASTCALL WU32SetClassWord(PVDMFRAME pFrame)
{
    ULONG  ul;
    HWND   hwnd;
    INT    iOffset;
    register PSETCLASSWORD16 parg16;

    GETARGPTR(pFrame, sizeof(SETCLASSWORD16), parg16);

    // Make sure Win32 didn't change offsets

#if (GCL_HBRBACKGROUND != (-10) || GCL_HCURSOR != (-12) || GCL_HICON != (-14) || GCL_CBWNDEXTRA != (-18) || GCL_CBCLSEXTRA != (-20) || GCL_STYLE != (-26))
#error Win16/Win32 GCW constants differ
#endif

    // Make sure the 16-bit app is requesting allowable offsets
    // (It's just assertion code, so it doesn't have to be pretty! -JTP)

    iOffset = INT32(parg16->f2);
    WOW32ASSERT(iOffset >= 0 ||
        iOffset == GCL_HBRBACKGROUND ||
        iOffset == GCL_HCURSOR ||
        iOffset == GCL_HICON ||
        iOffset == GCL_CBWNDEXTRA ||
        iOffset == GCL_CBCLSEXTRA ||
        iOffset == GCL_STYLE)

    hwnd = HWND32(parg16->f1);
    ul = WORD32(parg16->f3);

    switch(iOffset) {
        case GCL_HBRBACKGROUND:
            if (ul > COLOR_ENDCOLORS)
                ul = (LONG) HBRUSH32(ul);

            ul = SetClassLong(hwnd, iOffset, (LONG) ul);

            if (ul > COLOR_ENDCOLORS)
                ul = GETHBRUSH16(ul);
            break;

        case GCL_HCURSOR:
            ul = GETHCURSOR16(SetClassLong(hwnd, iOffset, (LONG)HCURSOR32(ul)));
            break;

        case GCL_HICON:
            ul = GETHICON16(SetClassLong(hwnd, iOffset, (LONG)HICON32(ul)));
            break;

        case GCL_HMODULE:
            ul = 0;         // not allowed to set this
            break;

        case GCL_CBWNDEXTRA:
        case GCL_STYLE:
            ul = SetClassLong(hwnd, iOffset, (LONG)ul);
            break;

        case GCL_CBCLSEXTRA:
            // apps shouldn't do this but of course some do!
            // (see GCW_CBCLSEXTRA notes in thunk for RegisterClass())
            WOW32WARNMSG(0, ("WOW:SetClassWord(): app changing cbClsExtra!"));

            // only allow this to be set by classes registered via WOW
            if(IsWOWProc (GetClassLong(hwnd, GCL_WNDPROC))) {

                ul = SetClassLong(hwnd, GCL_CBCLSEXTRA, (LONG)ul);
                /*
                 * The hard work is now done in User.  FritzS
                 */
            }
            else {
                ul = 0;  // no can do for non-WOW classes
            }
            break;

        default:
            ul = SetClassWord(hwnd, iOffset, (WORD)ul);
            break;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL  UnregisterClass(<lpClassName>, <hInstance>)

    The %UnregisterClass% function removes the window class specified by
    <lpClassName> from the window-class table, freeing the storage required for
    the class.

    <lpClassName>
        Points to a null-terminated string containing the class name. This class
        name must have been previously registered by calling the RegisterClass
        function with a valid hInstance field in the %WNDCLASS% structure
        parameter. Predefined classes, such as dialog-box controls, may not be
        unregistered.

    <hInstance>
        Identifies the instance of the module that created the class.

    The return value is TRUE if the function successfully removed the window
    class from the window-class table. It is FALSE if the class could not be
    found or if a window exists that was created with the class.

    Before using this function, destroy all windows created with the specified
    class.
--*/

#if 0 // intthunk
ULONG FASTCALL WU32UnregisterClass(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ pszClass;
    register PUNREGISTERCLASS16 parg16;

    GETARGPTR(pFrame, sizeof(UNREGISTERCLASS16), parg16);
    GETPSZIDPTR(parg16->vpszClass, pszClass);

    ul = GETBOOL16(UnregisterClass(
                    pszClass,
                    HMODINST32(parg16->hInstance)
                  ));

    FREEPSZIDPTR(pszClass);
    FREEARGPTR(parg16);
    RETURN(ul);
}
#endif
