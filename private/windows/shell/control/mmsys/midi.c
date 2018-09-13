/*==========================================================================*/
//
//  midi.c
//
//  Copyright (C) 1993-1994 Microsoft Corporation.  All Rights Reserved.
/*==========================================================================*/

#include "mmcpl.h"
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddkp.h>
#include <mmreg.h>
#include <cpl.h> 
#define NOSTATUSBAR
#include <commctrl.h>
#include <prsht.h>
#include <string.h>
#include <memory.h>
#include <regstr.h>
#include "draw.h"
#include "utils.h"
#include "roland.h"

#include "midi.h"
#include "tchar.h"

//#include "newexe.h"
#include <winnt.h>

#if defined DEBUG || defined DEBUG_RETAIL
 extern TCHAR szNestLevel[];
 TCHAR szNestLevel[] = TEXT ("0MidiProp:");
 #define MODULE_DEBUG_PREFIX szNestLevel
#endif

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define _INC_MMDEBUG_CODE_ TRUE
#include "mmdebug.h"

#include "medhelp.h"

#ifndef TVIS_ALL
#define TVIS_ALL 0xFF7F // internal
#endif

#ifndef MIDI_IO_CONTROL
#define MIDI_IO_CONTROL 0x00000008L     // internal
#endif

#ifndef DRV_F_ADD       // BUGBUG: These should be in MMDDK.H
#define DRV_F_ADD                0x00000000
#define DRV_F_REMOVE             0x00000001
#define DRV_F_CHANGE             0x00000002
#define DRV_F_PROP_INSTR         0x00000004
#define DRV_F_NEWDEFAULTS        0x00000008
#define DRV_F_PARAM_IS_DEVNODE   0x10000000
#endif

/*==========================================================================*/

// containing struct for what would otherwise be global variables
//
struct _globalstate gs;

// this is the registry key that has midi instrument aliases
// as subkeys
//
SZCODE cszSchemeRoot[] =  REGSTR_PATH_PRIVATEPROPERTIES TEXT ("\\MIDI\\Schemes");

SZCODE cszMidiMapRoot[] = REGSTR_PATH_MULTIMEDIA TEXT ("\\MIDIMap");

// this is the registry key that has midi driver/port names
//
SZCODE cszDriversRoot[] = REGSTR_PATH_MEDIARESOURCES TEXT ("\\MIDI");

// this is the list of known hindered midi drivers (or rather,
// known drivers that require special idf's)
//
SZCODE cszHinderedMidiList[] = REGSTR_PATH_MEDIARESOURCES TEXT ("\\NonGeneralMIDIDriverList");

// Key to trigger runonce of midi setup wizard.
//
SZCODE cszRunOnceSetup[] =
    REGSTR_PATH_RUNONCE TEXT ("\\Setup");

SZCODE  cszFriendlyName[]     = TEXT ("FriendlyName");
SZCODE  cszDescription[]      = TEXT ("Description");
SZCODE  cszSlashInstruments[] = TEXT ("\\Instruments");
SZCODE  cszExternal[]         = TEXT ("External");
SZCODE  cszActive[]           = TEXT ("Active");
SZCODE  cszDefinition[]       = TEXT ("Definition");
SZCODE  cszPort[]             = TEXT ("Port");
SZCODE  cszMidiSlash[]        = TEXT ("midi\\");
SZCODE  csz02d[]              = TEXT ("%02d");
SZCODE  cszEmpty[]            = TEXT ("");

static SZCODE cszChannels[]            = TEXT ("Channels");
static SZCODE cszCurrentScheme[]       = TEXT ("CurrentScheme");
static SZCODE cszCurrentInstrument[]   = TEXT ("CurrentInstrument");
static SZCODE cszUseScheme[]           = TEXT ("UseScheme");
static SZCODE cszAutoScheme[]          = TEXT ("AutoScheme");
static SZCODE cszRunOnceCount[]        = TEXT ("ConfigureCount");
static SZCODE cszDriverList[]          = TEXT ("DriverList");
static SZCODE cszDriverVal[]           = TEXT ("Driver");

//
// structures used to hold data for the control panel dialogs.
//
//
typedef struct _midi_scheme {
    PMCMIDI  pmcm;
    HKEY     hkSchemes;
    TCHAR    szNone[MAX_ALIAS];
    DWORD    dwChanMask;
    TCHAR    szName[MAX_ALIAS];
    UINT     nChildren;
    BOOL     bDirty;
    struct {
	PINSTRUM  pi;
	DWORD     dwMask;
	} a[NUM_CHANNEL*4 +1];
    } MSCHEME, * PMSCHEME;

typedef struct _midi_cpl {
    LPPROPSHEETPAGE ppsp;

    MSCHEME         ms;
    TCHAR           szScheme[MAX_ALIAS];
    TCHAR           szDefault[MAX_ALIAS];
    PINSTRUM        piSingle;
    BOOL            bUseScheme;
    BOOL            bAutoScheme;  // TRUE if scheme was auto created
    DWORD           dwRunCount;   // counts the number of times runonce
    LPTSTR           pszReason;    // reason for choosing external port
    BOOL            bDlgType2;
    BOOL            bPastInit;
    BOOL            bIgnoreSelChange;

    MCMIDI          mcm;

    } MCLOCAL, * PMCLOCAL;

BOOL WINAPI ShowDetails (
    HWND     hWnd,
    PMCLOCAL pmcl);

LONG SHRegDeleteKey(HKEY hKey, LPCTSTR lpSubKey);

static UINT
DeviceIDFromDriverName(
    PTSTR pstrDriverName);

extern BOOL AccessServiceController(void);

/*+ SimulateNotify
 *
 *-=================================================================*/

STATICFN LRESULT SimulateNotify (
    HWND hWnd,
    WORD uId,
    WORD wNotify)
{
    #ifdef _WIN32
     return SendMessage (hWnd, WM_COMMAND,
			 MAKELONG(uId, wNotify),
			 (LPARAM)GetDlgItem (hWnd, uId));
    #else
    #error this code is not designed for 16 bits
    #endif
}


/*+ Confirm
 *
 *-=================================================================*/

STATICFN UINT Confirm (
    HWND    hWnd,
    UINT    idQuery,
    LPTSTR  pszArg)
{
    TCHAR szQuery[255];
    TCHAR sz[255];

    LoadString (ghInstance, idQuery, sz, NUMELMS(sz));
    wsprintf (szQuery, sz, pszArg);

    LoadString (ghInstance, IDS_DEF_CAPTION, sz, NUMELMS(sz));

    return MessageBox (hWnd, szQuery, sz, MB_YESNO | MB_ICONQUESTION);
}


/*+ TellUser
 *
 *-=================================================================*/

STATICFN UINT TellUser (
    HWND    hWnd,
    UINT    idQuery,
    LPTSTR  pszArg)
{
    TCHAR szQuery[255];
    TCHAR sz[255];

    LoadString (ghInstance, idQuery, sz, NUMELMS(sz));
    wsprintf (szQuery, sz, pszArg);

    LoadString (ghInstance, IDS_DEF_CAPTION, sz, NUMELMS(sz));

    return MessageBox (hWnd, szQuery, sz, MB_OK | MB_ICONINFORMATION);
}



/*+ ForwardBillNotify
 *
 *-=================================================================*/

STATICFN void ForwardBillNotify (
    HWND  hWnd,
    NMHDR FAR * lpnm)
{
    static struct {
	UINT code;
	UINT uId;
	} amap[] = {PSN_KILLACTIVE, IDOK,
		    PSN_APPLY,      ID_APPLY,
		    PSN_SETACTIVE,  ID_INIT,
		    PSN_RESET,      IDCANCEL,
		    };
    UINT ii;

#ifdef DEBUG
    AuxDebugEx (4, DEBUGLINE TEXT ("ForwardBillNotify() code = %X\r\n"), lpnm->code);
#endif

    for (ii = 0; ii < NUMELMS(amap); ++ii)
	if (lpnm->code == amap[ii].code)
	{
	    FORWARD_WM_COMMAND (hWnd, amap[ii].uId, 0, 0, SendMessage);
	    break;
	}
    return;
}


/*+
 *
 *-=================================================================*/

STATICFN void EnumChildrenIntoCombo (
    HWND   hWndT,
    LPTSTR pszSelect,
    HKEY   hKey)
{
    TCHAR  sz[MAX_ALIAS];
    DWORD  cch = sizeof(sz)/sizeof(TCHAR);
    UINT   ii = 0;

    //SetWindowRedraw (hWndT, FALSE);
    ComboBox_ResetContent (hWndT);

    if (!hKey)
	return;

    while (RegEnumKey (hKey, ii, sz, cch) == ERROR_SUCCESS)
    {
	int ix = ComboBox_AddString (hWndT, sz);
	//ComboBox_SetItemData (hWndT, ix, ii);
	++ii;
    }

    ii = 0;
    if (pszSelect)
       ii = ComboBox_FindString (hWndT, -1, pszSelect);

    ComboBox_SetCurSel (hWndT, ii);
}

STDAPI_(BOOL) QueryGSSynth(LPTSTR pszDriver)
{
    MIDIOUTCAPS moc;
    MMRESULT    mmr;
    UINT        mid;
    BOOL        fGSSynth = FALSE;

    
    if (pszDriver)
    {
        mid = DeviceIDFromDriverName(pszDriver);

        if (mid!=(UINT)-1)
        {
            mmr = midiOutGetDevCaps(mid, &moc, sizeof(moc));

            if (MMSYSERR_NOERROR == mmr)
            {
                if ((moc.wMid == MM_MICROSOFT) && (moc.wPid == MM_MSFT_WDMAUDIO_MIDIOUT) && (moc.wTechnology == MOD_SWSYNTH))
                {
                    fGSSynth = TRUE;
                } //end if synth
            } //end if no mm error
        } //end if mid is valid
    } //end if driver is valid string

    return(fGSSynth);
}

/*+
 *
 *-=================================================================*/

LONG WINAPI GetAlias (
    HKEY   hKey,
    LPTSTR szSub,
    LPTSTR pszAlias,
    DWORD  cchAlias,
    BOOL * pbExtern,
    BOOL * pbActive)
{
    LONG  lRet;
    DWORD cbSize;
    HKEY  hkSub;
    DWORD dw;

#ifdef DEBUG
    AuxDebugEx (8, DEBUGLINE TEXT ("GetAlias(%08x,'%s',%08x,%d,%08x)\r\n"),
		hKey, szSub, pszAlias, cchAlias, pbExtern);
#endif

    if (!(lRet = RegOpenKeyEx (hKey, szSub, 0, KEY_QUERY_VALUE, &hkSub)))
    {
	cbSize = cchAlias * sizeof (TCHAR);
	if ((lRet = RegQueryValueEx (hkSub, cszFriendlyName, NULL, &dw, (LPBYTE)pszAlias, &cbSize)) || cbSize <= 2)
	{
	    cbSize = cchAlias * sizeof (TCHAR);
	    if ((lRet = RegQueryValueEx (hkSub, cszDescription, NULL, &dw, (LPBYTE)pszAlias, &cbSize)) || cbSize <= 2)
	    {
		TCHAR szDriver[MAXSTR];

		cbSize = sizeof(szDriver);
		if (!RegQueryValueEx(hkSub, cszDriverVal, NULL, &dw, (LPBYTE)szDriver, &cbSize))
		{
		    LoadVERSION();
		    if (!LoadDesc(szDriver, pszAlias))
			lstrcpy(pszAlias, szDriver);
		    FreeVERSION();

		    cbSize = (lstrlen(pszAlias)+1) * sizeof(TCHAR);
		    RegSetValueEx(hkSub, cszFriendlyName, (DWORD)0, REG_SZ, (LPBYTE)pszAlias, cbSize);
		    RegSetValueEx(hkSub, cszDescription, (DWORD)0, REG_SZ, (LPBYTE)pszAlias, cbSize);
		}
		else
		    pszAlias[0] = 0;
	    }
	}

	if (pbExtern)
	{
	    *pbExtern = 0;
	    cbSize = sizeof(*pbExtern);
	    if (!(lRet = RegQueryValueEx (hkSub, cszExternal, NULL, &dw, (LPBYTE)pbExtern, &cbSize)))
	    {
		if (REG_SZ == dw)
		    *pbExtern = (*(LPTSTR)pbExtern == TEXT('0')) ? FALSE : TRUE;
	    }
	}

	if (pbActive)
	{
	    *pbActive = 0;
	    cbSize = sizeof(*pbActive);
	    if (!(lRet = RegQueryValueEx (hkSub, cszActive, NULL, &dw, (LPBYTE)pbActive, &cbSize)))
	    {
		if (REG_SZ == dw)
		    *pbActive = (*(LPTSTR)pbActive == TEXT('1')) ? TRUE : FALSE;
	    }
	}

	RegCloseKey (hkSub);
    }

   #ifdef DEBUG
    if (lRet)
    {
	TCHAR szErr[MAX_PATH];

	FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS, NULL, lRet, 0,
		      szErr, NUMELMS(szErr), NULL);

#ifdef DEBUG
	AuxDebugEx (1, DEBUGLINE TEXT ("GetAlias failed: %d %s\r\n"), lRet, szErr);
#endif
    }
   #endif

    return lRet;
}


/*+
 *
 *-=================================================================*/

LONG WINAPI GetDriverFilename (
    HKEY    hKey,
    LPTSTR  szSub,
    LPTSTR  pszDriver,
    DWORD   cchDriver)
{
    HKEY  hkSub;
    LONG  lRet;

    if (!(lRet = RegOpenKeyEx (hKey, szSub, 0, KEY_QUERY_VALUE, &hkSub)))
    {
       DWORD dwType;
       TCHAR sz[MAX_PATH];
       UINT  cb = sizeof(sz);

       // get the contents of the 'driver' value of the given key.
       // then copy the filename part
       //
       lRet = RegQueryValueEx(hkSub, cszDriverVal, NULL, &dwType, (LPBYTE)sz, &cb);
       if (lRet || dwType != REG_SZ)
	   *pszDriver = 0;
       else
       {
	   LPTSTR psz = sz;
	   UINT   ii;

	   // scan forward till we get to the file part of the pathname
	   // then copy that part into the supplied buffer
	   //
	   for (ii = 0; psz[ii]; )
	   {
		if (psz[ii] == TEXT('\\') || psz[ii] == TEXT(':'))
		{
		    psz += ii+1;
		    ii = 0;
		}
		else
		    ++ii;
	   }
	   lstrcpyn (pszDriver, psz, cchDriver);
       }
       RegCloseKey (hkSub);
    }
    return lRet;
}


/*+ LoadInstruments
 *
 * load interesting data for all instruments, if bDriverAsAlias
 * is true, then put driver filename in szFriendly field of each
 * instrument. (scheme init uses this for hindered driver detection)
 * if !bDriverAsAlias, put friendly name in friendly name slot
 *
 *
 *-=================================================================*/

