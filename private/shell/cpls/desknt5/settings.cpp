/**************************************************************************\
* Module Name: settings.cpp
*
* Contains Implementation of the CDeviceSettings class who is in charge of
* the settings of a single display. This is the data base class who does the
* real change display settings work
*
* Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
*
\**************************************************************************/


#include "precomp.h"
#include "settings.hxx"
#include "ntreg.hxx"


TCHAR gpszError[] = TEXT("Unknown Error");


UINT g_cfDisplayDevice = 0;
UINT g_cfDisplayName = 0;
UINT g_cfDisplayDeviceID = 0;
UINT g_cfMonitorDevice = 0;
UINT g_cfMonitorName = 0;
UINT g_cfMonitorDeviceID = 0;
UINT g_cfExtensionInterface = 0;
UINT g_cfDisplayDeviceKey = 0;
UINT g_cfDisplayStateFlags = 0;
UINT g_cfDisplayPruningMode = 0;


/*****************************************************************\
*
* "new" and "delete" functions
*
\*****************************************************************/

#if DEBUG
void *  __cdecl operator new(size_t Size, LPCTSTR File, int Line)
{
    return (LPVOID) DeskAllocPrivate(File, Line, LPTR, Size);
}
#endif 

void *  __cdecl operator new(size_t nSize)
    {
    // Zero init just to save some headaches
    return((LPVOID)LocalAlloc(LPTR, nSize));
    }


void  __cdecl operator delete(void *pv)
    {
    LocalFree((HLOCAL)pv);
    }

#if DEBUG
#define new DBG_NEW
#endif

/*****************************************************************\
*
* helper routine
*
\*****************************************************************/

int CDeviceSettings::_InsertSortedDwords(
    int val1,
    int val2,
    int cval,
    int **ppval)
{
    int *oldpval = *ppval;
    int *tmppval;
    int  i;

    for (i=0; i<cval; i++)
    {
        tmppval = (*ppval) + (i * 2);

        if (*tmppval == val1)
        {
            if (*(tmppval + 1) == val2)
            {
                return cval;
            }
            else if (*(tmppval + 1) > val2)
            {
                break;
            }
        }
        else if (*tmppval > val1)
        {
            break;
        }
    }

    TraceMsg(TF_FUNC,"_InsertSortedDword, vals = %d %d, cval = %d, index = %d", val1, val2, cval, i);

    *ppval = (int *) LocalAlloc(LPTR, (cval + 1) * 2 * sizeof(DWORD));

    if (*ppval)
    {
        //
        // Insert the items at the right location in the array
        //
        if (oldpval) {
            CopyMemory(*ppval,
                       oldpval,
                       i * 2 * sizeof(DWORD));
        }

        *(*ppval + (i * 2))     = val1;
        *(*ppval + (i * 2) + 1) = val2;

        if (oldpval) {
            CopyMemory((*ppval) + 2 * (i + 1),
                        oldpval+ (i * 2),
                        (cval-i) * 2 * sizeof(DWORD));

            LocalFree(oldpval);
        }

        return (cval + 1);
    }

    return 0;
}

/*****************************************************************\
*
* debug routine
*
\*****************************************************************/


void CDeviceSettings::_Dump_CDeviceSettings(BOOL bAll)
{
    TraceMsg(TF_DUMP_CSETTINGS,"Dump of CDeviceSettings structure");
    TraceMsg(TF_DUMP_CSETTINGS,"\t _DisplayDevice  = %s",     _pDisplayDevice->DeviceName);
    TraceMsg(TF_DUMP_CSETTINGS,"\t _cpdm           = %d",     _cpdm     );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _apdm           = %08lx",  _apdm     );

    TraceMsg(TF_DUMP_CSETTINGS,"\t OrgResolution   = %d, %d", _ORGXRES, _ORGYRES    );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _ptOrgPos       = %d, %d", _ptOrgPos.x ,_ptOrgPos.y);
    TraceMsg(TF_DUMP_CSETTINGS,"\t OrgColor        = %d",     _ORGCOLOR      );
    TraceMsg(TF_DUMP_CSETTINGS,"\t OrgFrequency    = %d",     _ORGFREQ  );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _pOrgDevmode    = %08lx",  _pOrgDevmode   );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _fOrgAttached   = %d",     _fOrgAttached  );

    TraceMsg(TF_DUMP_CSETTINGS,"\t CurResolution   = %d, %d", _CURXRES, _CURYRES     );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _ptCurPos       = %d, %d", _ptCurPos.x ,_ptCurPos.y);
    TraceMsg(TF_DUMP_CSETTINGS,"\t CurColor        = %d",     _CURCOLOR      );
    TraceMsg(TF_DUMP_CSETTINGS,"\t CurFrequency    = %d",     _CURFREQ  );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _pCurDevmode    = %08lx",  _pCurDevmode   );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _fCurAttached   = %d",     _fCurAttached  );

    TraceMsg(TF_DUMP_CSETTINGS,"\t _fUsingDefault  = %d",     _fUsingDefault );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _fPrimary       = %d",     _fPrimary      );
    TraceMsg(TF_DUMP_CSETTINGS,"\t _cRef           = %d",     _cRef          );

    if (bAll)
    {
        _Dump_CDevmodeList();
    }
}

void CDeviceSettings::_Dump_CDevmodeList(VOID)
{
    ULONG i;

    for (i=0; i<_cpdm; i++)
    {
        LPDEVMODE lpdm = (_apdm + i)->lpdm;

        TraceMsg(TF_DUMP_CSETTINGS,"\t\t mode %d, %08lx, Flags %08lx, X=%d Y=%d C=%d F=%d",
                 i, lpdm, (_apdm + i)->dwFlags, 
                 lpdm->dmPelsWidth, lpdm->dmPelsHeight, lpdm->dmBitsPerPel, lpdm->dmDisplayFrequency);
    }
}

void CDeviceSettings::_Dump_CDevmode(LPDEVMODE pdm)
{
    TraceMsg(TF_DUMP_DEVMODE,"  Size        = %d",    pdm->dmSize);
    TraceMsg(TF_DUMP_DEVMODE,"  Fields      = %08lx", pdm->dmFields);
    TraceMsg(TF_DUMP_DEVMODE,"  XPosition   = %d",    pdm->dmPosition.x);
    TraceMsg(TF_DUMP_DEVMODE,"  YPosition   = %d",    pdm->dmPosition.y);
    TraceMsg(TF_DUMP_DEVMODE,"  XResolution = %d",    pdm->dmPelsWidth);
    TraceMsg(TF_DUMP_DEVMODE,"  YResolution = %d",    pdm->dmPelsHeight);
    TraceMsg(TF_DUMP_DEVMODE,"  Bpp         = %d",    pdm->dmBitsPerPel);
    TraceMsg(TF_DUMP_DEVMODE,"  Frequency   = %d",    pdm->dmDisplayFrequency);
    TraceMsg(TF_DUMP_DEVMODE,"  Flags       = %d",    pdm->dmDisplayFlags);
    TraceMsg(TF_DUMP_DEVMODE,"  XPanning    = %d",    pdm->dmPanningWidth);
    TraceMsg(TF_DUMP_DEVMODE,"  YPanning    = %d",    pdm->dmPanningHeight);
    TraceMsg(TF_DUMP_DEVMODE,"  DPI         = %d",    pdm->dmLogPixels);
    TraceMsg(TF_DUMP_DEVMODE,"  DriverExtra = %d",    pdm->dmDriverExtra);
    if (pdm->dmDriverExtra)
    {
        TraceMsg(TF_DUMP_CSETTINGS,"\t - %08lx %08lx",
        *(PULONG)(((PUCHAR)pdm)+pdm->dmSize),
        *(PULONG)(((PUCHAR)pdm)+pdm->dmSize + 4));
    }
}

