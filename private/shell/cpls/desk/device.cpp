/**************************************************************************\
* Module Name: device.cpp (old settings.cpp from NT5)
*
* Contains all the utility code to help change or manage display device
* settings
*
* Copyright (c) Microsoft Corp.  1992-1996 All Rights Reserved
*
* NOTES:
*
* History: Created by (AndreVa?)
*          Modified and changed name by dli  on July 17, 1997
*
\**************************************************************************/


#include "precomp.h"
#include "setcdcl.hxx"
#include "device.hxx"
#include "settings.hxx"
/****************************************************************\
*
* Global vars and structures used only in this file
*
\****************************************************************/

DWORD g_aiSetHelpIds[] = {

//    ID_DSP_CHANGE,      IDH_DSKTPMONITOR_CHANGE_DISPLAY,
    IDC_COLORBOX,       IDH_DSKTPMONITOR_COLOR,
    ID_DSP_CLRPALGRP,   IDH_DSKTPMONITOR_COLOR,
//    ID_DSP_COLORBAR,    IDH_DSKTPMONITOR_COLOR,
    IDC_SCREENSIZE,     IDH_DSKTPMONITOR_AREA,
    ID_DSP_DSKAREAGRP,  IDH_DSKTPMONITOR_AREA,
//    ID_DSP_X_BY_Y,      IDH_DSKTPMONITOR_AREA,
    ID_DSP_REFFREQGRP,  IDH_DSKTPMONITOR_REFRESH,
    ID_DSP_FREQ,        IDH_DSKTPMONITOR_REFRESH,
    ID_DSP_LIST_ALL,    IDH_DSKTPMONITOR_LIST_MODES,
    IDC_SCREENSAMPLE,   IDH_DSKTPMONITOR_MONITOR,
    ID_DSP_TEST,        IDH_DSKTPMONITOR_TEST,
    ID_DSP_FONTSIZEGRP, IDH_DSKTPMONITOR_FONTSIZE,
    ID_DSP_FONTSIZE,    IDH_DSKTPMONITOR_FONTSIZE,
    ID_ADP_ADPGRP,      IDH_DSKTPMONITOR_ADTYPE,
    ID_ADP_ADAPTOR,     IDH_DSKTPMONITOR_ADTYPE,
//    ID_ADP_CHGADP,      IDH_DSKTPMONITOR_CHANGE1,
//    ID_ADP_DETECT,      IDH_DSKTPMONITOR_DETECT,
    ID_ADP_ADPINFGRP,   IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_CHIP,        IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_DAC,         IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_MEM,         IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_ADP_STRING,  IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_BIOS_INFO,   IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_AI1,         IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_AI2,         IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_AI3,         IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_AI4,         IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_AI5,         IDH_DSKTPMONITOR_AD_FACTS,
    ID_ADP_DRVINFGRP,   IDH_DSKTPMONITOR_DRIVER,
    ID_ADP_MANUFACT,    IDH_DSKTPMONITOR_DRIVER,
    ID_ADP_VERSION,     IDH_DSKTPMONITOR_DRIVER,
    ID_ADP_CURFILES,    IDH_DSKTPMONITOR_DRIVER,
    ID_ADP_DI1,         IDH_DSKTPMONITOR_DRIVER,
    ID_ADP_DI2,         IDH_DSKTPMONITOR_DRIVER,
    ID_ADP_DI3,         IDH_DSKTPMONITOR_DRIVER,


    // not implemented yet: IDH_DSKTPMONITOR_CUSTOM
    //                      IDH_DSKTPMONITOR_ENERGY
    //                      IDH_DSKTPMONITOR_CHANGE2
    //                      IDH_DSKTPMONITOR_MONTYPE
    0, 0
};

//
// Globals
//

//
// Currently there is a bug in the system that occationally causes the
// control panel applet to be resized.  The resize causes the control
// panel to display at about half its usual size, and the user can not
// see any of the buttons, etc.  This code will work around the problem.
//
// However, after we ship, we should really determine why this is
// happening.
//

//#define RESIZE_PROBLEM 1

#if RESIZE_PROBLEM
LRESULT WINAPI MyProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

WNDPROC gOldProc=NULL;
#endif

HWND ghwndPropSheet;


/*****************************************************************\
*
* Pure "C" functions used only in this file
*
\*****************************************************************/
LPTSTR SubStrEnd( LPTSTR pszTarget, LPTSTR pszScan);
DWORD WINAPI ApplyNowThd( LPVOID lpThreadParameter);

/*****************************************************************\
*
* "new" and "delete" functions
*
\*****************************************************************/
//#if defined(__cplusplus) && defined(CPP_FUNCTIONS)

void *  __cdecl operator new(unsigned int nSize)
    {
    // Zero init just to save some headaches
    return((LPVOID)LocalAlloc(LPTR, nSize));
    }


void  __cdecl operator delete(void *pv)
    {
    LocalFree((HLOCAL)pv);
    }

//extern "C" int __cdecl _purecall(void) {return 0;}

//#endif

/*****************************************************************\
*
* List class
*
\*****************************************************************/

typedef struct tagLISTELEM LISTELEM;

struct tagLISTELEM
{
    LPDEVMODE value;
    LISTELEM *next;
};

class CList
{
private:
    LISTELEM *Head;

public:
    CList() { Head = (LISTELEM *) NULL; }
    ~CList();

    void Insert(LPDEVMODE p);
    LPDEVMODE Pop();
    void Process();

    BOOL InOrder(LISTELEM *a, LISTELEM *b);
    BOOL EquivExists(LISTELEM *p);
};

CList::~CList()
{

    //
    // We should have removed all elements in the list
    // before the list object was destroyed.
    //

    ASSERT(Head == NULL);
}

