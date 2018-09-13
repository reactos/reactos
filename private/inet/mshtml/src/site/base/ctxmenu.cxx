//+---------------------------------------------------------------------
//
//   File:      ctxmenu.cxx
//
//  Contents:   forms kernel command handlers
//
//  Notes:      These commands are exposed at design time only, and then
//              only through context menus. The form kernel has no menu bars
//              of it's own.
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef WIN16
#ifndef X_OLEDLG_H_
#define X_OLEDLG_H_
#include <oledlg.h>
#endif
#endif

#ifndef X_PROPUTIL_HXX_
#define X_PROPUTIL_HXX_
#include "proputil.hxx"      // For font dialog
#endif

//+---------------------------------------------------------------
//
//  Member:     CDoc::EditUndo
//
//  Synopsis:   Manage Undo command
//
//---------------------------------------------------------------

ExternTag(tagUndo);
PerfDbgExtern(tagMarkupUndo);

HRESULT
CDoc::EditUndo()
{
    TraceTag((tagUndo, "CDoc::EditUndo"));

    HRESULT hr = THR(_pUndoMgr->UndoTo(NULL));

    RRETURN(SetErrorInfo(hr)); // BUGBUG more context needed
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::EditRedo
//
//  Synopsis:   Performs a Redo
//
//---------------------------------------------------------------

HRESULT
CDoc::EditRedo()
{
    TraceTag((tagUndo, "CDoc::EditRedo"));

    HRESULT hr = THR(_pUndoMgr->RedoTo(NULL));

    RRETURN(SetErrorInfo(hr)); // BUGBUG more context needed
}

