/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/stop.c
 * PURPOSE:     Stops a service
 * COPYRIGHT:   Copyright 2005-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

BOOL DoStop(PMAIN_WND_INFO Info)
{
    BOOL ret = FALSE;
    HWND hProgDlg;

    hProgDlg = CreateProgressDialog(Info->hMainWnd,
                                    Info->CurrentService->lpServiceName);

    if (hProgDlg)
    {
        ret = Control(Info,
                      hProgDlg,
                      SERVICE_CONTROL_STOP);

        SendMessage(hProgDlg, WM_DESTROY, 0, 0);
    }

    return ret;
}
