/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Wine MIDI mapper driver
 *
 * Copyright 	1999, 2000, 2001 Eric Pouech
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * TODO:
 *	notification has to be implemented
 *	IDF file loading
 */

#include <stdarg.h>
//#include <string.h>
//#include <stdlib.h>
//#include <ctype.h>
#include <windef.h>
//#include "winbase.h"
//#include "wingdi.h"
#include <winuser.h>
#include <mmddk.h>
#include <winreg.h>
#include <wine/unicode.h>
#include <wine/debug.h>

/*
 * Here's how Windows stores the midiOut mapping information.
 *
 * Full form (in HKU) is:
 *
 * [Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia\\MIDIMap] 988836060
 * "AutoScheme"=dword:00000000
 * "ConfigureCount"=dword:00000004
 * "CurrentInstrument"="Wine OSS midi"
 * "CurrentScheme"="epp"
 * "DriverList"=""
 * "UseScheme"=dword:00000000
 *
 * AutoScheme: 		?
 * CurrentInstrument: 	name of midiOut device to use when UseScheme is 0. Wine uses an extension
 *			of the form #n to link to n'th midiOut device of the system
 * CurrentScheme:	when UseScheme is non null, it's the scheme to use (see below)
 * DriverList:		?
 * UseScheme:		trigger for simple/complex mapping
 *
 * A scheme is defined (in HKLM) as:
 *
 * [System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Midi\\Schemes\\<nameScheme>]
 * <nameScheme>:	one key for each defined scheme (system wide)
 * under each one of these <nameScheme> keys, there's:
 * [...\\<nameScheme>\\<idxDevice>]
 * "Channels"="<bitMask>"
 * (the default value of this key also refers to the name of the device).
 *
 * this defines, for each midiOut device (identified by its index in <idxDevice>), which
 * channels have to be mapped onto it. The <bitMask> defines the channels (from 0 to 15)
 * will be mapped (mapping occurs for channel <ch> if bit <ch> is set in <bitMask>
 *
 * Further mapping information can also be defined in:
 * [System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Midi\\Ports\\<nameDevice>\\Instruments\\<idx>]
 * "Definition"="<.idf file>"
 * "FriendlyName"="#for .idx file#"
 * "Port"="<idxPort>"
 *
 * This last part isn't implemented (.idf file support).
 */

WINE_DEFAULT_DEBUG_CHANNEL(msacm);

typedef struct tagMIDIOUTPORT
{
    WCHAR		name[MAXPNAMELEN];
    int			loaded;
    HMIDIOUT		hMidi;
    unsigned short	uDevID;
    LPBYTE		lpbPatch;
    unsigned int	aChn[16];
} MIDIOUTPORT;

typedef	struct tagMIDIMAPDATA
{
    struct tagMIDIMAPDATA*	self;
    MIDIOUTPORT*	ChannelMap[16];
} MIDIMAPDATA;

static	MIDIOUTPORT*	midiOutPorts;
static  unsigned	numMidiOutPorts;

static	BOOL	MIDIMAP_IsBadData(MIDIMAPDATA* mm)
{
    if (!IsBadReadPtr(mm, sizeof(MIDIMAPDATA)) && mm->self == mm)
	return FALSE;
    TRACE("Bad midimap data (%p)\n", mm);
    return TRUE;
}

static BOOL	MIDIMAP_FindPort(const WCHAR* name, unsigned* dev)
{
    for (*dev = 0; *dev < numMidiOutPorts; (*dev)++)
    {
	TRACE("%s\n", wine_dbgstr_w(midiOutPorts[*dev].name));
	if (strcmpW(midiOutPorts[*dev].name, name) == 0)
	    return TRUE;
    }
    /* try the form #nnn */
    if (*name == '#' && isdigit(name[1]))
    {
        const WCHAR*  ptr = name + 1;
        *dev = 0;
        do
        {
            *dev = *dev * 10 + *ptr - '0';
        } while (isdigit(*++ptr));
	if (*dev < numMidiOutPorts)
	    return TRUE;
    }
    return FALSE;
}

