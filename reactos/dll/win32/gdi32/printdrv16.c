/*
 * Implementation of some printer driver bits
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 Huw Davies
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_IO_H
# include <io.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "wine/wingdi16.h"
#include "winspool.h"
#include "winerror.h"
#include "winreg.h"
#include "wownt32.h"
#include "wine/debug.h"
#include "gdi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(print);

static const char PrinterModel[]      = "Printer Model";
static const char DefaultDevMode[]    = "Default DevMode";
static const char PrinterDriverData[] = "PrinterDriverData";
static const char Printers[]          = "System\\CurrentControlSet\\Control\\Print\\Printers\\";

/**********************************************************************
 *           QueryAbort   (GDI.155)
 *
 *  Calls the app's AbortProc function if avail.
 *
 * RETURNS
 * TRUE if no AbortProc avail or AbortProc wants to continue printing.
 * FALSE if AbortProc wants to abort printing.
 */
BOOL16 WINAPI QueryAbort16(HDC16 hdc16, INT16 reserved)
{
    BOOL ret = TRUE;
    HDC hdc = HDC_32( hdc16 );
    DC *dc = get_dc_ptr( hdc );

    if(!dc) {
        ERR("Invalid hdc %p\n", hdc);
	return FALSE;
    }
    if (dc->pAbortProc) ret = dc->pAbortProc(hdc, 0);
    release_dc_ptr( dc );
    return ret;
}


/**********************************************************************
 *           call_abort_proc16
 */
static BOOL CALLBACK call_abort_proc16( HDC hdc, INT code )
{
    ABORTPROC16 proc16;
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;
    proc16 = dc->pAbortProc16;
    release_dc_ptr( dc );
    if (proc16)
    {
        WORD args[2];
        DWORD ret;

        args[1] = HDC_16(hdc);
        args[0] = code;
        WOWCallback16Ex( (DWORD)proc16, WCB16_PASCAL, sizeof(args), args, &ret );
        return LOWORD(ret);
    }
    return TRUE;
}


/**********************************************************************
 *           SetAbortProc   (GDI.381)
 */
INT16 WINAPI SetAbortProc16(HDC16 hdc16, ABORTPROC16 abrtprc)
{
    HDC hdc = HDC_32( hdc16 );
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;
    dc->pAbortProc16 = abrtprc;
    dc->pAbortProc   = call_abort_proc16;
    release_dc_ptr( dc );
    return TRUE;
}


/****************** misc. printer related functions */

/*
 * The following function should implement a queing system
 */
struct hpq
{
    struct hpq 	*next;
    int		 tag;
    int		 key;
};

static struct hpq *hpqueue;

/**********************************************************************
 *           CreatePQ   (GDI.230)
 *
 */
HPQ16 WINAPI CreatePQ16(INT16 size)
{
#if 0
    HGLOBAL16 hpq = 0;
    WORD tmp_size;
    LPWORD pPQ;

    tmp_size = size << 2;
    if (!(hpq = GlobalAlloc16(GMEM_SHARE|GMEM_MOVEABLE, tmp_size + 8)))
       return 0xffff;
    pPQ = GlobalLock16(hpq);
    *pPQ++ = 0;
    *pPQ++ = tmp_size;
    *pPQ++ = 0;
    *pPQ++ = 0;
    GlobalUnlock16(hpq);

    return (HPQ16)hpq;
#else
    FIXME("(%d): stub\n",size);
    return 1;
#endif
}

/**********************************************************************
 *           DeletePQ   (GDI.235)
 *
 */
INT16 WINAPI DeletePQ16(HPQ16 hPQ)
{
    return GlobalFree16(hPQ);
}

/**********************************************************************
 *           ExtractPQ   (GDI.232)
 *
 */
INT16 WINAPI ExtractPQ16(HPQ16 hPQ)
{
    struct hpq *queue, *prev, *current, *currentPrev;
    int key = 0, tag = -1;
    currentPrev = prev = NULL;
    queue = current = hpqueue;
    if (current)
        key = current->key;

    while (current)
    {
        currentPrev = current;
        current = current->next;
        if (current)
        {
            if (current->key < key)
            {
                queue = current;
                prev = currentPrev;
            }
        }
    }
    if (queue)
    {
        tag = queue->tag;

        if (prev)
            prev->next = queue->next;
        else
            hpqueue = queue->next;
        HeapFree(GetProcessHeap(), 0, queue);
    }

    TRACE("%x got tag %d key %d\n", hPQ, tag, key);

    return tag;
}

