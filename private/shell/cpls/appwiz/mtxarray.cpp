//+-----------------------------------------------------------------------
//
//  Matrix Array
//
//------------------------------------------------------------------------


#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include <shstr.h>
#include "mtxarray.h"
#include "adcctl.h"         // for EnumArea string constants
#include "util.h"           // for ReleaseShellCategory
#include "dump.h"
#include "appwizid.h"

#ifdef WINNT
#include <tsappcmp.h>       // for TermsrvAppInstallMode
#endif

//--------------------------------------------------------------------
//
//
//  CAppData class
//
//
//--------------------------------------------------------------------

#define MAX_DATE_SIZE   50

#define IFS_APPINFODATA      1
#define IFS_SHELLCAT         2
#define IFS_CAPABILITY       3
#define IFS_SUPPORTINFO      4
#define IFS_INDEXLABEL       5
#define IFS_INDEXVALUE       6
#define IFS_PROPERTIES       7
#define IFS_SIZE             8
#define IFS_TIMESUSED        9
#define IFS_LASTUSED         10
#define IFS_OCSETUP          11
#define IFS_SCHEDULE         12
#define IFS_ICON             13
#define IFS_ISINSTALLED      14


const APPFIELDS g_rginstfields[] = {
    { /* 0 */ L"displayname",       SORT_NAME,      IFS_APPINFODATA, VT_BSTR, FIELD_OFFSET(APPINFODATA, pszDisplayName) },
    { /* 1 */ L"size",              SORT_SIZE,      IFS_SIZE,        VT_BSTR, 0 },
    { /* 2 */ L"timesused",         SORT_TIMESUSED, IFS_TIMESUSED,   VT_BSTR, 0 },
    { /* 3 */ L"lastused",          SORT_LASTUSED,  IFS_LASTUSED,    VT_BSTR, 0 },
    { /* 4 */ L"capability",        SORT_NA,        IFS_CAPABILITY , VT_UI4, 0 },
    { /* 5 */ L"supportinfo",       SORT_NA,        IFS_SUPPORTINFO, VT_BSTR, 0 },
    { /* 6 */ L"indexlabel",        SORT_NA,        IFS_INDEXLABEL,  VT_BSTR, 0 },
    { /* 7 */ L"indexvalue",        SORT_NA,        IFS_INDEXVALUE,  VT_BSTR, 0 },
    { /* 8 */ L"htmlproperties",    SORT_NA,        IFS_PROPERTIES,  VT_BSTR, 0 },  
    { /* 9 */ L"icon",              SORT_NA,        IFS_ICON,        VT_BSTR, 0 },    
};

const APPFIELDS g_rgpubfields[] = {
    { /* 0 */ L"displayname",       SORT_NAME,  IFS_APPINFODATA, VT_BSTR, FIELD_OFFSET(APPINFODATA, pszDisplayName) },
    { /* 1 */ L"capability",        SORT_NA,    IFS_CAPABILITY , VT_UI4, 0 },
    { /* 2 */ L"supporturl",        SORT_NA,    IFS_APPINFODATA, VT_BSTR, FIELD_OFFSET(APPINFODATA, pszSupportUrl) },
    { /* 3 */ L"htmlproperties",    SORT_NA,    IFS_PROPERTIES,  VT_BSTR, 0 },
    { /* 4 */ L"addlaterschedule",  SORT_NA,    IFS_SCHEDULE,    VT_BSTR, 0 },
    { /* 5 */ L"isinstalled",       SORT_NA,    IFS_ISINSTALLED, VT_BSTR, 0 },
};

const APPFIELDS g_rgsetupfields[] = {
    { /* 0 */ L"displayname",       SORT_NAME,  IFS_OCSETUP, VT_BSTR, FIELD_OFFSET(COCSetupApp, _szDisplayName) },
};

const APPFIELDS g_rgcatfields[] = {
    { /* 0 */ L"DisplayName",       SORT_NAME,  IFS_SHELLCAT, VT_BSTR, FIELD_OFFSET(SHELLAPPCATEGORY, pszCategory) },
    { /* 1 */ L"ID",                SORT_NA,    IFS_SHELLCAT, VT_UI4, FIELD_OFFSET(SHELLAPPCATEGORY, idCategory) },
};





// Overloaded constructor
CAppData::CAppData(IInstalledApp* pia, APPINFODATA* paid, PSLOWAPPINFO psai) : _cRef(1)
{
    _pia = pia;
    CopyMemory(&_ai, paid, sizeof(_ai));
    //NOTE: psai can be NULL
    if (psai)
    {
        CopyMemory(&_aiSlow, psai, sizeof(_aiSlow)); 
        
        // Let's massage some values.  
        _MassageSlowAppInfo();
    }
    _dwEnum = ENUM_INSTALLED;
    InitializeCriticalSection(&_cs);
    _fCsInitialized = TRUE;
}

// Overloaded constructor
CAppData::CAppData(IPublishedApp* ppa, APPINFODATA* paid, PUBAPPINFO * ppai) : _cRef(1)
{
    _ppa = ppa;
    CopyMemory(&_ai, paid, sizeof(_ai));
    CopyMemory(&_pai, ppai, sizeof(_pai));
    _dwEnum = ENUM_PUBLISHED;
}

// Overloaded constructor
CAppData::CAppData(COCSetupApp * pocsa, APPINFODATA* paid) : _cRef(1)
{
    _pocsa = pocsa;
    CopyMemory(&_ai, paid, sizeof(_ai));
    _dwEnum = ENUM_OCSETUP;
}

// Overloaded constructor
CAppData::CAppData(SHELLAPPCATEGORY * psac) : _cRef(1)
{
    _psac = psac;
    _dwEnum = ENUM_CATEGORIES;
}

// destructor
CAppData::~CAppData()
{
    if (ENUM_CATEGORIES == _dwEnum)
        ReleaseShellCategory(_psac);
    else if (ENUM_OCSETUP == _dwEnum)
    {
        delete _pocsa;
        ClearAppInfoData(&_ai);
    }
    else
    {
        // Release _pia or _ppa.  Take your pick.  Either way releases
        // the object because both inherit from IUnknown.
        _pia->Release();
        
        ClearAppInfoData(&_ai);
        ClearSlowAppInfo(&_aiSlow);
        ClearPubAppInfo(&_pai);
    }

    if (_fCsInitialized)
        DeleteCriticalSection(&_cs);
}


/*--------------------------------------------------------------------
Purpose: IUnknown::QueryInterface
*/
STDMETHODIMP CAppData::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CAppData, IAppData),
        { 0 },
    };

    return QISearch(this, (LPCQITAB)qit, riid, ppvObj);
}


