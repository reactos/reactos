;/*++
;
;Copyright (c) 1995  Microsoft Corporation
;
;Module Name:
;
;    inetmsg.mc
;
;Abstract:
;
;    Contains internationalizable message text for Windows Internet Client DLL
;    error codes
;
;Author:
;
;    Richard L Firth (rfirth) 03-Feb-1995
;
;Revision History:
;
;    03-Feb-1995 rfirth
;        Created
;
;--*/

;//
;// INTERNET errors - errors common to all functionality
;//

MessageId=12000 SymbolicName=INTERNET_ERROR_BASE
Language=English
.

MessageId=12001 SymbolicName=ERROR_INTERNET_OUT_OF_HANDLES
Language=English
No more Internet handles can be allocated
.

MessageId=12002 SymbolicName=ERROR_INTERNET_TIMEOUT
Language=English
The operation timed out
.

MessageId=12003 SymbolicName=ERROR_INTERNET_EXTENDED_ERROR
Language=English
The server returned extended information
.

MessageId=12004 SymbolicName=ERROR_INTERNET_INTERNAL_ERROR
Language=English
An internal error occurred in the Microsoft Internet extensions
.

MessageId=12005 SymbolicName=ERROR_INTERNET_INVALID_URL
Language=English
The URL is invalid
.

MessageId=12006 SymbolicName=ERROR_INTERNET_UNRECOGNIZED_SCHEME
Language=English
The URL does not use a recognized protocol
.

MessageId=12007 SymbolicName=ERROR_INTERNET_NAME_NOT_RESOLVED
Language=English
The server name or address could not be resolved
.

MessageId=12008 SymbolicName=ERROR_INTERNET_PROTOCOL_NOT_FOUND
Language=English
A protocol with the required capabilities was not found
.

MessageId=12009 SymbolicName=ERROR_INTERNET_INVALID_OPTION
Language=English
The option is invalid
.

MessageId=12010 SymbolicName=ERROR_INTERNET_BAD_OPTION_LENGTH
Language=English
The length is incorrect for the option type
.

MessageId=12011 SymbolicName=ERROR_INTERNET_OPTION_NOT_SETTABLE
Language=English
The option value cannot be set
.

MessageId=12012 SymbolicName=ERROR_INTERNET_SHUTDOWN
Language=English
Microsoft Internet Extension support has been shut down
.

MessageId=12013 SymbolicName=ERROR_INTERNET_INCORRECT_USER_NAME
Language=English
The user name was not allowed
.

MessageId=12014 SymbolicName=ERROR_INTERNET_INCORRECT_PASSWORD
Language=English
The password was not allowed
.

MessageId=12015 SymbolicName=ERROR_INTERNET_LOGIN_FAILURE
Language=English
The login request was denied
.

MessageId=12106 SymbolicName=ERROR_INTERNET_INVALID_OPERATION
Language=English
The requested operation is invalid
.

MessageId=12017 SymbolicName=ERROR_INTERNET_OPERATION_CANCELLED
Language=English
The operation has been canceled
.

MessageId=12018 SymbolicName=ERROR_INTERNET_INCORRECT_HANDLE_TYPE
Language=English
The supplied handle is the wrong type for the requested operation
.

MessageId=12019 SymbolicName=ERROR_INTERNET_INCORRECT_HANDLE_STATE
Language=English
The handle is in the wrong state for the requested operation
.

MessageId=12020 SymbolicName=ERROR_INTERNET_NOT_PROXY_REQUEST
Language=English
The request cannot be made on a Proxy session
.

MessageId=12021 SymbolicName=ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND
Language=English
The registry value could not be found
.

MessageId=12022 SymbolicName=ERROR_INTERNET_BAD_REGISTRY_PARAMETER
Language=English
The registry parameter is incorrect
.

MessageId=12023 SymbolicName=ERROR_INTERNET_NO_DIRECT_ACCESS
Language=English
Direct Internet access is not available
.

MessageId=12024 SymbolicName=ERROR_INTERNET_NO_CONTEXT
Language=English
No context value was supplied
.

MessageId=12025 SymbolicName=ERROR_INTERNET_NO_CALLBACK
Language=English
No status callback was supplied
.

MessageId=12026 SymbolicName=ERROR_INTERNET_REQUEST_PENDING
Language=English
There are outstanding requests
.

