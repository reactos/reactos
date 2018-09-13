#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include <prsht.h>
#include <tchar.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellids.h>

#include "winreg.h"
#include "resource.h"
#include "shwizard.h"

#define DXA_GROWTH_CONST    10

#define CUSTOMIZE_ALWAYS    TEXT('A')
#define CUSTOMIZE_NEVER     TEXT('N')
#define CUSTOMIZE_DEFAULT   TEXT('D')
#define CUSTOMIZE_ONCHOICE  TEXT('O')

#include <commctrl.h>   // The DSA functions are implemented here

HDSA g_hdsaTemplateInfo = NULL;
int g_nNextTemplateId = 0;

const TCHAR c_szWebvwTemplatePreview[] = TEXT("WebvwTemplatePreview");
HWND g_hwndLV_Templates = NULL;
HWND g_hwndText_Desc = NULL;
HWND g_hwndBitmap_Preview = NULL;
HWND g_hwndDlg = NULL;
TCHAR g_szTemplateFile[MAX_PATH];
TCHAR g_szPreviewBitmapFile[MAX_PATH];
TCHAR g_szKey[20];

INT_PTR APIENTRY PageA3Proc (HWND, UINT, WPARAM, LPARAM);

BOOL PageA3_OnInitDialog (HWND);
void PageA3_OnSetActive  (HWND);
void PageA3_OnWizardBack (HWND);
void PageA3_OnWizardNext (HWND);
void PageA3CleanUp();

BOOL LoadWebViewTemplatesInfoFromRegistry();
BOOL LoadTemplateInfo(HKEY hkey, LPCTSTR pszKey);
BOOL AddInfoToTemplateList(WebViewTemplateInfo* pwvTemplate);
BOOL InitTemplateListView(HWND hDlg);
void FillTemplateListView(BOOL fEmpty);
BOOL GetTemplateInfoByIndex(int iIndex, WebViewTemplateInfo* pwvTemplate);
int AppendTemplateToListView(WebViewTemplateInfo* pwvTemplate);
void SetDefaultSelection();
void SetSelectionByIndex(int iIndex);
int GetTemplateInfoById(int iId, WebViewTemplateInfo* pwvTemplate);
void SetCheck_Customizable(LPCTSTR lpszCustomizable);
void SetText_CustomizedPreview(LPCTSTR pszCustomizable, LPCTSTR pszPreviewBitmapFile);
void SetTemplateDescription(LPCTSTR lpszDesc);
void ShowPreviewBitmap(LPCTSTR pszPreviewBitmapFile);
void TemplateLV_OnItemChanged();
void PageA3OnDestroy();
void EmptyTemplateList();

// Not used currently
#if 0
void SetSelectionById(int iId);
void EmptyTemplateListview();
#endif


INT_PTR APIENTRY PageA3Proc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = TRUE;
    switch (msg)
    {
    case WM_COMMAND:
        break;
    case WM_INITDIALOG:
        return PageA3_OnInitDialog(hDlg);
    case WM_DESTROY:
    {
        PageA3OnDestroy();
        if (g_pCommonInfo->WasItCustomized() && g_pCommonInfo->WasThisOptionalPathUsed(IDD_PAGEA3))
        {
            UpdateAddWebView(); // Update desktop.ini
        }
        else
        {
            PageA3CleanUp();
        }
        break;
    }
    case WM_NOTIFY:
    {
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_QUERYCANCEL:
            bRet = FALSE;
        case PSN_KILLACTIVE:
        case PSN_RESET:
            Unsubclass(GetDlgItem(hDlg, IDC_BITMAP_PREVIEW));
            g_pCommonInfo->OnCancel(hDlg);
            break;
        case PSN_SETACTIVE:
            Subclass(GetDlgItem(hDlg, IDC_BITMAP_PREVIEW));
            PageA3_OnSetActive(hDlg);
            // Make sure that the global webview is on.
            if (ProdWebViewOn(hDlg))
            {
                break;  // Continue with normal processing
            }
            // Fall through - i.e., if not g_fProceed, go back to the start page.
        case PSN_WIZBACK:
            Unsubclass(GetDlgItem(hDlg, IDC_BITMAP_PREVIEW));
            PageA3_OnWizardBack(hDlg);
            break;
        case PSN_WIZNEXT:
            Unsubclass(GetDlgItem(hDlg, IDC_BITMAP_PREVIEW));
            PageA3_OnWizardNext(hDlg);
            break;
        case LVN_ITEMCHANGED:
        {
            NM_LISTVIEW* pnmlv = (NM_LISTVIEW*)lParam;
            if ((pnmlv->uChanged & LVIF_STATE) &&
                    (pnmlv->uNewState & LVIS_SELECTED))
            {
                TemplateLV_OnItemChanged();
            }
        }
        default:
            bRet = FALSE;
        }
        break;
    }
    default:
        bRet = FALSE;
    }
    return bRet;
}