BOOL CList::InOrder(LISTELEM *a, LISTELEM *b)
{
    //
    // Sort on color depth...
    //

    if (a->value->dmBitsPerPel != b->value->dmBitsPerPel)
    {
        return (a->value->dmBitsPerPel < b->value->dmBitsPerPel);
    }
    else
    {
        //
        // ...then on resolution (area)...
        //

        if ((a->value->dmPelsWidth * a->value->dmPelsHeight) !=
            (b->value->dmPelsWidth * b->value->dmPelsHeight))
        {
            return ((a->value->dmPelsWidth * a->value->dmPelsHeight) <
                    (b->value->dmPelsWidth * b->value->dmPelsHeight));
        }
        else
        {
            //
            // ...then on frequency
            //

            if (a->value->dmDisplayFrequency != b->value->dmDisplayFrequency)
            {
                return (a->value->dmDisplayFrequency < b->value->dmDisplayFrequency);
            }
        }
    }

    return TRUE;
}

void CList::Insert(LPDEVMODE p)
{
    LISTELEM *pElem;
    LISTELEM *pCurrElem;

    pElem = (LISTELEM *) LocalAlloc(LPTR, sizeof(LISTELEM));
    pElem->value = (LPDEVMODE) LocalAlloc(LPTR, sizeof(DEVMODE) + p->dmDriverExtra);

    memcpy(pElem->value, p, sizeof(DEVMODE) + p->dmDriverExtra);

    if (!Head || InOrder(pElem, Head))
    {
        pElem->next = Head;
        Head = pElem;
    }
    else
    {
        pCurrElem = Head;

        while (pCurrElem->next && !InOrder(pElem, pCurrElem->next))
        {
            pCurrElem = pCurrElem->next;
        }

        pElem->next = pCurrElem->next;
        pCurrElem->next = pElem;
    }
}

LPDEVMODE CList::Pop()
{
    LISTELEM *pCurrElem;
    LPDEVMODE ret;

    if (Head)
    {
        pCurrElem = Head;
        Head = Head->next;

        ret = pCurrElem->value;
        LocalFree(pCurrElem);

        return ret;
    }
    else
    {
        return NULL;
    }
}

BOOL CList::EquivExists(LISTELEM *p)
{
    LISTELEM *pCurrElem = Head;

    //
    // We are only looking for equivalent 8bpp modes, so stop
    // looking once we've passed the 8bpp modes.
    //

    while (pCurrElem && (pCurrElem->value->dmBitsPerPel <= 8))
    {
        if ((p != pCurrElem) &&  // A will not be considered equivalent to A
            (p->value->dmPelsWidth  == pCurrElem->value->dmPelsWidth &&
             p->value->dmPelsHeight == pCurrElem->value->dmPelsHeight))
        {
            return TRUE;
        }

        pCurrElem = pCurrElem->next;
    }

    return FALSE;
}

//
// Lets perform the following operations on the list
//
// (1) Remove identical modes
// (2) Remove 16 color modes for which there is a 256
//     color equivalent.
// (3) Remove modes with any dimension less than 640x480
//

void CList::Process()
{
    LISTELEM *pCurrElem;
    LISTELEM *pPrevElem;
    LISTELEM *dup;
    BOOL bDisplay4BppModes = FALSE;
    BOOL bDisplaySafeModes = FALSE;

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

    if (bpp <= 4 && w <= 800 && h <= 600)
       bDisplaySafeModes = TRUE;
#endif

    pCurrElem = Head;

    while (pCurrElem)
    {
        //
        // If any of the following conditions are true, then we want to
        // remove the current mode.
        //

        if (((pCurrElem->next) &&
             (pCurrElem->value->dmBitsPerPel == pCurrElem->next->value->dmBitsPerPel) &&
             (pCurrElem->value->dmPelsWidth  == pCurrElem->next->value->dmPelsWidth)  &&
             (pCurrElem->value->dmPelsHeight == pCurrElem->next->value->dmPelsHeight) &&
             (pCurrElem->value->dmDisplayFrequency == pCurrElem->next->value->dmDisplayFrequency))
          ||
            ((pCurrElem->value->dmBitsPerPel == 4) &&
             (!bDisplay4BppModes || !(pCurrElem->value->dmFields & DM_POSITION)) &&
             (EquivExists(pCurrElem)))
          ||
            (bDisplaySafeModes &&
              (pCurrElem->value->dmPelsWidth * pCurrElem->value->dmPelsHeight *
               pCurrElem->value->dmBitsPerPel) > 1024*768*8)
          ||
            (pCurrElem->value->dmPelsHeight < 480)
          ||
            (pCurrElem->value->dmPelsWidth < 640))
        {
            dup = pCurrElem;

            if (pCurrElem == Head)
            {
                Head = pCurrElem->next;
            }
            else
            {
                pPrevElem->next = pCurrElem->next;
            }

            pCurrElem = pCurrElem->next;

            LocalFree(dup->value);
            LocalFree(dup);
        }
        else
        {
            pPrevElem = pCurrElem;
            pCurrElem = pCurrElem->next;
        }
    }
}


int CDEVMODEIDAT::Index(int i1, int i2) {

    PCDEVMODEIDAT pcdmi = this;
    int i = 0;

    while (pcdmi) {

        if (pcdmi->iRepData1 == i1 && pcdmi->iRepData2 == i2) {

            return i;

        } else {

            i += 1;
            pcdmi = pcdmi->pcdmiNext;

        }
    }

    return -1;
}

BOOL CDEVMODEIDAT::GetData(int iOrd, int *pi1, int *pi2) {
    if (iOrd == 0) {
        *pi1 = this->iRepData1;
        *pi2 = this->iRepData2;
        return TRUE;
    } else if (this->pcdmiNext != NULL)
        return this->pcdmiNext->GetData(iOrd - 1, pi1, pi2);
    else {
        return FALSE;
    }
}