MessageId=12027 SymbolicName=ERROR_INTERNET_INCORRECT_FORMAT
Language=English
The information format is incorrect
.

MessageId=12028 SymbolicName=ERROR_INTERNET_ITEM_NOT_FOUND
Language=English
The requested item could not be found
.

MessageId=12029 SymbolicName=ERROR_INTERNET_CANNOT_CONNECT
Language=English
A connection with the server could not be established
.

MessageId=12030 SymbolicName=ERROR_INTERNET_CONNECTION_ABORTED
Language=English
The connection with the server was terminated abnormally
.

MessageId=12031 SymbolicName=ERROR_INTERNET_CONNECTION_RESET
Language=English
The connection with the server was reset
.

MessageId=12032 SymbolicName=ERROR_INTERNET_FORCE_RETRY
Language=English
The action must be retried
.

MessageId=12033 SymbolicName=ERROR_INTERNET_INVALID_PROXY_REQUEST
Language=English
The proxy request is invalid
.

MessageId=12034 SymbolicName=ERROR_INTERNET_NEED_UI
Language=English
User interaction is required to complete the operation
.

MessageId=12036 SymbolicName=ERROR_INTERNET_HANDLE_EXISTS
Language=English
The handle already exists
.

MessageId=12037 SymbolicName=ERROR_INTERNET_SEC_CERT_DATE_INVALID
Language=English
The date in the certificate is invalid or has expired
.

MessageId=12038 SymbolicName=ERROR_INTERNET_SEC_CERT_CN_INVALID
Language=English
The host name in the certificate is invalid or does not match
.

MessageId=12039 SymbolicName=ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR
Language=English
A redirect request will change a non-secure to a secure connection
.

MessageId=12040 SymbolicName=ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR
Language=English
A redirect request will change a secure to a non-secure connection
.

MessageId=12041 SymbolicName=ERROR_INTERNET_MIXED_SECURITY
Language=English
Mixed secure and non-secure connections
.

MessageId=12042 SymbolicName=ERROR_INTERNET_CHG_POST_IS_NON_SECURE
Language=English
Changing to non-secure post
.

MessageId=12043 SymbolicName=ERROR_INTERNET_POST_IS_NON_SECURE
Language=English
Data is being posted on a non-secure connection
.

MessageId=12044 SymbolicName=ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED
Language=English
A certificate is required to complete client authentication
.

MessageId=12045 SymbolicName=ERROR_INTERNET_INVALID_CA
Language=English
The certificate authority is invalid or incorrect
.

MessageId=12046 SymbolicName=ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP
Language=English
Client authentication has not been correctly installed
.

MessageId=12047 SymbolicName=ERROR_INTERNET_ASYNC_THREAD_FAILED
Language=English
An error has occurred in a Wininet asynchronous thread. You may need to restart
.

MessageId=12048 SymbolicName=ERROR_INTERNET_REDIRECT_SCHEME_CHANGE
Language=English
The protocol scheme has changed during a redirect operaiton
.

MessageId=12049 SymbolicName=ERROR_INTERNET_DIALOG_PENDING
Language=English
There are operations awaiting retry
.

MessageId=12050 SymbolicName=ERROR_INTERNET_RETRY_DIALOG
Language=English
The operation must be retried
.

MessageId=12051 SymbolicName=ERROR_INTERNET_NO_NEW_CONTAINERS
Language=English
There are no new cache containers
.

MessageId=12052 SymbolicName=ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR
Language=English
A security zone check indicates the operation must be retried
.

MessageId=12157 SymbolicName=ERROR_INTERNET_SECURITY_CHANNEL_ERROR
Language=English
An error occurred in the secure channel support
.

MessageId=12158 SymbolicName=ERROR_INTERNET_UNABLE_TO_CACHE_FILE
Language=English
The file could not be written to the cache
.

MessageId=12159 SymbolicName=ERROR_INTERNET_TCPIP_NOT_INSTALLED
Language=English
The TCP/IP protocol is not installed properly
.

MessageId=12163 SymbolicName=ERROR_INTERNET_DISCONNECTED
Language=English
The computer is disconnected from the network
.

MessageId=12164 SymbolicName=ERROR_INTERNET_SERVER_UNREACHABLE
Language=English
The server is unreachable
.