/**********************************************************************
 *           InsertPQ   (GDI.233)
 *
 */
INT16 WINAPI InsertPQ16(HPQ16 hPQ, INT16 tag, INT16 key)
{
    struct hpq *queueItem = HeapAlloc(GetProcessHeap(), 0, sizeof(struct hpq));
    if(queueItem == NULL) {
        ERR("Memory exausted!\n");
        return FALSE;
    }
    queueItem->next = hpqueue;
    hpqueue = queueItem;
    queueItem->key = key;
    queueItem->tag = tag;

    FIXME("(%x %d %d): stub???\n", hPQ, tag, key);
    return TRUE;
}

/**********************************************************************
 *           MinPQ   (GDI.231)
 *
 */
INT16 WINAPI MinPQ16(HPQ16 hPQ)
{
    FIXME("(%x): stub\n", hPQ);
    return 0;
}

/**********************************************************************
 *           SizePQ   (GDI.234)
 *
 */
INT16 WINAPI SizePQ16(HPQ16 hPQ, INT16 sizechange)
{
    FIXME("(%x %d): stub\n", hPQ, sizechange);
    return -1;
}



/*
 * The following functions implement part of the spooling process to
 * print manager.  I would like to see wine have a version of print managers
 * that used LPR/LPD.  For simplicity print jobs will be sent to a file for
 * now.
 */
typedef struct PRINTJOB
{
    char	*pszOutput;
    char 	*pszTitle;
    HDC16  	hDC;
    HANDLE16 	hHandle;
    int		nIndex;
    int		fd;
} PRINTJOB, *PPRINTJOB;

#define MAX_PRINT_JOBS 1
#define SP_OK 1

static PPRINTJOB gPrintJobsTable[MAX_PRINT_JOBS];


static PPRINTJOB FindPrintJobFromHandle(HANDLE16 hHandle)
{
    return gPrintJobsTable[0];
}

