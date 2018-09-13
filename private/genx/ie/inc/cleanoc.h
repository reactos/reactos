#ifndef __CLEANOC_API__
#define __CLEANOC_API__

// Flags used in GetControlInfo()
#define GCI_NAME         1
#define GCI_FILE         2
#define GCI_CLSID        3
#define GCI_TYPELIBID    4
#define GCI_TOTALSIZE    5
#define GCI_SIZESAVED    6
#define GCI_TOTALFILES   7
#define GCI_CODEBASE     8
#define GCI_ISDISTUNIT   9
#define GCI_DIST_UNIT_VERSION 10
#define GCI_STATUS       11
#define GCI_HAS_ACTIVEX  12
#define GCI_HAS_JAVA     13

// control status flags
#define STATUS_CTRL_UNKNOWN             0   // Errors prevent determining the actual control state
#define STATUS_CTRL_INSTALLED           1   // Control is properly installed and ready for use
#define STATUS_CTRL_SHARED              2   // One or more components are shared by more than one control
#define STATUS_CTRL_DAMAGED             3   // The control file or some part of the installation is damaged or missing
#define STATUS_CTRL_UNPLUGGED           4   // The control has been re-registered in another location, the cache's
                                            // instance of the control is no longer being used.

// RemoveExpiredControls flags
#define REC_SILENT     1    // If set, controls whose deletion would require confirmation are not removed.

///////////////////////////////////////////////////////////////////////////////
// FindFirstControl
//
// Purpose:
//     Initiate a search on the registry for an installed ActiveX control.
//
// Return Value:
//     - ERROR_SUCCESS if a control is found and search has been successfully
//       initiated.
//     - ERROR_NO_MORE_ITEMS if no control is found.
//     - If an error has occurred, the return value is a error code defined in
//       winerror.h
//
// Parameters:
//     hFindHandle    -- a handle needed for resuming the search.  Caller must
//                       pass this handle to FindNextControl to retrieve the
//                       the next installed ActiveX control.
//     hControlHandle -- handle to a control's data.  Caller must pass this
//                       handle into GetControlInfo to retrieve information
//                       about the control.  Call ReleaseControlHandle on the
//                       handle when done.  
//     lpCachePath    -- points to a string buffer that has the path where
//                       all controls to be retrieved are located.  If it
//                       is NULL, the internet cache path will be read
//                       from the registry.  If a path is to be supplied,
//                       the path must be a full pathname without any ~'s
//                       in order for the enumeration to work correctly.
//
#define axcFINDFIRSTCONTROL "FindFirstControl"

LONG WINAPI FindFirstControl(
                     HANDLE& hFindHandle,
                     HANDLE& hControlHandle, 
                     LPCTSTR lpszCachePath = NULL
                     );

typedef LONG (WINAPI *FINDFIRSTCONTROL)(
                     HANDLE& hFindHandle,
                     HANDLE& hControlHandle, 
                     LPCTSTR lpszCachePath = NULL
                     );


///////////////////////////////////////////////////////////////////////////////
// FindNextControl
//
// Purpose:
//     Resume a previously started search for installed ActiveX controls. The
//     search must have been initiated by a call to FirstFirstControl.
//
// Return Value:
//     - ERROR_SUCCESS if a control is found and search has been successfully
//       initiated.
//     - ERROR_NO_MORE_ITEMS if no control is found.
//     - If an error has occurred, the return value is a error code defined in
//       winerror.h.  In this situation, the caller can choose to continue
//       the search with another call to FindNextControl, or simply abort.
//
// Parameters:
//     hFindHandle    -- a handle received from a call to FindFirstControl.
//                       Pass this handle to subsequent calls to
//                       FindNextControl to retrieve controls one at a time.
//     hControlHandle -- handle to a control's data.  Caller must pass this
//                       handle into GetControlInfo to retrieve information
//                       about the control.  Call ReleaseControlHandle on the
//                       handle when done.  
//
#define axcFINDNEXTCONTROL "FindNextControl"