STDMETHODIMP_(ULONG) CAppData::AddRef()
{
    InterlockedIncrement(&_cRef);
    TraceAddRef(CAppData, _cRef);
    return _cRef;
}


STDMETHODIMP_(ULONG) CAppData::Release()
{
    ASSERT(_cRef > 0);
    InterlockedDecrement(&_cRef);

    TraceRelease(CAppData, _cRef);
    
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


/*-------------------------------------------------------------------------
Purpose: Massage some values so they are sorted correctly
*/
void CAppData::_MassageSlowAppInfo(void)
{
    // We don't tell the difference b/t unknown app sizes and zero 
    // app sizes, so to make sorting easier, change the unknown app 
    // size to zero.
    if (-1 == (__int64)_aiSlow.ullSize)
        _aiSlow.ullSize = 0;

    // Unmarked last-used fields will get marked as 'not used', so
    // they are sorted correctly.
    if (0 == _aiSlow.ftLastUsed.dwHighDateTime && 
        0 == _aiSlow.ftLastUsed.dwLowDateTime)
    {
        _aiSlow.ftLastUsed.dwHighDateTime = NOTUSED_HIGHDATETIME;
        _aiSlow.ftLastUsed.dwLowDateTime = NOTUSED_LOWDATETIME;
    }
}


/*-------------------------------------------------------------------------
Purpose: IAppData::ReadSlowData

         Read the slow app data.  Call GetSlowDataPtr() to get it.
*/
STDMETHODIMP CAppData::ReadSlowData(void)
{
    HRESULT hres = E_FAIL;
    
    if (ENUM_INSTALLED == _dwEnum && _pia)
    {
        SLOWAPPINFO sai = {0};
        hres = _pia->GetSlowAppInfo(&sai);
      
        if (S_OK == hres)
        {
            // Switch our current SLOWAPPINFO with the new one
            // This is necessary because our current one is from the GetCachedSlowAppInfo
            // It may not have the most up-to-date info.
            if (IsSlowAppInfoChanged(&_aiSlow, &sai))
            {
                EnterCriticalSection(&_cs);
                {
                    ClearSlowAppInfo(&_aiSlow);
                    _aiSlow = sai;
                    hres = S_OK;
                }
                LeaveCriticalSection(&_cs);

                // Let's massage some values.  
                _MassageSlowAppInfo();
            }
            else
                hres = S_FALSE;
        }        
    }

    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IAppData::GetDataPtr

         Returns the pointer to the internal data structure.  The
         caller must hold the appdata object to guarantee the pointer
         remains valid.
*/
STDMETHODIMP_(APPINFODATA *) CAppData::GetDataPtr(void)
{
    return &_ai;
}


/*-------------------------------------------------------------------------
Purpose: IAppData::GetSlowDataPtr

         Returns the pointer to the internal data structure for slow
         data.  The caller must hold the appdata object to guarantee the 
         pointer remains valid.
*/
STDMETHODIMP_(SLOWAPPINFO *) CAppData::GetSlowDataPtr(void)
{
    return &_aiSlow;
}


/*-------------------------------------------------------------------------
Purpose: Return the capability flags
*/
DWORD CAppData::_GetCapability(void)
{
    DWORD dwCapability = 0;
    
    if (ENUM_INSTALLED == _dwEnum || ENUM_PUBLISHED == _dwEnum)
    {
        // We can use _pia or _ppa for this method because both
        // inherit from IShellApp.
        _pia->GetPossibleActions(&dwCapability);
    }
    
    return dwCapability;
}


/*-------------------------------------------------------------------------
Purpose: Returns the current sort index
*/
DWORD CAppData::_GetSortIndex(void)
{
    DWORD dwRet = SORT_NAME;        // default
    
    if (_pmtxParent)
        _pmtxParent->GetSortIndex(&dwRet);

    return dwRet;
}


/*-------------------------------------------------------------------------
Purpose: Return the amount of disk space the app occupies.  

         Returns S_OK if the field is valid.
*/
HRESULT CAppData::_GetDiskSize(LPTSTR pszBuf, int cchBuf)
{
    HRESULT hres = S_FALSE;
    ULONGLONG ullSize = GetSlowDataPtr()->ullSize;

    if (pszBuf && 0 < cchBuf)
        *pszBuf = 0;

    // Is this size printable?
    if (-1 != (__int64)ullSize && 0 != (__int64)ullSize)
    {
        // Yes
        if (pszBuf)
            ShortSizeFormat64(ullSize, pszBuf);
        hres = S_OK;
    }

    return hres;
}


#define FREQDECAY_OFTEN     10


/*-------------------------------------------------------------------------
Purpose: IAppData::GetFrequencyOfUse

         Return friendly names that map from the frequency-of-use
         metric from the UEM.  Returns S_OK if the field is valid.
*/
STDMETHODIMP CAppData::GetFrequencyOfUse(LPTSTR pszBuf, int cchBuf)
{
    HRESULT hres;
    int iTemp = GetSlowDataPtr()->iTimesUsed;
    
    if (0 > iTemp)
    {
        if (pszBuf && 0 < cchBuf)
            *pszBuf = 0;
        hres = S_FALSE;
    }    
    else
    {
        UINT ids = IDS_OFTEN;
        
        if (2 >= iTemp)
            ids = IDS_RARELY;
        else if (FREQDECAY_OFTEN >= iTemp)
            ids = IDS_SOMETIMES;

        if (pszBuf)
            LoadString(g_hinst, ids, pszBuf, cchBuf);
            
        hres = S_OK;
    }        
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: Return the friendly date when the app was last used.

         Returns S_OK if the field is valid.
*/
HRESULT CAppData::_GetLastUsed(LPTSTR pszBuf, int cchBuf)
{
    HRESULT hres;
    FILETIME ft = GetSlowDataPtr()->ftLastUsed;
    
    if (NOTUSED_HIGHDATETIME == ft.dwHighDateTime && 
        NOTUSED_LOWDATETIME == ft.dwLowDateTime)
    {
        if (pszBuf && 0 < cchBuf)
            *pszBuf = 0;
        hres = S_FALSE;
    }
    else
    {
        DWORD dwFlags = FDTF_SHORTDATE;
        
        ASSERT(0 != ft.dwHighDateTime || 0 != ft.dwLowDateTime);
        
        if (pszBuf)
            SHFormatDateTime(&ft, &dwFlags, pszBuf, cchBuf);
        hres = S_OK;
    }
    
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: Build up a structured string that the html script can read and
         parse.  The Boilerplate JavaScript class assumes the fields
         follow this format:

         <fieldname1 "value1"><fieldname2 value2>...

         It can accept quoted or non-quoted values.  If the value has a '>' in
         it, then it should be quoted, otherwise the Boilerplate code will 
         get confused.

         The order of the fieldnames is not important, and some or all may
         be missing.
*/
LPTSTR CAppData::_BuildSupportInfo(void)
{
    const static struct 
    {
        LPWSTR pszFieldname;
        DWORD ibOffset;        // offset into structure designated by dwStruct
    } s_rgsupfld[] = {
        { L"comments",      FIELD_OFFSET(APPINFODATA, pszComments) }, 
        { L"contact",       FIELD_OFFSET(APPINFODATA, pszContact) },
        { L"displayname",   FIELD_OFFSET(APPINFODATA, pszDisplayName) },
        { L"helpurl",       FIELD_OFFSET(APPINFODATA, pszHelpLink) },
        { L"helpphone",     FIELD_OFFSET(APPINFODATA, pszSupportTelephone) },
        { L"productID",     FIELD_OFFSET(APPINFODATA, pszProductID) },
        { L"publisher",     FIELD_OFFSET(APPINFODATA, pszPublisher) },
        { L"readmeUrl",     FIELD_OFFSET(APPINFODATA, pszReadmeUrl) },
        { L"regcompany",    FIELD_OFFSET(APPINFODATA, pszRegisteredCompany) },
        { L"regowner",      FIELD_OFFSET(APPINFODATA, pszRegisteredOwner) },
        { L"supporturl",    FIELD_OFFSET(APPINFODATA, pszSupportUrl) },
        { L"updateinfoUrl", FIELD_OFFSET(APPINFODATA, pszUpdateInfoUrl) },
        { L"version",       FIELD_OFFSET(APPINFODATA, pszVersion) },
    };

    ShStr shstr;
    int i;

    // Now populate the structured string
    for (i = 0; i < ARRAYSIZE(s_rgsupfld); i++)
    {
        LPWSTR pszT = *(LPWSTR *)((LPBYTE)&_ai + s_rgsupfld[i].ibOffset);
        if (pszT && *pszT)
        {
            TCHAR sz[64];

            wsprintf(sz, TEXT("<%s \""), s_rgsupfld[i].pszFieldname);
            shstr.Append(sz);
            shstr.Append(pszT);
            shstr.Append(TEXT("\">"));
        }
    }

    return shstr.CloneStr();
}


#define c_szPropRowBegin    TEXT("<TR><TD class=PropLabel>")
#define c_szPropRowMid      TEXT("</TD><TD class=PropValue>")
#define c_szPropRowEnd      TEXT("</TD></TR>")

// CAppData::_BuildPropertiesHTML
//      Build up a string of HTML that represents the list of useful 
//      properties for this app.  We skip any blank fields.  

LPTSTR CAppData::_BuildPropertiesHTML(void)
{
    LPTSTR pszRet = NULL;
    DWORD dwSort = _GetSortIndex();

    // The HTML consists of a table, each row is a property label and value.
    // Since the 'indexlabel' and 'indexvalue' properties vary depending
    // on the sort criteria, we exclude them to avoid duplication.  For 
    // example, if the user is sorting by size, we don't show the size
    // in the properties HTML because the size is being shown via the
    // 'indexvalue' property.

    // Let's first make a pass of the eligible properties to see which
    // ones to include.

    #define PM_SIZE         0x0001
    #define PM_TIMESUSED    0x0002
    #define PM_LASTUSED     0x0004
    #define PM_INSTALLEDON  0x0008

    TCHAR szSize[64];
    TCHAR szFreq[64];
    TCHAR szLastUsed[64];
    DWORD dwPropMask = 0;       // Can be combination of PM_*
    
    dwPropMask |= (S_OK == _GetDiskSize(szSize, SIZECHARS(szSize))) ? PM_SIZE : 0;
    dwPropMask |= (S_OK == GetFrequencyOfUse(szFreq, SIZECHARS(szFreq))) ? PM_TIMESUSED : 0;
    dwPropMask |= (S_OK == _GetLastUsed(szLastUsed, SIZECHARS(szLastUsed))) ? PM_LASTUSED : 0;

    // The sorting criteria should be removed
    if (SORT_NAME == dwSort || SORT_SIZE == dwSort)
        dwPropMask &= ~PM_SIZE;
    else if (SORT_TIMESUSED == dwSort)
        dwPropMask &= ~PM_TIMESUSED;
    else if (SORT_LASTUSED == dwSort)
        dwPropMask &= ~PM_LASTUSED;

    // Are there any properties to show at all?
    if (dwPropMask)
    {
        // Yes; build the html for the table.
        ShStr shstr;
        TCHAR szLabel[64];

        shstr = TEXT("<TABLE id=idTblExtendedProps class=Focus>");

        struct {
            DWORD dwMask;
            UINT  ids;
            LPTSTR psz;
        } s_rgProp[] = {
            { PM_SIZE, IDS_LABEL_SIZE, szSize },
            { PM_TIMESUSED, IDS_LABEL_TIMESUSED, szFreq },
            { PM_LASTUSED, IDS_LABEL_LASTUSED, szLastUsed },
        };

        int i;

        for (i = 0; i < ARRAYSIZE(s_rgProp); i++)
        {
            if (dwPropMask & s_rgProp[i].dwMask)
            {
                LoadString(g_hinst, s_rgProp[i].ids, szLabel, SIZECHARS(szLabel));
                
                shstr.Append(c_szPropRowBegin);
                shstr.Append(szLabel);
                shstr.Append(c_szPropRowMid);

                // Size and frequency-of-use get anchor elements around them...
                if (s_rgProp[i].dwMask & PM_TIMESUSED)
                    shstr.Append(TEXT("<SPAN id=idAFrequency class='FakeAnchor' tabIndex=0 onKeyDown='_OnKeyDownFakeAnchor()' onClick='_OpenDefinition();'>&nbsp;<U>"));
                else if (s_rgProp[i].dwMask & PM_SIZE)
                    shstr.Append(TEXT("<SPAN id=idASize class='FakeAnchor' tabIndex=0 onKeyDown='_OnKeyDownFakeAnchor()' onClick='_OpenDefinition();'>&nbsp;<U>"));
                
                shstr.Append(s_rgProp[i].psz);

                if (s_rgProp[i].dwMask & PM_TIMESUSED)
                    shstr.Append(TEXT("</U></SPAN>"));
                else if (s_rgProp[i].dwMask & PM_SIZE)
                    shstr.Append(TEXT("</U></SPAN>"));
                    
                shstr.Append(c_szPropRowEnd);
            }
        }

        shstr.Append(TEXT("</TABLE>"));

        pszRet = shstr.CloneStr();  // clone for the caller
    }

    return pszRet;
}

#define c_szIconHTMLFormat TEXT("sysimage://%s/small")

/*-------------------------------------------------------------------------
Purpose: Return the HTML that points to the icon of this application 
         use instshld.ico as a default
*/
void CAppData::_GetIconHTML(LPTSTR pszIconHTML, UINT cch)
{
    if (cch > (MAX_PATH + ARRAYSIZE(c_szIconHTMLFormat)))
    {
        TCHAR szImage[MAX_PATH+10];

        if (_ai.pszImage && _ai.pszImage[0])
        {
            StrCpyN(szImage, _ai.pszImage, ARRAYSIZE(szImage));
            //PathParseIconLocation(szImage);
        }
        else
        {
            BOOL bUseDefault = FALSE;
            EnterCriticalSection(&_cs);        
            if (_aiSlow.pszImage && _aiSlow.pszImage[0])
                lstrcpy(szImage, _aiSlow.pszImage);
            else
                bUseDefault = TRUE;
            LeaveCriticalSection(&_cs);

            if (bUseDefault)
                goto DEFAULT;
        }

        wnsprintf(pszIconHTML, cch, c_szIconHTMLFormat, szImage);
    }
    else
DEFAULT:
        // In the default case, return instshld.ico
        lstrcpy(pszIconHTML, TEXT("res://appwiz.cpl/instshld.ico"));
}

/*-------------------------------------------------------------------------
Purpose: Sets the variant with the correct data given the field
*/
HRESULT CAppData::_VariantFromData(const APPFIELDS * pfield, LPVOID pvData, VARIANT * pvar)
{
    HRESULT hres = S_OK;

    VariantInit(pvar);
    
    if (pvData)
    {
        if (VT_BSTR == pfield->vt)
        {
            LPWSTR psz = (LPWSTR)*(LPDWORD)pvData;
            if (NULL == psz)
                psz = L"";

            ASSERT(IS_VALID_STRING_PTR(psz, -1));

            pvar->bstrVal = SysAllocString(psz);
            if (pvar->bstrVal)
                pvar->vt = VT_BSTR;
            else
                hres = E_OUTOFMEMORY;
        }
        else if (VT_UI4 == pfield->vt)
        {
            pvar->lVal = (LONG)*(LPDWORD)pvData;
            pvar->vt = pfield->vt;
        }
    }
    
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: Get the value of the installed-app field.
*/
HRESULT CAppData::_GetInstField(DB_LORDINAL iField, VARIANT * pvar)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_WRITE_PTR(pvar, VARIANT));
    ASSERT(ENUM_INSTALLED == _dwEnum);
    
    // Columns map to different fields.

    if (IsInRange(iField, 0, ARRAYSIZE(g_rginstfields)))
    {
        const APPFIELDS * pfield = &g_rginstfields[iField];
        LPVOID pvData = NULL;       // assume no value
        TCHAR szTemp[MAX_PATH*2];
        LPCTSTR pszTmp;
        DWORD dwTemp;

        // Map the field ordinal to the actual value in the appropriate structure
        switch (pfield->dwStruct)
        {
        case IFS_APPINFODATA:
            pvData = (LPVOID)((LPBYTE)&_ai + pfield->ibOffset);
            break;

        case IFS_CAPABILITY:
            dwTemp = _GetCapability();
            pvData = (LPVOID) &dwTemp;
            break;

        case IFS_SUPPORTINFO:
            pszTmp = CAppData::_BuildSupportInfo();
            if (pszTmp)
            {
                TraceMsg(TF_VERBOSEDSO, "HTML (supportinfo): \"%s\"", pszTmp);
                pvData = (LPVOID) &pszTmp;
            }
            break;

        case IFS_SIZE:
            _GetDiskSize(szTemp, SIZECHARS(szTemp));
            pszTmp = szTemp;
            pvData = (LPVOID) &pszTmp;
            break;

        case IFS_TIMESUSED:
            GetFrequencyOfUse(szTemp, SIZECHARS(szTemp));
            pszTmp = szTemp;
            pvData = (LPVOID) &pszTmp;
            break;

        case IFS_LASTUSED:
            _GetLastUsed(szTemp, SIZECHARS(szTemp));
            pszTmp = szTemp;
            pvData = (LPVOID) &pszTmp;
            break;
            
        case IFS_INDEXLABEL:
            // Unknown values shouldn't show the labels, so assume the
            // value will be unknown
            dwTemp = 0;
            
            // The index label is according to the current sort criteria
            switch (_GetSortIndex())
            {
            case SORT_NAME:     // when sorting by name, we show the size
            case SORT_SIZE:
                if (S_OK == _GetDiskSize(NULL, 0))
                    dwTemp = IDS_LABEL_SIZE;
                break;

            case SORT_TIMESUSED:
                if (S_OK == GetFrequencyOfUse(NULL, 0))
                    dwTemp = IDS_LABEL_TIMESUSED;
                break;

            case SORT_LASTUSED:
                if (S_OK == _GetLastUsed(NULL, 0))
                    dwTemp = IDS_LABEL_LASTUSED;
                break;
            }

            if (0 != dwTemp)
            {
                LoadString(g_hinst, dwTemp, szTemp, SIZECHARS(szTemp));
                pszTmp = szTemp;
                pvData = (LPVOID) &pszTmp;

                TraceMsg(TF_VERBOSEDSO, "HTML (indexlabel): \"%s\"", pszTmp);
            }
            break;

        case IFS_INDEXVALUE:
            // The index value is according to the current sort criteria
            switch (_GetSortIndex())
            {
            case SORT_NAME:     // when sorting by name, we show the size
            case SORT_SIZE:
                _GetDiskSize(szTemp, SIZECHARS(szTemp));
                break;

            case SORT_TIMESUSED:
                GetFrequencyOfUse(szTemp, SIZECHARS(szTemp));
                break;

            case SORT_LASTUSED:
                _GetLastUsed(szTemp, SIZECHARS(szTemp));
                break;
            }
            
            pszTmp = szTemp;
            pvData = (LPVOID) &pszTmp;

            TraceMsg(TF_VERBOSEDSO, "HTML (indexvalue) for %s: \"%s\"", GetDataPtr()->pszDisplayName, pszTmp);
            break;

        case IFS_PROPERTIES:
            pszTmp = _BuildPropertiesHTML();
            if (pszTmp)
            {
                pvData = (LPVOID) &pszTmp;
                TraceMsg(TF_VERBOSEDSO, "HTML (properties) for %s: \"%s\"", GetDataPtr()->pszDisplayName, pszTmp);
            }
            break;

        case IFS_ICON:
            _GetIconHTML(szTemp, ARRAYSIZE(szTemp));
            pszTmp = szTemp;
            pvData = (LPVOID) &pszTmp;
            TraceMsg(TF_VERBOSEDSO, "Icon HTML for %s: \"%s\"", GetDataPtr()->pszDisplayName, pszTmp);
            break;
            
        default:
            ASSERTMSG(0, "invalid field type");
            break;
        }

        hres = _VariantFromData(pfield, pvData, pvar);

        // Clean up
        switch (pfield->dwStruct)
        {
        case IFS_SUPPORTINFO:
        case IFS_PROPERTIES:
            if (pvData)
            {
                pszTmp = (LPWSTR)*(LPDWORD)pvData;
                LocalFree((HLOCAL)pszTmp);
            }
            break;
        }
    }
    return hres;
}


#define c_szAddLaterHTML TEXT("<TABLE id=idTblExtendedProps class=Focus><TR><TD class=AddPropLabel>%s</TD><TD class=AddPropValue><A id=idASchedule class=Focus href=''>%s</A></TD></TR></TABLE>")


/*-------------------------------------------------------------------------
Purpose: Get the value of the published app field.
*/
HRESULT CAppData::_GetPubField(DB_LORDINAL iField, VARIANT * pvar)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_WRITE_PTR(pvar, VARIANT));
    ASSERT(ENUM_PUBLISHED == _dwEnum);
    
    // Columns map to different fields.

    if (IsInRange(iField, 0, ARRAYSIZE(g_rgpubfields)))
    {
        DWORD dwTemp = 0;
        const APPFIELDS * pfield = &g_rgpubfields[iField];
        LPVOID pvData = NULL;
        WCHAR szTemp[MAX_PATH*2];
        LPWSTR pszTmp = NULL;        
        // Map the field ordinal to the actual value in the appropriate structure
        switch (pfield->dwStruct)
        {
            case IFS_APPINFODATA:
                // For the display name, we want to display it in the following format if there are 
                // duplicate entries: Ex "Word : Policy1" and "Word : Policy2"
                if ((iField == 0) && _bNameDupe && (_pai.dwMask & PAI_SOURCE) && _pai.pszSource && _pai.pszSource[0])
                {
                    wnsprintf(szTemp, ARRAYSIZE(szTemp), L"%ls: %ls", _ai.pszDisplayName, _pai.pszSource);
                    pszTmp = szTemp;
                    pvData = (LPVOID) &pszTmp;
                }
                else
                    pvData = (LPVOID)((LPBYTE)&_ai + pfield->ibOffset);
                break;

        case IFS_CAPABILITY:
            dwTemp = _GetCapability();
            pvData = (LPVOID) &dwTemp;
            break;

        case IFS_SCHEDULE:
            if ((_GetCapability() & APPACTION_ADDLATER) && (_pai.dwMask & PAI_SCHEDULEDTIME))
            {
                TCHAR szTime[MAX_PATH];
                if (FormatSystemTimeString(&_pai.stScheduled, szTime, ARRAYSIZE(szTime)))
                {
                    TCHAR szAddLater[100];
                    LoadString(g_hinst, IDS_ADDLATER, szAddLater, ARRAYSIZE(szAddLater));           

                    wsprintf(szTemp, c_szAddLaterHTML, szAddLater, szTime);
                    pszTmp = szTemp;
                    pvData = (LPVOID) &pszTmp;
                }
            }
            break;

        case IFS_ISINSTALLED:
            if (S_OK == _ppa->IsInstalled())
            {
                LoadString(g_hinst, IDS_INSTALLED, szTemp, SIZECHARS(szTemp));
                pszTmp = szTemp;
                pvData = (LPVOID) &pszTmp;
            }
            break;
                
        default:
            ASSERTMSG(0, "invalid field type");
            hres = E_FAIL;
            break;
        }

        hres = _VariantFromData(pfield, pvData, pvar);
    }
    return hres;
}

/*-------------------------------------------------------------------------
Purpose: Get the value of the ocsetup app field.
*/
HRESULT CAppData::_GetSetupField(DB_LORDINAL iField, VARIANT * pvar)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_WRITE_PTR(pvar, VARIANT));
    ASSERT(ENUM_OCSETUP == _dwEnum);
    
    // Columns map to different fields.

    if (IsInRange(iField, 0, ARRAYSIZE(g_rgsetupfields)))
    {
        const APPFIELDS * pfield = &g_rgsetupfields[iField];
        LPVOID pvData = NULL;

        ASSERT(IFS_OCSETUP == pfield->dwStruct);

        pvData = (LPVOID)((LPBYTE)&_pocsa + pfield->ibOffset);

        hres = _VariantFromData(pfield, pvData, pvar);
    }
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: Get the value of the category field.
*/
HRESULT CAppData::_GetCatField(DB_LORDINAL iField, VARIANT * pvar)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_WRITE_PTR(pvar, VARIANT));
    ASSERT(ENUM_CATEGORIES == _dwEnum);
    
    // Columns map to different fields.

    if (IsInRange(iField, 0, ARRAYSIZE(g_rgcatfields)))
    {
        const APPFIELDS * pfield = &g_rgcatfields[iField];
        LPVOID pvData = NULL;

        ASSERT(IFS_SHELLCAT == pfield->dwStruct);

        pvData = (LPVOID)((LPBYTE)_psac + pfield->ibOffset);

        hres = _VariantFromData(pfield, pvData, pvar);
    }
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IAppData::GetVariant

         Return the particular field as a variant.  The caller is responsible
         for clearing/freeing the variant.  The iField is assumed to be
         0-based.  