PCDEVMODEIDAT CDEVMODEIDAT::Insert(int i1, int i2) {

    PCDEVMODEIDAT pcdmi = NULL;
    PCDEVMODEIDAT pcdmiNext = this;

    //
    // Try to find the entry
    //

    while (1) {

        if ( (pcdmiNext == NULL) ||
             (pcdmiNext->iRepData1 > i1) ||
             ((pcdmiNext->iRepData1 == i1) && (pcdmiNext->iRepData2 > i2)) ) {

            break;

        }

        pcdmi = pcdmiNext;
        pcdmiNext = pcdmiNext->pcdmiNext;

    }

    //
    // If we did not find it, add it to the list.
    //

    if (pcdmi == NULL || pcdmi->iRepData1 != i1 || pcdmi->iRepData2 != i2) {

        PCDEVMODEIDAT pcdmiNew = new CDEVMODEIDAT(i1, i2);

        if (pcdmiNew == NULL)
            return NULL;

        if (pcdmi == NULL) {
            pcdmiNew->pcdmiNext = this;
            return pcdmiNew;

        } else {

            pcdmiNew->pcdmiNext = pcdmi->pcdmiNext;
            pcdmi->pcdmiNext = pcdmiNew;
            return this;
        }
    }

    return NULL;
}


int CDEVMODEINDEX::MapRepresentationToOrdinal(int i1, int i2) {
    if (pcdmiHead == NULL)
        return -1;

    return pcdmiHead->Index(i1, i2);
}

BOOL CDEVMODEINDEX::MapOrdinalToRepresentation(int iOrd, int *pi1, int *pi2) {
    if (pcdmiHead == NULL)
        return FALSE;

    return pcdmiHead->GetData(iOrd, pi1, pi2);
}


int CDEVMODEINDEX::AddIndex(int i1, int i2) {
    PCDEVMODEIDAT pcdmi;

    if (pcdmiHead == NULL) {

        pcdmiHead = new CDEVMODEIDAT(i1, i2);

    } else {

        pcdmi = pcdmiHead->Insert(i1, i2);

        if (pcdmi == NULL)
            return 0;

        if (pcdmi != pcdmiHead)
            pcdmiHead = pcdmi;

    }

    return 1;
}


