//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// view.cpp 
//
//   IShellView helper functions.  Cdf view uses the default IShellView and
//   relies on a callback to supply specific information.
//
//   History:
//
//       3/20/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "view.h"
#include "cdfidl.h"
#include "resource.h"

#include <mluisupp.h>

#include <shellp.h>     // SHCreateShellFolderViewEx


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CreateDefaultShellView ***
//
//
// Description:
//     Creates a shell implemented default IShellView object for the given
//     folder.
//
// Parameters:
//     [In]  pIShellFolder - The folder for which the default IShellView is
//                           created.
//     [In]  pidl          - The id list for the given folder.
//     [Out] ppIShellView  - A pointer to receive the IShellView interface.
//
// Return:
//     The result from the private shell function SHCreateShellFolderViewEx.
//
// Comments:
//     The default IShellView object communicates with its associated folder
//     via a callback function.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CreateDefaultShellView(
    IShellFolder *pIShellFolder,
    LPITEMIDLIST pidl,
    IShellView** ppIShellView
)
{
    ASSERT(pIShellFolder);
    ASSERT(ppIShellView);

    CSFV csfv;

    csfv.cbSize      = sizeof(CSFV);
    csfv.pshf        = pIShellFolder;
    csfv.psvOuter    = NULL;
    csfv.pidl        = pidl;
    csfv.lEvents     = 0; //SHCNE_DELETE | SHCNE_CREATE;
    csfv.pfnCallback = IShellViewCallback;
    csfv.fvm         = (FOLDERVIEWMODE)0; // FVM_ICON, FVM_DETAILS, etc.

    return SHCreateShellFolderViewEx(&csfv, ppIShellView);
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** IShellViewCallback ***
//
//
// Description:
//     The callback function used by the default ISHellView to request
//     inforamtion.
//
// Parameters:
//     [In]  pIShellViewOuter - Always NULL.
//     [In]  pIShellFolder    - The folder associated with this view.
//     [In]  hwnd             - The hwnd of the shell view.
//     [In]  msg              - The callback message.
//     [InOut] wParam         - Message specific parameter.
//     [InOut] lParam         - Message specific parameter.
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CALLBACK IShellViewCallback(
    IShellView* pIShellViewOuter,
    IShellFolder* pIShellFolder,
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    HRESULT hr;

    switch (msg)
    {
    case DVM_GETDETAILSOF:
        hr = IShellView_GetDetails((UINT)wParam, (PDETAILSINFO)lParam);
        break;

    //
    // Background enumeration only works for default shell view.
    //

    //case SFVM_BACKGROUNDENUM:
    //    hr = S_OK;
    //    TraceMsg(TF_CDFENUM, "Enum Background thread callback tid:0x%x",
    //             GetCurrentThreadId());
    //    break;
    
    default:
        hr = E_FAIL;
        break;
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** IShellView_GetDetails ***
//
//
// Description:
//     The IShellView callback DVM_GETDETAILSOF message handler.
//
// Parameters:
//     [In]  nColumn    - The column for wich information is requested.
//     [InOut] pDetails - For column headings the pidl param is NULL and the
//                        columns format, width, and title are returned.  For
//                        items the pidl member conatins the id list of the
//                        requested item and the string value of the requested
//                        item is returned.
//
// Return:
//     S_OK if nColumn is supported.
//     E_FAIL if nColumn is greater than the number of supported columns.
//
// Comments:
//     The default shell view calls this function with successively higher
//     column numbers until an E_FAIL is returned.
//
//     The first (0) column is the display name.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
IShellView_GetDetails(
    UINT nColumn,
    PDETAILSINFO pDetails
)
{
    //
    // Column information.
    //

    #define     COLUMNS  (sizeof(aColumnInfo) / sizeof(aColumnInfo[0]))

    static const struct _tagCOLUMNINFO
    {
        UINT   idsName;
        UINT   cchWidth;
        USHORT uFormat;
    }
    aColumnInfo[] = {
                      {IDS_COLUMN_NAME, 50, LVCFMT_LEFT}
                    };
    
    HRESULT hr;

    if (nColumn < COLUMNS)
    {
        if (NULL != pDetails->pidl) {

            //
            // Get item information from the pidl.
            //

            switch (aColumnInfo[nColumn].idsName)
            {
            case IDS_COLUMN_NAME:
                //pDetails->str.uType = STRRET_CSTR;
                CDFIDL_GetDisplayName((PCDFITEMIDLIST)pDetails->pidl,
                                      &pDetails->str);
                break;
            }

        }
        else
        {
            //
            // Get column heading information.
            //

            pDetails->fmt       = aColumnInfo[nColumn].uFormat;
            pDetails->cxChar    = aColumnInfo[nColumn].cchWidth;
            pDetails->str.uType = STRRET_CSTR;

            //
            // REVIEW:  Using MLLoadStringA.
            //
            
            MLLoadStringA(aColumnInfo[nColumn].idsName,
                          pDetails->str.cStr, sizeof(pDetails->str.cStr));
        }

        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}