//
// Lets perform the following operations on the list
//
// (1) Remove identical modes
// (2) Remove 16 color modes for which there is a 256
//     color equivalent.
// (3) Remove modes with any dimension less than 640x480
//

void CDeviceSettings::_FilterModes()
{
    BOOL bDisplay4BppModes;

#ifdef WINNT
    HKEY hkeyDriver;

    //
    // Check the registry to see if the user wants us
    // to show 16 color modes.
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_DISPLAY_4BPP_MODES,
                     0,
                     KEY_READ,
                     &hkeyDriver) ==  ERROR_SUCCESS)
    {
        bDisplay4BppModes = TRUE;
        RegCloseKey(hkeyDriver);
    }
    else
    {
        bDisplay4BppModes = FALSE;
    }
#else
    // Memphis always allows 16 color modes for the primary, for the secondary devices,
    // if they are attached, 16 color should not be a Enumerated at all.
    // But if they are unattached, all the modes will be there, so we should not allow for
    // 16 colors mode
    bDisplay4BppModes = TRUE;

    // on Win9x we only want to show "safe" display modes if we are
    // currently running in VGA mode (16 colors <= 800x600)
    //
    // a "safe" mode we define as a mode that does not use more than 1MB
    // of memory. we do this becuase we cant validate the mode correctly
    // when the display driver is not loaded.
    //
    HDC hdc = GetDC(NULL);
    int w   = GetDeviceCaps(hdc, HORZRES);
    int h   = GetDeviceCaps(hdc, VERTRES);
    int bpp = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);

    BOOL bDisplaySafeModes = FALSE;

    if (bpp <= 4 && w <= 800 && h <= 600)
       bDisplaySafeModes = TRUE;
#endif

    DWORD      i, j;
    int        cRes = 0;
    int       *pResTmp = NULL;
    LPDEVMODE  pdm;

    for (i = 0; i < _cpdm; i++)
    {
        // Don't show invalid modes;

        if ((_apdm + i)->dwFlags & MODE_INVALID)
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        //
        // If any of the following conditions are true, then we want to
        // remove the current mode.
        //

        // Mode is too small

        if ((pdm->dmPelsHeight < 480) ||
            (pdm->dmPelsWidth < 640))
        {
            TraceMsg(TF_DUMP_CSETTINGS,"_FilterModes: Mode %d - resolution too small", i);
            (_apdm + i)->dwFlags |= MODE_INVALID;
        }

        // 4 Bpp Mode

        if (pdm->dmBitsPerPel == 4)
        {
            //if (multimon)
            //{
            //    (_apdm + i)->dwFlags |= MODE_INVALID;
            //}

            // BUGBUG
            // ToddLa, why did you put DM_POSITION ?
            //if ((!bDisplay4BppModes) || !(pdm->dmFields & DM_POSITION)) &&
            //    (EquivExists(pCurrElem))
            //{

            if (!bDisplay4BppModes)
            {
                for (j = 0; j < _cpdm; j++)
                {
                    if (((_apdm + j)->lpdm->dmPelsWidth  == pdm->dmPelsWidth)  &&
                        ((_apdm + j)->lpdm->dmPelsHeight == pdm->dmPelsHeight) &&
                        ((_apdm + j)->lpdm->dmBitsPerPel >= 8)                 &&
                        (!((_apdm + j)->dwFlags & MODE_INVALID)))
                    {
                        TraceMsg(TF_DUMP_CSETTINGS,"_FilterModes: Mode %d - 4Bpp duplicated", i);
                        (_apdm + i)->dwFlags |= MODE_INVALID;
                    }
                }
            }
        }

        // Apparent duplicate

        for (j = 0; j < _cpdm; j++)
        {
            if (((_apdm + j)->lpdm->dmBitsPerPel == pdm->dmBitsPerPel) &&
                ((_apdm + j)->lpdm->dmPelsWidth  == pdm->dmPelsWidth)  &&
                ((_apdm + j)->lpdm->dmPelsHeight == pdm->dmPelsHeight) &&
                ((_apdm + j)->lpdm->dmDisplayFrequency == pdm->dmDisplayFrequency) &&
                (!((_apdm + j)->dwFlags & MODE_INVALID)) &&
                (j != i))
            {
                TraceMsg(TF_DUMP_CSETTINGS,"_FilterModes: Mode %d - Duplicate Mode", i);
                (_apdm + i)->dwFlags |= MODE_INVALID;
            }
        }

#ifndef WINNT
        // Win95 safe mode

        if (bDisplaySafeModes &&
            ((pdm->dmPelsWidth * pdm->dmPelsHeight * pdm->dmBitsPerPel) > 1024*768*8))
        {
            TraceMsg(TF_DUMP_CSETTINGS,"_FilterModes: Mode %d - Win9x Safe Mode", i);
            (_apdm + i)->dwFlags |= MODE_INVALID;
        }
#endif
    }
}


//
// _AddDevMode method
//
//  This method builds the index lists for the matrix.  There is one
//  index list for each axes of the three dimemsional matrix of device
//  modes.
//
// The entry is also automatically added to the linked list of modes if
// it is not alreay present in the list.
//

BOOL CDeviceSettings::_AddDevMode(LPDEVMODE lpdm)
{
    PMODEARRAY newapdm, tempapdm;

    //
    // Make sure all the fields of the DEVMODE are correctly filled in
    //

    //
    // Warn about:
    // - Drivers that return refresh rate of 0.
    // - Drivers that have an invalid set of DisplayFlags.
    // - Drivers that do not mark their devmode appropriately.
    //

    if ((lpdm->dmDisplayFrequency == 0) ||
        (lpdm->dmDisplayFlags & ~DMDISPLAYFLAGS_VALID) ||
        (lpdm->dmFields & ~(DM_BITSPERPEL    |
                            DM_PELSWIDTH     |
                            DM_PELSHEIGHT    |
                            DM_DISPLAYFLAGS  |
                            DM_DISPLAYFREQUENCY)))
    {
        _fBadData = TRUE;

        _Dump_CDevmode(lpdm);
    }

    //
    // Set the height for the test of the 1152 mode
    //

    if (lpdm->dmPelsWidth == 1152) {

        // Set1152Mode(lpdm->dmPelsHeight);
    }

    newapdm = (PMODEARRAY) LocalAlloc(LPTR, (_cpdm + 1) * sizeof(MODEARRAY));
    CopyMemory(newapdm, _apdm, _cpdm * sizeof(MODEARRAY));

    (newapdm + _cpdm)->dwFlags &= ~MODE_INVALID;
    (newapdm + _cpdm)->dwFlags |= MODE_RAW;
    (newapdm + _cpdm)->lpdm     = lpdm;

    tempapdm = _apdm;
    _apdm = newapdm;
    _cpdm++;

    if (tempapdm) {
        LocalFree(tempapdm);
    }

    return TRUE;
}

//
// Return a list of Resolutions supported, given a color depth
//

int CDeviceSettings::GetResolutionList(
    int Color,
    PPOINT *ppRes)
{
    DWORD      i;
    int        cRes = 0;
    int       *pResTmp = NULL;
    LPDEVMODE  pdm;

    *ppRes = NULL;

    for (i = 0; i < _cpdm; i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        if ((Color == -1) ||
            (Color == (int)pdm->dmBitsPerPel))
        {
            cRes = _InsertSortedDwords(pdm->dmPelsWidth,
                                       pdm->dmPelsHeight,
                                       cRes,
                                       &pResTmp);
        }
    }

    *ppRes = (PPOINT) pResTmp;

    return cRes;
}

//
//
// Return a list of color depths supported
//

