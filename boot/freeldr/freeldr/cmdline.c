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
    PCSTR DebugString;
    PCSTR DefaultOs;
    LONG  TimeOut;
} CMDLINEINFO, *PCMDLINEINFO;

CCHAR DebugString[256];
CCHAR DefaultOs[256];
CMDLINEINFO CmdLineInfo;

/* FUNCTIONS ******************************************************************/

VOID
CmdLineParse(IN PCSTR CmdLine)
{
    PCHAR End, Setting;
    ULONG_PTR Length, Offset = 0;

    /* Set defaults */
    CmdLineInfo.DebugString = NULL;
    CmdLineInfo.DefaultOs = NULL;
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
        Setting += sizeof("debug=") - sizeof(ANSI_NULL);
        End = strstr(Setting, " ");
        Length = (End ? (End - Setting) : strlen(Setting));

        /* Copy the debug string and upcase it */
        RtlStringCbCopyNA(DebugString, sizeof(DebugString), Setting, Length);
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
    if (Setting)
    {
        CmdLineInfo.TimeOut = atoi(Setting +
                                   sizeof("timeout=") - sizeof(ANSI_NULL));
    }

    /* Get default OS */
    Setting = strstr(CmdLine, "defaultos=");
    if (Setting)
    {
        /* Check if there are more command-line parameters following */
        Setting += sizeof("defaultos=") - sizeof(ANSI_NULL);
        End = strstr(Setting, " ");
        Length = (End ? (End - Setting) : strlen(Setting));

        /* Copy the default OS */
        RtlStringCbCopyNA(DefaultOs, sizeof(DefaultOs), Setting, Length);
        CmdLineInfo.DefaultOs = DefaultOs;
    }

    /* Get ramdisk base address */
    Setting = strstr(CmdLine, "rdbase=");
    if (Setting)
    {
        gInitRamDiskBase =
            (PVOID)(ULONG_PTR)strtoull(Setting +
                                       sizeof("rdbase=") - sizeof(ANSI_NULL),
                                       NULL, 0);
    }

    /* Get ramdisk size */
    Setting = strstr(CmdLine, "rdsize=");
    if (Setting)
    {
        gInitRamDiskSize = strtoul(Setting +
                                   sizeof("rdsize=") - sizeof(ANSI_NULL),
                                   NULL, 0);
    }

    /* Get ramdisk offset */
    Setting = strstr(CmdLine, "rdoffset=");
    if (Setting)
    {
        Offset = strtoul(Setting +
                         sizeof("rdoffset=") - sizeof(ANSI_NULL),
                         NULL, 0);
    }

    /* Fix it up */
    gInitRamDiskBase = (PVOID)((ULONG_PTR)gInitRamDiskBase + Offset);
}

PCSTR
CmdLineGetDebugString(VOID)
{
    return CmdLineInfo.DebugString;
}

PCSTR
CmdLineGetDefaultOS(VOID)
{
    return CmdLineInfo.DefaultOs;
}

LONG
CmdLineGetTimeOut(VOID)
{
    return CmdLineInfo.TimeOut;
}
