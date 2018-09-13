// Locate.cpp : implementation file
//

#include "stdafx.h"
#include "efsadu.h"
#include "AddSheet.h"
#include "Locate.h"
#include <initguid.h>
#include <cmnquery.h>
#include <dsquery.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <ntdsapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define szCertAttr  _T("?userCertificate")

/////////////////////////////////////////////////////////////////////////////
// CLocate property page

IMPLEMENT_DYNCREATE(CLocate, CPropertyPage)

CLocate::CLocate() : CPropertyPage(CLocate::IDD)
{
	EnableAutomation();
	//{{AFX_DATA_INIT(CLocate)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CLocate::~CLocate()
{
}

void CLocate::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CPropertyPage::OnFinalRelease();
}

void CLocate::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLocate)
	DDX_Control(pDX, IDC_ADDLIST, m_UserAddList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLocate, CPropertyPage)
	//{{AFX_MSG_MAP(CLocate)
	ON_BN_CLICKED(IDC_BROWSE_DIR, OnBrowseDir)
	ON_BN_CLICKED(IDC_BROWSE_FILE, OnBrowseFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CLocate, CPropertyPage)
	//{{AFX_DISPATCH_MAP(CLocate)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ILocate to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {8C048CD8-54B2-11D1-BB63-00A0C906345D}
static const IID IID_ILocate =
{ 0x8c048cd8, 0x54b2, 0x11d1, { 0xbb, 0x63, 0x0, 0xa0, 0xc9, 0x6, 0x34, 0x5d } };

BEGIN_INTERFACE_MAP(CLocate, CPropertyPage)
	INTERFACE_PART(CLocate, IID_ILocate, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLocate message handlers

BOOL CLocate::OnSetActive()
{
    CAddSheet *AddSheet = (CAddSheet *) GetParent();
	AddSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );
	
	return CPropertyPage::OnSetActive();
}

void CLocate::OnBrowseDir()
{
	FindUserFromDir();	
}