LONG WINAPI FindNextControl(
                     HANDLE& hFindHandle,
                     HANDLE& hControlHandle
                     );

typedef LONG (WINAPI *FINDNEXTCONTROL)(
                     HANDLE& hFindHandle,
                     HANDLE& hControlHandle
                     );

///////////////////////////////////////////////////////////////////////////////
// FindControlClose
//
// Purpose:
//     Called when search is over.  Missing a call to this function after a
//     search might contribute memory leak.  This function can be called
//     regardless of what FindFirstControl and/or FindNextControl return.
//
// Return Value:
//     None.
//
// Parameters:
//     hFindHandle -- a handle obtained from calls to FindFirstControl and
//                    FindNextControl in the current search.
//
#define axcFINDCONTROLCLOSE "FindControlClose"

void WINAPI FindControlClose(
                     HANDLE hFindHandle
                     );

typedef void (WINAPI *FINDCONTROLCLOSE)(
                     HANDLE hFindHandle
                     );


///////////////////////////////////////////////////////////////////////////////
// ReleaseControlHandle
//
// Purpose:
//     When a handle of a control is retrieved via FindFirstControl or
//     FindNextControl, the caller is responsible to release that handle
//     by call this function.
//
// Return Value:
//     None.
//
// Parameters:
//     hControlHandle -- a handle to a control obtained from FindFirstControl
//                       or FindNextControl.
//
#define axcRELEASECONTROLHANDLE "ReleaseControlHandle"

void WINAPI ReleaseControlHandle(
                          HANDLE hControlHandle
                          );

typedef void (WINAPI *RELEASECONTROLHANDLE)(
                          HANDLE hControlHandle
                          );


///////////////////////////////////////////////////////////////////////////////
// GetControlInfo
//
// Purpose:
//     Once a handle to a control is obtained via FindFirstControl or
//     FindNextControl, the caller may retrieve information about the control
//     by call this function with a flag (nFlag) indicating what info to
//     retrieve.  The supported flags are:
//     GCI_NAME       -- friendly name of control
//     GCI_FILE       -- main full path & file name of control
//     GCI_CLSID      -- clsid of control, in a NULL-terminated string
//     GCI_TYPELIBID  -- typelib guid of control, in a NULL-terminated string
//     GCI_TOTALSIZE  -- total size in bytes of all control's dependent files
//     GCI_SIZESAVED  -- total size in bytes restored if control is removed
//                       It can be different from GCI_TOTALSIZE since some
//                       of the control's dependent files might be shared dlls
//     GCI_TOTALFILES -- total number of control dependent files, including
//                       shared dlls if there are any
//     GCI_STATUS     -- the controls status value from STATUS_CTRL_* <above>
//     GCI_HAS_ACTIVEX -- non-zero if control includes ActiveX contols(s)
//     GCI_HAS_JAVA   -- non-zero if control includes Java packages
//
// Return Value:
//     TRUE if succeeded, FALSE otherwise.
//
// Parameters:
//     hControlHandle -- handle to a control for which information is to be
//                       retrieved.
//     nFlag          -- indicate which information to retrieve. Please refer
//                       to Purpose section above for a list of supported 
//                       flags.  nFlag can only equal to one of them so do
//                       not pass in multiple flags OR'ed together.
//     lpdwData       -- address of a buffer for storing a numerical value.
//                       (ie. GCI_TOTALSIZE, GCI_SIZESAVED & GCI_TOTALFILES)
//                       This parameter is ignored for other flags.
//     lpszData       -- address of a buffer for storing a NULL-terminated
//                       string value (ie. GCI_NAME, GCI_FILE, GCI_CLSID &
//                       GCI_TYPELIBID)  This paramter is ignored if other
//                       flags are specified.
//     nBufLen        -- length of string buffer pointed to by lpszData.
//                       This parameter is ignored if a numerical value is
//                       being retrieved.
//
#define axcGETCONTROLINFO "GetControlInfo"