static BOOL	MIDIMAP_LoadSettingsDefault(MIDIMAPDATA* mom, const WCHAR* port)
{
    unsigned i, dev = 0;

    if (port != NULL && !MIDIMAP_FindPort(port, &dev))
    {
	ERR("Registry glitch: couldn't find midi out (%s)\n", wine_dbgstr_w(port));
	dev = 0;
    }

    /* this is necessary when no midi out ports are present */
    if (dev >= numMidiOutPorts)
	return FALSE;
    /* sets default */
    for (i = 0; i < 16; i++) mom->ChannelMap[i] = &midiOutPorts[dev];

    return TRUE;
}

static BOOL	MIDIMAP_LoadSettingsScheme(MIDIMAPDATA* mom, const WCHAR* scheme)
{
    HKEY	hSchemesKey, hKey, hPortKey;
    unsigned	i, idx, dev;
    WCHAR       buffer[256], port[256];
    DWORD	type, size, mask;

    for (i = 0; i < 16; i++)	mom->ChannelMap[i] = NULL;

    if (RegOpenKeyA(HKEY_LOCAL_MACHINE,
		    "System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Midi\\Schemes",
		    &hSchemesKey))
    {
	return FALSE;
    }
    if (RegOpenKeyW(hSchemesKey, scheme, &hKey))
    {
	RegCloseKey(hSchemesKey);
	return FALSE;
    }

    for (idx = 0; !RegEnumKeyW(hKey, idx, buffer, sizeof(buffer)); idx++)
    {
	if (RegOpenKeyW(hKey, buffer, &hPortKey)) continue;

	size = sizeof(port);
	if (RegQueryValueExW(hPortKey, NULL, 0, &type, (void*)port, &size)) continue;

	if (!MIDIMAP_FindPort(port, &dev)) continue;

	size = sizeof(mask);
	if (RegQueryValueExA(hPortKey, "Channels", 0, &type, (void*)&mask, &size))
	    continue;

	for (i = 0; i < 16; i++)
	{
	    if (mask & (1 << i))
	    {
		if (mom->ChannelMap[i])
		    ERR("Quirks in registry, channel %u is mapped twice\n", i);
		mom->ChannelMap[i] = &midiOutPorts[dev];
	    }
	}
    }

    RegCloseKey(hSchemesKey);
    RegCloseKey(hKey);

    return TRUE;
}

static BOOL	MIDIMAP_LoadSettings(MIDIMAPDATA* mom)
{
    HKEY 	hKey;
    BOOL	ret;

    if (RegOpenKeyA(HKEY_CURRENT_USER,
		    "Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia\\MIDIMap", &hKey))
    {
	ret = MIDIMAP_LoadSettingsDefault(mom, NULL);
    }
    else
    {
	DWORD	type, size, out;
	WCHAR   buffer[256];

	ret = 2;
	size = sizeof(out);
	if (!RegQueryValueExA(hKey, "UseScheme", 0, &type, (void*)&out, &size) && out)
	{
            static const WCHAR cs[] = {'C','u','r','r','e','n','t','S','c','h','e','m','e',0};
	    size = sizeof(buffer);
	    if (!RegQueryValueExW(hKey, cs, 0, &type, (void*)buffer, &size))
	    {
		if (!(ret = MIDIMAP_LoadSettingsScheme(mom, buffer)))
		    ret = MIDIMAP_LoadSettingsDefault(mom, NULL);
	    }
	    else
	    {
		ERR("Wrong registry: UseScheme is active, but no CurrentScheme found\n");
	    }
	}
	if (ret == 2)
	{
            static const WCHAR ci[] = {'C','u','r','r','e','n','t','I','n','s','t','r','u','m','e','n','t',0};
	    size = sizeof(buffer);
	    if (!RegQueryValueExW(hKey, ci, 0, &type, (void*)buffer, &size) && *buffer)
	    {
		ret = MIDIMAP_LoadSettingsDefault(mom, buffer);
	    }
	    else if (!RegQueryValueExW(hKey, L"szPname", 0, &type, (void*)buffer, &size) && *buffer)
	    {
		/* Windows XP and higher setting */
		ret = MIDIMAP_LoadSettingsDefault(mom, buffer);
	    }
	    else
	    {
		ret = MIDIMAP_LoadSettingsDefault(mom, NULL);
	    }
	}
    }
    RegCloseKey(hKey);

    if (ret && TRACE_ON(msacm))
    {
	unsigned	i;

	for (i = 0; i < 16; i++)
	{
	    TRACE("chnMap[%2d] => %d\n",
		  i, mom->ChannelMap[i] ? mom->ChannelMap[i]->uDevID : -1);
	}
    }
    return ret;
}