static int CreateSpoolFile(LPCSTR pszOutput)
{
    int fd=-1;
    char psCmd[1024];
    const char *psCmdP = psCmd;
    HKEY hkey;

    /* TTD convert the 'output device' into a spool file name */

    if (pszOutput == NULL || *pszOutput == '\0')
      return -1;

    psCmd[0] = 0;
    /* @@ Wine registry key: HKCU\Software\Wine\Printing\Spooler */
    if(!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Printing\\Spooler", &hkey))
    {
        DWORD type, count = sizeof(psCmd);
        RegQueryValueExA(hkey, pszOutput, 0, &type, (LPBYTE)psCmd, &count);
        RegCloseKey(hkey);
    }
    if (!psCmd[0] && !strncmp("LPR:",pszOutput,4))
        sprintf(psCmd,"|lpr -P'%s'",pszOutput+4);

    TRACE("Got printerSpoolCommand '%s' for output device '%s'\n",
	  psCmd, pszOutput);
    if (!*psCmd)
        psCmdP = pszOutput;
    else
    {
        while (*psCmdP && isspace(*psCmdP))
        {
            psCmdP++;
        }
        if (!*psCmdP)
            return -1;
    }
    TRACE("command: '%s'\n", psCmdP);
#ifdef HAVE_FORK
    if (*psCmdP == '|')
    {
        int fds[2];
        if (pipe(fds)) {
	    ERR("pipe() failed!\n");
            return -1;
	}
        if (fork() == 0)
        {
            psCmdP++;

            TRACE("In child need to exec %s\n",psCmdP);
            close(0);
            dup2(fds[0],0);
            close (fds[1]);

            /* reset signals that we previously set to SIG_IGN */
            signal( SIGPIPE, SIG_DFL );
            signal( SIGCHLD, SIG_DFL );

            execl("/bin/sh", "/bin/sh", "-c", psCmdP, NULL);
            _exit(1);

        }
        close (fds[0]);
        fd = fds[1];
        TRACE("Need to execute a cmnd and pipe the output to it\n");
    }
    else
#endif
    {
        char *buffer;
        WCHAR psCmdPW[MAX_PATH];

        TRACE("Just assume it's a file\n");

        /**
         * The file name can be dos based, we have to find its
         * corresponding Unix file name.
         */
        MultiByteToWideChar(CP_ACP, 0, psCmdP, -1, psCmdPW, MAX_PATH);
        if ((buffer = wine_get_unix_file_name(psCmdPW)))
        {
            if ((fd = open(buffer, O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0)
            {
                ERR("Failed to create spool file '%s' ('%s'). (error %s)\n",
                    buffer, psCmdP, strerror(errno));
            }
            HeapFree(GetProcessHeap(), 0, buffer);
        }
    }
    return fd;
}

static int FreePrintJob(HANDLE16 hJob)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob;

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL)
    {
	gPrintJobsTable[pPrintJob->nIndex] = NULL;
	HeapFree(GetProcessHeap(), 0, pPrintJob->pszOutput);
	HeapFree(GetProcessHeap(), 0, pPrintJob->pszTitle);
	if (pPrintJob->fd >= 0) close(pPrintJob->fd);
	HeapFree(GetProcessHeap(), 0, pPrintJob);
	nRet = SP_OK;
    }
    return nRet;
}

/**********************************************************************
 *           OpenJob   (GDI.240)
 *
 */
HPJOB16 WINAPI OpenJob16(LPCSTR lpOutput, LPCSTR lpTitle, HDC16 hDC)
{
    HPJOB16 hHandle = (HPJOB16)SP_ERROR;
    PPRINTJOB pPrintJob;

    TRACE("'%s' '%s' %04x\n", lpOutput, lpTitle, hDC);

    pPrintJob = gPrintJobsTable[0];
    if (pPrintJob == NULL)
    {
	int fd;

	/* Try and create a spool file */
	fd = CreateSpoolFile(lpOutput);
	if (fd >= 0)
	{
	    pPrintJob = HeapAlloc(GetProcessHeap(), 0, sizeof(PRINTJOB));
            if(pPrintJob == NULL) {
                WARN("Memory exausted!\n");
                return hHandle;
            }

            hHandle = 1;

	    pPrintJob->pszOutput = HeapAlloc(GetProcessHeap(), 0, strlen(lpOutput)+1);
	    strcpy( pPrintJob->pszOutput, lpOutput );
	    if(lpTitle)
            {
	        pPrintJob->pszTitle = HeapAlloc(GetProcessHeap(), 0, strlen(lpTitle)+1);
	        strcpy( pPrintJob->pszTitle, lpTitle );
            }
	    pPrintJob->hDC = hDC;
	    pPrintJob->fd = fd;
	    pPrintJob->nIndex = 0;
	    pPrintJob->hHandle = hHandle;
	    gPrintJobsTable[pPrintJob->nIndex] = pPrintJob;
	}
    }
    TRACE("return %04x\n", hHandle);
    return hHandle;
}

/**********************************************************************
 *           CloseJob   (GDI.243)
 *
 */
INT16 WINAPI CloseJob16(HPJOB16 hJob)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob = NULL;

    TRACE("%04x\n", hJob);

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL)
    {
	/* Close the spool file */
	close(pPrintJob->fd);
	FreePrintJob(hJob);
	nRet  = 1;
    }
    return nRet;
}

/**********************************************************************
 *           WriteSpool   (GDI.241)
 *
 */
INT16 WINAPI WriteSpool16(HPJOB16 hJob, LPSTR lpData, INT16 cch)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob = NULL;

    TRACE("%04x %p %04x\n", hJob, lpData, cch);

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL && pPrintJob->fd >= 0 && cch)
    {
	if (write(pPrintJob->fd, lpData, cch) != cch)
	  nRet = SP_OUTOFDISK;
	else
	  nRet = cch;
#if 0
	/* FIXME: We just cannot call 16 bit functions from here, since we
	 * have acquired several locks (DC). And we do not really need to.
	 */
	if (pPrintJob->hDC == 0) {
	    TRACE("hDC == 0 so no QueryAbort\n");
	}
        else if (!(QueryAbort16(pPrintJob->hDC, (nRet == SP_OUTOFDISK) ? nRet : 0 )))
	{
	    CloseJob16(hJob); /* printing aborted */
	    nRet = SP_APPABORT;
	}
#endif
    }
    return nRet;
}

typedef INT (WINAPI *MSGBOX_PROC)( HWND, LPCSTR, LPCSTR, UINT );

/**********************************************************************
 *           WriteDialog   (GDI.242)
 *
 */
