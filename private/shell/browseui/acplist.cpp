/**************************************************************\
    FILE: acplist.cpp

    DESCRIPTION:
        CACPList implements the AutoComplete Possibilities List.
    This will find the possible Autcompletions.
\**************************************************************/

#include "priv.h"
#include "dbgmem.h"
#include "addrlist.h"
#include "util.h"
#include "autocomp.h"


/**************************************************************\
    CLASS: CACPList

    DESCRIPTION:
        CACPList implements the AutoComplete Possibilities List.
    This will find the possible Autcompletions.
\**************************************************************/
class CACPList  : public CAddressList
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IAddressList methods ***
    virtual STDMETHODIMP Connect(BOOL fConnect, HWND hwnd, IBrowserService * pbs, IBandProxy * pbp, IAutoComplete * pac);
    virtual STDMETHODIMP Refresh(DWORD dwType);

protected:
    //////////////////////////////////////////////////////
    // Private Member Functions
    //////////////////////////////////////////////////////

    // Constructor / Destructor
    CACPList();
    ~CACPList(void);        // This is now an OLE Object and cannot be used as a normal Class.

    // Address List Modification Functions
    HRESULT _Populate(void);
    void _InitCombobox(void);

    // Friend Functions
    friend IAddressList * CACPList_Create(void);

    //////////////////////////////////////////////////////
    //  Private Member Variables 
    //////////////////////////////////////////////////////
    IAutoComplete *     _pac;               // AutoComplete to use to fill ComboBox.
    SHSTR               _shstrLastSearch;
};




//================================================================= 
// Implementation of CACPList
//=================================================================


/****************************************************\
    FUNCTION: CACPList_Create
  
    DESCRIPTION:
        This function will create an instance of the
    CACPList COM object.
\****************************************************/
IAddressList * CACPList_Create(void)
{
    CACPList * p = new CACPList();
    return p;
}


/****************************************************\
  
    Address Band Constructor
  
\****************************************************/
CACPList::CACPList()
{
    // This needs to be allocated in Zero Inited Memory.
    // ASSERT that all Member Variables are inited to Zero.
    ASSERT(!_pac);
}


/****************************************************\
  
    Address Band destructor
  
\****************************************************/
CACPList::~CACPList()
{
    if (_pac)
        _pac->Release();
}

//================================
// *** IAddressList Interface ***

/****************************************************\
    FUNCTION: Connect
  
    DESCRIPTION:
        We are either becoming the selected list for
    the AddressBand's combobox, or lossing this status.
    We need to populate or unpopulate the combobox
    as appropriate.
\****************************************************/
HRESULT CACPList::Connect(BOOL fConnect, HWND hwnd, IBrowserService * pbs, IBandProxy * pbp, IAutoComplete * pac)
{
    HRESULT hr = S_OK;

    // Copy the (IAutoComplete *) parameter
    if (_pac)
        _pac->Release();

    CAddressList::Connect(fConnect, hwnd, pbs, pbp, pac);

    _pac = pac;
    if (_pac)
        _pac->AddRef();

    if (!fConnect)
    {
        // Unconnecting from the list will delete the combobox contents,
        // so we need to mark the string as empty to force a repopulation.
        _shstrLastSearch.SetStr(TEXT("\0"));
    }

    return hr;
}


/****************************************************\
    FUNCTION: Refresh
  
    DESCRIPTION:
        TODO.
\****************************************************/
HRESULT CACPList::Refresh(DWORD dwType)
{
    if (OLECMD_REFRESH_ENTIRELIST == dwType)
    {
        // Force a repopulation next time Combobox is dropped down.
        _shstrLastSearch.SetStr(TEXT("\0"));
    }
    return S_OK;
}


/****************************************************\
    FUNCTION: _InitCombobox
    
    DESCRIPTION:
        Prepare the combo box for this list.  This normally
    means that the indenting and icon are either turned
    on or off.
\****************************************************/
void CACPList::_InitCombobox()
{
    HIMAGELIST himlSysSmall;
    Shell_GetImageLists(NULL, &himlSysSmall);

    SendMessage(_hwnd, CBEM_SETIMAGELIST, 0, (LPARAM)himlSysSmall);
    SendMessage(_hwnd, CBEM_SETEXTENDEDSTYLE, 0,0);    
    CAddressList::_InitCombobox();    
}


/****************************************************\
    FUNCTION: _Populate
    
    DESCRIPTION:
        Fills in the combobox with at most
    ACP_LIST_MAX_CONST available AutoCompletions.
\****************************************************/
HRESULT CACPList::_Populate(void)
{
    ASSERT(_hwnd);

    HRESULT hr = S_OK;
    IEnumString * pes;
    TCHAR szCurrSearchString[MAX_URL_STRING];
    HWND hwndEdit = (HWND)SendMessage(_hwnd, CBEM_GETEDITCONTROL, 0, 0L);

    ASSERT(ARRAYSIZE(szCurrSearchString) > GetWindowTextLength(hwndEdit));
    GetWindowText(hwndEdit, szCurrSearchString, SIZECHARS(szCurrSearchString));
    
    if ((_shstrLastSearch.GetSize()) &&
        (0 == StrCmpN(szCurrSearchString, _shstrLastSearch.GetStr(), ARRAYSIZE(szCurrSearchString))))
    {
        // This is the same AutoComplete string as we used to populate
        // last time so we don't need to repopulate the list.
        return S_OK;
    }
    else
    {
        // The string to use to generate the AutoComplete List is
        // new so we need to purge the contents of the current
        // combobox.
        SendMessage(_hwnd, CB_RESETCONTENT, 0, 0L);
        _shstrLastSearch.SetStr(szCurrSearchString);
    }

    hr = _pac->QueryInterface(IID_IEnumString, (void **)&pes);
    if (SUCCEEDED(hr))
    {
        // Get values from autocomplete and put them in combo box
        int nIndex;

        // This may take a while so inform user that we are working on the task.
        HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

        hr = pes->Reset();
        for (nIndex = 0; (S_OK == hr) && (nIndex < ACP_LIST_MAX_CONST); nIndex++)
        {
            LPOLESTR pbstrUrl;
            ULONG celtFetched;
            hr = pes->Next(1, &pbstrUrl, &celtFetched);
            if ((S_OK == hr) && (1 == celtFetched))
            {
                TCHAR szUrl[MAX_URL_STRING];
                SHUnicodeToTChar(pbstrUrl, szUrl, ARRAYSIZE(szUrl));
                _ComboBoxInsertURL(szUrl, ARRAYSIZE(szUrl), ACP_LIST_MAX_CONST);
                TraceMsg(TF_BAND|TF_GENERAL, "CACPList: _Populate(). CoTaskMemFree(%lx);.", pbstrUrl);
                CoTaskMemFree(pbstrUrl);
            }
        }
        SetCursor(hCursorOld);
        pes->Release();
    }

    TraceMsg(TF_BAND|TF_GENERAL, "CACPList: _Populate(). This is a VERY EXPENSIVE operation.");
    return hr;
} 

