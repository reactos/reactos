//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft, 1997
//
//  File:       Dispatch.cpp
//
//  Contents:   CPreviewBand::IDispatch methods
//
//  History:    7-24-97  Davepl  Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "PreviewBand.h"
#include "exdispid.h"
#include "sfview.h"

HRESULT CPreviewBand::Invoke( DISPID       dispidMember, 
                              REFIID       riid, 
                              LCID         lcid, 
                              WORD         wFlags, 
                              DISPPARAMS * pdispparams, 
                              VARIANT    * pvarResult, 
                              EXCEPINFO  * pexcepinfo, 
                              UINT       * puArgErr )
{
    HRESULT hr = S_OK;

    if (DISPID_BEFORENAVIGATE == dispidMember)
    {
        // View is about to recycle to a new folder, so we need to detach from
        // the current one

        return DetachFromView();
    }
    else if (DISPID_NAVIGATECOMPLETE == dispidMember)
    {
        // Browser has completed navigating, so we can now attach to the currently
        // actiue view

        return AttachToView();
    }
    else if (DISPID_SELECTIONCHANGED == dispidMember)
    {
        // Selection has changed in the current view.  Get a pointer to the current
        // view and the count of items selected int it.
         
        m_ptrExtractImage = NULL;

        CComPtr<IShellView> ptrShellView;
        m_ptrBrowser->QueryActiveShellView(&ptrShellView);
        if (ptrShellView)
        {
            CComQIPtr<IShellFolderView, &IID_IShellFolderView> ptrFolderView = ptrShellView;
            if (ptrFolderView)
            {
                ptrFolderView->GetSelectedCount(&m_uSelected);
                if (m_uSelected)
                {
                    // Get the IExtractImage handler for the currently selected item(s)

                    if (SUCCEEDED(hr = ptrShellView->GetItemObject(SVGIO_SELECTION, 
                                                                   IID_IExtractImage, 
                                                                   (void **)&m_ptrExtractImage)))
                    {
                    }
                }
            }
        }

        // Success or failure, we update the preview

        UpdatePreview();
    }

    return hr;
}

