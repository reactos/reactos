//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// view.h 
//
//   Definitions used by the IShellView helper functions.
//
//   History:
//
//       3/20/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _VIEW_H_

#define _VIEW_H_

//
// Function prototypes.
//

HRESULT CreateDefaultShellView(IShellFolder *pIShellFolder,
                               LPITEMIDLIST pidl,
                               IShellView** ppIShellView);

HRESULT CALLBACK IShellViewCallback(IShellView* pIShellViewOuter,
                                    IShellFolder* pIShellFolder,
                                    HWND hwnd,
                                    UINT msg,
                                    WPARAM wParam,
                                    LPARAM lParam);

HRESULT IShellView_GetDetails(UINT nColumn, PDETAILSINFO pDetails);



#endif // _VIEW_H_