#define WINVER 0x0400

extern "C" {

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

}

#include <windows.h>

#include <cplp.h>
#include "wingdip.h"
#include "settings.h"
#include "setinc.h"
#include "setcdcl.hxx"

extern "C" {

#include "stdio.h"
#include "stdlib.h"

#include <commctrl.h>
#include <setupapi.h>
#include <syssetup.h>

#include "shlobj.h"   // IsUserAnAdmin
#include "shellp.h"   // IsUserAnAdmin

#include "help.h"

//
// NT5 added an extra parameter to this API
//
WINUSERAPI
BOOL
WINAPI
EnumDisplayDevicesA(
    PVOID Unused,
    DWORD iDevNum,
    PDISPLAY_DEVICEA lpDisplayDevice);
WINUSERAPI
BOOL
WINAPI
EnumDisplayDevicesW(
    PVOID Unused,
    DWORD iDevNum,
    PDISPLAY_DEVICEW lpDisplayDevice);
#ifdef UNICODE
#define EnumDisplayDevices  EnumDisplayDevicesW
#else
#define EnumDisplayDevices  EnumDisplayDevicesA
#endif // !UNICODE

}

/****************************************************************\
*
* Global vars and structures used only in this file
*
\****************************************************************/

TCHAR gpszError[] = TEXT("Unknown Error");

extern "C" {

extern TCHAR szControlHlp[];
extern UINT wHelpMessage;
extern HWND hwndDevModeNotify;
}

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

#define RESIZE_PROBLEM 1

#if RESIZE_PROBLEM
LRESULT WINAPI MyProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

WNDPROC gOldProc=NULL;
#endif

HWND ghwndPropSheet;

ULONG gUnattenedConfigureAtLogon = 0;
ULONG gUnattenedInstall = 0;
TCHAR gUnattenedPszInf[512];
TCHAR gUnattenedPszOption[512];
ULONG gUnattenedBitsPerPel = 0;
ULONG gUnattenedXResolution = 0;
ULONG gUnattenedYResolution = 0;
ULONG gUnattenedVRefresh = 0;
ULONG gUnattenedFlags = 0;
ULONG gUnattenedAutoConfirm = 0;

DWORD g_aiSetHelpIds[] = {

    ID_DSP_CHANGE,      IDH_DSKTPMONITOR_CHANGE_DISPLAY,
    ID_DSP_COLORBOX,    IDH_DSKTPMONITOR_COLOR,
    ID_DSP_CLRPALGRP,   IDH_DSKTPMONITOR_COLOR,
    ID_DSP_COLORBAR,    IDH_DSKTPMONITOR_COLOR,
    ID_DSP_AREA_SB,     IDH_DSKTPMONITOR_AREA,
    ID_DSP_DSKAREAGRP,  IDH_DSKTPMONITOR_AREA,
    ID_DSP_X_BY_Y,      IDH_DSKTPMONITOR_AREA,
    ID_DSP_REFFREQGRP,  IDH_DSKTPMONITOR_REFRESH,
    ID_DSP_FREQ,        IDH_DSKTPMONITOR_REFRESH,
    ID_DSP_LIST_ALL,    IDH_DSKTPMONITOR_LIST_MODES,
    IDC_SCREENSAMPLE,   IDH_DSKTPMONITOR_MONITOR,
    ID_DSP_TEST,        IDH_DSKTPMONITOR_TEST,
    ID_DSP_FONTSIZEGRP, IDH_DSKTPMONITOR_FONTSIZE,
    ID_DSP_FONTSIZE,    IDH_DSKTPMONITOR_FONTSIZE,
    ID_ADP_ADPGRP,      IDH_DSKTPMONITOR_ADTYPE,
    ID_ADP_ADAPTOR,     IDH_DSKTPMONITOR_ADTYPE,
    ID_ADP_CHGADP,      IDH_DSKTPMONITOR_CHANGE1,
    ID_ADP_DETECT,      IDH_DSKTPMONITOR_DETECT,
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


/*****************************************************************\
*
* Pure "C" functions used only in this file
*
\*****************************************************************/
LPTSTR SubStrEnd( LPTSTR pszTarget, LPTSTR pszScan);
DWORD WINAPI ApplyNowThd( LPVOID lpThreadParameter);

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
    LISTELEM *temp;

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

    pElem = (LISTELEM *) malloc(sizeof(LISTELEM));
    pElem->value = (LPDEVMODE) malloc(sizeof(DEVMODE) + p->dmDriverExtra);

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
        free(pCurrElem);

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
// (3) Remove modes with less then 480 scan lines
//

void CList::Process()
{
    LISTELEM *pCurrElem;
    LISTELEM *pPrevElem;
    LISTELEM *dup;
    HKEY hkeyDriver;
    BOOL bDisplay4BppModes = FALSE;

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
             (!bDisplay4BppModes) &&
             (EquivExists(pCurrElem)))
          ||
            (pCurrElem->value->dmPelsHeight < 480))
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

            free(dup->value);
            free(dup);
        }
        else
        {
            pPrevElem = pCurrElem;
            pCurrElem = pCurrElem->next;
        }
    }
}

/*****************************************************************\
*
* CDEVMODE class
*
\*****************************************************************/
typedef class CDEVMODE *PCDEVMODE;

class CDEVMODE {

private:
    PCDEVMODE pcdmNext;
    BOOL bTested;
    PDEVMODE pdm;

public:
    CDEVMODE() : pdm(NULL), pcdmNext(NULL), bTested(FALSE) {};

    ~CDEVMODE() { if (pcdmNext != NULL) { free(pcdmNext->pdm); delete pcdmNext; } }

    void AddElement(LPDEVMODE pdm) {

        PCDEVMODE pcdm = new CDEVMODE;

        pcdm->pdm = pdm;
        pcdm->pcdmNext = this->pcdmNext;
        this->pcdmNext = pcdm;
    }

    PCDEVMODE NextDevMode()         { return this->pcdmNext; }

    LPDEVMODE GetData()             { return this->pdm; }

    VOID vTestMode(BOOL bSuccess)   { this->bTested = bSuccess; }
    BOOL bModeTested()              { return this->bTested; }
};

/*****************************************************************\
*
* CDEVMODEIDAT class
*
\*****************************************************************/
typedef class CDEVMODEIDAT  *PCDEVMODEIDAT;

class CDEVMODEIDAT {
private:
    PCDEVMODEIDAT  pcdmiNext;
    int iRepData1;
    int iRepData2;

public:
    CDEVMODEIDAT() : pcdmiNext(NULL), iRepData1(0), iRepData2(0) {};
    CDEVMODEIDAT(int i1, int i2) : pcdmiNext(NULL), iRepData1(i1),
            iRepData2(i2) {};

    ~CDEVMODEIDAT() { if (pcdmiNext != NULL) delete pcdmiNext; }

    int Index(int i1, int i2);
    BOOL GetData(int iOrd, int *pi1, int *pi2);
    PCDEVMODEIDAT Insert(int i1, int i2);
};

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

/*****************************************************************\
*
* CDEVMODEINDEX class
*
\*****************************************************************/
typedef class CDEVMODEINDEX *PCDEVMODEINDEX;

class CDEVMODEINDEX {
private:
    PCDEVMODEIDAT  pcdmiHead;

public:
    CDEVMODEINDEX() :  pcdmiHead(NULL) {}
    ~CDEVMODEINDEX()    { if (pcdmiHead) delete pcdmiHead; };

    int AddIndex(int i1, int i2);
    int MapRepresentationToOrdinal(int i1, int i2);
    BOOL MapOrdinalToRepresentation(int iOrd, int *pi1, int *pi2);
};

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

/*****************************************************************\
*
* CDEVMODELST class
*
\*****************************************************************/
class CDEVMODELST {
private:
    int cRes;
    int cClr;
    int cFreq;

    //
    // The cdmHead is the linked list of valid mode.
    // The pcdmArray is a cube of pointers to the appropriate CDEVMODE
    // structure.
    // If the pointer is valid, it indicates there is such a mode; otherwise,
    // if the pointer is NULL, there is no such mode.
    //

    CDEVMODE cdmHead;
    PCDEVMODE *pcdmArray;

    CDEVMODEINDEX cdmiRes;
    CDEVMODEINDEX cdmiClr;
    CDEVMODEINDEX cdmiFreq;


    //
    // No driver should return frequencies of 0 anymore. Use 1 for
    // hardware default.
    //

    BOOL bBadData;


public:
    CDEVMODELST() : cRes(0), cClr(0), cFreq(0), pcdmArray(NULL),
                    bBadData(FALSE) {};

    ~CDEVMODELST() {
        delete pcdmArray;
    }

    BOOL BuildList(LPTSTR pszDevName, HWND hwnd);

    void AddDevMode(LPDEVMODE lpdm);

    PCDEVMODE LookUp(int iRes, int iClr, int iFreq)
    {
        if ((iFreq != -1) &&
            (iRes  != -1) &&
            (iClr  != -1))
        {
            return *(pcdmArray + (((iRes * cClr) + iClr) * cFreq) + iFreq);
        }
        else
        {
            return NULL;
        }
    }

    BOOL FindClosestMode(int iRes, int *iClr, int *iFreq);
    BOOL FindClosestMode(int *iRes, int iClr, int *iFreq);
    BOOL FindClosestMode(int *iRes, int *iClr, int iFreq);


    int ColorFromIndex(int iClr)  {
        int i1, i2;

        cdmiClr.MapOrdinalToRepresentation(iClr, &i1, &i2);

        return i1;
    }

    void ResXYFromIndex(int iRes, int *pcx, int *pcy) {
        cdmiRes.MapOrdinalToRepresentation(iRes, pcx, pcy);
    }

    int FreqFromIndex(int iFreq) {
        int i1, i2;

        cdmiFreq.MapOrdinalToRepresentation(iFreq, &i1, &i2);

        return i1;
    }

    int IndexFromColor(int cBitsPerPel) {
        return cdmiClr.MapRepresentationToOrdinal(cBitsPerPel, 0);
    }

    int IndexFromResXY(int cx, int cy) {
        return cdmiRes.MapRepresentationToOrdinal(cx, cy);
    }

    int IndexFromFreq(int cHz) {
        return cdmiFreq.MapRepresentationToOrdinal(cHz, 0);
    }

    int GetResCount()   {return cRes;}
    int GetClrCount()   {return cClr;}
    int GetFreqCount()   {return cFreq;}

    BOOL IsDriverBadData()        {return bBadData;}
    VOID SetDriverBadData()       {bBadData = TRUE;}

    PCDEVMODE GetDevmodeList()    {return ((PCDEVMODE)(&cdmHead));}
};

typedef CDEVMODELST *PCDEVMODELST;

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