//
// FindClosestMode methods
//
// DON'T PANIC!!!!
//
// C++ lets you over load functions.  It figures out which one to call
// based on the parameters you pass it.
//
// These methods find the closest matching devmode in the matrix.  You
// pass a pointer for the two parameters you are trying to find, and
// an int for the one you want to remain the same.
// (Rather PROLOGish, isn't it?)
//
BOOL CDEVMODELST::FindClosestMode(int iRes, int *piClr, int *piFreq) {
    int iClr, iFreq;

    /*
     * Try lower colors until we find one that will work
     */
    for (iClr = *piClr; iClr >= 0; iClr--) {

        /* Try lower frequencies first, incase the monitor can't handle
         * higher ones.
         */
        for (iFreq = *piFreq; iFreq >= 0; iFreq--) {
            if(LookUp(iRes, iClr, iFreq)) {
                *piFreq = iFreq;
                *piClr = iClr;
                return TRUE;
            }
        }

        /* Couldn't find lower freq, try higher */
        for (iFreq = *piFreq;  iFreq  < this->cFreq; iFreq++) {
            if(LookUp(iRes, iClr, iFreq)) {
                *piFreq = iFreq;
                *piClr = iClr;
                return TRUE;
            }
        }
    }


    /*
     * Couldn't find a lower clr mode, check for higher
     */
    for (iClr = *piClr; iClr < this->cClr; iClr++) {
        /* Try lower frequencies first, incase the monitor can't handle
         * higher ones.
         */
        for (iFreq = *piFreq; iFreq >= 0; iFreq--) {
            if(LookUp(iRes, iClr, iFreq)) {
                *piFreq = iFreq;
                *piClr = iClr;
                return TRUE;
            }
        }

        /* Couldn't find lower freq, try higher */
        for (iFreq = *piFreq;  iFreq  < this->cFreq; iFreq++) {
            if (LookUp(iRes, iClr, iFreq)) {
                *piFreq = iFreq;
                *piClr = iClr;
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL CDEVMODELST::FindClosestMode(int *piRes, int iClr, int *piFreq ) {
    int iRes, iFreq;

    /*
     * Try lower resolutions until we find one that will work
     */
    for(iRes = *piRes; iRes >= 0; iRes-- ) {

        /* Try lower frequencies first, incase the monitor can't handle
         * higher ones.
         */
        for(iFreq = *piFreq; iFreq >= 0; iFreq-- ) {
            if(LookUp(iRes, iClr, iFreq )) {
                *piFreq = iFreq;
                *piRes = iRes;
                return TRUE;
            }
        }

        /* Couldn't find lower freq, try higher */
        for(iFreq = *piFreq;  iFreq  < this->cFreq; iFreq++ ) {
            if(LookUp(iRes, iClr, iFreq )) {
                *piFreq = iFreq;
                *piRes = iRes;
                return TRUE;
            }
        }
    }


    /*
     * Couldn't find a lower res mode, check for higher
     */
    for(iRes = *piRes; iRes < this->cRes; iRes++ ) {
        /* Try lower frequencies first, incase the monitor can't handle
         * higher ones.
         */
        for(iFreq = *piFreq; iFreq >= 0; iFreq-- ) {
            if(LookUp(iRes, iClr, iFreq )) {
                *piFreq = iFreq;
                *piRes = iRes;
                return TRUE;
            }
        }

        /* Couldn't find lower freq, try higher */
        for(iFreq = *piFreq;  iFreq  < this->cFreq; iFreq++ ) {
            if(LookUp(iRes, iClr, iFreq )) {
                *piFreq = iFreq;
                *piRes = iRes;
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL CDEVMODELST::FindClosestMode(int *piRes, int *piClr, int iFreq ) {
    int iRes, iClr;

    /*
     * Try lower resolutions until we find one that will work
     */
    for(iRes = *piRes; iRes >= 0; iRes-- ) {

        /* Try lower Colors first, incase the adaptor can't handle
         * higher ones.
         */
        for(iClr = *piClr; iClr >= 0; iClr-- ) {
            if(LookUp(iRes, iClr, iFreq )) {
                *piClr = iClr;
                *piRes = iRes;
                return TRUE;
            }
        }

        /* Couldn't find lower color, try higher */
        for(iClr = *piClr; iClr < this->cClr; iClr++ ) {
            if(LookUp(iRes, iClr, iFreq )) {
                *piClr = iClr;
                *piRes = iRes;
                return TRUE;
            }
        }
    }


    /*
     * Couldn't find a lower res mode, check for higher
     */
    for(iRes = *piRes; iRes < this->cRes; iRes++ ) {
        /* Try lower colors first, incase the adaptor can't handle
         * higher ones.
         */
        for(iClr = *piClr; iClr >= 0; iClr-- ) {
            if(LookUp(iRes, iClr, iFreq )) {
                *piClr = iClr;
                *piRes = iRes;
                return TRUE;
            }
        }

        /* Couldn't find lower color, try higher */
        for(iClr = *piClr; iClr < this->cClr; iClr++ ) {
            if(LookUp(iRes, iClr, iFreq )) {
                *piClr = iClr;
                *piRes = iRes;
                return TRUE;
            }
        }
    }

    return FALSE;
}



BOOL CDEVMODELST::ReconstructModeMatrix()
{
    DWORD size;
    //
    // If the cube is empty, return failiure.
    //

    if ((size = cRes * cClr * cFreq) == 0)
    {
        WarnMsg(TEXT("ReconstructModeList Failed!!!!"), TEXT(""));
        return FALSE;
    }
    pcdmArray = new PCDEVMODE[size];

    if (pcdmArray == NULL) {

        return FALSE;

    } else {

        PCDEVMODE pcdm, *pArray;
        LPDEVMODE lpdm;
        int iRes, iClr, iFreq;

        ZeroMemory(pcdmArray, sizeof(PCDEVMODE) * size);

        for (pcdm = cdmHead.NextDevMode();
             pcdm != NULL;
             pcdm = pcdm->NextDevMode()) {

            lpdm = pcdm->GetData();

            iRes = cdmiRes.MapRepresentationToOrdinal(lpdm->dmPelsWidth,
                    lpdm->dmPelsHeight);
            iClr = cdmiClr.MapRepresentationToOrdinal(lpdm->dmBitsPerPel, 0);
            iFreq = cdmiFreq.MapRepresentationToOrdinal(lpdm->dmDisplayFrequency, 0);

            //
            // Save the pointer in the right index in the array.
            // NOTE:
            // If we have a pretested mode, it will show up first in the list
            // of modes - so we must make sure we do not overwrite that entry
            // with a duplicate further on in the list.
            //

            pArray = pcdmArray + (((iRes * cClr) + iClr) * cFreq) + iFreq;

            if (!*pArray) {
                *pArray = pcdm;
            }

        }

        return TRUE;

    }
}

//
// BuildList
//
//  Enumerates all the device modes and puts them into the list.
//  This method must be called before other methods in DEVMODELST.
//
//  (I suppose I should have made this the constructor for the DEVMODELST
//  class, but I wanted it to return a value).
//
BOOL CDEVMODELST::BuildList(LPTSTR pszDevName, HWND hwnd )
{
    BYTE devmode[sizeof(DEVMODE) + 0xFFFF];
    LPDEVMODE pdevmode = (LPDEVMODE) devmode;
    DWORD i;
    CList list;

#if 0

    KdPrint(("Display.cpl: Root of linked list of modes = %08lx\n", &cdmHead));

#endif

    //
    // Lets generate a list with all the possible modes.  Then we'll
    // remove 16 color modes for which we have an exact 256 color
    // duplicate.
    //

    ZeroMemory(pdevmode,sizeof(DEVMODE));
    pdevmode->dmSize = sizeof(DEVMODE);
    pdevmode->dmDriverExtra = 0xFFFF;

    i = 0;

    while (EnumDisplaySettingsEx(pszDevName, i++, pdevmode, 0))
    {
        list.Insert(pdevmode);
        pdevmode->dmDriverExtra = 0xFFFF;
    }

    list.Process();

    while (pdevmode = list.Pop())
    {
        AddDevMode(pdevmode);
    }

    return ReconstructModeMatrix();
}

//
// AddDevMode method
//
//  This method builds the index lists for the matrix.  There is one
//  index list for each axes of the three dimemsional matrix of device
//  modes.
//
// The entry is also automatically added to the linked list of modes if
// it is not alreay present in the list.
//

void CDEVMODELST::AddDevMode(LPDEVMODE lpdm)
{
    //
    // !!!
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
        (lpdm->dmFields & ~(DM_BITSPERPEL   |
                            DM_PELSWIDTH    |
                            DM_PELSHEIGHT   |
                            DM_DISPLAYFLAGS |
                            DM_DISPLAYFREQUENCY)))
    {
        SetDriverBadData();
    }

    //
    // Set the height for the test of the 1152 mode
    //

    if (lpdm->dmPelsWidth == 1152) {

        Set1152Mode(lpdm->dmPelsHeight);
    }

    //
    // The mode was not in the list, so add it.
    //
    cdmHead.AddElement(lpdm);

    cRes += cdmiRes.AddIndex(lpdm->dmPelsWidth, lpdm->dmPelsHeight);
    cClr += cdmiClr.AddIndex(lpdm->dmBitsPerPel, 0);
    cFreq += cdmiFreq.AddIndex(lpdm->dmDisplayFrequency, 0);
}


/*****************************************************************\
*
* CREGVIDOBJ class
*
*        Class to access the video part of the registry
*
\*****************************************************************/
//
// CREGVIDOBJ constructor
//
CREGVIDOBJ::CREGVIDOBJ(LPTSTR pszDisplay):
    hkServiceReg(NULL), hkVideoRegR(NULL), hkVideoRegW(NULL), pszDrvName(NULL),
    pszKeyName(NULL), bdisablable(FALSE) {

    TCHAR szName[MAX_PATH];
    TCHAR szPath[MAX_PATH];
    TCHAR szGroup[MAX_PATH];
    LPTSTR pszPath;
    HKEY hkeyMap;
    int i;
    DWORD cb;

    LPTSTR pszName = NULL;

    //
    // map the driver's symbolic link into an NT name
    //

    pszDisplay = SubStrEnd(SZ_BACKBACKDOT, pszDisplay);

    if (QueryDosDevice(pszDisplay, szName, MAX_PATH) == 0) {
        return;
    }

    //
    // get the video part of the device map
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_VIDEOMAP,
                     0,
                     KEY_READ,
                     &hkeyMap) !=  ERROR_SUCCESS) {
        return;
    }

    //
    // parse the device map and open the registry.
    //

    //Reg functions deal with bytes, not chars
    cb = sizeof(szPath);

    i = RegQueryValueEx(hkeyMap,
                        szName,
                        NULL,
                        NULL,
                        (LPBYTE)szPath,
                        &cb);

    RegCloseKey(hkeyMap);

    if (i != ERROR_SUCCESS) {

        return;

    }

    //
    // At this point, szPath has something like:
    //  \REGISTRY\Machine\System\ControlSet001\Services\Jazzg300\Device0
    //
    // To use the Win32 registry calls, we have to strip off the \REGISTRY
    // and convert \Machine to HKEY_LOCAL_MACHINE
    //

    hkeyMap = HKEY_LOCAL_MACHINE;
    pszPath = SubStrEnd(SZ_REGISTRYMACHINE, szPath);

    //
    // try to open the registry key.
    // Get Read access and Write access seperately so we can query stuff
    // when an ordinary user is logged on.
    //

    if (RegOpenKeyEx(hkeyMap,
                     pszPath,
                     0,
                     KEY_READ,
                     &hkVideoRegR) != ERROR_SUCCESS) {

        hkVideoRegR = 0;

    }

    if (RegOpenKeyEx(hkeyMap,
                     pszPath,
                     0,
                     KEY_WRITE,
                     &hkVideoRegW) != ERROR_SUCCESS) {

        hkVideoRegW = 0;

    }

    //
    // Save the mini port driver name
    //

    {
        LPTSTR pszEnd;
        HKEY hkeyDriver;

        pszEnd = pszPath + lstrlen(pszPath);

        //
        // Remove the \DeviceX at the end of the path
        //

        while (pszEnd != pszPath && *pszEnd != TEXT('\\')) {

            pszEnd--;
        }

        *pszEnd = UNICODE_NULL;

        //
        // First check if their is a binary name in there that we should use.
        //

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         pszPath,
                         0,
                         KEY_READ,
                         &hkeyDriver) ==  ERROR_SUCCESS) {

            //
            // parse the device map and open the registry.
            //

            cb = sizeof(szName);

            if (RegQueryValueEx(hkeyDriver,
                                TEXT("ImagePath"),
                                NULL,
                                NULL,
                                (LPBYTE)szName,
                                &cb) == ERROR_SUCCESS) {

                //
                // The is a binary.
                // extract the name, which will be of the form ...\driver.sys
                //

                {

                    LPTSTR pszDriver, pszDriverEnd;

                    pszDriver = szName;
                    pszDriverEnd = pszDriver + lstrlen(pszDriver);

                    while(pszDriverEnd != pszDriver &&
                          *pszDriverEnd != TEXT('.')) {
                        pszDriverEnd--;
                    }

                    *pszDriverEnd = UNICODE_NULL;

                    while(pszDriverEnd != pszDriver &&
                          *pszDriverEnd != TEXT('\\')) {
                        pszDriverEnd--;
                    }

                    pszDriverEnd++;

                    //
                    // If pszDriver and pszDriverEnd are different, we now
                    // have the driver name.
                    //

                    if (pszDriverEnd > pszDriver) {

                        pszName = pszDriverEnd;

                    }
                }
            }

            //
            // while we have this key open, determine if the driver is in
            // the Video group (to know if we can disable it automatically.
            //

            cb = sizeof(szName);

            if (RegQueryValueEx(hkeyDriver,
                                TEXT("Group"),
                                NULL,
                                NULL,
                                (LPBYTE)szGroup,
                                &cb) == ERROR_SUCCESS) {

                //
                // Compare the string , case insensitive, to the "Video" group.
                //

                bdisablable = !(BOOL)(lstrcmpi(szGroup, TEXT("Video")));

            }

            RegCloseKey(hkeyDriver);
        }

        while(pszEnd > pszPath && *pszEnd != TEXT('\\')) {
            pszEnd--;
        }
        pszEnd++;

        //
        // Save the key name
        //

        this->pszKeyName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT,
                                              (lstrlen(pszEnd) + 1) * sizeof(TCHAR));

        if (this->pszKeyName != NULL) {

            CopyMemory(this->pszKeyName, pszEnd, lstrlen(pszEnd) * sizeof(TCHAR));

        }

        //
        // something failed trying to get the binary name.
        // just get the device name
        //

        if (!pszName) {

            pszDrvName = pszKeyName;

        } else {

            this->pszDrvName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT,
                                                  (lstrlen(pszName) + 1) * sizeof(TCHAR));

            if (this->pszDrvName != NULL) {

                CopyMemory(this->pszDrvName, pszName, lstrlen(pszName) * sizeof(TCHAR));

            }
        }
    }

    //
    // Finally, Get a "write" handle to the service Key so we can disable the
    // driver at a later time.
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     pszPath,
                     0,
                     KEY_READ | KEY_WRITE,
                     &hkServiceReg) !=  ERROR_SUCCESS) {

        this->hkServiceReg = NULL;
        bdisablable = FALSE;

    }


}

