// USERLIST.cpp : implementation file
//

#include "stdafx.h"
#include "EFSADU.h"
#include "USERLIST.h"
#include "ADDSHEET.h"
#include <winefs.h>

extern "C" {
void __stdcall EfsDetail(LPCTSTR FileName);
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// USERLIST dialog


USERLIST::USERLIST(CWnd* pParent /*=NULL*/)
	: CDialog(USERLIST::IDD, pParent)
{
	//{{AFX_DATA_INIT(USERLIST)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

USERLIST::USERLIST(LPCTSTR FileName, CWnd* pParent /*=NULL*/)
	: CDialog(USERLIST::IDD, pParent)
{
    m_FileName = FileName;
}


void USERLIST::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(USERLIST)
	DDX_Control(pDX, IDC_ADD, m_AddButton);
	DDX_Control(pDX, IDC_REMOVE, m_RemoveButton);
	DDX_Control(pDX, IDC_USERLIST, m_UserList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(USERLIST, CDialog)
	//{{AFX_MSG_MAP(USERLIST)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_USERLIST, OnItemchangedUserlist)
	ON_NOTIFY(NM_KILLFOCUS, IDC_USERLIST, OnKillfocusUserlist)
	ON_NOTIFY(NM_SETFOCUS, IDC_USERLIST, OnSetfocusUserlist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// USERLIST message handlers

void USERLIST::OnAdd()
{
    CString WinTitle;

    AfxFormatString1( WinTitle, IDS_ADDTITLE, m_FileName );
	CAddSheet EfsAddSheet(WinTitle, this);

    EfsAddSheet.SetWizardMode();
	INT_PTR nResponse = EfsAddSheet.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	
}

void USERLIST::OnRemove()
{
	int ItemPos;
    BOOL    NoAction = FALSE;
    CString UnKnownUser;
    CString NoCertName;

    try{
        UnKnownUser.LoadString(IDS_UNKNOWNUSER);
        NoCertName.LoadString(IDS_NOCERTNAME);
    }
    catch(...){
        NoAction = TRUE;
    }

    if (NoAction){
        return;
    }

	ItemPos = m_UserList.GetNextItem( -1, LVNI_SELECTED );
    while ( ItemPos != -1 ){

        CString UserName;
        CString CertName;
        LPTSTR  pUserName;
        LPTSTR  pCertName;


        UserName = m_UserList.GetItemText( ItemPos, 0 );
        CertName = m_UserList.GetItemText( ItemPos, 1 );
        if ( !UserName.Compare(UnKnownUser) ){
            pUserName = NULL;
        } else {
            pUserName = UserName.GetBuffer(UserName.GetLength() + 1);
        }

        if (!CertName.Compare(NoCertName)){
            pCertName = NULL;
        } else {
            pCertName = CertName.GetBuffer(CertName.GetLength() + 1);
        }
        m_Users.Remove( pUserName, pCertName );
        m_UserList.DeleteItem( ItemPos );
        if (pUserName){
            UserName.ReleaseBuffer();
        }
        if (pCertName){
            CertName.ReleaseBuffer();
        }

        //
        // Because we have deleted the item. We have to start from -1 again.
        //

        ItemPos = m_UserList.GetNextItem( -1, LVNI_SELECTED );

    }
}

void USERLIST::OnCancel()
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void USERLIST::OnOK()
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}


void  __stdcall EfsDetail(LPCTSTR FileName)
{
    INT_PTR RetCode;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    USERLIST DetailDialog(FileName);

    RetCode = DetailDialog.DoModal();
    if ( IDOK == RetCode ){

        //
        // Commit the change
        //

        DetailDialog.ApplyChanges( FileName );

    } else if (IDCANCEL == RetCode) {

        //
        // Nothing needs to be done
        //

    }

}

BOOL USERLIST::OnInitDialog()
{
	CDialog::OnInitDialog();
    RECT ListRect;
    DWORD   ColWidth;
    CString ColName;
    CString ColDN;
    CString WinTitle;
    LPTSTR   UName = NULL;
    LPTSTR  DomainName = NULL;
    LPTSTR  UserNt4Name = NULL;
    LPTSTR  UserCertName = NULL;
    BOOL    CheckDS = FALSE;
    DS_NAME_RESULT* UserName = NULL;
    HANDLE  hDS = NULL;
    BOOL    EnableAddButton = FALSE;
    PENCRYPTION_CERTIFICATE_HASH_LIST pUsers = NULL;

    try {

        DWORD RetCode;

        AfxFormatString1( WinTitle, IDS_DETAILWINTITLE, m_FileName );
        SetWindowText( WinTitle ); 	
	    m_UserList.GetClientRect(&ListRect);
        ColName.LoadString(IDS_USERCOLTITLE);
        ColDN.LoadString(IDS_DNCOLTITLE);
        ColWidth = ( ListRect.right - ListRect.left  ) / 2;
        m_UserList.InsertColumn(0, ColName, LVCFMT_LEFT, ColWidth );
        m_UserList.InsertColumn(1, ColDN, LVCFMT_LEFT, ColWidth );
        RetCode = QueryUsersOnEncryptedFile( (LPWSTR)(LPCWSTR) m_FileName, &pUsers);
        if ( !RetCode ){

            //
            // Got the info about the encrypted file
            //

            SID_NAME_USE    peUse;
            PSID    USid;
            DWORD   CharsName;
            DWORD   CharsDomainName;

            RetCode = DsBind(NULL, NULL, &hDS);
            if ( RetCode == NO_ERROR ){

                //
                //  We are going to use the DS to crack the names
                //

                CheckDS = TRUE;
            }

            DWORD   NUsers = pUsers->nCert_Hash;

            UName  = new TCHAR[UNLEN];
            DomainName = new TCHAR[DNS_MAX_NAME_LENGTH];
            if ( (NULL == UName) || (NULL == DomainName) ){
                AfxThrowMemoryException( );
            }
            CharsName  = UNLEN;
            CharsDomainName = DNS_MAX_NAME_LENGTH;

            //
            // Get all the users
            //

            while ( NUsers > 0 ){

                USid = NULL;

                //
                // Check too see if the SID is available or not
                //
                if ( pUsers->pUsers[NUsers - 1]->pUserSid){

                    //
                    // Get the user name from the SID
                    //

                    BOOL    bRet;
                    DWORD   RequiredCharsName;
                    DWORD   RequiredCharsDomainName;

                    USid = (PSID) (pUsers->pUsers[NUsers - 1]->pUserSid);

                    RequiredCharsName = CharsName;
                    RequiredCharsDomainName = CharsDomainName;
                    bRet = LookupAccountSid(
                                        NULL,
                                        USid,
                                        UName,
                                        &RequiredCharsName,
                                        DomainName,
                                        &RequiredCharsDomainName,
                                        &peUse
                                 );
                    if ( !bRet ){
                        if ( ERROR_INSUFFICIENT_BUFFER == GetLastError() ){

                            BOOL    TryAgain = FALSE;

                            if ( CharsName < RequiredCharsName ){
                                CharsName = RequiredCharsName;
                                delete [] UName;
                                UName = new TCHAR[CharsName];
                                if ( NULL == UName ){
                                    AfxThrowMemoryException( );
                                }
                                TryAgain = TRUE;
                            }
                            if ( CharsDomainName < RequiredCharsDomainName ){
                                CharsDomainName = RequiredCharsDomainName;
                                delete [] DomainName;
                                DomainName = new TCHAR[CharsName];
                                if ( NULL == DomainName ){
                                    AfxThrowMemoryException( );
                                }
                                TryAgain = TRUE;
                            }
                            if ( TryAgain ) {

                                //
                                // Try again with larger buffer
                                //

                                continue;
                            }
                        }
                        //
                        // This user's information is invalid. Try Next One
                        //

                        NUsers--;
                        continue;

                    }

                    //
                    // Got the information about the user. Get the right format for the user name
                    //

                    DWORD UserNt4NameLen;
                    BOOL    UserNameGot = FALSE;

                    UserNt4NameLen = _tcslen(UName) +
                                                    _tcslen(DomainName) +
                                                    2 ;

                    UserNt4Name = new TCHAR[ UserNt4NameLen ];
                    if ( NULL == UserNt4Name ){
                        AfxThrowMemoryException( );
                    }
                    _tcscpy(UserNt4Name, DomainName);
                    _tcscat(UserNt4Name, _T("\\"));
                    _tcscat(UserNt4Name, UName);

                    if ( CheckDS ){

                        RetCode = DsCrackNames(
                                                hDS,
                                                DS_NAME_NO_FLAGS,
                                                DS_NT4_ACCOUNT_NAME,
                                                DS_USER_PRINCIPAL_NAME,
                                                1,
                                                &UserNt4Name,
                                                &UserName
                                                );

                        if ( NO_ERROR == RetCode ){
                            if (( UserName->cItems > 0 ) && (DS_NAME_NO_ERROR == UserName->rItems[0].status)){

                                    //
                                    // We got our name in the DS, repack it
                                    //

                                    DWORD DsNameLen = _tcslen(UserName->rItems[0].pName) + 1;
                                    if ( DsNameLen > UserNt4NameLen){
                                        delete [] UserNt4Name;
                                        UserNt4Name = NULL;
                                        UserNt4Name = new TCHAR[ DsNameLen ];
                                        if ( NULL == UserNt4Name ){
                                            AfxThrowMemoryException( );
                                        }
                                        UserNt4NameLen = DsNameLen;
                                    }

                                    _tcscpy( UserNt4Name, UserName->rItems[0].pName);
                                    UserNameGot = TRUE;

                            }

                        }

                    }

                } else {

                    //
                    // BUGBUG, SID is not available. Just in case.
                    //
                    DWORD UserCertNameLen= 0;

                    UserNt4Name = NULL;

                }

                //
                // Get the display name to the end of the name.
                //

                UserCertName = new TCHAR[_tcslen(pUsers->pUsers[NUsers - 1]->lpDisplayInformation) + 1];
                if (UserCertName){
                    _tcscpy(UserCertName, pUsers->pUsers[NUsers - 1]->lpDisplayInformation);
                } else {
                    AfxThrowMemoryException( );
                }

                //
                // We got the user name
                //
                RetCode = m_Users.Add(
                                        UserNt4Name,
                                        UserCertName,
                                        pUsers->pUsers[NUsers - 1]->pHash,
                                        USid
                                        );

                if ( (NO_ERROR != RetCode) && (CRYPT_E_EXISTS  != RetCode) ) {
                    delete [] UserNt4Name;
                    UserNt4Name = NULL;
                    delete [] UserCertName;
                    UserCertName = NULL;
                } else if ( CRYPT_E_EXISTS  == RetCode ){
                    UserNt4Name = NULL;
                    UserCertName = NULL;
                }

                NUsers--;
            }

            delete [] UName;
            delete [] DomainName;
            UName = NULL;
            DomainName = NULL;

            if ( pUsers ){
                FreeEncryptionCertificateHashList( pUsers );
                pUsers = NULL;
            }
            //
            // Unbind DS
            //

            if ( CheckDS ){
                DsUnBindW( &hDS );
            }

            //
            // In memory intial list established
            //

            SetUpListBox(&EnableAddButton);

        } else {
            CString ErrMsg;

            if (ErrMsg.LoadString(IDS_NOINFO)){
                MessageBox(ErrMsg);
            }
        }

    }
     catch (...) {
        //
        // The exception mostly is caused by out of memory.
        // We can not prevent the page to be displayed from this routine,
        // So we just go ahead with empty list
        //

        m_UserList.DeleteAllItems( );

        //
        // Delete works even if UName == NULL
        //

        delete [] UName;
        delete [] DomainName;
        delete [] UserNt4Name;
        delete [] UserCertName;
        if ( pUsers ){
            FreeEncryptionCertificateHashList( pUsers );
        }

        if ( CheckDS ){
            if (UserName){
                DsFreeNameResult(UserName);
                UserName = NULL;
            }
            DsUnBindW( &hDS );
        }


    }

    m_RemoveButton.EnableWindow( FALSE );
    if ( !EnableAddButton ){
        m_AddButton.EnableWindow( FALSE );
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void USERLIST::OnItemchangedUserlist(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
    ShowRemove();

	*pResult = 0;
}

void USERLIST::OnKillfocusUserlist(NMHDR* pNMHDR, LRESULT* pResult)
{

    ShowRemove();
	
	*pResult = 0;
}

void USERLIST::OnSetfocusUserlist(NMHDR* pNMHDR, LRESULT* pResult)
{
    ShowRemove();
	
	*pResult = 0;
}

void USERLIST::ShowRemove()
{
    if (m_UserList.GetSelectedCount() > 0){

        //
        // Enable the Remove Button
        //

        m_RemoveButton.EnableWindow( TRUE );

    } else {

        //
        // Disable the Remove Button
        //

        m_RemoveButton.EnableWindow( FALSE );

    }
}

DWORD
USERLIST::ApplyChanges(
    LPCTSTR FileName
    )
{
    DWORD RetCode = NO_ERROR;
    DWORD NoUsersToRemove;
    DWORD NoUsersToAdd;
    DWORD RemoveUserIndex;
    DWORD AddUserIndex;
    PENCRYPTION_CERTIFICATE_HASH_LIST RemoveUserList = NULL;
    PENCRYPTION_CERTIFICATE_LIST AddUserList = NULL;
    PVOID   EnumHandle;


    //
    //  Get all the users to be added or removed first
    //

    NoUsersToAdd =  m_Users.GetUserAddedCnt();
    NoUsersToRemove = m_Users.GetUserRemovedCnt();

    if ( (NoUsersToAdd == 0) && (NoUsersToRemove == 0)){
        return NO_ERROR;
    }

    if ( NoUsersToAdd ) {

        //
        // At least one user is to be added
        //

        DWORD   BytesToAllocate;

        BytesToAllocate = sizeof ( ENCRYPTION_CERTIFICATE_LIST ) +
                                       NoUsersToAdd  * sizeof ( PENCRYPTION_CERTIFICATE ) +
                                       NoUsersToAdd * sizeof (ENCRYPTION_CERTIFICATE);
        AddUserList = (PENCRYPTION_CERTIFICATE_LIST) new BYTE[BytesToAllocate];
        if ( NULL == AddUserList ){

            //
            // Out of memory. Try our best to display the error message.
            //

            try {

                CString ErrMsg;

                if (ErrMsg.LoadString(IDS_ERRORMEM)){
                    ::MessageBox(NULL, ErrMsg, NULL, MB_OK);
                }
            }
            catch (...) {
            }

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        AddUserList->nUsers = NoUsersToAdd;
        AddUserList->pUsers = (PENCRYPTION_CERTIFICATE *)(((PBYTE) AddUserList) +
                    sizeof ( ENCRYPTION_CERTIFICATE_LIST ));
    }

    if ( NoUsersToRemove ){

            //
            // At least one user is to be removed
            //

        DWORD   BytesToAllocate;

        BytesToAllocate = sizeof ( ENCRYPTION_CERTIFICATE_HASH_LIST ) +
                                       NoUsersToRemove  * sizeof ( PENCRYPTION_CERTIFICATE_HASH) +
                                       NoUsersToRemove * sizeof (ENCRYPTION_CERTIFICATE_HASH);


        RemoveUserList = (PENCRYPTION_CERTIFICATE_HASH_LIST) new BYTE[BytesToAllocate];
        if ( NULL == RemoveUserList ){

            //
            // Out of memory. Try our best to display the error message.
            //

            if (AddUserList){
                delete [] AddUserList;
                AddUserList = NULL;
            }

            try {

                CString ErrMsg;

                if (ErrMsg.LoadString(IDS_ERRORMEM)){
                    ::MessageBox(NULL, ErrMsg, NULL, MB_OK);
                }
            }
            catch (...) {
            }

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RemoveUserList->nCert_Hash = NoUsersToRemove;
        RemoveUserList->pUsers =  (PENCRYPTION_CERTIFICATE_HASH *)(((PBYTE) RemoveUserList) +
                    sizeof ( ENCRYPTION_CERTIFICATE_HASH_LIST ));
    }

    EnumHandle = m_Users.StartEnum();
    RemoveUserIndex = 0;
    AddUserIndex = 0;
    while ( EnumHandle ){

        DWORD   Flag;
        PSID         UserSid;
        PVOID      CertData;
        LPTSTR   UserName;
        LPTSTR   CertName;

        EnumHandle = m_Users.GetNextChangedUser(
                                    EnumHandle,
                                    &UserName,
                                    &CertName,
                                    &UserSid,
                                    &CertData,
                                    &Flag
                                    );

        if ( Flag ){

            //
            // We get our changed user
            //

            if ( Flag & USERADDED ){

                ASSERT( AddUserList );

                //
                // Add the user to the adding list
                //

                PENCRYPTION_CERTIFICATE   EfsCert;

                ASSERT (AddUserIndex < NoUsersToAdd);

                EfsCert= (PENCRYPTION_CERTIFICATE)(((PBYTE) AddUserList) +
                            sizeof ( ENCRYPTION_CERTIFICATE_LIST ) +
                            NoUsersToAdd  * sizeof ( PENCRYPTION_CERTIFICATE) +
                            AddUserIndex * sizeof (ENCRYPTION_CERTIFICATE));

                AddUserList->pUsers[AddUserIndex] = EfsCert;
                EfsCert->pUserSid =  (SID *) UserSid;
                EfsCert->cbTotalLength = sizeof (ENCRYPTION_CERTIFICATE);
                EfsCert->pCertBlob = (PEFS_CERTIFICATE_BLOB) CertData;

                AddUserIndex++;

            } else if ( Flag & USERREMOVED ) {

                ASSERT (RemoveUserList);

                //
                // Add the user to the removing list
                //

                PENCRYPTION_CERTIFICATE_HASH    EfsCertHash;

                ASSERT (RemoveUserIndex < NoUsersToRemove);

                EfsCertHash= (PENCRYPTION_CERTIFICATE_HASH)(((PBYTE) RemoveUserList) +
                            sizeof ( ENCRYPTION_CERTIFICATE_HASH_LIST ) +
                            NoUsersToRemove   * sizeof ( PENCRYPTION_CERTIFICATE_HASH) +
                            RemoveUserIndex * sizeof (ENCRYPTION_CERTIFICATE_HASH));

                RemoveUserList->pUsers[RemoveUserIndex] = EfsCertHash;
                EfsCertHash->cbTotalLength = sizeof (ENCRYPTION_CERTIFICATE_HASH);
                EfsCertHash->pUserSid = (SID *)UserSid;
                EfsCertHash->pHash = (PEFS_HASH_BLOB) CertData;
                EfsCertHash->lpDisplayInformation = NULL;

                RemoveUserIndex++;
            } else {
                ASSERT(FALSE);
            }

        }

    }

    ASSERT(RemoveUserIndex == NoUsersToRemove);
    ASSERT(AddUserIndex == NoUsersToAdd);

    if ( AddUserIndex && AddUserList ){

        //
        // Add the user to the file list
        //

        RetCode = AddUsersToEncryptedFile(FileName, AddUserList);
        if ( NO_ERROR != RetCode ){

            CString ErrMsg;
            TCHAR   ErrCode[16];

            _ltot(RetCode, ErrCode, 10 );
            AfxFormatString1( ErrMsg, IDS_ADDUSERERR, ErrCode );
            MessageBox(ErrMsg);

        }

    }

    if ( RemoveUserIndex && RemoveUserList){

        //
        // Remove the user from the list
        //

        DWORD RetCodeBak = RetCode;

        RetCode = RemoveUsersFromEncryptedFile(FileName, RemoveUserList);
        if ( NO_ERROR != RetCode ){

            CString ErrMsg;
            TCHAR   ErrCode[16];

            _ltot(RetCode, ErrCode, 10 );
            AfxFormatString1( ErrMsg, IDS_REMOVEUSERERR, ErrCode );
            MessageBox(ErrMsg);

        } else {

            //
            // Reflect the error happened
            //

            RetCode = RetCodeBak;
        }

    }

    if (AddUserList){
        delete [] AddUserList;
    }
    if (RemoveUserList){
        delete [] RemoveUserList;
    }

    return RetCode;
}

DWORD
USERLIST::AddNewUsers(CUsers &NewUser)
{
    DWORD RetCode;

    m_UserList.DeleteAllItems( );
    RetCode = m_Users.Add(NewUser);
    SetUpListBox(NULL);

    return RetCode;
}


void USERLIST::SetUpListBox(BOOL *EnableAdd)
{
    PVOID   EnumHandle;

    try{
        CString UnKnownUser;
        CString NoCertName;

        UnKnownUser.LoadString(IDS_UNKNOWNUSER);
        NoCertName.LoadString(IDS_NOCERTNAME);

        if (EnumHandle = m_Users.StartEnum()){

            LV_ITEM fillItem;

            fillItem.mask = LVIF_TEXT;


            //
            // At least one user is available
            //

            while ( EnumHandle ){
                CString UserNameInFile;
                CString CertName;

                fillItem.iItem = 0;
                fillItem.iSubItem = 0;
                EnumHandle = m_Users.GetNextUser(EnumHandle, UserNameInFile, CertName);

                if (UserNameInFile.IsEmpty()){
                    fillItem.pszText = UnKnownUser.GetBuffer(UnKnownUser.GetLength() + 1);
                } else {
                    fillItem.pszText = UserNameInFile.GetBuffer(UserNameInFile.GetLength() + 1);
                }

                //
                // Add the user to the list
                //

                fillItem.iItem = m_UserList.InsertItem(&fillItem);
                if (UserNameInFile.IsEmpty()){
                    UnKnownUser.ReleaseBuffer();
                } else {
                    UserNameInFile.ReleaseBuffer();
                }
                if ( fillItem.iItem != -1 ){
                    if ( EnableAdd ){
                        *EnableAdd = TRUE;
                    }

                    if (CertName.IsEmpty()){
                        fillItem.pszText = NoCertName.GetBuffer(NoCertName.GetLength() + 1);
                    } else {
                        fillItem.pszText = CertName.GetBuffer(CertName.GetLength() + 1);
                    }

                    fillItem.iSubItem = 1;
                    m_UserList.SetItem(&fillItem);

                    if (CertName.IsEmpty()){
                        NoCertName.ReleaseBuffer();
                    } else {
                        CertName.ReleaseBuffer();
                    }
                }
            }
        }
    }
    catch(...){
        m_UserList.DeleteAllItems( );
        if ( EnableAdd ){
            *EnableAdd = FALSE;
        }
    }

}