BOOL PageA3_OnInitDialog (HWND hDlg)
{
    BOOL fRet = FALSE;
    g_hwndDlg = hDlg;
    if (LoadWebViewTemplatesInfoFromRegistry() && InitTemplateListView(hDlg))
    {
        g_hwndText_Desc = GetDlgItem(hDlg, IDC_TEXT_DESC);
        g_hwndBitmap_Preview = GetDlgItem(hDlg, IDC_BITMAP_PREVIEW);
        if (g_hwndText_Desc && g_hwndBitmap_Preview)
        {
            SetDefaultSelection();
            fRet = TRUE;
        }
    }
    return fRet;
}   /*  end PageA3_OnInitDialog() */


BOOL LoadWebViewTemplatesInfoFromRegistry()
{
    BOOL fRet = FALSE;
    HKEY hkey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_WEBVIEW_TEMPLATES, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        WebViewTemplateInfo wvTemplate;
        // If the folder was previously customized, add the Current template to the list.
        int fPrevCustom = HasPersistMoniker(wvTemplate.szTemplateFile, ARRAYSIZE(wvTemplate.szTemplateFile),
                wvTemplate.szPreviewBitmapFile, ARRAYSIZE(wvTemplate.szPreviewBitmapFile));
        if (fPrevCustom != PM_NONE)
        {
            wvTemplate.szKey[0] = TEXT('\0');
            LoadString(g_hAppInst, IDS_CURRENT_TEMPLATE, wvTemplate.szFriendlyName, ARRAYSIZE(wvTemplate.szFriendlyName));
            if (fPrevCustom == PM_LOCAL)
            {
                wvTemplate.chCustomizable[0] = CUSTOMIZE_ALWAYS;
            }
            else
            {
                wvTemplate.chCustomizable[0] = CUSTOMIZE_NEVER;
            }
            wvTemplate.chCustomizable[1] = TEXT('\0');
            LoadString(g_hAppInst, IDS_CURRENT_TEMPLATE_DESC, wvTemplate.szDecsription, ARRAYSIZE(wvTemplate.szDecsription));
            wvTemplate.iId = g_nNextTemplateId++;   // Used to identify the selected item in listview
            if (AddInfoToTemplateList(&wvTemplate))
            {
                fRet = TRUE;    // Return TRUE if we get even one template in the list.
            }
        }

        TCHAR szKey[MAX_PATH];

        // Read-in info of every template
        for (int i = 0; RegEnumKey(hkey, i, szKey, ARRAYSIZE(szKey)) == ERROR_SUCCESS; i++)
        {
            // Return TRUE if even one template is read-in.
            if (LoadTemplateInfo(hkey, szKey))
            {
                fRet = TRUE;
            }
        }
        RegCloseKey(hkey);
    }
    return fRet;
}