//
// CREGVIDOBJ destructor
//
//  (gets called whenever a CREGVIDOBJ is destroyed or goes out of scope)
//

CREGVIDOBJ::~CREGVIDOBJ() {

    //
    // Close the registry
    //

    if (hkVideoRegW) {
        RegCloseKey(hkVideoRegW);
    }

    if (hkVideoRegR) {
        RegCloseKey(hkVideoRegR);
    }

    if (hkServiceReg) {
        RegCloseKey(hkServiceReg);
    }

    //
    // Free the strings
    //

    if (pszDrvName) {
        LocalFree(pszDrvName);
    }
}

//
// Method to get the hardware information fields.
//

VOID CREGVIDOBJ::GetHardwareInformation(
    PHARDWARE_INFO pInfo)
{

    DWORD cb, dwType;
    LPTSTR psz;
    DWORD i;
    LONG lRet;
    TCHAR pszTmp[MAX_PATH];

    LPTSTR pKeyNames[5] = {
        TEXT("HardwareInformation.MemorySize"),
        TEXT("HardwareInformation.ChipType"),
        TEXT("HardwareInformation.DacType"),
        TEXT("HardwareInformation.AdapterString"),
        TEXT("HardwareInformation.BiosString")
    };

    ZeroMemory(pInfo, sizeof(HARDWARE_INFO));

    //
    // Query each entry one after the other.
    //

    for (i = 0; i < 5; i++) {

        //
        // query the size of the string
        //

        cb = 0;
        lRet = RegQueryValueEx(hkVideoRegR,
                               pKeyNames[i],
                               NULL,
                               &dwType,
                               NULL,
                               &cb);

        //
        // check to see if there is a string, and the string is more than just
        // a UNICODE_NULL (detection will put an empty string there).
        //

        psz = NULL;

        if (lRet == ERROR_SUCCESS || lRet == ERROR_MORE_DATA) {

            if (i == 0) {

                ULONG mem;

                if (RegQueryValueEx(hkVideoRegR,
                                    pKeyNames[i],
                                    NULL,
                                    &dwType,
                                    (PUCHAR) (&mem),
                                    &cb) == ERROR_SUCCESS) {

                    //
                    // If we queried the memory size, we actually have
                    // a DWORD.  Transform the DWORD to a string
                    //

                    // Divide down to Ks
                    mem =  mem >> 10;

                    // if a MB multiple, divide again.

                    if ((mem & 0x3FF) != 0) {

                        psz = FmtSprint( MSG_SETTING_KB, mem );

                    } else {

                        psz = FmtSprint( MSG_SETTING_MB, mem >> 10 );

                    }
                }

            } else {

                //
                // alloc a string buffer
                //

                psz = (LPTSTR)LocalAlloc(LPTR, cb);

                if (psz) {

                    //
                    // get the string
                    //

                    if (RegQueryValueEx(hkVideoRegR,
                                        pKeyNames[i],
                                        NULL,
                                        &dwType,
                                        (LPBYTE)psz,
                                        &cb) != ERROR_SUCCESS) {

                        LocalFree(psz);
                        psz = NULL;

                    }
                }
            }
        }

        if (psz == NULL) {

            //
            // Put in the default string
            //

            LoadString(hInstance,
                       IDS_UNAVAILABLE,
                       pszTmp,
                       sizeof(pszTmp));

            cb = lstrlen(pszTmp) * sizeof(TCHAR);

            psz = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cb + sizeof(TCHAR));

            if (psz) {

                CopyMemory(psz, pszTmp, cb);

            }
        }

        *(((LPTSTR *)pInfo) + i) = psz;
    }


    return;
}

