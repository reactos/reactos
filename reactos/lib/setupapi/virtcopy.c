/*
 * SetupAPI virtual copy operations
 *
 * Copyright 2001 Andreas Mohr
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FIXME: we now rely on builtin setupapi.dll for dialog resources.
 *        This is bad ! We ought to have 16bit resource handling working.
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "wownt32.h"
#include "wingdi.h"
#include "winnls.h"
#include "setupapi.h"
#include "setupx16.h"
#include "setupapi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

static FARPROC16 VCP_Proc = NULL;
static LPARAM VCP_MsgRef = 0;

static BOOL VCP_opened = FALSE;

static VCPSTATUS vcp_status;

static HINSTANCE SETUPAPI_hInstance;

static WORD VCP_Callback( LPVOID obj, UINT16 msg, WPARAM16 wParam, LPARAM lParam, LPARAM lParamRef )
{
    WORD args[8];
    DWORD ret = OK;
    if (VCP_Proc)
    {
        args[7] = HIWORD(obj);
        args[6] = LOWORD(obj);
        args[5] = msg;
        args[4] = wParam;
        args[3] = HIWORD(lParam);
        args[2] = LOWORD(lParam);
        args[1] = HIWORD(lParamRef);
        args[0] = LOWORD(lParamRef);
        WOWCallback16Ex( (DWORD)VCP_Proc, WCB16_PASCAL, sizeof(args), args, &ret );
    }
    return (WORD)ret;
}

/****************************** VHSTR management ******************************/

/*
 * This is a totally braindead implementation for now;
 * I don't care about speed at all ! Size and implementation time
 * is much more important IMHO. I could have created some sophisticated
 * tree structure, but... what the hell ! :-)
 */
typedef struct {
    DWORD refcount;
    LPCSTR pStr;
} VHSTR_STRUCT;

static VHSTR_STRUCT **vhstrlist = NULL;
static VHSTR vhstr_alloc = 0;

#define VALID_VHSTR(x)		((x < vhstr_alloc) && (vhstrlist[x]) && (vhstrlist[x]->refcount))

/***********************************************************************
 *		vsmStringAdd (SETUPX.207)
 */
VHSTR WINAPI vsmStringAdd16(LPCSTR lpszName)
{
    VHSTR n;
    VHSTR index = 0xffff;
    HANDLE heap;

    TRACE("add string '%s'\n", lpszName);
    /* search whether string already inserted */
    TRACE("searching for existing string...\n");
    for (n = 0; n < vhstr_alloc; n++)
    {
	if ((vhstrlist[n]) && (vhstrlist[n]->refcount))
	{
		TRACE("checking item: %d\n", n);
	    if (!strcmp(vhstrlist[n]->pStr, lpszName))
	    {
		TRACE("found\n");
		vhstrlist[n]->refcount++;
		return n;
	    }
	}
    }

    /* hmm, not found yet, let's insert it */
    TRACE("inserting item\n");
    for (n = 0; n < vhstr_alloc; n++)
    {
	if ((!(vhstrlist[n])) || (!(vhstrlist[n]->refcount)))
	{
	    index = n;
	    break;
	}
    }
    heap = GetProcessHeap();
    if (n == vhstr_alloc) /* hmm, no free index found yet */
    {
	index = vhstr_alloc;
	vhstr_alloc += 20;

	if (vhstrlist)
	    vhstrlist = HeapReAlloc(heap, HEAP_ZERO_MEMORY, vhstrlist,
					sizeof(VHSTR_STRUCT *) * vhstr_alloc);
	else
	    vhstrlist = HeapAlloc(heap, HEAP_ZERO_MEMORY,
					sizeof(VHSTR_STRUCT *) * vhstr_alloc);
    }
    if (index == 0xffff)
	return 0xffff; /* failure */
    if (!vhstrlist[index])
	vhstrlist[index] = HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(VHSTR_STRUCT));
    vhstrlist[index]->refcount = 1;
    vhstrlist[index]->pStr = HeapAlloc(heap, 0, strlen(lpszName)+1);
    strcpy((LPSTR)vhstrlist[index]->pStr, lpszName);
    return index;
}

/***********************************************************************
 *		vsmStringDelete (SETUPX.206)
 */