void CLocate::OnBrowseFile()
{
    try{

        CString FileFilter;
        FileFilter.LoadString(IDS_CERTFILEFILTER);

	    CFileDialog     BrowseCertFile(
                                        TRUE,
                                        NULL,
                                        NULL,
                                        OFN_PATHMUSTEXIST | OFN_READONLY,
                                        FileFilter,
                                        this
                                        );

        INT_PTR RetCode = BrowseCertFile.DoModal();

        if (IDOK == RetCode){

            CString FileName = BrowseCertFile.GetPathName( );

            //
            // Open cert store from the file
            //

            HCERTSTORE      hCcertStore = NULL;
            PVOID   FileNameVoidP = (PVOID)(LPCTSTR)FileName;
            PCCERT_CONTEXT  CertContext = NULL;
            DWORD   EncodingType = 0;
            DWORD   ContentType = 0;
            DWORD   FormatType = 0;
            BOOL        b;

            b = CryptQueryObject(
                CERT_QUERY_OBJECT_FILE,
                FileNameVoidP,
                CERT_QUERY_CONTENT_FLAG_ALL,
                CERT_QUERY_FORMAT_FLAG_ALL,
                0,
                &EncodingType,
                &ContentType,
                &FormatType,
                &hCcertStore,
                NULL,
                (const void **)&CertContext
                );

            if ( b ) {

                //
                // Success. See what we get back. A store or a cert.
                //

                if (  (ContentType == CERT_QUERY_CONTENT_SERIALIZED_STORE) && hCcertStore){

                    //
                    // Cert Store file. Go get the cert. To be implemented later
                    //

                    if ( hCcertStore ){
                        CertCloseStore(hCcertStore, 0);
                    }
                    return;

                } else if (  (ContentType != CERT_QUERY_CONTENT_CERT) || !CertContext ) {

                    //
                    // Neither a valid cert file nor a store file we like.
                    //

                    if ( hCcertStore ){
                        CertCloseStore(hCcertStore, 0);
                    }

                    if  ( CertContext ){
                        CertFreeCertificateContext(CertContext);
                    }

                    CString ErrMsg;

                    ErrMsg.LoadString(IDS_CERTFILEFORMATERR);
                    MessageBox(ErrMsg);
                    return;

                }

                if ( hCcertStore ){
                    CertCloseStore(hCcertStore, 0);
                    hCcertStore = NULL;
                }
                //
                // We got the cert. Add it to the structure. We need get the subject name first.
                //

                LPTSTR  UserCertName;

                RetCode = GetCertNameFromCertContext(
                                        CertContext,
                                        &UserCertName
                                        );

                if ( ERROR_SUCCESS != RetCode ){

                    //
                    // BUGBUG: Out of memory. What should we do.
                    // We don't even have memory to pop a dialog box.
                    // We may need reserve the memory for the error box.
                    //

                    if  ( CertContext ){
                        CertFreeCertificateContext(CertContext);
                    }

                    return;
                }

                //
                // Add the user
                //

                CAddSheet *AddSheet = (CAddSheet *) GetParent();
                EFS_CERTIFICATE_BLOB CertBlob;

                CertBlob.cbData = CertContext->cbCertEncoded;
                CertBlob.pbData = CertContext->pbCertEncoded;
                CertBlob.dwCertEncodingType = CertContext->dwCertEncodingType;
                RetCode = AddSheet->Add(
                                                            NULL,
                                                            UserCertName,
                                                            (PVOID)&CertBlob,
                                                            NULL,
                                                            USERADDED,
                                                            (PVOID)CertContext
                                                            );

                if ( (ERROR_SUCCESS != RetCode) && (CRYPT_E_EXISTS != RetCode) ){

                    //
                    // Error in adding the user
                    //

                    CertFreeCertificateContext(CertContext);
                    CertContext = NULL;

                } else {

                    //
                    // Add the user to the list box.
                    //

                    if ( RetCode == ERROR_SUCCESS ){
                        LV_ITEM fillItem;
                        CString UserUnknown;

                        try {
                            if (!UserUnknown.LoadString(IDS_UNKNOWNUSER)){
                                UserUnknown.Empty();
                            }
                        }
                        catch (...){
                            UserUnknown.Empty();
                        }

                        fillItem.mask = LVIF_TEXT;
                        fillItem.iItem = 0;
                        fillItem.iSubItem = 0;
                        if (UserUnknown.IsEmpty()){
                            fillItem.pszText = _T("");
                        } else {
                            fillItem.pszText = UserUnknown.GetBuffer(UserUnknown.GetLength() + 1);
                        }
                        fillItem.iItem = m_UserAddList.InsertItem(&fillItem);
                        if (!UserUnknown.IsEmpty()){
                            UserUnknown.ReleaseBuffer();
                        }

                        if (fillItem.iItem != -1 ){
                            fillItem.pszText = UserCertName;
                            fillItem.iSubItem = 1;
                            m_UserAddList.SetItem(&fillItem);
                        } else {
                            AddSheet->Remove( NULL,  UserCertName);
                        }
                        UserCertName = NULL;

                    } else {

                        //
                        // Already deleted inside the Add.
                        //

                        UserCertName = NULL;
                    }
                }

                if (UserCertName){
                    delete [] UserCertName;
                    UserCertName = NULL;
                }

            } else {

                //
                // Fail. Get the error code.
                //

                RetCode = GetLastError();
                CString ErrMsg;
                TCHAR   ErrCode[16];

                _ltot((LONG)RetCode, ErrCode, 10 );
                AfxFormatString1( ErrMsg, IDS_CERTFILEOPENERR, ErrCode );
                MessageBox(ErrMsg);

            }

        }

    }
    catch (...){

        //
        // BUGBUG: Out of memory. Should we pop up an error message?
        //

    }
	
}