int CDeviceSettings::GetColorList(
    LPPOINT Res,
    PLONGLONG *ppColor)
{
    DWORD      i;
    int        cColor = 0;
    int       *pColorTmp = NULL;
    LPDEVMODE  pdm;

    for (i = 0; i < _cpdm; i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        if ((Res == NULL) ||
            (Res->x == -1)                    ||
            (Res->y == -1)                    ||
            (Res->x == (int)pdm->dmPelsWidth) ||
            (Res->y == (int)pdm->dmPelsHeight))
        {
            cColor = _InsertSortedDwords(pdm->dmBitsPerPel,
                                         0,
                                         cColor,
                                         &pColorTmp);
        }
    }

    *ppColor = (PLONGLONG) pColorTmp;

    return cColor;
}

int CDeviceSettings::GetFrequencyList(int Color, LPPOINT Res, PLONGLONG *ppFreq)
{
    DWORD      i;
    int        cFreq = 0;
    int       *pFreqTmp = NULL;
    LPDEVMODE  pdm;
    POINT      res;

    if (Color == -1) {
        Color = _CURCOLOR;
    }

    if (Res == NULL) 
    {
        MAKEXYRES(&res, _CURXRES, _CURYRES);
    }
    else
    {
        res = *Res;
    }

    for (i = 0; i < _cpdm; i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        if (res.x == (int)pdm->dmPelsWidth  &&
            res.y == (int)pdm->dmPelsHeight &&
            Color == (int)pdm->dmBitsPerPel) 
        {
            cFreq = _InsertSortedDwords(pdm->dmDisplayFrequency,
                                         0,
                                         cFreq,
                                         &pFreqTmp);
        }
    }

    *ppFreq = (PLONGLONG) pFreqTmp;

    return cFreq;
}

void CDeviceSettings::SetCurFrequency(int Frequency)
{
    LPDEVMODE pdm;
    LPDEVMODE pdmMatch = NULL;
    ULONG i;

    for (i = 0; i < _cpdm; i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        //
        // Find the exact match. 
        //
        if (_CURCOLOR == (int) pdm->dmBitsPerPel &&
            _CURXRES == (int) pdm->dmPelsWidth      &&
            _CURYRES == (int) pdm->dmPelsHeight     &&
            Frequency        == (int) pdm->dmDisplayFrequency)
        {
            pdmMatch = pdm;
            break;
        }
    }

    //
    // We should always make a match because the list of frequencies shown to
    // the user is only for the current color & resolution
    //
    ASSERT(pdmMatch);
    if (pdmMatch) {
        _SetCurrentValues(pdmMatch);
    }
}

LPDEVMODE CDeviceSettings::GetCurrentDevMode(void)
{
    ULONG dmSize = _pCurDevmode->dmSize + _pCurDevmode->dmDriverExtra;
    PDEVMODE pdevmode  = (LPDEVMODE) LocalAlloc(LPTR, dmSize);

    if (pdevmode) {
        CopyMemory(pdevmode, _pCurDevmode, dmSize);
    }

    return pdevmode;
}

void CDeviceSettings::_SetCurrentValues(LPDEVMODE lpdm)
{
    _pCurDevmode   = lpdm;

    //
    // Don't save the other fields (like position) as they are programmed by
    // the UI separately.
    //
    // This should only save hardware specific fields.
    //

    TraceMsg(TF_DUMP_CSETTINGS,"");
    TraceMsg(TF_DUMP_CSETTINGS,"_SetCurrentValues complete");
    _Dump_CDeviceSettings(FALSE);
}


BOOL CDeviceSettings::_PerfectMatch(LPDEVMODE lpdm)
{
    for (DWORD i = 0; i < _cpdm; i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        if ((_apdm + i)->lpdm == lpdm)
        {
            _SetCurrentValues((_apdm + i)->lpdm);

            TraceMsg(TF_WARNING, "_PerfectMatch -- return TRUE");

            return TRUE;
        }
    }

    TraceMsg(TF_WARNING, "_PerfectMatch -- return FALSE");

    return FALSE;
}

BOOL CDeviceSettings::_ExactMatch(LPDEVMODE lpdm)
{
    LPDEVMODE pdm;
    LPDEVMODE pdmMatch = NULL;
    ULONG i;

    for (i = 0; i < _cpdm; i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        if (
            ((lpdm->dmFields & DM_BITSPERPEL) &&
             (pdm->dmBitsPerPel != lpdm->dmBitsPerPel))

            ||

            ((lpdm->dmFields & DM_PELSWIDTH) &&
             (pdm->dmPelsWidth != lpdm->dmPelsWidth))

            ||

            ((lpdm->dmFields & DM_PELSHEIGHT) &&
             (pdm->dmPelsHeight != lpdm->dmPelsHeight))

            ||

            ((lpdm->dmFields & DM_DISPLAYFREQUENCY) &&
             (pdm->dmDisplayFrequency != lpdm->dmDisplayFrequency))
           )
        {
           continue;
        }

        _SetCurrentValues(pdm);

        TraceMsg(TF_WARNING, "_ExactMatch -- return TRUE");

        return TRUE;
    }

    TraceMsg(TF_WARNING, "_ExactMatch -- return FALSE");

    return FALSE;
}

void CDeviceSettings::_BestMatch(LPPOINT Res, int Color)
{
    //
    // -1 means match loosely, based on current _xxx value
    //

    LPDEVMODE pdm;
    LPDEVMODE pdmMatch = NULL;
    ULONG i;

    for (i = 0; i < _cpdm; i++)
    {
        if(!_IsModeVisible(i))
        {
            continue;
        }

        pdm = (_apdm + i)->lpdm;

        //
        // Take care of exact matches
        //

        if ((Color != -1) &&
            (Color != (int)pdm->dmBitsPerPel))
        {
            continue;
        }

        if ((Res != NULL)  &&
            (Res->x != -1) &&
            ( (Res->x != (int)pdm->dmPelsWidth) ||
              (Res->y != (int)pdm->dmPelsHeight)) )
        {
            continue;
        }

        //
        // Find Best Match
        //

        if (pdmMatch == NULL)
        {
            pdmMatch = pdm;
        }

        //
        // Find best Color.
        //

        if ((Color == -1) &&
            ((int)pdmMatch->dmBitsPerPel != _CURCOLOR))
        {
            if ((int)pdmMatch->dmBitsPerPel > _CURCOLOR)
            {
                if ((int)pdm->dmBitsPerPel < (int)pdmMatch->dmBitsPerPel)
                {
                    pdmMatch = pdm;
                }
            }
            else
            {
                if (((int)pdm->dmBitsPerPel > (int)pdmMatch->dmBitsPerPel) &&
                    ((int)pdm->dmBitsPerPel <= _CURCOLOR))
                {
                    pdmMatch = pdm;
                }
            }
        }

        //
        // Find best Resolution.
        //

        if (((Res == NULL) || (Res->x == -1)) &&
            (((int)pdmMatch->dmPelsWidth  != _CURXRES) ||
             ((int)pdmMatch->dmPelsHeight != _CURYRES)))
        {
            if (((int)pdmMatch->dmPelsWidth   >  _CURXRES) ||
                (((int)pdmMatch->dmPelsWidth  == _CURXRES) &&
                 ((int)pdmMatch->dmPelsHeight >  _CURYRES)))
            {
                if (((int)pdm->dmPelsWidth  <  (int)pdmMatch->dmPelsWidth) ||
                    (((int)pdm->dmPelsWidth  == (int)pdmMatch->dmPelsWidth) &&
                     ((int)pdm->dmPelsHeight <  (int)pdmMatch->dmPelsHeight)))
                {
                    pdmMatch = pdm;
                }
            }
            else
            {
                if (((int)pdm->dmPelsWidth  >  (int)pdmMatch->dmPelsWidth) ||
                    (((int)pdm->dmPelsWidth  == (int)pdmMatch->dmPelsWidth) &&
                     ((int)pdm->dmPelsHeight >  (int)pdmMatch->dmPelsHeight)))
                {
                    if (((int)pdm->dmPelsWidth  <= _CURXRES) ||
                        (((int)pdm->dmPelsWidth  == _CURXRES) &&
                         ((int)pdm->dmPelsHeight <= _CURYRES)))
                    {
                        pdmMatch = pdm;
                    }
                }
            }
        }

        //
        // Find best Frequency.
        //
        if (((int)pdmMatch->dmDisplayFrequency != _CURFREQ) &&
            (!((Res == NULL) && 
               ((int)pdmMatch->dmPelsWidth  == _CURXRES) &&
               ((int)pdmMatch->dmPelsHeight == _CURYRES) &&
               (((int)pdm->dmPelsWidth  != _CURXRES) ||
                ((int)pdm->dmPelsHeight != _CURYRES)))) &&
            (!((Color == -1) && 
               ((int)pdmMatch->dmBitsPerPel == _CURCOLOR) &&
               ((int)pdm->dmBitsPerPel != _CURCOLOR))))
        {
            if ((int)pdmMatch->dmDisplayFrequency > _CURFREQ)
            {
                if ((int)pdm->dmDisplayFrequency < (int)pdmMatch->dmDisplayFrequency)
                {
                    pdmMatch = pdm;
                }
            }
            else
            {
                if (((int)pdm->dmDisplayFrequency > (int)pdmMatch->dmDisplayFrequency) &&
                    ((int)pdm->dmDisplayFrequency <= _CURFREQ))
                {
                    pdmMatch = pdm;
                }
            }
        }
    }

    _SetCurrentValues(pdmMatch);
}


