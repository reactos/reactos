//------------------------------------------------------------------------------
// icmdtgt.cpp
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
//
// Author
//     bash
//
// History
//      7-19-97     created     (bash)
//
// Implementation of IOleCommandTarget
//
//------------------------------------------------------------------------------

#include "stdafx.h"

#include <mshtmcid.h>
#include <designer.h>

//#include "mfcincl.h"
#include "triedit.h"
#include "document.h"
#include "triedcid.h"       //TriEdit Command IDs here.
#include "dispatch.h"
#include "undo.h"

#define CMDSTATE_NOTSUPPORTED  0
#define CMDSTATE_DISABLED      OLECMDF_SUPPORTED
#define CMDSTATE_UP           (OLECMDF_SUPPORTED | OLECMDF_ENABLED)
#define CMDSTATE_DOWN         (OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_LATCHED)
#define CMDSTATE_NINCHED      (OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED)

// Mapping from TriEdit to Trident commands
typedef struct {
ULONG cmdTriEdit;
ULONG cmdTrident;    
} CMDMAP;

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::MapTriEditCommand 
//
// Map the given TriEdit IDM to the equivalent Trident IDM. 
//
// Return:
//   Mapped command under *pCmdTrident and S_OK for a valid command.
//   E_FAIL for an invalid command.
//

HRESULT CTriEditDocument::MapTriEditCommand(ULONG cmdTriEdit, ULONG *pCmdTrident)
{
    static CMDMAP rgCmdMap[] = {
        { IDM_TRIED_ACTIVATEACTIVEXCONTROLS, IDM_NOACTIVATENORMALOLECONTROLS }, 
        { IDM_TRIED_ACTIVATEAPPLETS, IDM_NOACTIVATEJAVAAPPLETS },
        { IDM_TRIED_ACTIVATEDTCS, IDM_NOACTIVATEDESIGNTIMECONTROLS },
        { IDM_TRIED_BACKCOLOR, IDM_BACKCOLOR },
        { IDM_TRIED_BLOCKFMT, IDM_BLOCKFMT },
        { IDM_TRIED_BOLD, IDM_BOLD },
        { IDM_TRIED_BROWSEMODE, IDM_BROWSEMODE },
        { IDM_TRIED_COPY, IDM_COPY },
        { IDM_TRIED_CUT, IDM_CUT },
        { IDM_TRIED_DELETE, IDM_DELETE },
        { IDM_TRIED_EDITMODE, IDM_EDITMODE },
        { IDM_TRIED_FIND, IDM_FIND },
        { IDM_TRIED_FONT, IDM_FONT },
        { IDM_TRIED_FONTNAME, IDM_FONTNAME },
        { IDM_TRIED_FONTSIZE, IDM_FONTSIZE },
        { IDM_TRIED_FORECOLOR, IDM_FORECOLOR },
        { IDM_TRIED_GETBLOCKFMTS, IDM_GETBLOCKFMTS },
        { IDM_TRIED_HYPERLINK, IDM_HYPERLINK },
        { IDM_TRIED_IMAGE, IDM_IMAGE },
        { IDM_TRIED_INDENT, IDM_INDENT },
        { IDM_TRIED_ITALIC, IDM_ITALIC },
        { IDM_TRIED_JUSTIFYCENTER, IDM_JUSTIFYCENTER },
        { IDM_TRIED_JUSTIFYLEFT, IDM_JUSTIFYLEFT },
        { IDM_TRIED_JUSTIFYRIGHT, IDM_JUSTIFYRIGHT },
        { IDM_TRIED_ORDERLIST, IDM_ORDERLIST },
        { IDM_TRIED_OUTDENT, IDM_OUTDENT },
        { IDM_TRIED_PASTE, IDM_PASTE },
        { IDM_TRIED_PRINT, IDM_PRINT },
        { IDM_TRIED_REDO, IDM_REDO },
        { IDM_TRIED_REMOVEFORMAT, IDM_REMOVEFORMAT },
        { IDM_TRIED_SELECTALL, IDM_SELECTALL },
        { IDM_TRIED_SHOWBORDERS, IDM_SHOWZEROBORDERATDESIGNTIME },
        { IDM_TRIED_SHOWDETAILS, IDM_SHOWALLTAGS },
        { IDM_TRIED_UNDERLINE, IDM_UNDERLINE },
        { IDM_TRIED_UNDO, IDM_UNDO },
        { IDM_TRIED_UNLINK, IDM_UNLINK },
        { IDM_TRIED_UNORDERLIST, IDM_UNORDERLIST }
    };

    if (NULL == pCmdTrident)
        return E_POINTER;

    for (int i=0; i < sizeof(rgCmdMap)/sizeof(CMDMAP); ++i)
    {
        if (cmdTriEdit == rgCmdMap[i].cmdTriEdit)
        {
            *pCmdTrident = rgCmdMap[i].cmdTrident;
            return S_OK;
        }
    }

    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::SetUpDefaults
//
// Set Trident flags to the TriEdit default values:
//
//      IDM_PRESERVEUNDOALWAYS                  On
//      IDM_NOFIXUPURLSONPASTE                  On
//      IDM_NOACTIVATEDESIGNTIMECONTROLS        Off
//      IDM_NOACTIVATEJAVAAPPLETS               On
//      IDM_NOACTIVATENORMALOLECONTROLS         On
//
//
// No return value.

void CTriEditDocument::SetUpDefaults()
{
    VARIANT var;

    // Turn on Trident's preserve undo flag for setting properties
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = TRUE;

    m_pCmdTgtTrident->Exec(&CMDSETID_Forms3,
             6049, // IDM_PRESERVEUNDOALWAYS
             OLECMDEXECOPT_DONTPROMPTUSER,
             &var,
             NULL);

    // Turn on Trident's url fixup flag for paste and drag-drop
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = TRUE;

    m_pCmdTgtTrident->Exec(&CMDSETID_Forms3,
             2335, // IDM_NOFIXUPURLSONPASTE
             OLECMDEXECOPT_DONTPROMPTUSER,
             &var,
             NULL);

    // Set up defaults for Activating DTCs but not Applets or other ActiveX Controls
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = FALSE;

    m_pCmdTgtTrident->Exec(&CMDSETID_Forms3,
             IDM_NOACTIVATEDESIGNTIMECONTROLS,
             OLECMDEXECOPT_DONTPROMPTUSER,
             &var,
             NULL);

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = TRUE;

    m_pCmdTgtTrident->Exec(&CMDSETID_Forms3,
             IDM_NOACTIVATEJAVAAPPLETS,
             OLECMDEXECOPT_DONTPROMPTUSER,
             &var,
             NULL);

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = TRUE;

    m_pCmdTgtTrident->Exec(&CMDSETID_Forms3,
             IDM_NOACTIVATENORMALOLECONTROLS,
             OLECMDEXECOPT_DONTPROMPTUSER,
             &var,
             NULL);
}

///////////////////////////////////////////////////////////////////////////////
//
//
// CTriEditDocument::SetUpGlyphTable
//
// Load the glyphs from the DLL and install them in Trident's table. No return
// value.
//

void CTriEditDocument::SetUpGlyphTable(BOOL fDetails)
{
    VARIANT var;
    const int RuleMax = 100; // This needs to be updated if we ever have a long rule
    const int PathMax = 256; // For %program files%\common files\microsoft shared\triedit\triedit.dll
    int iGlyphTableStart = IDS_GLYPHTABLESTART;
    int iGlyphTableEnd = fDetails ? IDS_GLYPHTABLEEND : IDS_GLYPHTABLEFORMEND;
    TCHAR szPathName[PathMax];
    TCHAR szRule[RuleMax + PathMax];
    TCHAR szGlyphTable[(RuleMax + PathMax) * (IDS_GLYPHTABLEEND - IDS_GLYPHTABLESTART + 1)];
    TCHAR *pchGlyphTable, *pchTemp;

    // Get full path name for triedit.dll
    ::GetModuleFileName(_Module.GetModuleInstance(),
            szPathName,
            sizeof(szPathName)
            );

    // Load glyph table
    pchGlyphTable = szGlyphTable;
    for (int i = iGlyphTableStart; i <= iGlyphTableEnd; i++)
    {
        ::LoadString(_Module.GetModuleInstance(), i, szRule, RuleMax);
        pchTemp = wcsstr(szRule, _T("!"));
        if (pchTemp) // else bad rule, ignore
        {
            *pchTemp = 0;
            // Copy upto the "!"
            wcscpy(pchGlyphTable, szRule);
            pchGlyphTable += wcslen(szRule);
            // Append pathname
            wcscpy(pchGlyphTable, szPathName);
            pchGlyphTable += wcslen(szPathName);
            // Skip past "!"
            pchTemp = pchTemp + 1;
            // Copy remaining characters
            wcscpy(pchGlyphTable, pchTemp);
            pchGlyphTable += wcslen(pchTemp);
        }
    }
     
    // First empty the glyph table
    m_pCmdTgtTrident->Exec(&CMDSETID_Forms3,
             IDM_EMPTYGLYPHTABLE,
             OLECMDEXECOPT_DONTPROMPTUSER,
             NULL,
             NULL);
    
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(szGlyphTable);
    m_pCmdTgtTrident->Exec(&CMDSETID_Forms3,
             IDM_ADDTOGLYPHTABLE,
             OLECMDEXECOPT_DONTPROMPTUSER,
             &var,
             NULL);
    VariantInit(&var);

}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::QueryStatus
//
// Report on the status of the given array of TriEdit and Trident commands. 
// Pass Trident commands on to Trident. Fix the Trident return value to
// compensate for some inconsistencies. Return S_OK if all goes well, or
// E_FAIL if not.
//


STDMETHODIMP CTriEditDocument::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
                                           OLECMD prgCmds[], OLECMDTEXT *pCmdText)