BOOL LoadTemplateInfo(HKEY hkey, LPCTSTR pszKey)
{
    BOOL fRet = FALSE;
    HKEY hkeyTemplate;

    if (RegOpenKeyEx(hkey, pszKey, 0, KEY_READ, &hkeyTemplate) == ERROR_SUCCESS)
    {
        DWORD cbSize, dwType;
        WebViewTemplateInfo wvTemplate;

        cbSize = sizeof(wvTemplate.szFriendlyName);
        if ((RegQueryValueEx(hkeyTemplate, REG_VAL_DISPLAYNAME, NULL, &dwType, (LPBYTE)wvTemplate.szFriendlyName, &cbSize) == ERROR_SUCCESS)
                && wvTemplate.szFriendlyName[0])
        {
            // Save the key name
            lstrcpyn(wvTemplate.szKey, pszKey, sizeof(wvTemplate.szKey));
            // Read in the Read-only flag.
            cbSize = sizeof(wvTemplate.chCustomizable);
            if (RegQueryValueEx(hkeyTemplate, REG_VAL_CUSTOMIZABLE, NULL, &dwType, (LPBYTE)wvTemplate.chCustomizable, &cbSize) != ERROR_SUCCESS)
            {
                wvTemplate.chCustomizable[0] = CUSTOMIZE_NEVER;
                wvTemplate.chCustomizable[1] = TEXT('\0');
            }

            // Read in the Template file name.
            cbSize = sizeof(wvTemplate.szTemplateFile);
            if (RegQueryValueEx(hkeyTemplate, REG_VAL_TEMPLATEFILE, NULL, &dwType, (LPBYTE)wvTemplate.szTemplateFile, &cbSize) != ERROR_SUCCESS)
            {
                wvTemplate.szTemplateFile[0] = TEXT('\0');
            }

            // Read in the Preview bitmap file name.
            cbSize = sizeof(wvTemplate.szPreviewBitmapFile);
            if (RegQueryValueEx(hkeyTemplate, REG_VAL_PREVIEWBITMAPFILE, NULL, &dwType, (LPBYTE)wvTemplate.szPreviewBitmapFile, &cbSize) != ERROR_SUCCESS)
            {
                wvTemplate.szPreviewBitmapFile[0] = TEXT('\0');
            }

            // Read in the template Decsription.
            cbSize = sizeof(wvTemplate.szDecsription);
            if (RegQueryValueEx(hkeyTemplate, REG_VAL_DESCRIPTION, NULL, &dwType, (LPBYTE)wvTemplate.szDecsription, &cbSize) != ERROR_SUCCESS)
            {
                wvTemplate.szDecsription[0] = TEXT('\0');
            }

            wvTemplate.iId = g_nNextTemplateId++;   // Used to identify the selected item in listview

            // Add this TemplateInfo to the TemplateInfo list.
            fRet = AddInfoToTemplateList(&wvTemplate);
        }
        RegCloseKey(hkeyTemplate);
    }
    return fRet;
}


// Adds template info read-in from the registry into a DSA
BOOL AddInfoToTemplateList(WebViewTemplateInfo* pwvTemplate)
{
    BOOL fRet = FALSE;

    if (pwvTemplate)
    {
        if (g_hdsaTemplateInfo == NULL)
        {
            g_hdsaTemplateInfo = DSA_Create(sizeof(WebViewTemplateInfo), DXA_GROWTH_CONST);
        }

        if (g_hdsaTemplateInfo)
        {
            if (DSA_AppendItem(g_hdsaTemplateInfo, pwvTemplate) != -1)
            {
                fRet = TRUE;
            }
        }
    }
    return fRet;
}


void PageA3OnDestroy()
{
    EmptyTemplateList();    // Free the list
}