//
// BuildList
//
//  Enumerates all the device modes and puts them into the list.
//  This method must be called before other methods in DEVMODELST.
//
//  (I suppose I should have made this the constructor for the DEVMODELST
//  class, but I wanted it to return a value).
//
BOOL CDEVMODELST::BuildList(LPTSTR pszDevName, HWND hwnd ) {

    DWORD size;
    BOOL ret = TRUE;
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

    pdevmode->dmSize = sizeof(DEVMODE);
    pdevmode->dmDriverExtra = 0xFFFF;

    i = 0;

    while (EnumDisplaySettings(pszDevName, i++, pdevmode))
    {
        list.Insert(pdevmode);
        pdevmode->dmDriverExtra = 0xFFFF;
    }

    list.Process();

    while (pdevmode = list.Pop())
    {
        AddDevMode(pdevmode);
    }

    //
    // If the cube is empty, return failiure.
    //

    if ((size = cRes * cClr * cFreq) == 0)
    {
        KdPrint(("Display.cpl: error doing enummodes %08lx\n", ret));
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
// AddDevMode method
//
//  This method builds the index lists for the matrix.  There is one
//  index list for each axes of the three dimemsional matrix of device
//  modes.
//
// The entry is also automatically added to the linked list of modes if
// it is not alreay present in the list.
//

void CDEVMODELST::AddDevMode(LPDEVMODE lpdm) {

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
* CDISPLAYOBJ class
*
\*****************************************************************/
class CDISPLAYOBJ {

protected:
    RECT    rcPos;
    HWND    hwndParent;

public:
    void Redraw()   { InvalidateRect(this->hwndParent, &(this->rcPos), FALSE); }
    void SetPosition(HWND hwnd, LPRECT lprc )
                    { hwndParent = hwnd; rcPos = *lprc; }
    virtual void Paint(HDC hdc, LPRECT lprc = NULL) = 0;
};

/*****************************************************************\
*
* CMONITOR class
*
\*****************************************************************/

extern HBITMAP FAR LoadMonitorBitmap( BOOL bFillDesktop );

class CMONITOR : public CDISPLAYOBJ {
private:
    HBITMAP hbmMonitor;
    HBITMAP hbmScrSample;
    int iOrgXRes;
    int iOrgYRes;
    int iCurXRes;
    int iCurYRes;
    BOOL fInit;

public:
    CMONITOR();
    ~CMONITOR();
    void SetScreenSize(int HRes, int VRes);
    void Paint(HDC hdc, LPRECT lprc = NULL);
    void SysColorChange( UINT msg, WPARAM wParam, LPARAM lParam );
};


CMONITOR::CMONITOR() : fInit(TRUE), iOrgXRes(800), iOrgYRes(600), iCurXRes(800), iCurYRes(600)  {
    HDC hdc;

    this->hbmScrSample = LoadMonitorBitmap( TRUE ); // let them do the desktop

    // get a base copy of the bitmap for when the "internals" change
    this->hbmMonitor = LoadMonitorBitmap( FALSE ); // we'll do the desktop

    //hdc = GetWindowDC(GetDesktopWindow());
    //this->iOrgXRes = GetDeviceCaps(hdc, HORZRES);
    //this->iOrgYRes = GetDeviceCaps(hdc, VERTRES);
    //ReleaseDC(GetDesktopWindow(), hdc);

}


CMONITOR::~CMONITOR() {
    SendDlgItemMessage(this->hwndParent, IDC_SCREENSAMPLE, STM_SETIMAGE, IMAGE_BITMAP, NULL);

    if (this->hbmScrSample)
    {
        DeleteObject(this->hbmScrSample);
        this->hbmScrSample = NULL;
    }
    if (this->hbmMonitor)
    {
        DeleteObject(this->hbmMonitor);
        this->hbmMonitor = NULL;
    }
}


void CMONITOR::SetScreenSize(int HRes, int VRes)
{
    HBITMAP hbmOld;
    HBRUSH hbrOld;
    HDC hdcMem2;

    // stretching the taskbar could get messy, we'll only do the desktop
    int mon_dy = MON_DY - MON_TRAY;

    // init to identical extents
    SIZE dSrc = { MON_DX, mon_dy };
    SIZE dDst = { MON_DX, mon_dy };

    // set up a work area to play in
    if (!this->hbmMonitor || !this->hbmScrSample)
        return;
    hdcMem2 = CreateCompatibleDC(g_hdcMem);
    if (!hdcMem2)
        return;
    SelectObject(hdcMem2, this->hbmScrSample);
    hbmOld = (HBITMAP)SelectObject(g_hdcMem, this->hbmMonitor);

    // see if we need to shrink either aspect of the image
    if (HRes > this->iOrgXRes || VRes > this->iOrgYRes)
    {
        // make sure the uncovered area will be seamless with the desktop
        RECT rc = { MON_X, MON_Y, MON_X + MON_DX, MON_Y + mon_dy };
        HBRUSH hbr =
            CreateSolidBrush( GetPixel( g_hdcMem, MON_X + 1, MON_Y + 1 ) );

        FillRect(hdcMem2, &rc, hbr);
        DeleteObject( hbr );
    }

    // stretch the image to reflect the new resolution
    if( HRes > this->iOrgXRes )
        dDst.cx = MulDiv( MON_DX, this->iOrgXRes, HRes );
    else if( HRes < this->iOrgXRes )
        dSrc.cx = MulDiv( MON_DX, HRes, this->iOrgXRes );

    if( VRes > this->iOrgYRes )
        dDst.cy = MulDiv( mon_dy, this->iOrgYRes, VRes );
    else if( VRes < this->iOrgYRes )
        dSrc.cy = MulDiv( mon_dy, VRes, this->iOrgYRes );

    SetStretchBltMode( hdcMem2, COLORONCOLOR );
    StretchBlt( hdcMem2, MON_X, MON_Y, dDst.cx, dDst.cy,
        g_hdcMem, MON_X, MON_Y, dSrc.cx, dSrc.cy, SRCCOPY );

    // now fill the new image's desktop with the possibly-dithered brush
    // the top right corner seems least likely to be hit by the stretch...
    hbrOld = (HBRUSH)SelectObject( hdcMem2, GetSysColorBrush( COLOR_DESKTOP ) );
    ExtFloodFill(hdcMem2, MON_X + MON_DX - 2, MON_Y+1,
        GetPixel(hdcMem2, MON_X + MON_DX - 2, MON_Y+1), FLOODFILLSURFACE);

    // clean up after ourselves
    SelectObject( hdcMem2, hbrOld );
    SelectObject( hdcMem2, g_hbmDefault );
    DeleteObject( hdcMem2 );
    SelectObject( g_hdcMem, hbmOld );

    this->iCurXRes = HRes;
    this->iCurYRes = VRes;
}

void CMONITOR::Paint(HDC hdc, LPRECT lprc) {
    if (this->fInit) {
        this->fInit = FALSE;
        SendDlgItemMessage(this->hwndParent, IDC_SCREENSAMPLE, STM_SETIMAGE, IMAGE_BITMAP, (DWORD)this->hbmScrSample);
    }

    InvalidateRect(GetDlgItem(this->hwndParent, IDC_SCREENSAMPLE), NULL, FALSE);
}

void CMONITOR::SysColorChange( UINT msg, WPARAM wParam, LPARAM lParam ) {
    if( this->hbmMonitor )
        DeleteObject( this->hbmMonitor );

    if( this->hbmScrSample )
        DeleteObject( this->hbmScrSample );

    this->hbmScrSample = LoadMonitorBitmap( TRUE ); // let them do the desktop

    // get a base copy of the bitmap for when the "internals" change
    this->hbmMonitor = LoadMonitorBitmap( FALSE ); // we'll do the desktop

    this->SetScreenSize( this->iCurXRes, this->iCurYRes );

    SendDlgItemMessage(this->hwndParent, IDC_SCREENSAMPLE, STM_SETIMAGE, IMAGE_BITMAP, (DWORD)this->hbmScrSample);
}


/*****************************************************************\
*
* CCOLORBAR class
*
\*****************************************************************/

class CCOLORBAR : public CDISPLAYOBJ {
private:
    HBITMAP ahbmColorBar[IDB_COLORMAX - IDB_COLORMIN + 1];
    int     iClrIndex;  /* 0 = 2 clr, 1 = 16 clr, 3 = 256 clr */
    SIZE    size;


public:
    CCOLORBAR();
    ~CCOLORBAR();
    void SetColorIndex(int iClr) {this->iClrIndex = iClr; this->Redraw();}
    void Paint(HDC hdc, LPRECT lprc = NULL);
};

CCOLORBAR::CCOLORBAR() : iClrIndex(0) {
    int i;
    BITMAP bmp;
    HBITMAP hbm;

    this->size.cx = this->size.cy = 0;

    for(i = 0; i <= IDB_COLORMAX - IDB_COLORMIN; i++) {
        hbm = LoadBitmap(ghmod, MAKEINTRESOURCE(i + IDB_COLORMIN));

        if (GetObject(hbm, sizeof(bmp), &bmp)) {
            this->size.cx = bmp.bmWidth;
            this->size.cy = bmp.bmHeight;
        }

        ahbmColorBar[i] = hbm;
    }
}

CCOLORBAR::~CCOLORBAR() {
    int i;

    for (i = IDB_COLORMIN; i <= IDB_COLORMAX; i++) {
        DeleteObject(ahbmColorBar[i]);
    }
}

void CCOLORBAR::Paint(HDC hdc, LPRECT lprc) {
    HDC hdcMem;
    HBITMAP hbmOld;
    RECT rcTmp;

    if ((lprc != NULL && !IntersectRect(&rcTmp, lprc, &(this->rcPos))) ||
         (this->size.cx == 0) ||
         (this->size.cy == 0)) {

        return;

    }

    hdcMem = CreateCompatibleDC(hdc);

    if (hdcMem != NULL) {

        hbmOld = (HBITMAP)SelectObject(hdcMem, this->ahbmColorBar[iClrIndex]);

        StretchBlt(hdc,
                   this->rcPos.left,
                   this->rcPos.top,
                   this->rcPos.right - this->rcPos.left,
                   this->rcPos.bottom - this->rcPos.top,
                   hdcMem,
                   0,
                   0,
                   this->size.cx,
                   this->size.cy,
                   SRCCOPY);

        SelectObject(hdcMem, hbmOld);

        DeleteDC(hdcMem);
    }
}

/*****************************************************************\
*
* CFILEVER class
*
*        Class to access the filever info in a driver
*
\*****************************************************************/
class CFILEVER {
private:
    LPBYTE lpbVerInfo;
    TCHAR szVersionKey[MAX_PATH];

    LPTSTR GetVerInfo(LPTSTR szKey);

public:
    CFILEVER(LPTSTR szFile);
    ~CFILEVER();


    LPTSTR GetFileVer()         { return this->GetVerInfo(SZ_FILEVER); }
    LPTSTR GetCompanyName()     { return this->GetVerInfo(SZ_COMPNAME); }
};

CFILEVER::CFILEVER(LPTSTR szFile): lpbVerInfo(NULL) {

    DWORD cb;
    DWORD dwHandle;

    cb = GetFileVersionInfoSize(szFile, &dwHandle);

    if (cb != 0) {

        lpbVerInfo = (LPBYTE)LocalAlloc(LPTR, cb);

        if (lpbVerInfo != NULL) {

            GetFileVersionInfo(szFile, dwHandle, cb, lpbVerInfo);

        }
    }
}

CFILEVER::~CFILEVER() {

    if (lpbVerInfo != NULL)
        LocalFree(lpbVerInfo);

}

LPTSTR CFILEVER::GetVerInfo(LPTSTR pszKey) {

    LPTSTR lpValue;
    UINT cb = 0;

    //
    // Try to get info in the local language
    //

    wsprintf(szVersionKey, TEXT("\\StringFileInfo\\%04X04B0\\%ws"),
            LANGIDFROMLCID(GetUserDefaultLCID()), pszKey);

    if (lpbVerInfo)
    {
        VerQueryValue(lpbVerInfo, szVersionKey, (LPVOID*)&lpValue, &cb);

        if (cb == 0)
        {
            //
            // No local language, try US English
            //

            wsprintf(szVersionKey, TEXT("\\StringFileInfo\\040904B0\\%ws"), pszKey);

            VerQueryValue(lpbVerInfo, szVersionKey, (LPVOID*)&lpValue, &cb);
        }
    }

    if (cb == 0)
    {
        //
        // We have no file information.
        //

        LoadString(ghmod, IDS_NO_VERSION_INFO, szVersionKey, sizeof(szVersionKey));
        lpValue = szVersionKey;
    }

    return lpValue;
}


/*****************************************************************\
*
* CREGVIDOBJ class
*
*        Class to access the video part of the registry
*
\*****************************************************************/
class CREGVIDOBJ {
private:
    HKEY hkVideoRegR;
    HKEY hkVideoRegW;
    HKEY hkServiceReg;
    LPTSTR pszDrvName;
    LPTSTR pszKeyName;
    BOOL bdisablable;

public:
    CREGVIDOBJ(LPTSTR pszDisplay);
    ~CREGVIDOBJ();

    LPTSTR CloneDescription(void);
    LPTSTR CloneDisplayFileNames(BOOL bPreprocess);

    //
    // Returns a pointer to the mini port name.  THIS IS NOT A CLONE!
    // THE CALLER MUST COPY IT IF IT NEEDS TO KEEP IT AROUND!
    //
    // DO NOT FREE THE POINTER RETURNED FROM THIS CALL!
    //

    LPTSTR GetMiniPort(void)         { return this->pszDrvName; }
    LPTSTR GetKeyName(void)          { return this->pszKeyName; }

    VOID GetHardwareInformation(PHARDWARE_INFO pInfo);

    BOOL EnableDriver(BOOL Enable);
    BOOL SetErrorControlNormal(void);

};

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
                                L"ImagePath",
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
                                L"Group",
                                NULL,
                                NULL,
                                (LPBYTE)szGroup,
                                &cb) == ERROR_SUCCESS) {

                //
                // Compare the string , case insensitive, to the "Video" group.
                //

                bdisablable = !(BOOL)(lstrcmpi(szGroup, L"Video"));

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
// CloneDescription
//
// Gets the descriptive name of the driver out of the registry.
// (eg. "Stealth Pro" instead of "S3").  If there is no
// DeviceDescription value in the registry, then it returns
// the generic driver name (like 'S3' or 'ATI')
//
// NOTE: The caller must LocalFree the returned pointer when they
// are done with it.  (Which is why it is called Clone instead of Get)
//

LPTSTR CREGVIDOBJ::CloneDescription(void) {

    DWORD cb, dwType;
    LPTSTR psz = NULL;
    LONG lRet;

    //
    // query the size of the string
    //

    cb = 0;
    lRet = RegQueryValueEx(hkVideoRegR,
                           SZ_DEVICEDESCRIPTION,
                           NULL,
                           &dwType,
                           NULL,
                           &cb);

    //
    // check to see if there is a string, and the string is more than just
    // a UNICODE_NULL (detection will put an empty string there).
    //

    if ( (lRet == ERROR_SUCCESS || lRet == ERROR_MORE_DATA) &&
         (cb > 2) ) {

        //
        // alloc a string buffer
        //

        psz = (LPTSTR)LocalAlloc(LPTR, cb);

        if (psz) {

            //
            // get the string
            //

            if (RegQueryValueEx(hkVideoRegR,
                                SZ_DEVICEDESCRIPTION,
                                NULL,
                                &dwType,
                                (LPBYTE)psz,
                                &cb) != ERROR_SUCCESS) {

                LocalFree(psz);
                psz = NULL;

            }
        }
    }

    if (!psz) {

        //
        // we can't read the registry, just us the generic name
        //

        psz = FmtSprint(ID_DSP_TXT_COMPATABLE_DEV, this->pszDrvName);

    }

    ASSERT(psz != NULL);

    return psz;
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
        L"HardwareInformation.MemorySize",
        L"HardwareInformation.ChipType",
        L"HardwareInformation.DacType",
        L"HardwareInformation.AdapterString",
        L"HardwareInformation.BiosString"
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

            LoadString(ghmod,
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
                               L"Start",
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
                               L"ErrorControl",
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
* CREGCLEANUP class
*
\*****************************************************************/

class CREGCLEANUP {

private:

    PMARK_FILE pkKeyUsedRoot;
    PMARK_FILE pfMiniUsedRoot;
    PMARK_FILE pfDispUsedRoot;

    PMARK_KEY pkMarkRoot;

    VOID DeleteDriver(LPTSTR DriverName, BOOL Mini, HSPFILELOG hfileLog);

public:
    CREGCLEANUP(void);
    ~CREGCLEANUP();

    VOID vClean(BOOL bCleanEverything);
    VOID ListDriver(LPTSTR deviceName);

};

typedef CREGCLEANUP *PCREGCLEANUP;

//
// cleanup regsitry
//

//
// CREGCLEANUP destructor
//
CREGCLEANUP::~CREGCLEANUP() {

    //
    // Delete the linked list we built
    //


}

//
// CREGCLEANUP constructor
//

CREGCLEANUP::CREGCLEANUP(void) {

    //
    // We should only be performing cleanup if we did NOT postpone the
    // configuration until later.
    //

    ASSERT(gUnattenedConfigureAtLogon == 0);

}

//
// vClean - main function
//
VOID CREGCLEANUP::vClean(BOOL bCleanEverything) {

    HKEY hkServices;
    DWORD i;
    WCHAR pszKeyName[MAX_PATH];
    DWORD cbNameLength;
    FILETIME filetime;
    ULONG ulStat;
    HKEY hkDriver;
    DWORD type;
    WCHAR pGroup[MAX_PATH];
    DWORD cbGroup;
    WCHAR pImage[MAX_PATH];
    DWORD cbImage;
    ULONG bDeleteKey;
    ULONG bDeleteDriver;
    LPTSTR pszDriverName;
    HKEY hkDevice0;

    HSPFILELOG hfileLog = NULL;

    pkKeyUsedRoot = NULL;
    pfMiniUsedRoot = NULL;
    pfDispUsedRoot = NULL;

    pkMarkRoot = NULL;

#if 0
    KdPrint(("Display.cpl: Cleaning up the registry\n"));
#endif

    //
    // Clean up any errors that may have been reported by the system (invalid
    // display resolution) while the applet is running.
    //
    // This will solve the boot up case when a new driver is installed because
    // a ChangeDisplaySettings call is made *after* the display applet has been
    // started to reconfigure the mode, which cause the invalid display key to
    // be put in the registry.
    //

    RegDeleteKey(HKEY_LOCAL_MACHINE,
                 SZ_INVALID_DISPLAY);


    //
    // Only clean we in a setup mode
    //

    if ( (gbExecMode == EXEC_SETUP) ||
         (gbExecMode == EXEC_DETECT) ) {


    } else {

        return;

    }

    //
    // If we are to clean everything, then ignore any loaded driver (dont
    // enumerate them). That way we will only stay with VGA
    //

    if (bCleanEverything) {

#ifndef _X86_
        ASSERT(FALSE);
#endif

#if 0
        KdPrint(("Display.cpl: Cleaning everything in the registry\n"));
#endif

    } else {

        //
        // Get our loaded drivers. There will never be more than three ...
        //

        DWORD iDevNum;

#if 0
        KdPrint(("Display.cpl: Cleaning unused information in the registry\n"));
#endif

        for (iDevNum = 1; ; iDevNum++) {

            DISPLAY_DEVICE displayDevice;

            displayDevice.cb = sizeof(DISPLAY_DEVICE);

            if (EnumDisplayDevices(NULL, iDevNum, &displayDevice)) {

                 ListDriver(displayDevice.DeviceName);

            } else {

                break;

            }
        }

    }

    //
    // We first open the registry, and get a pointer to the services node.
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     L"System\\CurrentControlSet\\Services",
                     0,
                     KEY_READ,
                     &hkServices) !=  ERROR_SUCCESS) {
        return;
    }

    hfileLog = SetupInitializeFileLog(NULL, SPFILELOG_SYSTEMLOG);

    //
    // Enumerate all the drivers in this list, and find the one that are
    // in the video group
    //

    for (i=0; ; i++) {

        pszDriverName = NULL;
        cbNameLength = MAX_PATH;

        ulStat = RegEnumKeyEx(hkServices,
                              i,
                              pszKeyName,
                              &cbNameLength,
                              NULL,
                              NULL,
                              NULL,
                              &filetime);

        if (ulStat == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (ulStat != ERROR_SUCCESS) {
            continue;
        }

        //
        // We now have a key name.
        // Get the group name
        //

        ulStat = RegOpenKeyEx(hkServices,
                              pszKeyName,
                              0,
                              KEY_ALL_ACCESS,
                              &hkDriver);

        if (ulStat != ERROR_SUCCESS) {
            continue;
        }

        cbGroup = MAX_PATH;

        ulStat = RegQueryValueEx(hkDriver,
                                 TEXT("GROUP"),
                                 NULL,
                                 &type,
                                 (LPBYTE) &pGroup,
                                 &cbGroup);

        if ( (ulStat != ERROR_SUCCESS)    ||
             (lstrcmpi(pGroup, L"Video")) ) {

            RegCloseKey(hkDriver);
            continue;
        }

        //
        // This driver is in the video group.
        // check to see if the name is one of our loaded drivers.
        // assume it will be deleted and check against the driver names
        // to see if it should not.
        //

        PMARK_FILE pkUsed = pkKeyUsedRoot;
        bDeleteKey = 1;

        while (pkUsed) {

            if (pkUsed->File) {
                bDeleteKey &= lstrcmpi(pszKeyName, pkUsed->File);
            }

            pkUsed = pkUsed->Next;
        }

        if (bDeleteKey) {

            //
            // We need to get the list of the display drivers and miniport
            // dirvers used by this
            // adapter and see if they are used by anyone else.
            //

            //
            // Now determine if the miniport driver must be deleted
            // Get the binary name if possible. Otherwise just use the
            // key name as the miniport name.
            //

            cbImage = MAX_PATH;

            ulStat = RegQueryValueEx(hkDriver,
                                     TEXT("ImagePath"),
                                     NULL,
                                     &type,
                                     (LPBYTE) &pImage,
                                     &cbImage);

            if (ulStat == ERROR_SUCCESS) {

                //
                // The is a binary.
                // extract the name, which will be of the form ...\driver.sys
                //

                LPTSTR pszDriver, pszDriverEnd;

                pszDriver = pImage;
                pszDriverEnd = pImage + lstrlen(pImage);

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

                    pszDriverName = pszDriverEnd;

                }
            }

            if (pszDriverName == NULL) {

                pszDriverName = pszKeyName;

            }

            //
            // We now have an image name.  Check to see if the image can be
            // deleted;
            //

            PMARK_FILE pkMini = pfMiniUsedRoot;
            bDeleteDriver = 1;

            while (pkMini) {

                if (pkMini->File) {
                    bDeleteDriver &= lstrcmpi(pszDriverName, pkMini->File);
                }

                pkMini = pkMini->Next;
            }

            if (bDeleteDriver) {

                DeleteDriver(pszDriverName, TRUE, hfileLog);

            }

            //
            // Now determine if the display drivers must be deleted
            //

            ulStat = RegOpenKeyEx(hkDriver,
                                  TEXT("Device0"),
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hkDevice0);

            if (ulStat == ERROR_SUCCESS) {

                cbImage = MAX_PATH;

                ulStat = RegQueryValueEx(hkDevice0,
                                         TEXT("InstalledDisplayDrivers"),
                                         NULL,
                                         &type,
                                         (LPBYTE) &pImage,
                                         &cbImage);

                if (ulStat == ERROR_SUCCESS) {

                    pszDriverName = pImage;

                    while (*pszDriverName) {

                        PMARK_FILE pkDisp = pfDispUsedRoot;
                        bDeleteDriver = 1;

                        while (pkDisp) {

                            if (pkDisp->File) {

                                bDeleteDriver &= lstrcmpi(pszDriverName, pkDisp->File);
                            }

                            pkDisp = pkDisp->Next;
                        }

                        if (bDeleteDriver) {

                            DeleteDriver(pszDriverName, FALSE, hfileLog);

                        }

                        while(*pszDriverName++);

                    }
                }

                RegCloseKey(hkDevice0);
            }

            //
            // Now save the key for later deletion
            //

            PMARK_KEY pkMark;

            pkMark = (PMARK_KEY)
                    LocalAlloc (LMEM_ZEROINIT, sizeof(MARK_KEY));
#if 0
            KdPrint(("Display.cpl: Deleting key %ws\n", pszKeyName));
#endif
            if (pkMark) {

                if (pkMarkRoot) {

                    pkMark->Next = pkMarkRoot;

                }

                pkMarkRoot = pkMark;
                pkMarkRoot->hKey = hkDriver;

            } else {

                RegCloseKey(hkDriver);
            }

        } else {

            RegCloseKey(hkDriver);

        }
    }

    //
    // disble the list of all keys to be deleted.
    //

    while (pkMarkRoot)
    {
        PMARK_KEY pktemp = pkMarkRoot;
        DWORD dw = SERVICE_DISABLED;

        RegSetValueEx(pkMarkRoot->hKey,
                      L"Start",
                      NULL,
                      REG_DWORD,
                      (LPBYTE) &dw,
                      sizeof(DWORD));

        RegCloseKey(pkMarkRoot->hKey);

        pkMarkRoot = pkMarkRoot->Next;
        LocalFree(pktemp);
    }

    RegCloseKey(hkServices);


    return;
}

VOID CREGCLEANUP::DeleteDriver(LPTSTR DriverName, BOOL Mini, HSPFILELOG hfileLog) {

    WCHAR pszFilePath[MAX_PATH];
    WCHAR pszSystemPath[MAX_PATH];
    DWORD cb = MAX_PATH;

    cb = GetSystemDirectory(pszSystemPath, cb);

    if (cb) {

        if (lstrcmpi(L"vga", DriverName) &&
            lstrcmpi(L"vga256", DriverName) &&
            lstrcmpi(L"vga64k", DriverName) &&
            lstrcmpi(L"framebuf", DriverName) ) {

            //
            // build up the name
            //

            swprintf(pszFilePath,
                     L"%ws%ws%ws%ws",
                     pszSystemPath,
                     Mini ? L"\\Drivers\\" : L"\\",
                     DriverName,
                     Mini ? SZ_DOTSYS :  SZ_DOTDLL);
#if 0
            KdPrint(("Display.cpl: Deleting driver %ws\n", pszFilePath));
#endif
            //
            // Make sure we delete the file from the filelog
            //

            if (hfileLog)
            {
                //
                // Setup wants a name that start with no drive letter in front
                // that is - \winnt ...
                //

                SetupRemoveFileLogEntry(hfileLog, NULL, pszFilePath + 2);
            }

            DeleteFile(pszFilePath);

        }
    }
}


//
// function that builds up the list up keys, miniport drivers and
// display drivers that are used in the system.
//

VOID CREGCLEANUP::ListDriver(LPTSTR deviceName) {

    CREGVIDOBJ crv(deviceName);

    LPTSTR pszKey = crv.GetKeyName();
    LPTSTR pszMini = crv.GetMiniPort();
    LPTSTR pszDisp = crv.CloneDisplayFileNames(FALSE);

    if (!pszKey) {
        return;
    }

    //
    // Add to the list of used keys
    //

    PMARK_FILE pfUsed;

    pfUsed = (PMARK_FILE) LocalAlloc (LMEM_ZEROINIT, sizeof(MARK_FILE));

    if (pfUsed) {

        if (pkKeyUsedRoot) {

            pfUsed->Next = pkKeyUsedRoot;

        }

        pkKeyUsedRoot = pfUsed;
        pkKeyUsedRoot->File = CloneString(pszKey);

    }

    //
    // Add the miniport to the list of used miniports
    //

    if (pszMini) {

        PMARK_FILE pfMini;

        pfMini = (PMARK_FILE)
                LocalAlloc (LMEM_ZEROINIT, sizeof(MARK_FILE));

        if (pfMini) {

            if (pfMiniUsedRoot) {

                pfMini->Next = pfMiniUsedRoot;

            }

            pfMiniUsedRoot = pfMini;
            pfMiniUsedRoot->File = CloneString(pszMini);
        }
    }

    //
    // Add the miniport to the list of used miniports
    //

    if (pszDisp) {

        LPTSTR pszTmp = pszDisp;

        while (*pszTmp) {

            PMARK_FILE pfDisp;

            pfDisp = (PMARK_FILE)
                    LocalAlloc (LMEM_ZEROINIT, sizeof(MARK_FILE));

            if (pfDisp) {

                if (pfDispUsedRoot) {

                    pfDisp->Next = pfDispUsedRoot;

                }

                pfDispUsedRoot = pfDisp;
                pfDispUsedRoot->File = CloneString(pszTmp);
            }

            //
            // Go to the next string in the list
            //

            while(*pszTmp++);

        }

        LocalFree(pszDisp);
    }

    //
    // Finally mark the driver as ERROR_NORMAL now.
    //

    crv.SetErrorControlNormal();

}

/*****************************************************************\
*
* CDIALOG class
*
\*****************************************************************/

class CDIALOG {

protected:
    HWND hwnd;
    HWND hwndParent;
    HINSTANCE hmodModule;
    LPARAM lParam;
    int  iDlgResID;
    void SetHWnd(HWND hwndNew) { this->hwnd = hwndNew; }
    void SetHWndParent(HWND hwndNew) { this->hwndParent = hwndNew; }
    friend BOOL CALLBACK DisplayDlgProc(HWND hwnd, UINT msg, WPARAM wParam,
                            LPARAM lParam);

    friend DLGPROC NewDisplayDialogBox(HINSTANCE hmod, LPARAM *lplParam);


public:
    int Dialog(HINSTANCE hmod, HWND hwndParentCaller, LPARAM lInitParam = 0);

    HWND GetHWnd(void )        { return this->hwnd; }
    HWND GetHWndParent(void )  { return this->hwndParent; }
    HINSTANCE GetHMod(void )   { return this->hmodModule; }


    LONG SendCtlMsg(int idCtl, UINT msg, WPARAM wParam, LPARAM lParam ) {
        return SendDlgItemMessage(this->hwnd, idCtl, msg, wParam, lParam);
    }

    BOOL SetCtlInt(int idCtl, UINT uiValue, BOOL fSigned = TRUE) {
        return SetDlgItemInt(this->hwnd, idCtl, uiValue, fSigned);
    }

    BOOL Disable(void ) { return EnableWindow(this->hwnd, FALSE); }
    BOOL Enable(void )  { return EnableWindow(this->hwnd, TRUE); }

    virtual BOOL WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
    virtual BOOL InitDlg(HWND hwndFocus)    { return TRUE; }    //BUGBUG - possible dead code!
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode ) = 0;
    virtual BOOL DoNotify(int idControl, NMHDR *lpnmh, UINT iNoteCode ) { return FALSE; }
    virtual BOOL HScroll(int idCtrl, int iCode, int iPos)               { return FALSE; }
    virtual BOOL Paint(void)        { return FALSE; }
    virtual BOOL OnDestroy(void)    { return FALSE; }
    virtual BOOL TimerTick(int id)  { return FALSE; }
    virtual BOOL InitMessage()      { return FALSE; }
    virtual BOOL SysColorChange( UINT msg, WPARAM wParam, LPARAM lParam )   { return FALSE; }
    virtual BOOL DoHelp( LPHELPINFO lphi );
    virtual BOOL DoContextMenu( HWND hwnd, WORD xPos, WORD yPos );

};

typedef CDIALOG *PCDIALOG;

//
// Dialog method
//
//  Creates the dialog box (shows it on the screen)
//

CDIALOG::Dialog(HINSTANCE hmod, HWND hwndCallerParent, LPARAM lInitParam) {

    this->hwndParent = hwndCallerParent;
    this->hmodModule = hmod;
    this->lParam = lInitParam;

    return DialogBoxParam(hmod, MAKEINTRESOURCE(this->iDlgResID),
            this->hwndParent, (DLGPROC)::DisplayDlgProc, (LONG)this);
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
    case WM_WININICHANGE:
    case WM_DISPLAYCHANGE:
        return this->SysColorChange(msg, wParam, lParam);

    case WM_HELP:
        return this->DoHelp( (LPHELPINFO) lParam );

    case WM_CONTEXTMENU:
        return this->DoContextMenu( (HWND)wParam, LOWORD(lParam), HIWORD(lParam) );

    default:

        // if (msg == wHelpMessage) {
        //
        //     return this->DoCommand(ID_DEFAULT_HELP, (HWND) NULL, 0);
        //
        // }

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
* CDLGSTARTUP class
*
* Dialog presented at startup.
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDLGSTARTUP : public CDIALOG {
protected:

    LPTSTR pszDetectDevice;

public:
    CDLGSTARTUP(LPTSTR pszDisplay);
    ~CDLGSTARTUP();

    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
};


CDLGSTARTUP::CDLGSTARTUP(LPTSTR pszDisplay) {

   CREGVIDOBJ rvoVideo(pszDisplay);

   pszDetectDevice = FmtSprint(ID_DSP_TXT_COMPATABLE_DEV,
                               rvoVideo.GetMiniPort());

   this->iDlgResID = DLG_SET_STARTUP;

}

CDLGSTARTUP::~CDLGSTARTUP() {

   if (pszDetectDevice) {

       LocalFree(pszDetectDevice);

    }

}

BOOL CDLGSTARTUP::InitDlg(HWND hwndFocus) {


    this->SendCtlMsg(ID_STARTUP_DETECT,
                     WM_SETTEXT,
                     (WPARAM) 0,
                     (LPARAM) pszDetectDevice);

    return CDIALOG::InitDlg(hwndFocus);

}

BOOL CDLGSTARTUP::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    switch(idControl ) {

    case IDOK:

        EndDialog(this->hwnd, 1);

        break;

    default:

        return FALSE;

    }

    return TRUE;
}

/*****************************************************************\
*
* CDLGCHGADAPTOR class
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDLGCHGADAPTOR : public CDIALOG {
protected:
    int iRet;

public:
    CDLGCHGADAPTOR();

    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
};

CDLGCHGADAPTOR::CDLGCHGADAPTOR() : iRet(RET_NO_CHANGE) {
    this->iDlgResID = DLG_SET_CHANGE_VID;
}

BOOL CDLGCHGADAPTOR::InitDlg(HWND hwndFocus) {

    CREGVIDOBJ crvo(*(LPTSTR *)(this->lParam));
    LPTSTR psz;
    TCHAR szPath[MAX_PATH];
    HDC hdc;
    ULONG DrivVer;
    HARDWARE_INFO hardwareInfo;
    DWORD i;

    //
    // Get the installed driver names and put them in the dialog
    //

    psz = crvo.CloneDisplayFileNames(TRUE);

    if (psz) {

        this->SendCtlMsg(ID_ADP_CURFILES, WM_SETTEXT, 0, (LPARAM)psz);
        LocalFree(psz);

    }

    //
    // display the adaptor type
    //

    psz = crvo.CloneDescription();

    if (psz) {

        this->SendCtlMsg(ID_ADP_ADAPTOR, WM_SETTEXT, 0, (LPARAM)psz);
        LocalFree(psz);

    }

    //
    // Get the miniport driver path
    //

    wsprintf(szPath, TEXT("drivers\\%s.sys"), crvo.GetMiniPort());

    //
    // Open the file version resource for the driver
    //

    CFILEVER cfv(szPath);

    //
    // Get the company name and put it in the dialog
    //

    this->SendCtlMsg(ID_ADP_MANUFACT, WM_SETTEXT, 0,
            (LPARAM)cfv.GetCompanyName());

    //
    // Get the version number from the miniport, and append "," and the
    // display driver version number.
    //

    hdc = GetDC(this->hwnd);
    DrivVer = GetDeviceCaps(hdc, DRIVERVERSION);
    ReleaseDC(this->hwnd, hdc);

    wsprintf(szPath, TEXT("%s, %d.%d.%d"), cfv.GetFileVer(),
             (DrivVer >> 12) & 0xF, (DrivVer >> 8) & 0xF, DrivVer & 0xFF);

    this->SendCtlMsg(ID_ADP_VERSION, WM_SETTEXT, 0, (LPARAM)szPath);


    //
    // Now put in the hardware information.
    //

    crvo.GetHardwareInformation(&hardwareInfo);

    this->SendCtlMsg(ID_ADP_CHIP,       WM_SETTEXT, 0, (LPARAM)hardwareInfo.ChipType);
    this->SendCtlMsg(ID_ADP_DAC,        WM_SETTEXT, 0, (LPARAM)hardwareInfo.DACType);
    this->SendCtlMsg(ID_ADP_MEM,        WM_SETTEXT, 0, (LPARAM)hardwareInfo.MemSize);
    this->SendCtlMsg(ID_ADP_ADP_STRING, WM_SETTEXT, 0, (LPARAM)hardwareInfo.AdapString);
    this->SendCtlMsg(ID_ADP_BIOS_INFO,  WM_SETTEXT, 0, (LPARAM)hardwareInfo.BiosString);

    for (i=0; i < 5; i++) {

        if ( *(((LPTSTR *) (&hardwareInfo)) + i) != NULL) {

            LocalFree(*(((LPTSTR *) (&hardwareInfo)) + i));

        }
    }

    return TRUE;
}

BOOL CDLGCHGADAPTOR::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    DWORD err;
    BOOL  bKeepEnabled = FALSE;

    switch(idControl ) {

    case IDCANCEL:
        EndDialog(this->hwnd, iRet);
        break;

    case ID_ADP_DETECT:
    case ID_ADP_CHGADP: {

        LPTSTR lpsz;

        if ( (gbExecMode == EXEC_SETUP) ||
             (gbExecMode == EXEC_DETECT) ) {

            //
            // Can not change a driver during setup.
            // run the control panel later on.
            //

            FmtMessageBox(this->hwnd,
                          MB_ICONINFORMATION,
                          FALSE,
                          ID_DSP_TXT_INSTALL_DRIVER,
                          ID_DSP_TXT_DRIVER_IN_SETUP_MODE);

            break;

        }

        if (IsUserAnAdmin() == FALSE)
        {
            FmtMessageBox(this->hwnd,
                          MB_ICONEXCLAMATION,
                          FALSE,
                          ID_DSP_TXT_INSTALL_DRIVER,
                          ID_DSP_TXT_ADMIN_CHANGE);
            break;
        }

        //
        // Let's get the current driver description, if it is available,
        // so we can pass it as the default driver to select in setup.
        //

        CREGVIDOBJ crvo(*(LPTSTR *)(this->lParam));

        crvo.EnableDriver(FALSE);

        err = InstallNewDriver(hwnd,
                               crvo.CloneDescription(),
                               (idControl == ID_ADP_DETECT),
                               &bKeepEnabled);


        if ((err != NO_ERROR) ||
            (bKeepEnabled))
        {
            //
            // Re-enable the driver if the installation failed.
            //

            crvo.EnableDriver(TRUE);
        }

        if (err == NO_ERROR)
        {
            //
            // Remember the driver has been changed.
            //

            iRet = RET_CHANGED;

            //
            // Reset the buttons in the UI
            //

            lpsz = FmtSprint(ID_DSP_TXT_CLOSE);
            this->SendCtlMsg(IDCANCEL, WM_SETTEXT, 0, (LPARAM)lpsz);
            LocalFree(lpsz);

            this->InitDlg(NULL);
        }

        break;
    }

    default:

        return FALSE;

    }

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
        WCHAR pszMode[1024];

        swprintf(pszMode, L"%ws%ws", lpsz,
                 pcdevCurrent->bModeTested() ? L" - Tested" : L"");

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

/*****************************************************************\
*
* CDISPLAYDLG class
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDISPLAYDLG : public CDIALOG {
protected:
    CMONITOR monitor;
    CCOLORBAR clrbar;

    LPTSTR pszDisplay;
    PCDEVMODELST pcdmlModes;

    int iOrgResolution;
    int iOrgColor;
    int iOrgFrequency;

    int iResolution;
    int iColor;
    int iFrequency;

    int cResolutions;

    //
    // current log pixels of the screen.
    // does not change !

    int cLogPix;

    //
    // Determines if the current adapter is the adapter that was booted with.
    //

    BOOL fActiveDsp;

    BOOL bBadDriver;

    //
    // When a new driver is installed, we don't want to save the parameters
    // to the registry.
    //

    BOOL bNewDriver;

public:
    CDISPLAYDLG();
    ~CDISPLAYDLG();

    VOID vPreExecMode();
    VOID vPostExecMode();

    BOOL SaveParamsToReg();

    void SetCurColor(int iClr);
    void SetCurFrequency(int iFreq);
    void SetCurResolution(int iRes);

    void ForceSmallFont( BOOL fDoit );

    BOOL InitEverything();
    BOOL InitFontList(void);

    BOOL bDoClean(void);


    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
    virtual BOOL DoNotify(int idControl, NMHDR *lpnmh, UINT iNoteCode );
    virtual BOOL HScroll(int idCtrl, int iCode, int iPos);
    virtual BOOL Paint(void);
    virtual BOOL InitMessage();
    virtual BOOL SysColorChange( UINT msg, WPARAM wParam, LPARAM lParam );

};

typedef CDISPLAYDLG *PDISPLAYDLG;

//
// Constructor for CDISPLAYDLG
//
//  (gets called when ever a CDISPLAYDLG object is created)
//

CDISPLAYDLG::CDISPLAYDLG() : cResolutions(0), pszDisplay(NULL),
        bBadDriver(FALSE), bNewDriver(FALSE) {

    this->iDlgResID = DLG_SET_DISPLAY;

    HKEY hkFont;
    DWORD cb;

    pcdmlModes = new CDEVMODELST;

    //
    // For font size just always use the one of the current screen.
    // Whether or not we are testing the current screen.
    //

    cLogPix = 96;

    //
    // If the size does not match what is in the registry, then install
    // the new one
    //

    if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      SZ_FONTDPI_PROF,
                      0,
                      KEY_READ,
                      &hkFont) == ERROR_SUCCESS) ||
        (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      SZ_FONTDPI,
                      0,
                      KEY_READ,
                      &hkFont) == ERROR_SUCCESS)) {

        cb = sizeof(DWORD);

        if (RegQueryValueEx(hkFont,
                            SZ_LOGPIXELS,
                            NULL,
                            NULL,
                            (LPBYTE) &cLogPix,
                            &cb) != ERROR_SUCCESS) {

            cLogPix = 96;

        }

        RegCloseKey(hkFont);

    }

}

//
// Destructor
//
CDISPLAYDLG::~CDISPLAYDLG() {
    if (pszDisplay != NULL)
        LocalFree(pszDisplay);

    if (pcdmlModes != NULL) {

        delete pcdmlModes;
        pcdmlModes = NULL;

    }
}

//
// InitDlg method
//

BOOL CDISPLAYDLG::InitDlg(HWND hwndFocus) {

    int cb;
    RECT rc;
    HDC hdc;
    int cxRes, cyRes;
    int passes;

    //
    // Determine in what mode we are running the applet before getting
    // information
    //

    vPreExecMode();

    //
    // Tell Monitor picture where it should paint itself
    //

    GetWindowRect(GetDlgItem(this->hwnd, ID_DSP_REPRESENT), &rc);
    ScreenToClient(this->hwnd, (LPPOINT)&(rc.left));
    ScreenToClient(this->hwnd, (LPPOINT)&(rc.right));

    this->monitor.SetPosition(this->hwnd, &rc);

    //
    // Tell Color bar where it should paint itself
    //

    GetWindowRect(GetDlgItem(this->hwnd, ID_DSP_COLORBAR), &rc);
    ScreenToClient(this->hwnd, (LPPOINT)&(rc.left));
    ScreenToClient(this->hwnd, (LPPOINT)&(rc.right));

    this->clrbar.SetPosition(this->hwnd, &rc);

    //
    // Now tell the entire window where to paint itself (centered).
    //

    GetWindowRect(this->hwnd, &rc);
    hdc = GetDC(this->hwnd);
    cxRes = GetDeviceCaps(hdc, HORZRES);
    cyRes = GetDeviceCaps(hdc, VERTRES);


    cxRes = (cxRes - (rc.right - rc.left)) / 2;
    cyRes = (cyRes - (rc.bottom - rc.top)) / 2;

    SetWindowPos(this->hwnd,
                 0,
                 cxRes < 1 ? 1 : cxRes,
                 cyRes < 1 ? 1 : cyRes,
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOZORDER);

    //
    // If we are in setupmode, and someone wants a driver automatically
    // installed, install it before we enum the drivers and configure them.
    //

    if (gUnattenedInstall)
    {
        UNICODE_STRING UnicodeString;
        TCHAR ServiceName[MAX_PATH];
        NTSTATUS Status;
        BOOLEAN PrivEnabled;
        LPTSTR lpUnattenedPszOptionEnd = gUnattenedPszOption;
        LPTSTR lpUnattenedPszInfEnd = gUnattenedPszInf;
        LPTSTR lpUnattenedPszOption = gUnattenedPszOption;
        LPTSTR lpUnattenedPszInf = gUnattenedPszInf;

        //
        // Transform the string to a MULTI_SZ
        //

        do
        {
            if (*lpUnattenedPszOptionEnd == TEXT(','))
            {
                *lpUnattenedPszOptionEnd = 0;
                lpUnattenedPszOptionEnd++;
            }
        } while (*lpUnattenedPszOptionEnd++);

        lpUnattenedPszOptionEnd = 0;

        do
        {
            if (*lpUnattenedPszInfEnd == TEXT(','))
            {
                *lpUnattenedPszInfEnd = 0;
                lpUnattenedPszInfEnd++;
            }
        } while (*lpUnattenedPszInfEnd++);

        //
        // Install all the specified drivers in a loop.
        // If an error occurs (except in LoadDriver), just go straight to
        // the cleanup so the machine boots in VGA.
        //

        while (*lpUnattenedPszOption && *lpUnattenedPszInf)
        {
            if (PreInstallDriver(this->hwnd,
                                 lpUnattenedPszOption,
                                 lpUnattenedPszInf,
                                 ServiceName) != NO_ERROR)
            {
                return FALSE;
            }

            //
            // Let's try to load the driver on the fly
            //

            Status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                                        TRUE,
                                        FALSE,
                                        &PrivEnabled);

            if (NT_SUCCESS(Status)) {

                RtlInitUnicodeString(&UnicodeString,
                                     ServiceName);

                //
                // Don't check the return value since it is OK to fail the
                // driver if the hardware is not present.
                // The driver will get cleaned up properly upon exit from the
                // applet
                //

                Status = NtLoadDriver(&UnicodeString);

                RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                                   PrivEnabled,
                                   FALSE,
                                   &PrivEnabled);

                //
                // If we loaded the driver successfully, then try to
                // reenumerate the device list and find the driver in there.
                // Otherwise ask the user if this driver should really be
                // kept around.
                //
            }

            //
            // These are MULTI_SZ strings, except they can have initial spaces
            //

            while (*lpUnattenedPszOption++ != 0);
            while (*lpUnattenedPszOption == TEXT(' '))
            {
                lpUnattenedPszOption++;
            }

            while (*lpUnattenedPszInf++ != 0);
            while (*lpUnattenedPszInf == TEXT(' '))
            {
                lpUnattenedPszInf++;
            }
        }

        //
        // Force the screen to repeint in case the detection messed up
        // the contents of the screen.
        //

        ChangeDisplaySettings(NULL, CDS_RESET);
    }

    //
    // Get the current primary device and all it's information
    //
    // Which driver do we want to display by default ?
    //
    // 1) The primary - unless VGA
    // 2) A non-primary, to avoid VGA.
    // 3) Anything
    //

    for (passes = 1; (passes <= 3) && (this->pszDisplay == NULL); passes++)
    {
        int iDevNum = 1;
        DISPLAY_DEVICE displayDevice;

        displayDevice.cb = sizeof(DISPLAY_DEVICE);

        while (EnumDisplayDevices(NULL, iDevNum++, &displayDevice))
        {
            fActiveDsp = displayDevice.StateFlags &
                         DISPLAY_DEVICE_PRIMARY_DEVICE;

            if (passes == 1)
            {
                //
                // Search for the primary.
                //

                if (fActiveDsp)
                {
                    CREGVIDOBJ crv(displayDevice.DeviceName);

                    LPTSTR pszMini = crv.GetMiniPort();

                    //
                    // If VGA is active, then go to pass 2.
                    // Otherwise, let's try to use this device
                    //

                    if (pszMini && (!lstrcmpi(TEXT("vga"), pszMini)))
                    {
                        break;
                    }
                }
                else
                {
                    continue;
                }

            }
            else if (passes == 2)
            {
                //
                // Search for a non-primary, and try that for initialization.
                //

                if (fActiveDsp)
                {
                    continue;
                }
            }
            else
            {
                //
                // If we make it to pass 3, then we want to try any device
                //
            }


            //
            // Save the name of this device for later use.
            //

            this->pszDisplay = CloneString(displayDevice.DeviceName);

            //
            // Try to initialize the applet.
            // If for some reason this fails, then we return and
            // try the next device (just like the system init code does).
            //
            // In this case we want to make the sure the "old driver" pop-up
            // shows up.
            //

            if (this->InitEverything() == TRUE)
            {
                break;
            }
            else
            {
                //
                // We failed. Try the next video device.
                //

                bBadDriver = TRUE;

                LocalFree(this->pszDisplay);
                this->pszDisplay = NULL;
            }
        }
    }

    //
    // If the active display did not work, give it a last try with the active
    // device
    //

    if (this->pszDisplay == NULL)
    {
        ASSERT(FALSE);
    }

    //
    // Determine if any errors showed up during enumerations and initialization
    //

    vPostExecMode();

    //
    // Now tell the user what we found out during initialization
    // Errors, or what we found during detection
    //

    PostMessage(this->hwnd, MSG_DSP_SETUP_MESSAGE, 0, 0);

    //
    // Since this could have taken a very long time, just make us visible
    // if another app (like progman) came up.
    //

    ShowWindow(this->hwnd, SW_SHOW);

    return TRUE;
}

//
// InitEveryting
//
// Inits all the controls in the list box and bulids the devmode matrix
//

BOOL CDISPLAYDLG::InitEverything() {

    int i;
    DEVMODE defaultdm;
    PCDEVMODE pcdev;
    int iResOk, iClrOk, iFreqOk;

    //
    // Clear out old values
    //

    if (pcdmlModes != NULL) {

        delete pcdmlModes;
        pcdmlModes = NULL;

    }

    this->SendCtlMsg(ID_DSP_COLORBOX, CB_RESETCONTENT, 0, 0);
    this->SendCtlMsg(ID_DSP_FREQ, CB_RESETCONTENT, 0, 0);
    this->SendCtlMsg(ID_DSP_FONTSIZE, CB_RESETCONTENT, 0, 0);

    //
    // Clear cached values for selected modes.
    // These are reset at the end of the routine before exiting.
    //

    this->iColor = -1;
    this->iFrequency = -1;
    this->iResolution = -1;

    //
    // Clear initial mode value.
    // Setting all these values to -1 means the user has to save on OK.
    //

    this->iOrgResolution = -1;
    this->iOrgColor = -1;
    this->iOrgFrequency = -1;

    //
    // new matrix storage
    //

    pcdmlModes = new CDEVMODELST;

#if 0

    HDC hdc;

    //
    // This code will determine exactly what bit depth the video mode
    // is set at.
    //

    hdc = GetDC(this->hwnd);

    cxRes = GetDeviceCaps(hdc, DESKTOPHORZRES);
    cyRes = GetDeviceCaps(hdc, DESKTOPVERTRES);
    cClr = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    chzRefresh = GetDeviceCaps(hdc, VREFRESH);

    if (cClr == 16) {

        BYTE ajBitmapInfo[sizeof(BITMAPINFO) + 3 * sizeof(DWORD)];
        BITMAPINFO *pbmi;
        HBITMAP hbm;

        pbmi = (BITMAPINFO *) ajBitmapInfo;

        //
        // Some devices that are actually 15bpp have to return 16bpp
        // for compatibility, but we need  to know the precise bits
        // per pixel so that we can distinguish between available 15bpp
        // and 16bpp modes.  As such, we call GDI to determine the
        // RGB bit masks, and we count the bits in those.
        //
        // First, we need a compatible bitmap from which to query:
        //

        hbm = CreateCompatibleBitmap(hdc, 1, 1);
        if (hbm != 0) {

            //
            // Now call GetDIBits to fill in our bitmap info header:
            //

            pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            pbmi->bmiHeader.biBitCount = 0;

            if (GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS)) {

                if (pbmi->bmiHeader.biCompression == BI_BITFIELDS) {

                    //
                    // Now call GetDIBits to fill in the colour table.
                    //

                    if (GetDIBits(hdc, hbm, 0, pbmi->bmiHeader.biHeight, NULL,
                                  pbmi, DIB_RGB_COLORS)) {

                        DWORD *adwMask;
                        DWORD dwMask;

                        adwMask = (DWORD *) &pbmi->bmiColors[0];

                        dwMask = adwMask[0] | adwMask[1] | adwMask[2];

                        //
                        // Okay, we can now count the number of bits in the
                        // masks to determine the true bits per pixel.
                        //

                        for (cClr = 0; dwMask != 0; dwMask >>= 1) {

                            if (dwMask & 1) {

                                cClr++;
                            }
                        }

                        //
                        // Let's be paranoid, since we're so close to shipping.
                        //

                        if ((cClr != 15) && (cClr != 16)) {

                            cClr = 16;
                        }
                    }

                } else {

                    //
                    // Default DIB format for non BI_BITFIELDS.
                    //

                    cClr = 15;
                }
            }

            DeleteObject(hbm);
        }
    }

    ReleaseDC(this->hwnd, hdc);

    defaultdm.dmBitsPerPel = cClr;
    defaultdm.dmPelsWidth  = cxRes;
    defaultdm.dmPelsHeight = cyRes;
    defaultdm.dmDisplayFrequency = chzRefresh;
    defaultdm.dmDisplayFlags = 0;

#endif

    //
    // Try to build the list of mode.
    //

    if (pcdmlModes->BuildList(this->pszDisplay, this->hwnd) == FALSE) {

        return FALSE;
    }

    //
    // Tell the controls what their valid values are.
    //

    cResolutions = pcdmlModes->GetResCount();
    this->SendCtlMsg(ID_DSP_AREA_SB, TBM_SETRANGE, TRUE, MAKELONG(0, cResolutions - 1));

    for (i = 0; i < pcdmlModes->GetClrCount(); i++) {

        int cBits;
        LPTSTR lpszColor;

        //
        // convert bit count to number of colors and make it a string
        //

        cBits = pcdmlModes->ColorFromIndex(i);

        if (cBits == 32) {

            lpszColor = FmtSprint(ID_DSP_TXT_TRUECOLOR);

        } else {

            lpszColor = FmtSprint(ID_DSP_TXT_COLOR, (1 << cBits));

        }

        this->SendCtlMsg(ID_DSP_COLORBOX, CB_INSERTSTRING, i,
                (LPARAM)lpszColor);

        LocalFree(lpszColor);
    }

    for (i = 0; i < pcdmlModes->GetFreqCount(); i++) {

        LPTSTR lpszFreq;
        int cHz = pcdmlModes->FreqFromIndex(i);

        //
        // convert bit count to number of colors and make it a string.
        //

        if ((cHz == 0) ||
            (cHz == 1) ) {

            lpszFreq = FmtSprint(ID_DSP_TXT_DEFFREQ);

        } else if (cHz < 50) {

            lpszFreq = FmtSprint(ID_DSP_TXT_INTERLACED, cHz);


        } else {

            lpszFreq = FmtSprint(ID_DSP_TXT_FREQ, cHz);
        }

        this->SendCtlMsg(ID_DSP_FREQ, CB_INSERTSTRING, i,
                (LPARAM)lpszFreq);

        LocalFree(lpszFreq);
    }

    //
    // These values
    //

    iResOk  = 0;
    iClrOk  = 0;
    iFreqOk = 0;

    pcdev = NULL;

    //
    // If the device we are getting the modes for is the current device,
    // then lets get the current system mode.
    // We will want to mark that mode as pretested.
    //

    if (fActiveDsp) {

        int iRes, iClr, iFreq;

        RtlZeroMemory(&defaultdm, sizeof(DEVMODE));
        defaultdm.dmSize = sizeof(DEVMODE);

        if (EnumDisplaySettings(this->pszDisplay, (ULONG) -1, &defaultdm))
        {
            PCDEVMODE pcdev;

            iRes = pcdmlModes->IndexFromResXY(defaultdm.dmPelsWidth,
                                              defaultdm.dmPelsHeight);
            iClr = pcdmlModes->IndexFromColor(defaultdm.dmBitsPerPel);
            iFreq = pcdmlModes->IndexFromFreq(defaultdm.dmDisplayFrequency);

            if (pcdev = pcdmlModes->LookUp(iRes, iClr, iFreq))
            {
                pcdev->vTestMode(TRUE);
                iResOk  = iRes;
                iClrOk  = iClr;
                iFreqOk = iFreq;

                //
                // For the active display, set the original values to the current
                // values. These might be invalid values, such as -1, in the
                // case of setup or a bad_mode in the registry ...
                //

                iOrgResolution = iRes;
                iOrgColor      = iClr;
                iOrgFrequency  = iFreq;
            }
        }
    }

    //
    // Let's get the mode that is in the registry.
    // We will use this mode to set the default settings of the controls.
    //

    RtlZeroMemory(&defaultdm, sizeof(DEVMODE));
    defaultdm.dmSize = sizeof(DEVMODE);

    if (EnumDisplaySettings(this->pszDisplay, (ULONG) -2, &defaultdm))
    {
        int iRes, iClr, iFreq;

        iRes = pcdmlModes->IndexFromResXY(defaultdm.dmPelsWidth,
                                          defaultdm.dmPelsHeight);
        iClr = pcdmlModes->IndexFromColor(defaultdm.dmBitsPerPel);
        iFreq = pcdmlModes->IndexFromFreq(defaultdm.dmDisplayFrequency);

        if (pcdev = pcdmlModes->LookUp(iRes, iClr, iFreq))
        {
            //
            // We found a mode in the registry.  We will use this mode as
            // the new default ofr the applet since someone could have
            // changed the mode on the fly, and the current active mode
            // would not be the "current state of the machine".
            //

            iResOk  = iRes;
            iClrOk  = iClr;
            iFreqOk = iFreq;
        }
        else
        {
            //
            // BUGBUG - if the mode in the registry does not work, we should
            // generate and error here (this needs to be done in conjunction
            // with the code in USER.
            //

        }
    }


    if (pcdev == NULL)
    {
        //
        // If the current mode is invalid, and there is no mode in the
        // registry that we can read, then we are in some sort of setup mode.
        // Try to use the setup values that are passed in by setup.
        //

        //
        // Try to find the mode that matches the predefined parameters
        // set during setup.
        // If they are not valid, we will find the closest match.
        //
        // Try to find a 60 Hz Mode if no frequency is specified.
        // Try to find a 256 color mode if no color depth is defined
        //

        if (gUnattenedVRefresh == 0) {
            gUnattenedVRefresh = 60;
        }

        if (gUnattenedBitsPerPel == 0) {
            gUnattenedBitsPerPel = 8;
        }

        // DbgPrint("VRefresh = %d  BitsPerPel = %d  XResolution = %d  YResolution = %d \n",
        //          gUnattenedVRefresh, gUnattenedBitsPerPel,
        //          gUnattenedXResolution, gUnattenedYResolution);

        iFreqOk = pcdmlModes->IndexFromFreq(gUnattenedVRefresh);
        iClrOk  = pcdmlModes->IndexFromColor(gUnattenedBitsPerPel);
        iResOk  = pcdmlModes->IndexFromResXY(gUnattenedXResolution,
                                           gUnattenedYResolution);

        if (iFreqOk == -1) {
            iFreqOk = 0;
        }
        if (iResOk == -1) {
            iResOk = 0;
        }
        if (iClrOk == -1) {
            iClrOk = 0;
        }
    }

    //
    // Set the fonts.  This must be done before calling SetCurResolution
    //

    this->InitFontList();

    //
    // Call FindClosest mode to make sure we actually have a valid mode
    // show up in the display applet.
    //

    pcdmlModes->FindClosestMode(iResOk, &iClrOk, &iFreqOk);

    this->SetCurResolution(iResOk);
    this->SetCurColor(iClrOk);
    this->SetCurFrequency(iFreqOk);

    // Recompute bitmaps incase syscolors have changed since when this dialog object was constructed
    // and the dialog has been shown for this first time.
    this->SysColorChange(WM_SYSCOLORCHANGE, 0, 0);

    return TRUE;

}

// Init Font sizes
//
// Read the supported fonts out of the inf file(s)
// Select was the user currently has.
//

BOOL CDISPLAYDLG::InitFontList() {

    HINF InfFileHandle;
    INFCONTEXT infoContext;
    int i;
    ULONG currentSel = (ULONG) -1;

    //
    // Get all font entries from both inf files
    //

    InfFileHandle = SetupOpenInfFile(TEXT("font.inf"),
                                     NULL,
                                     INF_STYLE_WIN4,
                                     NULL);

    if (InfFileHandle != INVALID_HANDLE_VALUE)
    {
        if (SetupFindFirstLine(InfFileHandle,
                               TEXT("Font Sizes"),
                               NULL,
                               &infoContext))
        {
            while(TRUE)
            {
                int cPix = 0;
                TCHAR awcDesc[LINE_LEN];

                if (SetupGetStringField(&infoContext,
                                        0,
                                        awcDesc,
                                        sizeof(awcDesc),
                                        NULL) &&
                    SetupGetIntField(&infoContext,
                                     1,
                                     &cPix))
                {
                    //
                    // Add it to the list box
                    //

                    i = this->SendCtlMsg(ID_DSP_FONTSIZE,
                                         CB_ADDSTRING,
                                         0,
                                         (LPARAM) awcDesc);

                    this->SendCtlMsg(ID_DSP_FONTSIZE,
                                     CB_SETITEMDATA,
                                     i,
                                     cPix);

                    if (cLogPix == cPix)
                    {
                        currentSel = i;
                    }
                }

                //
                // Try to get the next line.
                //

                if (!SetupFindNextLine(&infoContext,
                                       &infoContext))
                {
                    break;
                }
            }
        }

        SetupCloseInfFile(InfFileHandle);
    }

    //
    // Put in the empty entry if necessary.
    //

    if (currentSel == (ULONG) -1)
    {
        LPTSTR lpszNoFonts;

        //
        // Put a line that says <No Fonts>
        //

        lpszNoFonts = FmtSprint(ID_DSP_NO_FONTS_AVAIL);

        i = this->SendCtlMsg(ID_DSP_FONTSIZE, CB_ADDSTRING, 0,
                             (LPARAM) lpszNoFonts);

        this->SendCtlMsg(ID_DSP_FONTSIZE, CB_SETITEMDATA, i, 0);

        currentSel = i;
    }

    //
    // Select the right entry.
    //

    this->SendCtlMsg(ID_DSP_FONTSIZE, CB_SETCURSEL, currentSel, 0);

    return TRUE;
}

//
// SaveParamsToReg
//
//  Writes the new display parameters to the proper place in the
//  registry.
//

BOOL CDISPLAYDLG::SaveParamsToReg() {

    CREGVIDOBJ rvoVideo(this->pszDisplay);

    int cOk = 0;
    int cx, cy;
    int chzFreq;
    int cFontSize = 0;

    int i;

    //
    // Save all of the new values out to the registry
    //

    //
    // Resolution color bits and frequency
    //

    PCDEVMODE pcdev;

    pcdev = pcdmlModes->LookUp(this->iResolution,
                               this->iColor,
                               this->iFrequency);

    pcdmlModes->ResXYFromIndex(this->iResolution, &cx, &cy);
    chzFreq = pcdmlModes->FreqFromIndex(this->iFrequency);

    //
    // If we are in DevModeNotify mode then send them the DEVMODE info
    // and return
    //
    ASSERT(pcdev != NULL);

    if (hwndDevModeNotify) {
        COPYDATASTRUCT cds;

        cds.dwData = CPL_INIT_DEVMODE_TAG;
        cds.cbData = sizeof(DEVMODE);
        cds.lpData = pcdev->GetData();
        SendMessage(hwndDevModeNotify, WM_COPYDATA, CPL_INIT_DEVMODE_TAG, (LPARAM)&cds);
        return TRUE;
    }

    if (ChangeDisplaySettingsEx(this->pszDisplay,
                                pcdev->GetData(),
                                NULL,
                                CDS_UPDATEREGISTRY | CDS_NORESET,
                                NULL) == DISP_CHANGE_NOTUPDATED)
    {
        FmtMessageBox(this->hwnd,
                      MB_ICONEXCLAMATION,
                      FALSE,
                      ID_DSP_TXT_CHANGE_SETTINGS,
                      ID_DSP_TXT_ADMIN_CHANGE);

        return FALSE;
    }

    //
    // Change font size if necessary
    //

    i = this->SendCtlMsg(ID_DSP_FONTSIZE, CB_GETCURSEL, 0, 0);

    if (i != CB_ERR ) {

        WCHAR awcDesc[10];

        cFontSize = this->SendCtlMsg(ID_DSP_FONTSIZE, CB_GETITEMDATA, i, 0);

        if ( (cFontSize != CB_ERR) &&
             (cFontSize != 0) &&
             (cFontSize != cLogPix)) {

            //
            // The user has changed the fonts.
            // Lets make sure they want this.
            //

            if (FmtMessageBox(this->hwnd,
                              MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION,
                              FALSE,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_NEW_FONT)
                == IDNO) {

                return FALSE;

            }

            //
            // Call setup to change the font size.
            //

            wsprintf(awcDesc, TEXT("%d"), cFontSize);

            if (SetupChangeFontSize(this->hwnd, awcDesc) == NO_ERROR)
            {
                //
                // A font size change will require a system reboot.
                //

                PropSheet_RestartWindows(ghwndPropSheet);
            }
            else
            {
                //
                // Setup failed.
                //

                FmtMessageBox(this->hwnd,
                              MB_ICONSTOP | MB_OK,
                              FALSE,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_ADMIN_INSTALL);

                return FALSE;
            }
        }
    }

    if (cFontSize == 0)
    {
        //
        // If we could not read the inf, then ignore the font selection
        // and don't force the reboot on account of that.
        //

        cFontSize = cLogPix;
    }

    return TRUE;
}

//
// SetCurResolution method
//
//  Sets the string in under the resolution slider, sets the thumb to the
//  correct pos. and remembers the new resolution index.
//
//
void CDISPLAYDLG::SetCurResolution(int iRes ) {
    int cx, cy;
    LPTSTR lpszXbyY;

    pcdmlModes->ResXYFromIndex(iRes, &cx, &cy);

    monitor.SetScreenSize(cx, cy);

    lpszXbyY = FmtSprint(ID_DSP_TXT_XBYY, cx, cy);
    this->SendCtlMsg(ID_DSP_X_BY_Y, WM_SETTEXT, 0, (LPARAM)lpszXbyY);
    LocalFree(lpszXbyY);

    this->SendCtlMsg(ID_DSP_AREA_SB, TBM_SETPOS, TRUE, iRes);

    this->iResolution = iRes;

    //
    // Force Small fonts at 640x480
    //
    if (cx < 800 || cy < 600) {
        this->ForceSmallFont(TRUE);
    } else {
        this->ForceSmallFont(FALSE);
    }
}

//
// ForceSmallFont method
//
//
void CDISPLAYDLG::ForceSmallFont( BOOL fDoit ) {
    int i, iSmall, dpiSmall, dpi;
    static int iOld = CB_ERR;

    if ( fDoit) {
        //
        // Force only small fonts
        //

        // Save the current setting so we can restore it later
        if (iOld == CB_ERR)
            iOld = this->SendCtlMsg(ID_DSP_FONTSIZE, CB_GETCURSEL, 0, 0);

        //
        // Set small font size in the listbox.
        //
        iSmall = CB_ERR;
        dpiSmall = 9999;
        for (i=0; i <=1; i++)
        {
            dpi = this->SendCtlMsg(ID_DSP_FONTSIZE, CB_GETITEMDATA, i, 0);
            if (dpi == CB_ERR)
                continue;

            if (dpi < dpiSmall || iSmall < CB_ERR)
            {
                iSmall = i;
                dpiSmall = dpi;
            }
        }

        if (iSmall != -1)
            this->SendCtlMsg(ID_DSP_FONTSIZE, CB_SETCURSEL, iSmall, 0);

        EnableWindow(GetDlgItem(this->hwnd, ID_DSP_FONTSIZE), FALSE);

    } else {
        //
        // Allow any font
        //

        EnableWindow(GetDlgItem(this->hwnd, ID_DSP_FONTSIZE), TRUE);

        //
        // Restore the old setting we remembered before (only restore
        // if we are coming out of a Disabled case)
        //
        if (iOld != CB_ERR) {
            this->SendCtlMsg(ID_DSP_FONTSIZE, CB_SETCURSEL, iOld, 0);
        }

        iOld = CB_ERR;
    }
}

//
// SetCurFrequency method
//
//  Updates the combo list, and remembers the new frequency index
//
void CDISPLAYDLG::SetCurFrequency(int iFreq ) {

    this->SendCtlMsg(ID_DSP_FREQ, CB_SETCURSEL, iFreq, 0);

    this->iFrequency = iFreq;
}

//
// SetCurColor method
//
//  Updates the combo list, repaints the correct color bar, and
//  remembers the new color index
//
void CDISPLAYDLG::SetCurColor(int iClr) {
    int cBits;

    this->SendCtlMsg(ID_DSP_COLORBOX, CB_SETCURSEL, iClr, 0);

    cBits = pcdmlModes->ColorFromIndex(iClr);
    if (cBits < C_CLR_BITS_VGA )
        clrbar.SetColorIndex(ICLR_MONO);
    else if (cBits < C_CLR_BITS_PALLET)
        clrbar.SetColorIndex(ICLR_STANDARD);
    else {
        clrbar.SetColorIndex(ICLR_PALLET);
    }

    this->iColor = iClr;
}


//
// DoNotify method
//
BOOL CDISPLAYDLG::DoNotify(int idControl, NMHDR *lpnmh, UINT iNoteCode ) {
    PCDEVMODE pcdev;

    switch (iNoteCode) {


    case PSN_APPLY:

        if (bNewDriver)
        {
            //
            // If a new driver is installed, we just want to tell the
            // system to reboot.
            //
            // NOTE - this is only until we can get drivers to load on the fly.
            //

            PropSheet_RestartWindows(ghwndPropSheet);
            SetWindowLong(this->hwnd, DWL_MSGRESULT, PSNRET_NOERROR);
            break;
        }

        //
        // Check to see if the user has tried the resolution before they
        // leave.
        //

        pcdev = pcdmlModes->LookUp(this->iResolution,
                                   this->iColor,
                                   this->iFrequency);

        if (!gUnattenedAutoConfirm) {

            if (!(pcdev->bModeTested())) {

                //
                // Put up a pop asking the user if they really want to save
                // this selection.
                //
                // Only support this feature in normal operation
                //

                if ( (gbExecMode == EXEC_SETUP) ||
                     (gbExecMode == EXEC_DETECT) ) {

                    FmtMessageBox(this->hwnd,
                                  MB_ICONSTOP,
                                  FALSE,
                                  ID_DSP_TXT_SETTINGS,
                                  ID_DSP_TXT_MODE_UNTESTED);

                    SetWindowLong(this->hwnd,
                                  DWL_MSGRESULT,
                                  PSNRET_INVALID_NOCHANGEPAGE);

                    break;

                } else {

                    if (FmtMessageBox(this->hwnd,
                                      MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONSTOP,
                                      FALSE,
                                      ID_DSP_TXT_SETTINGS,
                                      ID_DSP_TXT_MODE_UNTESTED_RESTART) == IDOK)
                    {
                        //
                        // The user pressed OK.
                        // Let's assume the mode works
                        //

                       pcdev->vTestMode(TRUE);
                    }
                    else
                    {
                        //
                        // The user cancelled, so go back to the app.
                        //

                        SetWindowLong(this->hwnd,
                                      DWL_MSGRESULT,
                                      PSNRET_INVALID_NOCHANGEPAGE);

                        break;
                    }
                }
            }
        }

        {
            //
            // Clean up (will only happen in setup\detect modes)
            // The user succesfully tested, so we want to keep the drivers.
            //

            CREGCLEANUP cregClean;
            cregClean.vClean(FALSE);

        }

        //
        // Save all values out to the registry.
        //

        if (!SaveParamsToReg()) {
            SetWindowLong(this->hwnd, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            break;
        }

        //
        // Change the resolution on the fly, if we are not in setup.
        //

        if ( (gbExecMode == EXEC_NORMAL) ||
             (gbExecMode == EXEC_INVALID_MODE) ||
             (gbExecMode == EXEC_DETECT) ) {

            //
            // Try to change the resolution on the fly to the new settings
            // we saved in the registry.
            //
            // If it fails, request a reboot.
            //

            if ((fActiveDsp == FALSE) ||
                (ChangeDisplaySettings(NULL, NULL) != DISP_CHANGE_SUCCESSFUL))
            {
                PropSheet_RestartWindows(ghwndPropSheet);
            }
        }

        SetWindowLong(this->hwnd, DWL_MSGRESULT, PSNRET_NOERROR);

        break;


    case PSN_QUERYCANCEL:

        //
        // Clean up *everything* (will only happen in setup\detect modes)
        // We will boot in vga mode next time !
        //
        // HOWEVER, nothing will happen if the configure at logon value is set,
        // which means we will do everything on the next boot.
        //
        // If we did run with the active display, we can not clean everything
        // though (this must be a system with no VgaStart).
        //

        if (gUnattenedConfigureAtLogon == 0)
        {
            CREGCLEANUP cregClean;
            cregClean.vClean(TRUE && (!fActiveDsp));
        }

        //
        // If we break and return TRUE, that means we want to prevent CANCEL.
        // What actually happens in the property sheet stuff is that we get
        // called back with a PSN_RESET call which causes the property sheet
        // to go away anyways.
        //
        // So let just return FALSE here for now.
        // Later, when we have an APPLY call, then we may want to deal with
        // this message differently.
        //

        return FALSE;

    case PSN_SETACTIVE:

        this->InitEverything();
        return FALSE;

    default:
        return FALSE;
    }

    return TRUE;
}


//
// DoCommand method
//
BOOL CDISPLAYDLG::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    int iFreq, iRes, iClr;
    PCDEVMODE pcdev;

    switch (idControl) {

    case ID_DSP_TEST: {

        HANDLE hThread;
        DWORD idThread;
        DWORD bTest;
        NEW_DESKTOP_PARAM desktopParam;

        //
        // Warn the user
        //

        if (FmtMessageBox(this->hwnd,
                          MB_OKCANCEL | MB_ICONINFORMATION,
                          FALSE,
                          ID_DSP_TXT_TEST_MODE,
                          ID_DSP_TXT_DID_TEST_WARNING)
            == IDCANCEL) {

            break;

        }

        /*
         * The user wants to test his new selection.  We need to
         * create a new desktop with the selected resolution, switch
         * to it, and put up a dialog box so he can see if it works with
         * his hardware.
         *
         * All this needs to be done in a separate thread since you can't
         * switch a thread with open windows to a new desktop.
         */

        //
        // Disable this window so we can simulate a modal dialog in a different
        // desktop.
        //

        this->Disable();

        //
        // Create the test thread.  It will do the work of creating the desktop
        //

        pcdev = pcdmlModes->LookUp(this->iResolution,
                                   this->iColor,
                                   this->iFrequency);

        ASSERT(pcdev != NULL);

        desktopParam.lpdevmode = pcdev->GetData();
        desktopParam.pwszDevice = this->pszDisplay;

        hThread = CreateThread(NULL,
                               CB_THREAD_STACK,
                               ApplyNowThd,
                               (LPVOID) (&desktopParam),
                               SYNCHRONIZE | THREAD_QUERY_INFORMATION,
                               &idThread);

        WaitForSingleObject(hThread, INFINITE);

        GetExitCodeThread(hThread, &bTest);

        //
        // clean up un-needed non-paged pool space
        //

        CloseHandle(hThread);

        //
        // If the user saw what they expected, then we want to save the results
        //

        if ((bTest) &&
             (FmtMessageBox(this->hwnd,
                            MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2,
                            FALSE,
                            ID_DSP_TXT_TEST_MODE,
                            ID_DSP_TXT_DID_TEST_RESULT)
              == IDYES) ) {

            //
            // Mark this mode as tested.
            //

            pcdev->vTestMode(TRUE);

            //
            // In setup mode, don't save the results.
            // They will be saved when the user exits the applet.
            //

            if (gbExecMode == EXEC_SETUP) {

                FmtMessageBox(this->hwnd,
                              0,
                              FALSE,
                              ID_DSP_TXT_SETTINGS,
                              ID_DSP_TXT_SETUP_SAVE);
            }

        } else {

            FmtMessageBox(this->hwnd,
                          MB_ICONEXCLAMATION | MB_OK,
                          FALSE,
                          ID_DSP_TXT_TEST_MODE,
                          ID_DSP_TXT_TEST_FAILED);

        }

        this->Enable();
        SetForegroundWindow(this->hwnd);

        break;
    }

    case ID_DSP_FONTSIZE:

        switch(iNoteCode) {

        case CBN_SELCHANGE:

            //
            // Can not change a font during setup.
            // run the control panel later on.
            //

            if (gbExecMode == EXEC_SETUP) {

                ULONG i;

                FmtMessageBox(this->hwnd,
                              MB_ICONINFORMATION,
                              FALSE,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_FONT_IN_SETUP_MODE);

                //
                // Reset the value in the listbox.
                // We only have two entries for now !
                //

                for (i=0; i <=1; i++)
                {
                    if (this->SendCtlMsg(ID_DSP_FONTSIZE,
                                         CB_GETITEMDATA, i, 0) == cLogPix)
                    {
                        this->SendCtlMsg(ID_DSP_FONTSIZE,
                                         CB_SETCURSEL, i, 0);
                    }
                }

            } else {

                //
                // Warn the USER font changes will not be seen until after
                // reboot
                //

                FmtMessageBox(this->hwnd,
                              MB_ICONINFORMATION,
                              FALSE,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_FONT_LATER);

            }

            break;

        default:

            break;

        }

        break;


    case ID_DSP_COLORBOX:

        switch(iNoteCode ) {

        case CBN_SELCHANGE:

            iClr = this->SendCtlMsg(ID_DSP_COLORBOX, CB_GETCURSEL, 0, 0);

            if (iClr != CB_ERR ) {

                //
                // Realize the new monitor mode
                //

                iFreq = this->iFrequency;
                iRes = this->iResolution;

                pcdmlModes->FindClosestMode(&iRes, iClr, &iFreq);

                this->SetCurResolution(iRes);
                this->SetCurFrequency(iFreq);
                this->SetCurColor(iClr);
            }

            break;

        default:
            break;
        }

        break;


    case ID_DSP_FREQ:

        switch(iNoteCode ) {

        case CBN_SELCHANGE:

            iFreq = this->SendCtlMsg(ID_DSP_FREQ, CB_GETCURSEL, 0, 0);
            if (iFreq != CB_ERR ) {
                int iClr, iRes;

                //
                // Realize the new monitor mode
                //

                iClr = this->iColor;
                iRes = this->iResolution;

                pcdmlModes->FindClosestMode(&iRes, &iClr, iFreq);

                this->SetCurResolution(iRes);
                this->SetCurFrequency(iFreq);
                this->SetCurColor(iClr);

            }
            break;

        default:
            break;
        }

        break;


    case ID_DSP_CHANGE: {

        CDLGCHGADAPTOR cdcaNewCard;

        LPTSTR lpszNewDevice = this->pszDisplay;
        int i;

        i = cdcaNewCard.Dialog(this->hmodModule, this->hwnd,
                (LPARAM)&lpszNewDevice);

        if (i != RET_NO_CHANGE && lpszNewDevice != NULL) {

            //
            // A new device has been installed.
            //
            // NOTE Disable saving the information to the registry until we
            // can actually load drivers on the fly.
            //

            bNewDriver = TRUE;
#if 0
        //
        // Re-enable this when we support mutiple devices again.
        //

            LPTSTR pszOldString;

            //
            // One should not be able to change the driver in one of these
            // modes. This would cause our cleanup code to screw up since
            // it would go and delete any new driver the user tried to
            // install.
            //

            if ( (gbExecMode == EXEC_SETUP) ||
                 (gbExecMode == EXEC_DETECT) ) {

                ASSERT(FALSE);

            }

            //
            // Delete the old string
            //

            pszOldString = this->pszDisplay;

            //
            // now try to see if we can setup the modes for the new device.
            //

            //
            // Build new lists
            //

            this->pszDisplay = CloneString(lpszNewDevice);

            if (this->InitEverything()) {

                //
                // We have modes on the new device.
                // Delete the old name
                //

                LocalFree(pszOldString);

            } else {

                //
                // Put a pop-up because we could not build a list for the
                // applet.
                //


                //
                // Could not get the modes.
                // Restore to old device.
                //

                LocalFree(this->pszDisplay);
                this->pszDisplay = pszOldString;

                //
                // Reset the table. This time is must work since it did last
                // time.
                //

                InitEverything();

            }
#endif

        }
        break;

    }

    case ID_DSP_LIST_ALL:
        {

        CDLGMODELIST cmodelist;
        LPDEVMODE lpdevmode;

        lpdevmode = (LPDEVMODE) cmodelist.Dialog(this->hmodModule,
                                                 this->hwnd,
                                                 (LPARAM)&(pcdmlModes));

        if (lpdevmode) {

            //
            // Realize the new monitor mode
            //

            iRes = pcdmlModes->IndexFromResXY(lpdevmode->dmPelsWidth,
                                              lpdevmode->dmPelsHeight);
            iFreq = pcdmlModes->IndexFromFreq(lpdevmode->dmDisplayFrequency);
            iClr = pcdmlModes->IndexFromColor(lpdevmode->dmBitsPerPel);

            this->SetCurResolution(iRes);
            this->SetCurFrequency(iFreq);
            this->SetCurColor(iClr);

        }
        }

        break;

    default:

        return FALSE;

    }

    //
    // Enable the apply button only if we are not in setup.
    //

    if ( (gbExecMode == EXEC_NORMAL) ||
         (gbExecMode == EXEC_INVALID_MODE) ||
         (gbExecMode == EXEC_DETECT) ) {

        //
        // Set the apply button if something changed
        //

        if ((iOrgResolution != iResolution) ||
            (iOrgColor      != iColor)      ||
            (iOrgFrequency  != iFrequency))
        {
            SendMessage(this->hwndParent, PSM_CHANGED, (WPARAM) this->hwnd, 0L);
        }
    }

    return TRUE;
}

