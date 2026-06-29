/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/diransi.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
CreateDirectoryExA(IN LPCSTR lpTemplateDirectory,
                   IN LPCSTR lpNewDirectory,
                   IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    PUNICODE_STRING TemplateDirectoryW;
    UNICODE_STRING NewDirectoryW;
    BOOL ret;

    TemplateDirectoryW = Basep8BitStringToStaticUnicodeString(lpTemplateDirectory);
    if (!TemplateDirectoryW)
    {
        return FALSE;
    }

    if (!Basep8BitStringToDynamicUnicodeString(&NewDirectoryW, lpNewDirectory))
    {
        return FALSE;
    }

    ret = CreateDirectoryExW(TemplateDirectoryW->Buffer,
                             NewDirectoryW.Buffer,
                             lpSecurityAttributes);

    RtlFreeUnicodeString(&NewDirectoryW);

    return ret;
}

/* EOF */