BOOL CREGVIDOBJ::EnableDriver(BOOL Enable) {

    DWORD dw;

    //
    // just return TRUE if it is not a driver we want to disable
    // i.e. it is not in the group "Video"
    //

    if (!bdisablable) {

        return TRUE;

    }

    //
    // actually try to disable other drivers
    //

    if (Enable)
    {
        dw = SERVICE_SYSTEM_START;
    }
    else
    {
        dw = SERVICE_DISABLED;
    }


    if (hkServiceReg) {

        return ((RegSetValueEx(this->hkServiceReg,
                               TEXT("Start"),
                               NULL,
                               REG_DWORD,
                               (LPBYTE) &dw,
                               sizeof(DWORD))) == ERROR_SUCCESS);

    } else {

        return FALSE;

    }
}

//
// Sets the error control to whteve the driver wants - ERROR_NORMAL
// it what we will probably set it to.
//

BOOL CREGVIDOBJ::SetErrorControlNormal(void) {

    DWORD dw = SERVICE_ERROR_NORMAL;

    //
    // just return TRUE if it is not a driver we want to report errors for
    // i.e. it is not in the group "Video"
    //

    if (!bdisablable) {

        return TRUE;

    }


    //
    // change the ErrorControl value in the registry.
    //

    if (hkServiceReg) {

        return ((RegSetValueEx(this->hkServiceReg,
                               TEXT("ErrorControl"),
                               NULL,
                               REG_DWORD,
                               (LPBYTE) &dw,
                               sizeof(DWORD))) == ERROR_SUCCESS);

    } else {

        return FALSE;

    }
}


//
// returns the display drivers
//

