//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************
//
//      CACHECPL.H - header file for cachecpl
//
//      HISTORY:
//      
//      4/6/98      v-sriran         Moved some definitions from cachecpl.cpp
//                                   to this file, so that it can be easier
//                                   to make unix specific changes to cachecpl.
//

#ifndef _CACHECPL_H_
#define _CACHECPL_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// private structures for cachecpl
//
typedef struct {
    // hold our dialog handle
    HWND hDlg;

    // hold dialog item handles
    HWND hwndTrack;

    // data
    TCHAR szHistoryLocation[MAX_PATH+1];
    INT  iHistoryNumPlaces;
    UINT uiCacheQuota;
    UINT uiDiskSpaceTotal;
    WORD iCachePercent;
    TCHAR szCacheLocation[MAX_PATH+1];
    TCHAR szNewCacheLocation[MAX_PATH+1];
    INT  iCacheUpdFrequency;
    INT  iHistoryExpireDays;

    // something changed
    BOOL bChanged;
    BOOL bChangedLocation;

} TEMPDLG, *LPTEMPDLG;

#define CONTENT 0

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CACHECPL_H_ */