INT16 WINAPI WriteDialog16(HPJOB16 hJob, LPSTR lpMsg, INT16 cchMsg)
{
    HMODULE mod;
    MSGBOX_PROC pMessageBoxA;
    INT16 ret = 0;

    TRACE("%04x %04x '%s'\n", hJob,  cchMsg, lpMsg);

    if ((mod = GetModuleHandleA("user32.dll")))
    {
        if ((pMessageBoxA = (MSGBOX_PROC)GetProcAddress( mod, "MessageBoxA" )))
            ret = pMessageBoxA(0, lpMsg, "Printing Error", MB_OKCANCEL);
    }
    return ret;
}


/**********************************************************************
 *           DeleteJob  (GDI.244)
 *
 */
INT16 WINAPI DeleteJob16(HPJOB16 hJob, INT16 nNotUsed)
{
    int nRet;

    TRACE("%04x\n", hJob);

    nRet = FreePrintJob(hJob);
    return nRet;
}

/*
 * The following two function would allow a page to be sent to the printer
 * when it has been processed.  For simplicity they haven't been implemented.
 * This means a whole job has to be processed before it is sent to the printer.
 */

/**********************************************************************
 *           StartSpoolPage   (GDI.246)
 *
 */
INT16 WINAPI StartSpoolPage16(HPJOB16 hJob)
{
    FIXME("StartSpoolPage GDI.246 unimplemented\n");
    return 1;

}


/**********************************************************************
 *           EndSpoolPage   (GDI.247)
 *
 */
INT16 WINAPI EndSpoolPage16(HPJOB16 hJob)
{
    FIXME("EndSpoolPage GDI.247 unimplemented\n");
    return 1;
}


/**********************************************************************
 *           GetSpoolJob   (GDI.245)
 *
 */
DWORD WINAPI GetSpoolJob16(int nOption, LONG param)
{
    DWORD retval = 0;
    TRACE("In GetSpoolJob param 0x%x noption %d\n",param, nOption);
    return retval;
}


/******************************************************************
 *                  DrvGetPrinterDataInternal
 *
 * Helper for DrvGetPrinterData
 */
static DWORD DrvGetPrinterDataInternal(LPSTR RegStr_Printer,
LPBYTE lpPrinterData, int cbData, int what)
{
    DWORD res = -1;
    HKEY hkey;
    DWORD dwType, cbQueryData;

    if (!(RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey))) {
        if (what == INT_PD_DEFAULT_DEVMODE) { /* "Default DevMode" */
            if (!(RegQueryValueExA(hkey, DefaultDevMode, 0, &dwType, 0, &cbQueryData))) {
                if (!lpPrinterData)
		    res = cbQueryData;
		else if ((cbQueryData) && (cbQueryData <= cbData)) {
		    cbQueryData = cbData;
		    if (RegQueryValueExA(hkey, DefaultDevMode, 0,
				&dwType, lpPrinterData, &cbQueryData))
		        res = cbQueryData;
		}
	    }
	} else { /* "Printer Driver" */
	    cbQueryData = 32;
	    RegQueryValueExA(hkey, "Printer Driver", 0,
			&dwType, lpPrinterData, &cbQueryData);
	    res = cbQueryData;
	}
    }
    if (hkey) RegCloseKey(hkey);
    return res;
}

/******************************************************************
 *                DrvGetPrinterData     (GDI.282)
 *
 */