//
// HScroll method
//  In this dialog, the HScroll method is used to work the resolution slider.
//
BOOL CDISPLAYDLG::HScroll(int idCtrl, int iCode, int iPos) {
    BOOL fRealize = TRUE;
    int iRes;

    iRes = this->iResolution;

    switch(iCode ) {
    case TB_LINEUP:
    case TB_PAGEUP:
        if (iRes != 0)
            iRes--;
        break;

    case TB_LINEDOWN:
    case TB_PAGEDOWN:
        if (++(iRes) >= this->cResolutions)
            iRes = this->cResolutions - 1;
        break;

    case TB_BOTTOM:
        iRes = this->cResolutions - 1;
        break;

    case TB_TOP:
        iRes = 0;
        break;

    case TB_THUMBTRACK:
        fRealize = FALSE;

    case TB_THUMBPOSITION:
        iRes = iPos;
        break;

    default:
        return FALSE;
    }


    if (fRealize) {

        int iFreq, iClr;
        iFreq = this->iFrequency;
        iClr = this->iColor;

        pcdmlModes->FindClosestMode(iRes, &iClr, &iFreq);

        this->SetCurFrequency(iFreq);
        this->SetCurColor(iClr);

    }

    this->SetCurResolution(iRes);

    //
    // Enable the apply button only if we are not in setup.
    //

    if ( (gbExecMode == EXEC_NORMAL) ||
         (gbExecMode == EXEC_INVALID_MODE) ||
         (gbExecMode == EXEC_DETECT) ) {

        //
        // Set the apply button if something changed
        //

        if ((iOrgResolution != iResolution) ||
            (iOrgColor      != iColor)      ||
            (iOrgFrequency  != iFrequency))
        {
            SendMessage(this->hwndParent, PSM_CHANGED, (WPARAM) this->hwnd, 0L);
        }
    }


    return TRUE;


    idCtrl; //there is only one hscroll in this dlg box
}