LPTSTR CREGVIDOBJ::CloneDisplayFileNames(BOOL bPreprocess) {
    DWORD cb, dwType;
    LPTSTR psz, pszName, tmppsz;
    LONG lRet;
    DWORD cNumStrings;

    //
    // query the size of the string
    //

    cb = 0;

    lRet = RegQueryValueEx(hkVideoRegR,
                           SZ_INSTALLEDDRIVERS,
                           NULL,
                           &dwType,
                           NULL,
                           &cb);

    if (lRet != ERROR_SUCCESS && lRet != ERROR_MORE_DATA) {

        return NULL;

    }

    //
    // alloc a string buffer
    //

    psz = (LPTSTR)LocalAlloc(LPTR, cb);

    if (psz) {

        //
        // get the string
        //

        if (RegQueryValueEx(hkVideoRegR,
                            SZ_INSTALLEDDRIVERS,
                            NULL,
                            &dwType,
                            (LPBYTE)psz,
                            &cb) != ERROR_SUCCESS) {

            LocalFree(psz);
            return NULL;

        }

        //
        // If the caller want a preprocessed list, we will add the commas,
        // remove the NULLs, etc.
        //

        if (bPreprocess) {

            //
            // if it is a multi_sz, count the number of sub strings.
            //

            if (dwType == REG_MULTI_SZ) {

                tmppsz = psz;
                cNumStrings = 0;

                while(*tmppsz) {

                    while(*tmppsz++);
                    cNumStrings++;

                }

            } else {

                cNumStrings = 1;

            }

            //
            // the buffer must contain enought space for :
            // the miniport name,
            // the .sys extension,
            // all the display driver names,
            // the .dll extension for each of them.
            // and place for ", " between each name
            // we forget about NULL, so our buffer is a bit bigger.
            //

            cb = lstrlen(this->GetMiniPort()) +
                 cb +
                 lstrlen(SZ_DOTSYS) +
                 cNumStrings * (lstrlen(SZ_DOTDLL) + lstrlen(SZ_FILE_SEPARATOR));


            pszName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cb * sizeof(TCHAR));

            if (pszName != NULL) {

                lstrcpy(pszName, this->GetMiniPort());
                lstrcat(pszName, SZ_DOTSYS);

                tmppsz = psz;

                while (cNumStrings--) {

                    lstrcat(pszName, SZ_FILE_SEPARATOR);
                    lstrcat(pszName, tmppsz);
                    lstrcat(pszName, SZ_DOTDLL);

                    while (*tmppsz++);
                }
            }

            LocalFree(psz);
            psz = pszName;
        }
    }

    //
    // return it to the caller
    //

    return psz;

}

/*****************************************************************\
*
* CDIALOG class
*
\*****************************************************************/
//
// Dialog method
//
//  Creates the dialog box (shows it on the screen)
//

int CDIALOG::Dialog(HINSTANCE hmod, HWND hwndCallerParent, LPARAM lInitParam) {

    this->hwndParent = hwndCallerParent;
    this->hmodModule = hmod;
    this->lParam = lInitParam;

#if 0
    return DialogBoxParam(hmod, MAKEINTRESOURCE(this->iDlgResID),
            this->hwndParent, (DLGPROC)::CDialogProc, (LONG)this);
#else
    {
    int i;

    i = DialogBoxParam(hmod, MAKEINTRESOURCE(this->iDlgResID),
            this->hwndParent, (DLGPROC)::CDialogProc, (LONG)this);
    GetLastError();
    return i;
    }
#endif
}

#if RESIZE_PROBLEM
LRESULT WINAPI MyProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_WINDOWPOSCHANGING)
    {
        RECT  r;
        ULONG currHeight, newHeight;
        LPWINDOWPOS lpWinPos = (LPWINDOWPOS)lParam;

        GetWindowRect(hwnd, &r);

        currHeight = r.bottom - r.top;
        newHeight = lpWinPos->cy;

        if ( (!(lpWinPos->flags & SWP_NOSIZE)) &&
             (currHeight != newHeight)         &&
             (newHeight < 350))
        {
#if DBG
            CHAR buffer[256];

            sprintf(buffer, "!! IMPORTANT !!\n\n"
                            "We are trying to track down a bug in the system "
                            "where occasionally this display applet has its size "
                            "changed on the fly.  We think that is about to happen "
                            "on your machine!  Please contact ErickS, or AndreVa.\n\n"
                            "The current height is: %d\n"
                            "The new height is: %d",
                            currHeight,
                            newHeight);

            MessageBoxA(hwnd, buffer, "NOTE: Please contact ErickS!",
                       MB_ICONSTOP | MB_OK);

#else

            //
            // Work around the problem by setting the SWP_NOSIZE
            // flag.  Now our window size won't change.
            //

            ((LPWINDOWPOS)lParam)->flags |= SWP_NOSIZE;
#endif
        }

        return 0;

    }
    else
    {
        return gOldProc(hwnd, uMsg, wParam, lParam);
    }
}
#endif

