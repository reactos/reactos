/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/toolspage.cpp
 * PURPOSE:     Tools page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *              Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"
#include "xmldomparser.hpp"
#include "utils.h"
#include "listviewfuncs.h"
#include "uxthemesupp.h"

static HWND hToolsPage     = NULL;
static HWND hToolsListCtrl = NULL;
static int  iSortedColumn  = 0;

struct TOOL
{
    TOOL(const _bstr_t& Command,
         const _bstr_t& DefParam,
         const _bstr_t& AdvParam) :
        m_Command(Command),
        m_DefParam(DefParam),
        m_AdvParam(AdvParam)
    { }

    ~TOOL(void)
    { }

    DWORD Run(BOOL bUseAdvParams)
    {
        return RunCommand(m_Command, bUseAdvParams ? m_AdvParam : m_DefParam, SW_SHOW);
    }

    _bstr_t m_Command;
    _bstr_t m_DefParam;
    _bstr_t m_AdvParam;
};

static void AddTool(IXMLDOMElement*, BOOL);

static HRESULT
ParseToolsList(IXMLDOMDocument* pXMLDom, BOOL bIsStandard)
{
    static const _bstr_t XMLFileTag(L"MSCONFIGTOOLFILE");
    static const _bstr_t XMLToolsTag(L"MSCONFIGTOOLS");

    HRESULT hr = S_OK;

    IXMLDOMNode    *pIterator = NULL, *pTmp = NULL;
    IXMLDOMElement* pEl       = NULL;
    DOMNodeType     type;
    _bstr_t         tagName;

    if (!pXMLDom)
        return E_POINTER; // E_INVALIDARG

    pXMLDom->get_documentElement(&pEl);

    pEl->get_tagName(&tagName.GetBSTR());
    _wcsupr(tagName);
    if (tagName == XMLFileTag)
    {
        pEl->get_firstChild(&pIterator); SAFE_RELEASE(pEl);
        while (pIterator)
        {
            pIterator->get_nodeType(&type);
            if (type == NODE_ELEMENT)
            {
                pIterator->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pEl) /* IID_PPV_ARGS(&pEl) */);

                pEl->get_tagName(&tagName.GetBSTR());
                _wcsupr(tagName);
                if (tagName == XMLToolsTag)
                {
                    pEl->get_firstChild(&pIterator); SAFE_RELEASE(pEl);
                    while (pIterator)
                    {
                        pIterator->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pEl) /* IID_PPV_ARGS(&pEl) */);
                        AddTool(pEl, bIsStandard);
                        SAFE_RELEASE(pEl);

                        pIterator->get_nextSibling(&pTmp);
                        SAFE_RELEASE(pIterator); pIterator = pTmp;
                    }
                    // SAFE_RELEASE(pIterator);

                    break;
                }

                SAFE_RELEASE(pEl);
            }

            pIterator->get_nextSibling(&pTmp);
            SAFE_RELEASE(pIterator); pIterator = pTmp;
        }
        // SAFE_RELEASE(pIterator);
    }
    else if (tagName == XMLToolsTag)
    {
        pEl->get_firstChild(&pIterator); SAFE_RELEASE(pEl);
        while (pIterator)
        {
            pIterator->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pEl) /* IID_PPV_ARGS(&pEl) */);
            AddTool(pEl, bIsStandard);
            SAFE_RELEASE(pEl);

            pIterator->get_nextSibling(&pTmp);
            SAFE_RELEASE(pIterator); pIterator = pTmp;
        }
        // SAFE_RELEASE(pIterator);
    }

    SAFE_RELEASE(pEl);

    return hr;
}

static void
AddItem(BOOL bIsStandard, const _bstr_t& name, const _bstr_t& descr, TOOL* tool)
{
    LPWSTR lpszStandard;
    LVITEM item = {};

    assert(tool);

    item.mask   = LVIF_TEXT | LVIF_PARAM;
    item.lParam = (LPARAM)tool;

    item.pszText  = (LPWSTR)name;
    item.iSubItem = 0;
    // item.iItem    = ListView_GetItemCount(hToolsListCtrl);

    ListView_InsertItem(hToolsListCtrl, &item);

    if (bIsStandard)
    {
        lpszStandard = LoadResourceString(hInst, IDS_YES);
        ListView_SetItemText(hToolsListCtrl, item.iItem, 1, lpszStandard);
        MemFree(lpszStandard);
    }
    else
    {
        lpszStandard = LoadResourceString(hInst, IDS_NO);
        ListView_SetItemText(hToolsListCtrl, item.iItem, 1, lpszStandard);
        MemFree(lpszStandard);
    }

    ListView_SetItemText(hToolsListCtrl, item.iItem, 2, (LPWSTR)descr);
}

