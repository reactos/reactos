
#ifndef _WIN_SIC_CTXT_H_
#define _WIN_SIC_CTXT_H_

#define DEFAULT_SERVER_NAME_LEN		128

typedef struct _WINCONTEXT	{

    //
    //  Buffer for storing exchange blob returned by SSPI before 
    //  PreAuthenticateUser is called
    //
    char        szOutBuffer[MAX_BLOB_SIZE];
	DWORD		dwOutBufferLength;

    char        szInBuffer[MAX_BLOB_SIZE];
    PCHAR       pInBuffer;
	DWORD		dwInBufferLength;

	DWORD		dwCallId;

	DWORD		pkgId;

    CredHandle  Credential;     // SSPI credential handle for this connection
    PCredHandle pCredential;

	//
	// The SSPI Context Handle is stored here
	//
	CtxtHandle	SspContextHandle;
	PCtxtHandle	pSspContextHandle;  // before any ctxt is created, this is NULL

	LPSTR		lpszServerName;

	char		szServerName[DEFAULT_SERVER_NAME_LEN];

} WINCONTEXT, *PWINCONTEXT;

#endif  // _WIN_SIC_CTXT_H_