MessageId=12165 SymbolicName=ERROR_INTERNET_PROXY_SERVER_UNREACHABLE
Language=English
The proxy server is unreachable
.

MessageId=12166 SymbolicName=ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT
Language=English
The proxy auto-configuration script is in error
.

MessageId=12167 SymbolicName=ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT
Language=English
Could not download the proxy auto-configuration script file
.

MessageId=12169 SymbolicName=ERROR_INTERNET_SEC_INVALID_CERT
Language=English
The supplied certificate is invalid
.

MessageId=12170 SymbolicName=ERROR_INTERNET_SEC_CERT_REVOKED
Language=English
The supplied certificate has been revoked
.

MessageId=12171 SymbolicName=ERROR_INTERNET_FAILED_DUETOSECURITYCHECK
Language=English
The Dialup failed because file sharing was turned on and a failure was requested if security check was needed
.


;//
;// FTP errors
;//

MessageId=12110 SymbolicName=ERROR_FTP_TRANSFER_IN_PROGRESS
Language=English
There is already an FTP request in progress on this session
.

MessageId=12111 SymbolicName=ERROR_FTP_DROPPED
Language=English
The FTP session was terminated
.

MessageId=12112 SymbolicName=ERROR_FTP_NO_PASSIVE_MODE
Language=English
FTP Passive mode is not available
.

;//
;// GOPHER errors
;//

MessageId=12130 SymbolicName=ERROR_GOPHER_PROTOCOL_ERROR
Language=English
A gopher protocol error occurred
.

MessageId=12131 SymbolicName=ERROR_GOPHER_NOT_FILE
Language=English
The locator must be for a file
.

MessageId=12132 SymbolicName=ERROR_GOPHER_DATA_ERROR
Language=English
An error was detected while parsing the data
.

MessageId=12133 SymbolicName=ERROR_GOPHER_END_OF_DATA
Language=English
There is no more data
.

MessageId=12134 SymbolicName=ERROR_GOPHER_INVALID_LOCATOR
Language=English
The locator is invalid
.

MessageId=12135 SymbolicName=ERROR_GOPHER_INCORRECT_LOCATOR_TYPE
Language=English
The locator type is incorrect for this operation
.

MessageId=12136 SymbolicName=ERROR_GOPHER_NOT_GOPHER_PLUS
Language=English
The request must be for a gopher+ item
.

MessageId=12137 SymbolicName=ERROR_GOPHER_ATTRIBUTE_NOT_FOUND
Language=English
The requested attribute was not found
.

MessageId=12138 SymbolicName=ERROR_GOPHER_UNKNOWN_LOCATOR
Language=English
The locator type is not recognized
.


;//
;// HTTP errors
;//

MessageId=12150 SymbolicName=ERROR_HTTP_HEADER_NOT_FOUND
Language=English
The requested header was not found
.

MessageId=12151 SymbolicName=ERROR_HTTP_DOWNLEVEL_SERVER
Language=English
The server does not support the requested protocol level
.

MessageId=12152 SymbolicName=ERROR_HTTP_INVALID_SERVER_RESPONSE
Language=English
The server returned an invalid or unrecognized response
.

MessageId=12153 SymbolicName=ERROR_HTTP_INVALID_HEADER
Language=English
The supplied HTTP header is invalid
.

MessageId=12154 SymbolicName=ERROR_HTTP_INVALID_QUERY_REQUEST
Language=English
The request for a HTTP header is invalid
.

MessageId=12155 SymbolicName=ERROR_HTTP_HEADER_ALREADY_EXISTS
Language=English
The HTTP header already exists
.

MessageId=12156 SymbolicName=ERROR_HTTP_REDIRECT_FAILED
Language=English
The HTTP redirect request failed
.

MessageId=12160 SymbolicName=ERROR_HTTP_NOT_REDIRECTED
Language=English
The HTTP request was not redirected
.

MessageId=12161 SymbolicName=ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION
Language=English
A cookie from the server must be confirmed by the user
.

MessageId=12162 SymbolicName=ERROR_HTTP_COOKIE_DECLINED
Language=English
A cookie from the server has been declined acceptance
.

MessageId=12168 SymbolicName=ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION
Language=English
The HTTP redirect request must be confirmed by the user
.