static void
AddTool(IXMLDOMElement* pXMLTool, BOOL bIsStandard)
{
    TOOL* tool;
    _variant_t varLocID, varName, varPath,
               varDefOpt, varAdvOpt, varHelp;

    assert(pXMLTool);

    pXMLTool->getAttribute(_bstr_t(L"_locID")     , &varLocID );
    pXMLTool->getAttribute(_bstr_t(L"NAME")       , &varName  );
    pXMLTool->getAttribute(_bstr_t(L"PATH")       , &varPath  );
    pXMLTool->getAttribute(_bstr_t(L"DEFAULT_OPT"), &varDefOpt);
    pXMLTool->getAttribute(_bstr_t(L"ADV_OPT")    , &varAdvOpt);
    pXMLTool->getAttribute(_bstr_t(L"HELP")       , &varHelp  );

    // TODO: check if the tool really exists... ??

    tool = new TOOL(_bstr_t(varPath), _bstr_t(varDefOpt), _bstr_t(varAdvOpt));
    AddItem(bIsStandard, _bstr_t(varName), _bstr_t(varHelp), tool);
}

static void
FillListView(void)
{
    IXMLDOMDocument* pXMLDom = NULL;

    if (!SUCCEEDED(InitXMLDOMParser()))
        return;

    if (SUCCEEDED(CreateAndInitXMLDOMDocument(&pXMLDom)))
    {
        // Load the internal tools list.
        if (LoadXMLDocumentFromResource(pXMLDom, L"MSCFGTL.XML"))
            ParseToolsList(pXMLDom, TRUE);

        // Try to load the user-provided tools list. If it doesn't exist,
        // then the second list-view's column "Standard" tool is removed.
        if (LoadXMLDocumentFromFile(pXMLDom, L"MSCFGTLC.XML", TRUE))
            ParseToolsList(pXMLDom, FALSE);
        else
            ListView_DeleteColumn(hToolsListCtrl, 1);
    }

    SAFE_RELEASE(pXMLDom);
    UninitXMLDOMParser();
}

static size_t
BuildCommandLine(LPWSTR lpszDest, LPCWSTR lpszCmdLine, LPCWSTR lpszParam, size_t bufSize)
{
    size_t numOfChars = 0; // The null character is counted in ExpandEnvironmentStrings(...).
    // TODO: Take into account the "plus one" for numOfChars for ANSI version (see MSDN for more details).

    if (lpszCmdLine && *lpszCmdLine)
    {
        numOfChars += ExpandEnvironmentStringsW(lpszCmdLine, NULL, 0);
        if (lpszDest)
            ExpandEnvironmentStringsW(lpszCmdLine, lpszDest, (DWORD)bufSize); // TODO: size_t to DWORD conversion !

        if (lpszParam && *lpszParam)
        {
            ++numOfChars;
            if (lpszDest)
                wcscat(lpszDest, L" ");
        }
    }

    if (lpszParam && *lpszParam)
    {
        numOfChars += wcslen(lpszParam);
        if (lpszDest)
            wcscat(lpszDest, lpszParam);
    }

    return numOfChars;
}

#define Button_IsEnabled(hwndCtl) IsWindowEnabled((hwndCtl))

