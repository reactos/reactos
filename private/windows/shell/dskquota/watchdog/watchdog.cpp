///////////////////////////////////////////////////////////////////////////////
/*  File: watchdog.cpp

    Description: The CWatchDog class is the main control object for the
        disk quota watchdog applet.  A client merely creates a CWatchDog
        and tells it to "Run()".

            CWatchDog

        To run, the object does the following:

        1. Enumerates all local and connected volumes on the machine.
        2. For any volumes that support quotas, quota statistics are 
           gathered for both the volume and the local user on the volume.
           Statistics are maintained in a list of CStatistics objects; one
           object for each volume/user pair.
        3. Once all information has been gathered, a list of action objects
           (CActionEmail, CActionPopup) are created.  System policy and 
           previous notification history are considered when creating action
           objects.
        4. When all action objects are created, they are performed.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include <precomp.hxx>
#pragma hdrstop

#include "action.h"
#include "watchdog.h"
#include "resource.h"

//
// Required so we can create auto_ptr<BYTE>
//
struct Byte
{
    BYTE b;
};


CWatchDog::CWatchDog(
    HANDLE htokenUser
    ) : m_htokenUser(htokenUser),
        m_history(m_policy)
{
    //
    // Nothing else to do.
    //
}


CWatchDog::~CWatchDog(
    VOID
    )
{
    ClearActionList();
}


//
// This is the function that does it all!
// Just create a watchdog object and tell it to run.
//
HRESULT
CWatchDog::Run(
    VOID
    )
{
    HRESULT hr = NOERROR;

    //
    // First check to see if we should do anything based upon the user's
    // notification history stored in the registry and the notification
    // policy defined by the system administrator.  If we've recently
    // displayed a popup or sent email, or if the system policy says
    // "don't send email or display a popup", we won't go any further.
    // No sense doing anything expensive if we don't need to.
    //
    if (ShouldDoAnyNotifications())
    {
        shauto_ptr<ITEMIDLIST> ptrIdlDrives;
        oleauto_ptr<IShellFolder> ptrDesktop;
        //
        // Bind to the desktop folder.
        //
        hr = SHGetDesktopFolder(ptrDesktop._getoutptr());
        if (SUCCEEDED(hr))
        {
            hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, ptrIdlDrives._getoutptr());
            if (SUCCEEDED(hr))
            {
                //
                // Bind to the "Drives" folder.
                //
                oleauto_ptr<IShellFolder> ptrDrives;
                hr = ptrDesktop->BindToObject(ptrIdlDrives, NULL, IID_IShellFolder, (LPVOID *)ptrDrives._getoutptr());
                if (SUCCEEDED(hr))
                {
                    //
                    // Gather quota statistics for local and connected drives.
                    //
                    hr = GatherQuotaStatistics(ptrDrives);
                    if (SUCCEEDED(hr) && (0 < m_statsList.Count()))
                    {
                        //
                        // Build up the list of actions required.
                        //
                        hr = BuildActionList();
                        if (SUCCEEDED(hr) && (0 < m_actionList.Count()))
                        {
                            //
                            // We have a list of actions.  Do them.
                            //
                            hr = DoActions();
                        }
                    }
                }
            }
        }
    }
    return hr;
}


//
// Gather up all of the quota statistics for the volumes in the shell's "Drives" 
// folder and the quota statistics on those volumes for the current user if
// applicable.  
//
HRESULT
CWatchDog::GatherQuotaStatistics(
    IShellFolder *psfDrives
    )
{
    HRESULT hr;
    oleauto_ptr<IEnumIDList> ptrEnum;

    //
    // Enumerate all of the non-folder objects in the drives folder.
    //
    hr = psfDrives->EnumObjects(NULL, SHCONTF_NONFOLDERS, ptrEnum._getoutptr());
    if (SUCCEEDED(hr))
    {
        //
        // Convert the user's token to a SID.
        //
        auto_ptr<Byte> ptrSid = (Byte *)GetUserSid(m_htokenUser);
        if (NULL != (Byte *)ptrSid)
        {
            shauto_ptr<ITEMIDLIST> ptrIdlItem;
            ULONG ulFetched = 0;
            //
            // For each item in the drives folder...
            //
            while(S_OK == ptrEnum->Next(1, ptrIdlItem._getoutptr(), &ulFetched))
            {
                TCHAR szName[MAX_PATH];
                TCHAR szDisplayName[MAX_PATH];
                StrRet strretName((LPITEMIDLIST)ptrIdlItem);
                StrRet strretDisplayName((LPITEMIDLIST)ptrIdlItem);

                //
                // Get the non-display name form; i.e. "G:\"
                //
                hr = psfDrives->GetDisplayNameOf(ptrIdlItem, SHGDN_FORPARSING, &strretName);
                if (SUCCEEDED(hr))
                {
                    strretName.GetString(szName, ARRAYSIZE(szName));
                    //
                    // Protect against getting something like "Dial-up Networking"
                    // and thinking it's drive 'D'.
                    //
                    if (TEXT(':') == szName[1])
                    {
                        //
                        // Get the display name form; i.e. "My Disk (G:)"
                        //
                        psfDrives->GetDisplayNameOf(ptrIdlItem, SHGDN_NORMAL, &strretDisplayName);
                        strretDisplayName.GetString(szDisplayName, ARRAYSIZE(szDisplayName));
                   
                        //
                        // Add the volume/user pair to the statistics list.
                        // This function does a lot of things.
                        //   1. Checks to see if volume supports quotas.
                        //   2. Gets volume quota information.
                        //   3. Gets user quota information for this volume.
                        //
                        // If something fails or the volume doesn't support quotas,
                        // the entry is not added to the list and will therefore not
                        // participate in any quota notification actions.
                        //
                        m_statsList.AddEntry(szName[0], szDisplayName, (LPBYTE)((Byte *)ptrSid));
                    }
                }
            }
        }
    }
    return hr;
}                                


//
// Build up the list of actions from our list of volume/user statistics.
// Note that it's important we build the email actions before the popup
// dialog actions.  Actions are appended to the action list.  The popup
// dialog used in popup actions may be a modal dialog which will block
// the call to DoAction() until the user responds.  If the user is away
// from the computer for a while, this will prevent any pending email
// notifications from going out.  Therefore, we always want to get the
// email notifications out of the way before we handle any notifications
// that might require user intervention.
//
HRESULT
CWatchDog::BuildActionList(
    VOID
    )
{
    HRESULT hr = NOERROR;

    //
    // Clear out any previous action objects.
    //
    ClearActionList();

    if (ShouldSendEmail())
    {
        //
        // Build up any actions requiring email.
        // Important that email comes before popup dialogs in action list.
        // See note in function header for details.
        //
        BuildEmailActions();
    }

    if (ShouldPopupDialog())
    {
        //
        // Build up any actions requiring popup dialogs.
        //
        BuildPopupDialogActions();
    }

    return hr;
}


//
// Build up all of the required email messages, appending them to the action
// list.  This is where all of the email message formatting is done.
// Currently, it's all done in this one function.  If more sophisticated
// email is required at a later date, this function may need to be
// divided into more functions with this as the central entry point.
//
HRESULT
CWatchDog::BuildEmailActions(
    VOID
    )
{
    HRESULT hr = NOERROR;
    INT cStatsEntries = m_statsList.Count();

    if (0 < cStatsEntries)
    {
        //
        // Now we know we'll need MAPI.  Go ahead and initialize the
        // session.  This will:
        //   1. Load MAPI32.DLL.
        //   2. Logon to MAPI creating a MAPI session.
        //   3. Obtain a pointer to the outbox.
        //
        // Note that MAPI will be uninitialized and MAPI32.DLL will be
        // unloaded when the session object is destroyed.
        //
        hr = m_mapiSession.Initialize();
        if (SUCCEEDED(hr))
        {
            //
            // Get the outbox object interface pointer.
            //
            LPMAPIFOLDER pOutBoxFolder;
            hr = m_mapiSession.GetOutBoxFolder(&pOutBoxFolder);
            if (SUCCEEDED(hr))       
            {
                //
                // OK.  Now we have loaded MAPI, created a MAPI session
                // and we have a pointer to the outbox folder.  Now we can
                // create messages and send them.
                //

                //
                // These are the pieces of the text message.
                //
                CMapiMessageBody body;    // Message body text.

                //
                // Add the canned header to the email message.
                //
                body.Append(g_hInstDll, IDS_EMAIL_HEADER);

                //
                // Subject line contains the computer name.
                //
                CString strComputerName;
                DWORD cchComputerName = MAX_COMPUTERNAME_LENGTH + 1;
                GetComputerName(strComputerName.GetBuffer(cchComputerName), 
                                &cchComputerName);

                CString strSubject(g_hInstDll, IDS_EMAIL_SUBJECT_LINE, (LPCTSTR)strComputerName);
                CString strVolume(g_hInstDll, IDS_EMAIL_VOLUME);
                CString strWarning(g_hInstDll, IDS_EMAIL_WARNING);
                CString strLimit(g_hInstDll, IDS_EMAIL_LIMIT);
                CString strUsed(g_hInstDll, IDS_EMAIL_USED);

                INT cEmailMsgEntries = 0;
                //
                // Enumerate all of the entries in the statistics list.
                // Each entry contains information for a volume/user pair.
                // 
                for (INT i = 0; i < cStatsEntries && SUCCEEDED(hr); i++)
                {
                    //
                    // Retrieve the next entry from the list.
                    //
                    const CStatistics *pStats = m_statsList.GetEntry(i);
                    Assert(NULL != pStats);
                    //
                    // Do these stats require reporting?  Some reasons
                    // why not...
                    //    1. Volume doesn't support quotas 
                    //           (non-NTFS, non-NTFS 5.0)
                    //    2. No access to volume.  Can't open it.
                    //    3. "Warn user over threshold" bit on the volume
                    //       is not set.
                    //    4. User's quota usage is below threshold value.
                    // 
                    if (pStats->IncludeInReport())
                    {
                        //
                        // Now create the message text for the volume 
                        // in this statistics object.
                        //
                        TCHAR szBytes[40];
                        TCHAR szBytesOver[40];

                        //
                        // "Location: MyShare on MyDisk (C:)"
                        //
                        LPCTSTR pszVolDisplayName = pStats->GetVolumeDisplayName() ?
                                                    pStats->GetVolumeDisplayName() :
                                                    TEXT("");
                        body.Append(g_hInstDll, 
                                    IDS_EMAIL_FMT_VOLUME, 
                                    (LPCTSTR)strVolume,
                                    pszVolDisplayName);

                        //
                        // "Quota Used: 91MB (1MB over warning)"
                        //
                        XBytes::FormatByteCountForDisplay(pStats->GetUserQuotaUsed().QuadPart,
                                                          szBytes, ARRAYSIZE(szBytes));

                        //
                        // Format and add the "1MB" part of "(1MB over warning)".  
                        //
                        __int64 diff = pStats->GetUserQuotaUsed().QuadPart - pStats->GetUserQuotaThreshold().QuadPart;
                        if (0 > diff)
                        {
                            diff = 0;
                        }

                        XBytes::FormatByteCountForDisplay(diff, szBytesOver, ARRAYSIZE(szBytesOver));

                        body.Append(g_hInstDll,
                                    IDS_EMAIL_FMT_USED, 
                                    (LPCTSTR)strUsed, 
                                    szBytes,
                                    szBytesOver);
                        //
                        // "Warning Level: 90MB"
                        //
                        XBytes::FormatByteCountForDisplay(pStats->GetUserQuotaThreshold().QuadPart,
                                                          szBytes, ARRAYSIZE(szBytes));
                        body.Append(g_hInstDll,
                                    IDS_EMAIL_FMT_WARNING, 
                                    (LPCTSTR)strWarning, 
                                    szBytes);

                        //
                        // "Quota Limit: 100MB"
                        //
                        XBytes::FormatByteCountForDisplay(pStats->GetUserQuotaLimit().QuadPart,
                                                          szBytes, ARRAYSIZE(szBytes));
                        body.Append(g_hInstDll,
                                    IDS_EMAIL_FMT_LIMIT, 
                                    (LPCTSTR)strLimit, 
                                    szBytes);

                        if (pStats->DenyAtLimit())
                        {
                            //
                            // Volume is set to deny writes at the quota limit.
                            // Add a reminder of this fact.
                            //
                            body.Append(g_hInstDll, IDS_EMAIL_DENY_AT_LIMIT);
                        }
                        //
                        // Keep track of how many volume entries we've added to
                        // the message.  If the count is 0 when we're done, no sense
                        // in creating/sending the message.
                        //
                        cEmailMsgEntries++;
                    }
                }
                if (0 < cEmailMsgEntries)
                {
                    //
                    // Create an email action object from the text
                    // we've formatted above.
                    //
                    CAction *pAction = new CActionEmail(m_mapiSession,
                                                        pOutBoxFolder,
                                                        m_policy.GetOtherEmailTo(),
                                                        m_policy.GetOtherEmailCc(),
                                                        m_policy.GetOtherEmailBcc(),
                                                        strSubject,
                                                        body);
                    if (NULL != pAction)
                    {
                        //
                        // Add the action object to the list of action objects.
                        //
                        m_actionList.Append(pAction);
                    }
                }
                pOutBoxFolder->Release();
            }
            else
            {
                DebugMsg(DM_ERROR, TEXT("CWatchDog::BuildEmailActions: Error 0x%08X getting MAPI outbox"), hr);
            }
        }
        else
        {
            DebugMsg(DM_ERROR, TEXT("CWatchDog::BuildEmailActions: Error 0x%08X initializing MAPI session"), hr);
        }
    }
    return hr;
}



//
// Build up all of the required popup actions, appending them to the action
// list.  
//
HRESULT
CWatchDog::BuildPopupDialogActions(
    VOID
    )
{
    HRESULT hr = NOERROR;
    //
    // Create a dialog popup action object.
    // It uses the statistics list to fill it's listview
    // control.  Other than that, it's fully self-contained.
    // Note however that we're just passing a reference
    // to the statistics list and the CActionPopup object doesn't
    // create a copy of the object.  Therefore, the statistics
    // list object MUST live longer than the popup object.
    // This is a safe assumption in the current design.
    //
    CAction *pAction = new CActionPopup(m_statsList);

    if (NULL != pAction)
    {
        //
        // Add the action object to the list of action objects.
        //
        m_actionList.Append(pAction);
    }
    return hr;
}

//
// Remove all actions from the action list.
//
VOID
CWatchDog::ClearActionList(
    VOID
    )
{
    INT cActions = m_actionList.Count();
    for (INT i = 0; i < cActions; i++)
    {
        delete m_actionList[i];
    }
    m_actionList.Clear();
}


//
// Perform the actions in the action list.
//
HRESULT
CWatchDog::DoActions(
    VOID
    )
{
    HRESULT hr = NOERROR;

    INT cActions = m_actionList.Count();
    
    //
    // For each action in the action list...
    //
    for (INT i = 0; i < cActions; i++)
    {
        Assert(NULL != m_actionList[i]);
        //
        // Do the action.
        // Pass in a reference to our CHistory object so that the
        // action object can update the history record appropriately.
        //
        hr = m_actionList[i]->DoAction(m_history);
    }
    return hr;
}


//
// Convert a user's token handle into a SID.
// Caller is responsible for calling delete[] on the returned buffer when
// done with it.
//
LPBYTE
CWatchDog::GetUserSid(
    HANDLE htokenUser
    )
{
    LPBYTE pbUserSid = NULL;
    BOOL bResult     = FALSE;

    if (NULL != htokenUser)
    {
        //
        // Note the use of "Byte" and not "BYTE".  auto_ptr<> requires
        // a class, struct or union.  No scalar types.
        //
        auto_ptr<Byte> ptrUserToken = NULL;
        DWORD cbUserToken = 256;

        //
        // Start with a 256-byte buffer.
        //
        ptrUserToken = new Byte[cbUserToken];

        if (NULL != (Byte *)ptrUserToken)
        {
            //
            // Get the user's token information from the system.
            //
            DWORD cbUserTokenReqd;
            bResult = ::GetTokenInformation(htokenUser,
                                            TokenUser,
                                            ptrUserToken,
                                            cbUserToken,
                                            &cbUserTokenReqd);

            if (!bResult && (cbUserTokenReqd > cbUserToken))
            {
                //
                // Buffer wasn't large enough.  Try again
                // with the size value returned from the previous call.
                // ptrUserToken is an auto_ptr so the original buffer
                // is automagically deleted.
                //
                ptrUserToken = new Byte[cbUserTokenReqd];
                if (NULL != (Byte *)ptrUserToken)
                {
                    bResult = ::GetTokenInformation(htokenUser,
                                                    TokenUser,
                                                    ptrUserToken,
                                                    cbUserTokenReqd,
                                                    &cbUserTokenReqd);
                }
            }
        }
        if (bResult)
        {
            TOKEN_USER *pToken = (TOKEN_USER *)((Byte *)ptrUserToken);
            if (::IsValidSid(pToken->User.Sid))
            {
                //
                // If the SID is valid, create a new buffer and copy
                // the SID bytes to it.  We'll return the new buffer
                // to the caller.
                //
                INT cbUserSid = ::GetLengthSid(pToken->User.Sid);
            
                pbUserSid = new BYTE[cbUserSid];
                if (NULL != pbUserSid)
                {
                    ::CopySid(cbUserSid, pbUserSid, pToken->User.Sid);
                }
            }
            else
            {
                DebugMsg(DM_ERROR, TEXT("CWatchDog::GetUserSid - Invalid SID for user token 0x%08X"), htokenUser);
            }
        }
        else
        {
            DebugMsg(DM_ERROR, TEXT("CWatchDog::GetUserSid - GetTokenInformation failed with error 0x%08X"), GetLastError());
        }
    }        
    else
    {
        DebugMsg(DM_ERROR, TEXT("CWatchDog::GetUserSid - OpenThreadToken failed with error 0x%08X"), GetLastError());
    }
    return pbUserSid;
}

//
// Should we send email based on policy/history?
//
BOOL 
CWatchDog::ShouldSendEmail(
    VOID
    )
{
    return m_policy.ShouldSendEmail() &&
           m_history.ShouldSendEmail();
}


//
// Should we show a popup based on policy/history?
//
BOOL
CWatchDog::ShouldPopupDialog(
    VOID
    )
{
    return m_policy.ShouldPopupDialog() &&
           m_history.ShouldPopupDialog();
}

//
// Should we do anything based on policy/history?
//
BOOL
CWatchDog::ShouldDoAnyNotifications(
    VOID
    )
{
    return m_policy.ShouldDoAnyNotifications() &&
           m_history.ShouldDoAnyNotifications();
}