DWORD WINAPI DrvGetPrinterData16(LPSTR lpPrinter, LPSTR lpProfile,
                               LPDWORD lpType, LPBYTE lpPrinterData,
                               int cbData, LPDWORD lpNeeded)
{
    LPSTR RegStr_Printer;
    HKEY hkey = 0, hkey2 = 0;
    DWORD res = 0;
    DWORD dwType, PrinterAttr, cbPrinterAttr, SetData, size;

    if (HIWORD(lpPrinter))
            TRACE("printer %s\n",lpPrinter);
    else
            TRACE("printer %p\n",lpPrinter);
    if (HIWORD(lpProfile))
            TRACE("profile %s\n",lpProfile);
    else
            TRACE("profile %p\n",lpProfile);
    TRACE("lpType %p\n",lpType);

    if ((!lpPrinter) || (!lpProfile) || (!lpNeeded))
	return ERROR_INVALID_PARAMETER;

    RegStr_Printer = HeapAlloc(GetProcessHeap(), 0,
                               strlen(Printers) + strlen(lpPrinter) + 2);
    strcpy(RegStr_Printer, Printers);
    strcat(RegStr_Printer, lpPrinter);

    if ((PtrToUlong(lpProfile) == INT_PD_DEFAULT_DEVMODE) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, DefaultDevMode)))) {
	size = DrvGetPrinterDataInternal(RegStr_Printer, lpPrinterData, cbData,
					 INT_PD_DEFAULT_DEVMODE);
	if (size+1) {
	    *lpNeeded = size;
	    if ((lpPrinterData) && (*lpNeeded > cbData))
		res = ERROR_MORE_DATA;
	}
	else res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    if ((PtrToUlong(lpProfile) == INT_PD_DEFAULT_MODEL) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, PrinterModel)))) {
	*lpNeeded = 32;
	if (!lpPrinterData) goto failed;
	if (cbData < 32) {
	    res = ERROR_MORE_DATA;
	    goto failed;
	}
	size = DrvGetPrinterDataInternal(RegStr_Printer, lpPrinterData, cbData,
					 INT_PD_DEFAULT_MODEL);
	if ((size+1) && (lpType))
	    *lpType = REG_SZ;
	else
	    res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    {
	if ((res = RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey)))
	    goto failed;
        cbPrinterAttr = 4;
        if ((res = RegQueryValueExA(hkey, "Attributes", 0,
                        &dwType, (LPBYTE)&PrinterAttr, &cbPrinterAttr)))
	    goto failed;
	if ((res = RegOpenKeyA(hkey, PrinterDriverData, &hkey2)))
	    goto failed;
        *lpNeeded = cbData;
        res = RegQueryValueExA(hkey2, lpProfile, 0,
                lpType, lpPrinterData, lpNeeded);
        if ((res != ERROR_CANTREAD) &&
         ((PrinterAttr &
        (PRINTER_ATTRIBUTE_ENABLE_BIDI|PRINTER_ATTRIBUTE_NETWORK))
        == PRINTER_ATTRIBUTE_NETWORK))
        {
	    if (!(res) && (*lpType == REG_DWORD) && (*(LPDWORD)lpPrinterData == -1))
	        res = ERROR_INVALID_DATA;
	}
	else
        {
	    SetData = -1;
	    RegSetValueExA(hkey2, lpProfile, 0, REG_DWORD, (LPBYTE)&SetData, 4); /* no result returned */
	}
    }

failed:
    if (hkey2) RegCloseKey(hkey2);
    if (hkey) RegCloseKey(hkey);
    HeapFree(GetProcessHeap(), 0, RegStr_Printer);
    return res;
}


/******************************************************************
 *                 DrvSetPrinterData     (GDI.281)
 *
 */
DWORD WINAPI DrvSetPrinterData16(LPSTR lpPrinter, LPSTR lpProfile,
                               DWORD lpType, LPBYTE lpPrinterData,
                               DWORD dwSize)
{
    LPSTR RegStr_Printer;
    HKEY hkey = 0;
    DWORD res = 0;

    if (HIWORD(lpPrinter))
            TRACE("printer %s\n",lpPrinter);
    else
            TRACE("printer %p\n",lpPrinter);
    if (HIWORD(lpProfile))
            TRACE("profile %s\n",lpProfile);
    else
            TRACE("profile %p\n",lpProfile);
    TRACE("lpType %08x\n",lpType);

    if ((!lpPrinter) || (!lpProfile) ||
    (PtrToUlong(lpProfile) == INT_PD_DEFAULT_MODEL) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, PrinterModel))))
	return ERROR_INVALID_PARAMETER;

    RegStr_Printer = HeapAlloc(GetProcessHeap(), 0,
			strlen(Printers) + strlen(lpPrinter) + 2);
    strcpy(RegStr_Printer, Printers);
    strcat(RegStr_Printer, lpPrinter);

    if ((PtrToUlong(lpProfile) == INT_PD_DEFAULT_DEVMODE) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, DefaultDevMode)))) {
	if ( RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey)
	     != ERROR_SUCCESS ||
	     RegSetValueExA(hkey, DefaultDevMode, 0, REG_BINARY,
			      lpPrinterData, dwSize) != ERROR_SUCCESS )
	        res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    {
	strcat(RegStr_Printer, "\\");

	if( (res = RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey)) ==
	    ERROR_SUCCESS ) {

	    if (!lpPrinterData)
	        res = RegDeleteValueA(hkey, lpProfile);
	    else
                res = RegSetValueExA(hkey, lpProfile, 0, lpType,
				       lpPrinterData, dwSize);
	}
    }

    if (hkey) RegCloseKey(hkey);
    HeapFree(GetProcessHeap(), 0, RegStr_Printer);
    return res;
}