BOOL CDISPLAYDLG::SysColorChange( UINT msg, WPARAM wParam, LPARAM lParam ) {
    this->monitor.SysColorChange(msg, wParam, lParam);
    this->SendCtlMsg(ID_DSP_AREA_SB, msg, wParam, lParam);

    return FALSE;
}

//
// Called to put up initial messages that need to appear above the dialog
// box
//

BOOL CDISPLAYDLG::InitMessage(void) {


    //
    // If configure at logon is set, then we don't want to do anything
    //

    if (gUnattenedConfigureAtLogon)
    {
        PropSheet_PressButton(ghwndPropSheet, PSBTN_CANCEL);
    }
    else if (gUnattenedAutoConfirm)
    {
        PropSheet_PressButton(ghwndPropSheet, PSBTN_OK);
    }
    else
    {
        //
        // Put up a pop saying what kind of video card we detected.
        //

        if ( (gbExecMode == EXEC_SETUP) ||
             (gbExecMode == EXEC_DETECT) ) {


            CDLGSTARTUP startup(this->pszDisplay);

            startup.Dialog(this->hmodModule, this->hwnd);

        }

        //
        // bBadDriver will be set when we fail to build the list of modes,
        // or something else failed during initialization.
        //
        // In almost every case, we should already know about this situation
        // based on our boot code.
        // However, if this is a new situation, just report a "bad driver"
        //

        if (bBadDriver)
        {
            ASSERT(gbExecMode == EXEC_INVALID_MODE);

            gbExecMode = EXEC_INVALID_MODE;
            gbInvalidMode = EXEC_INVALID_DISPLAY_DRIVER;

        }


        if (gbExecMode == EXEC_INVALID_MODE)
        {
            DWORD Mesg;

            switch(gbInvalidMode) {

            case EXEC_INVALID_NEW_DRIVER:
                Mesg = MSG_INVALID_NEW_DRIVER;
                break;
            case EXEC_INVALID_DEFAULT_DISPLAY_MODE:
                Mesg = MSG_INVALID_DEFAULT_DISPLAY_MODE;
                break;
            case EXEC_INVALID_DISPLAY_DRIVER:
                Mesg = MSG_INVALID_DISPLAY_DRIVER;
                break;
            case EXEC_INVALID_OLD_DISPLAY_DRIVER:
                Mesg = MSG_INVALID_OLD_DISPLAY_DRIVER;
                break;
            case EXEC_INVALID_16COLOR_DISPLAY_MODE:
                Mesg = MSG_INVALID_16COLOR_DISPLAY_MODE;
                break;
            case EXEC_INVALID_DISPLAY_MODE:
                Mesg = MSG_INVALID_DISPLAY_MODE;
                break;
            case EXEC_INVALID_CONFIGURATION:
            default:
                Mesg = MSG_INVALID_CONFIGURATION;
                break;
            }

            FmtMessageBox(this->hwnd,
                          MB_ICONEXCLAMATION,
                          FALSE,
                          MSG_CONFIGURATION_PROBLEM,
                          Mesg);

            //
            // For a bad display driver or old display driver, let's send the
            // user straight to the installation dialog.
            //

            if ((gbInvalidMode == EXEC_INVALID_OLD_DISPLAY_DRIVER) ||
                (gbInvalidMode == EXEC_INVALID_DISPLAY_DRIVER))
            {
                CREGVIDOBJ rvoVideo(pszDisplay);
                BOOL unused;

                if (InstallNewDriver(this->hwnd,
                                     rvoVideo.CloneDescription(),
                                     FALSE,
                                     &unused) == NO_ERROR)
                {
                    //
                    // Set this flag so that we don't get a message about
                    // having an untested mode.
                    //

                    bNewDriver = TRUE;

                    //
                    // Let's leave the applet so the user sees the reboot
                    // popup.
                    //

                    PropSheet_PressButton(ghwndPropSheet, PSBTN_OK);
                }
            }
        }
    }

    return TRUE;
}