INT16 WINAPI vsmStringDelete16(VHSTR vhstr)
{
    if (VALID_VHSTR(vhstr))
    {
	vhstrlist[vhstr]->refcount--;
	if (!vhstrlist[vhstr]->refcount)
	{
	    HeapFree(GetProcessHeap(), 0, (LPSTR)vhstrlist[vhstr]->pStr);
	    vhstrlist[vhstr]->pStr = NULL;
	}
	return VCPN_OK;
    }

    /* string not found */
    return VCPN_FAIL;
}

/*
 * vsmStringFind() - not exported from a standard SETUPX.DLL, it seems
 */
VHSTR WINAPI vsmStringFind16(LPCSTR lpszName)
{
    WORD n;
    for (n = 0; n < vhstr_alloc; n++)
	if ((vhstrlist[n]) && (vhstrlist[n]->refcount) && (!strcmp(vhstrlist[n]->pStr, lpszName)))
	    return n;
    return 0xffff;
}

/***********************************************************************
 *		vsmGetStringName (SETUPX.205)
 *
 * Pretty correct, I guess
 */
INT16 WINAPI vsmGetStringName16(VHSTR vhstr, LPSTR lpszBuffer, int cbBuffer)
{
    if (VALID_VHSTR(vhstr))
    {
	int len = strlen(vhstrlist[vhstr]->pStr)+1;
	if (cbBuffer >= len)
	{
	    if (lpszBuffer)
	        strcpy(lpszBuffer, vhstrlist[vhstr]->pStr);
	    return len;
	}
    }
    return VCPN_FAIL;
}

/***********************************************************************
 *		vsmStringCompare (not exported from a standard SETUPX.DLL, it seems)
 */
INT16 WINAPI vsmStringCompare16(VHSTR vhstrA, VHSTR vhstrB)
{
    if ((!VALID_VHSTR(vhstrA)) || (!VALID_VHSTR(vhstrB)))
	return VCPN_FAIL; /* correct ? */
    return strcmp(vhstrlist[vhstrA]->pStr, vhstrlist[vhstrB]->pStr);
}

/***********************************************************************
 *		vsmGetStringRawName (SETUPX.208)
 */
LPCSTR WINAPI vsmGetStringRawName16(VHSTR vhstr)
{
    return (VALID_VHSTR(vhstr)) ? vhstrlist[vhstr]->pStr : NULL;
}


/***************************** VIRTNODE management ****************************/
static LPVIRTNODE *pvnlist = NULL;
static DWORD vn_num = 0;
static DWORD vn_last = 0;

RETERR16 VCP_VirtnodeCreate(LPVCPFILESPEC vfsSrc, LPVCPFILESPEC vfsDst, WORD fl, LPARAM lParam, LPEXPANDVTBL lpExpandVtbl)
{
    HANDLE heap;
    LPVIRTNODE lpvn;
    RETERR16 cbres;

    while (vn_last < vn_num)
    {
	if (pvnlist[vn_last] == NULL)
	    break;
	vn_last++;
    }
    heap = GetProcessHeap();
    if (vn_last == vn_num)
    {
	vn_num += 20;
	if (pvnlist)
	    pvnlist = HeapReAlloc(heap, HEAP_ZERO_MEMORY, pvnlist,
		    		sizeof(LPVIRTNODE *) * vn_num);
	else
	    pvnlist = HeapAlloc(heap, HEAP_ZERO_MEMORY, 
		    		sizeof(LPVIRTNODE *) * vn_num);
    }
    pvnlist[vn_last] = HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(VIRTNODE));
    lpvn = pvnlist[vn_last];
    vn_last++;

    lpvn->cbSize = sizeof(VIRTNODE);

    if (vfsSrc)
        memcpy(&lpvn->vfsSrc, vfsSrc, sizeof(VCPFILESPEC));

    if (vfsDst)
        memcpy(&lpvn->vfsDst, vfsDst, sizeof(VCPFILESPEC));

    lpvn->fl = fl;
    lpvn->lParam = lParam;
    lpvn->lpExpandVtbl = lpExpandVtbl;

    lpvn->vhstrDstFinalName = 0xffff; /* FIXME: what is this ? */

    cbres = VCP_Callback(lpvn, VCPM_NODECREATE, 0, 0, VCP_MsgRef);
    lpvn->fl |= VFNL_CREATED;
    cbres = VCP_Callback(lpvn, VCPM_NODEACCEPT, 0, 0, VCP_MsgRef);

    return OK;
}