{
    OLECMD *pCmd;
    INT c;
    HRESULT hr;

    if (pguidCmdGroup && IsEqualGUID((const GUID&)*pguidCmdGroup, GUID_TriEditCommandGroup))
    {
        // Loop through each command in the ary, setting the status of each.
        for (pCmd = prgCmds, c = cCmds; --c >= 0; pCmd++)
        {
            // Assume this is a valid command and set default command status to DISABLED.
            // The state will get reset to UP, DOWN or NOTSUPPORTED in the switch statement below.
            pCmd->cmdf = CMDSTATE_DISABLED;
        
            switch(pCmd->cmdID)
            {
                case IDM_TRIED_IS_1D_ELEMENT:   
                case IDM_TRIED_IS_2D_ELEMENT:
                    {
                        if (SUCCEEDED(GetElement()) && m_pihtmlElement)
                        {
                            pCmd->cmdf = CMDSTATE_UP;
                        }
                        break;
                    }

                case IDM_TRIED_SET_ALIGNMENT:
                    {
                        pCmd->cmdf = CMDSTATE_UP;
                        break;
                    }

                case IDM_TRIED_LOCK_ELEMENT:
                    {
                        if (SUCCEEDED(GetElement()) && m_pihtmlElement)
                        {
                            BOOL f2d=FALSE;
                            if (SUCCEEDED(Is2DElement(m_pihtmlElement, &f2d)) && f2d)
                            {
                                BOOL fLocked=FALSE;
                                pCmd->cmdf =
                                        (SUCCEEDED(IsLocked(m_pihtmlElement, &fLocked)) && fLocked)
                                        ? CMDSTATE_DOWN : CMDSTATE_UP;
                            }
                        }
                        break;
                    }
                case IDM_TRIED_CONSTRAIN:
                    {
                        pCmd->cmdf = (m_fConstrain) ? CMDSTATE_DOWN : CMDSTATE_UP;
                        break;
                    }

                case IDM_TRIED_SEND_TO_BACK:
                case IDM_TRIED_SEND_TO_FRONT:
                case IDM_TRIED_SEND_BACKWARD:
                case IDM_TRIED_SEND_FORWARD:
                case IDM_TRIED_SEND_BEHIND_1D:
                case IDM_TRIED_SEND_FRONT_1D:
                    {
                        if (SUCCEEDED(GetElement()) && m_pihtmlElement)
                        {
                            BOOL f2d=FALSE;

                            if (SUCCEEDED(Is2DElement(m_pihtmlElement, &f2d)) && f2d)
                            {
                                pCmd->cmdf = CMDSTATE_UP;
                            }
                        }
                        break;
                    }

                case IDM_TRIED_NUDGE_ELEMENT:
                    {
                        BOOL f2d = FALSE;

                        if (SUCCEEDED(GetElement()) && m_pihtmlElement
                            && SUCCEEDED(Is2DElement(m_pihtmlElement, &f2d)) && f2d)
                        {
                            BOOL fLock = FALSE;

                            if (!(SUCCEEDED(IsLocked(m_pihtmlElement, &fLock)) && fLock))
                                pCmd->cmdf = CMDSTATE_UP;
                        }
                        break;
                    }

                case IDM_TRIED_MAKE_ABSOLUTE:
                    {
                        if (SUCCEEDED(GetElement()) && m_pihtmlElement)
                        {
                            BOOL f2d = FALSE;

                            if (SUCCEEDED(IsElementDTC(m_pihtmlElement)))
                                break;

                            if (SUCCEEDED(Is2DElement(m_pihtmlElement, &f2d)))
                            {
                                BOOL f2dCapable=FALSE;
                                if ( f2d )
                                {
                                    pCmd->cmdf = CMDSTATE_DOWN;
                                }
                                else if (SUCCEEDED(Is2DCapable(m_pihtmlElement, &f2dCapable)) && f2dCapable)
                                {
                                    pCmd->cmdf = CMDSTATE_UP;
                                }
                            }
                        }
                        break;
                    }

                case IDM_TRIED_SET_2D_DROP_MODE:
                    {
                        pCmd->cmdf = (m_f2dDropMode) ? CMDSTATE_DOWN : CMDSTATE_UP;
                        break;
                    }

                case IDM_TRIED_INSERTROW:
                case IDM_TRIED_DELETEROWS:
                case IDM_TRIED_INSERTCELL:
                case IDM_TRIED_DELETECELLS:
                case IDM_TRIED_INSERTCOL:
                    {
                        pCmd->cmdf = (IsSelectionInTable() == S_OK && GetSelectionTypeInTable() != -1)? CMDSTATE_UP : CMDSTATE_DISABLED;
                        break;
                    }

                case IDM_TRIED_MERGECELLS:
                    {
                        ULONG grf = IsSelectionInTable() == S_OK ? GetSelectionTypeInTable() : 0;
                        pCmd->cmdf =  ( (grf != -1) && (!(grf & grfSelectOneCell) && (grf & (grfInSingleRow|grpSelectEntireRow))))  ? CMDSTATE_UP : CMDSTATE_DISABLED;
                        break;
                    }

                case IDM_TRIED_SPLITCELL:
                    {
                        ULONG grf = IsSelectionInTable() == S_OK ? GetSelectionTypeInTable() : 0;
                        pCmd->cmdf = ((grf != -1) && (grf & grfSelectOneCell)) ? CMDSTATE_UP : CMDSTATE_DISABLED;
                        break;
                    }

                case IDM_TRIED_DELETECOLS:
                    {
                        ULONG grf = IsSelectionInTable() == S_OK ? GetSelectionTypeInTable() : 0;
                        pCmd->cmdf = ((grf != -1) && (grf & grfInSingleRow)) ? CMDSTATE_UP : CMDSTATE_DISABLED;
                        break;
                    }

                case IDM_TRIED_INSERTTABLE:
                    {
                        pCmd->cmdf = FEnableInsertTable() ? CMDSTATE_UP : CMDSTATE_DISABLED;
                        break;
                    }

                case IDM_TRIED_DOVERB:
                    {
                        if (SUCCEEDED(GetElement()) && m_pihtmlElement && SUCCEEDED(DoVerb(NULL, TRUE)))
                            pCmd->cmdf = CMDSTATE_UP;

                        break;
                    }

                case IDM_TRIED_ACTIVATEACTIVEXCONTROLS:
                case IDM_TRIED_ACTIVATEAPPLETS:
                case IDM_TRIED_ACTIVATEDTCS:
                case IDM_TRIED_BACKCOLOR:
                case IDM_TRIED_BLOCKFMT:
                case IDM_TRIED_BOLD:
                case IDM_TRIED_BROWSEMODE:
                case IDM_TRIED_COPY:
                case IDM_TRIED_CUT:
                case IDM_TRIED_DELETE:
                case IDM_TRIED_EDITMODE:
                case IDM_TRIED_FIND:
                case IDM_TRIED_FONT:
                case IDM_TRIED_FONTNAME:
                case IDM_TRIED_FONTSIZE:
                case IDM_TRIED_FORECOLOR:
                case IDM_TRIED_GETBLOCKFMTS:
                case IDM_TRIED_HYPERLINK:
                case IDM_TRIED_IMAGE:
                case IDM_TRIED_INDENT:
                case IDM_TRIED_ITALIC:
                case IDM_TRIED_JUSTIFYCENTER:
                case IDM_TRIED_JUSTIFYLEFT:
                case IDM_TRIED_JUSTIFYRIGHT:
                case IDM_TRIED_ORDERLIST:
                case IDM_TRIED_OUTDENT:
                case IDM_TRIED_PASTE:
                case IDM_TRIED_PRINT:
                case IDM_TRIED_REDO:
                case IDM_TRIED_REMOVEFORMAT:
                case IDM_TRIED_SELECTALL:
                case IDM_TRIED_SHOWBORDERS:
                case IDM_TRIED_SHOWDETAILS:
                case IDM_TRIED_UNDERLINE:
                case IDM_TRIED_UNDO:
                case IDM_TRIED_UNLINK:
                case IDM_TRIED_UNORDERLIST:
                    {
                        // We will return E_UNEXPECTED if Trident's command target is not available
                        hr = E_UNEXPECTED;

                        _ASSERTE(m_pCmdTgtTrident);
                        if (m_pCmdTgtTrident)
                        {
                            OLECMD olecmd;
                            
                            olecmd.cmdf = pCmd->cmdf;
                            if (SUCCEEDED(MapTriEditCommand(pCmd->cmdID, &olecmd.cmdID)))
                            {
                                hr = m_pCmdTgtTrident->QueryStatus(&CMDSETID_Forms3, 1, &olecmd, pCmdText);
                            }
                            pCmd->cmdf = olecmd.cmdf;
                        }
                        
                        if (FAILED(hr))
                            return hr;

                        // Trident returns NOTSUPPORTED sometimes when they really mean DISABLED, so we fix this up here.
                        if (pCmd->cmdf == CMDSTATE_NOTSUPPORTED)
                            pCmd->cmdf = CMDSTATE_DISABLED;

                        // Trident returns CMDSTATE_DISABLED for IDM_TRIED_GETBLOCKFMTS but this command should never be disabled
                        if (pCmd->cmdID == IDM_TRIED_GETBLOCKFMTS)
                            pCmd->cmdf = CMDSTATE_UP;

                        // Trident bug: Trident returns the wrong value for IDM_TRIED_SHOWBORDERS,
                        // IDM_TRIED_SHOWDETAILS and the IDM_TRIED_ACTIVATE* commands, so we fix
                        // them up here.  We don't have code for IDM_TRIED_ACTIVATE* since the logic
                        // of the Trident commands is actually reverse in these cases.

                        if (pCmd->cmdID == IDM_TRIED_SHOWBORDERS ||
                            pCmd->cmdID == IDM_TRIED_SHOWDETAILS)
                        {
                            if (pCmd->cmdf == CMDSTATE_UP)
                                pCmd->cmdf = CMDSTATE_DOWN;
                            else if (pCmd->cmdf == CMDSTATE_DOWN)
                                pCmd->cmdf = CMDSTATE_UP;
                        }

                        break;
                    }

                default:
                    {
                        pCmd->cmdf = CMDSTATE_NOTSUPPORTED;
                        break;
                    }
            } // switch
        } // for

        return S_OK;
    }
    else if (m_pCmdTgtTrident)
    {
        hr = m_pCmdTgtTrident->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
        if (hr != S_OK)
            return hr;

        // Loop through each command in the ary, fixing up the status of each.
        for (pCmd = prgCmds, c = cCmds; --c >= 0; pCmd++)
        {
            // Trident returns NOTSUPPORTED sometimes when they really mean DISABLED.
            if (pCmd->cmdf == CMDSTATE_NOTSUPPORTED)
                pCmd->cmdf = CMDSTATE_DISABLED;

            if (pguidCmdGroup && IsEqualGUID((const GUID&)*pguidCmdGroup, CMDSETID_Forms3))
            {
                // Trident returns CMDSTATE_DISABLED for IDM_GETBLOCKFMTS but this command should never be disabled
                if (pCmd->cmdID == IDM_GETBLOCKFMTS)
                    pCmd->cmdf = CMDSTATE_UP;

                // Trident bug: Trident returns the wrong value for IDM_SHOWZEROBORDER*,
                // IDM_SHOWALLTAGS and the IDM_NOACTIVATE* commands, so we fix
                // them up here.

                if (pCmd->cmdID == IDM_NOACTIVATENORMALOLECONTROLS ||
                    pCmd->cmdID == IDM_NOACTIVATEJAVAAPPLETS ||
                    pCmd->cmdID == IDM_NOACTIVATEDESIGNTIMECONTROLS ||
                    pCmd->cmdID == IDM_SHOWZEROBORDERATDESIGNTIME ||
                    pCmd->cmdID == IDM_SHOWALLTAGS)
                {
                    if (pCmd->cmdf == CMDSTATE_UP)
                        pCmd->cmdf = CMDSTATE_DOWN;
                    else if (pCmd->cmdf == CMDSTATE_DOWN)
                        pCmd->cmdf = CMDSTATE_UP;
                }
            }
        }

        return S_OK;
    }

    return E_UNEXPECTED;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::Exec
//
// Perform the given TriEdit or Trident command. Pass Trident commands on to
// Trident for execution. Return S_OK if all goes well or E_FAIL if not.
//

STDMETHODIMP CTriEditDocument::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
                                DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    if (pguidCmdGroup && IsEqualGUID((const GUID&)*pguidCmdGroup, GUID_TriEditCommandGroup) &&
        m_pUnkTrident)
    {
        HRESULT hr = GetElement();

        switch(nCmdID)
        {
            case IDM_TRIED_IS_1D_ELEMENT:   //[out,VT_BOOL]
                if (pvaOut && m_pihtmlElement &&
                    SUCCEEDED(VariantChangeType(pvaOut, pvaOut, 0, VT_BOOL)))
                {
                    hr = Is2DElement(m_pihtmlElement, (BOOL*)&pvaOut->boolVal);
                    _ASSERTE(SUCCEEDED(hr));
                    if (SUCCEEDED(hr))
                    {
                        pvaOut->boolVal = !pvaOut->boolVal;
                    }
                }
                break;
           case IDM_TRIED_IS_2D_ELEMENT:   //[out,VT_BOOL]
                if (pvaOut && m_pihtmlElement &&
                    SUCCEEDED(VariantChangeType(pvaOut, pvaOut, 0, VT_BOOL)))
                {
                    hr = Is2DElement(m_pihtmlElement, (BOOL*)&pvaOut->boolVal);
                    _ASSERTE(SUCCEEDED(hr));
                }
                break;
            case IDM_TRIED_NUDGE_ELEMENT:   //[in,VT_BYREF (VARIANT.byref=LPPOINT)]
                {
                    BOOL fLock = FALSE;
                    IsLocked(m_pihtmlElement, &fLock);
                    if (!pvaIn)
                        hr = E_FAIL;
                    else if (!fLock && VT_BYREF == pvaIn->vt && pvaIn->byref)
                    {
                        hr = NudgeElement(m_pihtmlElement, (LPPOINT)pvaIn->byref);
                        _ASSERTE(SUCCEEDED(hr));
                    }
                }
                break;
            case IDM_TRIED_SET_ALIGNMENT:   //[in,VT_BYREF (VARIANT.byref=LPPOINT)]
                if (!pvaIn)
                    hr = E_FAIL;
                else if (VT_BYREF == pvaIn->vt && pvaIn->byref)
                {
                    hr = SetAlignment((LPPOINT)pvaIn->byref);
                    _ASSERTE(SUCCEEDED(hr));
                }
                break;
            case IDM_TRIED_LOCK_ELEMENT:
                if (m_pihtmlElement)
                {
                    BOOL f2d=FALSE;
                    BOOL fLocked=TRUE;
                    if (SUCCEEDED(Is2DElement(m_pihtmlElement, &f2d)) && f2d &&
                            SUCCEEDED(IsLocked(m_pihtmlElement, &fLocked)))
                    {
                        hr = LockElement(m_pihtmlElement, !fLocked);
                        _ASSERTE(SUCCEEDED(hr));
                    }
                }
                break;
            case IDM_TRIED_SEND_TO_BACK:
                if (m_pihtmlElement)
                {
                    hr = AssignZIndex(m_pihtmlElement, SEND_TO_BACK);
                    _ASSERTE(SUCCEEDED(hr));
                }
                break;
            case IDM_TRIED_SEND_TO_FRONT:
                if (m_pihtmlElement)
                {
                    hr = AssignZIndex(m_pihtmlElement, SEND_TO_FRONT);
                    _ASSERTE(SUCCEEDED(hr));
                }
                break;
            case IDM_TRIED_SEND_BACKWARD:
                if (m_pihtmlElement)
                {
                    hr = AssignZIndex(m_pihtmlElement, SEND_BACKWARD);
                    _ASSERTE(SUCCEEDED(hr));
                }
                break;
            case IDM_TRIED_SEND_FORWARD:
                if (m_pihtmlElement)
                {
                    hr = AssignZIndex(m_pihtmlElement, SEND_FORWARD);
                    _ASSERTE(SUCCEEDED(hr));
                }
                break;
            case IDM_TRIED_SEND_BEHIND_1D:
                if (m_pihtmlElement)
                {
                    hr = AssignZIndex(m_pihtmlElement, SEND_BEHIND_1D);
                    _ASSERTE(SUCCEEDED(hr));
                }
                break;
            case IDM_TRIED_SEND_FRONT_1D:
                if (m_pihtmlElement)
                {
                    hr = AssignZIndex(m_pihtmlElement, SEND_FRONT_1D);
                    _ASSERTE(SUCCEEDED(hr));
                }
                break;
            case IDM_TRIED_CONSTRAIN:
                if (!pvaIn)
                    hr = E_FAIL;
                else if (SUCCEEDED(hr = VariantChangeType(pvaIn, pvaIn, 0, VT_BOOL)))
                {
                    hr = Constrain((BOOL)pvaIn->boolVal);
                }
                break;
            case IDM_TRIED_SET_2D_DROP_MODE:
                if (!pvaIn)
                    hr = E_FAIL;
                else if (SUCCEEDED(hr = VariantChangeType(pvaIn, pvaIn, 0, VT_BOOL)))
                {
                    m_f2dDropMode = pvaIn->boolVal;
                }
                break;
            case IDM_TRIED_INSERTROW:
                hr = InsertTableRow();
                break;
            case IDM_TRIED_INSERTCOL:
                hr = InsertTableCol();
                break;
            case IDM_TRIED_INSERTCELL:
                hr = InsertTableCell();
                break;
            case IDM_TRIED_DELETEROWS:
                hr = DeleteTableRows();
                break;
            case IDM_TRIED_DELETECOLS:
                hr = DeleteTableCols();
                break;
            case IDM_TRIED_DELETECELLS:
                hr = DeleteTableCells();
                break;
            case IDM_TRIED_MERGECELLS:
                hr = MergeTableCells();
                break;
            case IDM_TRIED_SPLITCELL:
                hr = SplitTableCell();
                break;
            case IDM_TRIED_INSERTTABLE:
                hr = InsertTable(pvaIn);
                break;
            case IDM_TRIED_DOVERB:
                if (m_pihtmlElement)
                    hr = DoVerb(pvaIn, FALSE);
                else
                    hr = E_FAIL;
                break;
            case IDM_TRIED_MAKE_ABSOLUTE:
                if (m_pihtmlElement)
                {
                    BOOL f2d = FALSE;
                    hr = Is2DElement(m_pihtmlElement, &f2d);

                    if (SUCCEEDED(hr))
                    {
                        BOOL f2dCapable=FALSE;
                        if ( f2d )
                        {
                            hr = Make1DElement(m_pihtmlElement);
                            _ASSERTE(SUCCEEDED(hr));
                        }
                        else if (SUCCEEDED(Is2DCapable(m_pihtmlElement, &f2dCapable)) && f2dCapable)
                        {
                            hr = Make2DElement(m_pihtmlElement);
                            _ASSERTE(SUCCEEDED(hr));
                        }

                    }

                }
                break;

            case IDM_TRIED_ACTIVATEACTIVEXCONTROLS:
            case IDM_TRIED_ACTIVATEAPPLETS:
            case IDM_TRIED_ACTIVATEDTCS:
            case IDM_TRIED_BACKCOLOR:
            case IDM_TRIED_BLOCKFMT:
            case IDM_TRIED_BOLD:
            case IDM_TRIED_BROWSEMODE:
            case IDM_TRIED_COPY:
            case IDM_TRIED_CUT:
            case IDM_TRIED_DELETE:
            case IDM_TRIED_EDITMODE:
            case IDM_TRIED_FIND:
            case IDM_TRIED_FONT:
            case IDM_TRIED_FONTNAME:
            case IDM_TRIED_FONTSIZE:
            case IDM_TRIED_FORECOLOR:
            case IDM_TRIED_GETBLOCKFMTS:
            case IDM_TRIED_HYPERLINK:
            case IDM_TRIED_IMAGE:
            case IDM_TRIED_INDENT:
            case IDM_TRIED_ITALIC:
            case IDM_TRIED_JUSTIFYCENTER:
            case IDM_TRIED_JUSTIFYLEFT:
            case IDM_TRIED_JUSTIFYRIGHT:
            case IDM_TRIED_ORDERLIST:
            case IDM_TRIED_OUTDENT:
            case IDM_TRIED_PASTE:
            case IDM_TRIED_PRINT:
            case IDM_TRIED_REDO:
            case IDM_TRIED_REMOVEFORMAT:
            case IDM_TRIED_SELECTALL:
            case IDM_TRIED_SHOWBORDERS:
            case IDM_TRIED_SHOWDETAILS:
            case IDM_TRIED_UNDERLINE:
            case IDM_TRIED_UNDO:
            case IDM_TRIED_UNLINK:
            case IDM_TRIED_UNORDERLIST:
                {
                    ULONG cmdTrident;
                    VARIANT varColor;

                    // We will return E_FAIL if Trident's command target is not available
                    hr = E_FAIL;

                    _ASSERTE(m_pCmdTgtTrident);
                    if (m_pCmdTgtTrident && (SUCCEEDED(MapTriEditCommand(nCmdID, &cmdTrident))))
                    {
                        if (nCmdID == IDM_TRIED_ACTIVATEACTIVEXCONTROLS ||
                            nCmdID == IDM_TRIED_ACTIVATEAPPLETS ||
                            nCmdID == IDM_TRIED_ACTIVATEDTCS)
                        {
                            if (pvaIn && pvaIn->vt == VT_BOOL)
                                pvaIn->boolVal = !pvaIn->boolVal;
                        }
                       
                        // Trident bug: When you exec the forecolor, fontname or fontsize command, they also change the backcolor,
                        // so we apply a workaround here.  The workaround is to save the old backcolor and exec it later.
                        if (pvaIn && (nCmdID == IDM_TRIED_FORECOLOR || nCmdID == IDM_TRIED_FONTNAME || nCmdID == IDM_TRIED_FONTSIZE))
                        {
                            HRESULT hrT;
 
                            VariantInit(&varColor);
                            V_VT(&varColor) = VT_I4;

                            hrT = m_pCmdTgtTrident->Exec(&CMDSETID_Forms3, IDM_BACKCOLOR, OLECMDEXECOPT_DONTPROMPTUSER, NULL, &varColor);
                            _ASSERTE(SUCCEEDED(hrT));
                        }

                        // Trident bug: When you exec the block format command with "Normal", they don't remove OL and UL tags
                        if (pvaIn && nCmdID == IDM_TRIED_BLOCKFMT && pvaIn->vt == VT_BSTR && (_wcsicmp(pvaIn->bstrVal, L"Normal") == 0))
                        {
                            OLECMD olecmd;

                            olecmd.cmdID = IDM_ORDERLIST;
                            olecmd.cmdf = CMDSTATE_NOTSUPPORTED;
                            if (S_OK == m_pCmdTgtTrident->QueryStatus(&CMDSETID_Forms3, 1, &olecmd, NULL) && olecmd.cmdf == CMDSTATE_DOWN)
                                m_pCmdTgtTrident->Exec(&CMDSETID_Forms3, IDM_ORDERLIST, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
                            
                            olecmd.cmdID = IDM_UNORDERLIST;
                            olecmd.cmdf = CMDSTATE_NOTSUPPORTED;
                            if (S_OK == m_pCmdTgtTrident->QueryStatus(&CMDSETID_Forms3, 1, &olecmd, NULL) && olecmd.cmdf == CMDSTATE_DOWN)
                                m_pCmdTgtTrident->Exec(&CMDSETID_Forms3, IDM_UNORDERLIST, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
                        }

                        hr = m_pCmdTgtTrident->Exec(&CMDSETID_Forms3, cmdTrident, nCmdExecOpt, pvaIn, pvaOut);

                        if (pvaIn && (nCmdID == IDM_TRIED_FORECOLOR || nCmdID == IDM_TRIED_FONTNAME || nCmdID == IDM_TRIED_FONTSIZE))
                        {
                            HRESULT hrT;

                            hrT = m_pCmdTgtTrident->Exec(&CMDSETID_Forms3, IDM_BACKCOLOR, OLECMDEXECOPT_DONTPROMPTUSER, &varColor, NULL);
                            _ASSERTE(SUCCEEDED(hrT));
                        }
                        else if (nCmdID == IDM_TRIED_SHOWDETAILS && pvaIn && pvaIn->vt == VT_BOOL)
                        {
                            SetUpGlyphTable(pvaIn->boolVal);
                        }

                        // Trident bug: They enable the justify commands but not actually support them.
                        // We workaround this by returning S_OK for these no matter what Trident returns.
                        if (nCmdID == IDM_TRIED_JUSTIFYLEFT || nCmdID == IDM_TRIED_JUSTIFYCENTER || nCmdID == IDM_TRIED_JUSTIFYRIGHT)
                            hr = S_OK;
                    }

                    break;
                }

            default:
                hr = E_FAIL;
                break;
        }

        if (pvaIn)
            VariantClear(pvaIn);

        // We shouldn't return any unexpected error codes here, so return E_FAIL
        if (FAILED(hr))
            hr = E_FAIL;

        return hr;
    }
    else if (m_pCmdTgtTrident)
    {
        HRESULT hr;
        BOOL fTridentCmdSet;
        VARIANT varColor;

        fTridentCmdSet = pguidCmdGroup && IsEqualGUID((const GUID&)*pguidCmdGroup, CMDSETID_Forms3);

#ifdef NEEDED
        if (fTridentCmdSet)
        {
            if (nCmdID == IDM_PARSECOMPLETE)
                OnObjectModelComplete();
            return S_OK;
        }
#endif //NEEDED

        // Trident bug: When you exec the forecolor, fontname or fontsize command, they also change the backcolor,
        // so we apply a workaround here.  The workaround is to save the old backcolor and exec it later.
        if (pvaIn && fTridentCmdSet && (nCmdID == IDM_FORECOLOR || nCmdID == IDM_FONTNAME || nCmdID == IDM_FONTSIZE))
        {
            HRESULT hrT;

            VariantInit(&varColor);
            V_VT(&varColor) = VT_I4;

            hrT = m_pCmdTgtTrident->Exec(pguidCmdGroup, IDM_BACKCOLOR, OLECMDEXECOPT_DONTPROMPTUSER, NULL, &varColor);
            _ASSERTE(SUCCEEDED(hrT));
        }

        // Trident bug: When you exec the block format command with "Normal", they don't remove OL and UL tags
        if (pvaIn && fTridentCmdSet && nCmdID == IDM_BLOCKFMT && pvaIn->vt == VT_BSTR && (_wcsicmp(pvaIn->bstrVal, L"Normal") == 0))
        {
            OLECMD olecmd;

            olecmd.cmdID = IDM_ORDERLIST;
            olecmd.cmdf = CMDSTATE_NOTSUPPORTED;
            if (S_OK == m_pCmdTgtTrident->QueryStatus(&CMDSETID_Forms3, 1, &olecmd, NULL) && olecmd.cmdf == CMDSTATE_DOWN)
                m_pCmdTgtTrident->Exec(&CMDSETID_Forms3, IDM_ORDERLIST, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
            
            olecmd.cmdID = IDM_UNORDERLIST;
            olecmd.cmdf = CMDSTATE_NOTSUPPORTED;
            if (S_OK == m_pCmdTgtTrident->QueryStatus(&CMDSETID_Forms3, 1, &olecmd, NULL) && olecmd.cmdf == CMDSTATE_DOWN)
                m_pCmdTgtTrident->Exec(&CMDSETID_Forms3, IDM_UNORDERLIST, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
        }

        hr = m_pCmdTgtTrident->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        if (pvaIn && fTridentCmdSet && (nCmdID == IDM_FORECOLOR || nCmdID == IDM_FONTNAME || nCmdID == IDM_FONTSIZE))
        {
            HRESULT hrT;

            hrT = m_pCmdTgtTrident->Exec(pguidCmdGroup, IDM_BACKCOLOR, OLECMDEXECOPT_DONTPROMPTUSER, &varColor, NULL);
            _ASSERTE(SUCCEEDED(hrT));
        }
        else if ((nCmdID == IDM_SHOWALLTAGS || nCmdID == IDM_SHOWMISCTAGS) && pvaIn && pvaIn->vt == VT_BOOL)
        {
            SetUpGlyphTable(pvaIn->boolVal);
        }

        // Trident bug: They enable the justify commands but not actually support them.
        // We workaround this by returning S_OK for these no matter what Trident returns.
        if (fTridentCmdSet && (nCmdID == IDM_JUSTIFYLEFT || nCmdID == IDM_JUSTIFYCENTER || nCmdID == IDM_JUSTIFYRIGHT))
            hr = S_OK;

        return hr;
    }

    return E_UNEXPECTED;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::Is2DElement
//
// Test the given HTML element to ascertain if it is 2D positioned or not.
// Return:
//      S_OK and *pf2D = TRUE if the element is 2D positioned.
//      S_OK and *pf2D = FALSE if the element is not 2D positioned.
//

HRESULT CTriEditDocument::Is2DElement(IHTMLElement* pihtmlElement, BOOL* pf2D)
{
    IHTMLStyle* pihtmlStyle = NULL;
    BSTR bstrPosition = NULL;
    BOOL f2DCapable;
    _ASSERTE(pihtmlElement);
    _ASSERTE(pf2D);

    *pf2D = FALSE;

    if (SUCCEEDED(Is2DCapable(pihtmlElement, &f2DCapable)))
    {
        if (f2DCapable && SUCCEEDED(pihtmlElement->get_style(&pihtmlStyle)))
        {
            _ASSERTE(pihtmlStyle);
            if (SUCCEEDED(pihtmlStyle->get_position(&bstrPosition)))
            {
                if (bstrPosition)
                {
                    *pf2D = (_wcsicmp(bstrPosition, L"absolute") == 0);
                    SysFreeString(bstrPosition);
                }
            SAFERELEASE(pihtmlStyle);
            }
        }
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEDitDocument::NudgeElement
//
// Move the given HTML element (which must be 2D positioned) as indicated
// by pptNudge, further adjusted by the grid spacing in m_ptAlign. Returns
// S_OK if all goes well; E_UNEXPECTED otherwise.
//

HRESULT CTriEditDocument::NudgeElement(IHTMLElement* pihtmlElement, LPPOINT pptNudge)
{
    HRESULT hr = E_UNEXPECTED;
    IHTMLStyle* pihtmlStyle = NULL;
    long x, y;

    _ASSERTE(pihtmlElement);
    _ASSERTE(pptNudge);
    if (pihtmlElement)
    {
        if (SUCCEEDED(pihtmlElement->get_style(&pihtmlStyle)) &&
            pihtmlStyle &&
            SUCCEEDED(pihtmlStyle->get_pixelTop(&y)) &&
            SUCCEEDED(pihtmlStyle->get_pixelLeft(&x)))
        {
            if (x == 0 || y == 0)
            {
                IHTMLElement *pihtmlElementParent = NULL;
                RECT rcElement, rcParent;

                if (SUCCEEDED(pihtmlElement->get_offsetParent(&pihtmlElementParent))
                    && pihtmlElementParent)
                {
                    if (SUCCEEDED(GetElementPosition(pihtmlElement, &rcElement)))
                    {
                        ::SetRect(&rcParent, 0, 0, 0, 0);

                        if (SUCCEEDED(GetElementPosition(pihtmlElementParent, &rcParent)))
                        {
                            x = rcElement.left - rcParent.left;
                            y = rcElement.top - rcParent.top;
                        }
                    }
                    pihtmlElementParent->Release();
                }
            }

            x += pptNudge->x;
            y += pptNudge->y;
            if (pptNudge->x != 0)
            {
                if (x >= 0)
                    x -= (x % m_ptAlign.x);
                else
                    x -= (((x % m_ptAlign.x) ? m_ptAlign.x : 0) + (x % m_ptAlign.x));
            }
            if (pptNudge->y != 0)
            {
                if (y >= 0)
                    y -= (y % m_ptAlign.y);
                else
                    y -= (((y % m_ptAlign.y) ? m_ptAlign.y : 0) + (y % m_ptAlign.y));
            }
            pihtmlStyle->put_pixelTop(y);
            pihtmlStyle->put_pixelLeft(x);
            return S_OK;
        }
    }
    SAFERELEASE(pihtmlStyle);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::SetAlignment
//
// Set the TriEdit alignment values as indicated. Return S_OK if all goes
// well; or E_POINTER if a bad pointer is supplied.
//

HRESULT CTriEditDocument::SetAlignment(LPPOINT pptAlign)
{
    _ASSERTE(pptAlign);
    if (pptAlign)
    {
        m_ptAlign.x = max(pptAlign->x, 1);
        m_ptAlign.y = max(pptAlign->y, 1);
        return S_OK;
    }
    return E_POINTER;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::LockElement
//
// Set or clear the TriEdit design-time locking flag (an expando attribute) as
// indicated by fLock. Return S_OK if all goes well; E_FAIL if not. Note that
// setting the locking flag also sets the top and left attributes if they
// were not already set.
//

HRESULT CTriEditDocument::LockElement(IHTMLElement* pihtmlElement, BOOL fLock)
{
    IHTMLStyle* pihtmlStyle=NULL;
    HRESULT hr = E_FAIL;
    VARIANT var;
    VARIANT_BOOL fSuccess = FALSE;

    if (pihtmlElement)
    {
        hr = pihtmlElement->get_style(&pihtmlStyle);
        _ASSERTE(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            _ASSERTE(pihtmlStyle);
            if (pihtmlStyle)
            {
                if(!fLock)
                {
                    hr = pihtmlStyle->removeAttribute(DESIGN_TIME_LOCK, 0, &fSuccess);
                    _ASSERTE(fSuccess);
                }
                else
                {
                    // Trident doesn't persist the Design_Time_Lock attribute
                    // if left, top, width and height properties are not present as part of
                    // the elements style attribute. Hence as a part of locking the element 
                    // we also assign the top and left styles only if they don't exist.

                    LONG lTop, lLeft;

                    pihtmlStyle->get_pixelTop(&lTop);
                    pihtmlStyle->get_pixelLeft(&lLeft);

                    if (lTop == 0 || lLeft == 0)
                    {
                        IHTMLElement *pihtmlElementParent = NULL;

                        if (SUCCEEDED(pihtmlElement->get_offsetParent(&pihtmlElementParent))
                            && pihtmlElementParent)
                        {
                            if (SUCCEEDED(GetElementPosition(pihtmlElement, &m_rcElement)))
                            {
                                RECT rcParent;
                                ::SetRect(&rcParent, 0, 0, 0, 0);
    
                                if (SUCCEEDED(GetElementPosition(pihtmlElementParent, &rcParent)))
                                {
                                    m_rcElement.left   = m_rcElement.left - rcParent.left;
                                    m_rcElement.top    = m_rcElement.top  - rcParent.top;
                                    pihtmlStyle->put_pixelTop(m_rcElement.top);
                                    pihtmlStyle->put_pixelLeft(m_rcElement.left);
                                }
                            }
                            pihtmlElementParent->Release();
                        }
                    }

                    VariantInit(&var);
                    var.vt = VT_BSTR;
                    var.bstrVal = SysAllocString(L"True");
                    hr = pihtmlStyle->setAttribute(DESIGN_TIME_LOCK, var, 0);
                    hr = SUCCEEDED(hr) ? S_OK:E_FAIL;
                }
                pihtmlStyle->Release();
            }
        }

        if (SUCCEEDED(hr))
        {
            RECT rcElement;

            hr = GetElementPosition(pihtmlElement, &rcElement);
            _ASSERTE(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
            {
                 InflateRect(&rcElement, ELEMENT_GRAB_SIZE, ELEMENT_GRAB_SIZE);
                 if( SUCCEEDED(hr = GetTridentWindow()))
                 {
                     _ASSERTE(m_hwndTrident);
                     InvalidateRect(m_hwndTrident,&rcElement, FALSE);
                 }
            }

            // Trident doesn't set itself to be dirty, so force the dirty state.
            VariantInit(&var);
            var.vt = VT_BOOL;
            var.boolVal = TRUE; 
            if (m_pCmdTgtTrident)           
                m_pCmdTgtTrident->Exec(&CMDSETID_Forms3, IDM_SETDIRTY, 0, &var, NULL);
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::IsLocked
//
// Test the given HTML element to ascertain if it is design-time locked or not.
// Return:
//      S_OK and *pfLocked = TRUE if the element is design-time locked.
//      S_OK and *pfLocked = FALSE if the element is not design-time locked.
//

HRESULT CTriEditDocument::IsLocked(IHTMLElement* pihtmlElement, BOOL* pfLocked)
{
    IHTMLStyle* pihtmlStyle=NULL;
    BSTR bstrAttributeName = NULL;
    HRESULT hr = E_FAIL;
    VARIANT var;

    VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = NULL;

    if (pihtmlElement)
    {
        hr = pihtmlElement->get_style(&pihtmlStyle);
        _ASSERTE(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            _ASSERTE(pihtmlStyle);
            if (pihtmlStyle)
            {
                bstrAttributeName = SysAllocString(DESIGN_TIME_LOCK);

                if (bstrAttributeName)
                {
                    hr = pihtmlStyle->getAttribute(bstrAttributeName, 0, &var);
                    _ASSERTE(SUCCEEDED(hr));
                    if (var.bstrVal == NULL)
                        *pfLocked = FALSE;
                    else
                        *pfLocked = TRUE;
                    SysFreeString(bstrAttributeName);
                }
                pihtmlStyle->Release();
            }
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//
// CTriEditDocument::Make1DElement
//
// Set the given HTML element to layout in the flow. As a side effect this
// also removes any design-time lock on the element. Return S_OK if all goes
// well; E_UNEXPECTED otherwise.
//

HRESULT CTriEditDocument::Make1DElement(IHTMLElement* pihtmlElement)
{
    IHTMLStyle* pihtmlStyle=NULL;
    VARIANT_BOOL fSuccess = FALSE;
    VARIANT var;
    HRESULT hr;

    if (pihtmlElement)
    {
        pihtmlElement->get_style(&pihtmlStyle);
        _ASSERTE(pihtmlStyle);
        if (pihtmlStyle)
        {
            VariantInit(&var);
            var.vt = VT_I4;
            var.lVal = 0; 
            hr = pihtmlStyle->put_zIndex(var);
            _ASSERTE(SUCCEEDED(hr));

            pihtmlStyle->removeAttribute(DESIGN_TIME_LOCK, 0, &fSuccess);
            pihtmlStyle->removeAttribute(L"position", 0, &fSuccess);
            pihtmlStyle->Release();
        }
    }
    
    return (fSuccess? S_OK: E_UNEXPECTED);
}

///////////////////////////////////////////////////////////////////////////////
//
//
// CTriEditDocument::Make2DElement
//
// Set the given HTML element to be positioned. Return S_OK if all goes
// well; E_FAIL otherwise.
//

HRESULT CTriEditDocument::Make2DElement(IHTMLElement* pihtmlElement, POINT *ppt)
{

    IHTMLElement* pihtmlElementParent = NULL;
    IHTMLElementCollection* pihtmlCollection = NULL;
    IHTMLElement* pihtmlElementNew = NULL;
    IHTMLStyle* pihtmlElementStyle = NULL;
    VARIANT var;
    LONG lSourceIndex;
    HRESULT hr = E_FAIL;
    BSTR bstrOuterHtml = NULL;

    _ASSERTE(pihtmlElement);

    if(!pihtmlElement)
    {
        return E_FAIL;
    }
    
    hr = pihtmlElement->get_style(&pihtmlElementStyle);
    _ASSERTE(SUCCEEDED(hr) && pihtmlElementStyle);

    if (FAILED(hr) || !pihtmlElementStyle)
    {
        return E_FAIL;
    }

    // The reason to save the source index here is that once we call put_outerHTML
    // the element is lost, we later use the source index to get back the element from the collection.  
    // Note that the source index remains the same after put_outerHTML.
    hr = pihtmlElement->get_sourceIndex(&lSourceIndex); 
    _ASSERTE(SUCCEEDED(hr) && (lSourceIndex != -1));
    
    if (lSourceIndex == -1 || FAILED(hr))
    {
        return E_FAIL;
    }

    hr = pihtmlElement->get_offsetParent(&pihtmlElementParent);
    _ASSERTE(SUCCEEDED(hr) && pihtmlElementParent);

    if (SUCCEEDED(hr) && pihtmlElementParent)
    {
        VariantInit(&var);
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString(L"absolute");
        hr = pihtmlElementStyle->setAttribute(L"position", var, 1);

        if (var.bstrVal)
            SysFreeString(var.bstrVal);

        _ASSERTE(SUCCEEDED(hr));

        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(hr = GetElementPosition(pihtmlElement, &m_rcElement)))
            {
                IHTMLTable* pihtmlTable = NULL;
                IHTMLElement* pihtmlElementTemp = NULL, *pihtmlElementPrev = NULL;
                RECT rcParent;
                BOOL f2d = FALSE;
                BOOL fIsIE5AndBeyond = IsIE5OrBetterInstalled();

                ::SetRect(&rcParent, 0, 0, 0, 0);

                pihtmlElementTemp = pihtmlElementParent;
                pihtmlElementTemp->AddRef();

                // Handle tables specially since the offset parent may have been the TD or the TR
                while (pihtmlElementTemp)
                {
                    if (SUCCEEDED(pihtmlElementTemp->QueryInterface(IID_IHTMLTable, (void **)&pihtmlTable)) && pihtmlTable)
                        break;

                    pihtmlElementPrev = pihtmlElementTemp;
                    pihtmlElementPrev->get_offsetParent(&pihtmlElementTemp);
                    SAFERELEASE(pihtmlElementPrev);
                }

                // If parent is a 2d element, we need to offset its top and left
                if (pihtmlElementTemp && SUCCEEDED(Is2DElement(pihtmlElementTemp, &f2d)) && f2d)
                {
                    GetElementPosition(pihtmlElementTemp, &rcParent);
                }
                else if (SUCCEEDED(Is2DElement(pihtmlElementParent, &f2d)) && f2d)
                {
                    GetElementPosition(pihtmlElementParent, &rcParent);
                }

                SAFERELEASE(pihtmlTable);
                SAFERELEASE(pihtmlElementTemp);
                SAFERELEASE(pihtmlElementPrev);

                m_rcElement.left   = (ppt ? ppt->x : m_rcElement.left) - rcParent.left;
                m_rcElement.top    = (ppt ? ppt->y : m_rcElement.top) - rcParent.top;

                // We need to call get_outerHTML and put_outerHTML to work around a Trident bug
                // We should not really have to call these here, but the element doesn't get
                // updated unless we do this.
                if (fIsIE5AndBeyond || SUCCEEDED(hr = pihtmlElement->get_outerHTML(&bstrOuterHtml)))
                {
                    if (fIsIE5AndBeyond || SUCCEEDED(hr = pihtmlElement->put_outerHTML(bstrOuterHtml)))
                    {
                        hr = GetAllCollection(&pihtmlCollection);
                        _ASSERTE(SUCCEEDED(hr));
                        _ASSERTE(pihtmlCollection);

                        if (SUCCEEDED(hr) && pihtmlCollection)
                        {
                            hr = GetCollectionElement(pihtmlCollection, lSourceIndex, &pihtmlElementNew);
                            _ASSERTE(SUCCEEDED(hr));
                            _ASSERTE(pihtmlElementNew);

                            if (SUCCEEDED(hr) && pihtmlElementNew)
                            {
                                hr = SelectElement(pihtmlElementNew, pihtmlElementParent);

                                GetElement(); // to update m_pihtmlElement and friends after the above SelectElement

                                if (SUCCEEDED(hr))
                                {
                                    hr = AssignZIndex(pihtmlElementNew,  MADE_ABSOLUTE);
                                    _ASSERTE(SUCCEEDED(hr));

                                    if (SUCCEEDED(hr))
                                    {
                                        SAFERELEASE(pihtmlElementStyle);
                                        if (SUCCEEDED(hr = pihtmlElementNew->get_style(&pihtmlElementStyle)))
                                        {
                                            pihtmlElementStyle->put_pixelLeft(m_rcElement.left);
                                            pihtmlElementStyle->put_pixelTop(m_rcElement.top);
                                            VariantInit(&var);
                                            var.vt = VT_BOOL;
                                            var.boolVal = FALSE;
                                            pihtmlElementNew->scrollIntoView(var);
                                         }

                                    }

                                }

                            }

                        }

                    }

                }

            }

        }

    }
               
    if (bstrOuterHtml)                  
        SysFreeString(bstrOuterHtml);

    SAFERELEASE(pihtmlElementParent);
    SAFERELEASE(pihtmlElementStyle);
    SAFERELEASE(pihtmlElementNew);
    SAFERELEASE(pihtmlCollection);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::Constrain
// 
// Set the TriEdit constraint flag as indicated by fConstrain. Also, reset
// the constraint direction to CONSTRAIN_NONE. Return S_OK.

HRESULT CTriEditDocument::Constrain(BOOL fConstrain)
{
    m_fConstrain = (fConstrain) ? TRUE:FALSE;
    m_eDirection = CONSTRAIN_NONE;
    return S_OK;
}

typedef struct SELCELLINFO
   {
       LONG cCellIndex; // cell index in a row
       LONG cRowIndex; // which row is this cell in
       CComPtr<IDispatch> srpCell; // cell element
       CComPtr<IDispatch> srpRow; // row element
       CComPtr<IDispatch> srpTable;
   } SELCELLINFO;


///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::GetTableRowElementAndTableFromCell
//
// Given the IDispatch pointer to an element within a table, return the
// row index in *pindexRow (if pindexRow is not NULL) and/or the 
// actual row element in *psrpRow (if psrpRow is not NULL) of the
// element within the table. If psrpTable is not NULL, return the
// table containing the element therein. Return S_OK if all goes well,
// or E_FAIL if something goes wrong.
//

HRESULT CTriEditDocument::GetTableRowElementAndTableFromCell(IDispatch *srpCell, LONG *pindexRow , IDispatch **psrpRow, IDispatch **psrpTable)
{
   CComPtr<IDispatch>    srpParent,srpElement;
   HRESULT hr = E_FAIL;
   CComBSTR bstrTag;

    _ASSERTE(srpCell != NULL);

    if (pindexRow == NULL && psrpRow == NULL)
        goto Fail;

    srpParent = srpCell;

    while (srpParent != NULL)
    {
        srpElement.Release();
        if (FAILED(hr = GetDispatchProperty(srpParent, L"parentElement", VT_DISPATCH, (void**)&srpElement)))
            goto Fail;

        if (srpElement == NULL)
            {
            hr = E_FAIL;
            goto Fail;
            }

        bstrTag.Empty();
        if (FAILED(hr = GetDispatchProperty(srpElement, L"tagName", VT_BSTR, &bstrTag)))
            goto Fail;

        if (lstrcmpi(_T("TR"), OLE2T(bstrTag)) == 0)
        {
            if (psrpRow != NULL)
            {
                *psrpRow = srpElement;
                (*psrpRow)->AddRef();
            }

            if (pindexRow != NULL)
            {
                if (FAILED(hr = GetDispatchProperty(srpElement, L"rowIndex", VT_I4, pindexRow)))
                    goto Fail;
            }
            break;
        }
        srpParent = srpElement;
    }

   if (psrpTable != NULL)
   {
       srpParent = srpElement;
       while (srpParent != NULL)
       {
            srpElement.Release();
            if (FAILED(hr = GetDispatchProperty(srpParent, L"parentElement", VT_DISPATCH, (void**)&srpElement)))
                goto Fail;

            if (srpElement == NULL)
                {
                hr = E_FAIL;
                goto Fail;
                }

            bstrTag.Empty();
            if (FAILED(hr = GetDispatchProperty(srpElement, L"tagName", VT_BSTR, &bstrTag)))
                goto Fail;

            if (lstrcmpi(_T("TABLE"), OLE2T(bstrTag)) == 0)
            {
                if (psrpTable != NULL)
                {
                    *psrpTable = srpElement;
                    (*psrpTable)->AddRef();
                }
                break;
            }
            srpParent = srpElement;
        }
   }

Fail:

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::FEnableInsertTable
//
// Return TRUE if the Trident selection is within a table and if the selection
// type and location will allow elements to be inserted within the table. 
// Return FALSE otherwise.
//

BOOL CTriEditDocument::FEnableInsertTable(void)
{
    BOOL fRet = FALSE;
    CComPtr<IDispatch>    srpRange,srpParent,srpElement;
    CComPtr<IHTMLSelectionObject>    srpSel;
    CComPtr<IHTMLDocument2>    srpiHTMLDoc;
    CComBSTR    bstr;
    CComBSTR    bstrTag;

    if (FAILED(m_pUnkTrident->QueryInterface(IID_IHTMLDocument2, (void**)&srpiHTMLDoc)))
        goto Fail;

    if (FAILED(srpiHTMLDoc->get_selection(&srpSel)))
        goto Fail;

    if (FAILED(GetDispatchProperty(srpSel, L"type", VT_BSTR, &bstr)))
        goto Fail;

    if (lstrcmpi(_T("CONTROL"), OLE2T(bstr)) == 0)
    {
        return FALSE;
    }

    if (FAILED(CallDispatchMethod(srpSel, L"createRange", VTS_DISPATCH_RETURN, (void**)&srpRange)))
        goto Fail;

    if (srpRange == NULL)
        goto Fail;
        
    srpParent = srpRange;

    while (srpParent != NULL)
    {
        srpElement.Release();
        if (FAILED(GetDispatchProperty(srpParent, L"parentElement", VT_DISPATCH, (void**)&srpElement)))
            goto Fail;

        if (srpElement == NULL)
            break;

        bstrTag.Empty();
        if (FAILED(GetDispatchProperty(srpElement, L"tagName", VT_BSTR, &bstrTag)))
            goto Fail;

        if (lstrcmpi(_T("INPUT"), OLE2T(bstrTag)) == 0)
        {
           return FALSE;
        }
        srpParent = srpElement;
    }

    // if the selection is inside a table, make sure only one  cell is selected
    if (IsSelectionInTable() == S_OK)
    {
        UINT grf = GetSelectionTypeInTable();
        if (grf != -1 && !(grf & grfSelectOneCell)) 
            return FALSE;
    }
    
    fRet = TRUE;

Fail:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::IsSelectionInTable
//
// Return S_OK if the Trident selection is within a table. Return
// E_FAIL otherwise.
//

HRESULT CTriEditDocument::IsSelectionInTable(IDispatch **ppTable)
{
    HRESULT    hr=0;
    CComPtr<IHTMLSelectionObject>    srpSel;
    CComPtr<IDispatch>    srpRange,srpParent,srpElement;
    CComPtr<IHTMLDocument2>    srpiHTMLDoc;
    CComBSTR    bstrTag;
    BOOL  fTable= FALSE;

    if (FAILED(hr = m_pUnkTrident->QueryInterface(IID_IHTMLDocument2, (void**)&srpiHTMLDoc)))
        goto Fail;

    if (FAILED(hr = srpiHTMLDoc->get_selection(&srpSel)))
        goto Fail;

    if (FAILED(hr = CallDispatchMethod(srpSel, L"createRange", VTS_DISPATCH_RETURN, (void**)&srpRange)))
        goto Fail;

    srpParent = srpRange;
    
    while (srpParent != NULL)
    {
        srpElement.Release();
        if (FAILED(hr = GetDispatchProperty(srpParent, L"parentElement", VT_DISPATCH, (void**)&srpElement)))
            goto Fail;

        if (srpElement == NULL)
            break;

        bstrTag.Empty();
        if (FAILED(hr = GetDispatchProperty(srpElement, L"tagName", VT_BSTR, &bstrTag)))
            goto Fail;

        if (lstrcmpi(_T("TABLE"), OLE2T(bstrTag)) == 0)
        {
            if (ppTable != NULL)
            {
                *ppTable = srpElement;
                (*ppTable)->AddRef();
            }
            fTable = TRUE;
            break;
        }
        else if (lstrcmpi(_T("CAPTION"), OLE2T(bstrTag)) == 0)
        {
            fTable = FALSE;
            break;
        }

        srpParent = srpElement;
    }

Fail:

    return fTable ? S_OK : E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::FillInSelectionCellsInfo
//
// Fill *pSelStart with the information concerning the table cell containing
// the beginning of the Trident selection and *pSelSle with the information
// on the table cell at the end of the selection. Return S_OK if all goes well,
// or E_FAIL otherwise.

HRESULT   CTriEditDocument::FillInSelectionCellsInfo(struct SELCELLINFO * pselStart, struct SELCELLINFO *pselEnd)
{
    CComPtr<IHTMLDocument2>  srpiHTMLDoc;
    CComPtr<IHTMLSelectionObject>   srpSel;
    CComPtr<IHTMLTxtRange>      srpRange[2];
    CComPtr<IDispatch>    srpParent;
    CComBSTR       bstrText, bstrTag;;
    LONG cReturn=0;
    HRESULT i=0, hr=0;
    LONG cCharSelected=0;
    WCHAR *pData = NULL;
    BOOL fContain = FALSE;

    if (FAILED(hr = IsSelectionInTable()))
        goto Fail;

    if (FAILED(hr = m_pUnkTrident->QueryInterface(IID_IHTMLDocument2, (void**)&srpiHTMLDoc)))
        goto Fail;

    if (FAILED(hr = srpiHTMLDoc->get_selection(&srpSel)))
        goto Fail;

    for (i=0; i<2 ; i++)
    {
        if (FAILED(hr = CallDispatchMethod(srpSel, L"createRange", VTS_DISPATCH_RETURN, (void**)&srpRange[i])))
             goto Fail;
    }

    bstrText.Empty();
    hr = srpRange[0]->get_text(&bstrText);
    if (FAILED(hr))
    goto Fail;

    cCharSelected = bstrText ? ocslen(bstrText) : 0;
    pData = (WCHAR *) bstrText;

    // VID98 bug 3117: trident use '0x0D' to mark column/row and this char is ignored when
    // move range so we need to deduct these characters
    while (pData != NULL && *pData !='\0')
    {
        if (*pData == 0x0D)
            cCharSelected--;
        pData++;
    }

    if (pselStart != NULL)
    {
        hr = srpRange[0]->collapse(TRUE);
        if (FAILED(hr))
        goto Fail;

        srpParent = srpRange[0];
        while (srpParent != NULL)
        {
            pselStart->srpCell.Release();
            if (FAILED(hr = GetDispatchProperty(srpParent, L"parentElement", VT_DISPATCH, (void**)&pselStart->srpCell)))
                goto Fail;

            if (pselStart->srpCell == NULL)
                {
                hr = E_FAIL;
                goto Fail;
                }

            bstrTag.Empty();
            if (FAILED(hr = GetDispatchProperty(pselStart->srpCell, L"tagName", VT_BSTR, &bstrTag)))
                goto Fail;

            if (lstrcmpi(_T("TD"), OLE2T(bstrTag)) == 0 || lstrcmpi(_T("TH"), OLE2T(bstrTag)) == 0)
            {
                break;
            }
          
            srpParent = pselStart->srpCell;
        }

        _ASSERTE(pselStart->srpCell != NULL);
        if (FAILED(hr = GetDispatchProperty(pselStart->srpCell, L"cellIndex", VT_I4, &pselStart->cCellIndex)))
            goto Fail;

        pselStart->srpRow.Release();
        if (FAILED(hr = GetTableRowElementAndTableFromCell(pselStart->srpCell, &pselStart->cRowIndex, &pselStart->srpRow, &pselStart->srpTable)))
            goto Fail;
    }

    if (pselEnd != NULL)
    {
        hr = srpRange[1]->collapse(FALSE);
        if (FAILED(hr))
            goto Fail;

        if (cCharSelected != 0)
        {
            hr = srpRange[1]->moveStart(L"Character", -1, &cReturn);
            if (FAILED(hr))
                goto Fail;
    
            hr = srpRange[1]->moveEnd(L"Character", -1, &cReturn);
            if (FAILED(hr))
                goto Fail;
        }

        srpParent = srpRange[1];
        while (srpParent != NULL)
        {
            pselEnd->srpCell.Release();
            if (FAILED(hr = GetDispatchProperty(srpParent, L"parentElement", VT_DISPATCH, (void**)&pselEnd->srpCell)))
                goto Fail;

            if (pselEnd->srpCell == NULL)
                {
                hr = E_FAIL;
                goto Fail;
                }

            bstrTag.Empty();
            if (FAILED(hr = GetDispatchProperty(pselEnd->srpCell, L"tagName", VT_BSTR, &bstrTag)))
                goto Fail;
            
            if (lstrcmpi(_T("TD"), OLE2T(bstrTag)) == 0 || lstrcmpi(_T("TH"), OLE2T(bstrTag)) == 0)
            {
                break;
            }
            srpParent = pselEnd->srpCell;
        }

        _ASSERTE(pselEnd->srpCell != NULL);
        if (FAILED(hr = GetDispatchProperty(pselEnd->srpCell, L"cellIndex", VT_I4, &pselEnd->cCellIndex)))
            goto Fail;

        pselEnd->srpRow.Release();
        if (FAILED(hr =  GetTableRowElementAndTableFromCell(pselEnd->srpCell, &pselEnd->cRowIndex, &pselEnd->srpRow, &pselEnd->srpTable)))
            goto Fail;
    }

    if (pselEnd != NULL && pselStart != NULL)
    {
    // VID 98 bug 3116: we need to check if first cell and last cell are in the same table. If they are not
    // the row index and cell index we just got do not make sense
        if (FAILED(hr = CallDispatchMethod(pselEnd->srpTable, L"contains", VTS_DISPATCH VTS_BOOL_RETURN, pselStart->srpRow, &fContain)))
            goto Fail;

        if (!fContain)
           return E_FAIL;

        fContain = FALSE;
        if (FAILED(hr = CallDispatchMethod(pselStart->srpTable, L"contains", VTS_DISPATCH VTS_BOOL_RETURN, pselEnd->srpRow, &fContain)))
            goto Fail;

        if (!fContain)
           return E_FAIL;
    }


Fail:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::GetSelectionTypeInTable
//
// Return a set of flags that characterize the current selection. Return
// -1 if something goes wrong. The flags are as follows:
//
//      grfInSingleRow          Selection is comprised of one or more cells 
//                              within a single row.
//
//      grfSelectOneCell        Selection is comprised of a single cell.
//
//      grpSelectEntireRow      Selection is comprised of one or more 
//                              complete rows.

ULONG    CTriEditDocument::GetSelectionTypeInTable(void)
{
    CComPtr<IDispatch>    srpCells;
    struct SELCELLINFO    selinfo[2]; // 0 is start cell, 1 is end cell
    LONG cCells=0;
    HRESULT hr=0;
    ULONG grf=0;

    if (FAILED(hr = FillInSelectionCellsInfo(&selinfo[0], &selinfo[1])))
        goto Fail;

    if (selinfo[0].cRowIndex == selinfo[1].cRowIndex)
    {
        grf |= grfInSingleRow;
        if (selinfo[0].cCellIndex == selinfo[1].cCellIndex)
            grf |= grfSelectOneCell;
    }
    else
    {
        grf &= ~grfInSingleRow;
    }

    if (selinfo[0].cCellIndex != 0)
        grf &= ~grpSelectEntireRow;
    else
    {
        srpCells.Release();
        if (FAILED(hr = GetDispatchProperty(selinfo[1].srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
            goto Fail;

        if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
            goto Fail;

        if (selinfo[1].cCellIndex != cCells-1)
            grf &= ~grpSelectEntireRow;
        else
            grf |= grpSelectEntireRow;
    }



Fail:
    return FAILED(hr) ? -1 : grf;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::CopyProperty
//
// Copy properties from the pFrom element on to the pTo element. Return S_OK.
//

HRESULT CTriEditDocument::CopyProperty(IDispatch *pFrom, IDispatch *pTo)
{
    CComVariant varProp;
    CComBSTR bstrProp;
    VARIANT_BOOL bProp;

    bstrProp.Empty();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"align", VT_BSTR, (void **)&bstrProp)))
    {
        if (lstrcmpW(bstrProp, L""))
            PutDispatchProperty(pTo, L"align", VT_BSTR, bstrProp);
    }
    
    bstrProp.Empty();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"vAlign", VT_BSTR, (void **)&bstrProp)))
    {
        if (lstrcmpW(bstrProp, L""))
            PutDispatchProperty(pTo, L"vAlign", VT_BSTR, bstrProp);
    }

    bstrProp.Empty();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"background", VT_BSTR, (void **)&bstrProp)))
    {
        if (lstrcmpW(bstrProp, L""))
            PutDispatchProperty(pTo, L"background", VT_BSTR, bstrProp);
    }

    bstrProp.Empty();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"lang", VT_BSTR, (void **)&bstrProp)))
    {
        if (lstrcmpW(bstrProp, L""))
            PutDispatchProperty(pTo, L"lang", VT_BSTR, bstrProp);
    }   

    bstrProp.Empty();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"className", VT_BSTR, (void **)&bstrProp)))
    {
        if (lstrcmpW(bstrProp, L""))
            PutDispatchProperty(pTo, L"className", VT_BSTR, bstrProp);
    }   
    
    varProp.Clear();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"bgColor", VT_VARIANT, (void **)&varProp)))
        PutDispatchProperty(pTo, L"bgColor", VT_VARIANT, varProp);

    varProp.Clear();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"borderColor", VT_VARIANT, (void **)&varProp)))
        PutDispatchProperty(pTo, L"borderColor", VT_VARIANT, varProp);
    
    varProp.Clear();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"borderColorLight", VT_VARIANT, (void **)&varProp)))
        PutDispatchProperty(pTo, L"borderColorLight", VT_VARIANT, varProp);

    varProp.Clear();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"borderColorDark", VT_VARIANT, (void **)&varProp)))
        PutDispatchProperty(pTo, L"borderColorDark", VT_VARIANT, varProp);

    varProp.Clear();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"height", VT_VARIANT, (void **)&varProp)))
        PutDispatchProperty(pTo, L"height", VT_VARIANT, varProp);

    varProp.Clear();
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"width", VT_VARIANT, (void **)&varProp)))
        PutDispatchProperty(pTo, L"width", VT_VARIANT, varProp);

    
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"noWrap", VT_BOOL, (void **)&bProp)))
    {
#pragma warning(disable: 4310) // cast truncates constant value
        if (bProp == VARIANT_TRUE) 
#pragma warning(default: 4310) // cast truncates constant value
            PutDispatchProperty(pTo, L"noWrap", VT_BOOL, bProp);
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::CopyStyle
//
// Copy style properties from style element pFrom on to style element pTo.
// Return S_OK.
//      

HRESULT CTriEditDocument::CopyStyle(IDispatch *pFrom, IDispatch *pTo)
{
    CComPtr<IDispatch>  srpStyleTo, srpStyleFrom;
  
    if (SUCCEEDED(GetDispatchProperty(pFrom, L"style", VT_DISPATCH, (void **)&srpStyleFrom)))
    {
        if (SUCCEEDED(GetDispatchProperty(pTo, L"style", VT_DISPATCH, (void **)&srpStyleTo)))
        {
            CComVariant varProp;
            CComBSTR bstrProp;

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"backgroundAttachment", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"backgroundAttachment", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"backgroundImage", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"backgroundImage", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"backgroundRepeat", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"backgroundRepeat", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"borderBottom", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"borderBottom", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"borderLeft", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"borderLeft", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"borderTop", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"borderTop", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"borderRight", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"borderRight", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"fontFamily", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"fontFamily", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"fontStyle", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"fontStyle", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"fontVariant", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"fontVariant", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"fontWeight", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"fontWeight", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"textAlign", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"textAlign", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"textTransform", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"textTransform", VT_BSTR, bstrProp);
            }   

            bstrProp.Empty();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"textDecoration", VT_BSTR, (void **)&bstrProp)))
            {
                if (lstrcmpW(bstrProp, L""))
                    PutDispatchProperty(srpStyleTo, L"textDecoration", VT_BSTR, bstrProp);
            }   
            
            varProp.Clear();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"backgroundcolor", VT_VARIANT, (void **)&varProp)))
                PutDispatchProperty(srpStyleTo, L"backgroundcolor", VT_VARIANT, varProp);

            varProp.Clear();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"color", VT_VARIANT, (void **)&varProp)))
                PutDispatchProperty(srpStyleTo, L"color", VT_VARIANT, varProp);

            varProp.Clear();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"fontSize", VT_VARIANT, (void **)&varProp)))
                PutDispatchProperty(srpStyleTo, L"fontSize", VT_VARIANT, varProp);

            varProp.Clear();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"height", VT_VARIANT, (void **)&varProp)))
                PutDispatchProperty(srpStyleTo, L"height", VT_VARIANT, varProp);

            varProp.Clear();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"letterSpacing", VT_VARIANT, (void **)&varProp)))
                PutDispatchProperty(srpStyleTo, L"letterSpacing", VT_VARIANT, varProp);

            varProp.Clear();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"lineHeight", VT_VARIANT, (void **)&varProp)))
                PutDispatchProperty(srpStyleTo, L"lineHeight", VT_VARIANT, varProp);

            varProp.Clear();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"paddingRight", VT_VARIANT, (void **)&varProp)))
                PutDispatchProperty(srpStyleTo, L"paddingRight", VT_VARIANT, varProp);

            varProp.Clear();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"paddingBottom", VT_VARIANT, (void **)&varProp)))
                PutDispatchProperty(srpStyleTo, L"paddingBottom", VT_VARIANT, varProp);

            varProp.Clear();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"paddingLeft", VT_VARIANT, (void **)&varProp)))
                PutDispatchProperty(srpStyleTo, L"paddingLeft", VT_VARIANT, varProp);

            varProp.Clear();
            if (SUCCEEDED(GetDispatchProperty(srpStyleFrom, L"paddingTop", VT_VARIANT, (void **)&varProp)))
                PutDispatchProperty(srpStyleTo, L"paddingTop", VT_VARIANT, varProp);
        }
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DeleteTableRows
//
// Delete the table row(s) contained within the Trident selection. The 
// entire operation is a single undo unit. Return S_OK or a Trident error.
//