void WINAPI LoadInstruments (
    PMCMIDI pmcm,
    BOOL    bDriverAsAlias)
{
    HKEY     hkMidi;
    TCHAR    sz[MAX_ALIAS];
    DWORD    cch = sizeof(sz)/sizeof(TCHAR);
    UINT     ii;
    UINT     nInstr;
    PINSTRUM pi;
    UINT     idxPort = 0;

    pmcm->nInstr = 0;
    pmcm->bHasExternal = FALSE;

    if (!(hkMidi = pmcm->hkMidi))
    {
	if (RegCreateKey (HKEY_LOCAL_MACHINE, cszDriversRoot, &hkMidi))
	    return;
	pmcm->hkMidi = hkMidi;
    }

    if (!(pi = pmcm->api[0]))
    {
	pmcm->api[0] = pi = (LPVOID)LocalAlloc (LPTR, sizeof(*pi));
	if (!pi)
	    return;
    }

    for (cch = sizeof(pi->szKey)/sizeof(TCHAR), nInstr = 0, ii = 0;
	 ! RegEnumKey (hkMidi, ii, pi->szKey, cch);
	 ++ii)
    {
	UINT        jj;
	HKEY        hkInst;
	PINSTRUM    piParent;
	BOOL        bActive;

	// get driver alias, external, and active flags.  This has the side
	// effect of initializing the friendly name key for legacy drivers
	// that have neither friendly name, nor description
	//
	GetAlias (hkMidi, pi->szKey, pi->szFriendly, 
		  NUMELMS(pi->szFriendly), &pi->bExternal, &bActive);

	// if requested, stomp friendly name with driver filename
	//
	if (bDriverAsAlias)
	    GetDriverFilename (hkMidi, pi->szKey, 
			       pi->szFriendly, NUMELMS(pi->szFriendly));

    pi->fGSSynth = QueryGSSynth(pi->szKey);
    pi->uID = idxPort;

	if (pi->bExternal)
	    pmcm->bHasExternal = TRUE;

	pi->piParent = 0;
	pi->bActive = bActive;
	piParent = pi;

	++nInstr;
	if (nInstr >= NUMELMS(pmcm->api))
	{
	    assert2 (0, TEXT ("Tell JohnKn to make midi instrument table bigger"));
	    break;
	}

	if (!(pi = pmcm->api[nInstr]))
	{
	    pmcm->api[nInstr] = pi = (LPVOID)LocalAlloc (LPTR, sizeof(*pi));
	    if (!pi)
		break;
	}

	// open the parent's instruments subkey
	//
	lstrcpy (sz, piParent->szKey);
	lstrcat (sz, cszSlashInstruments);
	if (RegCreateKey (hkMidi, sz, &hkInst))
	    continue;

	// enum the instruments and add them to the list
	//
	for (jj = 0; ! RegEnumKey (hkInst, jj, sz, cch); ++jj)
	{
	    lstrcpy (pi->szKey, piParent->szKey);
	    lstrcat (pi->szKey, cszSlashInstruments);
	    lstrcat (pi->szKey, cszSlash);
	    lstrcat (pi->szKey, sz);

	    GetAlias (hkInst, sz, pi->szFriendly, 
		      NUMELMS(pi->szFriendly), NULL, NULL);
	    pi->piParent = piParent;
	    pi->bExternal = FALSE;
	    pi->bActive = bActive;

	    ++nInstr;
	    if (nInstr >= NUMELMS(pmcm->api))
	    {
		assert2 (0, TEXT ("Tell JohnKn to make midi instrument table bigger"));
		break;
	    }

	    if (!(pi = pmcm->api[nInstr]))
	    {
		pmcm->api[nInstr] = pi = (LPVOID)LocalAlloc (LPTR, sizeof(*pi));
		if (!pi)
		    break;
	    }
	}

	RegCloseKey (hkInst);
    }

    // create a 'none' entry at the end
    //
    if (pi)
    {
	pi->piParent = 0;
	pi->bExternal = FALSE;
	pi->bActive = TRUE;
	pi->szKey[0] = 0;
	LoadString (ghInstance, IDS_NONE, pi->szFriendly, NUMELMS(pi->szFriendly));
	++nInstr;
    }

    pmcm->nInstr = nInstr;
}


/*+
 *
 *-=================================================================*/

void WINAPI FreeInstruments (
    PMCMIDI pmcm)
{
    UINT ii;

    for (ii = 0; ii < NUMELMS (pmcm->api); ++ii)
	if (pmcm->api[ii])
	    LocalFree ((HLOCAL)(PVOID)pmcm->api[ii]), pmcm->api[ii] = NULL;

    pmcm->nInstr = 0;
}



#ifdef DEBUG
/*+ CleanStringCopy
 *
 * Replaces unprintable characters with '.'
 *
 *-=================================================================*/

STATICFN LPTSTR CleanStringCopy (
    LPTSTR pszOut,
    LPTSTR pszIn,
    UINT   cbOut)
{
    LPTSTR psz = pszOut;
    while (cbOut && *pszIn)
    {
	*psz = (*pszIn >= 32 && *pszIn < 127) ? *pszIn : TEXT('.');
	++psz;
	++pszIn;
    }

    *psz = 0;
    return pszOut;
}


/*+ DumpInstruments
 *
 *-=================================================================*/

STATICFN void DumpInstruments (
    PMCMIDI pmcm)
{
    UINT     ii;
    PINSTRUM pi;

#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("DumpInstruments(%08x) nInstr=%d\r\n"),
		pmcm, pmcm->nInstr);
#endif

    for (ii = 0; ii < pmcm->nInstr; ++ii)
    {
	TCHAR szKey[MAX_ALIAS];
	TCHAR szFriendly[MAX_ALIAS];

	pi = pmcm->api[ii];
	if (!pi)
	{
#ifdef DEBUG
	    AuxDebugEx (2, TEXT ("\tapi[%d] NULL\r\n"), ii);
#endif
	    continue;
	}

	CleanStringCopy (szKey, pi->szKey, NUMELMS(szKey));
	CleanStringCopy (szFriendly, pi->szFriendly, NUMELMS(szFriendly));

#ifdef DEBUG
	AuxDebugEx (3, TEXT ("\tapi[%d]%08X p:%08x x:%d a:%d '%s' '%s'\r\n"),
		    ii, pi, pi->piParent,
		    pi->bExternal, pi->bActive,
		    szKey, szFriendly);
#endif
    }
}
#endif


/*+
 *
 *-=================================================================*/

STATICFN PINSTRUM WINAPI FindInstrumPath (
    PMCMIDI pmcm,
    LPTSTR   pszPath)
{
    UINT  ii;

    for (ii = 0; ii < pmcm->nInstr; ++ii)
    {
	assert (pmcm->api[ii]);
	if (IsSzEqual(pszPath, pmcm->api[ii]->szKey))
	    return pmcm->api[ii];
    }

    return NULL;
}


/*+
 *
 *-=================================================================*/

PINSTRUM WINAPI FindInstrumentFromKey (
    PMCMIDI  pmcm,
    LPTSTR   pszKey)
{
    UINT  ii;

    if (!pszKey || !pszKey[0])
	return NULL;

    for (ii = 0; ii < pmcm->nInstr; ++ii)
    {
	assert (pmcm->api[ii]);
	if (IsSzEqual(pszKey, pmcm->api[ii]->szKey))
	    return pmcm->api[ii];
    }

    return NULL;
}



/*+
 *
 *-=================================================================*/

STATICFN void LoadInstrumentsIntoCombo (
    HWND     hWnd,
    UINT     uId,
    PINSTRUM piSelect,
    PMCMIDI  pmcm)
{
    HWND   hWndT = GetDlgItem (hWnd, uId);
    UINT   ii;
    int    ix;

#ifdef DEBUG
    AuxDebugEx (4, DEBUGLINE TEXT ("LoadInstrumentsIntoCombo(%08X,%d,%08x,%08x)\r\n"),
		hWnd, uId, piSelect, pmcm);
#endif

    assert (hWndT);
    if (!hWndT)
	return;

    if (pmcm->nInstr > 0)
	SetWindowRedraw (hWndT, FALSE);
    ComboBox_ResetContent(hWndT);

    for (ii = 0; ii < pmcm->nInstr; ++ii)
    {
	if (ii == pmcm->nInstr-1)
	   SetWindowRedraw (hWndT, TRUE);

	if (pmcm->api[ii]->bActive
       #ifdef EXCLUDE_EXTERNAL
	    && !pmcm->api[ii]->bExternal
       #endif
	   )
	{
#ifdef DEBUG
	    AuxDebugEx (7, DEBUGLINE TEXT ("Instrument[%d] = '%s'\r\n"),
			ii, pmcm->api[ii]->szFriendly);
#endif

	    ix = ComboBox_AddString (hWndT, pmcm->api[ii]->szFriendly);
	    ComboBox_SetItemData (hWndT, ix, (LPARAM)pmcm->api[ii]);

	    if (piSelect && pmcm->api[ii] == piSelect)
		ComboBox_SetCurSel (hWndT, ix);
	}
    }
}


/*+
 *
 *-=================================================================*/

STATICFN void LoadInstrumentsIntoTree (
    HWND     hWnd,
    UINT     uId,
    UINT     uIdSingle,
    PINSTRUM piSelect,
    PMCLOCAL pmcl)
{
    PMCMIDI   pmcm = &pmcl->mcm;
    HWND      hWndT = GetDlgItem (hWnd, uId);
    UINT      ii;
    HTREEITEM htiSelect = NULL;
    HTREEITEM htiParent = TVI_ROOT;

    assert (hWndT);
    if (!hWndT)
	return;

    #ifdef UNICODE
    TreeView_SetUnicodeFormat(hWndT,TRUE);
    #endif

    //if (pmcm->nInstr > 0)
    //    SetWindowRedraw (hWndT, FALSE);
    pmcl->bIgnoreSelChange = TRUE;
#ifdef DEBUG
    AuxDebugEx (6, DEBUGLINE TEXT ("tv_DeleteAllItems(%08X)\r\n"), hWndT);
#endif
    TreeView_DeleteAllItems(hWndT);
#ifdef DEBUG
    AuxDebugEx (6, DEBUGLINE TEXT ("tv_DeleteAllItems(%08X) ends\r\n"), hWndT);
#endif

    pmcl->bIgnoreSelChange = FALSE;

    for (ii = 0; ii < pmcm->nInstr; ++ii)
    {
	PINSTRUM        pi = pmcm->api[ii];
	TV_INSERTSTRUCT ti;
	HTREEITEM       hti;

	//if (ii == pmcm->nInstr-1)
	//   SetWindowRedraw (hWndT, TRUE);

	if (!pi->szKey[0] || !pi->bActive)
	    continue;

	ZeroMemory (&ti, sizeof(ti));

	ti.hParent = TVI_ROOT;
	if (pi->piParent)
	    ti.hParent = htiParent;

	ti.hInsertAfter   = TVI_SORT;
	ti.item.mask      = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;

	// BUGBUG:  TV_ITEM may not be ported to UNICODE ?!?
	ti.item.pszText   = pi->szFriendly;
	ti.item.state     = TVIS_EXPANDED;
	ti.item.stateMask = TVIS_ALL;
	ti.item.lParam    = (LPARAM)pi;

	hti = TreeView_InsertItem (hWndT, &ti);

	if (piSelect && (piSelect == pi))
	   htiSelect = hti;

	if ( ! pi->piParent)
	    htiParent = hti;
    }

    // if a 'single' control id has been specified, propagate
    // selected item text into this control
    //
    if (uIdSingle)
    {
	if (htiSelect)
	{
	    assert (piSelect);
	    TreeView_SelectItem (hWndT, htiSelect);
	    SetDlgItemText (hWnd, uIdSingle, piSelect->szFriendly);
        EnableWindow(GetDlgItem(hWnd, IDC_ABOUTSYNTH), piSelect->fGSSynth);
	}
	else
	    SetDlgItemText (hWnd, uIdSingle, cszEmpty);
    }

}


/*+
 *
 *-=================================================================*/

STATICFN void LoadSchemesIntoCombo (
    HWND     hWnd,
    UINT     uId,
    LPTSTR   pszSelect,
    PMSCHEME pms)
{
    HWND  hWndT = GetDlgItem (hWnd, uId);
    HKEY  hKey;

    assert (hWndT);
    if (!hWndT)
	return;

    hKey = pms->hkSchemes;
    if (!hKey &&
	!RegCreateKey (HKEY_LOCAL_MACHINE, cszSchemeRoot, &hKey))
	pms->hkSchemes = hKey;

    EnumChildrenIntoCombo (hWndT, pszSelect, hKey);
}


/*+ ChildKeyExists
 *
 * given an open registry key, and the name of a child of that
 * registry key,  returns true if a child key with the given
 * name exists.
 *
 *-=================================================================*/

STATICFN BOOL ChildKeyExists (
    HKEY     hKey,
    LPTSTR   pszChild)
{
    TCHAR  sz[MAX_ALIAS];
    UINT   ii;

    if (!hKey)
	return FALSE;

    for (ii = 0; ! RegEnumKey (hKey, ii, sz, sizeof(sz)/sizeof(TCHAR)); ++ii)
    {
	if (IsSzEqual (pszChild, sz))
	    return TRUE;
    }

    return FALSE;
}


/*+ LoadSchemeFromReg
 *
 *-=================================================================*/

STATICFN void LoadSchemeFromReg (
    PMCMIDI   pmcm,
    PMSCHEME  pms,
    LPTSTR    pszName)
{
    HKEY  hKey;
    DWORD dwAccum;
    UINT  count;

    // try to open the indicated scheme key in the registry
    // and read channel map from it.  Failure here is permissible.
    // it indicates that we are createing a new scheme.
    //
    count = 0;
    if (RegOpenKey (pms->hkSchemes, pszName, &hKey) == ERROR_SUCCESS)
    {
	DWORD cb;
	TCHAR sz[MAX_ALIAS];

	while (RegEnumKey (hKey, count, sz, sizeof(sz)/sizeof(TCHAR)) == ERROR_SUCCESS)
	{
	    HKEY  hKeyA;
	    DWORD dwType;

	    if (RegOpenKey (hKey, sz, &hKeyA) != ERROR_SUCCESS)
		    break;

	    pms->a[count].pi = NULL;
	    cb = sizeof(sz);
	    if ( ! RegQueryValue (hKeyA, NULL, sz, &cb))
		    pms->a[count].pi = FindInstrumPath (pmcm, sz);

	    pms->a[count].dwMask = 0;
	    cb = sizeof(pms->a[count].dwMask);
	    RegQueryValueEx (hKeyA, cszChannels, NULL,
			     &dwType, (LPBYTE)&pms->a[count].dwMask, &cb);

	    assert (dwType == REG_DWORD);

	    RegCloseKey (hKeyA);

        // Don't allow empty entries
	    //assert (pms->a[ii].dwMask);
        if (0 == pms->a[count].dwMask)
        {
    	    pms->a[count].pi = NULL;
        }
        
        ++count;

#ifdef DEBUG
	    AuxDebugEx (4, DEBUGLINE TEXT ("[%d]Chan %08X Alias '%s'\r\n"),
			count, pms->a[count].dwMask, pms->a[count].pi
					       ? pms->a[count].pi->szFriendly
					       : TEXT ("(null)"));
#endif
	    if (count == NUMELMS(pms->a) -1)
		    break;
	}

	RegCloseKey (hKey);
    }
    pms->nChildren = count;
    lstrcpyn (pms->szName, pszName, NUMELMS(pms->szName));

    // slam a dummy (none) alias that matches all channels
    // at the end of our channel/alias list
    //
    assert (count < NUMELMS(pms->a));
    pms->a[count].dwMask = (DWORD)~0;
    pms->a[count].pi = NULL;

#ifdef DEBUG
    AuxDebugEx (4, DEBUGLINE TEXT ("[%d]Chan %08X Alias '%s'\r\n"),
		count, pms->a[count].dwMask, "null");
#endif

    // make sure scheme channel masks are in a valid state
    //
    for (dwAccum = 0, count = 0; count < NUMELMS(pms->a); ++count)
    {
	pms->a[count].dwMask &= ~dwAccum;
	dwAccum |= pms->a[count].dwMask;
    }

    return;
}