BOOL CDeviceSettings::GetMonitorName(LPTSTR pszName)
{
    DISPLAY_DEVICE ddTmp;
    ZeroMemory(&ddTmp, sizeof(ddTmp));
    ddTmp.cb = sizeof(DISPLAY_DEVICE);

    if (EnumDisplayDevices(_pDisplayDevice->DeviceName, 0, &ddTmp, 0))
    {
        lstrcpy(pszName, (LPTSTR)ddTmp.DeviceString);

        ZeroMemory(&ddTmp, sizeof(ddTmp));
        ddTmp.cb = sizeof(DISPLAY_DEVICE);

        if (EnumDisplayDevices(_pDisplayDevice->DeviceName, 1, &ddTmp, 0))
            // Multiple monitors on the same adapter
            LoadString(hInstance, IDS_MULTIPLEMONITORS, pszName, 128 * SIZEOF(TCHAR));

        return TRUE;
    }
    else
    {
        LoadString(hInstance, IDS_UNKNOWNMONITOR, pszName, 128 * SIZEOF(TCHAR));
        return FALSE;
    }
}

BOOL CDeviceSettings::GetMonitorDevice(LPTSTR pszDevice)
{
    DISPLAY_DEVICE ddTmp;

    ZeroMemory(&ddTmp, sizeof(ddTmp));
    ddTmp.cb = sizeof(DISPLAY_DEVICE);

    if (EnumDisplayDevices(_pDisplayDevice->DeviceName, 0, &ddTmp, 0))
    {
        lstrcpy(pszDevice, (LPTSTR)ddTmp.DeviceName);

        return TRUE;
    }

    return FALSE;
}