//
// deterines if the applet is in detect mode.
//

VOID CDISPLAYDLG::vPreExecMode() {

    HKEY hkey;
    DWORD cb;
    DWORD data;

    //
    // This function sets up the execution mode of the applet.
    // There are four vlid modes.
    //
    // EXEC_NORMAL - When the apple is launched from the control panel
    //
    // EXEC_INVALID_MODE is exactly the same as for NORMAL except we will
    //                   not mark the current mode as tested so the user has
    //                   to at least test a mode
    //
    // EXEC_DETECT - When the applet is launched normally, but a detect was
    //               done on the previous boot (the key in the registry is
    //               set)
    //
    // EXEC_SETUP  - When we launch the applet in setup mode from setup (Both
    //               the registry key is set and the setup flag is passed in).
    //

    //
    // These two keys should only be checked \ deleted if the machine has been
    // rebooted and the detect \ new display has actually happened.
    // So we will look for the RebootNecessary key (a volatile key) and if
    // it is not present, then we can delete the key.  Otherwise, the reboot
    // has not happened, and we keep the key
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_REBOOT_NECESSARY,
                     0,
                     KEY_READ | KEY_WRITE,
                     &hkey) != ERROR_SUCCESS) {

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        SZ_DETECT_DISPLAY,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkey) == ERROR_SUCCESS) {

            //
            // NOTE: This key is also set when EXEC_SETUP is being run.
            //

            if (gbExecMode == EXEC_NORMAL) {

                gbExecMode = EXEC_DETECT;

            } else {

                //
                // If we are in setup mode, we also check the extra values
                // under DetectDisplay that control the unattended installation.
                //

                ASSERT(gbExecMode == EXEC_SETUP);

                cb = 4;
                if (RegQueryValueEx(hkey,
                                    SZ_CONFIGURE_AT_LOGON,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedConfigureAtLogon,
                                    &cb) == ERROR_SUCCESS) {

                    //
                    // We delete only this value since the other values must remain
                    // until the next boot
                    //

                    RegDeleteValue(hkey,
                                   SZ_CONFIGURE_AT_LOGON);
                }

                if (gUnattenedConfigureAtLogon == 0)
                {

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_INSTALL,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedInstall,
                                    &cb);

                    cb = sizeof(gUnattenedPszInf);
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_INF,
                                    NULL,
                                    NULL,
                                    (LPBYTE) gUnattenedPszInf,
                                    &cb);

                    cb = sizeof(gUnattenedPszOption);
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_OPTION,
                                    NULL,
                                    NULL,
                                    (LPBYTE) gUnattenedPszOption,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_BPP,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedBitsPerPel,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_X,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedXResolution,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_Y,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedYResolution,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_REF,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedVRefresh,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_FLAGS,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedFlags,
                                    &cb);

                    cb = 4;
                    RegQueryValueEx(hkey,
                                    SZ_UNATTEND_CONFIRM,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &gUnattenedAutoConfirm,
                                    &cb);
                }
            }

            RegCloseKey(hkey);
        }

        //
        // Check for a new driver being installed
        //

        if ( (gbExecMode == EXEC_NORMAL) &&
             (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           SZ_NEW_DISPLAY,
                           0,
                           KEY_READ | KEY_WRITE,
                           &hkey) == ERROR_SUCCESS) ) {

            gbExecMode = EXEC_INVALID_MODE;
            gbInvalidMode = EXEC_INVALID_NEW_DRIVER;

            RegCloseKey(hkey);
        }

        RegDeleteKey(HKEY_LOCAL_MACHINE,
                     SZ_DETECT_DISPLAY);

        RegDeleteKey(HKEY_LOCAL_MACHINE,
                     SZ_NEW_DISPLAY);
    }
{
    LPTSTR psz;
    LPTSTR pszInv;

    switch(gbExecMode) {

    case EXEC_NORMAL:
        psz = L"Normal Execution mode";
        break;
    case EXEC_DETECT:
        psz = L"Detection Execution mode";
        break;
    case EXEC_SETUP:
        psz = L"Setup Execution mode";
        break;
    case EXEC_INVALID_MODE:
        psz = L"Invalid Mode Execution mode";

        switch(gbInvalidMode) {

        case EXEC_INVALID_NEW_DRIVER:
            pszInv = L"Invalid new driver";
            break;
        default:
            psz = L"*** Invalid *** Invalid mode";
            break;
        }
        break;
    default:
        psz = L"*** Invalid *** Execution mode";
        break;
    }

    KdPrint(("\n \nDisplay.cpl: The display applet is in : %ws\n", psz));

    if (gbExecMode == EXEC_INVALID_MODE)
    {
        KdPrint(("\t\t sub invalid mode : %ws", pszInv));
    }
    KdPrint(("\n\n", psz));
}
}