BOOL VCP_VirtnodeDelete(LPVIRTNODE lpvnDel)
{
    DWORD n;
    RETERR16 cbres;

    for (n = 0; n < vn_last; n++)
    {
	if (pvnlist[n] == lpvnDel)
	{
	    cbres = VCP_Callback(lpvnDel, VCPM_NODEDESTROY, 0, 0, VCP_MsgRef);
	    HeapFree(GetProcessHeap(), 0, lpvnDel);
	    pvnlist[n] = NULL;
	    return TRUE;
	}
    }
    return FALSE;
}

/***********************************************************************
 *		VcpOpen (SETUPX.200)
 *
 * Sets up a virtual copy operation.
 * This means that functions such as GenInstall()
 * create a VIRTNODE struct for every file to be touched in a .INF file
 * instead of actually touching the file.
 * The actual copy/move/rename gets started when VcpClose or
 * VcpFlush is called; several different callbacks are made
 * (copy, rename, open, close, version conflicts, ...) on every file copied.
 */
RETERR16 WINAPI VcpOpen16(VIFPROC vifproc, LPARAM lparamMsgRef)
{
    TRACE("(%p, %08lx)\n", vifproc, lparamMsgRef);
    if (VCP_opened)
	return ERR_VCP_BUSY;

    VCP_Proc = (FARPROC16)vifproc;
    VCP_MsgRef = lparamMsgRef;

    /* load SETUPAPI needed for dialog resources etc. */
    SETUPAPI_hInstance = LoadLibraryA("setupapi.dll");
    if (!SETUPAPI_hInstance)
    {
	ERR("Could not load sibling setupapi.dll\n");
	return ERR_VCP_NOMEM;
    }
    VCP_opened = TRUE;
    return OK;
}

/***********************************************************************
 *		VcpQueueCopy		[SETUPX.13]
 *
 * lpExpandVtbl seems to be deprecated.
 * fl are the CNFL_xxx and VNFL_xxx flags.
 * lParam are the VNLP_xxx flags.
 */
RETERR16 WINAPI VcpQueueCopy16(
	LPCSTR lpszSrcFileName, LPCSTR lpszDstFileName,
	LPCSTR lpszSrcDir, LPCSTR lpszDstDir,
	LOGDISKID16 ldidSrc, LOGDISKID16 ldidDst,
	LPEXPANDVTBL lpExpandVtbl,
	WORD fl, LPARAM lParam
)
{
    VCPFILESPEC vfsSrc, vfsDst;

    if (!VCP_opened)
	return ERR_VCP_NOTOPEN;

    TRACE("srcdir: %s, srcfile: %s, dstdir: %s, dstfile: %s\n",
      lpszSrcDir, lpszSrcFileName, lpszDstDir, lpszDstFileName);

    TRACE("ldidSrc == %d, ldidDst == %d\n", ldidSrc, ldidDst);

    vfsSrc.ldid = ldidSrc;
    vfsSrc.vhstrDir = vsmStringAdd16(lpszSrcDir);
    vfsSrc.vhstrFileName = vsmStringAdd16(lpszSrcFileName);

    vfsDst.ldid = ldidDst;
    vfsDst.vhstrDir = vsmStringAdd16(lpszDstDir);
    vfsDst.vhstrFileName = vsmStringAdd16(lpszDstFileName);

    return VCP_VirtnodeCreate(&vfsSrc, &vfsDst, fl, lParam,
		    lpExpandVtbl);
}

/***********************************************************************
 *		VcpQueueDelete		[SETUPX.17]
 *
 * Is lParamRef the same as lParam in VcpQueueCopy ?
 * Damn docu !! Err... which docu ?
 */
RETERR16 WINAPI VcpQueueDelete16(
	LPCSTR lpszDstFileName,
	LPCSTR lpszDstDir,
	LOGDISKID16 ldidDst,
	LPARAM lParamRef
)
{
    VCPFILESPEC vfsDst;

    if (!VCP_opened)
	return ERR_VCP_NOTOPEN;

    vfsDst.ldid = ldidDst;
    vfsDst.vhstrDir = vsmStringAdd16(lpszDstDir);
    vfsDst.vhstrFileName = vsmStringAdd16(lpszDstFileName);

    return VCP_VirtnodeCreate(NULL, &vfsDst, VNFL_DELETE, lParamRef, 0);
}