/*+ KickMapper
 *
 *-=================================================================*/


void WINAPI KickMapper (
    HWND hWnd)
{
    HMIDIOUT  hmo;

    if (! midiOutOpen(&hmo, MIDI_MAPPER, 0, 0, MIDI_IO_CONTROL))
    {
	BOOL bDone;

#ifdef DEBUG
	AuxDebugEx (2, DEBUGLINE TEXT ("Kicking Midi Mapper\r\n"));
#endif

	bDone = midiOutMessage(hmo, DRVM_MAPPER_RECONFIGURE, 0, DRV_F_PROP_INSTR);
	
	midiOutClose(hmo);
    /*
    //no longer necessary due to winmm change allowing configure during play
	if (!bDone && hWnd)
	    TellUser (hWnd, IDS_MAPPER_BUSY, NULL);
    */
    }

#ifdef DEBUG
    AuxDebugEx (2, DEBUGLINE TEXT ("Done Kicking Midi Mapper\r\n"));
#endif
}


/*+ SaveSchemeToReg
 *
 *-=================================================================*/

STATICFN void SaveSchemeToReg (
    PMCMIDI   pmcm,
    PMSCHEME  pms,
    LPTSTR    pszName,
    HWND      hWnd)
{
    TCHAR sz[MAX_ALIAS];
    HKEY  hKey;
    DWORD dwAccum;
    UINT  ii;
    UINT  kk;
    UINT  cb;

    #ifdef DEBUG
     AuxDebugEx (4, DEBUGLINE TEXT ("Saving Scheme '%s' children=%d\r\n"),
		 pszName, pms->nChildren);
     for (ii = 0; ii < NUMELMS(pms->a); ++ii)
     {
	 AuxDebugEx (4, TEXT ("\t%08X '%s'\r\n"),
		     pms->a[ii].dwMask,
		     pms->a[ii].pi ? pms->a[ii].pi->szKey : TEXT ("(null)"));
     }
    #endif

    // make sure scheme channel masks are in a valid state,
    // that is, prevent a channel bit from being set in more
    // than one member of a scheme
    //
    for (dwAccum = 0, ii = 0; ii < NUMELMS(pms->a); ++ii)
    {
	pms->a[ii].dwMask &= ~dwAccum;
	dwAccum |= pms->a[ii].dwMask;
    }

    // try to open/create the indicated scheme key in the registry
    // and write/update channel map to it.
    //
    if (!RegCreateKey (pms->hkSchemes, pszName, &hKey))
    {
	HKEY  hKeyA;
	BOOL  bKill;

	// salvage all of the existing keys that we can. delete
	// the rest.
	//
	for (dwAccum = 0, ii = 0; !RegEnumKey (hKey, ii, sz, sizeof(sz)/sizeof(TCHAR)); ++ii)
	{
	    if (ii >= NUMELMS(pms->a))
		break;

	    // we reuse the first N keys in this scheme
	    // and delete the rest.
	    //
	    bKill = TRUE;
	    if (((dwAccum & 0xFFFF) != 0xFFFF) &&
		pms->a[ii].pi &&
		(!ii || (pms->a[ii].pi->szKey[0] && pms->a[ii].dwMask)))
	       bKill = FALSE;

	    dwAccum |= pms->a[ii].dwMask;

	    // if we have an obsolete alias key, remove it now
	    // otherwise create/open the alias key and set it's
	    // channel property to the correct value.
	    //
	    if (bKill)
	    {
#ifdef DEBUG
		AuxDebugEx (3, DEBUGLINE TEXT ("Deleting key[%d] '%s'\r\n"), ii, sz);
#endif
		RegDeleteKey (hKey, sz);
	    }
	    else
	    {
#ifdef DEBUG
		AuxDebugEx (3, DEBUGLINE TEXT ("Reusing key[%d] '%s'\r\n"), ii, pms->a[ii].pi->szKey);
#endif
		if (RegOpenKeyEx (hKey, sz, 0, KEY_ALL_ACCESS, &hKeyA))
		    break;


		cb = (lstrlen(pms->a[ii].pi->szKey) + 1) * sizeof(TCHAR);
		RegSetValueEx (hKeyA, NULL, 0, REG_SZ,
			       (LPBYTE)(pms->a[ii].pi->szKey), cb);

		RegSetValueEx (hKeyA, cszChannels, 0,
			       REG_DWORD,
			       (LPBYTE)&pms->a[ii].dwMask,
			       sizeof(DWORD));

		RegCloseKey (hKeyA);
	    }

	}

	// if we have channels that have not yet been written.
	// do that now
	//
	for (kk = 0; ii < NUMELMS(pms->a); ++ii)
	{
	    // if this alias has any assigned channels, create
	    // a key and give it a channels value
	    //
	    if (pms->a[ii].pi &&
		(!ii || (pms->a[ii].pi->szKey[0] && pms->a[ii].dwMask)))
	    {
#ifdef DEBUG
		AuxDebugEx (3, DEBUGLINE TEXT ("Creating key[%d] '%s'\r\n"), ii, pms->a[ii].pi->szKey);
#endif
		// find an unused keyname;
		//
		for ( ; kk < NUMELMS(pms->a); ++kk)
		{
		   wsprintf (sz, csz02d, kk);
		   if (RegOpenKey (hKey, sz, &hKeyA))
		       break;
		   RegCloseKey (hKeyA);
		}

		// create a key with that name
		//
		if (RegCreateKey (hKey, sz, &hKeyA))
		    break;

		cb = (lstrlen(pms->a[ii].pi->szKey) + 1) * sizeof(TCHAR);
		RegSetValueEx (hKeyA, NULL, 0, REG_SZ,
			       (LPBYTE)(pms->a[ii].pi->szKey),cb);

#ifdef DEBUG
		AuxDebugEx (3, DEBUGLINE TEXT ("Setting Channel Value %08X\r\n"), pms->a[ii].dwMask);
#endif
		RegSetValueEx (hKeyA, cszChannels, 0,
			       REG_DWORD,
			       (LPBYTE)&pms->a[ii].dwMask,
			       sizeof(DWORD));

		RegCloseKey (hKeyA);
	    }
	}

	RegCloseKey (hKey);
    }

    // if no HWND supplied, we are in the runonce, so we dont
    // want to kick mapper just because a scheme has changed
    //
    if (hWnd)
       KickMapper (hWnd);
    return;
}


/*+ DeleteSchemeFromReg
 *
 *-=================================================================*/

STATICFN void DeleteSchemeFromReg (
    HKEY      hkSchemes,
    LPTSTR    pszName)
{
    TCHAR sz[MAX_ALIAS];
    HKEY  hKey;
    UINT  ii;

#ifdef DEBUG
    AuxDebugEx (4, DEBUGLINE TEXT ("DeletingSchemeFromReg(%08X,'%s')\r\n"),
		hkSchemes,pszName);
#endif
    SHRegDeleteKey(hkSchemes, pszName);
/*
    // if we cannot open this key as a child of the 'schemes' key
    // we are done.
    //
    if (RegOpenKey (hkSchemes, pszName, &hKey))
	return;

    // Before we can delete a key, we must delete its children
    //
    for (ii = 0; !RegEnumKey (hKey, ii, sz, sizeof(sz)/sizeof(TCHAR)); ++ii)
    {
	// if we have an obsolete alias key, remove it now
	// otherwise create/open the alias key and set it's
	// channel property to the correct value.
	//
	AuxDebugEx (3, DEBUGLINE TEXT ("Deleting key[%d] '%s'\r\n"), ii, sz);
	RegDeleteKey (hKey, sz);
    }

    RegCloseKey (hKey);

    // now delete this key
    //
    RegDeleteKey (hkSchemes, pszName);
    return;
*/
}


/*+
 *
 *-=================================================================*/

STATICFN void LoadChannelsIntoList (
    HWND     hWnd,
    UINT     uId,
    UINT     uIdLabel,
    PMSCHEME pms)
{
    HWND  hWndT = GetDlgItem (hWnd, uId);
    RECT  rc;
    UINT  ii;
    UINT  nChan;
    int   nTabs;

    assert (pms);

    // empty the list
    //
    SetWindowRedraw (hWndT, FALSE);
    ListBox_ResetContent (hWndT);

    // calculate the width of the tabstop
    // so that the second column lines up under the indicated
    // label
    //
    GetWindowRect (GetDlgItem(hWnd, uIdLabel), &rc);
    nTabs = rc.left;
    GetWindowRect (hWnd, &rc);
    nTabs = MulDiv(nTabs - rc.left, 4, LOWORD(GetDialogBaseUnits()));
    ListBox_SetTabStops (hWndT, 1, &nTabs);

    // fill the list with channel data
    //
    for (nChan = 0; nChan < NUM_CHANNEL; ++nChan)
    {
	static CONST TCHAR cszDtabS[] = TEXT ("%d\t%s");
	TCHAR sz[MAX_ALIAS + 10];

	for (ii = 0; ii < NUMELMS(pms->a); ++ii)
	   if (pms->a[ii].dwMask & (1 << nChan))
	       break;
	assert (ii < NUMELMS(pms->a));

	wsprintf (sz, cszDtabS, nChan+1,
		  pms->a[ii].pi ? pms->a[ii].pi->szFriendly
					 : pms->szNone);
	if (nChan == (UINT)NUM_CHANNEL-1)
	   SetWindowRedraw (hWndT, TRUE);
	ListBox_InsertString (hWndT, nChan, sz);

	if (pms->dwChanMask & (1 << nChan))
	    ListBox_SetSel (hWndT, TRUE, nChan);
    }
}


/*+
 *
 *-=================================================================*/

/*+ ChannelMaskToEdit
 *
 * convert a bit mask to a string containing list of set bits
 * and bit ranges. Then SetWindowText the result into the given
 * edit control.
 *
 * This function loads prefix text from resource strings.
 *
 * For Example: ChannelMaskToEdit(....0x0000F0F) would set the text
 * 'Channels 1-4,9-12'.
 *
 *-=================================================================*/

STATICFN void ChannelMaskToEdit (
    HWND     hWnd,
    UINT     uId,
    DWORD    dwMask)
{
    HWND   hWndT = GetDlgItem (hWnd, uId);
    TCHAR  sz[NUM_CHANNEL * 4 + MAX_ALIAS];

    if (!dwMask)
	LoadString (ghInstance, IDS_NOCHAN, sz, NUMELMS(sz));
    else
    {
	LPTSTR psz;
	LPTSTR pszT;
	int    ii;
	int    iSpan;
	DWORD  dwLast;
	DWORD  dwBit;

	LoadString (ghInstance,
		    (dwMask & (dwMask-1)) ? IDS_CHANPLURAL : IDS_CHANSINGULAR,
		    sz, NUMELMS(sz));

	pszT = psz = sz + lstrlen(sz);

	for (ii = 0, dwBit = 1, dwLast = 0, iSpan = 0;
	     ii <= 32;
	     dwLast = dwMask & dwBit, ++ii, dwBit += dwBit)
	{
	    if ((dwMask & dwBit) ^ (dwLast + dwLast))
	    {
		static CONST TCHAR cszCommaD[] = TEXT (",%d");
		static CONST TCHAR cszDashD[] = TEXT ("-%d");

		if ( ! dwLast)
		    psz += wsprintf (psz, cszCommaD, ii+1);
		else if (iSpan)
		    psz += wsprintf (psz, cszDashD, ii);
		iSpan = 0;
	    }
	    else
		++iSpan;
	}

	*pszT = TEXT (' ');
    }

    SetWindowText (hWndT, sz);
}



/*+ MidiChangeCommands
 *
 *-=================================================================*/

BOOL WINAPI MidiChangeCommands (
    HWND hWnd,
    UINT wId,
    HWND hWndCtl,
    UINT wNotify)
{
    PMCLOCAL pmcl = GetDlgData(hWnd);
    PMSCHEME pms = &pmcl->ms;

#ifdef DEBUG
    AuxDebugEx (5, DEBUGLINE TEXT ("MidiChangeCommands(%08X,%d,%08X,%d)\r\n"),
		hWnd, wId, hWndCtl, wNotify);
#endif

    switch (wId)
    {
	case ID_APPLY:
	    return TRUE;

	case IDOK:
	{
	    int  ix;
	    HWND hWndT = GetDlgItem (hWnd, IDC_INSTRUMENTS);

	    ix = ComboBox_GetCurSel (hWndT);
	    if (ix >= 0)
	    {
		BOOL     bFound = FALSE;
		PINSTRUM piSel;
	BOOL     bFoundFirst = FALSE;

		piSel = (LPVOID)ComboBox_GetItemData (hWndT, ix);
		assert (!IsBadWritePtr(piSel, sizeof(*piSel)));

		// has the <none> item been selected?  in this
		// case, set stuff up so that we will not try
		// to add none to the scheme, but we will clear
		// all bits from other channels that are set
		// to none.
		//
		if ( ! piSel || ! piSel->szKey[0])
		    piSel = NULL, bFound = TRUE;

		// turn channels on for this instrument and off for
		// other instruments in this scheme.
		//
	for (ix = 0; ix < (int)NUMELMS(pms->a); ++ix)
	{
		    if (pms->a[ix].pi != piSel)
		pms->a[ix].dwMask &= ~pms->dwChanMask;
		    else if (! pms->a[ix].pi)
	    {
		if (! bFoundFirst)
		{
		    pms->a[ix].dwMask |= pms->dwChanMask;
		    bFound = TRUE;
		    bFoundFirst = TRUE;
		}
	    }
	    else
		    {
		pms->a[ix].dwMask |= pms->dwChanMask;
		bFound = TRUE;
		    }
	}

		// if this instrument was not already in the scheme,
		// find an empty slot and add it to the scheme.
		//
		if (!bFound)
	{
	    for (ix = 0; ix < (int)NUMELMS(pms->a); ++ix)
	    {
		if ( ! pms->a[ix].dwMask)
		{
		    pms->a[ix].dwMask = pms->dwChanMask;
		    pms->a[ix].pi = piSel;
		    bFound = TRUE;
		    break;
		}
	    }
	}

		assert2 (bFound, TEXT ("no room to add instrument to scheme"));
	    }
	    EndDialog (hWnd, IDOK);
	    break;
	}

	case IDCANCEL:
	    EndDialog (hWnd, IDCANCEL);
	    break;

	//
	//case ID_INIT:
	//    break;
    }

    return FALSE;
}


/*+ SaveAsDlgProc
 *
 *-=================================================================*/

const static DWORD aSaveAsHelpIds[] = {  // Context Help IDs
    IDE_SCHEMENAME,     IDH_MIDI_SAVEDLG_SCHEMENAME,

    0, 0
};

INT_PTR CALLBACK SaveAsDlgProc (
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
	case WM_COMMAND:
	    switch (GET_WM_COMMAND_ID(wParam, lParam))
	    {
		case IDOK:
		{
		    LPTSTR pszName = GetDlgData (hWnd);
		    assert (pszName);
		    GetDlgItemText (hWnd, IDE_SCHEMENAME, pszName, MAX_ALIAS);
		}
		// fall through
		case IDCANCEL:
		   EndDialog (hWnd, GET_WM_COMMAND_ID(wParam, lParam));
		   break;
	    }
	    break;
	
	case WM_CLOSE:
	    SendMessage (hWnd, WM_COMMAND, IDCANCEL, 0);
	    break;

	case WM_INITDIALOG:
	{
	    LPTSTR pszName = (LPVOID) lParam;
	    assert (pszName);
	    SetDlgData (hWnd, pszName);
	    SetDlgItemText (hWnd, IDE_SCHEMENAME, pszName);
	    break;
	}
    
	case WM_CONTEXTMENU:
	    WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU,
		    (UINT_PTR) (LPTSTR) aSaveAsHelpIds);
	    return TRUE;

	case WM_HELP:
	{
	    LPHELPINFO lphi = (LPVOID) lParam;
	    WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP,
		    (UINT_PTR) (LPTSTR) aSaveAsHelpIds);
	    return TRUE;
	}
    }

    return FALSE;
}