STDMETHODIMP CDeviceSettings::GetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed)
{
    HRESULT hr;

    ASSERT(this);
    ASSERT(pfmtetc);
    ASSERT(pstgmed);

    // Ignore pfmtetc.ptd.  All supported data formats are device-independent.

    ZeroMemory(pstgmed, SIZEOF(*pstgmed));

    if ((hr = QueryGetData(pfmtetc)) == S_OK)
    {
        int cch;
        LPTSTR pszOut = NULL;
        TCHAR szMonitorName[130];
        TCHAR szMonitorDevice[40];

        if (pfmtetc->cfFormat == g_cfExtensionInterface)
        {
            //
            // Get the array of information back to the device
            //
            // Allocate a buffer large enough to store all of the information
            //

            PDESK_EXTENSION_INTERFACE pInterface;

            pInterface = (PDESK_EXTENSION_INTERFACE)
                             GlobalAlloc(GPTR,
                                         sizeof(DESK_EXTENSION_INTERFACE));

            // BUGBUG what do we do for Win9x
#ifdef WINNT
            if (pInterface)
            {
                CRegistrySettings * RegSettings = new CRegistrySettings(_pDisplayDevice->DeviceKey);

                pInterface->cbSize    = sizeof(DESK_EXTENSION_INTERFACE);
                pInterface->pContext  = this;

                pInterface->lpfnEnumAllModes    = CDeviceSettings::_lpfnEnumAllModes;
                pInterface->lpfnSetSelectedMode = CDeviceSettings::_lpfnSetSelectedMode;
                pInterface->lpfnGetSelectedMode = CDeviceSettings::_lpfnGetSelectedMode;
                pInterface->lpfnSetPruningMode = CDeviceSettings::_lpfnSetPruningMode;
                pInterface->lpfnGetPruningMode = CDeviceSettings::_lpfnGetPruningMode;

                RegSettings->GetHardwareInformation(&pInterface->Info);

                pstgmed->tymed = TYMED_HGLOBAL;
                pstgmed->hGlobal = pInterface;

                hr = S_OK;

                delete RegSettings;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
#endif
        }
        else if (pfmtetc->cfFormat == g_cfDisplayDeviceID)
        {
            hr = GetDevInstID(_pDisplayDevice->DeviceKey, pstgmed);
        }
        else if (pfmtetc->cfFormat == g_cfMonitorDeviceID)
        {
            DISPLAY_DEVICE ddTmp;
            BOOL    fKnownMonitor;

            ZeroMemory(&ddTmp, sizeof(ddTmp));
            ddTmp.cb = sizeof(DISPLAY_DEVICE);

            fKnownMonitor = EnumDisplayDevices(_pDisplayDevice->DeviceName, 0, &ddTmp, 0);
            hr = GetDevInstID((LPTSTR)(fKnownMonitor ? ddTmp.DeviceKey : TEXT("")), pstgmed);
        }
        else if (pfmtetc->cfFormat == g_cfDisplayStateFlags)
        {
            DWORD* pdwStateFlags = (DWORD*)GlobalAlloc(GPTR, sizeof(DWORD));
            if (pdwStateFlags)
            {
                *pdwStateFlags = _pDisplayDevice->StateFlags;
                pstgmed->tymed = TYMED_HGLOBAL;
                pstgmed->hGlobal = pdwStateFlags;
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else if (pfmtetc->cfFormat == g_cfDisplayPruningMode)
        {
            BYTE* pPruningMode = (BYTE*)GlobalAlloc(GPTR, sizeof(BYTE));
            if (pPruningMode)
            {
                *pPruningMode = (_bCanBePruned && _bIsPruningOn ? 1 : 0);
                pstgmed->tymed = TYMED_HGLOBAL;
                pstgmed->hGlobal = pPruningMode;
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            //
            // Get the data for legacy interfaces
            //

            if (pfmtetc->cfFormat == g_cfMonitorName)
            {
                GetMonitorName(szMonitorName);
                pszOut = szMonitorName;
            }
            else if (pfmtetc->cfFormat == g_cfMonitorDevice)
            {
                GetMonitorDevice(szMonitorDevice);
                pszOut = szMonitorDevice;
            }
            else if (pfmtetc->cfFormat == g_cfDisplayDevice)
            {
                pszOut = (LPTSTR)_pDisplayDevice->DeviceName;
            }
            else if (pfmtetc->cfFormat == g_cfDisplayDeviceKey)
            {
                pszOut = (LPTSTR)_pDisplayDevice->DeviceKey;
            }
            else 
            {
                ASSERT(pfmtetc->cfFormat == g_cfDisplayName);
                
                pszOut = (LPTSTR)_pDisplayDevice->DeviceString;
            }
            

            //
            // Transfer it.
            //

            cch = lstrlen(pszOut) + 1;

            LPWSTR pwszDevice = (LPWSTR)GlobalAlloc(GPTR, cch * SIZEOF(WCHAR));
            if (pwszDevice)
            {
                int cchConverted = 0;

                //
                // We always return UNICODE string
                //

#ifdef UNICODE
                lstrcpy(pwszDevice, pszOut);
#else
                cchConverted = MultiByteToWideChar(CP_ACP, 0, pszOut , -1, pwszDevice, cch);
                ASSERT(cchConverted == cch);
#endif
                pstgmed->tymed = TYMED_HGLOBAL;
                pstgmed->hGlobal = pwszDevice;

                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return(hr);
}


STDMETHODIMP CDeviceSettings::GetDataHere(FORMATETC *pfmtetc, STGMEDIUM *pstgpmed)
{
    ZeroMemory(pfmtetc, SIZEOF(pfmtetc));
    return E_NOTIMPL;
}

//
// Check that all the parameters to the interface are appropriately
//

STDMETHODIMP CDeviceSettings::QueryGetData(FORMATETC *pfmtetc)
{
    CLIPFORMAT cfFormat;

    if (pfmtetc->dwAspect != DVASPECT_CONTENT)
    {
        return DV_E_DVASPECT;
    }

    if ((pfmtetc->tymed & TYMED_HGLOBAL) == 0)
    {
        return  DV_E_TYMED;
    }

    cfFormat = pfmtetc->cfFormat;

    if ((cfFormat != g_cfDisplayDevice) &&
        (cfFormat != g_cfDisplayName)   &&
        (cfFormat != g_cfDisplayDeviceID)   &&
        (cfFormat != g_cfMonitorDevice) &&
        (cfFormat != g_cfMonitorName)   &&
        (cfFormat != g_cfMonitorDeviceID)   &&
        (cfFormat != g_cfExtensionInterface) &&
        (cfFormat != g_cfDisplayDeviceKey) &&
        (cfFormat != g_cfDisplayStateFlags) &&
        (cfFormat != g_cfDisplayPruningMode))
    {
        return DV_E_FORMATETC;
    }

    if (pfmtetc->lindex != -1)
    {
        return DV_E_LINDEX;
    }

    return S_OK;
}

STDMETHODIMP CDeviceSettings::GetCanonicalFormatEtc(FORMATETC *pfmtetcIn, FORMATETC *pfmtetcOut)
{
    HRESULT hr;
    ASSERT(pfmtetcIn);
    ASSERT(pfmtetcOut);

    hr = QueryGetData(pfmtetcIn);

    if (hr == S_OK)
    {
        *pfmtetcOut = *pfmtetcIn;

        if (pfmtetcIn->ptd == NULL)
            hr = DATA_S_SAMEFORMATETC;
        else
        {
            pfmtetcIn->ptd = NULL;
            ASSERT(hr == S_OK);
        }
    }
    else
        ZeroMemory(pfmtetcOut, SIZEOF(*pfmtetcOut));
    return(hr);
}


STDMETHODIMP CDeviceSettings::SetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed, BOOL bRelease)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDeviceSettings::EnumFormatEtc(DWORD dwDirFlags, IEnumFORMATETC ** ppiefe)
{
    HRESULT hr;

    ASSERT(ppiefe);
    *ppiefe = NULL;

    if (dwDirFlags == DATADIR_GET)
    {
        FORMATETC rgfmtetc[] =
        {
            { (CLIPFORMAT)g_cfDisplayDevice,      NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfDisplayName,        NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfMonitorDevice,      NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfMonitorName,        NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfExtensionInterface, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfDisplayDeviceID,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfMonitorDeviceID,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfDisplayDeviceKey,   NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfDisplayStateFlags,  NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { (CLIPFORMAT)g_cfDisplayPruningMode, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        };

        hr = SHCreateStdEnumFmtEtc(ARRAYSIZE(rgfmtetc), rgfmtetc, ppiefe);
    }
    else
        hr = E_NOTIMPL;

    return(hr);
}

STDMETHODIMP CDeviceSettings::DAdvise(FORMATETC *pfmtetc, DWORD dwAdviseFlags, IAdviseSink * piadvsink, DWORD * pdwConnection)
{
    ASSERT(pfmtetc);
    ASSERT(pdwConnection);

    *pdwConnection = 0;
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDeviceSettings::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDeviceSettings::EnumDAdvise(IEnumSTATDATA ** ppiesd)
{
    ASSERT(ppiesd);
    *ppiesd = NULL;
    return OLE_E_ADVISENOTSUPPORTED;
}


void CDeviceSettings::_InitClipboardFormats()
{
    if (g_cfDisplayDevice == 0)
        g_cfDisplayDevice = RegisterClipboardFormat(DESKCPLEXT_DISPLAY_DEVICE);

    if (g_cfDisplayDeviceID == 0)
        g_cfDisplayDeviceID = RegisterClipboardFormat(DESKCPLEXT_DISPLAY_ID);
        
    if (g_cfDisplayName == 0)
        g_cfDisplayName = RegisterClipboardFormat(DESKCPLEXT_DISPLAY_NAME);

    if (g_cfMonitorDevice == 0)
        g_cfMonitorDevice = RegisterClipboardFormat(DESKCPLEXT_MONITOR_DEVICE);

    if (g_cfMonitorDeviceID == 0)
        g_cfMonitorDeviceID = RegisterClipboardFormat(DESKCPLEXT_MONITOR_ID);
        
    if (g_cfMonitorName == 0)
        g_cfMonitorName = RegisterClipboardFormat(DESKCPLEXT_MONITOR_NAME);

    if (g_cfExtensionInterface == 0)
        g_cfExtensionInterface = RegisterClipboardFormat(DESKCPLEXT_INTERFACE);

    if (g_cfDisplayDeviceKey == 0)
        g_cfDisplayDeviceKey = RegisterClipboardFormat(DESKCPLEXT_DISPLAY_DEVICE_KEY);

    if (g_cfDisplayStateFlags == 0)
        g_cfDisplayStateFlags = RegisterClipboardFormat(DESKCPLEXT_DISPLAY_STATE_FLAGS);
    
    if (g_cfDisplayPruningMode == 0)
        g_cfDisplayPruningMode = RegisterClipboardFormat(DESKCPLEXT_PRUNING_MODE);
}

HRESULT CDeviceSettings::QueryInterface(REFIID riid, LPVOID * ppvObj)
{ 
    // ppvObj must not be NULL
    ASSERT(ppvObj != NULL);
    
    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = SAFECAST(this, IUnknown *);
    else if (IsEqualIID(riid, IID_IDataObject))
        *ppvObj = SAFECAST(this, IDataObject*);
    else
        return E_NOINTERFACE;  // Otherwise, don't delegate to HTMLObj!!
    
    AddRef();
    return S_OK;
}


ULONG CDeviceSettings::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CDeviceSettings::Release()
{
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;

    return 0;
}


STDMETHODIMP_(LPDEVMODEW)
CDeviceSettings::_lpfnEnumAllModes(
    LPVOID pContext,
    DWORD iMode)
{
    DWORD cCount = 0;
    DWORD i;

    CDeviceSettings *pSettings = (CDeviceSettings *) pContext;

    for (i = 0; i < pSettings->_cpdm; i++)
    {
        // Don't show invalid modes or raw modes if pruning is on;

        if(!_IsModeVisible(pSettings, i))
        {
            continue;
        }

        if (cCount == iMode)
        {
#ifdef WINNT
            return (pSettings->_apdm + i)->lpdm;
#endif
        }

        cCount++;
    }

    return NULL;
}

STDMETHODIMP_(BOOL)
CDeviceSettings::_lpfnSetSelectedMode(
    LPVOID pContext,
    LPDEVMODEW lpdm)
{
    CDeviceSettings *pSettings = (CDeviceSettings *) pContext;

#ifdef WINNT
    return pSettings->_PerfectMatch(lpdm);
#else
    return FALSE;
#endif
}

STDMETHODIMP_(LPDEVMODEW)
CDeviceSettings::_lpfnGetSelectedMode(
    LPVOID pContext
    )
{
    CDeviceSettings *pSettings = (CDeviceSettings *) pContext;

#ifdef WINNT
    return pSettings->_pCurDevmode;
#else
    return NULL;
#endif
}

STDMETHODIMP_(VOID)
CDeviceSettings::_lpfnSetPruningMode(
    LPVOID pContext,
    BOOL bIsPruningOn)
{
    CDeviceSettings *pSettings = (CDeviceSettings *) pContext;
    pSettings->SetPruningMode(bIsPruningOn);
}

STDMETHODIMP_(VOID)
CDeviceSettings::_lpfnGetPruningMode(
    LPVOID pContext,
    BOOL* pbCanBePruned,
    BOOL* pbIsPruningReadOnly,
    BOOL* pbIsPruningOn)
{
    CDeviceSettings *pSettings = (CDeviceSettings *) pContext;
    pSettings->GetPruningMode(pbCanBePruned, 
                              pbIsPruningReadOnly, 
                              pbIsPruningOn);
}

// If any attached device is at 640x480, we want to force small font
BOOL CDeviceSettings::IsSmallFontNecessary()
{
    if (_fOrgAttached || _fCurAttached)
    {
        //
        // Force Small fonts at 640x480
        //
        if (_CURXRES < 800 || _CURYRES < 600)
            return TRUE;
    }
    return FALSE;
}

// Constructor for CDeviceSettings
//
//  (gets called when ever a CDeviceSettings object is created)
//

CDeviceSettings::CDeviceSettings() 
    : _cRef(1)
    , _cpdm(0) 
    , _apdm(0)
    , _hPruningRegKey(NULL)
    , _bCanBePruned(FALSE)
    , _bIsPruningReadOnly(TRUE)
    , _bIsPruningOn(FALSE)
    , _pOrgDevmode(NULL)
    , _pCurDevmode(NULL)
    , _fOrgAttached(FALSE)
    , _fCurAttached(FALSE)
{
}

//
// Destructor
//
CDeviceSettings::~CDeviceSettings() {

    TraceMsg(TF_WARNING, "**** Destructing %s", _pDisplayDevice->DeviceName);

    while(_cpdm--)
    {
        LocalFree((_apdm + _cpdm)->lpdm);
    }
    _cpdm = 0;

    LocalFree(_apdm);
    _apdm = NULL;

    if(NULL != _hPruningRegKey)
        RegCloseKey(_hPruningRegKey);
}


//
// InitSettings -- Enumerate the settings, and build the mode list when
//

BOOL CDeviceSettings::InitSettings(LPDISPLAY_DEVICE pDisplay)
{
    BYTE      devmode[sizeof(DEVMODE) + 0xFFFF];
    LPDEVMODE pdevmode = (LPDEVMODE) devmode;
    ULONG  i = 0;
    BOOL   bCurrent = FALSE;
    BOOL   bRegistry = FALSE;
    BOOL   bExact = FALSE;

    //
    // Set the cached values for modes.
    //

    MAKEXYRES(&_ptCurPos, 0, 0);
    _fCurAttached  = FALSE;
    _pCurDevmode   = NULL;

    //
    // Save the display name
    //

    ASSERT(pDisplay);

    _pDisplayDevice = pDisplay;

    TraceMsg(TF_GENERAL, "Initializing CDeviceSettings for %s", _pDisplayDevice->DeviceName);

    //
    // Pruning Mode
    //

    _bCanBePruned = ((_pDisplayDevice->StateFlags & DISPLAY_DEVICE_MODESPRUNED) != 0);
    _bIsPruningReadOnly = TRUE;
    _bIsPruningOn = FALSE;
    if (_bCanBePruned)
        {
        _bIsPruningOn = TRUE; // if can be pruned, by default pruning is on 
        GetDeviceRegKey(_pDisplayDevice->DeviceKey, &_hPruningRegKey, &_bIsPruningReadOnly);
        if (_hPruningRegKey)
            {
            DWORD dwIsPruningOn = 1;
            DWORD cb = sizeof(dwIsPruningOn);
            RegQueryValueEx(_hPruningRegKey, 
                            SZ_PRUNNING_MODE,
                            NULL, 
                            NULL, 
                            (LPBYTE)&dwIsPruningOn, 
                            &cb);
            _bIsPruningOn = (dwIsPruningOn != 0);
            }
        }

    //
    // Lets generate a list with all the possible modes.
    //

    ZeroMemory(pdevmode,sizeof(DEVMODE));
    pdevmode->dmSize = sizeof(DEVMODE);
    pdevmode->dmDriverExtra = 0xFFFF;

    // 
    // Enum the raw list of modes
    // 

    while (EnumDisplaySettingsEx(_pDisplayDevice->DeviceName, i++, pdevmode, EDS_RAWMODE))
    {
        WORD      dmsize = pdevmode->dmSize + pdevmode->dmDriverExtra;
        LPDEVMODE lpdm = (LPDEVMODE) LocalAlloc(LPTR, dmsize);

        if (lpdm)
        {
            CopyMemory(lpdm, pdevmode, dmsize);
            _AddDevMode(lpdm);
        }

        pdevmode->dmDriverExtra = 0xFFFF;
    }

    //
    // Filter the list of modes
    //

    _FilterModes();

    if(_bCanBePruned)
    {
        // 
        // Enum pruned list of modes
        // 

        i = 0;
        _bCanBePruned = FALSE;
        pdevmode->dmDriverExtra = 0xFFFF;
        
        while (EnumDisplaySettingsEx(_pDisplayDevice->DeviceName, i++, pdevmode, 0))
        {
            if(_MarkMode(pdevmode))
                _bCanBePruned = TRUE; // at least one non-raw mode  
            pdevmode->dmDriverExtra = 0xFFFF;
        }

        if(!_bCanBePruned)
        {
            _bIsPruningReadOnly = TRUE;
            _bIsPruningOn = FALSE;
        }
    }

    //
    // Debug
    //

    _Dump_CDeviceSettings(TRUE);

    //
    // Get the current mode
    //

    ZeroMemory(pdevmode,sizeof(DEVMODE));
    pdevmode->dmSize = sizeof(DEVMODE);
    pdevmode->dmDriverExtra = 0xFFFF;

    bCurrent = EnumDisplaySettingsEx(_pDisplayDevice->DeviceName,
                                     ENUM_CURRENT_SETTINGS,
                                     pdevmode,
                                     0);

#ifndef WINNT
#if 0
    //
    // BUGBUG does not compile
    //
    // in VGA fallback or SafeMode EnumDisplaySettings will fail
    // deal with this case
    //
    if (!bAddMode)
    {
        ZeroMemory(pdefaultdm, sizeof(DEVMODE));
        pdefaultdm->dmSize = sizeof(DEVMODE);

        HDC hdc = GetDC(NULL);
        pdefaultdm->dmPelsWidth  = GetDeviceCaps(hdc, HORZRES);
        pdefaultdm->dmPelsHeight = GetDeviceCaps(hdc, VERTRES);
        pdefaultdm->dmBitsPerPel = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
        pdefaultdm->dmFields = DM_POSITION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        ReleaseDC(NULL, hdc);
        bAddMode = TRUE;
    }
#endif
#endif

    if (!bCurrent)
    {
        TraceMsg(TF_WARNING, "InitSettings -- No Current Mode. Try to use registry settings.");
        
        ZeroMemory(pdevmode,sizeof(DEVMODE));
        pdevmode->dmSize = sizeof(DEVMODE);
        pdevmode->dmDriverExtra = 0xFFFF;

        bRegistry = EnumDisplaySettingsEx(_pDisplayDevice->DeviceName,
                                          ENUM_REGISTRY_SETTINGS,
                                          pdevmode,
                                          0);
    }

    //
    // Set the default values based on registry or current settings.
    //

    if (bCurrent || bRegistry)
    {
        //
        // Check if this DEVMODE is in the list
        //

        TraceMsg(TF_FUNC, "Devmode for Exact Matching");
        _Dump_CDevmode(pdevmode);
        TraceMsg(TF_FUNC, "");

        bExact = _ExactMatch(pdevmode);

        if (!bExact && bCurrent)
            {
            //
            // If the current mode is not in the list, we may have a problem.
            //
#ifndef WINNT
            WORD      dmsize = pdevmode->dmSize + pdevmode->dmDriverExtra;
            LPDEVMODE lpdm = (LPDEVMODE) LocalAlloc(LPTR, dmsize);

            CopyMemory(lpdm, pdevmode, dmsize);
            _AddDevMode(lpdm);

            bExact = _ExactMatch(lpdm);
#endif
            }
            
        //
        // Is attached?
        //
    
        if(bCurrent)
            _fOrgAttached = _fCurAttached = ((pdevmode->dmFields & DM_POSITION) ? 1 : 0);
        
        //
        // Set the original values
        //
    
    	if (bExact == TRUE)
        {
            MAKEXYRES(&_ptCurPos, pdevmode->dmPosition.x, pdevmode->dmPosition.y);
            ConfirmChangeSettings();
        }
    }

    //
    // Generate a popup if the driver returns bad data.
    //

    if (_fBadData)
    {
        FmtMessageBox(ghwndPropSheet,
                      MB_ICONEXCLAMATION,
                      MSG_CONFIGURATION_PROBLEM,
                      IDS_OLD_DRIVER);
    }

    //
    // If we have no modes, return FALSE.
    //

    if (_cpdm == 0)
    {
        FmtMessageBox(ghwndPropSheet,
                      MB_ICONEXCLAMATION,
                      MSG_CONFIGURATION_PROBLEM,
                      MSG_INVALID_OLD_DISPLAY_DRIVER);

        return FALSE;
    }

    //
    // If there were no current values, set some now
    // But don't confirm them ...
    //
    if (bExact == FALSE)
    {
        TraceMsg(TF_WARNING, "InitSettings -- No Current OR Registry Mode");

        i = 0;

        //
        // Try setting any mode as the current.
        //

        while (_PerfectMatch((_apdm + i++)->lpdm) == FALSE)
        {
            if (i > _cpdm)
            {
                FmtMessageBox(ghwndPropSheet,
                              MB_ICONEXCLAMATION,
                              MSG_CONFIGURATION_PROBLEM,
                              MSG_INVALID_OLD_DISPLAY_DRIVER);

                return FALSE;
            }
        }
        
        if (_fCurAttached)
            MAKEXYRES(&_ptCurPos, _pCurDevmode->dmPosition.x, _pCurDevmode->dmPosition.y);
    }

    //
    // Export our interfaces for extended properly pages.
    //

    _InitClipboardFormats();

    //
    // Final debug output
    //

    TraceMsg(TF_DUMP_CSETTINGS," InitSettings successful - current values :");
    _Dump_CDeviceSettings(FALSE);


    return TRUE;
}

//
// SaveSettings
//
//  Writes the new display parameters to the proper place in the
//  registry.
//

int CDeviceSettings::SaveSettings(DWORD dwSet)
{
    int iResult;

    //
    // Make a copy of the current devmode
    //

    ULONG dmSize = _pCurDevmode->dmSize + _pCurDevmode->dmDriverExtra;
    PDEVMODE pdevmode  = (LPDEVMODE) LocalAlloc(LPTR, dmSize);

    if (pdevmode)
    {
        CopyMemory(pdevmode, _pCurDevmode, dmSize);

        //
        // Save all of the new values out to the registry
        // Resolution color bits and frequency
        //
        //
        // We always have to set DM_POSITION when calling the API.
        // In order to remove a device from the desktop, what actually needs
        // to be done is provide an empty rectangle.
        //

        pdevmode->dmFields |= DM_POSITION;

        if (!_fCurAttached)
        {
            pdevmode->dmPelsWidth = 0;
            pdevmode->dmPelsHeight = 0;
        }
        else
        {
            pdevmode->dmPosition.x = _ptCurPos.x;
            pdevmode->dmPosition.y = _ptCurPos.y;
        }

#ifndef WINNT
        //
        // on Win9x work around a bug, dont try to set the position (DM_POSITION)
        // on the primary if we are running a old driver (VGA.DRV)
        // it is allways going to fail, if we try.
        // if we are setting the same mode, dont change the position
        //
        HDC hdc = GetDC(NULL);
        int w   = GetDeviceCaps(hdc, HORZRES);
        int h   = GetDeviceCaps(hdc, VERTRES);
        int bpp = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
        int c1  = GetDeviceCaps(hdc, CAPS1);
        ReleaseDC(NULL, hdc);

        if (_fPrimary && !(c1 & C1_REINIT_ABLE) &&
            (int)pdevmode->dmPelsWidth  == w &&
            (int)pdevmode->dmPelsHeight == h &&
            (int)pdevmode->dmBitsPerPel == bpp)
        {
            pdevmode->dmFields &= ~(DM_POSITION|DM_DISPLAYFREQUENCY|DM_DISPLAYFLAGS);
        }
#endif

        TraceMsg(TF_GENERAL, "SaveSettings:: Display: %s", _pDisplayDevice->DeviceName);
        _Dump_CDevmode(pdevmode);

        //
        // These calls have NORESET flag set so that it only goes to
        // change the registry settings, it does not refresh the display
        //

        // If EnumDisplaySettings was called with EDS_RAWMODE, we need CDS_RAWMODE below.
        // Otherwise, it's harmless.
        iResult = ChangeDisplaySettingsEx(_pDisplayDevice->DeviceName,
                                          pdevmode,
                                          NULL,
                                          CDS_RAWMODE | dwSet | ( _fPrimary ? CDS_SET_PRIMARY : 0),
                                          NULL);

        if (iResult < 0)
        {
            TraceMsg(TF_WARNING, "**** SaveSettings:: ChangeDisplaySettingsEx not successful on %s", _pDisplayDevice->DeviceName);
        }

        if ((dwSet & CDS_UPDATEREGISTRY) && (iResult == DISP_CHANGE_SUCCESSFUL))
        {
            if (_fOrgAttached != _fCurAttached)
            {
                // This is for real, we should not have the test flag there.
                ASSERT(!(dwSet & CDS_TEST));
                if (_fOrgAttached == FALSE)
                {
                    ASSERT(_fCurAttached == TRUE);
                    // BUGBUG RefreshSettings(TRUE);
                }
                _fOrgAttached = _fCurAttached;
            }
        }

        LocalFree(pdevmode);
    }

    return iResult;
}

HRESULT CDeviceSettings::GetDevInstID(LPTSTR lpszDeviceKey, STGMEDIUM *pstgmed)
{
    HRESULT hr = E_FAIL;
    
// BUGBUG what do we do for Win9x
#ifdef WINNT
    LPWSTR  pwDevInstanceId;
    // Get the Display Device instance ID.
    if(pwDevInstanceId = (LPWSTR)(GlobalAlloc(GPTR, DEV_INSTANCE_ID_LENGTH*sizeof(WCHAR))))
    {
        CRegistrySettings * RegSettings = new CRegistrySettings(lpszDeviceKey);

        RegSettings->GetDeviceInstanceId(pwDevInstanceId, DEV_INSTANCE_ID_LENGTH);
                    
        pstgmed->tymed = TYMED_HGLOBAL;
        pstgmed->hGlobal = pwDevInstanceId;

        hr = S_OK;

        delete RegSettings;
    }
#endif //WINNT

    return hr;
}


BOOL CDeviceSettings::ConfirmChangeSettings()
{
    // Succeeded, so, reset the original settings

    _ptOrgPos      = _ptCurPos;
    _pOrgDevmode   = _pCurDevmode;
    _fOrgAttached  = _fCurAttached;

    return TRUE;
}

int CDeviceSettings::RestoreSettings()
{
    //
    // Test failed, so retore the old settings, only restore the color and resolution
    // information, and do restore the monitor position and its attached status
    // Although this function is currently only called when restoring resolution
    // the user could have changed position, then resolution and then clicked 'Apply,'
    // in which case we want to revert position as well.
    //

    int      iResult = DISP_CHANGE_SUCCESSFUL;
    PDEVMODE pdevmode;

    //
    // If this display was originally turned off, don't bother
    //

    if ((_pOrgDevmode != NULL) &&
        //(_pOrgDevmode != _pCurDevmode))
        ((_pOrgDevmode != _pCurDevmode) || (_ptOrgPos.x != _ptCurPos.x) || (_ptOrgPos.y != _ptCurPos.y) ))
    {
        pdevmode = _pOrgDevmode;
		
        pdevmode->dmFields |= DM_POSITION;
        pdevmode->dmPosition.x = _ptOrgPos.x;
        pdevmode->dmPosition.y = _ptOrgPos.y;

        TraceMsg(TF_GENERAL, "RestoreSettings:: Display: %s", _pDisplayDevice->DeviceName);
        _Dump_CDevmode(pdevmode);

        // If EnumDisplaySettings was called with EDS_RAWMODE, we need CDS_RAWMODE below.
        // Otherwise, it's harmless.
        iResult = ChangeDisplaySettingsEx(_pDisplayDevice->DeviceName,
                                          pdevmode,
                                          NULL,
                                          CDS_RAWMODE | CDS_UPDATEREGISTRY | CDS_NORESET | ( _fPrimary ? CDS_SET_PRIMARY : 0),
                                          NULL);
        if (iResult  < 0 )
        {
            TraceMsg(TF_WARNING, "**** RestoreSettings:: ChangeDisplaySettingsEx not successful on %s", _pDisplayDevice->DeviceName);
            ASSERT(FALSE);
            return FALSE;
        }
        else
        {
            // Succeeded, so, reset the original settings
            _ptCurPos      = _ptOrgPos;
            _pCurDevmode   = _pOrgDevmode;
            _fCurAttached  = _fOrgAttached;
            
            if(_bCanBePruned && !_bIsPruningReadOnly && _bIsPruningOn && _IsCurDevmodeRaw())
                SetPruningMode(FALSE);
        }

    }

    return iResult;
}

    
void CDeviceSettings::SetPruningMode(BOOL bIsPruningOn)
{
    ASSERT (_bCanBePruned && !_bIsPruningReadOnly);
    
    if (_bCanBePruned && 
        !_bIsPruningReadOnly &&
        ((bIsPruningOn != 0) != _bIsPruningOn))
    {
        _bIsPruningOn = (bIsPruningOn != 0);

        DWORD dwIsPruningOn = (DWORD)_bIsPruningOn;
        RegSetValueEx(_hPruningRegKey, 
                      SZ_PRUNNING_MODE,
                      NULL, 
                      REG_DWORD, 
                      (LPBYTE) &dwIsPruningOn, 
                      sizeof(dwIsPruningOn));

        //
        // handle the special case when we pruned out the current mode
        //
        if(_bIsPruningOn && _IsCurDevmodeRaw())
        {
            //
            // switch to the closest mode
            //
            _BestMatch(NULL, -1);
        }
        
    }
}


void CDeviceSettings::GetPruningMode(BOOL* pbCanBePruned, 
                                     BOOL* pbIsPruningReadOnly,
                                     BOOL* pbIsPruningOn)
{
    ASSERT(pbCanBePruned && pbIsPruningReadOnly && pbIsPruningOn);
    *pbCanBePruned = _bCanBePruned;
    *pbIsPruningReadOnly = _bIsPruningReadOnly;
    *pbIsPruningOn = _bIsPruningOn;
}


BOOL CDeviceSettings::_IsModeVisible(int i)
{
    return _IsModeVisible(this, i);
}


BOOL CDeviceSettings::_IsModeVisible(CDeviceSettings* pSettings, int i)
{
    ASSERT(pSettings);

    // (the mode is valid) AND
    // ((pruning mode is off) OR (mode is not raw))
    return ((!((pSettings->_apdm + i)->dwFlags & MODE_INVALID)) &&
            ((!pSettings->_bIsPruningOn) || 
             (!((pSettings->_apdm + i)->dwFlags & MODE_RAW))
            )
           );
}


BOOL CDeviceSettings::_MarkMode(LPDEVMODE lpdm)
{
    LPDEVMODE pdm;
    ULONG i;
    BOOL bMark = FALSE;

    for (i = 0; i < _cpdm; i++)
    {
        if (!((_apdm + i)->dwFlags & MODE_INVALID))
        {
            pdm = (_apdm + i)->lpdm;

            if (
                ((lpdm->dmFields & DM_BITSPERPEL) &&
                 (pdm->dmBitsPerPel == lpdm->dmBitsPerPel))

                &&

                ((lpdm->dmFields & DM_PELSWIDTH) &&
                 (pdm->dmPelsWidth == lpdm->dmPelsWidth))

                &&

                ((lpdm->dmFields & DM_PELSHEIGHT) &&
                 (pdm->dmPelsHeight == lpdm->dmPelsHeight))

                &&

                ((lpdm->dmFields & DM_DISPLAYFREQUENCY) &&
                 (pdm->dmDisplayFrequency == lpdm->dmDisplayFrequency))
               )
            {
               (_apdm + i)->dwFlags &= ~MODE_RAW;
               bMark = TRUE;
            }
        }
    }

    return bMark;
}


BOOL CDeviceSettings::_IsCurDevmodeRaw()
{
    LPDEVMODE pdm;
    ULONG i;
    BOOL bCurrentAndPruned = FALSE;

    for (i = 0; i < _cpdm; i++)
    {
        if (!((_apdm + i)->dwFlags & MODE_INVALID) &&
            ((_apdm + i)->dwFlags & MODE_RAW))
        {
            pdm = (_apdm + i)->lpdm;

            if (
                ((_pCurDevmode->dmFields & DM_BITSPERPEL) &&
                 (pdm->dmBitsPerPel == _pCurDevmode->dmBitsPerPel))

                &&

                ((_pCurDevmode->dmFields & DM_PELSWIDTH) &&
                 (pdm->dmPelsWidth == _pCurDevmode->dmPelsWidth))

                &&

                ((_pCurDevmode->dmFields & DM_PELSHEIGHT) &&
                 (pdm->dmPelsHeight == _pCurDevmode->dmPelsHeight))

                &&

                ((_pCurDevmode->dmFields & DM_DISPLAYFREQUENCY) &&
                 (pdm->dmDisplayFrequency == _pCurDevmode->dmDisplayFrequency))
               )
            {
                bCurrentAndPruned = TRUE;
                break;
            }
        }
    }

    return bCurrentAndPruned;     
}
