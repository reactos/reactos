/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    spluginx.hxx

Abstract:

    This file contains headers for the WININET 
    HTTP Authentication Plug In Model.

    Contents:
        AuthenticateUser
        PreAuthenticateUser
        UnloadAuthenticateUser
        AuthenticateUserUI        

Author:

    Arthur Bierer (arthurbi) 04-Apr-1996

Revision History:

    04-Apr-1996 arthurbi
        Created

--*/

//
// Macros and Defines for Authentication UI
//

#define MAX_FIELD_LENGTH MAX_PATH

typedef enum tagAuthType
{
    REALM_AUTH   = 0,
    NTLM_AUTH    = 1,
} AuthType;


typedef struct tagInvalidPassType
{

    LPSTR             lpszRealm;
    LPSTR             lpszUsername;
    LPSTR             lpszPassword;
	LPSTR			  lpszHost;
    ULONG             ulMaxField;
    BOOL              fIsProxy;
    AuthType          eAuthType;
} InvalidPassType;

#ifdef __cplusplus
extern "C" {
#endif

//
// AuthenticateUser - Called on recept of a 401 or 407 status code
//  from the Web Server.  This function should attempt to 
//  validate the Server's 401/407 response, and generate
//  ERROR_SUCCESS if it accepts this Server for authentication
//  Otherwise this function should returns an appropriate
//  error found in wininet.h, or winerror.h
//
//  lppvContext     - pointer to Context pointer that will be passed on
//      every unquie conversation with the Server. Will be a pointer to a
//      NULL the first time this function is called for a unquie session.
//      Its the Function's responsiblity to allocate or generate a unquie context
//      value that will be passed on future calls.
//      
//  lpszServerName  - Host Name of Web server.
//  lpszScheme      - Name of Authentication Scheme being used, ie Basic, MSN, NTLM..
//  dwFlags         - Flags.
//  lpszUserName    - Possible UserName if availble.
//  lpszPassword    - Possible Password if availble.
//
//  RETURNS:
//      ERROR_SUCCESS - Means it accepts this connection as valid, and 
//          asks for the authentication to continue by restarting
//          the HTTP connection, and calling PreAuthenticateUser BEFORE
//          that new HTTP connection is opened.
//          
//      ERROR_INTERNET_INCORRECT_PASSWORD - Means we do not understand this
//          username and/or password that is passed to us.  Will
//          return to User or Application for new username and password.
//
//      ERROR_INTERNET_FORCE_RETRY - An additional 401/407 response may
//          need to be generated from the server.  Forces PreAuthenticateUser
//          to be recalled, and new HTTP connection/request. The difference
//          is WININET will expect a 401/407 to be generated from this request.
//      

#if defined(unix) && defined(__cplusplus)
extern "C"
#endif
DWORD
WINAPI
AuthenticateUser(
    IN OUT LPVOID*  lppvContext,
    IN LPSTR        lpszServerName,
    IN LPSTR        lpszScheme,
    IN BOOL         fCanUseLogon,
    IN LPSTR        lpszParsedAuthHeader,
    IN DWORD        dwcbParsedAuthHeader,
    IN LPSTR        lpszUserName,
    IN LPSTR        lpszPassword,
    OUT SECURITY_STATUS *pssResult
    );


//
// PreAuthenticateUser - Called on BEFORE doing a GET or POST
//  to a Web Server. This function is called only if there
//  has been a previous connection with the server using the
//  passed in Scheme. 
//
//  This function will attempt to Generate a proper set of 
//  encoded bytes that can be sent to the server on a HTTP
//  header.  If lpdwOutBufferLength is not large enough
//  ERROR_INSUFFICENT_BUFFER can be returned with the correct
//  size stored in lpdwOutBufferLength.
//
//  lppvContext     - pointer to Context pointer that will be passed on
//      every unquie conversation with the Server.
//  lpszServerName  - Host Name of Web server.
//  lpszScheme      - Name of Authentication Scheme being used, ie Basic, MSN, NTLM..
//  fCanUseLogon    - whether zone policy allows ntlm to use logon credential
//  lpOutBuffer     - Pointer to Buffer that will contain output encoded header bytes.
//	lpdwOutBufferLength - Size of Buffer Above, will contain size of buffer above,
//                      on return will contain bytes actually stored. 
//  lpszUserName    - Possible UserName if availble.
//  lpszPassword    - Possible Password if availble.
//
//


#if defined(unix) && defined(__cplusplus)
extern "C"
#endif
DWORD
WINAPI
PreAuthenticateUser(
	IN OUT LPVOID*  lppvContext,
	IN LPSTR        lpszServerName,
	IN LPSTR        lpszScheme,
	IN DWORD        dwFlags,
	OUT LPSTR       lpOutBuffer,
	IN OUT LPDWORD  lpdwOutBufferLength,
	IN LPSTR        lpszUserName,
	IN LPSTR        lpszPassword,
    OUT SECURITY_STATUS *pssResult
	);


//
// UnloadAuthenticateUser - Called after HTTP authentication
//  has been completed, and the authentication process is
//  no longer needed. 
//
//  Its intended for cleanup of context memory that was allocated.
//  NOTE: For certain schemes these context values may live
//  until shutdown.
//  

#if defined(unix) && defined(__cplusplus)
extern "C"
#endif
VOID
WINAPI
UnloadAuthenticateUser(
    IN OUT LPVOID*  lppvContext,
    IN LPSTR        lpszScheme,
	IN LPSTR        lpszHost
    );



//
// AuthenticateUserUI - This function is called when a application
//  called InternetErrorDlg for the purposes of generating UI on
//  a specific HTTP Authentication Scheme.  For example, if a User
//  required a login to the MSN service. An application could 
//  call InternetErrorDlg to generate UI, and WinINet would 
//  determine that MSN could handle the UI for quering the user/passwd
//  internaly.
//
//  Note: An application does not need to generate the UserName,
//  and Password for WININET in pAuthInfo.  Rather it could simply
//  return an ERROR_SUCCESS, and have its PreAuthenticeUser recalled.
//
//  Note: To Cancel the authentication process.  ERROR_CANCELLED should
//  be returned.
//
//  lppvContext - pointer to Authenticateion Context.
//  hWnd        - Windows Handle to Parent Window
//  dwError     - Error code passed to InternetErrorDlg
//  dwFlags     - flags.
//  pAuthInfo   - Pointer to structure containing a place to store 
//                Username and password

#if defined(unix) && defined(__cplusplus)
extern "C"
#endif
DWORD
WINAPI
AuthenticateUserUI(
   IN OUT LPVOID*   lppvContext,
   IN HWND          hWnd,
   IN DWORD         dwError,
   IN DWORD         dwFlags,
   IN OUT InvalidPassType* pAuthInfo,
   IN LPSTR         lpszScheme,
   SECURITY_STATUS  *pssResult
   );


#ifdef __cplusplus
} // end extern "C"
#endif

