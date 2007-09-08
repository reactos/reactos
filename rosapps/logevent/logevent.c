/*
 *  ReactOS Win32 Applications
 *  Copyright (C) 2007 ReactOS Team
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
/* $Id: hostname.c 21664 2006-04-19 20:14:03Z gedmurphy $
 *
 * COPYRIGHT : See COPYING in the top level directory
 * PROJECT   : Event Logging Utility
 * FILE      : logevent.c
 * PROGRAMMER: Marc Piulachs (marc.piulachs at codexchange [dot] net)
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <tchar.h>
#include <stdarg.h>

TCHAR* m_MachineName    = NULL;
TCHAR* m_Text           = "No User Event Text";
TCHAR* m_Source         = "User Event";
WORD m_Severity         = EVENTLOG_INFORMATION_TYPE;
WORD m_Category         = 0;
DWORD m_EventID         = 1;

void
Usage(VOID)
{
    fputs("Usage: logevent.exe [-m \\MachineName] [options] \"Event Text\"", stderr);
    fputs("\n\n", stderr);
    fputs("Options:\n", stderr);
    fputs("  -s  Severity one of:\n", stderr);
    fputs("  \t(S)uccess\n", stderr);
    fputs("  \t(I)nformation\n", stderr);
    fputs("  \t(W)arning\n", stderr);
    fputs("  \t(E)rror\n", stderr);
    fputs("  \t(F)ailure\n", stderr);
    fputs("  -r  Source\n", stderr);
    fputs("  -c  Category number\n", stderr);
    fputs("  -e  Event ID\n", stderr);
    fputs("  /?  Help\n", stderr);
}

void 
WriteEvent (VOID)
{
    HANDLE hAppLog;
    BOOL bSuccess;
    LPCTSTR arrLogEntry[] = { m_Text }; //writing just one entry

    /* Get a handle to the Application event log */
    hAppLog = RegisterEventSource(
        (LPCSTR)m_MachineName,    /* machine  */
        (LPCSTR)m_Source);        /* source name */

    /* Now report the event, which will add this event to the event log */
    bSuccess = ReportEvent(
        hAppLog,                 /* event-log handle                */
        m_Severity,              /* event type                      */
        m_Category,              /* category                        */
        m_EventID,               /* event ID                        */
        NULL,                    /* no user SID                     */
        1,                       /* number of substitution strings  */
        0,                       /* no binary data                  */
        arrLogEntry,             /* string array                    */
        NULL);                   /* address of data                 */

    DeregisterEventSource(hAppLog);

    return;
}

/* Parse command line parameters */
static BOOL ParseCmdline(int argc, TCHAR **argv)
{
    INT i;
    BOOL ShowUsage;
    BOOL FoundEventText;
    BOOL InvalidOption;

    if (argc < 2) {
        ShowUsage = TRUE;
    } else {
        ShowUsage = FALSE;
    }

    FoundEventText = FALSE;
    InvalidOption = FALSE;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '/') {
            switch (argv[i][1]) {
                case 's':
                case 'S':
                    switch (argv[i + 1][0])
                    {
                        case 's':
                        case 'S':
                            m_Severity = EVENTLOG_SUCCESS;
                            i++;
                            break;
                        case 'i':
                        case 'I':
                            m_Severity = EVENTLOG_INFORMATION_TYPE;
                            i++;
                            break;
                        case 'w':
                        case 'W':
                            m_Severity = EVENTLOG_WARNING_TYPE;
                            i++;
                            break;
                        case 'e':
                        case 'E':
                            m_Severity = EVENTLOG_ERROR_TYPE;
                            i++;
                            break;
                        case 'f':
                        case 'F':
                            m_Severity = EVENTLOG_ERROR_TYPE;
                            i++;
                            break;
                        default:
                            printf("Bad option %s.\n", argv[i]);
                            Usage();
                            return FALSE;
                    }
                    break;
                case 'm':
                case 'M':
                    m_MachineName = argv[i + 1];
                    i++;
                    break;
                case 'r':
                case 'R':
                    m_Source = argv[i + 1];
                    i++;
                    break;
                case 'c':
                case 'C':
                    m_Category = atoi(argv[i + 1]);
                    i++;
                    break;
                case 'e':
                case 'E':
                    m_EventID  = atoi(argv[i + 1]);
                    i++;
                    break;
                case '?':
                    ShowUsage = TRUE;
                    break;
                default:
                    printf("Bad option %s.\n", argv[i]);
                    Usage();
                    return FALSE;
            }
            if (InvalidOption) {
                printf("Bad option format %s.\n", argv[i]);
                return FALSE;
            }
        } else {
            if (FoundEventText) {
                printf("Bad parameter %s.\n", argv[i]);
                return FALSE;
            } else {
                m_Text = argv[i];
                FoundEventText = TRUE;
            }
        }
    }

    if ((!ShowUsage) && (!FoundEventText)) {
        printf("The event text must be specified.\n");
        return FALSE;
    }

    if (ShowUsage) {
        Usage();
        return FALSE;
    }

    return TRUE;
}

int main(int argc, char **argv)
{
    if (ParseCmdline(argc, argv)) 
        WriteEvent ();

    return 0;
}