HRESULT CLocate::FindUserFromDir()
{
    HRESULT hr;
    LPTSTR  ListUserName = NULL;
    LPTSTR UserCertName = NULL;

    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = { TYMED_NULL, NULL, NULL };

    ICommonQuery* pCommonQuery = NULL;
    OPENQUERYWINDOW oqw;
    DSQUERYINITPARAMS dqip;
    BOOL    CheckDS = FALSE;
    HANDLE  hDS = NULL;
    CAddSheet *AddSheet = (CAddSheet *)GetParent();

    CoInitialize(NULL);

    hr = CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (LPVOID*)&pCommonQuery);

    if ( SUCCEEDED(hr) ) {

        dqip.cbStruct = sizeof(dqip);
        dqip.dwFlags = DSQPF_SHOWHIDDENOBJECTS |
                       DSQPF_ENABLEADMINFEATURES;

        dqip.pDefaultScope = NULL;  //szScopeLocn

        oqw.cbStruct = sizeof(oqw);
        oqw.dwFlags = OQWF_OKCANCEL |
                        OQWF_SINGLESELECT |
                        OQWF_DEFAULTFORM |
                        OQWF_REMOVEFORMS ;
        oqw.clsidHandler = CLSID_DsQuery;
        oqw.pHandlerParameters = &dqip;
        oqw.clsidDefaultForm = CLSID_DsFindPeople;

        IDataObject* pDataObject = NULL;

        hr = pCommonQuery->OpenQueryWindow(NULL, &oqw, &pDataObject );

        if ( SUCCEEDED(hr) && pDataObject )
        {
            // Fill the list view

            fmte.cfFormat = (CLIPFORMAT)AddSheet->GetDataFormat();

            if ( SUCCEEDED(hr = pDataObject->GetData(&fmte, &medium)) )
            {
                LPDSOBJECTNAMES pDsObjects = (LPDSOBJECTNAMES)medium.hGlobal;

                hr = DsBind(NULL, NULL, &hDS);
                if ( hr == NO_ERROR ){

                    //
                    //  We are going to use the DS to crack the names
                    //

                    CheckDS = TRUE;
                }


                if ( pDsObjects->cItems ) {

                    for ( UINT i = 0 ; i < pDsObjects->cItems ; i++ ) {

                        LPTSTR pTemp = (LPTSTR)(((LPBYTE)pDsObjects)+pDsObjects->aObjects[i].offsetName);
                        DS_NAME_RESULT* UserName = NULL;
                        PSID UserSid = NULL;
                        DWORD cbSid = 0;
                        LPTSTR ReferencedDomainName = NULL;
                        DWORD cbReferencedDomainName = 0;
                        SID_NAME_USE SidUse;
                        PCCERT_CONTEXT  CertContext = NULL;


                        //
                        // Get rid of the head :\\
                        //

                        LPTSTR pSearch = _tcschr(pTemp, _T(':'));
                        if (pSearch && (pSearch[1] == _T('/')) && (pSearch[2] == _T('/'))){
                            pTemp = pSearch + 3;
                        }

                        if ( CheckDS ){
                            hr = DsCrackNames(
                                                    hDS,
                                                    DS_NAME_NO_FLAGS,
                                                    DS_FQDN_1779_NAME,
                                                    DS_NT4_ACCOUNT_NAME,
                                                    1,
                                                    &pTemp,
                                                    &UserName
                                                    );

                            if ( ERROR_SUCCESS == hr ){
                                if (( UserName->cItems > 0 ) && (DS_NAME_NO_ERROR == UserName->rItems[0].status)){
                                    BOOL b;

                                    //
                                    // Save the NT4 name first, in case we cannot get the principle name
                                    //

                                    ListUserName = new TCHAR[_tcslen(UserName->rItems[0].pName) + 1];
                                    if (ListUserName){
                                        _tcscpy(ListUserName, UserName->rItems[0].pName);
                                    }

                                    b =  LookupAccountName(
                                                NULL,
                                                UserName->rItems[0].pName,
                                                UserSid,
                                                &cbSid,
                                                ReferencedDomainName,
                                                &cbReferencedDomainName,
                                                &SidUse
                                                );

                                    if ( !b && (ERROR_INSUFFICIENT_BUFFER == (hr = GetLastError())) ){

                                        //
                                        // We are expecting this error
                                        //

                                        UserSid = new BYTE[cbSid];
                                        ReferencedDomainName =  new TCHAR[cbReferencedDomainName];
                                        if ( UserSid && ReferencedDomainName ){

                                            b =  LookupAccountName(
                                                        NULL,
                                                        UserName->rItems[0].pName,
                                                        UserSid,
                                                        &cbSid,
                                                        ReferencedDomainName,
                                                        &cbReferencedDomainName,
                                                        &SidUse
                                                        );

                                            delete [] ReferencedDomainName;
                                            ReferencedDomainName = NULL;
                                            if (!b){
                                                //
                                                // Get SID failed. We can live with it.
                                                //

                                                UserSid = NULL;
                                            }
                                        } else {
                                            if (UserSid){
                                                delete [] UserSid;
                                                UserSid = NULL;
                                            }
                                            if (ReferencedDomainName){
                                                delete [] ReferencedDomainName;
                                                ReferencedDomainName = NULL;
                                            }
                                        }

                                    } else {

                                        ASSERT( !b);
                                        UserSid = NULL;
                                    }

                                }
                            } else {
                                //
                                // Cannot get the NT4 name. Set the SID to NULL. Go on.
                                //

                                UserSid = NULL;
                            }

                            if (UserName){
                                DsFreeNameResult(UserName);
                                UserName = NULL;
                            }

                            hr = DsCrackNames(
                                                    hDS,
                                                    DS_NAME_NO_FLAGS,
                                                    DS_FQDN_1779_NAME,
                                                    DS_USER_PRINCIPAL_NAME,
                                                    1,
                                                    &pTemp,
                                                    &UserName
                                                    );

                            if ( (ERROR_SUCCESS == hr) &&
                                  ( UserName->cItems > 0 ) &&
                                  (DS_NAME_NO_ERROR == UserName->rItems[0].status) ){

                                //
                                // We got the principal name
                                //

                                LPTSTR  TmpNameStr;

                                TmpNameStr =  new TCHAR[_tcslen(UserName->rItems[0].pName) + 1];
                                if ( TmpNameStr ){
                                    _tcscpy(TmpNameStr, UserName->rItems[0].pName);
                                    delete [] ListUserName;
                                    ListUserName = TmpNameStr;
                                } else {
                                    hr = ERROR_OUTOFMEMORY;
                                }

                            }
                        }

                        if ( (ERROR_OUTOFMEMORY != hr) && ( NULL ==  ListUserName)){

                            //
                            // Use the LDAP name
                            //
                            ListUserName = new TCHAR[_tcslen(pTemp)+1];
                            if ( ListUserName ){
                                _tcscpy(ListUserName, pTemp);
                            } else {
                                hr = ERROR_OUTOFMEMORY;
                            }

                        }

                        if (UserName){
                            DsFreeNameResult(UserName);
                            UserName = NULL;
                        }

                        if ( ERROR_OUTOFMEMORY != hr ){

                            //
                            // Now is the time to get the certificate
                            //

                            LPTSTR  LdapUrl = new TCHAR[_tcslen(pTemp)+9+_tcslen(szCertAttr)];

                            if ( LdapUrl ){

                                //
                                // This is really not necessary. MS should make the name convention consistant.
                                //

                                _tcscpy(LdapUrl, _T("LDAP:///"));
                                _tcscat(LdapUrl, pTemp);
                                _tcscat(LdapUrl, szCertAttr);

                                hr = ERROR_SUCCESS;
                                HCERTSTORE hDSCertStore = ::CertOpenStore (
                                                        sz_CERT_STORE_PROV_LDAP,
                                                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                        NULL,
                                                        CERT_STORE_READONLY_FLAG,
							                            (void*) LdapUrl
                                                        );
                                //
                                // In case delete change the result of GetLastError()
                                //

                                hr = GetLastError();

                                delete [] LdapUrl;
                                LdapUrl = NULL;

                                if (hDSCertStore){

                                    //
                                    // We get the certificate store
                                    //

                                    CertContext = ::CertFindCertificateInStore (
				                                                            hDSCertStore,
				                                                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				                                                            0 ,
				                                                            CERT_FIND_ANY,
				                                                            NULL,
 				                                                            NULL
                                                                            );

                                    if ( CertContext ){

                                        //
                                        // We got the certificate. Add it to the lists.
                                        // Get the certificate display name first
                                        //

                                        hr = GetCertNameFromCertContext(
                                                                CertContext,
                                                                &UserCertName
                                                                );

                                        //
                                        // Add the user
                                        //

                                        EFS_CERTIFICATE_BLOB CertBlob;

                                        CertBlob.cbData = CertContext->cbCertEncoded;
                                        CertBlob.pbData = CertContext->pbCertEncoded;
                                        CertBlob.dwCertEncodingType = CertContext->dwCertEncodingType;
                                        hr = AddSheet->Add(
                                                                    ListUserName,
                                                                    UserCertName,
                                                                    (PVOID)&CertBlob,
                                                                    UserSid,
                                                                    USERADDED,
                                                                    (PVOID)CertContext
                                                                    );

                                        if ( (ERROR_SUCCESS != hr) && (CRYPT_E_EXISTS != hr) ){

                                            //
                                            // Error in adding the user
                                            //

                                            CertFreeCertificateContext(CertContext);
                                            CertContext = NULL;

                                        } else {

                                            //
                                            // Add the user to the list box.
                                            //

                                            if (ERROR_SUCCESS == hr){
                                                LV_ITEM fillItem;

                                                fillItem.mask = LVIF_TEXT;
                                                fillItem.iItem = 0;
                                                fillItem.iSubItem = 0;

                                                fillItem.pszText = ListUserName;
                                                fillItem.iItem = m_UserAddList.InsertItem(&fillItem);

                                                if ( fillItem.iItem == -1 ){
                                                    AddSheet->Remove( ListUserName,  UserCertName);
                                                } else {
                                                    fillItem.pszText = UserCertName;
                                                    fillItem.iSubItem = 1;
                                                    m_UserAddList.SetItem(&fillItem);
                                                }

                                            }

                                            //
                                            //Either deleted (CRYPT_E_EXISTS) or should not be freed (ERROR_SUCCESS)
                                            //

                                            ListUserName = NULL;
                                            UserCertName = NULL;

                                        }

                                    }

                                    delete [] UserSid;
                                    UserSid = NULL;
                                    if (ListUserName){
                                        delete [] ListUserName;
                                        ListUserName = NULL;
                                    }
                                    if (UserCertName){
                                        delete [] UserCertName;
                                        UserCertName = NULL;
                                    }
                                    if ( hDSCertStore ){
                                        CertCloseStore(hDSCertStore, 0);
                                        hDSCertStore = NULL;
                                    }

                                } else {

                                    //
                                    // Failed to open the cert store
                                    //
                                    delete [] UserSid;
                                    UserSid = NULL;
                                    if (ListUserName){
                                        delete [] ListUserName;
                                        ListUserName = NULL;
                                    }
                                    if (UserCertName){
                                        delete [] UserCertName;
                                        UserCertName = NULL;
                                    }

                                }
                            } else {
                                hr = ERROR_OUTOFMEMORY;
                            }

                        }
                        if ( ERROR_OUTOFMEMORY == hr ) {

                            //
                            // Free the memory. Delete works for NULL. No check is needed.
                            //
                            delete [] UserSid;
                            UserSid = NULL;
                            delete [] ListUserName;
                            ListUserName = NULL;
                            delete [] UserCertName;
                            UserCertName = NULL;
                        }

                    }//For
                }

                if (CheckDS){
                    DsUnBindW( &hDS );
                }

                ReleaseStgMedium(&medium);

            }

            pDataObject->Release();

        }

        pCommonQuery->Release();

    }

    return hr;

}