static	DWORD	modOpen(DWORD_PTR *lpdwUser, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
    MIDIMAPDATA*	mom = HeapAlloc(GetProcessHeap(), 0, sizeof(MIDIMAPDATA));

    TRACE("(%p %p %08lx)\n", lpdwUser, lpDesc, dwFlags);

    if (!mom) return MMSYSERR_NOMEM;

    if (MIDIMAP_LoadSettings(mom))
    {
	*lpdwUser = (DWORD_PTR)mom;
	mom->self = mom;

	return MMSYSERR_NOERROR;
    }
    HeapFree(GetProcessHeap(), 0, mom);
    return MIDIERR_INVALIDSETUP;
}

static	DWORD	modClose(MIDIMAPDATA* mom)
{
    UINT	i;
    DWORD	ret = MMSYSERR_NOERROR;

    if (MIDIMAP_IsBadData(mom)) 	return MMSYSERR_ERROR;

    for (i = 0; i < 16; i++)
    {
	DWORD	t;
	if (mom->ChannelMap[i] && mom->ChannelMap[i]->loaded > 0)
	{
	    t = midiOutClose(mom->ChannelMap[i]->hMidi);
	    if (t == MMSYSERR_NOERROR)
	    {
		mom->ChannelMap[i]->loaded = 0;
		mom->ChannelMap[i]->hMidi = 0;
	    }
	    else if (ret == MMSYSERR_NOERROR)
		ret = t;
	}
    }
    if (ret == MMSYSERR_NOERROR)
	HeapFree(GetProcessHeap(), 0, mom);
    return ret;
}

static	DWORD	modLongData(MIDIMAPDATA* mom, LPMIDIHDR lpMidiHdr, DWORD_PTR dwParam2)
{
    WORD	chn;
    DWORD	ret = MMSYSERR_NOERROR;
    MIDIHDR	mh;

    if (MIDIMAP_IsBadData(mom))
	return MMSYSERR_ERROR;

    mh = *lpMidiHdr;
    for (chn = 0; chn < 16; chn++)
    {
	if (mom->ChannelMap[chn] && mom->ChannelMap[chn]->loaded > 0)
	{
	    mh.dwFlags = 0;
	    midiOutPrepareHeader(mom->ChannelMap[chn]->hMidi, &mh, sizeof(mh));
	    ret = midiOutLongMsg(mom->ChannelMap[chn]->hMidi, &mh, sizeof(mh));
	    midiOutUnprepareHeader(mom->ChannelMap[chn]->hMidi, &mh, sizeof(mh));
	    if (ret != MMSYSERR_NOERROR) break;
	}
    }
    return ret;
}