/***********************************************************************
 *		VcpQueueRename		[SETUPX.204]
 *
 */
RETERR16 WINAPI VcpQueueRename16(
	LPCSTR lpszSrcFileName, LPCSTR lpszDstFileName,
	LPCSTR lpszSrcDir, LPCSTR lpszDstDir,
	LOGDISKID16 ldidSrc, LOGDISKID16 ldidDst,
	LPARAM lParam
)
{
    VCPFILESPEC vfsSrc, vfsDst;

    if (!VCP_opened)
	return ERR_VCP_NOTOPEN;

    vfsSrc.ldid = ldidSrc;
    vfsSrc.vhstrDir = vsmStringAdd16(lpszSrcDir);
    vfsSrc.vhstrFileName = vsmStringAdd16(lpszSrcFileName);

    vfsDst.ldid = ldidDst;
    vfsDst.vhstrDir = vsmStringAdd16(lpszDstDir);
    vfsDst.vhstrFileName = vsmStringAdd16(lpszDstFileName);

    return VCP_VirtnodeCreate(&vfsSrc, &vfsDst, VNFL_RENAME, lParam,
		    0);
}

/***********************************************************************
 *		VcpEnumFiles (SETUPX.@)
 */
INT16 WINAPI VcpEnumFiles(VCPENUMPROC vep, LPARAM lParamRef)
{
    WORD n;

    for (n = 0; n < vn_last; n++)
	vep(pvnlist[n], lParamRef);

    return 0; /* FIXME: return value ? */
}

/***********************************************************************
 *		VcpExplain (SETUPX.411)
 */
LPCSTR WINAPI VcpExplain16(LPVIRTNODE lpVn, DWORD dwWhat)
{
    static char buffer[MAX_PATH]; /* FIXME: is this how it's done ? */
    buffer[0] = '\0';
    switch (dwWhat)
    {
	case VCPEX_SRC_FULL:
	case VCPEX_DST_FULL:
	    {
		LPVCPFILESPEC lpvfs =
		    (dwWhat == VCPEX_SRC_FULL) ?  &lpVn->vfsSrc : &lpVn->vfsDst;

                /* if we have an ldid, use it, otherwise use the string */
                /* from the vhstrlist array */
		if (lpvfs->ldid != 0xffff)
                  CtlGetLddPath16(lpvfs->ldid, buffer);
                else
                  strcat(buffer, vsmGetStringRawName16(lpvfs->vhstrDir));

                strcat(buffer, "\\");
                strcat(buffer, vsmGetStringRawName16(lpvfs->vhstrFileName));
	    }
	    break;
	default:
	    FIXME("%ld unimplemented !\n", dwWhat);
	    strcpy(buffer, "Unknown error");
	    break;
    }
    return buffer;
}

RETERR16 VCP_CheckPaths(void)
{
    DWORD n;
    LPVIRTNODE lpvn;
    RETERR16 cbres;

    cbres = VCP_Callback(&vcp_status, VCPM_VSTATPATHCHECKSTART, 0, 0, VCP_MsgRef);
    for (n = 0; n < vn_num; n++)
    {
	lpvn = pvnlist[n];
	if (!lpvn) continue;
        /* FIXME: check paths of all VIRTNODEs here ! */
	cbres = VCP_Callback(&lpvn->vfsDst, VCPM_CHECKPATH, 0, (DWORD)lpvn, VCP_MsgRef);
    }
    cbres = VCP_Callback(&vcp_status, VCPM_VSTATPATHCHECKEND, 0, 0, VCP_MsgRef);
    return OK;
}