DWORD
CLocate::GetCertNameFromCertContext(
    PCCERT_CONTEXT CertContext,
    LPTSTR * UserCertName
    )
//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Get the user name from the certificate
// Arguments:
//      CertContext -- Cert Context
//      UserCertName -- User name
//                                  ( Caller is responsible to delete this memory using delete [] )
//  Return Value:
//      ERROR_SUCCESS if succeed.
//      If No Name if found. "USER_UNKNOWN is returned".
//
//////////////////////////////////////////////////////////////////////
{
    DWORD   NameLength;
    DWORD   UserNameBufLen = 0;

    if ( NULL == UserCertName ){
        return ERROR_SUCCESS;
    }

    *UserCertName = NULL;
    NameLength = CertGetNameString(
                                CertContext,
                                CERT_NAME_EMAIL_TYPE,
                                0,
                                NULL,
                                NULL,
                                0
                                );

    if ( NameLength > 1){

        //
        // The E-Mail name exist. Go get the email name.
        //

        *UserCertName = new TCHAR[NameLength];
        if ( NULL == *UserCertName ){
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        UserNameBufLen = NameLength;
        NameLength = CertGetNameString(
                                    CertContext,
                                    CERT_NAME_EMAIL_TYPE,
                                    0,
                                    NULL,
                                    *UserCertName,
                                    UserNameBufLen
                                    );

        ASSERT (NameLength == UserNameBufLen);

    } else {

        //
        // Get the subject name in the X500 format
        //

        DWORD   StrType = CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG;

        NameLength = CertGetNameString(
                                    CertContext,
                                    CERT_NAME_RDN_TYPE,
                                    0,
                                    &StrType,
                                    NULL,
                                    0
                                    );

        if  ( NameLength > 1 ){

            //
            // Name exists. Go get it.
            //

            *UserCertName = new TCHAR[NameLength];
            if ( NULL == *UserCertName ){
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            UserNameBufLen = NameLength;
            NameLength = CertGetNameString(
                                        CertContext,
                                        CERT_NAME_RDN_TYPE,
                                        0,
                                        &StrType,
                                        *UserCertName,
                                        UserNameBufLen
                                        );

            ASSERT (NameLength == UserNameBufLen);
        } else {

            try {
                CString UnknownCertName;

                UnknownCertName.LoadString(IDS_NOCERTNAME);

                UserNameBufLen = UnknownCertName.GetLength();
                *UserCertName = new TCHAR[UserNameBufLen + 1];
                _tcscpy( *UserCertName, UnknownCertName);

            }
            catch (...){
                return ERROR_NOT_ENOUGH_MEMORY;
            }

        }

    }

    return ERROR_SUCCESS;

}

BOOL CLocate::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
    CString UserNameTitle;
    CString UserDnTitle;
    RECT ListRect;
    DWORD ColWidth;

    try {	
	    m_UserAddList.GetClientRect(&ListRect);
        ColWidth = (ListRect.right - ListRect.left)/2;
        UserNameTitle.LoadString(IDS_USERCOLTITLE);
        UserDnTitle.LoadString(IDS_DNCOLTITLE);
        m_UserAddList.InsertColumn(0, UserNameTitle, LVCFMT_LEFT, ColWidth );
        m_UserAddList.InsertColumn(1, UserDnTitle, LVCFMT_LEFT, ColWidth );
    }
    catch (...){
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CLocate::OnWizardBack()
{
    CAddSheet *AddSheet = (CAddSheet *) GetParent();

    AddSheet->ClearUserList();	
    m_UserAddList.DeleteAllItems( );
	return CPropertyPage::OnWizardBack();
}