/*+ GetNewSchemeName
 *
 *-=================================================================*/

STATICFN BOOL WINAPI GetNewSchemeName (
    HWND     hWnd,
    HKEY     hkSchemes,
    LPTSTR   pszName)
{
    TCHAR szNew[MAX_ALIAS];
    UINT_PTR  uBtn;

    lstrcpy (szNew, pszName);

    uBtn = DialogBoxParam (ghInstance,
			   MAKEINTRESOURCE(IDD_SAVENAME),
			   hWnd,
			   SaveAsDlgProc,
			   (LPARAM)szNew);
    if (IDOK == uBtn)
    {
	if (ChildKeyExists (hkSchemes, szNew))
	    uBtn = Confirm (hWnd, IDS_QUERY_OVERSCHEME, szNew);
	else
	    lstrcpy (pszName, szNew);
    }

    return (IDOK == uBtn || IDYES == uBtn);
}


/*+ MidiChangeDlgProc
 *
 *-=================================================================*/

const static DWORD aChngInstrHelpIds[] = {  // Context Help IDs
    IDC_INSTRUMENTS,     IDH_ADDMIDI_INSTRUMENT,
    IDC_TEXT_1,          IDH_ADDMIDI_CHANNEL,
    IDE_SHOW_CHANNELS,   IDH_ADDMIDI_CHANNEL,

    0, 0
};

INT_PTR CALLBACK MidiChangeDlgProc (
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
	case WM_COMMAND:
	    HANDLE_WM_COMMAND (hWnd, wParam, lParam, MidiChangeCommands);
	    break;
	
	case WM_NOTIFY:
	    ForwardBillNotify(hWnd, (NMHDR FAR *)lParam);
	    break;
	
	case WM_CLOSE:
	    SendMessage (hWnd, WM_COMMAND, IDCANCEL, 0);
	    break;

	case WM_INITDIALOG:
	{
	    PMCLOCAL pmcl = (LPVOID) lParam;
	    PMSCHEME pms = &pmcl->ms;

	    SetDlgData (hWnd, pmcl);

	    LoadInstrumentsIntoCombo (hWnd, IDC_INSTRUMENTS, NULL, &pmcl->mcm);
	    ChannelMaskToEdit (hWnd, IDE_SHOW_CHANNELS, pms->dwChanMask);
	    break;
	}

	//case WM_DESTROY:
	//    break;

	case WM_CONTEXTMENU:
	    WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU,
		    (UINT_PTR) (LPTSTR) aChngInstrHelpIds);
	    return TRUE;

	case WM_HELP:
	{
	    LPHELPINFO lphi = (LPVOID) lParam;
	    WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP,
		    (UINT_PTR) (LPTSTR) aChngInstrHelpIds);
	    return TRUE;
	}
    }

    return FALSE;
}


/*+ MidiConfigCommands
 *
 *-=================================================================*/

BOOL WINAPI MidiConfigCommands (
    HWND hWnd,
    UINT wId,
    HWND hWndCtl,
    UINT wNotify)
{
    PMCLOCAL pmcl = GetDlgData(hWnd);
    PMSCHEME pms  = &pmcl->ms;

#ifdef DEBUG
    AuxDebugEx (5, DEBUGLINE TEXT ("MidiConfigCommands(%08X,%d,%08X,%d)\r\n"),
		hWnd, wId, hWndCtl, wNotify);
#endif

    switch (wId)
    {
	case IDB_CHANGE:
	{
	    UINT_PTR uRet;
	    int      ii;
	    HWND     hWndList = GetDlgItem (hWnd, IDL_CHANNELS);

#ifdef DEBUG
	    AuxDebugEx (2, DEBUGLINE TEXT ("Launching Change Dialog\r\n"));
#endif
	    pms->dwChanMask = 0;
	    for (ii = 0; ii < NUM_CHANNEL; ++ii)
		 if (ListBox_GetSel (hWndList, ii))
		    pms->dwChanMask |= (1 << ii);

	    uRet = DialogBoxParam (ghInstance,
				   MAKEINTRESOURCE(IDD_MIDICHANGE),
				   hWnd,
				   MidiChangeDlgProc,
				   (LPARAM)pmcl);
	    if (uRet == IDOK)
	    {
	       LoadChannelsIntoList (hWnd, IDL_CHANNELS, IDC_TEXT_1, pms);
	       pms->bDirty = TRUE;
	    }
	    break;
	}

	case IDB_DELETE:
	    if (IsSzEqual(pmcl->szScheme, pmcl->szDefault))
	    {
		#pragma message ("error message here?")
		break;
	    }
	    if (Confirm (hWnd, IDS_QUERY_DELETESCHEME, pmcl->szScheme) == IDYES)
	    {
		HWND hWndCtl = GetDlgItem (hWnd, IDC_SCHEMES);
		int  ix = ComboBox_FindStringExact (hWndCtl, -1, pmcl->szScheme);
		assert (ix >= 0);

		DeleteSchemeFromReg (pms->hkSchemes, pmcl->szScheme);

		ComboBox_DeleteString (hWndCtl, ix);
		ComboBox_SetCurSel (hWndCtl, 0);
		SimulateNotify (hWnd, IDC_SCHEMES, CBN_SELCHANGE);
	    }
	    break;

	case IDB_SAVE_AS:
	    if (GetNewSchemeName (hWnd, pms->hkSchemes, pmcl->szScheme))
	    {
		SaveSchemeToReg (&pmcl->mcm, pms, pmcl->szScheme, hWnd);

		LoadSchemesIntoCombo (hWnd, IDC_SCHEMES,
				      pmcl->szScheme, pms);
	    }
	    SimulateNotify (hWnd, IDC_SCHEMES, CBN_SELCHANGE);
	    break;

	case IDC_SCHEMES:
	    if (wNotify == CBN_SELCHANGE)
	    {
		int   ix;

		ix = ComboBox_GetCurSel (hWndCtl);
		if (ix >= 0)
		    ComboBox_GetLBText (hWndCtl, ix, pmcl->szScheme);

		LoadSchemeFromReg (&pmcl->mcm, pms, pmcl->szScheme);

		pms->dwChanMask = 0;
		LoadChannelsIntoList (hWnd, IDL_CHANNELS, IDC_TEXT_1, pms);

		EnableWindow (GetDlgItem (hWnd, IDB_DELETE),
			      !IsSzEqual(pmcl->szDefault, pmcl->szScheme));
	    }
	    break;

	case IDL_CHANNELS:
	    if (wNotify == LBN_SELCHANGE)
	    {
		int ix;

		ix = ListBox_GetSelCount (hWndCtl);
		EnableWindow (GetDlgItem (hWnd, IDB_CHANGE), (ix > 0));
	    }
	    break;

	case IDOK:
	{
	    SaveSchemeToReg (&pmcl->mcm, pms, pmcl->szScheme, hWnd);

	    EndDialog (hWnd, IDOK);
	    break;
	}

	case IDCANCEL:
	    EndDialog (hWnd, IDCANCEL);
	    break;
    }

    return FALSE;
}


/*+ MidiConfigDlgProc
 *
 *-=================================================================*/

const static DWORD aMidiConfigHelpIds[] = {  // Context Help IDs
    IDC_GROUPBOX,    IDH_MMSE_GROUPBOX,
    IDC_SCHEMES,     IDH_MIDI_CFGDLG_SCHEME,
    IDB_SAVE_AS,     IDH_MIDI_CFGDLG_SAVEAS,
    IDB_DELETE,      IDH_MIDI_CFGDLG_DELETE,
    IDC_GROUPBOX_2,  IDH_MMSE_GROUPBOX,
    IDL_CHANNELS,    IDH_MIDI_INSTRUMENTS,
    IDB_CHANGE,      IDH_MIDI_CFGDLG_CHANGE,
    IDC_TEXT_1,      IDH_MIDI_INSTRUMENTS,
    IDC_TEXT_2,      IDH_MIDI_INSTRUMENTS,
    IDC_TEXT_3,      NO_HELP,

    0, 0
};

INT_PTR CALLBACK MidiConfigDlgProc (
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
	case WM_COMMAND:
	    HANDLE_WM_COMMAND (hWnd, wParam, lParam, MidiConfigCommands);
	    break;
	
	case WM_NOTIFY:
	    ForwardBillNotify(hWnd, (NMHDR FAR *)lParam);
	    break;

	case WM_CLOSE:
	    SendMessage (hWnd, WM_COMMAND, IDCANCEL, 0);
	    break;

	case WM_INITDIALOG:
	{
	    PMCLOCAL pmcl = (LPVOID) lParam;
	    
	    assert (pmcl);

	    SetDlgData (hWnd, pmcl);
	    LoadString (ghInstance, IDS_NONE, pmcl->ms.szNone, NUMELMS(pmcl->ms.szNone));

	    LoadSchemesIntoCombo (hWnd, IDC_SCHEMES, pmcl->szScheme, &pmcl->ms);
	    SimulateNotify (hWnd, IDC_SCHEMES, CBN_SELCHANGE);

	    EnableWindow (GetDlgItem(hWnd, IDB_CHANGE), FALSE);
	    break;
	}

	case WM_CONTEXTMENU:
	    WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU,
		    (UINT_PTR) (LPTSTR) aMidiConfigHelpIds);
	    return TRUE;

	case WM_HELP:
	{
	    LPHELPINFO lphi = (LPVOID) lParam;
	    WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP,
		    (UINT_PTR) (LPTSTR) aMidiConfigHelpIds);
	    return TRUE;
	}
    }

    return FALSE;
}


STATICFN void WINAPI PickMidiInstrument(
    LPTSTR   pszKey)
{
    HKEY            hKeyMR, hKeyDriver;
    UINT            cDevs;
    UINT            ii, jj;
    DWORD           cbSize, dwType;
    LPMIDIOUTCAPS   pmoc;
    MMRESULT        mmr;
    PWSTR           pszDevIntDev, pszDevIntKey;
    UINT            aTech[]  = { MOD_SWSYNTH,
                                 MOD_WAVETABLE,
                                 MOD_SYNTH,
                                 MOD_FMSYNTH,
                                 MOD_SQSYNTH,
                                 MOD_MIDIPORT};
    UINT            cTech    = sizeof(aTech)/sizeof(aTech[0]);
    TCHAR           szKey[MAX_ALIAS];
    TCHAR           szPname[MAXPNAMELEN];
    TCHAR           szPnameTarget[MAXPNAMELEN];
    LONG            lr;

    szPname[0] = 0;
    cDevs = midiOutGetNumDevs();

    if (0 == cDevs)
    {
        return;
    }

    pmoc = (LPMIDIOUTCAPS)LocalAlloc (LPTR, cDevs * sizeof(MIDIOUTCAPS));

    if (NULL == pmoc)
    {
        return;
    }

    for (ii = cDevs; ii; ii--)
    {
        mmr = midiOutGetDevCaps(ii - 1, &(pmoc[ii - 1]), sizeof(MIDIOUTCAPS));

        if (MMSYSERR_NOERROR != mmr)
        {
            LocalFree ((HLOCAL)pmoc);
            return;
        }
    }

    for (ii = 0; ii < cTech; ii++)
    {
        for (jj = cDevs; jj; jj--)
        {
            if (pmoc[jj - 1].wTechnology == aTech[ii])
            {
                lstrcpy(szPname, pmoc[jj - 1].szPname);
                break;
            }
        }

        if (jj)
        {
            //  Broke out of inner loop, found match

            break;
        }
    }

    LocalFree ((HLOCAL)pmoc);

    if (0 == jj)
    {
        //  This should never happen...

        return;
    }

    jj--;

    mmr = midiOutMessage ((HMIDIOUT)jj, DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)(PULONG)&cbSize, 0L);

    if (MMSYSERR_NOERROR != mmr)
    {
        return;
    }

	pszDevIntDev = (PWSTR)LocalAlloc (LPTR, cbSize);

    if (NULL == pszDevIntDev)
    {
        return;
    }

    mmr = midiOutMessage ((HMIDIOUT)jj, DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)pszDevIntDev, (DWORD)cbSize);

    if (MMSYSERR_NOERROR != mmr)
    {
        LocalFree ((HLOCAL)pszDevIntDev);
        return;
    }

    lr = RegOpenKey(HKEY_LOCAL_MACHINE, cszDriversRoot, &hKeyMR);

    if (ERROR_SUCCESS != lr)
    {
        LocalFree ((HLOCAL)pszDevIntDev);
        return;
    }

    for (ii = 0; ; )
    {
        lr = RegEnumKey(hKeyMR, ii++, szKey, sizeof(szKey));

        if (ERROR_SUCCESS != lr)
        {
            RegCloseKey(hKeyMR);
            LocalFree ((HLOCAL)pszDevIntDev);
            return;
        }

        lr = RegOpenKey(hKeyMR, szKey, &hKeyDriver);

        if (ERROR_SUCCESS != lr)
        {
            RegCloseKey(hKeyMR);
            LocalFree ((HLOCAL)pszDevIntDev);
            return;
        }

        cbSize = sizeof(szPnameTarget);

        lr = RegQueryValueEx(
                hKeyDriver,
                cszActive,
                NULL,
                &dwType,
                (LPSTR)szPnameTarget,
                &cbSize);

        if (ERROR_SUCCESS != lr)
        {
            RegCloseKey(hKeyDriver);
            RegCloseKey(hKeyMR);
            LocalFree ((HLOCAL)pszDevIntDev);
            return;
        }

        if (TEXT('1') != szPnameTarget[0])
        {
            RegCloseKey(hKeyDriver);
            continue;
        }

        cbSize = sizeof(szPnameTarget);

        lr = RegQueryValueEx(
                hKeyDriver,
                cszDescription,
                NULL,
                &dwType,
                (LPSTR)szPnameTarget,
                &cbSize);

        if (ERROR_SUCCESS != lr)
        {
            RegCloseKey(hKeyDriver);
            RegCloseKey(hKeyMR);
            LocalFree ((HLOCAL)pszDevIntDev);
            return;
        }

        if (0 != lstrcmp(szPnameTarget, szPname))
        {
            RegCloseKey(hKeyDriver);
            continue;
        }

        cbSize = 0;

        lr = RegQueryValueExW (
                hKeyDriver,
                L"DeviceInterface",
                NULL,
                &dwType,
                (LPSTR)NULL,
                &cbSize);

        if (ERROR_SUCCESS != lr)
        {
            RegCloseKey(hKeyDriver);
            RegCloseKey(hKeyMR);
            LocalFree ((HLOCAL)pszDevIntDev);
            return;
        }

        pszDevIntKey = (PWSTR) LocalAlloc (LPTR, cbSize);

        if (NULL == pszDevIntKey)
        {
            RegCloseKey(hKeyDriver);
            RegCloseKey(hKeyMR);
            LocalFree ((HLOCAL)pszDevIntDev);
            return;
        }

        lr = RegQueryValueExW (
                hKeyDriver,
                L"DeviceInterface",
                NULL,
                &dwType,
                (LPSTR)pszDevIntKey,
                &cbSize);

        RegCloseKey(hKeyDriver);

        if (ERROR_SUCCESS != lr)
        {
            LocalFree ((HLOCAL)pszDevIntKey);
            RegCloseKey(hKeyMR);
            LocalFree ((HLOCAL)pszDevIntDev);
            return;
        }

        if (0 == lstrcmpiW(pszDevIntKey, pszDevIntDev))
        {
            LocalFree ((HLOCAL)pszDevIntKey);
            RegCloseKey(hKeyMR);
            LocalFree ((HLOCAL)pszDevIntDev);
            lstrcpy(pszKey, szKey);
            return;
        }

        LocalFree ((HLOCAL)pszDevIntKey);
    }
}