BOOL InitTemplateListView(HWND hDlg)
{
    BOOL fRet = FALSE;
    
    g_hwndLV_Templates = GetDlgItem(hDlg, IDC_LIST_TEMPLATES);
    if (g_hwndLV_Templates && g_hdsaTemplateInfo)
    {
        // Add the single column that we want.
        LV_COLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT;
        lvc.iSubItem = 0;
        ListView_InsertColumn(g_hwndLV_Templates, 0, &lvc);
        // Fill up the listview
        FillTemplateListView(FALSE);
        fRet = TRUE;
    }
    return fRet;
}


// Fill the listview with templates' friendly names. The parameter is not used currently.
void FillTemplateListView(BOOL fEmpty)
{
    if (g_hdsaTemplateInfo) {

        // Disable redraws while we mess repeatedly with the listview contents.
        SendMessage(g_hwndLV_Templates, WM_SETREDRAW, FALSE, 0);

// Not used currently
#if 0
        if (fEmpty)
        {
            EmptyTemplateListview();
            EmptyTemplateList();
        }
#endif

        // Add each template to the listview.
        for (int nTemplate = 0; nTemplate < DSA_GetItemCount(g_hdsaTemplateInfo); nTemplate++)
        {
            // First, get the Template
            WebViewTemplateInfo wvTemplate;
            if (GetTemplateInfoByIndex(nTemplate, &wvTemplate))
            {
                AppendTemplateToListView(&wvTemplate);
            }
        }

        // Reenable redraws.
        SendMessage(g_hwndLV_Templates, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(g_hwndLV_Templates, NULL, TRUE);
    }
}


// Not used currently. Could be used if we have a Reset All or Delete All option.
#if 0
void EmptyTemplateListview()
{
    if (g_hdsaTemplateInfo) {

        SendMessage(g_hwndLV_Templates, WM_SETREDRAW, FALSE, 0);
        int cTemplate = DSA_GetItemCount(g_hdsaTemplateInfo);
        for (int nTemplate = 0; nTemplate < cTemplate; nTemplate++)
        {
            ListView_DeleteItem(g_hwndLV_Templates, 0);
        }
        SendMessage(g_hwndLV_Templates, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(g_hwndLV_Templates, NULL, TRUE);
    }
}
#endif


// Frees each item of the template list. It finally destroys the list as well.
void EmptyTemplateList()
{
    if (g_hdsaTemplateInfo)
    {
        // DSA_Destroy will implicitly delete the items too
        DSA_Destroy(g_hdsaTemplateInfo);
        g_hdsaTemplateInfo = NULL;
    }
}


// Given a listview (which should be in sync with the DSA list) index, it returns the template info
BOOL GetTemplateInfoByIndex(int iIndex, WebViewTemplateInfo* pwvTemplate)
{
    BOOL fRet = FALSE;
    if (g_hdsaTemplateInfo)
    {
        if (DSA_GetItem(g_hdsaTemplateInfo, iIndex, pwvTemplate) != -1)
        {
            fRet = TRUE;
        }
    }
    return fRet;
}


// Appends a new template's FriendlyName to the end of the listview
int AppendTemplateToListView(WebViewTemplateInfo* pwvTemplate)
{
    // Construct the listview item.
    LV_ITEM lvi = {0};
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = 0x7FFFFFFF; // Insert at the end.
    lvi.pszText = pwvTemplate->szFriendlyName;
    lvi.lParam = pwvTemplate->iId;

    int iIndex = ListView_InsertItem(g_hwndLV_Templates, &lvi);
    if (iIndex != -1)
    {
        ListView_SetColumnWidth(g_hwndLV_Templates, 0, LVSCW_AUTOSIZE);
    }
    return iIndex;
}


// Currently, the 0'th is chosen as the default
void SetDefaultSelection()
{
    if (g_hdsaTemplateInfo && DSA_GetItemCount(g_hdsaTemplateInfo))
    {
        SetSelectionByIndex(0); // By default, select the '0'th item.
    }
    EnableWindow(g_hwndLV_Templates, TRUE);
}


// Sets the selection in the listview, given an index
void SetSelectionByIndex(int iIndex)
{
    if (iIndex >= 0 && g_hdsaTemplateInfo && iIndex < DSA_GetItemCount(g_hdsaTemplateInfo))
    {
        // Set the focus on the 'iIndex'th item
        ListView_SetItemState(g_hwndLV_Templates, iIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

        // Get the 'iIndex'th item
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iIndex;
        ListView_GetItem(g_hwndLV_Templates, &lvi);

        WebViewTemplateInfo wvTemplate;
        GetTemplateInfoById((int)lvi.lParam, &wvTemplate);

        // Set the customize checkbox.
        SetCheck_Customizable(wvTemplate.chCustomizable);

        // Show/Hide the customized preview text.
        SetText_CustomizedPreview(wvTemplate.chCustomizable, wvTemplate.szPreviewBitmapFile);

        // Set the description.
        SetTemplateDescription(wvTemplate.szDecsription);

        // Show the Preview bitmap.
        ShowPreviewBitmap(wvTemplate.szPreviewBitmapFile);
    }
}


// Given a template's id, it returns other info by searching for it in the template list
int GetTemplateInfoById(int iId, WebViewTemplateInfo* pwvTemplate)
{
    int iIndex = -1;
    WebViewTemplateInfo wvTemplate;
    if (g_hdsaTemplateInfo) {
        // For each template
        for (int nTemplate = 0; nTemplate < DSA_GetItemCount(g_hdsaTemplateInfo)
                && GetTemplateInfoByIndex(nTemplate, &wvTemplate); nTemplate++)
        {
            // If the Id's match
            if (wvTemplate.iId == iId)
            {
                *pwvTemplate = wvTemplate;
                iIndex = nTemplate;
                break;
            }
        }
    }
    return iIndex;
}


// Not used currently. Given a template id, this func. sets it to be selected in the listview
#if 0
void SetSelectionById(int iId)
{
    WebViewTemplateInfo wvTemplate;
    int iIndex = GetTemplateInfoById(iId, &wvTemplate);
    if (iIndex > 0)
    {
        SetSelectionByIndex(iIndex);
    }
}
#endif

#define toupper(c) ((TCHAR)CharUpper((LPTSTR)c))

// Sets the text that specifies whether the template is customizable or not
void SetCheck_Customizable(LPCTSTR lpszCustomizable)
{
    // Check/Uncheck it
    if (toupper(lpszCustomizable[0]) == CUSTOMIZE_ALWAYS || toupper(lpszCustomizable[0]) == CUSTOMIZE_DEFAULT)
    {
        CheckDlgButton(g_hwndDlg, IDC_CHECK1, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(g_hwndDlg, IDC_CHECK1, BST_UNCHECKED);
    }
    // Enable/Disable it
    if (toupper(lpszCustomizable[0]) == CUSTOMIZE_ALWAYS || toupper(lpszCustomizable[0]) == CUSTOMIZE_NEVER)
    {
        EnableWindow(GetDlgItem(g_hwndDlg, IDC_CHECK1), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(g_hwndDlg, IDC_CHECK1), TRUE);
    }
}


// Show the customized preview text for the "Current" template. Hide it otherwise.
void SetText_CustomizedPreview(LPCTSTR pszCustomizable, LPCTSTR pszPreviewBitmapFile)
{
    TCHAR szText[MAX_PATH];
    szText[0] = TEXT('\0');
    if (!pszPreviewBitmapFile || !pszPreviewBitmapFile[0])
    {
        LoadString(g_hAppInst, IDS_NOTAVAILABLE, szText, ARRAYSIZE(szText));
    }
    else if (pszCustomizable[0] == CUSTOMIZE_ALWAYS)
    {
        LoadString(g_hAppInst, IDS_ORIGINAL, szText, ARRAYSIZE(szText));
    }
    else
    {
        LoadString(g_hAppInst, IDS_PREVIEW, szText, ARRAYSIZE(szText));
    }
    SendMessage(GetDlgItem(g_hwndDlg, IDC_TEXT_CUSTOMIZED), WM_SETTEXT, 0, (LPARAM)szText);
}


// Sets the given desc. text in the desc. dialog item
void SetTemplateDescription(LPCTSTR lpszDesc)
{
    SendMessage(g_hwndText_Desc, WM_SETTEXT, 0, (LPARAM)lpszDesc);
}


// Makes the given bmp file be shown in the preview dialog item
void ShowPreviewBitmap(LPCTSTR pszPreviewBitmapFile)
{
    if (pszPreviewBitmapFile && pszPreviewBitmapFile[0])
    {
        DisplayPreview(pszPreviewBitmapFile, g_hwndBitmap_Preview);
    }
    else
    {
        DisplayUnknown(g_hwndBitmap_Preview);
    }
}


// Changes all the appropriate dialog items
void TemplateLV_OnItemChanged()
{
    int iIndex = ListView_GetNextItem(g_hwndLV_Templates, -1, LVNI_SELECTED);
    SetSelectionByIndex(iIndex);
}


// Sets the appropriate wizard buttons
void PageA3_OnSetActive (HWND hDlg)
{
    g_pCommonInfo->OnSetActive(hDlg);
    // Make the preview etc. match the selection
    TemplateLV_OnItemChanged();
}   /*  end PageA3_OnSetActive() */


void PageA3CleanUp()
{
    // Remove desktop.ini only if we had copied it during this wizard session
    if (g_bTemplateCopied)
    {
        DeleteFile(g_szFullHTMLFile);
        g_bTemplateCopied = FALSE;
    }
}


void PageA3_OnWizardBack(HWND hDlg)
{
    PageA3CleanUp();
    g_pCommonInfo->OnBack(hDlg);
}


// Launches the template in an editor, if it is editable
void PageA3_OnWizardNext (HWND hDlg)
{
    WebViewTemplateInfo wvTemplate;
    int iIndex = ListView_GetNextItem(g_hwndLV_Templates, -1, LVNI_SELECTED);

    if (GetTemplateInfoByIndex(iIndex, &wvTemplate))
    {
        lstrcpy(g_szTemplateFile, wvTemplate.szTemplateFile);

        // Copy the reg key name
        lstrcpyn(g_szKey, wvTemplate.szKey, ARRAYSIZE(g_szKey));
        
        // Copy the PreviewBitmapFile name into g_szPreviewBitmapFile
        lstrcpy(g_szPreviewBitmapFile, wvTemplate.szPreviewBitmapFile);

        TCHAR szKeyVersion[MAX_PATH];
        GetWebViewTemplateKeyVersion(wvTemplate.szKey, szKeyVersion, ARRAYSIZE(szKeyVersion));
        
        if ((wvTemplate.chCustomizable[0] == CUSTOMIZE_NEVER)
                || ((szKeyVersion[0] && (StrCmpI(szKeyVersion, TEXT("IE4")) != 0))    // Only for non-IE4 templates
                && (wvTemplate.chCustomizable[0] == CUSTOMIZE_ONCHOICE)
                && !IsDlgButtonChecked(g_hwndDlg, IDC_CHECK1)))     // No local copy if we are not changing the template
        {
            //Do not launch HTML editor.
            g_iFlagA = SYSTEM_TEMPLATE;
        }
        else
        {
            g_iFlagA = CUSTOM_TEMPLATE;

            CopyTemplate();
        }

        if (IsDlgButtonChecked(g_hwndDlg, IDC_CHECK1)) // The selected item is a custom template.
        {
            //Launch HTML editor.
            LaunchRegisteredEditor();
        }
    }
    else
    {
        g_iFlagA = NO_TEMPLATE;
        g_szTemplateFile[0] = TEXT('\0');
    }
    
    g_pCommonInfo->OnNext(hDlg);
}