BOOL WINAPI GetControlInfo(
                      HANDLE hControlHandle, 
                      UINT nFlag,
                      LPDWORD lpdwData,
                      LPTSTR lpszBuf,
                      int nBufLen
                      );

typedef BOOL (WINAPI *GETCONTROLINFO)(
                      HANDLE hControlHandle, 
                      UINT nFlag,
                      LPDWORD lpdwData,
                      LPTSTR lpszBuf,
                      int nBufLen
                      );


///////////////////////////////////////////////////////////////////////////////
// GetControlDependentFile
//
// Purpose:
//     A given control might depend on other files.  For instance, FOO.OCX
//     might need FOO.INF and MFCXX.DLL in order to work.  This function
//     retrieves one file at a time from a list of files depended upon by a
//     given ActiveX control.  The list of files is NOT sorted.
//
// Return Value:
//     - ERROR_SUCCESS if a file is found at position iFile in the list.
//     - ERROR_NO_MORE_FILES if no file is found at position iFile in the list.
//     - If an error has occurred, the return value is a error code defined in
//       winerror.h.
//     
// Parameters:
//     iFile          -- a zero-based index indicating which file in the list
//                       to retrieve.
//     hControlHandle -- handle to a control obtained via FindFirstControl
//                       or FindNextControl.
//     lpszFile       -- points to a buffer used to store the retrieved name.
//     lpszSize       -- points to a DWORD variable that is to store the size
//                       in bytes of the file retrieved.  If it is 0, the file
//                       does not exist.
//     bToUpper       -- TRUE if the filename returned is to be converted to
//                       uppercase.  No conversion takes place if FALSE
//          
#define axcGETCONTROLDEPENDENTFILE "GetControlDependentFile"
         
LONG WINAPI GetControlDependentFile(
             int iFile,
             HANDLE hControlHandle,
             LPTSTR lpszFile,
             LPDWORD lpdwSize,
             BOOL bToUpper = FALSE
             );

typedef LONG (WINAPI *GETCONTROLDEPENDENTFILE)(
             int iFile,
             HANDLE hControlHandle,
             LPTSTR lpszFile,
             LPDWORD lpdwSize,
             BOOL bToUpper = FALSE
             );


///////////////////////////////////////////////////////////////////////////////
// IsModuleRemovable
//
// Purpose:
//     Checks whether a file can be removed by looking into the registry.
//     This function is called "IsModuleRemovable" instead of
//     "IsFileRemovable" because this routine does not check the actual file
//     for its status.  For instance, a file can be deemed removable even if
//     is being exclusively opened by someone.  This routine only tells from
//     the registry's point of view if a file can be safely removed or not.
//
// Return Value:
//     - FALSE if there is any indication that the given file is being shared
//       by other applications.
//     - TRUE otherwise.
//
// Parameter: 
//     lpszFile -- points to a buffer that has the name (with full path) of
//                 the file whose removal status is to be verified.
//
#define axcISMODULEREMOVABLE "IsModuleRemovable"

BOOL WINAPI IsModuleRemovable(
             LPCTSTR lpszFile
             );

typedef BOOL (WINAPI *ISMODULEREMOVABLE)(
             LPCTSTR lpszFile
             );


///////////////////////////////////////////////////////////////////////////////
// RemoveControlByHandle
//
// Purpose:
//     Remove a control from registry as well as all the files that the control
//     depends on.  
//
// Return Value:
//     - S_OK if control has been successfully uninstalled.
//     - S_FALSE if minor error has occurred, but not serious enough to
//       abort the uninstallation.  Control has been uninstalled when the
//       call returns.
//     - An error code defined in winerror.h if an serious error has occurred
//       and uninstallation has been aborted.  The state of the control
//       is not gaurenteed.
//
// Parameters:
//     lpControlData -- points to an instance of CONTROL_DATA representing the
//                      control to be removed.  The struct must have been
//                      initialized by a call to FindFirstControl or
//                      FindNextControl.  Be sure to call ReleaseControlData
//                      on this struct after successful removal, for the data
//                      in this struct is no longer useful.
//     bForceRemove  -- If this flag is FALSE, the removal routine will check
//                      if the control is safe for removal before removing it.
//                      If the flag is TRUE, the control will be removed
//                      regardless of its removal status (except for Shared
//                      Violation).  The flag only applies to the control file
//                      itself.  Other files upon which the control depends are
//                      removed only if they are deemed as safe for removal.
//
#define axcREMOVECONTROLBYHANDLE "RemoveControlByHandle"