/*+ SaveLocal
 *
 *-=================================================================*/

STATICFN void WINAPI SaveLocal (
    PMCLOCAL pmcl,
    BOOL     bUserSetting,
    HWND     hWnd)  // optional window to report errors: NULL - no reports
{
    HKEY hKey;
    UINT cb;

#ifdef DEBUG
    AuxDebugEx (2, DEBUGLINE TEXT ("SaveLocal(%08X,%X) %s\r\n"),
		pmcl, hWnd, pmcl->pszReason ? pmcl->pszReason : TEXT (""));
#endif
    
    if (RegCreateKey (HKEY_CURRENT_USER, cszMidiMapRoot, &hKey) == ERROR_SUCCESS)
    {
    	cb = (lstrlen(pmcl->szScheme) + 1) * sizeof(TCHAR);
	    RegSetValueEx (hKey, cszCurrentScheme, 0, REG_SZ,
		               (LPBYTE)pmcl->szScheme, cb);

	    //assert (pmcl->piSingle);
	    if ((pmcl->piSingle) && (pmcl->piSingle->bActive))
	    {
#ifdef DEBUG
	        AuxDebugEx (2, DEBUGLINE TEXT ("Setting CurrentInstrument Key to %08X '%s'\r\n"),
			            pmcl->piSingle, pmcl->piSingle->szKey);
#endif
    	    cb = (lstrlen(pmcl->piSingle->szKey) + 1) * sizeof(TCHAR);
	        RegSetValueEx (hKey, cszCurrentInstrument, 0, REG_SZ,
		    	           (LPBYTE)(pmcl->piSingle->szKey),
			               cb);
	    }
	    else
	    {
            // Assume No Match

            TCHAR   szKey[MAX_ALIAS];
            LONG    lr;

            szKey[0] = 0;

	        RegSetValueEx (hKey, cszCurrentInstrument, 0, REG_SZ, (LPBYTE)cszEmpty, 0);
            PickMidiInstrument(szKey);

	        cb = (lstrlen(szKey) + 1) * sizeof(TCHAR);
	        lr = RegSetValueEx (hKey, cszCurrentInstrument, 0, REG_SZ,
			                (LPBYTE)(szKey), cb);
            
	    }

	    RegSetValueEx (hKey, cszUseScheme, 0, REG_DWORD,
		               (LPBYTE)&pmcl->bUseScheme, sizeof(pmcl->bUseScheme));

	    if (bUserSetting)
	        pmcl->bAutoScheme = FALSE;

	    RegSetValueEx (hKey, cszAutoScheme, 0, REG_DWORD,
		               (LPBYTE)&pmcl->bAutoScheme, sizeof(pmcl->bAutoScheme));

	    RegSetValueEx (hKey, cszRunOnceCount, 0, REG_DWORD,
		               (LPBYTE)&pmcl->dwRunCount, sizeof(pmcl->dwRunCount));

	    if (pmcl->pszReason)
	    {
	        cb = (lstrlen(pmcl->pszReason) + 1) * sizeof(TCHAR);
	        RegSetValueEx (hKey, cszDriverList, 0, REG_SZ,
			               (LPBYTE)pmcl->pszReason, cb);
	    }
	    else
	        RegSetValueEx (hKey, cszDriverList, 0, REG_SZ, (LPBYTE)cszEmpty, 0);

	    RegCloseKey (hKey);

	    // Don't Kick mapper unless we have a window
	    if (hWnd)
	        KickMapper (hWnd);
    }
}


/*+ InitLocal
 *
 *-=================================================================*/

STATICFN void WINAPI InitLocal (
    PMCLOCAL pmcl,
    LPARAM   lParam,
    BOOL     bDriverAsAlias) // driver as alias mode used only for scheme init
{
    HKEY hKey;

    LoadString (ghInstance, IDS_DEFAULT_SCHEME_NAME,
		pmcl->szDefault, NUMELMS(pmcl->szDefault));

    // we allow driver as alias (szFriendly) only for InitLocal when called
    // from RunOnceSchemeInit.  this works because in this case we have
    // no UI so we dont need the friendly names for anything.
    //
    assert (!bDriverAsAlias || lParam == 0);
    LoadInstruments (&pmcl->mcm, bDriverAsAlias);

   #ifdef DEBUG
    if (mmdebug_OutputLevel >= 3)
       DumpInstruments (&pmcl->mcm);
   #endif

    if (RegCreateKey (HKEY_CURRENT_USER, cszMidiMapRoot, &hKey) == ERROR_SUCCESS)
    {
	DWORD cb;
	DWORD dwType;
	TCHAR szSingle[MAX_ALIAS];

	cb = sizeof(pmcl->szScheme);
	if (RegQueryValueEx (hKey, cszCurrentScheme, NULL, &dwType, (LPBYTE)pmcl->szScheme, &cb))
	    pmcl->szScheme[0] = 0;

	pmcl->piSingle = NULL;
	cb = sizeof(szSingle);
	if (!RegQueryValueEx (hKey, cszCurrentInstrument, NULL, &dwType, (LPBYTE)szSingle, &cb))
	    pmcl->piSingle = FindInstrumentFromKey (&pmcl->mcm, szSingle);

	cb = sizeof(pmcl->bUseScheme);
	if (RegQueryValueEx (hKey, cszUseScheme, NULL, &dwType, (LPBYTE)&pmcl->bUseScheme, &cb))
	    pmcl->bUseScheme = 0;

	cb = sizeof(pmcl->bAutoScheme);
	if (RegQueryValueEx (hKey, cszAutoScheme, NULL, &dwType, (LPBYTE)&pmcl->bAutoScheme, &cb))
	    pmcl->bAutoScheme = TRUE;

	cb = sizeof(pmcl->dwRunCount);
	if (RegQueryValueEx (hKey, cszRunOnceCount, NULL, &dwType, (LPBYTE)&pmcl->dwRunCount, &cb))
	    pmcl->dwRunCount = 0;

	pmcl->pszReason = NULL;

	RegCloseKey (hKey);
    }

    pmcl->ppsp = (LPVOID)lParam;
}


/*+ FixupHinderedIDFs
 *
 *-=================================================================*/

VOID WINAPI FixupHinderedIDFs (
    PMCLOCAL pmcl,
    LPTSTR   pszTemp,  // ptr to temp memory
    UINT     cchTemp)  // size of temp memory
{
    HKEY     hkHind; // hinderedMidiList root
    LPTSTR   pszDriver = pszTemp; // max size is short filename
    UINT     cch;
    LPTSTR   pszIDF = (LPVOID)(pszTemp + MAX_PATH);
    UINT     cbSize;
    DWORD    iEnum;
    DWORD    dwType;

#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("FixupHinderedIDFs(%08x)\r\n"), pmcl);
#endif
    assert (pszTemp);
    assert (cchTemp > MAX_PATH + MAX_PATH + 64);

    // the midi key should have already been opened.
    //
    assert (pmcl->mcm.hkMidi);

    if (RegCreateKey (HKEY_LOCAL_MACHINE, cszHinderedMidiList, &hkHind))
	return;

    // enumerate the known hindered driver list looking for
    // drivers that need to have their IDF's set
    //
    for (iEnum = 0, cch = MAX_PATH, cbSize = (MAX_PATH + 64) * sizeof(TCHAR);
	 ! RegEnumValue (hkHind, iEnum, pszDriver, &cch, NULL, &dwType, (LPBYTE)pszIDF, &cbSize);
	 ++iEnum, cch = MAX_PATH, cbSize = (MAX_PATH + 64) * sizeof(TCHAR))
    {
	UINT ii;

#ifdef DEBUG
	AuxDebugEx (3, DEBUGLINE TEXT ("enum[%d] pszDriver='%s' pszIDF='%s'\r\n"), iEnum, pszDriver, pszIDF);
#endif
	// just to be careful.  ignore any registry entry that
	// does not have string data
	//
	assert (dwType == REG_SZ);
	if (dwType != REG_SZ)
	    continue;

	// scan through the list of drivers looking for one that is
	// internal, and has the same driver name as one of our known
	// list of hindered drivers.  if we find one, force its
	// IDF to be the given IDF
	//
	for (ii = 0; ii < pmcl->mcm.nInstr; ++ii)
	{
	    PINSTRUM pi = pmcl->mcm.api[ii];
	    HKEY hkSub;

	    if (!pi || !pi->szKey[0] || pi->bExternal ||
		!IsSzEqual (pi->szFriendly, pszDriver))
		continue;

#ifdef DEBUG
	    AuxDebugEx (2, DEBUGLINE TEXT ("forcing driver '%s' to use IDF '%s'\r\n"), pi->szKey, pszIDF);
#endif
	    if ( ! RegOpenKeyEx (pmcl->mcm.hkMidi, pi->szKey, 0, KEY_ALL_ACCESS, &hkSub))
	    {
		RegSetValueEx (hkSub, cszDefinition, 0, REG_SZ, (LPBYTE)pszIDF, cbSize);
		RegCloseKey (hkSub);
	    }
	}
    }

    RegCloseKey (hkHind);
    return;
}


/*+ RunOnceSchemeInit
 *
 *-=================================================================*/

DWORD WINAPI RunOnceSchemeInit (HWND hwnd, HINSTANCE hInst, LPTSTR szCmd, int nShow)
{
    PMCLOCAL pmcl;
    BOOL     bHasExternal = FALSE;
    UINT     ii;
    PINSTRUM piFirst = NULL;
    LPTSTR   pszReason;
    LPTSTR   pszDefScheme;
    HANDLE   hMutex = NULL;
    LPVOID   pAlloc;
    UINT     cbSize;

    DebugSetOutputLevel (GetProfileInt( TEXT ("Debug"), TEXT ("midiprop"), 0));
#ifdef DEBUG
    AuxDebugEx (1, TEXT("\r\n"));
#endif
    // allocate memory for large temp variables off the heap
    // since we may be called from 16 bit context via thunk.
    //
    cbSize = sizeof(*pmcl) + (MAX_ALIAS + 1024) * sizeof(TCHAR);
    pAlloc = LocalAlloc (LPTR, cbSize);
    if (!pAlloc)
    {
#ifdef DEBUG
	AuxDebugEx (0, DEBUGLINE TEXT ("failed to alloc temp mem"));
#endif
	return MMSYSERR_NOMEM;
    }
    pmcl = pAlloc;
    pszDefScheme = (LPTSTR)(pmcl+1);
    pszReason = pszDefScheme + MAX_ALIAS;

    // prevent reentrancy in this code
    //
    hMutex = CreateMutex (NULL, FALSE, TEXT ("MidiSchemeRunonce"));
    if (hMutex)
    {
	assert2(GetLastError() != ERROR_ALREADY_EXISTS, TEXT ("runonce mutex already existed\r\n"));
#ifdef DEBUG
	AuxDebugEx (1, DEBUGLINE TEXT ("RunOnceSchemeInit waiting on Mutex %08X\r\n"), hMutex);
#endif
	WaitForSingleObject (hMutex, INFINITE);
    }

    ZeroMemory (pmcl, sizeof(*pmcl));
    InitLocal (pmcl, 0, TRUE); // load local w/ driver names as szFriendly

    // fix idf file references for the known list of hindered midi
    // drivers
    //
    FixupHinderedIDFs (pmcl, pszReason, 1024);

    // get default scheme name, and make it the current scheme if there
    // is not a current scheme
    //
    LoadString (ghInstance, IDS_DEFAULT_SCHEME_NAME, pszDefScheme, MAX_ALIAS);
    if ( ! pmcl->szScheme[0])
	lstrcpy (pmcl->szScheme, pszDefScheme);

    ++pmcl->dwRunCount;
    GetTimeFormat (LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, pszReason, 1024/sizeof(TCHAR));
    wsprintf (pszReason + lstrlen(pszReason), TEXT (", %d, "), pmcl->dwRunCount);

    if (!pmcl->mcm.hkMidi)
    {
	    if (RegCreateKey (HKEY_LOCAL_MACHINE, cszDriversRoot, &pmcl->mcm.hkMidi))
		goto cleanup;
    }

    if (RegCreateKey (HKEY_LOCAL_MACHINE, cszSchemeRoot, &pmcl->ms.hkSchemes))
	goto cleanup;

    //
    // determine if there are any external instruments, and set
    // piFirst to the first internal instrument we find.  if there
    // are no internal instruments, set piFirst to the first instrument.
    //
#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("RSEI: scanning %d instruments for first internal\r\n"), pmcl->mcm.nInstr);
#endif

    for (piFirst = NULL, ii = 0; ii < pmcl->mcm.nInstr; ++ii)
    {
	PINSTRUM pi = pmcl->mcm.api[ii];

	if (!pi)
	    continue;

	if (pi->bExternal)
	    bHasExternal = TRUE;
	else if ( ! piFirst && pi->bActive && pi->szKey[0])
	    piFirst = pi;

	if (pi->szKey[0])
	{
	   lstrcat (pszReason, pi->szKey);
	   if (pi->bActive)
	       lstrcat(pszReason, TEXT (":Active, "));
	   else
	       lstrcat(pszReason, TEXT (":Disabled, "));
	}

#ifdef DEBUG
	AuxDebugEx (3, DEBUGLINE TEXT ("RSEI:scan [%d] pi = %08x piFirst = %08x\r\n"),
		    ii, pi, piFirst);
#endif
    }

#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("RSEI:post scan piFirst = %08x\r\n"), piFirst);
#endif
    if ( ! piFirst)
	piFirst = pmcl->mcm.api[0];

    // if there is not already a single instrument chosen, or if
    // the previous time the single instrument was auto chosen.
    // choose the first internal instrument as the single instrument
    //
    if (( ! pmcl->piSingle) || pmcl->bAutoScheme)
    {
	pmcl->piSingle = piFirst;
	pmcl->bAutoScheme = TRUE;
    }

#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("RSEI:pmcl->piFirst = %08x\r\n"), pmcl->piSingle);
#endif
    // load default scheme
    //
    LoadSchemeFromReg (&pmcl->mcm, &pmcl->ms, pszDefScheme);

    // if this scheme has no children. then
    // assign all of its channels to the first
    // instrument we find.
    //
    if ( ! pmcl->ms.nChildren)
    {
	pmcl->ms.nChildren = 1;
	pmcl->ms.a[0].dwMask = 0x0000FFFF;
	pmcl->ms.a[0].pi   = piFirst;  // default to first instrument

	// slam a dummy (none) alias that matches all channels
	// at the end of our channel/alias list to be sure that

	// the loop to save schemes terminates.
	//
	pmcl->ms.a[1].dwMask = (DWORD)~0;
	pmcl->ms.a[1].pi = NULL;
    }

    // save the scheme back, this will either correct a
    // slightly damaged scheme, or create a new one
    //
    SaveSchemeToReg (&pmcl->mcm, &pmcl->ms, pszDefScheme, NULL);

#if 0		// Leave unset
    // save 'current user' choices
    //
    pmcl->pszReason = pszReason;
    SaveLocal (pmcl, FALSE, NULL);
#endif
    pmcl->pszReason = NULL;

  cleanup:
    FreeInstruments (&pmcl->mcm);

    if (pmcl->mcm.hkMidi)
	RegCloseKey (pmcl->mcm.hkMidi), pmcl->mcm.hkMidi = NULL;
    if (pmcl->ms.hkSchemes)
	RegCloseKey (pmcl->ms.hkSchemes), pmcl->ms.hkSchemes = NULL;

    LocalFree (pAlloc);
   #ifdef DEBUG
    pAlloc = NULL, pmcl = NULL, pszReason = NULL, pszDefScheme = NULL;
   #endif

    // if we are operating under a mutex, release and close it.
    //
    if (hMutex)
    {
	ReleaseMutex (hMutex);
	CloseHandle (hMutex);
    }
    return 0;
}