VOID CDISPLAYDLG::vPostExecMode() {

    HKEY hkey;
    DWORD cb;
    DWORD data;

    //
    // Check for various invalid configurations
    //

    if ( (gbExecMode == EXEC_NORMAL) &&
         (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       SZ_INVALID_DISPLAY,
                       0,
                       KEY_READ | KEY_WRITE,
                       &hkey) == ERROR_SUCCESS) ) {

        gbExecMode = EXEC_INVALID_MODE;

        //
        // Check for these fields in increasing order of "badness" or
        // "detail" so that the *worst* error is the one remaining in the
        // gbInvalidMode  variable once all the checks are done.
        //

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("DefaultMode"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_DEFAULT_DISPLAY_MODE;
        }

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("BadMode"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_DISPLAY_MODE;
        }

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("16ColorMode"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_16COLOR_DISPLAY_MODE;
        }


        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("InvalidConfiguration"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_CONFIGURATION;
        }

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("MissingDisplayDriver"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_DISPLAY_DRIVER;
        }

        //
        // This last case will be set in addition to the previous one in the
        // case where the driver was an old driver linking to winsvr.dll
        // and we can not load it.
        //

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("OldDisplayDriver"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_OLD_DISPLAY_DRIVER;
        }

        RegCloseKey(hkey);

    }

    //
    // Delete all of these bad configuration keys since we only want the
    // user to see the message once.
    //

    RegDeleteKey(HKEY_LOCAL_MACHINE,
                 SZ_INVALID_DISPLAY);