HRESULT WINAPI RemoveControlByHandle(
             HANDLE hControlHandle,
             BOOL bForceRemove = FALSE
             );

typedef HRESULT (WINAPI *REMOVECONTROLBYHANDLE)(
             HANDLE hControlHandle,
             BOOL bForceRemove = FALSE
             );


///////////////////////////////////////////////////////////////////////////////
// RemoveControlByName
//
// Purpose:
//     Remove a control from registry as well as all the files that the control
//     depends on.  It is an overloaded version.
//
// Return Value:
//     - S_OK if control has been successfully uninstalled.
//     - S_FALSE if minor error has occurred, but not serious enough to
//       abort the uninstallation.  Control has been uninstalled when the
//       call returns.
//     - An error code defined in winerror.h if an serious error has occurred
//       and uninstallation has been aborted.  The state of the control
//       is not gaurenteed.
//
// Parameters:
//     lpszFile      -- Address of a null-terminated string which is the main
//                      file for the control (ie "FOO.OCX" for FOO control).
//     lpszCLSID     -- Address of a null-terminated string which is the CLSID
//                      of the control.
//     lpszTypeLibID -- Address of a null-terminated string which is the TypeLib
//                      clsid of the control.
//     bForceRemove  -- If this flag is FALSE, the removal routine will check
//                      if the control is safe for removal before removing it.
//                      If the flag is TRUE, the control will be removed
//                      regardless of its removal status (except for Shared
//                      Violation).  The flag only applies to the control file
//                      itself.  Other files upon which the control depends are
//                      removed only if they are deemed as safe for removal.
//     dwIsDistUnit  -- boolean value to tell if this is really a dist unit
//
#define axcREMOVECONTROLBYNAME "RemoveControlByName"

HRESULT WINAPI RemoveControlByName(
             LPCTSTR lpszFile,
             LPCTSTR lpszCLSID,
             LPCTSTR lpszTypeLibID,
             BOOL bForceRemove = FALSE,
             DWORD dwIsDistUnit = FALSE
             );

typedef HRESULT (WINAPI *REMOVECONTROLBYNAME)(
             LPCTSTR lpszFile,
             LPCTSTR lpszCLSID,
             LPCTSTR lpszTypeLibID,
             BOOL bForceRemove = FALSE,
             DWORD dwIsDistUnit = FALSE
             );


///////////////////////////////////////////////////////////////////////////////
// type PFNDOBEFOREREMOVAL, used for function SweepControlsByLastAccessDate
//
// Purpose:
//     Define callback function to be called right before removing a control
//
// Return Values:
//     If a success code (S_XXX) is returned, the control will be removed.
//     If a fail code (E_XXX) is returned, the control will be skipped.
//
// Parameters:
//     HANDLE -- handle to the control to be removed.  One can get information
//               about the control using the GetControlInfo function.  Do NOT
//               call ReleaseControlHandle on the handle.
//     UINT   -- number of remaining controls including this one.
//
typedef HRESULT (CALLBACK *PFNDOBEFOREREMOVAL)(HANDLE, UINT);