STDAPI_(void) HandleSynthAboutBox(HWND hWnd)
{
    HWND hTree =  GetDlgItem(hWnd, IDL_INSTRUMENTS);
    HTREEITEM hItem = TreeView_GetSelection(hTree);
    TV_ITEM       ti;
    PINSTRUM      pi;

    memset(&ti, 0, sizeof(ti));
    ti.mask       = TVIF_PARAM;
    ti.hItem      = hItem;
    
    TreeView_GetItem (hTree, &ti);
    pi = (LPVOID)ti.lParam; 

    if (pi)
    {
        UINT uWaveID;

        if (GetWaveID(&uWaveID) != (MMRESULT)MMSYSERR_ERROR)
        {
            WAVEOUTCAPS woc;

            if (waveOutGetDevCaps(uWaveID, &woc, sizeof(woc)) == MMSYSERR_NOERROR)
            {
                RolandProp(hWnd, ghInstance, woc.szPname);
            }  
        }
    }
}



/*+ MidiCplCommands
 *
 *-=================================================================*/

BOOL WINAPI MidiCplCommands (
    HWND hWnd,
    UINT wId,
    HWND hWndCtl,
    UINT wNotify)
{
    PMCLOCAL pmcl = GetDlgData(hWnd);

#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("MidiCplCommands(%08X,%X,%08X,%d)\r\n"),
		hWnd, wId, hWndCtl, wNotify);
#endif

    assert (pmcl);
    if (!pmcl)
	return FALSE;

    switch (wId)
    {
	case IDB_CONFIGURE:
	{
	    UINT_PTR uRet;
	    TCHAR    szOld[MAX_ALIAS];

#ifdef DEBUG
	    AuxDebugEx (2, DEBUGLINE TEXT ("Launching Config Dialog\r\n"));
#endif

	    lstrcpy (szOld, pmcl->szScheme);
	    uRet = DialogBoxParam (ghInstance,
				   MAKEINTRESOURCE(IDD_MIDICONFIG),
				   hWnd,
				   MidiConfigDlgProc,
				   (LPARAM)pmcl);
	    if (uRet != IDOK)
		lstrcpy (pmcl->szScheme, szOld);
	    else
		PropSheet_Changed(GetParent(hWnd), hWnd);

	    LoadSchemesIntoCombo (hWnd, IDC_SCHEMES, pmcl->szScheme, &pmcl->ms);
	}
	    break;

    case IDC_ABOUTSYNTH:
    {
        HandleSynthAboutBox(hWnd);
    }
    break;

	case IDB_ADDWIZ:
#ifdef DEBUG
	    AuxDebugEx (2, DEBUGLINE TEXT ("Launching Midi Wizard\r\n"));
#endif
	    MidiInstrumentsWizard (hWnd, &pmcl->mcm, NULL);

	    LoadInstruments (&pmcl->mcm, FALSE);
	   #ifdef DEBUG
	    if (mmdebug_OutputLevel >= 3)
	       DumpInstruments (&pmcl->mcm);
	   #endif

	    if (pmcl->bDlgType2)
	    {
		LoadInstrumentsIntoTree (hWnd, IDL_INSTRUMENTS, IDC_INSTRUMENTS,
					 pmcl->piSingle, pmcl);
	    }
	    else
	    {
		LoadInstrumentsIntoCombo (hWnd, IDC_INSTRUMENTS,
					  pmcl->piSingle, &pmcl->mcm);
	    }
	    MMExtPropSheetCallback(MM_EPS_BLIND_TREECHANGE, 0,0,0);
	    break;

	case IDC_SCHEMES:
	    if (wNotify == CBN_SELCHANGE)
	    {
		int   ix;

		ix = ComboBox_GetCurSel (hWndCtl);
		if (ix >= 0)
		    ComboBox_GetLBText (hWndCtl, ix, pmcl->szScheme);
		PropSheet_Changed(GetParent(hWnd), hWnd);
	    }
	    break;

	case IDC_INSTRUMENTS:
	    if (wNotify == CBN_SELCHANGE)
	    {
		int   ix;

		assert (!pmcl->bDlgType2);

		ix = ComboBox_GetCurSel (hWndCtl);
		if (ix >= 0)
		{
		    pmcl->piSingle = (LPVOID)ComboBox_GetItemData (hWndCtl, ix);
		}
		PropSheet_Changed(GetParent(hWnd), hWnd);
	    }
	    break;

	case IDC_RADIO_CUSTOM:
	case IDC_RADIO_SINGLE:
	    {
	    BOOL bUseScheme = pmcl->bUseScheme;
	    pmcl->bUseScheme = IsDlgButtonChecked (hWnd, IDC_RADIO_CUSTOM);
	    if (bUseScheme != pmcl->bUseScheme)
		PropSheet_Changed(GetParent(hWnd), hWnd);

	    if (pmcl->bDlgType2)
	    {
		HWND hWndCtl;
		EnableWindow(GetDlgItem (hWnd, IDL_INSTRUMENTS), !pmcl->bUseScheme);
		if (hWndCtl = GetDlgItem (hWnd, IDB_DETAILS))
		    EnableWindow(hWndCtl, !pmcl->bUseScheme);
	    }
	    EnableWindow(GetDlgItem (hWnd, IDC_INSTRUMENTS), !pmcl->bUseScheme);
	    EnableWindow(GetDlgItem (hWnd, IDC_SCHEMES), pmcl->bUseScheme);
	    EnableWindow(GetDlgItem (hWnd, IDC_SCHEMESLABEL), pmcl->bUseScheme);
	    EnableWindow(GetDlgItem (hWnd, IDB_CONFIGURE), pmcl->bUseScheme);
	    }
	    break;

       #if 1 //def DETAILS_FROM_MAIN_CPL
	case IDB_DETAILS:
	    assert (pmcl->bDlgType2);
	    if (pmcl->bDlgType2)
	    {
		TCHAR szSingle[MAX_PATH];
		int  ix;

		ix = ComboBox_GetCurSel (hWndCtl);
		if (ix >= 0)
		    pmcl->piSingle = (LPVOID)ComboBox_GetItemData (hWndCtl, ix);

		szSingle[0] = 0;
		if (pmcl->piSingle)
		    lstrcpy (szSingle, pmcl->piSingle->szKey);

		if (ShowDetails (hWnd, pmcl))
		{
		    LoadInstruments (&pmcl->mcm, FALSE);
		    pmcl->piSingle = FindInstrumentFromKey (&pmcl->mcm, szSingle);

		    LoadInstrumentsIntoTree (hWnd, IDL_INSTRUMENTS,
					     IDC_INSTRUMENTS, pmcl->piSingle,
					     pmcl);
		}
	    }
	    break;
       #endif

	case ID_INIT:
	    LoadInstruments (&pmcl->mcm, FALSE);
	   #ifdef DEBUG
	    if (mmdebug_OutputLevel >= 3)
	       DumpInstruments (&pmcl->mcm);
	   #endif

	    if (pmcl->bDlgType2)
	    {
		LoadInstrumentsIntoTree (hWnd, IDL_INSTRUMENTS, IDC_INSTRUMENTS,
					 pmcl->piSingle, pmcl);
	    }
	    else
	    {
		LoadInstrumentsIntoCombo (hWnd, IDC_INSTRUMENTS,
					  pmcl->piSingle, &pmcl->mcm);
	    }
	    break;

	case ID_APPLY:
	    pmcl->bUseScheme = IsDlgButtonChecked (hWnd, IDC_RADIO_CUSTOM);
	    SaveLocal (pmcl, TRUE, hWnd);
	    break;

	case IDOK:
	    pmcl->bUseScheme = IsDlgButtonChecked (hWnd, IDC_RADIO_CUSTOM);
	    break;

	case IDCANCEL:
	    break;

	//
	//case ID_INIT:
	//    break;
    }

    return FALSE;
}


/*+ HandleInstrumentsSelChange
 *
 *-=================================================================*/

STATICFN BOOL WINAPI HandleInstrumentsSelChange (
    HWND     hWnd,
    LPNMHDR  lpnm)
{
    PMCLOCAL      pmcl = GetDlgData(hWnd);
    LPNM_TREEVIEW pntv = (LPVOID)lpnm;
    LPTV_ITEM     pti  = &pntv->itemNew;
    TV_ITEM       ti;
    PINSTRUM      pi;
    TCHAR         szSingle[MAX_ALIAS];
    BOOL          bChange = FALSE;

    if (!pmcl || pmcl->bIgnoreSelChange)
	return FALSE;

    // setup ti to get text & # of children
    // from the IDF filename entry.
    //
    ti.mask       = TVIF_TEXT | TVIF_PARAM;
    ti.pszText    = szSingle;
    ti.cchTextMax = NUMELMS(szSingle);
    ti.hItem      = pti->hItem;

    TreeView_GetItem (lpnm->hwndFrom, &ti);
    pi = (LPVOID)ti.lParam; // FindInstrument (&pmcl->mcm, szSingle);

#ifdef DEBUG
    AuxDebugEx (2, DEBUGLINE TEXT ("HandInstSelChg(%X,...) %X %X Init=%d\r\n"),
		hWnd, pmcl->piSingle, pi, !pmcl->bPastInit);
#endif

    SetDlgItemText (hWnd, IDC_INSTRUMENTS, szSingle);
    if (pmcl->piSingle != pi)
    {
	EnableWindow(GetDlgItem(hWnd,IDC_ABOUTSYNTH),pi->fGSSynth);
    bChange = TRUE;
	pmcl->piSingle = pi;
    }

    return (bChange && pmcl->bPastInit);
}


/*+ MidiCplDlgProc
 *
 *-=================================================================*/

const static DWORD aKeyWordIds[] = {  // Context Help IDs
    IDC_GROUPBOX,     IDH_MMSE_GROUPBOX,
    IDC_RADIO_SINGLE, IDH_MIDI_SINGLE_INST_BUTTON,
    IDC_INSTRUMENTS,  IDH_MIDI_SINGLE_INST,
    IDL_INSTRUMENTS,  IDH_MIDI_SINGLE_INST_LIST,
    IDC_RADIO_CUSTOM, IDH_MIDI_CUST_CONFIG,
    IDC_SCHEMESLABEL, IDH_MIDI_SCHEME,
    IDC_SCHEMES,      IDH_MIDI_SCHEME,
	IDC_ABOUTSYNTH,	  IDH_ABOUT,
    //IDB_DETAILS,      IDH_MIDI_SINGLE_INST_PROP,
    IDB_CONFIGURE,    IDH_MIDI_CONFIG_SCHEME,
    IDB_ADDWIZ,       IDH_MIDI_ADD_NEW,

    0, 0
};

INT_PTR CALLBACK MidiCplDlgProc (
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
#ifdef DEBUG
    AuxDebugEx (5, DEBUGLINE TEXT ("MidiCplDlgProc(%08X,%X,%08X,%08X)\r\n"),
		hWnd, uMsg, wParam, lParam);
#endif

    switch (uMsg)
    {
	case WM_COMMAND:
	    HANDLE_WM_COMMAND (hWnd, wParam, lParam, MidiCplCommands);
	    break;
	
	case WM_NOTIFY:
	{
	    LPNMHDR lpnm = (LPVOID)lParam;
	    if (lpnm->idFrom == (UINT)IDL_INSTRUMENTS &&
		lpnm->code == TVN_SELCHANGED)
	    {
		if (HandleInstrumentsSelChange (hWnd, lpnm))
		    PropSheet_Changed(GetParent(hWnd), hWnd);
	    }
	    else
		ForwardBillNotify(hWnd, (NMHDR FAR *)lParam);
	}
	    break;
	
	case WM_INITDIALOG:
	{
	    PMCLOCAL pmcl;
	    
	    pmcl = (LPVOID)LocalAlloc(LPTR, sizeof(*pmcl));
	    SetDlgData (hWnd, pmcl);
	    if (!pmcl)
	    {
		break;
	    }
	    pmcl->bPastInit = FALSE;

	    InitLocal (pmcl, lParam, FALSE);
	    EnableWindow (GetDlgItem (hWnd, IDB_ADDWIZ), pmcl->mcm.bHasExternal & AccessServiceController());
		EnableWindow(GetDlgItem(hWnd, IDC_ABOUTSYNTH), FALSE);

	    if (GetDlgItem(hWnd, IDL_INSTRUMENTS))
	    {
		pmcl->bDlgType2 = TRUE;
		LoadInstrumentsIntoTree (hWnd, IDL_INSTRUMENTS,
					 IDC_INSTRUMENTS, pmcl->piSingle,
					 pmcl);
	    }
	    else
	    {
		pmcl->bDlgType2 = FALSE;
		LoadInstrumentsIntoCombo (hWnd, IDC_INSTRUMENTS,
					  pmcl->piSingle, &pmcl->mcm);
	    }

	    CheckRadioButton (hWnd,
			      IDC_RADIO_SINGLE,
			      IDC_RADIO_CUSTOM,
			      pmcl->bUseScheme ? IDC_RADIO_CUSTOM
					       : IDC_RADIO_SINGLE);

	    LoadSchemesIntoCombo (hWnd, IDC_SCHEMES,
				  pmcl->szScheme, &pmcl->ms);

	    if (pmcl->mcm.nInstr > 1)
	    {
	       if (pmcl->bDlgType2)
	       {
		   HWND hWndCtl;

		   EnableWindow(GetDlgItem (hWnd, IDL_INSTRUMENTS), !pmcl->bUseScheme);
		   if (hWndCtl = GetDlgItem (hWnd, IDB_DETAILS))
		      EnableWindow (hWndCtl, !pmcl->bUseScheme);
	       }
	       EnableWindow(GetDlgItem (hWnd, IDC_INSTRUMENTS), !pmcl->bUseScheme);
	       EnableWindow(GetDlgItem (hWnd, IDC_SCHEMES), pmcl->bUseScheme);
	       EnableWindow(GetDlgItem (hWnd, IDC_SCHEMESLABEL), pmcl->bUseScheme);
	       EnableWindow(GetDlgItem (hWnd, IDB_CONFIGURE), pmcl->bUseScheme);
	    }
	    else
	    {
		UINT aid[] = { IDL_INSTRUMENTS, IDC_INSTRUMENTS, IDC_SCHEMES,
			       IDC_RADIO_SINGLE, IDC_RADIO_CUSTOM,
			       IDB_CONFIGURE, IDB_DETAILS, IDB_ADDWIZ };
		UINT ii;

		for (ii = 0; ii < NUMELMS(aid); ++ii)
		{
		    HWND hWndCtl = GetDlgItem (hWnd, aid[ii]);
		    if (hWndCtl)
			EnableWindow (hWndCtl, FALSE);
		}
	    }
	    pmcl->bPastInit = TRUE;
	    break;
	}

	case WM_DESTROY:
	{
	    PMCLOCAL pmcl = GetDlgData(hWnd);

#ifdef DEBUG
	    AuxDebugEx (5, DEBUGLINE TEXT ("MidiCPL - begin WM_DESTROY\r\n"));
#endif
	    if (pmcl)
	    {
		if (pmcl->mcm.hkMidi)
		    RegCloseKey (pmcl->mcm.hkMidi);
		if (pmcl->ms.hkSchemes)
		    RegCloseKey (pmcl->ms.hkSchemes);

		FreeInstruments (&pmcl->mcm);
		SetDlgData (hWnd, 0);
		LocalFree ((HLOCAL)(UINT_PTR)(DWORD_PTR)pmcl);
	    }
#ifdef DEBUG
	    AuxDebugEx (5, DEBUGLINE TEXT ("MidiCPL -  done with WM_DESTROY\r\n"));
#endif
	    break;
	}

	//case WM_DROPFILES:
	//    break;

	case WM_CONTEXTMENU:
	    WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU,
		    (UINT_PTR) (LPTSTR) aKeyWordIds);
	    return TRUE;

	case WM_HELP:
	{
	    LPHELPINFO lphi = (LPVOID) lParam;
	    WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP,
		    (UINT_PTR) (LPTSTR) aKeyWordIds);
	    return TRUE;
	}

       #if 0
	default:
	    if (uMsg == wHelpMessage)
	    {
		WinHelp (hWnd, gszWindowsHlp, HELP_CONTEXT, ID_SND_HELP);
		return TRUE;
	    }
	    break;
       #endif
    }

    return FALSE;
}