{
    LPTSTR psz;
    LPTSTR pszInv;

    if (gbExecMode == EXEC_INVALID_MODE)
    {
        switch (gbInvalidMode)
        {
        case EXEC_INVALID_DEFAULT_DISPLAY_MODE:
            pszInv = L"Default mode being used";
            break;
        case EXEC_INVALID_DISPLAY_DRIVER:
            pszInv = L"Invalid Display Driver";
            break;
        case EXEC_INVALID_OLD_DISPLAY_DRIVER:
            pszInv = L"Old Display Driver";
            break;
        case EXEC_INVALID_16COLOR_DISPLAY_MODE:
            pszInv = L"16 color mode not supported";
            break;
        case EXEC_INVALID_DISPLAY_MODE:
            pszInv = L"Invalid display mode";
            break;
        case EXEC_INVALID_CONFIGURATION:
            pszInv = L"Invalid configuration";
            break;
        default:
            psz = L"*** Invalid *** Invalid mode";
            break;
        }

        KdPrint(("\t\t sub invlid mode : %ws", pszInv));
        KdPrint(("\n\n", psz));
    }
}
}


//
// Paint method
//  Paints the dialog box.  In this case, it shows the Monitor picture
//  and the color bar.
//

BOOL CDISPLAYDLG::Paint(void) {
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(this->hwnd, &ps);

    this->monitor.Paint(hdc, &(ps.rcPaint));
    this->clrbar.Paint(hdc, &(ps.rcPaint));

    EndPaint(this->hwnd, &ps);
    return TRUE;
}