RETERR16 VCP_CopyFiles(void)
{
    char fn_src[MAX_PATH], fn_dst[MAX_PATH];
    RETERR16 res = OK, cbres;
    DWORD n;
    LPVIRTNODE lpvn;

    cbres = VCP_Callback(&vcp_status, VCPM_VSTATCOPYSTART, 0, 0, VCP_MsgRef);
    for (n = 0; n < vn_num; n++)
    {
	lpvn = pvnlist[n];
	if ((!lpvn) || ((lpvn->fl & VNFL_NODE_TYPE) != VNFL_COPY)) continue;
	/* FIXME: need to send VCPM_VSTATNEWDISK notification sometimes */
        strcpy(fn_src, VcpExplain16(lpvn, VCPEX_SRC_FULL));
        strcpy(fn_dst, VcpExplain16(lpvn, VCPEX_DST_FULL));
	/* FIXME: what is this VCPM_VSTATWRITE here for ?
	 * I guess it's to signal successful destination file creation */
	cbres = VCP_Callback(&vcp_status, VCPM_VSTATWRITE, 0, 0, VCP_MsgRef);

	/* FIXME: need to do the file copy in small chunks for notifications */
	TRACE("copying '%s' to '%s'\n", fn_src, fn_dst);
        /* perform the file copy */
        if (!(CopyFileA(fn_src, fn_dst,
	       (lpvn->fl & VNLP_COPYIFEXISTS) ? FALSE : TRUE )))
        {
            ERR("error copying, src: %s -> dst: %s\n", fn_src, fn_dst);
	    res = ERR_VCP_IOFAIL;
        }

	vcp_status.prgFileRead.dwSoFar++;
	cbres = VCP_Callback(&vcp_status, VCPM_VSTATREAD, 0, 0, VCP_MsgRef);
	vcp_status.prgFileWrite.dwSoFar++;
	cbres = VCP_Callback(&vcp_status, VCPM_VSTATWRITE, 0, 0, VCP_MsgRef);
    }

    cbres = VCP_Callback(&vcp_status, VCPM_VSTATCOPYEND, 0, 0, VCP_MsgRef);
    return res;
}

/***********************************************************************
 *		VcpFlush - internal (not exported), but documented
 *
 * VNFL_NOW is used for VcpFlush.
 */
RETERR16 VcpFlush16(WORD fl, LPCSTR lpszBackupDest)
{
    return OK;
}

/***********************************************************************
 *		VcpClose (SETUPX.201)
 *
 * Does callbacks (-> vifproc) with VCPM_VSTATCLOSESTART,
 * VCPM_VSTATCLOSEEND.
 *
 * fl gets VCPFL_xxx flags to indicate what to do with the
 * VIRTNODEs (files to mess with) created by e.g. GenInstall()
 */
RETERR16 WINAPI VcpClose16(WORD fl, LPCSTR lpszBackupDest)
{
    RETERR16 res = OK;
    WORD cbres = VCPN_PROCEED;

    TRACE("(%04x, '%s')\n", fl, lpszBackupDest);

    /* FIXME: needs to sort virtnodes in case VCPFL_INSPECIFIEDORDER
     * is not set. This is done by VCP_Callback(VCPM_NODECOMPARE) */

    TRACE("#1\n");
    memset(&vcp_status, 0, sizeof(VCPSTATUS));
    /* yes, vcp_status.cbSize is 0 ! */
    TRACE("#2\n");
    cbres = VCP_Callback(&vcp_status, VCPM_VSTATCLOSESTART, 0, 0, VCP_MsgRef);
    TRACE("#3\n");

    res = VCP_CheckPaths();
    TRACE("#4\n");
    if (res != OK)
	return res; /* is this ok ? */
    VCP_CopyFiles();

    TRACE("#5\n");
    cbres = VCP_Callback(&vcp_status, VCPM_VSTATCLOSEEND, 0, 0, VCP_MsgRef);
    TRACE("#6\n");
    VCP_Proc = NULL;
    FreeLibrary(SETUPAPI_hInstance);
    VCP_opened = FALSE;
    return OK;
}