/*+ MidiClassCommands
 *
 *-=================================================================*/

BOOL WINAPI MidiClassCommands (
    HWND hWnd,
    UINT wId,
    HWND hWndCtl,
    UINT wNotify)
{
    PMCLOCAL pmcl = GetDlgData(hWnd);

#ifdef DEBUG
    AuxDebugEx (5, DEBUGLINE TEXT ("MidiClassCommands(%08X,%d,%08X,%d)\r\n"),
		hWnd, wId, hWndCtl, wNotify);
#endif

    switch (wId)
    {
	case IDB_ADDWIZ:
	    MidiInstrumentsWizard (hWnd, &pmcl->mcm, NULL);
	    LoadInstruments (&pmcl->mcm, FALSE);
	    LoadInstrumentsIntoTree (hWnd, IDL_INSTRUMENTS, 0, NULL, pmcl);

	    // flog the parent property sheet to let it know that we have
	    // made changes to the advanced midi page structures
	    //
	    {
		PMPSARGS  pmpsa = (LPVOID)pmcl->ppsp->lParam;
		if (pmpsa && pmpsa->lpfnMMExtPSCallback)
		    pmpsa->lpfnMMExtPSCallback (MM_EPS_TREECHANGE, 0, 0, pmpsa->lParam);
	    }
	    break;

	//case ID_APPLY:
	//    return TRUE;
	//
	//case IDCANCEL:
	//    break;
    }

    return FALSE;
}


/*+ MidiClassDlgProc
 *
 *-=================================================================*/

const static DWORD aMidiClassHelpIds[] = {  // Context Help IDs
    IDB_ADDWIZ,      IDH_MIDI_ADD_NEW,
    IDC_CLASS_ICON,  NO_HELP,
    IDC_CLASS_LABEL, NO_HELP,
    IDL_INSTRUMENTS, IDH_MMCPL_DEVPROP_INST_LIST,

    0, 0
};

INT_PTR CALLBACK MidiClassDlgProc (
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
	case WM_COMMAND:
	    HANDLE_WM_COMMAND (hWnd, wParam, lParam, MidiClassCommands);
	    break;
	
	case WM_NOTIFY:
	    ForwardBillNotify (hWnd, (NMHDR FAR *)lParam);
	    break;
	
	case WM_INITDIALOG:
	{
	    PMCLOCAL pmcl;
	    TCHAR    sz[MAX_ALIAS];
	    
	    pmcl = (LPVOID)LocalAlloc(LPTR, sizeof(*pmcl));
	    SetDlgData (hWnd, pmcl);
	    if (!pmcl)
	    {
		break;
	    }


	    InitLocal (pmcl, lParam, FALSE);

#ifdef DEBUG
	    AuxDebugEx (5, DEBUGLINE TEXT ("midiClass.WM_INITDLG ppsp=%08X\r\n"), pmcl->ppsp);
#endif
	    //AuxDebugDump (8, pmcl->ppsp, sizeof(*pmcl->ppsp));

	    LoadString (ghInstance, IDS_MIDI_DEV_AND_INST, sz, NUMELMS(sz));
	    SetDlgItemText (hWnd, IDC_CLASS_LABEL, sz);
	    Static_SetIcon(GetDlgItem (hWnd, IDC_CLASS_ICON),
			   LoadIcon (ghInstance, MAKEINTRESOURCE(IDI_INSTRUMENT)));

	    LoadInstrumentsIntoTree (hWnd, IDL_INSTRUMENTS, 0, NULL, pmcl);
	    EnableWindow (GetDlgItem (hWnd, IDB_ADDWIZ), pmcl->mcm.bHasExternal & AccessServiceController());
	    break;
	}

	case WM_DESTROY:
	{
	    PMCLOCAL pmcl = GetDlgData(hWnd);

	    if (pmcl)
	    {
		if (pmcl->mcm.hkMidi)
		    RegCloseKey (pmcl->mcm.hkMidi);
		if (pmcl->ms.hkSchemes)
		    RegCloseKey (pmcl->ms.hkSchemes);

		FreeInstruments (&pmcl->mcm);
		LocalFree ((HLOCAL)(UINT_PTR)(DWORD_PTR)pmcl);
	    }
	    break;
	}
    
	case WM_CONTEXTMENU:
	    WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU,
		    (UINT_PTR) (LPTSTR) aMidiClassHelpIds);
	    return TRUE;

	case WM_HELP:
	{
	    LPHELPINFO lphi = (LPVOID) lParam;
	    WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP,
		    (UINT_PTR) (LPTSTR) aMidiClassHelpIds);
	    return TRUE;
	}
    }

    return FALSE;
}


/*+ PropPageCallback
 *
 *  add a property page
 *
 *-=================================================================*/

UINT CALLBACK PropPageCallback (
    HWND            hwnd,
    UINT            uMsg,
    LPPROPSHEETPAGE ppsp)
{
    if (uMsg == PSPCB_RELEASE) {
	//LocalFree ((HLOCAL)(UINT)(DWORD)ppsp->pszTitle);
	LocalFree ((HLOCAL)ppsp->lParam);
    }
    return 1;
}


/*+ AddPropPage
 *
 *  add a property page
 *
 *-=================================================================*/

STATICFN HPROPSHEETPAGE WINAPI AddPropPage (
    LPCTSTR                     pszTitle,
    LPFNMMEXTPROPSHEETCALLBACK  lpfnAddPropSheetPage,
    DLGPROC                     fnDlgProc,
    UINT                        idTemplate,
    LPARAM                      lParam)
{
    PROPSHEETPAGE   psp;
    PMPSARGS        pmpsa;
    UINT            cbSize;

    cbSize = sizeof(MPSARGS) + lstrlen (pszTitle) * sizeof(TCHAR);
    if (pmpsa = (PVOID) LocalAlloc (LPTR, cbSize))
    {
	HPROPSHEETPAGE  hpsp;

	lstrcpy (pmpsa->szTitle, pszTitle);
	pmpsa->lpfnMMExtPSCallback = lpfnAddPropSheetPage;
	pmpsa->lParam = lParam;

	psp.dwSize      = sizeof(psp);
	psp.dwFlags     = PSP_USETITLE | PSP_USECALLBACK;
	psp.hInstance   = ghInstance;
	psp.pszTemplate = MAKEINTRESOURCE(idTemplate);
	psp.pszIcon     = NULL;
	psp.pszTitle    = pmpsa->szTitle;
	psp.pfnDlgProc  = fnDlgProc;
	psp.lParam      = (LPARAM)pmpsa;
	psp.pfnCallback = PropPageCallback;
	psp.pcRefParent = NULL;

	if (hpsp = CreatePropertySheetPage (&psp))
	{
	    if ( ! lpfnAddPropSheetPage ||
		lpfnAddPropSheetPage (MM_EPS_ADDSHEET, (DWORD_PTR)hpsp, 0, lParam))
	    {

		return hpsp;
	    }
	    DestroyPropertySheetPage (hpsp);
	    LocalFree ((HLOCAL) pmpsa);
	}
    }
    return NULL;
}


/*+ AddInstrumentPages
 *
 *  add a midi page to a property sheet.  Invoked from Advanced tab
 *  of Muitimedia control panel when class midi is selected from
 *  the list.
 *
 *-=================================================================*/

BOOL CALLBACK  AddInstrumentPages (
    LPCTSTR                     pszTitle,
    LPFNMMEXTPROPSHEETCALLBACK  lpfnAddPropSheetPage,
    LPARAM                      lParam)
{
    HPROPSHEETPAGE hpsp;
    TCHAR          sz[MAX_ALIAS];

#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("AddInstrumentPages(%08X,%08X,%08X)\r\n"),
		pszTitle, lpfnAddPropSheetPage, lParam);
#endif

    LoadString (ghInstance, IDS_GENERAL, sz, NUMELMS(sz));
    hpsp = AddPropPage (sz,
			lpfnAddPropSheetPage,
			MidiInstrumentDlgProc,
			IDD_INSTRUMENT_GEN,
			lParam);
    if ( ! hpsp)
	return FALSE;

    LoadString (ghInstance, IDS_MIDIDETAILS, sz, NUMELMS(sz));
    hpsp = AddPropPage (sz,
			lpfnAddPropSheetPage,
			MidiInstrumentDlgProc,
			IDD_INSTRUMENT_DETAIL,
			lParam);

    return (hpsp != NULL);
}


/*+ AddDevicePages
 *
 *  add a midi page to a property sheet.  Invoked from Advanced tab
 *  of Multimedia control panel when class midi is selected from
 *  the list.
 *
 *-=================================================================*/

BOOL CALLBACK  AddDevicePages (
    LPCTSTR                     pszTitle,
    LPFNMMEXTPROPSHEETCALLBACK  lpfnAddPropSheetPage,
    LPARAM                      lParam)
{
    HPROPSHEETPAGE hpsp;
    TCHAR          sz[MAX_ALIAS];

#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("AddInstrumentPages(%08X,%08X,%08X)\r\n"),
		pszTitle, lpfnAddPropSheetPage, lParam);
#endif

    LoadString (ghInstance, IDS_MIDIDETAILS, sz, NUMELMS(sz));
    hpsp = AddPropPage (sz,
			lpfnAddPropSheetPage,
			MidiInstrumentDlgProc,
			IDD_DEVICE_DETAIL,
			lParam);

    return (hpsp != NULL);
}


/*+ ShowDetails
 *
 *  Show Instrument or device details sheet and allow edits
 *  return TRUE if changes were made
 *
 *-=================================================================*/

struct _show_details_args {
    PMCLOCAL        pmcl;
    BOOL            bChanged;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE  hpsp[2];
    };

BOOL CALLBACK fnPropCallback (
    DWORD dwFunc,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2,
    DWORD_PTR dwInstance)
{
    struct _show_details_args * psda = (LPVOID)dwInstance;

    assert (psda);
    if (!psda)
	return FALSE;

    switch (dwFunc)
    {
	case MM_EPS_GETNODEDESC:
	    *(LPTSTR)dwParam1 = 0;
	    if (psda->pmcl->piSingle)
	       lstrcpyn ((LPTSTR)dwParam1, psda->pmcl->piSingle->szFriendly, (int)(dwParam2/sizeof(TCHAR)));
	    break;

	case MM_EPS_GETNODEID:
	    *(LPTSTR)dwParam1 = 0;
	    if (psda->pmcl->piSingle)
	    {
		lstrcpy ((LPTSTR)dwParam1, cszMidiSlash);
		lstrcat ((LPTSTR)dwParam1, psda->pmcl->piSingle->szKey);
	    }
	    break;

	case MM_EPS_ADDSHEET:
	    if (psda->psh.nPages >= NUMELMS(psda->hpsp)-1)
		return FALSE;
	    psda->psh.phpage[psda->psh.nPages++] = (HPROPSHEETPAGE)dwParam1;
	    break;

	case MM_EPS_TREECHANGE:
	    psda->bChanged = TRUE;
	    break;

	default:
	    return FALSE;
    }

    return TRUE;
}

BOOL WINAPI ShowDetails (
    HWND     hWnd,
    PMCLOCAL pmcl)
{
    struct _show_details_args sda;
    TCHAR           szTitle[MAX_ALIAS];
    HPROPSHEETPAGE  hpsp;
    UINT            idDlg;

    idDlg = IDD_DEVICE_DETAIL;
    if (pmcl->piSingle && pmcl->piSingle->piParent)
	idDlg = IDD_INSTRUMENT_DETAIL;

    ZeroMemory (&sda, sizeof(sda));
    sda.pmcl            = pmcl;
    sda.psh.dwSize      = sizeof(sda.psh);
    sda.psh.dwFlags     = PSH_PROPTITLE;
    sda.psh.hwndParent  = hWnd;
    sda.psh.hInstance   = ghInstance;
    sda.psh.pszCaption  = MAKEINTRESOURCE (IDS_MMPROP);
    sda.psh.nPages      = 0;
    sda.psh.nStartPage  = 0;
    sda.psh.phpage      = sda.hpsp;

    LoadString (ghInstance, IDS_MIDIDETAILS, szTitle, NUMELMS(szTitle));
    hpsp = AddPropPage (szTitle,
			fnPropCallback,
			MidiInstrumentDlgProc,
			idDlg,
			(LPARAM)&sda);
    if (hpsp)
	sda.psh.nPages = 1;

    PropertySheet (&sda.psh);
    return sda.bChanged;
}

/*+ AddMidiPages
 *
 *  add a midi page to a property sheet.  Invoked from Advanced tab
 *  of Muitimedia control panel when class midi is selected from
 *  the list.
 *
 *-=================================================================*/

BOOL CALLBACK  AddMidiPages (
    LPCTSTR                     pszTitle,
    LPFNMMEXTPROPSHEETCALLBACK  lpfnAddPropSheetPage,
    LPARAM                      lParam)
{
    HPROPSHEETPAGE hpsp;
    TCHAR          sz[MAX_ALIAS];

    LoadString (ghInstance, IDS_GENERAL, sz, NUMELMS(sz));
    hpsp = AddPropPage (sz,
			lpfnAddPropSheetPage,
			MidiClassDlgProc,
			IDD_MIDICLASS_GEN,
			lParam);
    return (hpsp != NULL);
}


/*+ AddSimpleMidiPages
 *
 *  add a midi page to a MM control panel.
 *
 *-=================================================================*/

BOOL CALLBACK  AddSimpleMidiPages (
    LPTSTR                      pszTitle,
    LPFNMMEXTPROPSHEETCALLBACK  lpfnAddPropSheetPage,
    LPARAM                      lParam)
{
    HPROPSHEETPAGE hpsp;
    //static CONST TCHAR sz[13] = TEXT ("            ");
    //UINT cch = lstrlen (pszTitle);

    DebugSetOutputLevel (GetProfileInt(TEXT ("Debug"), TEXT ("midiprop"), 0));

    // pad my tab to 12 spaces so it looks nice with the
    // other simple tabls (as per request of vijr)
    //
    //if (cch < NUMELMS(sz)-2)
    //{
    //    lstrcpy (sz + NUMELMS(sz)/2 - cch/2, pszTitle);
    //    pszTitle = sz;
    //    pszTitle[lstrlen(pszTitle)] = TEXT (' ');
    //}

    hpsp = AddPropPage (pszTitle,
			lpfnAddPropSheetPage,
			MidiCplDlgProc,
			IDD_CPL_MIDI2,
			lParam);
    return (hpsp != NULL);
}