///////////////////////////////////////////////////////////////////////////////
// type PFNDOAFTERREMOVAL, used for function SweepControlsByLastAccessDate
//
// Purpose:
//     Define callback function to be called right after removing a control
//
// Return Values:
//     If a success code (S_XXX) is returned, the removal operation proceeds.
//     If a fail code (E_XXX) is returned, the removal operation is aborted.
//
// Parameters:
//     HRESULT -- result of removing the control.  The handle to this control
//                was passed to the callback of type PFNDOBEFOREREMOVAL before
//                the control was removed.  The possible values for this
//                HRESULT parameter are:
//                - S_OK (succeeded)
//                - S_FALSE (control had been removed with possibly some very
//                  minor errors)
//                - E_ACCESSDENIED (control not safe for removal)
//                - STG_E_SHAREVIOLATION (control being used by others)
//                - Other errors returned by registry functions
//                It is up to the implementator of this function to decide
//                what to do given the result of removing the last control.
//     UINT    -- number of remaining controls, NOT including the one just
//                removed.
//
typedef HRESULT (CALLBACK *PFNDOAFTERREMOVAL)(HRESULT, UINT);


///////////////////////////////////////////////////////////////////////////////
// SweepControlsByLastAccessDate
//
// Purpose:
//     Remove all controls whose last access date is before and on a given
//     date.
//
// Return Value:
//     - S_OK if succeeded and at least one control was removed.
//     - S_FALSE if succeeded but no controls have been removed.
//     - E_XXX defined in winerror.h if an error has occurred.
//
// Parameters:
//     pLastAccessTime -- specify a last access date.  All controls accessed
//                        before and on this date are to be removed.  Note
//                        that all fields except wYear, wMonth and wDay are
//                        ignored.  If NULL, all control will be removed.
//     pfnDoBefore     -- callback function called just before a control is
//                        removed.  Please read the description for type
//                        PFNDOBEFOREREMOVAL for details.  If NULL, nothing
//                        is to be done prior to removing a control.
//     pfnDoAfter      -- callback function called right after a control is
//                        removed.  Please read the description for type
//                        PFNDOAFTERREMOVAL for details.  If NULL, nothing
//                        is to be done after a control is removed.
//     dwSizeLimit     -- controls will be removed only if the total size
//                        (in bytes) of all controls exceeds the size
//                        specified by this paramter.  This parameter is
//                        ignored if 0 is specified.
//
#define axcSWEEPCONTROLSBYLASTACCESSDATE "SweepControlsByLastAccessDate"

HRESULT WINAPI SweepControlsByLastAccessDate(
                              SYSTEMTIME *pLastAccessTime = NULL,
                              PFNDOBEFOREREMOVAL pfnDoBefore = NULL,
                              PFNDOAFTERREMOVAL pfnDoAfter = NULL,
                              DWORD dwSizeLimit = 0
                              );

typedef HRESULT (WINAPI *SWEEPCONTROLSBYLASTACCESSDATE)(
                              SYSTEMTIME *pLastAccessTime = NULL,
                              PFNDOBEFOREREMOVAL pfnDoBefore = NULL,
                              PFNDOAFTERREMOVAL pfnDoAfter = NULL,
                              DWORD dwSizeLimit = 0
                              );


///////////////////////////////////////////////////////////////////////////////
// RemoveExpiredControls
//
// Purpose:
//     Similar to IEmptyVolumeCache. Removes all controls with a last
//     access date in the distant past and all controls flagged for more
//     rapid auto-expire.
//
// Return Value:
//     - S_OK if succeeded and at least one control was removed.
//     - S_FALSE if succeeded but no controls have been removed.
//     - E_XXX defined in winerror.h if an error has occurred.
//
// Parameters:
//     dwFlags         -- Currently, only REC_SILENT is defined.
//     dwReserved      -- Must be 0.
//
#define axcREMOVEEXPIREDCONTROLS "RemoveExpiredControls"

HRESULT WINAPI RemoveExpiredControls(DWORD dwFlags, DWORD dwReserved);

typedef HRESULT (WINAPI *REMOVEEXPIREDCONTROLS)(DWORD dwFlags, DWORD dwReserved);

#endif
