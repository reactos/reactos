
//***********************************************************************
// export.cpp
//
// This file contains the implementation of the CDlgExport class.  This class
// puts up the "Export Events" dialog and handles writing the events out
// to an export file in the user-selected format.
//
// Author: Larry A. French
//
// History:
//      1-Mar-1996     Larry A. French
//          Wrote it.
//
//      14-May-1996     Larry A. French
//          Fixed a problem where the defines for the file extension included
//          a "." prefix instead of just the base extension.  This caused various
//          problems such as generting file names such as "foo..cnf" when the
//          user just entered "foo".
//
// Copyright (C) 1995, 1996 Microsoft Corporation.  All rights reserved.
//
//************************************************************************


#include "stdafx.h"
#include "busy.h"
#include "export.h"
#include "globals.h"
#include "trapreg.h"
//#include "smsnames.h"

//**************************************************************************
// CDlgExport::CDlgExport
//
// Constructor for the CDlgExport class.
//
//**************************************************************************
CDlgExport::CDlgExport() : CFileDialog(FALSE)
{
    // We need to strip off the "." prefix for the filename extensions
    // so that it is easier to use them.  Ideally we would just change
    // the defines for these extension strings, but since their definitions
    // are in a global file, we will just fix the problem here.
    m_ofn.Flags |= OFN_PATHMUSTEXIST;
    m_sFileTitle.LoadString(IDS_EXPORT_DEFAULT_FILENAME);
}

//**************************************************************
// CDlgExport::GetFilters
//
// Get the filter strings that will be used in CFileDialog.
// These are the filter strings that appear in the drop-down
// file-type combo.  Each filter is described by a pair of
// adjacent strings.  The first string of each pair specifies the
// "type" string that the user sees.  The second string of each
// pair specifies the file extension associated with the file type.
//
// For more information, please see the CFileDialog documentation.
//
// Parameters:
//      LPTSTR pszDst
//          Pointer to the destination buffer.  The size of this
//          buffer should be MAX_STRING, so there will be plenty
//          of room for the filter strings since they are relatively
//          short.  Note that no bounds checking is done on the
//          buffer size.
//
// Returns:
//      The filter strings are returned in the buffer pointed to
//      by pszDst.
//
//****************************************************************
void CDlgExport::GetFilters(LPTSTR pszDst)
{
    CString sText;

    // Set the type1 filter
    sText.LoadString(IDS_EXPORT_CNF_FILTER);
    _tcscpy(pszDst, (LPCTSTR) sText);
    pszDst += sText.GetLength() + 1;

    // Set the type1 extension
    _tcscpy(pszDst, FILE_DEF_EXT);
    pszDst += _tcslen(FILE_DEF_EXT) + 1;

    *pszDst = 0;
}

//*************************************************************************
// CDlgExport::ExportEvents
//
// Write the events to the specified file in the specified file format.
//
// Parameters:
//      CXEventArray& aEvents
//          The array of events to be written to the file.
//
//      CString& sPath
//          The output file's pathname.
//
//      LONG iFileType
//          The output file's format type. This may be EXPORT_TYPE1 or
//          EXPORT_TYPE2.
//
// Returns:
//      SCODE
//          S_OK if everything was successfule, otherwise E_FAIL.
//
//***************************************************************************
SCODE CDlgExport::ExportEvents(CXEventArray& aEvents, CString& sPath, LONG iFileType)
{
    CBusy busy;
    FILE* pfile;

    // Create the export file
    while (TRUE) {
        pfile = _tfopen(sPath, _T("w"));
        if (pfile != NULL) {
            break;
        }

        CString sText;
        sText.LoadString(IDS_ERR_CANT_CREATE_FILE);
        sText = sText + sPath;
        if (AfxMessageBox(sText, MB_RETRYCANCEL) == IDRETRY) {
            continue;
        }
        return E_FAIL;
    }

    // Write the events to the file in the requested format.
    LONG nEvents = aEvents.GetSize();
    for (LONG iEvent = 0; iEvent < nEvents; ++iEvent)
    {
        CXEvent* pEvent = aEvents[iEvent];
        CXEventSource* pEventSource = pEvent->m_pEventSource;
        CXEventLog* pEventLog = pEventSource->m_pEventLog;

        _ftprintf(pfile, _T("#pragma add %s \"%s\" %lu %lu %lu\n"),
                (LPCTSTR) pEventLog->m_sName,
                (LPCTSTR) pEventSource->m_sName,
                pEvent->m_message.m_dwId,
                pEvent->m_dwCount,
                pEvent->m_dwTimeInterval
                );
    }

    fclose(pfile);
    return S_OK;
}


//*************************************************************************************
// CDlgExport::DoModal
//
// This is the only public method for CDlgExport.  It displays the "Export Events" dialog
// and does everthing necessary to write out the event file in the proper format.
//
// Parameters:
//      CXEventArray& aEvents
//          The events that the user wants to export.
//
// Returns:
//      int
//          IDOK if the user exported the events and everything went OK.
//          IDCANCEL if the user canceled the export or an error occurred writing
//          the export file.
//
//**************************************************************************************
INT_PTR CDlgExport::DoModal(CXEventArray& aEvents)
{
    ASSERT(aEvents.GetSize() > 0);

    // Put up a custom CFileDialog with a title of "Export Events"
    CString sTitle;
    sTitle.LoadString(IDS_EXPORT_DIALOG_TITLE);
    m_ofn.lpstrTitle = sTitle;

    // The value to initialize the filename edit item to.  A temporary
    // string is used because we only want to save the file title and
    // not its full path.
    CString sFile = m_sFileTitle;
    m_ofn.lpstrFile = sFile.GetBuffer(MAX_STRING);
    m_ofn.nMaxFile = MAX_STRING - 1;

    // Set the file title, so that when the user clicks OK, its
    // value will be set.
    m_ofn.lpstrFileTitle = m_sFileTitle.GetBuffer(MAX_STRING);
    m_ofn.nMaxFileTitle = MAX_STRING - 1;

    // Set the filters for the different file types.
    TCHAR szFilters[MAX_STRING];
    GetFilters(szFilters);
    m_ofn.lpstrFilter = (LPCTSTR) (void*) szFilters;

    // Put up the dialog.
    INT_PTR iStat = CFileDialog::DoModal();
    m_sFileTitle.ReleaseBuffer();
    sFile.ReleaseBuffer();

    sFile = GetPathName();

    // If the user selected "OK", write out the event file in the selected format.
    if (iStat == IDOK)
    {
        SCODE sc = ExportEvents(aEvents, sFile, m_ofn.nFilterIndex);
        if (FAILED(sc)) {
            iStat = IDCANCEL;
        }
    }
    return iStat;
}