static void Update_States(int iSelectedItem)
{
    TOOL* tool;
    LVITEM item = {};

    assert(hToolsPage);

    item.mask  = LVIF_PARAM;
    item.iItem = iSelectedItem;

    if (ListView_GetItem(hToolsListCtrl, &item)) // (item.iItem > -1) // TODO: corriger ailleurs ce genre de code...
    {
        LPWSTR lpszCmdLine = NULL;
        size_t numOfChars  = 0;
        tool = reinterpret_cast<TOOL*>(item.lParam);

        ListView_EnsureVisible(hToolsListCtrl, item.iItem, FALSE);

        Button_Enable(GetDlgItem(hToolsPage, IDC_BTN_RUN), TRUE);

        if (!*(wchar_t*)tool->m_AdvParam)
        {
            ShowWindow(GetDlgItem(hToolsPage, IDC_CBX_TOOLS_ADVOPT), SW_HIDE);
            Button_Enable(GetDlgItem(hToolsPage, IDC_CBX_TOOLS_ADVOPT), FALSE);
        }
        else
        {
            Button_Enable(GetDlgItem(hToolsPage, IDC_CBX_TOOLS_ADVOPT), TRUE);
            ShowWindow(GetDlgItem(hToolsPage, IDC_CBX_TOOLS_ADVOPT), SW_NORMAL);
        }

        if ( (Button_IsEnabled(GetDlgItem(hToolsPage, IDC_CBX_TOOLS_ADVOPT))) &&
             (Button_GetCheck(GetDlgItem(hToolsPage, IDC_CBX_TOOLS_ADVOPT)) == BST_CHECKED) )
        {
            numOfChars = BuildCommandLine(NULL, tool->m_Command, tool->m_AdvParam, 0);
            lpszCmdLine = (LPWSTR)MemAlloc(0, numOfChars * sizeof(WCHAR));
            BuildCommandLine(lpszCmdLine, tool->m_Command, tool->m_AdvParam, numOfChars);
        }
        else
        {
            numOfChars = BuildCommandLine(NULL, tool->m_Command, tool->m_DefParam, 0);
            lpszCmdLine = (LPWSTR)MemAlloc(0, numOfChars * sizeof(WCHAR));
            BuildCommandLine(lpszCmdLine, tool->m_Command, tool->m_DefParam, numOfChars);
        }

        SendDlgItemMessage(hToolsPage, IDC_TOOLS_CMDLINE, WM_SETTEXT, 0, (LPARAM)lpszCmdLine);

        MemFree(lpszCmdLine);
    }
    else
    {
        ShowWindow(GetDlgItem(hToolsPage, IDC_CBX_TOOLS_ADVOPT), SW_HIDE);
        Button_Enable(GetDlgItem(hToolsPage, IDC_CBX_TOOLS_ADVOPT), FALSE);
        Button_Enable(GetDlgItem(hToolsPage, IDC_BTN_RUN), FALSE);
    }
}

static BOOL RunSelectedTool(VOID)
{
    BOOL Success = FALSE;
    BOOL bUseAdvParams;

    LVITEM item = {};
    item.mask = LVIF_PARAM;
    item.iItem = ListView_GetSelectionMark(hToolsListCtrl);
    ListView_GetItem(hToolsListCtrl, &item);

    if (ListView_GetItem(hToolsListCtrl, &item)) // (item.iItem > -1) // TODO: corriger ailleurs ce genre de code...
    {
        if ( (Button_IsEnabled(GetDlgItem(hToolsPage, IDC_CBX_TOOLS_ADVOPT))) &&
             (Button_GetCheck(GetDlgItem(hToolsPage, IDC_CBX_TOOLS_ADVOPT)) == BST_CHECKED) )
            bUseAdvParams = TRUE;
        else
            bUseAdvParams = FALSE;

        // Values greater (strictly) than 32 indicate success (see MSDN documentation for ShellExecute(...) API).
        Success = (reinterpret_cast<TOOL*>(item.lParam)->Run(bUseAdvParams) > 32);
    }

    return Success;
}

