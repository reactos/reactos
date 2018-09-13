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


/*****************************************************************\
*
* CDIALOG class
*
\*****************************************************************/
typedef class CDISPLAYDLG *PCDISPLAYDLG;

class CDIALOG {

protected:
    HWND hwnd;
    HWND hwndParent;
    HINSTANCE hmodModule;
    LPARAM lParam;
    int  iDlgResID;
    void SetHWnd(HWND hwndNew) { this->hwnd = hwndNew; }
    void SetHWndParent(HWND hwndNew) { this->hwndParent = hwndNew; }
    friend BOOL CALLBACK CDialogProc(HWND hwnd, UINT msg, WPARAM wParam,
                            LPARAM lParam);

    friend PCDISPLAYDLG NewDisplayDialogBox(HINSTANCE hmod, LPARAM lParam);


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
    virtual BOOL SysColorChange(void)   { return FALSE; }
    virtual BOOL DoHelp( LPHELPINFO lphi );
    virtual BOOL DoContextMenu( HWND hwnd, WORD xPos, WORD yPos );

};

typedef CDIALOG *PCDIALOG;


/*****************************************************************\
*
* CDLGCHGADAPTOR class
*
*        derived from CDIALOG
*
\*****************************************************************/
typedef class CDLGCHGADAPTOR *PCDLGCHGADAPTOR;

class CDLGCHGADAPTOR : public CDIALOG {
protected:
    int iRet;

    friend BOOL CALLBACK AdvAdaptorPageProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

public:
    CDLGCHGADAPTOR();

    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
    virtual BOOL DoNotify(int idControl, NMHDR *lpnmh, UINT iNoteCode );
};

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

    BOOL ReconstructModeMatrix();
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

//typedef CDEVMODELST *PCDEVMODELST;
extern void *  __cdecl operator new(unsigned int nSize);
extern void  __cdecl operator delete(void *pv);
extern "C" int __cdecl _purecall(void);