//
// WndProc dialog
//
//  cracks the messages and dispatches them to the virtual message methods
//
BOOL CDIALOG::WndProc(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {

    case WM_INITDIALOG:

#if RESIZE_PROBLEM
        //
        // Lets try to subclass our parent window at this time. Make
        // sure we only do this once!!
        //

        if (gOldProc == NULL)
        {
            gOldProc = (WNDPROC) GetWindowLong(this->hwndParent, GWL_WNDPROC);
            SetWindowLong(this->hwndParent, GWL_WNDPROC, (LONG)MyProc);
        }
#endif

        return this->InitDlg((HWND)wParam);

    case WM_COMMAND:
        return this->DoCommand((int)LOWORD(wParam), (HWND)lParam,
                (int)HIWORD(wParam));

    case WM_NOTIFY:
        return this->DoNotify((int)wParam, (NMHDR *)lParam, ((NMHDR *)lParam)->code );

    case WM_HSCROLL:
        return this->HScroll(GetDlgCtrlID((HWND)lParam), LOWORD(wParam),
                HIWORD(wParam));

    case WM_PAINT:
        return this->Paint();

    case WM_TIMER:
        return this->TimerTick((int)wParam);

    case WM_DESTROY:

#if RESIZE_PROBLEM
        //
        // We should restore our parents original WndProc before returning...
        //

        if (gOldProc)
        {
            SetWindowLong(this->hwndParent, GWL_WNDPROC, (LONG)gOldProc);
            gOldProc = NULL;
        }
#endif

        return this->OnDestroy();

    case MSG_DSP_SETUP_MESSAGE:
        return this->InitMessage();

    case WM_SYSCOLORCHANGE:
    case WM_DISPLAYCHANGE:
        return this->SysColorChange();

    case WM_HELP:
        return this->DoHelp( (LPHELPINFO) lParam );

    case WM_CONTEXTMENU:
        return this->DoContextMenu( (HWND)wParam, LOWORD(lParam), HIWORD(lParam) );

    default:

        return FALSE;
    }
}

BOOL CDIALOG::DoHelp( LPHELPINFO lphi ) {

    WinHelp((HWND) lphi->hItemHandle, NULL, HELP_WM_HELP, (DWORD) g_aiSetHelpIds);
    return TRUE;
}

BOOL CDIALOG::DoContextMenu( HWND hwnd, WORD xPos, WORD yPos ) {

    WinHelp((HWND)hwnd, NULL, HELP_CONTEXTMENU, (DWORD) g_aiSetHelpIds);
    return TRUE;
}

/*****************************************************************\
*
* CDLGMODELIST class
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDLGMODELIST : public CDIALOG {

public:
    CDLGMODELIST();

    void AddMode(LPDEVMODE lpdm);

    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
    VOID CDLGMODELIST::RecurseList(PCDEVMODE pcdevCurrent);
};

typedef CDLGMODELIST *PCDLGMODELIST;

CDLGMODELIST::CDLGMODELIST() {
    this->iDlgResID = DLG_SET_MODE_LIST;
}

BOOL CDLGMODELIST::InitDlg(HWND hwndFocus) {

    PCDEVMODE pcModeList = (*(PCDEVMODELST *)(this->lParam))->GetDevmodeList();

    //
    // The first entry in the list is NULL, so ignore it.
    //

    RecurseList(pcModeList->NextDevMode());

    return TRUE;
}


VOID CDLGMODELIST::RecurseList(PCDEVMODE pcdevCurrent) {

    LPTSTR lpsz;
    int i;
    DWORD id;
    LPDEVMODE lpdm = pcdevCurrent->GetData();

    //
    // Recursive call till we hit the end of the list.
    //

    if ((pcdevCurrent->NextDevMode()) != NULL) {

        RecurseList(pcdevCurrent->NextDevMode());

    }

    //
    // Add it to the list
    //

    if (lpdm->dmDisplayFrequency == 1) {

        if (lpdm->dmBitsPerPel < 32) {

            id = ID_DSP_TXT_COLOR_MODE_DEF_REF;

        } else {

            id = ID_DSP_TXT_TRUE_COLOR_MODE_DEF_REF;

        }

    } else if (lpdm->dmDisplayFrequency < 50) {

        if (lpdm->dmBitsPerPel < 32) {

            id = ID_DSP_TXT_COLOR_MODE_INT_REF;

        } else {

            id = ID_DSP_TXT_TRUE_COLOR_MODE_INT_REF;

        }

    } else {

        if (lpdm->dmBitsPerPel < 32) {

            id = ID_DSP_TXT_COLOR_MODE;

        } else {

            id = ID_DSP_TXT_TRUE_COLOR_MODE;

        }

    }

    lpsz = FmtSprint(id,
                     lpdm->dmPelsWidth,
                     lpdm->dmPelsHeight,
                     1 << lpdm->dmBitsPerPel,
                     lpdm->dmDisplayFrequency);

#if DBG
    {
        TCHAR pszMode[1024];

        //DLI: make it compile
        wsprintf(pszMode, TEXT("%ws%ws"), lpsz,
                 pcdevCurrent->bModeTested() ? TEXT(" - Tested") : TEXT(""));

        i = this->SendCtlMsg(ID_MODE_LIST, LB_ADDSTRING, 0, (LPARAM)pszMode);
    }
#else

    i = this->SendCtlMsg(ID_MODE_LIST, LB_ADDSTRING, 0, (LPARAM)lpsz);

#endif

    LocalFree(lpsz);

    //
    // Save the devmode pointer in the data field of the list so we can
    // retrieve it when the user selects the entry.
    //

    this->SendCtlMsg(ID_MODE_LIST, LB_SETITEMDATA, i, (LPARAM)lpdm);

    return;
}

BOOL CDLGMODELIST::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    int iSel;
    LPDEVMODE lpdm;

    switch(idControl ) {

    case ID_MODE_LIST:

        if (iNoteCode != LBN_DBLCLK) {
            break;
        }

        //
        // If we do get a double click, then fall through as an OK
        //

    case IDOK:

        //
        // get the selected driver id.
        //

        iSel = this->SendCtlMsg(ID_MODE_LIST, LB_GETCURSEL, 0, 0);

        if (iSel == LB_ERR ||
            (lpdm = (LPDEVMODE) (this->SendCtlMsg(ID_MODE_LIST,
                                                  LB_GETITEMDATA,
                                                  iSel,
                                                  0))) == NULL) {

            lpdm = NULL;

        }

        EndDialog(this->hwnd, (int)lpdm);

        break;

    case IDCANCEL:
        EndDialog(this->hwnd, 0);
        break;

    default:

        return FALSE;

    }

    return TRUE;
}