extern "C" {

INT_PTR CALLBACK
ToolsPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            hToolsPage     = hDlg;
            hToolsListCtrl = GetDlgItem(hToolsPage, IDC_TOOLS_LIST);

            //
            // Initialize the styles.
            //
            DWORD dwStyle = ListView_GetExtendedListViewStyle(hToolsListCtrl);
            ListView_SetExtendedListViewStyle(hToolsListCtrl, dwStyle | LVS_EX_FULLROWSELECT);
            SetWindowTheme(hToolsListCtrl, L"Explorer", NULL);

            //
            // Initialize the application page's controls.
            //
            LVCOLUMN column = {};

            // First column : Tool's name.
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = LoadResourceString(hInst, IDS_TOOLS_COLUMN_NAME);
            column.cx = 150;
            ListView_InsertColumn(hToolsListCtrl, 0, &column);
            MemFree(column.pszText);

            // Second column : Whether the tool is a standard one or not.
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = LoadResourceString(hInst, IDS_TOOLS_COLUMN_STANDARD);
            column.cx = 60;
            ListView_InsertColumn(hToolsListCtrl, 1, &column);
            MemFree(column.pszText);

            // Third column : Description.
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = LoadResourceString(hInst, IDS_TOOLS_COLUMN_DESCR);
            column.cx = 500;
            ListView_InsertColumn(hToolsListCtrl, 2, &column);
            MemFree(column.pszText);

            //
            // Populate and sort the list.
            //
            FillListView();
            ListView_Sort(hToolsListCtrl, 0);

            // Force an update in case of an empty list (giving focus on it when empty won't emit a LVN_ITEMCHANGED message).
            Update_States(-1 /* Wrong index to initialize all the controls with their default state (i.e. disabled) */);

            PropSheet_UnChanged(GetParent(hToolsPage), hToolsPage);

            return TRUE;
        }

        case WM_DESTROY:
        {
            LVITEM lvitem = {};
            lvitem.mask  = LVIF_PARAM;
            lvitem.iItem = -1; // From the beginning.

            while ((lvitem.iItem = ListView_GetNextItem(hToolsListCtrl, lvitem.iItem, LVNI_ALL)) != -1)
            {
                // ListView_Update();   // Updates a list-view item.
                // ListView_FindItem(); // peut être intéressant pour faire de la recherche itérative à partir du nom (ou partie du...) de l'item.

                ListView_GetItem(hToolsListCtrl, &lvitem);

                delete reinterpret_cast<TOOL*>(lvitem.lParam);
                lvitem.lParam = NULL;
            }
            ListView_DeleteAllItems(hToolsListCtrl);

            return 0;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BTN_RUN:
                {
                    RunSelectedTool();
                    return TRUE;
                }

                case IDC_CBX_TOOLS_ADVOPT:
                {
                    Update_States(ListView_GetSelectionMark(hToolsListCtrl));
                    return TRUE;
                }

                default:
                    return FALSE;
            }
            return FALSE;
        }

        case WM_NOTIFY:
        {
            if (((LPNMHDR)lParam)->hwndFrom == hToolsListCtrl)
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case LVN_ITEMCHANGED:
                    {
                        if ( (((LPNMLISTVIEW)lParam)->uChanged  & LVIF_STATE) && /* The state has changed */
                             (((LPNMLISTVIEW)lParam)->uNewState & LVIS_SELECTED) /* The item has been (de)selected */ )
                        {
                            Update_States(((LPNMLISTVIEW)lParam)->iItem);
                        }

                        return TRUE;
                    }

                    case NM_DBLCLK:
                    case NM_RDBLCLK:
                    {
                        RunSelectedTool();
                        return TRUE;
                    }

                    case LVN_COLUMNCLICK:
                    {
                        int iSortingColumn = ((LPNMLISTVIEW)lParam)->iSubItem;

                        ListView_SortEx(hToolsListCtrl, iSortingColumn, iSortedColumn);
                        iSortedColumn = iSortingColumn;

                        return TRUE;
                    }

                    default:
                        break;
                }
            }
            else
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case PSN_APPLY:
                    {
                        // Since there are nothing to modify, applying modifications
                        // cannot return any error.
                        SetWindowLongPtr(hToolsPage, DWLP_MSGRESULT, PSNRET_NOERROR);
                        PropSheet_UnChanged(GetParent(hToolsPage), hToolsPage);
                        return TRUE;
                    }

                    case PSN_HELP:
                    {
                        MessageBoxW(hToolsPage, L"Help not implemented yet!", L"Help", MB_ICONINFORMATION | MB_OK);
                        return TRUE;
                    }

                    case PSN_KILLACTIVE: // Is going to lose activation.
                    {
                        // Changes are always valid of course.
                        SetWindowLongPtr(hToolsPage, DWLP_MSGRESULT, FALSE);
                        return TRUE;
                    }

                    case PSN_QUERYCANCEL:
                    {
                        // Allows cancellation since there are nothing to cancel...
                        SetWindowLongPtr(hToolsPage, DWLP_MSGRESULT, FALSE);
                        return TRUE;
                    }

                    case PSN_QUERYINITIALFOCUS:
                    {
                        // Give the focus on and select the first item.
                        ListView_SetItemState(hToolsListCtrl, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

                        SetWindowLongPtr(hToolsPage, DWLP_MSGRESULT, (LONG_PTR)hToolsListCtrl);
                        return TRUE;
                    }

                    //
                    // DO NOT TOUCH THESE NEXT MESSAGES, THEY ARE OK LIKE THIS...
                    //
                    case PSN_RESET: // Perform final cleaning, called before WM_DESTROY.
                        return TRUE;

                    case PSN_SETACTIVE: // Is going to gain activation.
                    {
                        SetWindowLongPtr(hToolsPage, DWLP_MSGRESULT, 0);
                        return TRUE;
                    }

                    default:
                        break;
                }
            }

            return FALSE;
        }

        default:
            return FALSE;
    }

    // return FALSE;
}

}

/* EOF */