RETERR16 VCP_RenameFiles(void)
{
    char fn_src[MAX_PATH], fn_dst[MAX_PATH];
    RETERR16 res = OK, cbres;
    DWORD n;
    LPVIRTNODE lpvn;

    cbres = VCP_Callback(&vcp_status, VCPM_VSTATRENAMESTART, 0, 0, VCP_MsgRef);
    for (n = 0; n < vn_num; n++)
    {
	lpvn = pvnlist[n];
	if ((!lpvn) || ((lpvn->fl & VNFL_NODE_TYPE) != VNFL_RENAME)) continue;
        strcpy(fn_src, VcpExplain16(lpvn, VCPEX_SRC_FULL));
        strcpy(fn_dst, VcpExplain16(lpvn, VCPEX_DST_FULL));
	cbres = VCP_Callback(&lpvn->vfsDst, VCPM_FILEOPENOUT, 0, (LPARAM)lpvn, VCP_MsgRef);
        if (!(MoveFileExA(fn_src, fn_dst, MOVEFILE_REPLACE_EXISTING)))
	    res = ERR_VCP_IOFAIL;
	else
	    VCP_VirtnodeDelete(lpvn);
    }
    cbres = VCP_Callback(&vcp_status, VCPM_VSTATRENAMEEND, 0, 0, VCP_MsgRef);
    return res;
}

/***********************************************************************
 *		vcpDefCallbackProc (SETUPX.202)
 */
RETERR16 WINAPI vcpDefCallbackProc16(LPVOID lpvObj, UINT16 uMsg, WPARAM wParam,
					LPARAM lParam, LPARAM lParamRef)
{
    static int count = 0;
    if (count < 10)
        FIXME("(%p, %04x, %04x, %08lx, %08lx) - what to do here ?\n",
		lpvObj, uMsg, wParam, lParam, lParamRef);
    count++;
    return OK;
}

/********************* point-and-click stuff from here ***********************/

static HWND hDlgCopy = 0;
static HKEY hKeyFiles = 0, hKeyRename = 0, hKeyConflict = 0;
static char BackupDir[12];

static INT_PTR CALLBACK VCP_UI_FileCopyDlgProc(HWND hWndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR retval = FALSE;

    if (iMsg == WM_INITDIALOG)
    {
        ShowWindow(hWndDlg, SW_SHOWNORMAL);
        UpdateWindow(hWndDlg);
	retval = TRUE;
    }
    return retval;
}

BOOL VCP_UI_GetDialogTemplate(LPCVOID *template32)
{
    HRSRC hResInfo;
    HGLOBAL hDlgTmpl32;

    if (!(hResInfo = FindResourceA(SETUPAPI_hInstance, MAKEINTRESOURCEA(COPYFILEDLGORD), (LPSTR)RT_DIALOG)))
	return FALSE;
    if (!(hDlgTmpl32 = LoadResource(SETUPAPI_hInstance, hResInfo )) ||
        !(*template32 = LockResource( hDlgTmpl32 )))
	return FALSE;
    return TRUE;
}

static LRESULT WINAPI
VCP_UI_FileCopyWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg != WM_CREATE)
        return DefWindowProcA (hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
	case WM_CREATE:
	    return 0;
	default:
	    FIXME("%04x: unhandled.\n", uMsg);
    }

    return 0;
}

void VCP_UI_RegisterProgressClass(void)
{
    static BOOL registered = FALSE;
    WNDCLASSA wndClass;

    if (registered)
	return;

    registered = TRUE;
    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = (WNDPROC)VCP_UI_FileCopyWndProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = 0;
    wndClass.hCursor       = LoadCursorA (0, (LPSTR)IDC_ARROW);
    wndClass.hbrBackground = NULL;
    wndClass.lpszClassName = "setupx_progress";

    RegisterClassA (&wndClass);
}

RETERR16 VCP_UI_NodeCompare(LPVIRTNODE vn1, LPVIRTNODE vn2)
{
    LPCSTR file1, file2;
    file1 = vsmGetStringRawName16(vn1->vfsSrc.vhstrFileName);
    file2 = vsmGetStringRawName16(vn2->vfsSrc.vhstrFileName);
    return (RETERR16)strcmp(file1, file2);
}