*/
STDMETHODIMP CAppData::GetVariant(DB_LORDINAL iField, VARIANT * pvar)
{
    HRESULT hres;

    switch (_dwEnum)
    {
    case ENUM_INSTALLED:
        hres = _GetInstField(iField, pvar);
        break;

    case ENUM_PUBLISHED:
        hres = _GetPubField(iField, pvar);
        break;

    case ENUM_OCSETUP:
        hres = _GetSetupField(iField, pvar);
        break;

    case ENUM_CATEGORIES:
        hres = _GetCatField(iField, pvar);
        break;
    }
    
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IAppData::SetMtxParent
*/
STDMETHODIMP CAppData::SetMtxParent(IMtxArray * pmtxParent)
{
    // We don't AddRef because this appdata object lives within the
    // lifetime of the matrix object.
    
    _pmtxParent = pmtxParent;
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IAppData::DoCommand

         Executes the given command depending on the capabilities
         of the item.
*/
STDMETHODIMP CAppData::DoCommand(HWND hwndParent, APPCMD appcmd)
{
    HRESULT hres = S_FALSE;
    DWORD dwActions = 0;

    switch (_dwEnum)
    {
    case ENUM_INSTALLED:
       
        // Find out of the requested command is allowed
        // BUGBUG (scotth): need to add policies check here
        _pia->GetPossibleActions(&dwActions);

        switch (appcmd)
        {
        case APPCMD_UNINSTALL:
            if (g_dwPrototype & PF_FAKEUNINSTALL)
            {
                // Say the app was deleted (this is optimized out in retail)
                hres = S_OK;
            }
            else
            {
                if (dwActions & APPACTION_UNINSTALL)
                {
                    hres = _pia->Uninstall(hwndParent);
                    if (SUCCEEDED(hres))
                    {
                        // Return S_FALSE if the app is still installed (the user
                        // might have cancelled the uninstall)
                        hres = (S_OK == _pia->IsInstalled()) ? S_FALSE : S_OK;
                    }
                }
            }
            break;

        case APPCMD_MODIFY:
            if (dwActions & APPACTION_MODIFY)
                hres = _pia->Modify(hwndParent);
            break;

        case APPCMD_REPAIR:
            if (dwActions & APPACTION_REPAIR)
                hres = _pia->Repair(TRUE);       // BUGBUG (scotth): this is hardcoded
            break;

        case APPCMD_UPGRADE:
            if (dwActions & APPACTION_UPGRADE)
                hres = _pia->Upgrade();
            break;

        default:
            TraceMsg(TF_ERROR, "(Ctl) invalid appcmd %s", Dbg_GetAppCmd(appcmd)); 
            break;
        }
        break;

    case ENUM_PUBLISHED:
        // Find out of the requested command is allowed
        // BUGBUG (scotth): need to add policies check here
        _ppa->GetPossibleActions(&dwActions);

        switch (appcmd)
        {
        case APPCMD_INSTALL:
            if (dwActions & APPACTION_INSTALL)
            {
                hres = S_OK;
                // Does the app have an expired publishing time?
                if (_pai.dwMask & PAI_EXPIRETIME)
                {
                    // Yes, it does. Let's compare the expired time with our current time
                    SYSTEMTIME stCur = {0};
                    GetLocalTime(&stCur);

                    // Is "now" later than the expired time?
                    if (CompareSystemTime(&stCur, &_pai.stExpire) > 0)
                    {
                        // Yes, warn the user and return failure
                        ShellMessageBox(g_hinst, hwndParent, MAKEINTRESOURCE(IDS_EXPIRED),
                                        MAKEINTRESOURCE(IDS_NAME), MB_OK | MB_ICONEXCLAMATION);
                        hres = E_FAIL;
                    }
                }

                // if hres is not set by the above code, preceed with installation
                if (hres == S_OK)
                {
                    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
#ifdef WINNT
                    // On NT,  let Terminal Services know that we are about to install an application.
                    // NOTE: This function should be called no matter the Terminal Services
                    // is running or not.
                    BOOL bPrevMode = TermsrvAppInstallMode();
                    SetTermsrvAppInstallMode(TRUE);
#endif
                    hres = _ppa->Install(NULL);
#ifdef WINNT
                    SetTermsrvAppInstallMode(bPrevMode);
#endif
                    SetCursor(hcur);
                }
            }
            break;

        case APPCMD_ADDLATER:
            if (dwActions & APPACTION_ADDLATER)
            {
                ADDLATERDATA ald = {0};
                        
                if (_pai.dwMask & PAI_SCHEDULEDTIME)
                {
                    ald.stSchedule = _pai.stScheduled;
                    ald.dwMasks |= ALD_SCHEDULE;
                }

                if (_pai.dwMask & PAI_ASSIGNEDTIME)
                {
                    ald.stAssigned = _pai.stAssigned;
                    ald.dwMasks |= ALD_ASSIGNED;
                }
                
                if (_pai.dwMask & PAI_EXPIRETIME)
                {
                    ald.stExpire = _pai.stExpire;
                    ald.dwMasks |= ALD_EXPIRE;
                }
                
                if (GetNewInstallTime(hwndParent, &ald))
                {
                    if (ald.dwMasks & ALD_SCHEDULE)
                        hres = _ppa->Install(&ald.stSchedule);
                    else
                        hres = _ppa->Unschedule();
                }
            }
        default:
            TraceMsg(TF_ERROR, "(Ctl) invalid appcmd %s", Dbg_GetAppCmd(appcmd)); 
            break;
        }
        break;

    case ENUM_OCSETUP:
        switch (appcmd)
        {
        case APPCMD_INSTALL:
            // call some command that "installs" the selected item.
            _pocsa->Run();
            hres = S_OK;
            break;

        default:
            TraceMsg(TF_ERROR, "(Ctl) invalid appcmd %s", Dbg_GetAppCmd(appcmd)); 
            break;
        }
        break;

    case ENUM_CATEGORIES:
        TraceMsg(TF_WARNING, "(Ctl) tried to exec appcmd %s on a category.  Not supported.", Dbg_GetAppCmd(appcmd));
        break;
    }

    return hres;
}



//---------------------------------------------------------------------------
//   Matrix Array class
//---------------------------------------------------------------------------


// constructor
CMtxArray2::CMtxArray2()
{
    ASSERT(NULL == _hdpa);
    ASSERT(0 == _cRefLock);

    TraceMsg(TF_OBJLIFE, "(MtxArray) creating");
    
    InitializeCriticalSection(&_cs);
}


// destructor
CMtxArray2::~CMtxArray2()
{
    DBROWCOUNT cItems;
    
    ASSERT(0 == _cRefLock);

    TraceMsg(TF_OBJLIFE, "(MtxArray) destroying");

    GetItemCount(&cItems);
    DeleteItems(0, cItems);
    if (_hdpa)
        DPA_Destroy(_hdpa);
    
    DeleteCriticalSection(&_cs);
}


/*--------------------------------------------------------------------
Purpose: IUnknown::QueryInterface
*/
STDMETHODIMP CMtxArray2::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CMtxArray2, IMtxArray),
        { 0 },
    };

    HRESULT hres = QISearch(this, (LPCQITAB)qit, riid, ppvObj);
    if (FAILED(hres))
        hres = CWorkerThread::QueryInterface(riid, ppvObj);

    return hres;
}


