/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/freeldr/cmdline.c
 * PURPOSE:         FreeLDR Command Line Parsing
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

typedef struct tagCMDLINEINFO
{
    PCCH DebugString;
    PCCH DefaultOperatingSystem;
    LONG TimeOut;
} CMDLINEINFO, *PCMDLINEINFO;

CCHAR DebugString[256];
CCHAR DefaultOs[256];
CMDLINEINFO CmdLineInfo;

/* FUNCTIONS ******************************************************************/

VOID
CmdLineParse(IN PCCH CmdLine)
{
    PCHAR End, Setting;
    ULONG_PTR Length, Offset = 0;

    /* Set defaults */
    CmdLineInfo.DebugString = NULL;
    CmdLineInfo.DefaultOperatingSystem = NULL;
    CmdLineInfo.TimeOut = -1;

    /*
     * Get debug string, in the following format:
     * "debug=option1=XXX;option2=YYY;..."
     * and translate it into the format:
     * "OPTION1=XXX OPTION2=YYY ..."
     */
    Setting = strstr(CmdLine, "debug=");
    if (Setting)
    {
        /* Check if there are more command-line parameters following */
        Setting += sizeof("debug=") + sizeof(ANSI_NULL);
        End = strstr(Setting, " ");
        if (End)
            Length = End - Setting;
        else
            Length = sizeof(DebugString);

        /* Copy the debug string and upcase it */
        strncpy(DebugString, Setting, Length);
        DebugString[Length - 1] = ANSI_NULL;
        _strupr(DebugString);

        /* Replace all separators ';' by spaces */
        Setting = DebugString;
        while (*Setting)
        {
            if (*Setting == ';') *Setting = ' ';
            Setting++;
        }

        CmdLineInfo.DebugString = DebugString;
    }

    /* Get timeout */
    Setting = strstr(CmdLine, "timeout=");
    if (Setting) CmdLineInfo.TimeOut = atoi(Setting +
                                            sizeof("timeout=") +
                                            sizeof(ANSI_NULL));

    /* Get default OS */
    Setting = strstr(CmdLine, "defaultos=");
    if (Setting)
    {
        /* Check if there are more command-line parameters following */
        Setting += sizeof("defaultos=") + sizeof(ANSI_NULL);
        End = strstr(Setting, " ");
        if (End)
            Length = End - Setting;
        else
            Length = sizeof(DefaultOs);

        /* Copy the default OS */
        strncpy(DefaultOs, Setting, Length);
        DefaultOs[Length - 1] = ANSI_NULL;
        CmdLineInfo.DefaultOperatingSystem = DefaultOs;
    }

    /* Get ramdisk base address */
    Setting = strstr(CmdLine, "rdbase=");
    if (Setting) gRamDiskBase = (PVOID)(ULONG_PTR)strtoull(Setting +
                                               sizeof("rdbase=") -
                                               sizeof(ANSI_NULL),
                                               NULL,
                                               0);

    /* Get ramdisk size */
    Setting = strstr(CmdLine, "rdsize=");
    if (Setting) gRamDiskSize = strtoul(Setting +
                                        sizeof("rdsize=") -
                                        sizeof(ANSI_NULL),
                                        NULL,
                                        0);

    /* Get ramdisk offset */
    Setting = strstr(CmdLine, "rdoffset=");
    if (Setting) Offset = strtoul(Setting +
                                  sizeof("rdoffset=") -
                                  sizeof(ANSI_NULL),
                                  NULL,
                                  0);

    /* Fix it up */
    gRamDiskBase = (PVOID)((ULONG_PTR)gRamDiskBase + Offset);
}

PCCH
CmdLineGetDebugString(VOID)
{
    return CmdLineInfo.DebugString;
}

PCCH
CmdLineGetDefaultOS(VOID)
{
    return CmdLineInfo.DefaultOperatingSystem;
}

LONG
CmdLineGetTimeOut(VOID)
{
    return CmdLineInfo.TimeOut;
}