/*
 ***************************************************************
 *  BOOL PASCAL LoadDesc(LPCTSTR pszFile, LPCTSTR pszDesc)
 *      This function gets the description string from the executable
 *      file specified. We first try to get the string from the version info
 *      If that fails then we try to get the string from the exehdr.
 *      If that fails we return a NULL string. 
 *      Return TRUE on success, else FALSE.
 ***************************************************************
 */

BOOL PASCAL LoadDesc(LPCTSTR pszFile, LPTSTR pszDesc)
{
   LPTSTR           psz;
   static TCHAR     szProfile[MAXSTR];
   UINT             cchSize;
   HANDLE           hFind;
   WIN32_FIND_DATA  wfd;
  
   DPF (TEXT ("LoadDesc: %s\r\n"), pszFile);

   // Make sure file exists
   hFind = FindFirstFile (pszFile, &wfd);
   if (hFind == INVALID_HANDLE_VALUE)
	   return(FALSE);
   FindClose (hFind);

   // Get User Friendly name from Version Info
   if (GetVerDesc (wfd.cFileName, pszDesc))
	   return TRUE;

   //
   // As a last resort, look at the description in the Executable Header
   //

   cchSize = sizeof(szProfile)/sizeof(TCHAR);
   if ((! GetExeDesc (wfd.cFileName, szProfile, cchSize)) ||
	    (lstrlen (szProfile) < 3))
      {
	   *pszDesc = 0;
	   return(FALSE);    
      }
   else    
      {    
	   // There is EXEHDR information Parse according to driver spec
	   psz = szProfile;
	   while (*psz && *psz++ != TEXT (':'))
	      {
	      ; // skip type information
	      }
	   if (!(*psz))
	      psz = szProfile;
	   lstrcpy (pszDesc, psz);
	   return(TRUE);
      }
}


/* BOOL FAR PASCAL GetExeDesc(szFile, szBuff, cchBuff)
 *
 *  Function will return the an executable's description
 *
 *      szFile      - Path Name a new exe
 *      pszBuf      - Buffer to place returned info
 *      cchBuf      - Size of buffer (in characters
 *
 *  returns:  TRUE if successful, FALSE otherwise.
 */

STATIC BOOL FAR PASCAL GetExeDesc(
    LPTSTR  szFile, 
    LPTSTR  pszBuff, 
    int     cchBuff)
{
   DWORD             dwSig;
   WORD              wSig;
   HANDLE            hFile;
   DWORD             offset;
   BYTE              cbLen;
   DWORD             cbRead;
   IMAGE_DOS_HEADER  doshdr;    // Original EXE Header

      // Open File
   hFile = CreateFile (szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
		       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile == INVALID_HANDLE_VALUE)
      return FALSE;

   // Get Original Dos Header
   if ((! ReadFile (hFile, (LPVOID)&doshdr, sizeof(doshdr), &cbRead, NULL)) ||
       (cbRead != sizeof(doshdr)) ||             // Read Error
       (doshdr.e_magic != IMAGE_DOS_SIGNATURE))  // Invalid DOS Header
      {
      goto error;        /* Abort("Not an exe",h); */
      }

   // Seek to new header
   offset = doshdr.e_lfanew;
   SetFilePointer (hFile, offset, NULL, FILE_BEGIN);
   
   // Read in signature
   if ((! ReadFile (hFile, (LPVOID)&dwSig, sizeof(dwSig), &cbRead, NULL)) ||
       (cbRead != sizeof(dwSig)))            // Read Error
      {
      goto error;        /* Abort("Not an exe",h); */
      }
   wSig = LOWORD (dwSig);

   if (dwSig == IMAGE_NT_SIGNATURE)
      {   
      DPF (TEXT ("GetExeDesc: NT Portable Executable Format\r\n"));

      // NOTE - The NT Portatble Executable Format does not store
      //        the executable's user friendly name.
      goto error;
      }
   else if (wSig == IMAGE_OS2_SIGNATURE) 
      {
      IMAGE_OS2_HEADER  winhdr;    // New Windows/OS2 header
      TCHAR              szInfo[256];

      DPF (TEXT ("GetExeDesc: Windows or OS2 Executable Format\r\n"));

      // Seek to Windows Header
      offset = doshdr.e_lfanew;
      SetFilePointer (hFile, offset, NULL, FILE_BEGIN);

      // Read Windows Header
      if ((! ReadFile (hFile, (LPVOID)&winhdr, sizeof(winhdr), 
		       &cbRead, NULL)) || 
	  (cbRead != sizeof(winhdr)) || // Read Error
	  (winhdr.ne_magic != IMAGE_OS2_SIGNATURE)) // Invalid Windows Header
	 {
	 goto error;
	 }

      // Seek to module name which is the first entry in the non-resident name table
      offset = winhdr.ne_nrestab;
      SetFilePointer (hFile, offset, NULL, FILE_BEGIN);

      // Get Size of Module Name
      if ((! ReadFile (hFile, (LPVOID)&cbLen, sizeof(BYTE),
		       &cbRead, NULL)) || 
	  (cbRead != sizeof(BYTE)))
	 {
	 goto error;
	 }

      cchBuff--;         // leave room for a \0

      if (cbLen > (BYTE)cchBuff)
	 cbLen = (BYTE)cchBuff;

      // Read Module Name
      if ((! ReadFile (hFile, (LPVOID)szInfo, cbLen,
		       &cbRead, NULL)) || 
	  (cbRead != cbLen))
	 {
	 goto error;
	 }
      szInfo[cbLen] = 0;

      // Copy to Buffer
      lstrcpy (pszBuff, szInfo);
      }
   else if (wSig == IMAGE_VXD_SIGNATURE)
      {
      IMAGE_VXD_HEADER  vxdhdr;    // New Windows/OS2 VXD  Header
      TCHAR              szInfo[256];

      DPF (TEXT ("GetExeDesc: Windows or OS2 VXD Executable Format\r\n"));

      // Seek to VXD Header
      offset = doshdr.e_lfanew;
      SetFilePointer (hFile, offset, NULL, FILE_BEGIN);

      // Read VXD Header
      if ((! ReadFile (hFile, (LPVOID)&vxdhdr, sizeof(vxdhdr), 
		       &cbRead, NULL)) || 
	  (cbRead != sizeof(vxdhdr)) || // Read Error
	  (vxdhdr.e32_magic != IMAGE_VXD_SIGNATURE)) // Invalid VXD Header
	 {
	 goto error;
	 }

      // Seek to module name which is the first entry in the non-resident name table
      offset = vxdhdr.e32_nrestab;
      SetFilePointer (hFile, offset, NULL, FILE_BEGIN);

      // Get Size of Module Name
      if ((! ReadFile (hFile, (LPVOID)&cbLen, sizeof(BYTE),
		       &cbRead, NULL)) || 
	  (cbRead != sizeof(BYTE)))
	 {
	 goto error;
	 }

      cchBuff--;         // leave room for a \0

      if (cbLen > (BYTE)cchBuff)
	 cbLen = (BYTE)cchBuff;

      // Read Module Name
      if ((! ReadFile (hFile, (LPVOID)szInfo, cbLen,
		       &cbRead, NULL)) || 
	  (cbRead != cbLen))
	 {
	 goto error;
	 }
      szInfo[cbLen] = 0;

      // Copy to Buffer
      lstrcpy (pszBuff, szInfo);
      }
   else
      {
      DPF (TEXT ("GetExeDesc: Unknown Executable\r\n"));
      goto error;        /* Abort("Not an exe",h); */
      }

   CloseHandle (hFile);
   return TRUE;

error:
   CloseHandle (hFile);
   return FALSE;
}


/*
 ***************************************************************
 * STATIC INT_PTR GetVerDesc
 *      Loads the version DLL and uses it to get Version Description string 
 *      from the specified file.
 ***************************************************************
 */

STATIC INT_PTR PASCAL GetVerDesc (LPCTSTR pstrFile, LPTSTR pstrDesc)
{
    DWORD_PTR dwVerInfoSize;
    DWORD dwVerHnd;
    INT_PTR  bRetCode;

    DPF( TEXT ("Getting VERSION string for %s \r\n"), pstrFile);

    dwVerInfoSize = GetFileVersionInfoSize (pstrFile, &dwVerHnd);

    if (dwVerInfoSize) 
    {
	LPBYTE   lpVffInfo;             // Pointer to block to hold info

	// Get a block big enough to hold version info
	if (lpVffInfo  = (LPBYTE) GlobalAllocPtr(GMEM_MOVEABLE, dwVerInfoSize)) 
	{


	   // Get the File Version first
	    if (GetFileVersionInfo (pstrFile, 0L, 
				    dwVerInfoSize, lpVffInfo)) 
	    {
		static SZCODE cszFileDescr[] = TEXT ("\\StringFileInfo\\040904E4\\FileDescription");
		TCHAR   szBuf[MAX_PATH];
		LPTSTR  lpVersion;       
		WORD    wVersionLen;

		   // Now try to get the FileDescription
		   // First try this for the "Translation" entry, and then
		   // try the American english translation.  
		   // Keep track of the string length for easy updating.  
		   // 040904E4 represents the language ID and the four 
		   // least significant digits represent the codepage for 
		   // which the data is formatted.  The language ID is 
		   // composed of two parts: the low ten bits represent 
		   // the major language and the high six bits represent 
		   // the sub language.

		lstrcpy(szBuf, cszFileDescr);
     
		wVersionLen   = 0;
		lpVersion     = NULL;

		// Look for the corresponding string. 
		bRetCode = VerQueryValue((LPVOID)lpVffInfo,    
					 (LPTSTR)szBuf,
					 (void FAR* FAR*)&lpVersion,
					 (UINT FAR *) &wVersionLen);

		if (bRetCode && wVersionLen > 2 && lpVersion)
		{
		    lstrcpy (pstrDesc, lpVersion);
		}
		else
		    bRetCode = FALSE;


		// Let go of the memory
		GlobalFreePtr(lpVffInfo);
	    }
	}
    } else
	bRetCode = FALSE;
    return bRetCode;
}




#if 0
/*+ RunOnceWizard
 *
 *-=================================================================*/

DWORD WINAPI RunOnceWizard (
    HWND      hWnd,
    HINSTANCE hInst,
    LPTSTR    pszCmd,
    int       nShow)
{
    static CONST TCHAR cszScheme[] = TEXT ("scheme");

    DebugSetOutputLevel (GetProfileInt(TEXT ("Debug"), TEXT ("midiprop"), 0));
#ifdef DEBUG
    AuxDebugEx (0, DEBUGLINE TEXT ("RunOnceWizard(%s)\r\n"), pszCmd);
#endif

    if (IsSzEqual(pszCmd, cszScheme))
	return RunOnceSchemeInit (hWnd, hInst, pszCmd, nShow);

    MidiInstrumentsWizard (hWnd, NULL, pszCmd);
    return 0;
}
#endif

LONG SHRegDeleteKey(HKEY hKey, LPCTSTR lpSubKey)
{
    LONG    lResult;
    HKEY    hkSubKey;
    DWORD   dwIndex;
    TCHAR   szSubKeyName[MAX_PATH + 1];
    DWORD   cchSubKeyName = ARRAYSIZE(szSubKeyName);
    TCHAR   szClass[MAX_PATH];
    DWORD   cchClass = ARRAYSIZE(szClass);
    DWORD   dwDummy1, dwDummy2, dwDummy3, dwDummy4, dwDummy5, dwDummy6;
    FILETIME ft;

    // Open the subkey so we can enumerate any children
    lResult = RegOpenKeyEx(hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hkSubKey);
    if (ERROR_SUCCESS == lResult)
    {
	// I can't just call RegEnumKey with an ever-increasing index, because
	// I'm deleting the subkeys as I go, which alters the indices of the
	// remaining subkeys in an implementation-dependent way.  In order to
	// be safe, I have to count backwards while deleting the subkeys.

	// Find out how many subkeys there are
	lResult = RegQueryInfoKey(hkSubKey, 
				  szClass, 
				  &cchClass, 
				  NULL, 
				  &dwIndex, // The # of subkeys -- all we need
				  &dwDummy1,
				  &dwDummy2,
				  &dwDummy3,
				  &dwDummy4,
				  &dwDummy5,
				  &dwDummy6,
				  &ft);

	if (ERROR_SUCCESS == lResult)
	{
	    // dwIndex is now the count of subkeys, but it needs to be 
	    // zero-based for RegEnumKey, so I'll pre-decrement, rather
	    // than post-decrement.
	    while (ERROR_SUCCESS == RegEnumKey(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
	    {
		SHRegDeleteKey(hkSubKey, szSubKeyName);
	    }
	}

	RegCloseKey(hkSubKey);

	lResult = RegDeleteKey(hKey, lpSubKey);
    }
    
    return lResult;
} // End SHRegDeleteKey


/* DeviceIDFromDriverName
 *
 * Query MMSYSTEM to find the given device. Return its base device ID.
 * Return -1 if we cannot find the driver
 */
static UINT
DeviceIDFromDriverName(
    PTSTR pstrDriverName)
{
    UINT idxDev;
    UINT cPorts;
    DWORD cPort;
    PTSTR pstrDriver;
    MMRESULT mmr;


    if (NULL == (pstrDriver = LocalAlloc(LPTR, MAX_ALIAS*sizeof(TCHAR))))
    {
        AuxDebugEx(3, DEBUGLINE TEXT("DN->ID: LocalAlloc() failed.\r\n"));
        return (UINT)-1;
    }

    // Walk through the base device ID of each driver. Use MMSYSTEM's
    // driver query messages to find out how many ports & the driver name
    //
    cPorts = midiOutGetNumDevs();
    for (idxDev = 0; idxDev < cPorts; idxDev++)
    {
        if (MMSYSERR_NOERROR != (mmr = midiOutMessage((HMIDIOUT)idxDev,
            DRV_QUERYNUMPORTS,
            (DWORD_PTR)(LPDWORD)&cPort,
            0)))
        {
            // Something is wrong with this driver. Skip it
            //
            AuxDebugEx(3, DEBUGLINE TEXT("DN->ID: DRV_QUERYNUMPORTS(%u)->%u\r\n"),
                       (UINT)idxDev,
                       (UINT)mmr);
            continue;
        }

        if (MMSYSERR_NOERROR != (mmr = midiOutMessage((HMIDIOUT)idxDev,
            DRV_QUERYDRVENTRY,
            (DWORD_PTR)(LPTSTR)pstrDriver,
            MAX_ALIAS)))
        {
            AuxDebugEx(3, DEBUGLINE TEXT("DN->ID: DRV_QUERYDRVENTRY(%u)->%u\r\n"),
                       (UINT)idxDev,
                       (UINT)mmr);
            continue;
        }

        if (!_tcscmp(pstrDriver, pstrDriverName))
            break;
    }

    if (idxDev >= cPorts)
    {
        AuxDebugEx(3, DEBUGLINE TEXT("DN->ID: No match for [%s]\r\n"),
                   (LPTSTR)pstrDriverName);
        idxDev = (UINT)-1;
    }
    else
        AuxDebugEx(3, DEBUGLINE TEXT("DN->ID: [%s] at %d\r\n"),
                   (LPTSTR)pstrDriverName,
                   (int)idxDev);

    LocalFree(pstrDriver);
    return (int)idxDev;
}
