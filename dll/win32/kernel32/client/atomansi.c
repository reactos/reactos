/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/atomansi.c
 * PURPOSE:         Atom functions (ANSI)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

/* Forward declarations from atom.c */
ATOM WINAPI InternalAddAtom(BOOLEAN Local, BOOLEAN Unicode, LPCSTR AtomName);
ATOM WINAPI InternalFindAtom(BOOLEAN Local, BOOLEAN Unicode, LPCSTR AtomName);

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
ATOM
WINAPI
GlobalAddAtomA(LPCSTR lpString)
{
    return InternalAddAtom(FALSE, FALSE, lpString);
}

/*
 * @implemented
 */
ATOM
WINAPI
GlobalFindAtomA(LPCSTR lpString)
{
    return InternalFindAtom(FALSE, FALSE, lpString);
}

/* EOF */