//
// Registry Flags Used for Defining what kind of authentication a 
//  plugin may support.
//

//
// Each TCP/IP Socket will contain a different Context.
//  Otherwise a new context will be passed for each Realm
//   or block URL template (http://www.foo.com/directory/*).
//

#define PLUGIN_AUTH_FLAGS_UNIQUE_CONTEXT_PER_TCPIP  0x01 

//
// This PlugIn Can handle doing it's own UI.  So call it
//  when UI is needed.
//

#define PLUGIN_AUTH_FLAGS_CAN_HANDLE_UI			    0x02

//
// This PlugIn may be capible of doing an Authentication
//  without prompting the User for a Password.  If this
//  is not the case AuthenticateUser should return
//  ERROR_INTERNET_INCORRECT_PASSWORD
//

#define PLUGIN_AUTH_FLAGS_CAN_HANDLE_NO_PASSWD      0x04


//
// This PlugIn does not use a standard HTTP Realm 
//  string.  Any data that appears to be a realm
//  is scheme specific data.
//

#define PLUGIN_AUTH_FLAGS_NO_REALM                  0x08

//
// This PlugIn doesn't need a persistent connection for challenge-response.
//

#define PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED   0x10

//
// Registry Values
//  To Register a Security PlugIn called FOO, place a key in
//  HKLM\Software\Microsoft\Internet Explorer\Security\FOO
//      In Foo Create the following values:
//          Flags           0x00000000      ( see above for Registry Flags )
//  


 