HRESULT CTriEditDocument::DeleteTableRows(void)
{
    HRESULT    hr = S_OK;
    CComPtr<IHTMLElement>       srpTable;
    struct SELCELLINFO    selinfo[2]; // 0 is start cell, 1 is end cell
    INT i=0;
    CUndoPackManager undoPackMgr(m_pUnkTrident);

    if (FAILED(hr = IsSelectionInTable((IDispatch**)&srpTable)))
        goto Fail;

    if (FAILED(hr = FillInSelectionCellsInfo(&selinfo[0], &selinfo[1])))
        goto Fail;
            
    undoPackMgr.Start();
    
    for(i= selinfo[0].cRowIndex; i <= selinfo[1].cRowIndex; i++)
    {
        if (FAILED(hr = DeleteRowEx(srpTable, selinfo[0].cRowIndex)))
            goto Fail;
    }
    
Fail:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DeleteRowEx
//
// Delete the indicated table row. If the row is the only row in the table,
// delete the whole table. Return S_OK or a Trident error.
//

inline HRESULT CTriEditDocument::DeleteRowEx(IHTMLElement *pTable, LONG index)
{
    HRESULT    hr = S_OK;
    CComPtr<IDispatch> srpRows;
    INT cRows = 0;

    if (FAILED(hr = GetDispatchProperty(pTable, L"rows", VT_DISPATCH, (void**)&srpRows)))
        goto Fail;

    if (FAILED(hr = GetDispatchProperty(srpRows, L"length", VT_I4, &cRows)))
        goto Fail;

    // if this is the only row in the table, delete the whole table
    if (cRows == 1)
    {
        _ASSERT(index == 0);
        hr = DeleteTable(pTable);
    }
    else
    {
        if (FAILED(hr = CallDispatchMethod(pTable, L"deleteRow", VTS_I4, index)))
            goto Fail;
    }

Fail:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DeleteCellEx
//
// Delete the indicated cell from the indicated row of the given table. If
// the cell is the only row in the table, delete the whole table. Return
// S_OK or a Trident error.
//

inline HRESULT CTriEditDocument::DeleteCellEx(IHTMLElement *pTable, IDispatch *pRow, LONG indexRow, LONG indexCell)
{
    HRESULT    hr = S_OK;
    CComPtr<IDispatch> srpCells;
    INT cCells = 0;

    if (FAILED(hr = GetDispatchProperty(pRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
        goto Fail;

    if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
        goto Fail;

    // if this is the only cell in the table, delete the whole row
    if (cCells == 1)
    {
        _ASSERT(indexCell == 0);
        hr = DeleteRowEx(pTable, indexRow);
    }
    else
    {
        if (FAILED(hr = CallDispatchMethod(pRow, L"deleteCell", VTS_I4, indexCell)))
            goto Fail;
    }

Fail:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DeleteTable
//
// Delete the given table. Return S_OK if all goes well; E_FAIL if something
// goes wrong.
//

HRESULT CTriEditDocument::DeleteTable(IHTMLElement *pTable)
{
    CComPtr<IHTMLElement>   srpParent;
    HRESULT hr = E_FAIL;

    _ASSERTE(pTable != NULL);

    if (pTable  == NULL)
        goto Fail;
        
    if (FAILED(hr=pTable->get_offsetParent(&srpParent)))
        goto Fail;

    _ASSERTE(srpParent != NULL);
    if (FAILED(hr = SelectElement(pTable, srpParent)))
        goto Fail;
        
    hr = Exec(&CMDSETID_Forms3, IDM_DELETE, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
    
Fail:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::InsertTableRow
//
// Insert a new table row in to the table which contains the Trident selection,
// in the row preceding the selection. The new row will have the same number of
// cells as the row containing the selection. The colSpan of each new cell
// will be copied from the row containing the selection. The entire operation
// is a single undo unit. Returns S_OK or a Trident error.
//

HRESULT CTriEditDocument::InsertTableRow(void)
{
    HRESULT    hr = S_OK;
    CComPtr<IDispatch> srpCell,srpCellNew, srpTable,srpCells,srpRows,srpNewRow,srpCellsNew;
    LONG ccolSpan=0;
    LONG cCells=0,i=0;
    struct SELCELLINFO    selinfo;
    CUndoPackManager undoPackMgr(m_pUnkTrident);

    undoPackMgr.Start();

    if (FAILED(hr = IsSelectionInTable(&srpTable)))
        goto Fail;

    if (FAILED(hr = FillInSelectionCellsInfo(&selinfo, NULL)))
        goto Fail;

    if (FAILED(hr = CallDispatchMethod(srpTable, L"insertRow", VTS_I4, selinfo.cRowIndex)))
        goto Fail;

    if (FAILED(hr = GetDispatchProperty(srpTable, L"rows", VT_DISPATCH, (void**)&srpRows)))
        goto Fail;

    if (FAILED(hr = CallDispatchMethod(srpRows, L"Item", VTS_I4 VTS_DISPATCH_RETURN, selinfo.cRowIndex, &srpNewRow)))
        goto Fail;

    CopyStyle(selinfo.srpRow, srpNewRow);
    
    // get the number of cells contains in the selected row
    if (FAILED(hr = GetDispatchProperty(selinfo.srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
        goto Fail;

    if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
        goto Fail;

    // now insert cells
    for (i=cCells-1; i >=0; i--)
    {
         if (FAILED(hr = CallDispatchMethod(srpNewRow, L"insertCell", VTS_I4, 0)))
             goto Fail;

         srpCell.Release();
         if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, i, &srpCell)))
             goto Fail;

         srpCellsNew.Release();
         if (FAILED(hr = GetDispatchProperty(srpNewRow, L"cells", VT_DISPATCH, (void**)&srpCellsNew)))
             goto Fail;
         srpCellNew.Release();
         if (FAILED(hr = CallDispatchMethod(srpCellsNew, L"Item", VTS_I4 VTS_DISPATCH_RETURN, 0, &srpCellNew)))
             goto Fail;

         CopyStyle(srpCell, srpCellNew);
         CopyProperty(srpCell, srpCellNew);
         
         {
         VARIANT width;
         VariantInit(&width);
         if (SUCCEEDED(hr = GetDispatchProperty(srpCell, L"width", VT_VARIANT, &width)))
             PutDispatchProperty(srpCellNew, L"width", VT_VARIANT, width);
         }

         if (SUCCEEDED(hr = GetDispatchProperty(srpCell, L"colSpan", VT_I4, &ccolSpan)))
             PutDispatchProperty(srpCellNew, L"colSpan", VT_I4, ccolSpan);
    }

Fail:
       return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::MapCellToFirstRowCell
//
// Given a table cell in pselInfo, return (by modifying pselInfo) the cell in
// the first row with the same column position, accounting for colSpans. Return
// S_OK or a Trident error.
//

HRESULT CTriEditDocument::MapCellToFirstRowCell(IDispatch *srpTable, struct SELCELLINFO *pselinfo)
{
    HRESULT hr = 0;
    CComPtr<IDispatch> srpCell, srpCells,srpRow,srpRows;
    INT i=0,iCellIndex=0,iColSpanCurRow=0,cSpan=0,iColSpanFirstRow=0,crowSpan=0;

    _ASSERTE(pselinfo != NULL);
    // if current selection is not first row, find the corresponding first row cell index
    if (pselinfo->cRowIndex == 0)
        return S_OK;

    srpCells.Release();
    _ASSERTE(pselinfo->srpRow != NULL);
    if (FAILED(hr = GetDispatchProperty(pselinfo->srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
        goto Fail;

    for (i=0; i < pselinfo->cCellIndex ; i++)
    {
        srpCell.Release();
        _ASSERTE(srpCells != NULL);
        if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, i, &srpCell)))
            goto Fail;

        _ASSERTE(srpCell != NULL);
        if (FAILED(hr = GetDispatchProperty(srpCell, L"colSpan", VT_I4, &cSpan)))
            goto Fail;

        iColSpanCurRow += cSpan;
    }

    srpRows.Release();
     _ASSERTE(srpTable != NULL);

    if (FAILED(hr = GetDispatchProperty(srpTable, L"rows", VT_DISPATCH, (void**)&srpRows)))
        goto Fail;

    _ASSERTE(srpRows != NULL);
    srpRow.Release();
    if (FAILED(hr = CallDispatchMethod(srpRows, L"Item",VTS_I4 VTS_DISPATCH_RETURN, 0, &srpRow)))
        goto Fail;

    srpCells.Release();
    if (FAILED(hr = GetDispatchProperty(srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
        goto Fail;

    iCellIndex=-1;
    while(iColSpanCurRow >= iColSpanFirstRow)
    {
        iCellIndex++;
        srpCell.Release();
        _ASSERTE(srpCells != NULL);
        if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, iCellIndex, &srpCell)))
            goto Fail;

        // we might hit the end. If so, first row is shorter than curret row and there's no mapping first row, bail out...
        if (srpCell == NULL)
        {
        hr = E_FAIL;
        goto Fail;
        }

        _ASSERTE(srpCell != NULL);
        if (FAILED(hr = GetDispatchProperty(srpCell, L"colSpan", VT_I4, &cSpan)))
            goto Fail;

        iColSpanFirstRow += cSpan;

        if (FAILED(hr = GetDispatchProperty(srpCell, L"rowSpan", VT_I4, &crowSpan)))
            goto Fail;

        if (crowSpan > pselinfo->cRowIndex)
        {
            iColSpanCurRow += cSpan;
        }

    }

    pselinfo->srpCell = srpCell;
    pselinfo->srpRow.Release();
    if (FAILED(hr = GetTableRowElementAndTableFromCell(pselinfo->srpCell, NULL, &pselinfo->srpRow)))
        goto Fail;

    pselinfo->cRowIndex = 0;
    _ASSERTE(iCellIndex >= 0);
    pselinfo->cCellIndex = iCellIndex;

Fail:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::InsertTableCol
//
// Insert a new column in to the table containing the selection, at the column
// of the selection. The entire operation is a single undo unit. Return S_OK
// or a Trident error.
//

HRESULT CTriEditDocument::InsertTableCol(void)
{
    HRESULT    hr = S_OK;
    CComPtr<IDispatch>               srpCellNew, srpTable,srpRows,srpRow,srpCells,srpCell;
    LONG    cRows=0,i=0, j=0, iColSpanInsert=0, iColSpanCur=0, cSpan=0,crowSpan = 0, cCells=0;
    struct SELCELLINFO    selinfo;
    INT *pccolFix = NULL;
    CUndoPackManager undoPackMgr(m_pUnkTrident);

    undoPackMgr.Start();

    if (FAILED(hr = IsSelectionInTable(&srpTable)))
        goto Fail;

    if (FAILED(hr = FillInSelectionCellsInfo(&selinfo, NULL)))
        goto Fail;

    MapCellToFirstRowCell(srpTable, &selinfo);

    srpCells.Release();
    _ASSERTE(selinfo.srpRow != NULL);
    if (FAILED(hr = GetDispatchProperty(selinfo.srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
        goto Fail;

    _ASSERTE(srpTable != NULL);
    if (FAILED(hr = GetDispatchProperty(srpTable, L"rows", VT_DISPATCH, (void**)&srpRows)))
       goto Fail;

    _ASSERTE(srpRows != NULL);
    if (FAILED(hr = GetDispatchProperty(srpRows, L"length", VT_I4, &cRows)))
       goto Fail;

    pccolFix = new INT[cRows];
    _ASSERTE(pccolFix != NULL);
    for (i=0; i< cRows; i++)
        *(pccolFix+i) = 0;

    for (i=0; i < selinfo.cCellIndex; i++)
    {
        srpCell.Release();
        _ASSERTE(srpCells != NULL);
        if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, i, &srpCell)))             goto Fail;

        _ASSERTE(srpCell != NULL);
        if (FAILED(hr = GetDispatchProperty(srpCell, L"colSpan", VT_I4, &cSpan)))
            goto Fail;

        iColSpanInsert += cSpan;

        if (FAILED(hr = GetDispatchProperty(srpCell, L"rowSpan", VT_I4, &crowSpan)))
            goto Fail;

    // if someone before the current cell has row span, this needs to propogate to
    // the next spanned rows
        if (crowSpan > 1)
            {
            for (j= selinfo.cRowIndex+1; j < (selinfo.cRowIndex+crowSpan); j++)
                *(pccolFix+j) += cSpan;
            }
    }

    for (i=0; i < cRows;)
    {
        srpRow.Release();
         _ASSERTE(srpRows != NULL);
        if (FAILED(hr = CallDispatchMethod(srpRows, L"Item", VTS_I4 VTS_DISPATCH_RETURN, i, &srpRow)))
            goto Fail;

        srpCells.Release();
        _ASSERTE(srpRow != NULL);
        if (FAILED(hr = GetDispatchProperty(srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
            goto Fail;

        _ASSERTE(srpCells != NULL);
        if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
            goto Fail;

        iColSpanCur =  *(pccolFix+i);
        for (j=0; j < cCells; j++)
        {
            srpCell.Release();
            if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, j, &srpCell)))             goto Fail;

             _ASSERTE(srpCell != NULL);
            if (FAILED(hr = GetDispatchProperty(srpCell, L"colSpan", VT_I4, &cSpan)))
                goto Fail;

            if (iColSpanCur >= iColSpanInsert)
                break;

            iColSpanCur += cSpan;
        }

        _ASSERTE(srpRow != NULL);
        if (FAILED(hr = CallDispatchMethod(srpRow, L"insertCell", VTS_I4, j)))
            goto Fail;

        srpCells.Release();
        if (FAILED(hr = GetDispatchProperty(srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
            goto Fail;

        srpCellNew.Release();
        if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, j, &srpCellNew)))
            goto Fail;

                
        if (!(!srpCell))
        {
            CopyStyle(srpCell, srpCellNew);
            CopyProperty(srpCell, srpCellNew);
            
            {
            VARIANT height;
            VariantInit(&height);
            if (SUCCEEDED(hr = GetDispatchProperty(srpCell, L"height", VT_VARIANT, &height)))
                PutDispatchProperty(srpCellNew, L"height", VT_VARIANT, height);
             }

            if (SUCCEEDED(GetDispatchProperty(srpCell, L"rowSpan", VT_I4, &cSpan)))
                PutDispatchProperty(srpCellNew, L"rowSpan", VT_I4, cSpan);
        }

        // cSpan might be 0 if we are inserting a cell into an empty row
        i += max(1, cSpan);
    }

Fail:
    if (pccolFix != NULL)
        delete [] pccolFix;
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DeleteTableCols
//
// Delete the table columns that are contained within the Trident selection.
// The entire operation is a single undo unit. Return S_OK or a Trident error. 
//

HRESULT CTriEditDocument::DeleteTableCols(void)
{
    CComPtr<IDispatch>       srpRows,srpRow,srpCells,srpCell;
    CComPtr<IHTMLElement>   srpTable;
    struct SELCELLINFO          selinfo[2]; // 0 is start cell, 1 is end cell
    LONG cRows=0, i=0, j=0, k=0, cCells=0;
    HRESULT      hr=0;
    LONG iColSpanStart=0, iColSpanEnd=0,cColSpan=0,iColSpanCur=0, crowSpan=0;
    INT *  pccolFixStart=NULL, *pccolFixEnd = NULL;
    CUndoPackManager undoPackMgr(m_pUnkTrident);

    undoPackMgr.Start();

    if (FAILED(hr = IsSelectionInTable((IDispatch**)&srpTable)))
        goto Fail;

    if (FAILED(hr = FillInSelectionCellsInfo(&selinfo[0], &selinfo[1])))
        goto Fail;

    if (!FAILED(MapCellToFirstRowCell(srpTable, &selinfo[1])))
        MapCellToFirstRowCell(srpTable, &selinfo[0]);

    _ASSERTE(selinfo[0].srpRow != NULL);
    if (FAILED(hr = GetDispatchProperty(selinfo[0].srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
        goto Fail;

    _ASSERTE(srpCells != NULL);
    if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
        goto Fail;

    _ASSERTE(selinfo[1].cRowIndex == selinfo[0].cRowIndex);
    _ASSERTE(selinfo[1].cCellIndex >= selinfo[0].cCellIndex);

    srpRows.Release();
    if (FAILED(hr = GetDispatchProperty(srpTable, L"rows", VT_DISPATCH, (void**)&srpRows)))
       goto Fail;

    _ASSERTE(srpRows != NULL);
    if (FAILED(hr = GetDispatchProperty(srpRows, L"length", VT_I4, &cRows)))
       goto Fail;

    pccolFixEnd = new INT[cRows];
    pccolFixStart = new INT[cRows];
    for (i=0; i< cRows; i++)
        {
        *(pccolFixStart+i) = 0;
        *(pccolFixEnd+i) = 0;
        }

    for (i=0; i<= selinfo[1].cCellIndex; i++)
    {
        srpCell.Release();
        if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, i, &srpCell)))             goto Fail;

        if (FAILED(hr = GetDispatchProperty(srpCell, L"colSpan", VT_I4, &cColSpan)))
            goto Fail;
        if (i < selinfo[0].cCellIndex)
           iColSpanStart += cColSpan;

        if (i <= selinfo[1].cCellIndex)
           iColSpanEnd += cColSpan;

        if (FAILED(hr = GetDispatchProperty(srpCell, L"rowSpan", VT_I4, &crowSpan)))
            goto Fail;

        if (crowSpan > 1)
        {
            if (i < selinfo[0].cCellIndex)
            {
                for (j= selinfo[0].cRowIndex+1; j < selinfo[0].cRowIndex+crowSpan; j++)
                    *(pccolFixStart+j) += cColSpan;
            }

            if (i <= selinfo[1].cCellIndex)
            {
                for (j= selinfo[0].cRowIndex+1; j < selinfo[0].cRowIndex+crowSpan; j++)
                    *(pccolFixEnd+j) += cColSpan;
            }
        }
    }

    for (j=cRows-1; j >= 0; j--)
    {
        srpRow.Release();
        if (FAILED(hr = CallDispatchMethod(srpRows, L"Item", VTS_I4 VTS_DISPATCH_RETURN, j, &srpRow)))
            goto Fail;

        srpCells.Release();
        if (FAILED(hr = GetDispatchProperty(srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
            goto Fail;

        if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
            goto Fail;

        iColSpanCur = 0;
        _ASSERTE(iColSpanEnd-*(pccolFixEnd+j) >= 0);
        _ASSERTE(iColSpanStart-*(pccolFixStart+j) >= 0);

        for (i=0, k=0; iColSpanCur <= (iColSpanEnd-*(pccolFixEnd+j)) && k < cCells ; i++, k++)
        {
            srpCell.Release();
            if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, i, &srpCell)))
                goto Fail;
            if (FAILED(hr = GetDispatchProperty(srpCell, L"colSpan", VT_I4, &cColSpan)))
                goto Fail;

            if (iColSpanCur >= (iColSpanStart-*(pccolFixStart+j)) && iColSpanCur < (iColSpanEnd-*(pccolFixEnd+j)))
            {
                if (FAILED(hr = DeleteCellEx(srpTable, srpRow, j, i)))
                    goto Fail;
                i--; // we've deleted one cell, need to decrement cell index
            }

            iColSpanCur += cColSpan;
        }
    }

Fail:
    if (pccolFixStart != NULL)
     {
         delete [] pccolFixStart;
     }

    if (pccolFixEnd != NULL)
     {
         delete [] pccolFixEnd;
     }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::InsertTableCell
//
// Insert a table cell before the cell containing the Trident selection; copy
// the properties and style of the cell containing the selection to the new
// cell. The entire operation is a single undo unit. Returns S_OK or a Trident
// error.
//

HRESULT CTriEditDocument::InsertTableCell(void)
{
    HRESULT    hr = S_OK;
    struct SELCELLINFO    selinfo;
    CComPtr<IDispatch>    srpCellNew, srpCells;
    CUndoPackManager undoPackMgr(m_pUnkTrident);

    undoPackMgr.Start();
    
    if (FAILED(hr = FillInSelectionCellsInfo(&selinfo, NULL)))
        goto Fail;

    if (FAILED(hr = CallDispatchMethod(selinfo.srpRow, L"insertCell", VTS_I4, selinfo.cCellIndex)))
       goto Fail;

    srpCells.Release();
    if (FAILED(hr = GetDispatchProperty(selinfo.srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
        goto Fail;

    srpCellNew.Release();
    if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, selinfo.cCellIndex, &srpCellNew)))
        goto Fail;

    CopyStyle(selinfo.srpCell, srpCellNew);
    CopyProperty(selinfo.srpCell, srpCellNew);

Fail:

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DeleteTableCells
//
// Delete the table cells contained within the Trident selection. Delete entire
// rows as indicated. The entire operation is a single undo unit. Return
// S_OK or a Trident error.
//

HRESULT CTriEditDocument::DeleteTableCells(void)
{
    CComPtr<IHTMLElement>       srpTable,srpCells;
    struct SELCELLINFO          selinfo[2]; // 0 is start cell, 1 is end cell
    LONG i=0, cCells=0;
    HRESULT      hr=0;
    CUndoPackManager undoPackMgr(m_pUnkTrident);

    undoPackMgr.Start();

    if (FAILED(hr = IsSelectionInTable((IDispatch**)&srpTable)))
        goto Fail;

    if (FAILED(hr = FillInSelectionCellsInfo(&selinfo[0], &selinfo[1])))
        goto Fail;

    if (selinfo[0].cRowIndex == selinfo[1].cRowIndex) // same row
    {
        srpCells.Release();
        if (FAILED(hr = GetDispatchProperty(selinfo[0].srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
            goto Fail;

        if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
            goto Fail;

        // if the selection is select all the cells in this row, delete the whole row instead
        if ( cCells == selinfo[1].cCellIndex+1 && selinfo[0].cCellIndex == 0)
        {
            if (FAILED(hr = DeleteRowEx(srpTable, selinfo[0].cRowIndex)))
                goto Fail;
        }
        else // delete cell by cell
        {
            for (i = selinfo[1].cCellIndex; i >= selinfo[0].cCellIndex; i--)
            {
                if (FAILED(hr = DeleteCellEx(srpTable, selinfo[0].srpRow, selinfo[0].cRowIndex, i)))
                    goto Fail;
            }
         }
    }
    else
    {
        srpCells.Release();
        if (FAILED(hr = GetDispatchProperty(selinfo[1].srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
                goto Fail;

        if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
                goto Fail;

        // if the selection ends at the last cell of the row, delete the whole row instead
        if ( cCells == selinfo[1].cCellIndex+1)
        {
            if (FAILED(hr = DeleteRowEx(srpTable, selinfo[1].cRowIndex)))
                goto Fail;
        }
        else // delete cell by cell
        {
            for (i = selinfo[1].cCellIndex; i >= 0; i--)
            {
                if (FAILED(hr = DeleteCellEx(srpTable, selinfo[1].srpRow, selinfo[1].cRowIndex, i)))
                    goto Fail;
            }
        }
        
        for (i = selinfo[1].cRowIndex-1; i > selinfo[0].cRowIndex; i--)
        {
            if (FAILED(hr = DeleteRowEx(srpTable, i)))
                goto Fail;
        }

       
        if (selinfo[0].cCellIndex == 0) // if the selection is from first cell of a row across other rows, delete the whole row instead
        {
            if (FAILED(hr = DeleteRowEx(srpTable, selinfo[0].cRowIndex)))
                goto Fail;
        }
        else // delete cell by cell
        {
            srpCells.Release();
            if (FAILED(hr = GetDispatchProperty(selinfo[0].srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
                goto Fail;

            if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
                goto Fail;

            for (i = cCells-1; i >= selinfo[0].cCellIndex; i--)
            {
                if (FAILED(hr = DeleteCellEx(srpTable, selinfo[0].srpRow, selinfo[0].cRowIndex, i)))
                    goto Fail;
            }
        }
    }

Fail:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::MergeTableCells
//
// Merge the indicated cells in to a single cell, and adjust its colSpan.
// The cells must be within a single table row. The innerHTML of all merged cells
// is concatenated and placed in the remaining cell. Return S_OK or a Trident error.
//

HRESULT CTriEditDocument::MergeTableCells(IDispatch* srpTable, INT iRow, INT iIndexStart, INT iIndexEnd)
{
    CComPtr<IDispatch>    srpCells,srpRows,srpCurRow,srpCell;
    INT ccolSpanTotal=0, i=0, ccolSpan=0;
    HRESULT      hr=0;
    CComBSTR    bstrText;
    CComBSTR    bstrMergedText;

    if (FAILED(hr = GetDispatchProperty(srpTable, L"rows", VT_DISPATCH, (void**)&srpRows)))
        goto Fail;

    if (FAILED(hr = CallDispatchMethod(srpRows, L"Item", VTS_I4 VTS_DISPATCH_RETURN, iRow, &srpCurRow)))
        goto Fail;

    srpCells.Release();
    if (FAILED(hr = GetDispatchProperty(srpCurRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
        goto Fail;

    bstrMergedText.Empty();
    ccolSpanTotal = 0;

    for (i = iIndexEnd; i >= iIndexStart; i--)
    {
        srpCell.Release();
        if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, i, &srpCell)))
            goto Fail;

        bstrText.Empty();
        if (FAILED(hr = GetDispatchProperty(srpCell, L"innerHTML", VT_BSTR, &bstrText)))
            goto Fail;
        bstrText += bstrMergedText;
        bstrMergedText = bstrText;

        if (FAILED(hr = GetDispatchProperty(srpCell, L"colSpan", VT_I4, &ccolSpan)))
            goto Fail;
        ccolSpanTotal += ccolSpan;

        if (i != iIndexStart)
        {
              if (FAILED(hr = DeleteCellEx((IHTMLElement*)srpTable, srpCurRow, iRow, i)))
                  goto Fail;
        }
        else
        {
            if (FAILED(hr = PutDispatchProperty(srpCell, L"colSpan", VT_I4, ccolSpanTotal)))
                goto Fail;
            if (FAILED(hr = PutDispatchProperty(srpCell, L"innerHTML", VT_BSTR, bstrMergedText)))
                goto Fail;
        }
    }
Fail:
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::MergeTableCells
//
// Merge the cells in the Trident selection in to a single cell, and adjust that
// cell's colSpan. The cells must be within a single table row. The innerHTML of
// all merged cells is concatenated and placed in the remaining cell. Return S_OK
// or a Trident error.
//

HRESULT CTriEditDocument::MergeTableCells(void)
{
    CComPtr<IDispatch>       srpCell, srpCells,srpElement,srpRows,srpRow;
    CComPtr<IHTMLElement>   srpTable;
    struct SELCELLINFO          selinfo[2]; // 0 is start cell, 1 is end cell
    LONG i=0, cCells=0;
    HRESULT      hr=0;
    CComBSTR    bstrText;
    CComBSTR    bstrMergedText;
    CUndoPackManager undoPackMgr(m_pUnkTrident);

    undoPackMgr.Start();

    if (FAILED(hr = IsSelectionInTable((IDispatch**)&srpTable)))
        goto Fail;

    if (FAILED(hr = FillInSelectionCellsInfo(&selinfo[0], &selinfo[1])))
        goto Fail;

    if (selinfo[0].cRowIndex == selinfo[1].cRowIndex)
    {
        if (selinfo[1].cCellIndex == selinfo[0].cCellIndex)
            {
                hr = S_OK;
                goto Fail;
            }

        if (FAILED(hr = MergeTableCells(srpTable, selinfo[0].cRowIndex, selinfo[0].cCellIndex, selinfo[1].cCellIndex)))
            goto Fail;
    }
    else
    {
        srpCells.Release();
        if (FAILED(hr = GetDispatchProperty(selinfo[1].srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
            goto Fail;

        if (FAILED(hr = MergeTableCells(srpTable, selinfo[1].cRowIndex, 0, selinfo[1].cCellIndex)))
            goto Fail;

        if (FAILED(hr = GetDispatchProperty(srpTable, L"rows", VT_DISPATCH, (void**)&srpRows)))
            goto Fail;

        for (i = selinfo[1].cRowIndex-1; i > selinfo[0].cRowIndex; i--)
        {
            srpElement.Release();
            if (FAILED(hr = CallDispatchMethod(srpRows, L"Item", VTS_I4 VTS_DISPATCH_RETURN, i, &srpElement)))
                goto Fail;

            srpCells.Release();
            if (FAILED(hr = GetDispatchProperty(srpElement, L"cells", VT_DISPATCH, (void**)&srpCells)))
                goto Fail;

            if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
                goto Fail;

            if (FAILED(hr = MergeTableCells(srpTable, i, 0, cCells-1)))
                goto Fail;
        }

        srpCells.Release();
        if (FAILED(hr = GetDispatchProperty(selinfo[0].srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
            goto Fail;

        if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
            goto Fail;

        if (FAILED(hr = MergeTableCells(srpTable, selinfo[0].cRowIndex, selinfo[0].cCellIndex, cCells-1)))
            goto Fail;

        bstrMergedText.Empty();
        for (i = selinfo[0].cRowIndex; i <= selinfo[1].cRowIndex; i++)
        {
            srpRows.Release();
            if (FAILED(hr = GetDispatchProperty(srpTable, L"rows", VT_DISPATCH, (void**)&srpRows)))
                goto Fail;

            srpRow.Release();
            if (FAILED(hr = CallDispatchMethod(srpRows, L"Item", VTS_I4 VTS_DISPATCH_RETURN, selinfo[0].cRowIndex, &srpRow)))
                goto Fail;

            srpCells.Release();
            if (FAILED(hr = GetDispatchProperty(srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
                goto Fail;

            srpCell.Release();
            if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, 0, &srpCell)))
                goto Fail;

            bstrText.Empty();
            if (FAILED(hr = GetDispatchProperty(srpCell, L"innerHTML", VT_BSTR, &bstrText)))
                goto Fail;
            bstrMergedText += L"<P>";
            bstrMergedText += bstrText;
            bstrMergedText += L"</P>";

            if (i != selinfo[1].cRowIndex)
            {
                if (FAILED(hr = DeleteRowEx(srpTable, selinfo[0].cRowIndex)))
                    goto Fail;
            }
        }
        if (FAILED(hr = PutDispatchProperty(srpCell, L"innerHTML", VT_BSTR, bstrMergedText)))
            goto Fail;
    }

Fail:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::SplitTableCell 
//
// Split the indicated table cell in to two cells and adjust the colSpan
// of the relevant cells in the other rows as needed. The entire operation is
// a single undo unit. Return S_OK or a Trident error.
//

HRESULT CTriEditDocument::SplitTableCell(IDispatch *srpTable, INT iRow, INT index)
{
    CComPtr<IDispatch>       srpCellSplit, srpCells,srpCell,srpElement,srpRows,srpRow,srpCurRow,srpCellNew;
    INT cRows=0,i=0,j=0,ccolSpan=0,ccolSpanCur=0,crowSpan=0, cCells=0;
    HRESULT      hr=0;
    CComBSTR    bstrText;
    INT *pccolFix = NULL;
    CUndoPackManager undoPackMgr(m_pUnkTrident);

    undoPackMgr.Start();
    
    if (FAILED(hr = GetDispatchProperty(srpTable, L"rows", VT_DISPATCH, (void**)&srpRows)))
        goto Fail;

    if (FAILED(hr = GetDispatchProperty(srpRows, L"length", VT_I4, &cRows)))
        goto Fail;

    if (FAILED(hr = CallDispatchMethod(srpRows, L"Item", VTS_I4 VTS_DISPATCH_RETURN, iRow, &srpCurRow)))
        goto Fail;

    if (FAILED(hr = CallDispatchMethod(srpCurRow, L"insertCell", VTS_I4, index+1)))
        goto Fail;

    srpCells.Release();
    if (FAILED(hr = GetDispatchProperty(srpCurRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
        goto Fail;

    srpCellNew.Release();
    if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, index+1, &srpCellNew)))
        goto Fail;
                
    srpCellSplit.Release();
    if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, index, (void**)&srpCellSplit)))
        goto Fail;

    ccolSpan=0;
    if (FAILED(hr = GetDispatchProperty(srpCellSplit, L"colSpan", VT_I4, &ccolSpan)))
        goto Fail;

    CopyStyle(srpCellSplit, srpCellNew);
    CopyProperty(srpCellSplit, srpCellNew);
    
    if (ccolSpan == 1)
    {
        INT ccolSpanStart = 0,ccolSpanEnd=0;
        INT ccolSpanTmp = 0, cRowSpan = 0;

        pccolFix = new INT[cRows];
        for (j=0; j < cRows; j++)
            *(pccolFix+j) = 0;

        for (j=0; j<index;j++)
        {
            srpCell.Release();
            if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, j, (void**)&srpCell)))
                goto Fail;

            ccolSpanTmp = 0;
            if (FAILED(hr = GetDispatchProperty(srpCell, L"colSpan", VT_I4, &ccolSpanTmp)))
                goto Fail;
            ccolSpanStart += ccolSpanTmp;

            if (FAILED(hr = GetDispatchProperty(srpCell, L"rowSpan", VT_I4, &cRowSpan)))
                goto Fail;

            if (cRowSpan > 1)
                for (i = index+1; i < index+cRowSpan; i++)
                    *(pccolFix+i) += ccolSpanTmp;
        }

        ccolSpanEnd = ccolSpanStart + ccolSpan;

        for (j=0; j < cRows; j++)
        {
            if (j == iRow)
                continue;

            srpRow.Release();
            if (FAILED(hr = CallDispatchMethod(srpRows, L"Item", VTS_I4 VTS_DISPATCH_RETURN, j, &srpRow)))
                goto Fail;

            srpCells.Release();
            if (FAILED(hr = GetDispatchProperty(srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
                goto Fail;

            if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
                goto Fail;

            ccolSpanCur = *(pccolFix+j);
            for(i=0 ; i < cCells; i++)
            {
                srpCell.Release();
                if (FAILED(hr = CallDispatchMethod(srpCells, L"Item", VTS_I4 VTS_DISPATCH_RETURN, i, &srpCell)))
                    goto Fail;

                ccolSpan=0;
                if (FAILED(hr = GetDispatchProperty(srpCell, L"colSpan", VT_I4, &ccolSpan)))
                    goto Fail;

                if (ccolSpanStart <= ccolSpanCur && ccolSpanCur < ccolSpanEnd)
                {
                    if (FAILED(hr = PutDispatchProperty(srpCell, L"colSpan", VT_I4, ccolSpan+1)))
                        goto Fail;
                }

                if (ccolSpanCur >= ccolSpanEnd)
                    break;

                ccolSpanCur += ccolSpan;
            }
         }
     }
     else
     {
         if (FAILED(hr = PutDispatchProperty(srpCellNew, L"colSpan", VT_I4, ccolSpan/2)))
             goto Fail;

         if (FAILED(hr = PutDispatchProperty(srpCellSplit, L"colSpan", VT_I4, ccolSpan-ccolSpan/2)))
             goto Fail;
     }

    
      // now copy row span
     if (FAILED(hr = GetDispatchProperty(srpCellSplit, L"rowSpan", VT_I4, &crowSpan)))
         goto Fail;

     if (FAILED(hr = PutDispatchProperty(srpCellNew, L"rowSpan", VT_I4, crowSpan)))
         goto Fail;

Fail:
     if (pccolFix != NULL)
     {
         delete [] pccolFix;
      }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::SplitTableCell 
//
// Split the table cell in the Trident selection in to two cells and adjust the
// colSpan of the relevant cells in the other rows as needed. The entire operation
// is a single undo unit. Return S_OK or a Trident error.
//

HRESULT CTriEditDocument::SplitTableCell(void)
{
    CComPtr<IDispatch>       srpCell, srpTable,srpCells,srpElement,srpRows,srpRow;
    struct SELCELLINFO          selinfo[2]; // 0 is start cell, 1 is end cell
    LONG i=0, j=0,cCells=0;
    HRESULT      hr=0;
    CUndoPackManager undoPackMgr(m_pUnkTrident);

    undoPackMgr.Start();

    if (FAILED(hr = IsSelectionInTable(&srpTable)))
        goto Fail;

    if (FAILED(hr = FillInSelectionCellsInfo(&selinfo[0], &selinfo[1])))
        goto Fail;

    if (FAILED(hr = GetDispatchProperty(srpTable, L"rows", VT_DISPATCH, (void**)&srpRows)))
        goto Fail;

    if (selinfo[0].cRowIndex == selinfo[1].cRowIndex)
    {
        for (i = selinfo[1].cCellIndex; i >= selinfo[0].cCellIndex; i--)
        {
           if (FAILED(hr = SplitTableCell(srpTable, selinfo[0].cRowIndex, i)))
               goto Fail;
        }
    }
    else
    {
        for (i = selinfo[1].cCellIndex; i >= 0; i--)
        {
            if (FAILED(hr = SplitTableCell(srpTable, selinfo[1].cRowIndex, i)))
               goto Fail;
        }

        for (i = selinfo[1].cRowIndex-1; i > selinfo[0].cRowIndex; i--)
        {
            srpElement.Release();
            if (FAILED(hr = CallDispatchMethod(srpRows, L"Item", VTS_I4 VTS_DISPATCH_RETURN, i, &srpElement)))
                goto Fail;

            srpCells.Release();
            if (FAILED(hr = GetDispatchProperty(srpElement, L"cells", VT_DISPATCH, (void**)&srpCells)))
                goto Fail;

            if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
                goto Fail;

            for (j = cCells-1; j >= 0; j--)
            {
                if (FAILED(hr = SplitTableCell(srpTable, i, j)))
                    goto Fail;
            }
        }

        srpCells.Release();
        if (FAILED(hr = GetDispatchProperty(selinfo[0].srpRow, L"cells", VT_DISPATCH, (void**)&srpCells)))
            goto Fail;

        if (FAILED(hr = GetDispatchProperty(srpCells, L"length", VT_I4, &cCells)))
            goto Fail;

        for (i = cCells-1; i >= selinfo[0].cCellIndex; i--)
        {
            if (FAILED(hr = SplitTableCell(srpTable, selinfo[0].cRowIndex, i)))
               goto Fail;
        }
    }

Fail:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::InsertTable
//
// Insert a table in to the document at the selection point. All parameters
// are optional and taken from members of pvarargIn as follows:
//
//      pvarargIn[0]    I4   - Number of rows; default 0.
//      pvarargIn[1]    I4   - Number of columns; default 0.
//      pvarargIn[2]    BSTR - Table tag attributes; default "".
//      pvarargIn[3]    BSTR - Table cell attributes; default "".
//      pvarargIn[4]    BSTR - Table caption; default "".
//
// pvarArgIn must be sipplied even if the default values are to be used for
// all parameters. The entire operation is a single undo unit. The wait cursor
// is displayed since this can be a fairly time-consuming operation. Returns S_OK
// or a Trident error.
//

HRESULT CTriEditDocument::InsertTable(VARIANTARG *pvarargIn)
{
    HRESULT    hr=0;
    CComPtr<IHTMLSelectionObject>    srpSel;
    CComPtr<IHTMLTxtRange> srpRange;
    CComPtr<IDispatch>    srpCell;
    CComPtr<IHTMLDocument2>    srpiHTMLDoc;
    CComBSTR    bstrHtml;
    CComBSTR    bstrTblAttr;
    CComBSTR    bstrTCellAttr;
    CComBSTR    bstrCaption;
    int i=0, j=0, iRow=0, iCol=0;
    VARIANT rgvar[5];
    HCURSOR hOldCursor;

    if (pvarargIn == NULL)
        return E_FAIL;

    hOldCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

    for(i = 0; i < sizeof(rgvar)/sizeof(VARIANT); i++)
        VariantInit(&rgvar[i]);

    // default
    iRow=1;
    iCol=1;
    bstrTCellAttr.Empty();
    bstrTblAttr.Empty();

    if (pvarargIn != NULL)
    {
        LONG lLBound=0, lUBound=0,lIndex=0;
        SAFEARRAY *psa;
        LONG cParam; // number of parameters host passes in

        psa = V_ARRAY(pvarargIn);
        SafeArrayGetLBound(psa, 1, &lLBound);
        SafeArrayGetUBound(psa, 1, &lUBound);
        cParam = 0;
        _ASSERTE(lLBound == 0);
        _ASSERTE(lUBound -  lLBound < 5);
        for (lIndex = lLBound; lIndex <= lUBound && cParam < sizeof(rgvar)/sizeof(VARIANT); lIndex++)
        {
             SafeArrayGetElement(psa, &lIndex, &rgvar[cParam++]);
        }

        // first element: number of rows
        if (cParam >= 1)
            iRow = V_I4(&rgvar[0]);
        // 2'rd element: number of columns
        if (cParam >= 2)
            iCol = V_I4(&rgvar[1]);
        // 3'rd element: table tag attributes
        if (cParam >= 3)
            bstrTblAttr = V_BSTR(&rgvar[2]);
        // 4'th element: table cell tag attributes
        if (cParam >= 4)
            bstrTCellAttr = V_BSTR(&rgvar[3]);
        if (cParam >= 5)
            bstrCaption = V_BSTR(&rgvar[4]);
    }

    if (iRow < 0 || iCol < 0)
        goto Fail;

    bstrHtml.Empty();
    bstrHtml += "<TABLE ";
    if (bstrTblAttr != NULL)
        bstrHtml += bstrTblAttr;
    bstrHtml += ">";

    if (bstrCaption != NULL)
    {
         bstrHtml += "<CAPTION>";
         bstrHtml += bstrCaption;
         bstrHtml += "</CAPTION>";
    }

    bstrHtml +="<TBODY>";

    for (i=0; i<iRow; i++)
    {
        bstrHtml += "<TR>";
        for (j=0; j<iCol; j++)
        {
            bstrHtml += "<TD ";
            if (bstrTCellAttr != NULL)
                bstrHtml += bstrTCellAttr;
            bstrHtml +="></TD>";
        }
        bstrHtml += "</TR>";
    }
    bstrHtml += "</TBODY></TABLE>";

    if (FAILED(hr = m_pUnkTrident->QueryInterface(IID_IHTMLDocument2, (void**)&srpiHTMLDoc)))
        goto Fail;

    if (FAILED(hr = srpiHTMLDoc->get_selection(&srpSel)))
        goto Fail;

    if (FAILED(hr = CallDispatchMethod(srpSel, L"createRange", VTS_DISPATCH_RETURN, (void**)&srpRange)))
        goto Fail;

    if (FAILED(hr = CallDispatchMethod(srpRange, L"pasteHTML", VTS_BSTR, bstrHtml)))
        goto Fail;

Fail:

    for(i = 0; i < sizeof(rgvar)/sizeof(VARIANT); i++)
        VariantClear(&rgvar[i]);

    ::SetCursor(hOldCursor);
    return hr;

}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DoVerb
//
// Execute the verb in pvarargIn (or OLEIVERB_PRIMARY if pvarargIn is NULL)
// on the current object (which must QI for IHTMLObjectElement). Return E_FAIL
// or the code returned as a result of executing the verb,
//

HRESULT CTriEditDocument::DoVerb(VARIANTARG *pvarargIn, BOOL fQueryStatus)
{
    LONG iVerb;
    IHTMLObjectElement *piHTMLObjectElement = NULL;
    IDispatch *pDisp = NULL;
    IOleObject *pOleObj = NULL;
    HRESULT hr = E_FAIL;

    _ASSERTE(m_pihtmlElement != NULL);

    if (SUCCEEDED(m_pihtmlElement->QueryInterface(IID_IHTMLObjectElement, (void **)&piHTMLObjectElement)) && piHTMLObjectElement)
    {
        if (SUCCEEDED(piHTMLObjectElement->get_object(&pDisp)) && pDisp)
        {
            if (SUCCEEDED(pDisp->QueryInterface(IID_IOleObject, (void **)&pOleObj)) && pOleObj)
            {
                if (fQueryStatus) // In the query status case, we're done
                    hr = S_OK;
                else
                {
                    if (pvarargIn == NULL)
                        iVerb = OLEIVERB_PRIMARY;
                    else if (pvarargIn->vt == VT_I4)
                        iVerb = V_I4(pvarargIn);    
                    else
                    {
                        hr = E_INVALIDARG;
                        goto LSkipDoVerb;
                    }

                    GetTridentWindow();
                    _ASSERTE(m_hwndTrident != NULL);

                    hr = pOleObj->DoVerb(iVerb, NULL, NULL, 0, m_hwndTrident, NULL);
                }
LSkipDoVerb:
                pOleObj->Release();
            }
            pDisp->Release();
        }
        piHTMLObjectElement->Release();
    }

    return hr;

}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::GetDocument
//
// Return the IHTMLDocument pointer (under *ppihtmlDocument) and S_OK, or
// E_FAIL/E_POINTER.
//

STDMETHODIMP CTriEditDocument::GetDocument(IHTMLDocument2** ppihtmlDocument)
{
    _ASSERTE(ppihtmlDocument);
    if (ppihtmlDocument)
    {
        if (m_pUnkTrident)
        {
            return m_pUnkTrident->QueryInterface(IID_IHTMLDocument2,
                        (LPVOID*)ppihtmlDocument);
        }
        return E_FAIL;
    }
    return E_POINTER;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::GetAllColllection
//
// Return the all collection of the HTML document (under *ppihtmlCollection),
// or E_FAIL.
//

STDMETHODIMP CTriEditDocument::GetAllCollection(IHTMLElementCollection** ppihtmlCollection)
{
    IHTMLDocument2* pihtmlDoc2;
    HRESULT hr=E_FAIL;

    _ASSERTE(ppihtmlCollection);
    if (ppihtmlCollection && SUCCEEDED(GetDocument(&pihtmlDoc2)))
    {
        _ASSERTE(pihtmlDoc2);
        hr = pihtmlDoc2->get_all(ppihtmlCollection);
        pihtmlDoc2->Release();
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::GetCollectionElement
//
// Return the indicated element from the given collection under *ppihtmlElement.
// Return S_OK if all goes well,or E_FAIL or a Triedent error on error.
//

STDMETHODIMP CTriEditDocument::GetCollectionElement(
    IHTMLElementCollection* pihtmlCollection,
    LONG iElem, IHTMLElement** ppihtmlElement)
{
    VARIANT var;
    VARIANT varEmpty;
    IDispatch* pidispElement=NULL;
    HRESULT hr = E_FAIL;

    _ASSERTE(pihtmlCollection && iElem >= 0 && ppihtmlElement);
    if (!pihtmlCollection || iElem < 0 || !ppihtmlElement)
        return E_POINTER;

    *ppihtmlElement = NULL;     //initialize [out] parameter

    VariantInit(&var);
    var.vt = VT_I4;
    var.lVal = iElem;

    VariantInit(&varEmpty);
    varEmpty.vt = VT_EMPTY;

    hr = pihtmlCollection->item(var, varEmpty, &pidispElement);
    if (SUCCEEDED(hr))
    {
        if (pidispElement)
        {
            hr = pidispElement->QueryInterface(IID_IHTMLElement, (LPVOID*)ppihtmlElement);
            pidispElement->Release();
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//
// CTriEditDocument::Is2DCapable
//
// Return (under *pfBool) TRUE if the given HTML element can be positioned
// out of the flow as a 2D element, or FALSE if not. Return S_OK in either
// case. Return E_FAIL or a Trident error if something goes wrong.
// 

STDMETHODIMP CTriEditDocument::Is2DCapable(IHTMLElement* pihtmlElement, BOOL* pfBool)
{
    HRESULT hr= E_FAIL;
    CComBSTR bstrTag;

    _ASSERTE(pihtmlElement);

    if (!pihtmlElement || !pfBool)
        return E_POINTER;

    *pfBool = FALSE;

     bstrTag.Empty();
     if (FAILED(hr = GetDispatchProperty(pihtmlElement, L"tagName", VT_BSTR, &bstrTag)))
            return E_FAIL;

     if (lstrcmpi(_T("APPLET"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("BUTTON"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("DIV"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("EMBED"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("FIELDSET"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("HR"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("IFRAME"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("IMG"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("INPUT"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("MARQUEE"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("OBJECT"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("SELECT"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("SPAN"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("TABLE"), OLE2T(bstrTag)) == 0 ||
         lstrcmpi(_T("TEXTAREA"), OLE2T(bstrTag)) == 0 )
    {
        *pfBool = TRUE;
        return S_OK;
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::SelectElement
//
// Select the given element within Trident as a site selection. Return S_OK or
// a Trident error.
//

STDMETHODIMP CTriEditDocument::SelectElement(IHTMLElement* pihtmlElement, IHTMLElement* pihtmlElementParent)
{
    IHTMLControlElement* picont=NULL;
    IHTMLElement* piParent=NULL;
    IDispatch* pidisp=NULL;
    IHTMLTextContainer* pitext=NULL;
    IHTMLControlRange* pirange=NULL;
    HRESULT hr;
    CComBSTR bstrTag;

    if ( !pihtmlElement || !pihtmlElementParent )
        return E_FAIL;
    
    hr = pihtmlElement->QueryInterface(IID_IHTMLControlElement, (LPVOID*)&picont);

    if ( FAILED(hr) )
        goto CleanUp;

    _ASSERTE(picont);

    hr = pihtmlElementParent->QueryInterface(IID_IHTMLTextContainer, (LPVOID*)&pitext);

    if ( FAILED(hr) )
        goto CleanUp;

    _ASSERTE(pitext);

    hr = pitext->createControlRange(&pidisp);

    if ( FAILED(hr) )
        goto CleanUp;

    _ASSERTE(pitext);

    hr = pidisp->QueryInterface(IID_IHTMLControlRange, (LPVOID*)&pirange);

    if ( FAILED(hr) )
        goto CleanUp;

    _ASSERTE(pirange);

    hr = pirange->add(picont);

    if ( FAILED(hr) )
        goto CleanUp;

    hr = pirange->select();

CleanUp:
    SAFERELEASE(picont);
    SAFERELEASE(piParent);
    SAFERELEASE(pidisp);
    SAFERELEASE(pitext);
    SAFERELEASE(pirange);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::IsElementDTC
//
// Return S_OK if the given element is a DTC (Design-Time Control) or E_FAIL 
// if not.
//

HRESULT CTriEditDocument::IsElementDTC(IHTMLElement *pihtmlElement)
{
    IHTMLObjectElement *piHTMLObjectElement = NULL;
    IDispatch *pDisp = NULL;
    IActiveDesigner *piActiveDesigner = NULL;
    IUnknown *piUnk = NULL;

    if (SUCCEEDED(pihtmlElement->QueryInterface(IID_IHTMLObjectElement, (void **)&piHTMLObjectElement)) && piHTMLObjectElement)
    {
        if (SUCCEEDED(piHTMLObjectElement->get_object(&pDisp)) && pDisp)
        {
            if (SUCCEEDED(pDisp->QueryInterface(IID_IUnknown, (void **)&piUnk)) && piUnk)
            {
                if (SUCCEEDED(piUnk->QueryInterface(IID_IActiveDesigner, (void **)&piActiveDesigner)) && piActiveDesigner)
                {
                    piHTMLObjectElement->Release();
                    pDisp->Release();
                    piUnk->Release();
                    piActiveDesigner->Release();
                    return S_OK;
                }
                piUnk->Release();
            }
            pDisp->Release();
        }
        piHTMLObjectElement->Release();
    }

    return E_FAIL;
}
