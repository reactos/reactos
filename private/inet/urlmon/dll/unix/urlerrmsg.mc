;/*++
;
;Copyright (c) 1995-1997  Microsoft Corporation
;
;Module Name:
;
;    urlmon.mc
;
;Abstract:
;
;    Contains internationalizable message text for URLMON DLL error codes
;
;--*/

; AS: Why are the severity names nonstandard (as per winerror.h?)
;     Take out to use default.              
SeverityNames=(
               CoError=0x2
              )
              
FacilityNames=(Internet=0xc:FACILITY_INTERNET
               CodeDownload=0xb:FACILITY_CODE_DOWNLOAD
              )

;//                                                                           
;//                                                                           
;// WinINet and protocol specific errors are mapped to one of the following   
;// error which are returned in IBSC::OnStopBinding                           
;//                                                                           
;//                                                                           

MessageId=0x2   Facility=Internet Severity=CoError SymbolicName=INET_E_INVALID_URL
Language=English
The URL is invalid.
.

MessageId=0x3   Facility=Internet Severity=CoError SymbolicName=INET_E_NO_SESSION
Language=English
No Internet session has been established.
.

MessageId=0x4   Facility=Internet Severity=CoError SymbolicName=INET_E_CANNOT_CONNECT
Language=English
Unable to connect to the target server.
.

MessageId=0x5   Facility=Internet Severity=CoError SymbolicName=INET_E_RESOURCE_NOT_FOUND
Language=English
The system cannot locate the resource specified.
.

MessageId=0x6   Facility=Internet Severity=CoError SymbolicName=INET_E_OBJECT_NOT_FOUND
Language=English
The system cannot locate the object specified.
.

MessageId=0x7   Facility=Internet Severity=CoError SymbolicName=INET_E_DATA_NOT_AVAILABLE
Language=English
No data is available for the requested resource.
.

MessageId=0x8   Facility=Internet Severity=CoError SymbolicName=INET_E_DOWNLOAD_FAILURE
Language=English
The download of the specified resource has failed.
.

MessageId=0x9   Facility=Internet Severity=CoError SymbolicName=INET_E_AUTHENTICATION_REQUIRED
Language=English
Authentication is required to access this resource.
.

MessageId=0xA   Facility=Internet Severity=CoError SymbolicName=INET_E_NO_VALID_MEDIA
Language=English
The server could not recognize the provided mime type.
.

MessageId=0xB   Facility=Internet Severity=CoError SymbolicName=INET_E_CONNECTION_TIMEOUT
Language=English
The operation was timed out.
.

MessageId=0xC   Facility=Internet Severity=CoError SymbolicName=INET_E_INVALID_REQUEST
Language=English
The server did not understand the request, or the request was invalid.
.

MessageId=0xD   Facility=Internet Severity=CoError SymbolicName=INET_E_UNKNOWN_PROTOCOL
Language=English
The specified protocol is unknown.
.

MessageId=0xE   Facility=Internet Severity=CoError SymbolicName=INET_E_SECURITY_PROBLEM
Language=English
A security problem occurred.
.

MessageId=0xF   Facility=Internet Severity=CoError SymbolicName=INET_E_CANNOT_LOAD_DATA
Language=English
The system could not load the persisted data.
.

MessageId=0x10  Facility=Internet Severity=CoError SymbolicName=INET_E_CANNOT_INSTANTIATE_OBJECT
Language=English
Unable to instantiate the object.
.

MessageId=0x14  Facility=Internet Severity=CoError SymbolicName=INET_E_REDIRECT_FAILED
Language=English
A redirection problem occured.
.
