static	DWORD	modData(MIDIMAPDATA* mom, DWORD_PTR dwParam)
{
    BYTE	lb = LOBYTE(LOWORD(dwParam));
    WORD	chn = lb & 0x0F;
    DWORD	ret = MMSYSERR_NOERROR;

    if (MIDIMAP_IsBadData(mom))
	return MMSYSERR_ERROR;

    if (!mom->ChannelMap[chn]) return MMSYSERR_NOERROR;

    switch (lb & 0xF0)
    {
    case 0x80:
    case 0x90:
    case 0xA0:
    case 0xB0:
    case 0xC0:
    case 0xD0:
    case 0xE0:
	if (mom->ChannelMap[chn]->loaded == 0)
	{
	    if (midiOutOpen(&mom->ChannelMap[chn]->hMidi, mom->ChannelMap[chn]->uDevID,
			    0L, 0L, CALLBACK_NULL) == MMSYSERR_NOERROR)
		mom->ChannelMap[chn]->loaded = 1;
	    else
		mom->ChannelMap[chn]->loaded = -1;
	    /* FIXME: should load here the IDF midi data... and allow channel and
	     * patch mappings
	     */
	}
	if (mom->ChannelMap[chn]->loaded > 0)
	{
	    /* change channel */
	    dwParam &= ~0x0F;
	    dwParam |= mom->ChannelMap[chn]->aChn[chn];

	    if ((LOBYTE(LOWORD(dwParam)) & 0xF0) == 0xC0 /* program change */ &&
		mom->ChannelMap[chn]->lpbPatch)
	    {
		BYTE patch = HIBYTE(LOWORD(dwParam));

		/* change patch */
		dwParam &= ~0x0000FF00;
		dwParam |= mom->ChannelMap[chn]->lpbPatch[patch];
	    }
	    ret = midiOutShortMsg(mom->ChannelMap[chn]->hMidi, dwParam);
	}
	break;
    case 0xF0:
	for (chn = 0; chn < 16; chn++)
	{
	    if (mom->ChannelMap[chn]->loaded > 0)
		ret = midiOutShortMsg(mom->ChannelMap[chn]->hMidi, dwParam);
	}
	break;
    default:
	FIXME("ooch %lx\n", dwParam);
    }

    return ret;
}