/***************************************************************************\
* NewDisplayDialogBox
*
*   Returns a DlgProc for our C++ display dialog
*
* History:
* 10-May-1995 JonPa     Created it
\***************************************************************************/
DLGPROC NewDisplayDialogBox(HINSTANCE hmod, LPARAM *lplParam) {
    CDISPLAYDLG *pddDialog;

    pddDialog = new CDISPLAYDLG;

    pddDialog->hwndParent = NULL;
    pddDialog->hmodModule = hmod;
    pddDialog->lParam = 0;

    *lplParam = (LPARAM)pddDialog;

    return DisplayPageProc;
}

/***************************************************************************\
* DeleteDisplayDialogBox
*
*   Deletes the C++ objects associated with a display dialog box
*
* History:
* 10-May-1995 JonPa     Created it
\***************************************************************************/
void DeleteDisplayDialogBox( LPARAM lParam ) {
    CDISPLAYDLG *pddDialog;

    pddDialog = (CDISPLAYDLG *)lParam;

    delete pddDialog;
}


/***************************************************************************\
* DisplayPageProc
*
*   Dialog Proc callable from PropertyPage code.
*
* History:
* 10-May-1995 JonPa     Created it
\***************************************************************************/
BOOL CALLBACK DisplayPageProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (msg == WM_INITDIALOG)
    {
        ghwndPropSheet = GetParent(hwnd);

        return DisplayDlgProc(hwnd, msg, wParam, ((PROPSHEETPAGE *)lParam)->lParam );
    }
    else
        return DisplayDlgProc(hwnd, msg, wParam, lParam );
}



/***************************************************************************\
* DisplayDlgProc
*
*
* History:
* 23-Sep-1993 JonPa     Created it
\***************************************************************************/

BOOL CALLBACK DisplayDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PCDIALOG pdlg;

    if (msg == WM_INITDIALOG) {
        /*
         * Set up Object/Window relationship
         */
        SetWindowLong(hwnd, DWL_USER, lParam);
        pdlg = (PCDIALOG) lParam;
        pdlg->SetHWnd(hwnd);
        pdlg->SetHWndParent(GetParent(hwnd));
    }

    pdlg = (PCDIALOG)GetWindowLong(hwnd, DWL_USER);



    if (pdlg != NULL) {
        /*
         * Dispatch to the Dialog Object's WndProc method
         */
        return pdlg->WndProc(msg, wParam, lParam);

    } else {

#if 0
        /*
         * We have not setup the Object/Dialog relationship yet,
         * just let DefDlgProc handle this message, (unless it is
         * a help request).
         */
        if (msg == wHelpMessage) {
            DspHelp(hwnd);
            return TRUE;
        }
#endif
    }

    return FALSE;
}



/***************************************************************************\
*
*     FUNCTION: FmtMessageBox(HWND hwnd, int dwTitleID, UINT fuStyle,
*                   BOOL fSound, DWORD dwTextID, ...);
*
*     PURPOSE:  Formats messages with FormatMessage and then displays them
*               in a message box
*
*     PARAMETERS:
*               hwnd        - parent window for message box
*               fuStyle     - MessageBox style
*               fSound      - if TRUE, MessageBeep will be called with fuStyle
*               dwTitleID   - Message ID for optional title, "Error" will
*                             be displayed if dwTitleID == -1
*               dwTextID    - Message ID for the message box text
*               ...         - optional args to be embedded in dwTextID
*                             see FormatMessage for more details
* History:
* 22-Apr-1993 JonPa         Created it.
\***************************************************************************/
int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    BOOL fSound,
    DWORD dwTitleID,
    DWORD dwTextID, ... )
{

    LPTSTR pszMsg;
    LPTSTR pszTitle;
    int idRet;

    va_list marker;

    va_start(marker, dwTextID);

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       ghmod,
                       dwTextID,
                       0,
                       (LPTSTR)&pszMsg,
                       1,
                       &marker)) {

        pszMsg = gpszError;

    }

    va_end(marker);

    GetLastError();

    pszTitle = NULL;

    if (dwTitleID != -1) {

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_HMODULE |
                          FORMAT_MESSAGE_MAX_WIDTH_MASK |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      ghmod,
                      dwTitleID,
                      0,
                      (LPTSTR)&pszTitle,
                      1,
                      NULL);
                      //(va_list *)&pszTitleStr);

    }

    //
    // Turn on the beep if requested
    //

    if (fSound) {
        MessageBeep(fuStyle & (MB_ICONASTERISK | MB_ICONEXCLAMATION |
                MB_ICONHAND | MB_ICONQUESTION | MB_OK));
    }

    idRet = MessageBox(hwnd, pszMsg, pszTitle, fuStyle);

    if (pszTitle != NULL)
        LocalFree(pszTitle);

    if (pszMsg != gpszError)
        LocalFree(pszMsg);

    return idRet;
}

/***************************************************************************\
*
*     FUNCTION: FmtSprint(DWORD id, ...);
*
*     PURPOSE:  sprintf but it gets the pattern string from the message rc,
*               and allocates the buffer for the end result.
*
* History:
* 03-May-1993 JonPa         Created it.
\***************************************************************************/
LPTSTR FmtSprint(DWORD id, ... ) {
    LPTSTR pszMsg;
    va_list marker;

    va_start(marker, id);

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       ghmod,
                       id,
                       0,
                       (LPTSTR)&pszMsg,
                       1,
                       &marker)) {


        GetLastError();
        pszMsg = TEXT("...");
    }
    va_end(marker);

    return pszMsg;
}

/****************************************************************************\
*
* LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan )
*
*   If pszScan starts with pszTarget, then the function returns the first
* char of pszScan that follows the pszTarget; other wise it returns pszScan.
*
* eg: SubStrEnd("abc", "abcdefg" ) ==> "defg"
*     SubStrEnd("abc", "abZQRT" ) ==> "abZQRT"
*
* History:
*   09-Dec-1993 JonPa   Wrote it.
\****************************************************************************/
LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan ) {
    int i;

    for (i = 0; pszScan[i] != TEXT('\0') && pszTarget[i] != TEXT('\0') &&
            CharUpper(CHARTOPSZ(pszScan[i])) ==
            CharUpper(CHARTOPSZ(pszTarget[i])); i++);

    if (pszTarget[i] == TEXT('\0')) {

        // we found the substring
        return pszScan + i;
    }

    return pszScan;
}

/****************************************************************************\
*
* CloneString
*
* Makes a copy of a string.  By copy, I mean it actually allocs memeory
* and copies the chars across.
*
* NOTE: the caller must LocalFree the string when they are done with it.
*
* Returns a pointer to a LocalAlloced buffer that has a copy of the
* string in it.  If an error occurs, it retuns NULl.
*
* 16-Dec-1993 JonPa     Wrote it.
\****************************************************************************/
LPTSTR CloneString(LPTSTR psz ) {
    int cb;
    LPTSTR psz2;

    if (psz == NULL)
        return NULL;

    cb = (lstrlen(psz) + 1) * sizeof(TCHAR);

    psz2 = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cb);
    if (psz2 != NULL)
        CopyMemory(psz2, psz, cb);

    return psz2;
}


/****************************************************************************\
*
* DWORD WINAPI ApplyNowThd(LPVOID lpThreadParameter)
*
* Thread that gets started when the use hits the Apply Now button.
* This thread creates a new desktop with the new video mode, switches to it
* and then displays a dialog box asking if the display looks OK.  If the
* user does not respond within the time limit, then 'NO' is assumed to be
* the answer.
*
\****************************************************************************/
DWORD WINAPI ApplyNowThd(LPVOID lpThreadParameter)
{

    PNEW_DESKTOP_PARAM lpDesktopParam = (PNEW_DESKTOP_PARAM) lpThreadParameter;
    HDESK hdsk = NULL;
    HDESK hdskDefault = NULL;
    BOOL bTest = FALSE;
    HDC hdc;

    //
    // HACK:
    // We need to make a USER call before calling the desktop stuff so we can
    // sure our threads internal data structure are associated with the default
    // desktop.
    // Otherwise USER has problems closing the desktop with our thread on it.
    //

    GetSystemMetrics(SM_CXSCREEN);

    //
    // Create the desktop
    //

    hdskDefault = GetThreadDesktop(GetCurrentThreadId());

    if (hdskDefault != NULL) {

        hdsk = CreateDesktop(TEXT("Display.Cpl Desktop"),
                             lpDesktopParam->pwszDevice,
                             lpDesktopParam->lpdevmode,
                             0,
                             MAXIMUM_ALLOWED,
                             NULL);

        if (hdsk != NULL) {

            //
            // use the desktop for this thread
            //

            if (SetThreadDesktop(hdsk)) {

                hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

                if (hdc) {

                    DrawBmp(hdc);

                    DeleteDC(hdc);

                    bTest = TRUE;
                }

                //
                // Sleep for some seconds so you have time to look at the screen.
                //

                Sleep(7000);

            }
        }


        //
        // Reset the thread to the right desktop
        //

        SetThreadDesktop(hdskDefault);

        SwitchDesktop(hdskDefault);

        //
        // Can only close the desktop after we have switched to the new one.
        //

        if (hdsk != NULL)
            CloseDesktop(hdsk);

    }

    ExitThread((DWORD) bTest);

    return 0;
}