void CMtxArray2::_Lock(void)
{
    EnterCriticalSection(&_cs);
    DEBUG_CODE( _cRefLock++; )
}

void CMtxArray2::_Unlock(void)
{
    DEBUG_CODE( _cRefLock--; )
    LeaveCriticalSection(&_cs);
}


/*-------------------------------------------------------------------------
Purpose: Create array 
*/
HRESULT CMtxArray2::_CreateArray(void)
{
    _Lock();
    if (NULL == _hdpa)
        _hdpa = DPA_Create(8);
    _Unlock();
    
    return _hdpa ? S_OK : E_OUTOFMEMORY;
}


HRESULT CMtxArray2::_DeleteItem(DBROWCOUNT idpa)
{
    // The caller must enter the lock first
    ASSERT(0 < _cRefLock);
    ASSERT(_hdpa);

    IAppData * pappdata = (IAppData *)DPA_GetPtr(_hdpa, (LONG)idpa);
    if (pappdata)
        pappdata->Release();
        
    DPA_DeletePtr(_hdpa, (LONG)idpa);
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IMtxArray::Initialize
*/
STDMETHODIMP CMtxArray2::Initialize(DWORD dwEnum)
{
    _dwEnum = dwEnum;
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IMtxArray::AddItem

         Add an item to the array

         Returns the row in which the record was added (0-based).
*/
STDMETHODIMP CMtxArray2::AddItem(IAppData * pappdata, DBROWCOUNT * piRow)
{
    HRESULT hres = E_FAIL;

    ASSERT(pappdata);
    ASSERT(NULL == piRow || IS_VALID_WRITE_PTR(piRow, DBROWCOUNT));
    
    hres = _CreateArray();
    if (SUCCEEDED(hres))
    {
        _Lock();
        {
            pappdata->AddRef();     // AddRef since we're storing it away
            
            LONG iRow = DPA_AppendPtr(_hdpa, pappdata);
            if (DPA_ERR != iRow)
            {
                DEBUG_CODE( TraceMsg(TF_DSO, "(CMtxArray) added record %d", iRow); )
                pappdata->SetMtxParent(SAFECAST(this, IMtxArray *));
                hres = S_OK;
            }
            else
            {
                pappdata->Release();
                hres = E_OUTOFMEMORY;
                iRow = -1;
            }

            if (piRow)
                *piRow = iRow;
        }
        _Unlock();
    }    
    return hres;
}

/*-------------------------------------------------------------------------
Purpose: IMtxArray::DeleteItems

         Delete a group of records (count of cRows) starting at iRow.

         Assumes iRow is 0-based.
*/
STDMETHODIMP CMtxArray2::DeleteItems(DBROWCOUNT idpa, DBROWCOUNT cdpa)
{
    DBROWCOUNT i;

    if (_hdpa)
    {
        _Lock();
        {
            ASSERT(IS_VALID_HANDLE(_hdpa, DPA));
            ASSERT(IsInRange(idpa, 0, DPA_GetPtrCount(_hdpa)));

            for (i = idpa + cdpa - 1; i >= idpa; i--)
            {
                _DeleteItem(i);
            }
        }
        _Unlock();
    }

    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IMtxArray::GetAppData

         Return a pointer to the appdata element of the given row.  The
         caller must Release the returned appdata.
*/
STDMETHODIMP CMtxArray2::GetAppData(DBROWCOUNT iRow, IAppData ** ppappdata)
{
    HRESULT hres = E_FAIL;
    IAppData * pappdata = NULL;
    
    if (_hdpa)
    {
        _Lock();
        pappdata = (IAppData *)DPA_GetPtr(_hdpa, iRow);
        if (pappdata)
        {
            pappdata->AddRef(); 
            hres = S_OK;
        }
        _Unlock();
    }

    *ppappdata = pappdata;

    return hres;
}

/*-------------------------------------------------------------------------
Purpose: IMtxArray::GetItemCount

         Return the count of items in the array.
*/
STDMETHODIMP CMtxArray2::GetItemCount(DBROWCOUNT * pcItems)
{
    if (_hdpa)
        *pcItems = DPA_GetPtrCount(_hdpa);
    else
        *pcItems = 0;

    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IMtxArray::GetFieldCount

         Return the count of fields based upon what we are enumerating.
*/
STDMETHODIMP CMtxArray2::GetFieldCount(DB_LORDINAL * pcFields)
{
    DB_LORDINAL lRet = 0;

    switch (_dwEnum)
    {
    case ENUM_INSTALLED:
        lRet = ARRAYSIZE(g_rginstfields);
        break;

    case ENUM_PUBLISHED:
        lRet = ARRAYSIZE(g_rgpubfields);
        break;

    case ENUM_OCSETUP:
        lRet = ARRAYSIZE(g_rgsetupfields);
        break;

    case ENUM_CATEGORIES:
        lRet = ARRAYSIZE(g_rgcatfields);
        break;
    }

    *pcFields = lRet;
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IMtxArray::GetFieldName

         Return the field name.
*/
STDMETHODIMP CMtxArray2::GetFieldName(DB_LORDINAL iField, VARIANT * pvar)
{
    HRESULT hres = E_INVALIDARG;
    DB_LORDINAL cfields = 0;
    const APPFIELDS *rgfield;
    
    VariantInit(pvar);

    switch (_dwEnum)
    {
    case ENUM_INSTALLED:
        cfields = ARRAYSIZE(g_rginstfields);
        rgfield = g_rginstfields;
        break;

    case ENUM_PUBLISHED:
        cfields = ARRAYSIZE(g_rgpubfields);
        rgfield = g_rgpubfields;
        break;

    case ENUM_OCSETUP:
        cfields = ARRAYSIZE(g_rgsetupfields);
        rgfield = g_rgsetupfields;
        break;

    case ENUM_CATEGORIES:
        cfields = ARRAYSIZE(g_rgcatfields);
        rgfield = g_rgcatfields;
        break;
    }

    if (IsInRange(iField, 0, cfields))
    {
        pvar->bstrVal = SysAllocString(rgfield[iField].pszLabel);
        if (pvar->bstrVal)
        {
            pvar->vt = VT_BSTR;
            hres = S_OK;
        }
        else
            hres = E_OUTOFMEMORY;
    }

    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IMtxArray:GetSortIndex
*/
STDMETHODIMP CMtxArray2::GetSortIndex(DWORD * pdwSort)
{
    ASSERT(pdwSort);

    *pdwSort = _dwSort;
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IMtxArray::SetSortCriteria

         Determines which field to sort by depending on the given criteria.
*/
STDMETHODIMP CMtxArray2::SetSortCriteria(LPCWSTR pszSortField)
{
    int i;
    int cfields;
    const APPFIELDS * pfield;
    
    ASSERT(IS_VALID_STRING_PTRW(pszSortField, -1));

    switch (_dwEnum)
    {
    case ENUM_INSTALLED:
        cfields = ARRAYSIZE(g_rginstfields);
        pfield = g_rginstfields;
        break;

    case ENUM_PUBLISHED:
        cfields = ARRAYSIZE(g_rgpubfields);
        pfield = g_rgpubfields;
        break;

    case ENUM_OCSETUP:
        cfields = ARRAYSIZE(g_rgsetupfields);
        pfield = g_rgsetupfields;
        break;

    case ENUM_CATEGORIES:
        cfields = ARRAYSIZE(g_rgcatfields);
        pfield = g_rgcatfields;
        break;
    }

    // Find the given field's sort index
    for (i = 0; i < cfields; i++, pfield++)
    {
        if (0 == StrCmpW(pszSortField, pfield->pszLabel))
        {
            // Can this field be sorted?
            if (SORT_NA != pfield->dwSort)
            {
                // Yes
                _dwSort = pfield->dwSort;
            }
            break;
        }
    }

    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: Static callback function to handle sorting.  lParam is the
         pointer to the IMtxArray.
*/
int CMtxArray2::s_SortItemsCallbackWrapper(LPVOID pv1, LPVOID pv2, LPARAM lParam)
{
    IMtxArray * pmtxarray = (IMtxArray *)lParam;
    ASSERT(pmtxarray);
    
    return pmtxarray->CompareItems((IAppData *)pv1, (IAppData *)pv2);
}


/*-------------------------------------------------------------------------
Purpose: IMtxArray::CompareItems

         Compares two appdata objects according to the current sort index.
*/
STDMETHODIMP_(int) CMtxArray2::CompareItems(IAppData * pappdata1, IAppData * pappdata2)
{
    
    switch (_dwSort)
    {
    case SORT_NAME:
        {
            LPWSTR pszName1 = pappdata1->GetDataPtr()->pszDisplayName;
            LPWSTR pszName2 = pappdata2->GetDataPtr()->pszDisplayName;

            if (pszName1 && pszName2)
                return StrCmpW(pszName1, pszName2);
            else if (pszName1)
                return 1;
            else if (pszName2)
                return -1;
        }
        break;

    case SORT_SIZE:
        {
            ULONGLONG ull1 = pappdata1->GetSlowDataPtr()->ullSize;
            ULONGLONG ull2 = pappdata2->GetSlowDataPtr()->ullSize;

            // Big apps come before smaller apps
            if (ull1 > ull2)
                return -1;
            else if (ull1 < ull2)
                return 1;
        }
        break;

    case SORT_TIMESUSED:
        {
            // Rarely used apps come before frequently used apps.  Blank
            // (unknown) apps go last.  Unknown apps are -1, so those sort
            // to the bottom if we simply compare unsigned values.
            UINT u1 = (UINT)pappdata1->GetSlowDataPtr()->iTimesUsed;
            UINT u2 = (UINT)pappdata2->GetSlowDataPtr()->iTimesUsed;

            if (u1 < u2)
                return -1;
            else if (u1 > u2)
                return 1;
        }
        break;

    case SORT_LASTUSED:
        {
            FILETIME ft1 = pappdata1->GetSlowDataPtr()->ftLastUsed;
            FILETIME ft2 = pappdata2->GetSlowDataPtr()->ftLastUsed;

            return CompareFileTime(&ft1, &ft2);
        }
        break;
    }
    
    return 0;
}


/*-------------------------------------------------------------------------
Purpose: IMtxArray::SortItems

         Sorts the array according to the current sort index.
*/
STDMETHODIMP CMtxArray2::SortItems(void)
{
    if (_hdpa)
    {
        _Lock();
        {
            ASSERT(IS_VALID_HANDLE(_hdpa, DPA));

            DPA_Sort(_hdpa, s_SortItemsCallbackWrapper, (LPARAM)this);
        }
        _Unlock();
    }
    return S_OK;
}

/*-------------------------------------------------------------------------
Purpose: IMtxArray::MarkDupEntries

         Mark the (name) duplicate entries in the DPA array of app data
*/
STDMETHODIMP CMtxArray2::MarkDupEntries(void)
{
    if (_hdpa)
    {
        _Lock();
        {
            int idpa;
            for (idpa = 0; idpa < DPA_GetPtrCount(_hdpa) - 1; idpa++) 
            {
                IAppData * pad = (IAppData *)DPA_GetPtr(_hdpa, idpa);
                IAppData * padNext = (IAppData *)DPA_GetPtr(_hdpa, idpa+1);
                LPWSTR pszName = pad->GetDataPtr()->pszDisplayName;
                LPWSTR pszNameNext = padNext->GetDataPtr()->pszDisplayName;
                if (pszName && pszNameNext && !StrCmpW(pszName, pszNameNext))
                {
                    pad->SetNameDupe(TRUE);
                    padNext->SetNameDupe(TRUE);
                }
            }
        }
        _Unlock();
    }
    return S_OK;
}

/*-------------------------------------------------------------------------
Purpose: IMtxArray::KillWT

         Kills the worker thread. 
*/

STDMETHODIMP CMtxArray2::KillWT()
{
    return CWorkerThread::KillWT();
}

/*----------------------------------------------------------
Purpose: Create-instance function for CMtxArray

*/
HRESULT CMtxArray_CreateInstance(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres = E_OUTOFMEMORY;

    *ppvObj = NULL;
    
    CMtxArray2 * pObj = new CMtxArray2();
    if (pObj)
    {
        hres = pObj->QueryInterface(riid, ppvObj);
        pObj->Release();
    }

    return hres;
}

/*-------------------------------------------------------------------------
Purpose: IMtxArray::_ThreadStartProc()

*/
DWORD CMtxArray2::_ThreadStartProc()
{
    DBROWCOUNT i;

    TraceMsg(TF_TASKS, "[%x] Starting slow app info thread", _dwThreadId);

    DBROWCOUNT cItems = 0;
    GetItemCount(&cItems);
    
    // Loop through all enumerated items, getting the 'slow' information
    for (i = 0 ; i < cItems ; i++)
    {
        IAppData * pappdata;
        
        // If we've been asked to bail, do so
        if (IsKilled())
            break;

        GetAppData(i, &pappdata);
        if (pappdata)
        {
            HRESULT hres = pappdata->ReadSlowData(); 
            // Call to get the slow info for the item. Fire an event on success
            if (hres == S_OK)
            {
                PostWorkerMessage(WORKERWIN_FIRE_ROW_READY, i, 0);

                TraceMsg(TF_TASKS, "[%x] Slow info for %d (%ls): hit", _dwThreadId, i, 
                        pappdata->GetDataPtr()->pszDisplayName);
            }
            else
            {
                TraceMsg(TF_TASKS, "[%x] Slow info for %d (%ls): miss", _dwThreadId, i, 
                         pappdata->GetDataPtr()->pszDisplayName);
            }
            pappdata->Release();
        }
    }

    PostWorkerMessage(WORKERWIN_FIRE_FINISHED, 0, 0);
    return  CWorkerThread::_ThreadStartProc();
}

#endif //DOWNLEVEL_PLATFORM