static	DWORD	modPrepare(MIDIMAPDATA* mom, LPMIDIHDR lpMidiHdr, DWORD_PTR dwParam2)
{
    if (MIDIMAP_IsBadData(mom)) return MMSYSERR_ERROR;
    if (lpMidiHdr->dwFlags & (MHDR_ISSTRM|MHDR_PREPARED))
	return MMSYSERR_INVALPARAM;

    lpMidiHdr->dwFlags |= MHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

static	DWORD	modUnprepare(MIDIMAPDATA* mom, LPMIDIHDR lpMidiHdr, DWORD_PTR dwParam2)
{
    if (MIDIMAP_IsBadData(mom)) return MMSYSERR_ERROR;
    if ((lpMidiHdr->dwFlags & MHDR_ISSTRM) || !(lpMidiHdr->dwFlags & MHDR_PREPARED))
	return MMSYSERR_INVALPARAM;

    lpMidiHdr->dwFlags &= ~MHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

static	DWORD	modGetDevCaps(UINT wDevID, MIDIMAPDATA* mom, LPMIDIOUTCAPSW lpMidiCaps, DWORD_PTR size)
{
    static const WCHAR name[] = {'W','i','n','e',' ','m','i','d','i',' ','m','a','p','p','e','r',0};
    lpMidiCaps->wMid = 0x00FF;
    lpMidiCaps->wPid = 0x0001;
    lpMidiCaps->vDriverVersion = 0x0100;
    lstrcpyW(lpMidiCaps->szPname, name);
    lpMidiCaps->wTechnology = MOD_MAPPER;
    lpMidiCaps->wVoices = 0;
    lpMidiCaps->wNotes = 0;
    lpMidiCaps->wChannelMask = 0xFFFF;
    lpMidiCaps->dwSupport = 0L;

    return MMSYSERR_NOERROR;
}

static	DWORD	modReset(MIDIMAPDATA* mom)
{
    WORD	chn;
    DWORD	ret = MMSYSERR_NOERROR;

    if (MIDIMAP_IsBadData(mom))
	return MMSYSERR_ERROR;

    for (chn = 0; chn < 16; chn++)
    {
	if (mom->ChannelMap[chn] && mom->ChannelMap[chn]->loaded > 0)
	{
	    ret = midiOutReset(mom->ChannelMap[chn]->hMidi);
	    if (ret != MMSYSERR_NOERROR) break;
	}
    }
    return ret;
}

/**************************************************************************
 * 				modMessage (MIDIMAP.@)
 */
DWORD WINAPI MIDIMAP_modMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
				DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    TRACE("(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg)
    {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;

    case MODM_OPEN: return modOpen((DWORD_PTR *)dwUser, (LPMIDIOPENDESC)dwParam1, dwParam2);
    case MODM_CLOSE:	 	return modClose		((MIDIMAPDATA*)dwUser);

    case MODM_DATA:		return modData		((MIDIMAPDATA*)dwUser, dwParam1);
    case MODM_LONGDATA:		return modLongData      ((MIDIMAPDATA*)dwUser, (LPMIDIHDR)dwParam1,     dwParam2);
    case MODM_PREPARE:	 	return modPrepare	((MIDIMAPDATA*)dwUser, (LPMIDIHDR)dwParam1, 	dwParam2);
    case MODM_UNPREPARE: 	return modUnprepare	((MIDIMAPDATA*)dwUser, (LPMIDIHDR)dwParam1, 	dwParam2);
    case MODM_RESET:		return modReset		((MIDIMAPDATA*)dwUser);

    case MODM_GETDEVCAPS:	return modGetDevCaps	(wDevID, (MIDIMAPDATA*)dwUser, (LPMIDIOUTCAPSW)dwParam1,dwParam2);
    case MODM_GETNUMDEVS:	return 1;
    case MODM_GETVOLUME:	return MMSYSERR_NOTSUPPORTED;
    case MODM_SETVOLUME:	return MMSYSERR_NOTSUPPORTED;
    default:
	FIXME("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Driver part                                         *
 *======================================================================*/

/**************************************************************************
 * 				MIDIMAP_drvOpen			[internal]
 */
static	LRESULT	MIDIMAP_drvOpen(LPSTR str)
{
    MIDIOUTCAPSW	moc;
    unsigned		dev, i;

    if (midiOutPorts)
	return 0;

    numMidiOutPorts = midiOutGetNumDevs();
    midiOutPorts = HeapAlloc(GetProcessHeap(), 0,
			     numMidiOutPorts * sizeof(MIDIOUTPORT));
    for (dev = 0; dev < numMidiOutPorts; dev++)
    {
	if (midiOutGetDevCapsW(dev, &moc, sizeof(moc)) == 0L)
	{
	    strcpyW(midiOutPorts[dev].name, moc.szPname);
	    midiOutPorts[dev].loaded = 0;
	    midiOutPorts[dev].hMidi = 0;
	    midiOutPorts[dev].uDevID = dev;
	    midiOutPorts[dev].lpbPatch = NULL;
	    for (i = 0; i < 16; i++)
		midiOutPorts[dev].aChn[i] = i;
	}
	else
	{
	    midiOutPorts[dev].loaded = -1;
	}
    }

    return 1;
}

/**************************************************************************
 * 				MIDIMAP_drvClose		[internal]
 */
static	LRESULT	MIDIMAP_drvClose(DWORD_PTR dwDevID)
{
    if (midiOutPorts)
    {
	HeapFree(GetProcessHeap(), 0, midiOutPorts);
	midiOutPorts = NULL;
	return 1;
    }
    return 0;
}

/**************************************************************************
 * 				DriverProc (MIDIMAP.@)
 */
LRESULT CALLBACK	MIDIMAP_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
				   LPARAM dwParam1, LPARAM dwParam2)
{
/* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
/* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */

    switch (wMsg)
    {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return MIDIMAP_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return MIDIMAP_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MIDIMAP MultiMedia Driver !", "OSS Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