RETERR16 VCP_UI_CopyStart(void)
{
    LPCVOID template32;
    char buf[256]; /* plenty */
    BOOL dirty;
    DWORD len;

    /* FIXME: should be registered at DLL startup instead */
    VCP_UI_RegisterProgressClass();
    if (!(VCP_UI_GetDialogTemplate(&template32)))
	return VCPN_FAIL;

    if (vn_num > 10)  /* hack */
    {
        hDlgCopy = CreateDialogIndirectParamA(SETUPAPI_hInstance, template32, 0,
                                              VCP_UI_FileCopyDlgProc, 0);
        if (!hDlgCopy)
            return VCPN_FAIL;
        SetDlgItemTextA(hDlgCopy, SOURCESTRORD, "Scanning ...");
        SetDlgItemTextA(hDlgCopy, DESTSTRORD, "NOT_IMPLEMENTED_YET");
    }
    strcpy(buf, REG_INSTALLEDFILES);
    if (RegCreateKeyA(HKEY_LOCAL_MACHINE, buf, &hKeyFiles))
	return VCPN_FAIL;
    strcat(buf, REGPART_RENAME);
    if (RegCreateKeyA(HKEY_LOCAL_MACHINE, buf, &hKeyRename))
	return VCPN_FAIL;
    if (RegCreateKeyA(HKEY_LOCAL_MACHINE, REG_VERSIONCONFLICT, &hKeyConflict))
	return VCPN_FAIL;
    len = 1;
    if (!(RegQueryValueExA(hKeyConflict, "Dirty", NULL, 0, (LPBYTE)&dirty, &len)))
    {
	/* FIXME: what does SETUPX.DLL do in this case ? */
	MESSAGE("Warning: another program using SETUPX is already running ! Failed.\n");
	return VCPN_FAIL;
    }
    dirty = TRUE;
    if (RegSetValueExA(hKeyConflict, "Dirty", 0, REG_BINARY, (LPBYTE)&dirty, 1))
	return VCPN_FAIL;
    len = 12;
    if (!(RegQueryValueExA(hKeyConflict, "BackupDirectory", NULL, 0, BackupDir, &len)))
	strcpy(BackupDir, "VCM");

    /* create C:\WINDOWS\[BackupDir] and set registry key to it */
    GetWindowsDirectoryA(buf, 256);
    strcat(buf, "\\");
    strcat(buf, BackupDir);
    if (!(CreateDirectoryA(buf, NULL)))
	return VCPN_FAIL;
    if (RegSetValueExA(hKeyConflict, "BackupDirectory", 0, REG_SZ, (LPBYTE)buf, strlen(buf)+1))
	return VCPN_FAIL;
    RegCloseKey(hKeyConflict);

    return VCPN_OK;
}

/***********************************************************************
 *		vcpUICallbackProc (SETUPX.213)
 */
RETERR16 WINAPI vcpUICallbackProc16(LPVOID lpvObj, UINT16 uMsg, WPARAM wParam,
					LPARAM lParam, LPARAM lParamRef)
{
    static int count = 0;
    RETERR16 res = VCPN_OK, cbres;

    if (count < 5)
        FIXME("(%p, %04x, %04x, %08lx, %08lx) - semi-stub\n",
		lpvObj, uMsg, wParam, lParam, lParamRef);
    count++;
    switch (uMsg)
    {
	/* unused messages, it seems */
	case VCPM_DISKPREPINFO:

	case VCPM_FILENEEDED:

	case VCPM_NODECREATE:
	case VCPM_NODEACCEPT:

	case VCPM_VSTATCLOSESTART:
	case VCPM_VSTATPATHCHECKSTART:
	case VCPM_VSTATPATHCHECKEND:

	case VCPM_CHECKPATH:
	    break;

	/* the real stuff */
	case VCPM_NODECOMPARE:
	    res = VCP_UI_NodeCompare((LPVIRTNODE)lpvObj, (LPVIRTNODE)lParam);
	    break;
	case VCPM_VSTATREAD:
	    break;
	case VCPM_VSTATWRITE:
	    cbres = VCP_Callback(&vcp_status, VCPM_DISKPREPINFO, 0, 0, VCP_MsgRef);
	    break;
	case VCPM_VSTATCLOSEEND:
	    RegCloseKey(hKeyFiles);
	    RegCloseKey(hKeyRename);
	    RegDeleteKeyA(HKEY_LOCAL_MACHINE, REG_VERSIONCONFLICT);
	    break;
	case VCPM_VSTATCOPYSTART:
	    res = VCP_UI_CopyStart();
	    break;
	case VCPM_VSTATCOPYEND:
	    if (hDlgCopy) DestroyWindow(hDlgCopy);
	    break;
	default:
	    FIXME("unhandled msg 0x%04x\n", uMsg);
    }
    return res;
}
