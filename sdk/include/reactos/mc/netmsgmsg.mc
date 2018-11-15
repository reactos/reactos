;
; netmsg_msg.mc MESSAGE resources for netmsg.dll
;
;
; IMPORTANT: When a new language is added, all messages in this file need to be
; either translated or at least duplicated for the new language.
; This is a new requirement by MS mc.exe
; To do this, start with a regex replace:
; - In VS IDE: "Language=English\r\n(?<String>(?:[^\.].*\r\n)*\.\r\n)" -> "Language=English\r\n${String}Language=MyLanguage\r\n${String}"
;

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
              )

LanguageNames=(English=0x409:MSG00409
               Russian=0x419:MSG00419)


;
; lmerr.h message definitions (2100 - 2999)
;

MessageId=2102
Severity=Success
Facility=System
SymbolicName=NERR_NetNotStarted
Language=English
The workstation driver is not installed.
.
Language=Russian
The workstation driver is not installed.
.

MessageId=2103
Severity=Success
Facility=System
SymbolicName=NERR_UnknownServer
Language=English
The server could not be located.
.
Language=Russian
The server could not be located.
.

MessageId=2104
Severity=Success
Facility=System
SymbolicName=NERR_ShareMem
Language=English
An internal error occurred. The network cannot access a shared memory segment.
.
Language=Russian
An internal error occurred. The network cannot access a shared memory segment.
.

MessageId=2105
Severity=Success
Facility=System
SymbolicName=NERR_NoNetworkResource
Language=English
A network resource shortage occurred.
.
Language=Russian
A network resource shortage occurred.
.

MessageId=2106
Severity=Success
Facility=System
SymbolicName=NERR_RemoteOnly
Language=English
This operation is not supported on workstations.
.
Language=Russian
This operation is not supported on workstations.
.

MessageId=2107
Severity=Success
Facility=System
SymbolicName=NERR_DevNotRedirected
Language=English
The device is not connected.
.
Language=Russian
The device is not connected.
.

MessageId=2114
Severity=Success
Facility=System
SymbolicName=NERR_ServerNotStarted
Language=English
The Server service is not started.
.
Language=Russian
The Server service is not started.
.

MessageId=2115
Severity=Success
Facility=System
SymbolicName=NERR_ItemNotFound
Language=English
The queue is empty.
.
Language=Russian
The queue is empty.
.

MessageId=2116
Severity=Success
Facility=System
SymbolicName=NERR_UnknownDevDir
Language=English
The device or directory does not exist.
.
Language=Russian
The device or directory does not exist.
.

MessageId=2117
Severity=Success
Facility=System
SymbolicName=NERR_RedirectedPath
Language=English
The operation is invalid on a redirected resource.
.
Language=Russian
The operation is invalid on a redirected resource.
.

MessageId=2118
Severity=Success
Facility=System
SymbolicName=NERR_DuplicateShare
Language=English
The name has already been shared.
.
Language=Russian
The name has already been shared.
.

MessageId=2119
Severity=Success
Facility=System
SymbolicName=NERR_NoRoom
Language=English
The server is currently out of the requested resource.
.
Language=Russian
The server is currently out of the requested resource.
.

MessageId=2121
Severity=Success
Facility=System
SymbolicName=NERR_TooManyItems
Language=English
Requested addition of items exceeds the maximum allowed.
.
Language=Russian
Requested addition of items exceeds the maximum allowed.
.

MessageId=2122
Severity=Success
Facility=System
SymbolicName=NERR_InvalidMaxUsers
Language=English
The Peer service supports only two simultaneous users.
.
Language=Russian
The Peer service supports only two simultaneous users.
.

MessageId=2123
Severity=Success
Facility=System
SymbolicName=NERR_BufTooSmall
Language=English
The API return buffer is too small.
.
Language=Russian
The API return buffer is too small.
.

MessageId=2127
Severity=Success
Facility=System
SymbolicName=NERR_RemoteErr
Language=English
A remote API error occurred.
.
Language=Russian
A remote API error occurred.
.

MessageId=2131
Severity=Success
Facility=System
SymbolicName=NERR_LanmanIniError
Language=English
An error occurred when opening or reading the configuration file.
.
Language=Russian
An error occurred when opening or reading the configuration file.
.

MessageId=2136
Severity=Success
Facility=System
SymbolicName=NERR_NetworkError
Language=English
A general network error occurred.
.
Language=Russian
A general network error occurred.
.

MessageId=2137
Severity=Success
Facility=System
SymbolicName=NERR_WkstaInconsistentState
Language=English
The Workstation service is in an inconsistent state. Restart the computer before restarting the Workstation service.
.
Language=Russian
The Workstation service is in an inconsistent state. Restart the computer before restarting the Workstation service.
.

MessageId=2138
Severity=Success
Facility=System
SymbolicName=NERR_WkstaNotStarted
Language=English
The Workstation service has not been started.
.
Language=Russian
The Workstation service has not been started.
.

MessageId=2139
Severity=Success
Facility=System
SymbolicName=NERR_BrowserNotStarted
Language=English
The requested information is not available.
.
Language=Russian
The requested information is not available.
.

MessageId=2140
Severity=Success
Facility=System
SymbolicName=NERR_InternalError
Language=English
An internal error occurred.
.
Language=Russian
An internal error occurred.
.

MessageId=2141
Severity=Success
Facility=System
SymbolicName=NERR_BadTransactConfig
Language=English
The server is not configured for transactions.
.
Language=Russian
The server is not configured for transactions.
.

MessageId=2142
Severity=Success
Facility=System
SymbolicName=NERR_InvalidAPI
Language=English
The requested API is not supported on the remote server.
.
Language=Russian
The requested API is not supported on the remote server.
.

MessageId=2143
Severity=Success
Facility=System
SymbolicName=NERR_BadEventName
Language=English
The event name is invalid.
.
Language=Russian
The event name is invalid.
.

MessageId=2144
Severity=Success
Facility=System
SymbolicName=NERR_DupNameReboot
Language=English
The computer name already exists on the network. Change it and restart the computer.
.
Language=Russian
The computer name already exists on the network. Change it and restart the computer.
.

MessageId=2146
Severity=Success
Facility=System
SymbolicName=NERR_CfgCompNotFound
Language=English
The specified component could not be found in the configuration information.
.
Language=Russian
The specified component could not be found in the configuration information.
.

MessageId=2147
Severity=Success
Facility=System
SymbolicName=NERR_CfgParamNotFound
Language=English
The specified parameter could not be found in the configuration information.
.
Language=Russian
The specified parameter could not be found in the configuration information.
.

MessageId=2149
Severity=Success
Facility=System
SymbolicName=NERR_LineTooLong
Language=English
A line in the configuration file is too long.
.
Language=Russian
A line in the configuration file is too long.
.

MessageId=2150
Severity=Success
Facility=System
SymbolicName=NERR_QNotFound
Language=English
The printer does not exist.
.
Language=Russian
The printer does not exist.
.

MessageId=2151
Severity=Success
Facility=System
SymbolicName=NERR_JobNotFound
Language=English
The print job does not exist.
.
Language=Russian
The print job does not exist.
.

MessageId=2152
Severity=Success
Facility=System
SymbolicName=NERR_DestNotFound
Language=English
The printer destination cannot be found.
.
Language=Russian
The printer destination cannot be found.
.

MessageId=2153
Severity=Success
Facility=System
SymbolicName=NERR_DestExists
Language=English
The printer destination already exists.
.
Language=Russian
The printer destination already exists.
.

MessageId=2154
Severity=Success
Facility=System
SymbolicName=NERR_QExists
Language=English
The printer queue already exists.
.
Language=Russian
The printer queue already exists.
.

MessageId=2155
Severity=Success
Facility=System
SymbolicName=NERR_QNoRoom
Language=English
No more printers can be added.
.
Language=Russian
No more printers can be added.
.

MessageId=2156
Severity=Success
Facility=System
SymbolicName=NERR_JobNoRoom
Language=English
No more print jobs can be added.
.
Language=Russian
No more print jobs can be added.
.

MessageId=2157
Severity=Success
Facility=System
SymbolicName=NERR_DestNoRoom
Language=English
No more printer destinations can be added.
.
Language=Russian
No more printer destinations can be added.
.

MessageId=2158
Severity=Success
Facility=System
SymbolicName=NERR_DestIdle
Language=English
This printer destination is idle and cannot accept control operations.
.
Language=Russian
This printer destination is idle and cannot accept control operations.
.

MessageId=2159
Severity=Success
Facility=System
SymbolicName=NERR_DestInvalidOp
Language=English
This printer destination request contains an invalid control function.
.
Language=Russian
This printer destination request contains an invalid control function.
.

MessageId=2160
Severity=Success
Facility=System
SymbolicName=NERR_ProcNoRespond
Language=English
The print processor is not responding.
.
Language=Russian
The print processor is not responding.
.

MessageId=2161
Severity=Success
Facility=System
SymbolicName=NERR_SpoolerNotLoaded
Language=English
The spooler is not running.
.
Language=Russian
The spooler is not running.
.

MessageId=2162
Severity=Success
Facility=System
SymbolicName=NERR_DestInvalidState
Language=English
This operation cannot be performed on the print destination in its current state.
.
Language=Russian
This operation cannot be performed on the print destination in its current state.
.

MessageId=2163
Severity=Success
Facility=System
SymbolicName=NERR_QInvalidState
Language=English
This operation cannot be performed on the printer queue in its current state.
.
Language=Russian
This operation cannot be performed on the printer queue in its current state.
.

MessageId=2164
Severity=Success
Facility=System
SymbolicName=NERR_JobInvalidState
Language=English
This operation cannot be performed on the print job in its current state.
.
Language=Russian
This operation cannot be performed on the print job in its current state.
.

MessageId=2165
Severity=Success
Facility=System
SymbolicName=NERR_SpoolNoMemory
Language=English
A spooler memory allocation failure occurred.
.
Language=Russian
A spooler memory allocation failure occurred.
.

MessageId=2166
Severity=Success
Facility=System
SymbolicName=NERR_DriverNotFound
Language=English
The device driver does not exist.
.
Language=Russian
The device driver does not exist.
.

MessageId=2167
Severity=Success
Facility=System
SymbolicName=NERR_DataTypeInvalid
Language=English
The data type is not supported by the print processor.
.
Language=Russian
The data type is not supported by the print processor.
.

MessageId=2168
Severity=Success
Facility=System
SymbolicName=NERR_ProcNotFound
Language=English
The print processor is not installed.
.
Language=Russian
The print processor is not installed.
.

MessageId=2180
Severity=Success
Facility=System
SymbolicName=NERR_ServiceTableLocked
Language=English
The service database is locked.
.
Language=Russian
The service database is locked.
.

MessageId=2181
Severity=Success
Facility=System
SymbolicName=NERR_ServiceTableFull
Language=English
The service table is full.
.
Language=Russian
The service table is full.
.

MessageId=2182
Severity=Success
Facility=System
SymbolicName=NERR_ServiceInstalled
Language=English
The requested service has already been started.
.
Language=Russian
The requested service has already been started.
.

MessageId=2183
Severity=Success
Facility=System
SymbolicName=NERR_ServiceEntryLocked
Language=English
The service does not respond to control actions.
.
Language=Russian
The service does not respond to control actions.
.

MessageId=2184
Severity=Success
Facility=System
SymbolicName=NERR_ServiceNotInstalled
Language=English
The service has not been started.
.
Language=Russian
The service has not been started.
.

MessageId=2185
Severity=Success
Facility=System
SymbolicName=NERR_BadServiceName
Language=English
The service name is invalid.
.
Language=Russian
The service name is invalid.
.

MessageId=2186
Severity=Success
Facility=System
SymbolicName=NERR_ServiceCtlTimeout
Language=English
The service is not responding to the control function.
.
Language=Russian
The service is not responding to the control function.
.

MessageId=2187
Severity=Success
Facility=System
SymbolicName=NERR_ServiceCtlBusy
Language=English
The service control is busy.
.
Language=Russian
The service control is busy.
.

MessageId=2188
Severity=Success
Facility=System
SymbolicName=NERR_BadServiceProgName
Language=English
The configuration file contains an invalid service program name.
.
Language=Russian
The configuration file contains an invalid service program name.
.

MessageId=2189
Severity=Success
Facility=System
SymbolicName=NERR_ServiceNotCtrl
Language=English
The service could not be controlled in its present state.
.
Language=Russian
The service could not be controlled in its present state.
.

MessageId=2190
Severity=Success
Facility=System
SymbolicName=NERR_ServiceKillProc
Language=English
The service ended abnormally.
.
Language=Russian
The service ended abnormally.
.

MessageId=2191
Severity=Success
Facility=System
SymbolicName=NERR_ServiceCtlNotValid
Language=English
The requested pause, continue, or stop is not valid for this service.
.
Language=Russian
The requested pause, continue, or stop is not valid for this service.
.

MessageId=2192
Severity=Success
Facility=System
SymbolicName=NERR_NotInDispatchTbl
Language=English
The service control dispatcher could not find the service name in the dispatch table.
.
Language=Russian
The service control dispatcher could not find the service name in the dispatch table.
.

MessageId=2193
Severity=Success
Facility=System
SymbolicName=NERR_BadControlRecv
Language=English
The service control dispatcher pipe read failed.
.
Language=Russian
The service control dispatcher pipe read failed.
.

MessageId=2194
Severity=Success
Facility=System
SymbolicName=NERR_ServiceNotStarting
Language=English
A thread for the new service could not be created.
.
Language=Russian
A thread for the new service could not be created.
.

MessageId=2200
Severity=Success
Facility=System
SymbolicName=NERR_AlreadyLoggedOn
Language=English
This workstation is already logged on to the local-area network.
.
Language=Russian
This workstation is already logged on to the local-area network.
.

MessageId=2201
Severity=Success
Facility=System
SymbolicName=NERR_NotLoggedOn
Language=English
The workstation is not logged on to the local-area network.
.
Language=Russian
The workstation is not logged on to the local-area network.
.

MessageId=2202
Severity=Success
Facility=System
SymbolicName=NERR_BadUsername
Language=English
The user name or group name parameter is invalid.
.
Language=Russian
The user name or group name parameter is invalid.
.

MessageId=2203
Severity=Success
Facility=System
SymbolicName=NERR_BadPassword
Language=English
The password parameter is invalid.
.
Language=Russian
The password parameter is invalid.
.

MessageId=2204
Severity=Success
Facility=System
SymbolicName=NERR_UnableToAddName_W
Language=English
The logon processor did not add the message alias.
.
Language=Russian
The logon processor did not add the message alias.
.

MessageId=2205
Severity=Success
Facility=System
SymbolicName=NERR_UnableToAddName_F
Language=English
The logon processor did not add the message alias.
.
Language=Russian
The logon processor did not add the message alias.
.

MessageId=2206
Severity=Success
Facility=System
SymbolicName=NERR_UnableToDelName_W
Language=English
The logoff processor did not delete the message alias.
.
Language=Russian
The logoff processor did not delete the message alias.
.

MessageId=2207
Severity=Success
Facility=System
SymbolicName=NERR_UnableToDelName_F
Language=English
The logoff processor did not delete the message alias.
.
Language=Russian
The logoff processor did not delete the message alias.
.

MessageId=2209
Severity=Success
Facility=System
SymbolicName=NERR_LogonsPaused
Language=English
Network logons are paused.
.
Language=Russian
Network logons are paused.
.

MessageId=2210
Severity=Success
Facility=System
SymbolicName=NERR_LogonServerConflict
Language=English
A centralized logon-server conflict occurred.
.
Language=Russian
A centralized logon-server conflict occurred.
.

MessageId=2211
Severity=Success
Facility=System
SymbolicName=NERR_LogonNoUserPath
Language=English
The server is configured without a valid user path.
.
Language=Russian
The server is configured without a valid user path.
.

MessageId=2212
Severity=Success
Facility=System
SymbolicName=NERR_LogonScriptError
Language=English
An error occurred while loading or running the logon script.
.
Language=Russian
An error occurred while loading or running the logon script.
.

MessageId=2214
Severity=Success
Facility=System
SymbolicName=NERR_StandaloneLogon
Language=English
The logon server was not specified.  Your computer will be logged on as STANDALONE.
.
Language=Russian
The logon server was not specified.  Your computer will be logged on as STANDALONE.
.

MessageId=2215
Severity=Success
Facility=System
SymbolicName=NERR_LogonServerNotFound
Language=English
The logon server could not be found.
.
Language=Russian
The logon server could not be found.
.

MessageId=2216
Severity=Success
Facility=System
SymbolicName=NERR_LogonDomainExists
Language=English
There is already a logon domain for this computer.
.
Language=Russian
There is already a logon domain for this computer.
.

MessageId=2217
Severity=Success
Facility=System
SymbolicName=NERR_NonValidatedLogon
Language=English
The logon server could not validate the logon.
.
Language=Russian
The logon server could not validate the logon.
.

MessageId=2219
Severity=Success
Facility=System
SymbolicName=NERR_ACFNotFound
Language=English
The security database could not be found.
.
Language=Russian
The security database could not be found.
.

MessageId=2220
Severity=Success
Facility=System
SymbolicName=NERR_GroupNotFound
Language=English
The group name could not be found.
.
Language=Russian
The group name could not be found.
.

MessageId=2221
Severity=Success
Facility=System
SymbolicName=NERR_UserNotFound
Language=English
The user name could not be found.
.
Language=Russian
The user name could not be found.
.

MessageId=2222
Severity=Success
Facility=System
SymbolicName=NERR_ResourceNotFound
Language=English
The resource name could not be found.
.
Language=Russian
The resource name could not be found.
.

MessageId=2223
Severity=Success
Facility=System
SymbolicName=NERR_GroupExists
Language=English
The group already exists.
.
Language=Russian
The group already exists.
.

MessageId=2224
Severity=Success
Facility=System
SymbolicName=NERR_UserExists
Language=English
The account already exists.
.
Language=Russian
The account already exists.
.

MessageId=2225
Severity=Success
Facility=System
SymbolicName=NERR_ResourceExists
Language=English
The resource permission list already exists.
.
Language=Russian
The resource permission list already exists.
.

MessageId=2226
Severity=Success
Facility=System
SymbolicName=NERR_NotPrimary
Language=English
This operation is only allowed on the primary domain controller of the domain.
.
Language=Russian
This operation is only allowed on the primary domain controller of the domain.
.

MessageId=2227
Severity=Success
Facility=System
SymbolicName=NERR_ACFNotLoaded
Language=English
The security database has not been started.
.
Language=Russian
The security database has not been started.
.

MessageId=2228
Severity=Success
Facility=System
SymbolicName=NERR_ACFNoRoom
Language=English
There are too many names in the user accounts database.
.
Language=Russian
There are too many names in the user accounts database.
.

MessageId=2229
Severity=Success
Facility=System
SymbolicName=NERR_ACFFileIOFail
Language=English
A disk I/O failure occurred.
.
Language=Russian
A disk I/O failure occurred.
.

MessageId=2230
Severity=Success
Facility=System
SymbolicName=NERR_ACFTooManyLists
Language=English
The limit of 64 entries per resource was exceeded.
.
Language=Russian
The limit of 64 entries per resource was exceeded.
.

MessageId=2231
Severity=Success
Facility=System
SymbolicName=NERR_UserLogon
Language=English
Deleting a user with a session is not allowed.
.
Language=Russian
Deleting a user with a session is not allowed.
.

MessageId=2232
Severity=Success
Facility=System
SymbolicName=NERR_ACFNoParent
Language=English
The parent directory could not be located.
.
Language=Russian
The parent directory could not be located.
.

MessageId=2233
Severity=Success
Facility=System
SymbolicName=NERR_CanNotGrowSegment
Language=English
Unable to add to the security database session cache segment.
.
Language=Russian
Unable to add to the security database session cache segment.
.

MessageId=2234
Severity=Success
Facility=System
SymbolicName=NERR_SpeGroupOp
Language=English
This operation is not allowed on this special group.
.
Language=Russian
This operation is not allowed on this special group.
.

MessageId=2235
Severity=Success
Facility=System
SymbolicName=NERR_NotInCache
Language=English
This user is not cached in user accounts database session cache.
.
Language=Russian
This user is not cached in user accounts database session cache.
.

MessageId=2236
Severity=Success
Facility=System
SymbolicName=NERR_UserInGroup
Language=English
The user already belongs to this group.
.
Language=Russian
The user already belongs to this group.
.

MessageId=2237
Severity=Success
Facility=System
SymbolicName=NERR_UserNotInGroup
Language=English
The user does not belong to this group.
.
Language=Russian
The user does not belong to this group.
.

MessageId=2238
Severity=Success
Facility=System
SymbolicName=NERR_AccountUndefined
Language=English
This user account is undefined.
.
Language=Russian
This user account is undefined.
.

MessageId=2239
Severity=Success
Facility=System
SymbolicName=NERR_AccountExpired
Language=English
This user account has expired.
.
Language=Russian
This user account has expired.
.

MessageId=2240
Severity=Success
Facility=System
SymbolicName=NERR_InvalidWorkstation
Language=English
The user is not allowed to log on from this workstation.
.
Language=Russian
The user is not allowed to log on from this workstation.
.

MessageId=2241
Severity=Success
Facility=System
SymbolicName=NERR_InvalidLogonHours
Language=English
The user is not allowed to log on at this time.
.
Language=Russian
The user is not allowed to log on at this time.
.

MessageId=2242
Severity=Success
Facility=System
SymbolicName=NERR_PasswordExpired
Language=English
The password of this user has expired.
.
Language=Russian
The password of this user has expired.
.

MessageId=2243
Severity=Success
Facility=System
SymbolicName=NERR_PasswordCantChange
Language=English
The password of this user cannot change.
.
Language=Russian
The password of this user cannot change.
.

MessageId=2244
Severity=Success
Facility=System
SymbolicName=NERR_PasswordHistConflict
Language=English
This password cannot be used now.
.
Language=Russian
This password cannot be used now.
.

MessageId=2245
Severity=Success
Facility=System
SymbolicName=NERR_PasswordTooShort
Language=English
The password does not meet the password policy requirements. Check the minimum password length, password complexity and password history requirements.
.
Language=Russian
The password does not meet the password policy requirements. Check the minimum password length, password complexity and password history requirements.
.

MessageId=2246
Severity=Success
Facility=System
SymbolicName=NERR_PasswordTooRecent
Language=English
The password of this user is too recent to change.
.
Language=Russian
The password of this user is too recent to change.
.

MessageId=2247
Severity=Success
Facility=System
SymbolicName=NERR_InvalidDatabase
Language=English
The security database is corrupted.
.
Language=Russian
The security database is corrupted.
.

MessageId=2248
Severity=Success
Facility=System
SymbolicName=NERR_DatabaseUpToDate
Language=English
No updates are necessary to this replicant network/local security database.
.
Language=Russian
No updates are necessary to this replicant network/local security database.
.

MessageId=2249
Severity=Success
Facility=System
SymbolicName=NERR_SyncRequired
Language=English
This replicant database is outdated; synchronization is required.
.
Language=Russian
This replicant database is outdated; synchronization is required.
.

MessageId=2250
Severity=Success
Facility=System
SymbolicName=NERR_UseNotFound
Language=English
The network connection could not be found.
.
Language=Russian
The network connection could not be found.
.

MessageId=2251
Severity=Success
Facility=System
SymbolicName=NERR_BadAsgType
Language=English
This asg_type is invalid.
.
Language=Russian
This asg_type is invalid.
.

MessageId=2252
Severity=Success
Facility=System
SymbolicName=NERR_DeviceIsShared
Language=English
This device is currently being shared.
.
Language=Russian
This device is currently being shared.
.

MessageId=2270
Severity=Success
Facility=System
SymbolicName=NERR_NoComputerName
Language=English
The computer name could not be added as a message alias.  The name may already exist on the network.
.
Language=Russian
The computer name could not be added as a message alias.  The name may already exist on the network.
.

MessageId=2271
Severity=Success
Facility=System
SymbolicName=NERR_MsgAlreadyStarted
Language=English
The Messenger service is already started.
.
Language=Russian
The Messenger service is already started.
.

MessageId=2272
Severity=Success
Facility=System
SymbolicName=NERR_MsgInitFailed
Language=English
The Messenger service failed to start.
.
Language=Russian
The Messenger service failed to start.
.

MessageId=2273
Severity=Success
Facility=System
SymbolicName=NERR_NameNotFound
Language=English
The message alias could not be found on the network.
.
Language=Russian
The message alias could not be found on the network.
.

MessageId=2274
Severity=Success
Facility=System
SymbolicName=NERR_AlreadyForwarded
Language=English
This message alias has already been forwarded.
.
Language=Russian
This message alias has already been forwarded.
.

MessageId=2275
Severity=Success
Facility=System
SymbolicName=NERR_AddForwarded
Language=English
This message alias has been added but is still forwarded.
.
Language=Russian
This message alias has been added but is still forwarded.
.

MessageId=2276
Severity=Success
Facility=System
SymbolicName=NERR_AlreadyExists
Language=English
This message alias already exists locally.
.
Language=Russian
This message alias already exists locally.
.

MessageId=2277
Severity=Success
Facility=System
SymbolicName=NERR_TooManyNames
Language=English
The maximum number of added message aliases has been exceeded.
.
Language=Russian
The maximum number of added message aliases has been exceeded.
.

MessageId=2278
Severity=Success
Facility=System
SymbolicName=NERR_DelComputerName
Language=English
The computer name could not be deleted.
.
Language=Russian
The computer name could not be deleted.
.

MessageId=2279
Severity=Success
Facility=System
SymbolicName=NERR_LocalForward
Language=English
Messages cannot be forwarded back to the same workstation.
.
Language=Russian
Messages cannot be forwarded back to the same workstation.
.

MessageId=2280
Severity=Success
Facility=System
SymbolicName=NERR_GrpMsgProcessor
Language=English
An error occurred in the domain message processor.
.
Language=Russian
An error occurred in the domain message processor.
.

MessageId=2281
Severity=Success
Facility=System
SymbolicName=NERR_PausedRemote
Language=English
The message was sent, but the recipient has paused the Messenger service.
.
Language=Russian
The message was sent, but the recipient has paused the Messenger service.
.

MessageId=2282
Severity=Success
Facility=System
SymbolicName=NERR_BadReceive
Language=English
The message was sent but not received.
.
Language=Russian
The message was sent but not received.
.

MessageId=2283
Severity=Success
Facility=System
SymbolicName=NERR_NameInUse
Language=English
The message alias is currently in use. Try again later.
.
Language=Russian
The message alias is currently in use. Try again later.
.

MessageId=2284
Severity=Success
Facility=System
SymbolicName=NERR_MsgNotStarted
Language=English
The Messenger service has not been started.
.
Language=Russian
The Messenger service has not been started.
.

MessageId=2285
Severity=Success
Facility=System
SymbolicName=NERR_NotLocalName
Language=English
The name is not on the local computer.
.
Language=Russian
The name is not on the local computer.
.

MessageId=2286
Severity=Success
Facility=System
SymbolicName=NERR_NoForwardName
Language=English
The forwarded message alias could not be found on the network.
.
Language=Russian
The forwarded message alias could not be found on the network.
.

MessageId=2287
Severity=Success
Facility=System
SymbolicName=NERR_RemoteFull
Language=English
The message alias table on the remote station is full.
.
Language=Russian
The message alias table on the remote station is full.
.

MessageId=2288
Severity=Success
Facility=System
SymbolicName=NERR_NameNotForwarded
Language=English
Messages for this alias are not currently being forwarded.
.
Language=Russian
Messages for this alias are not currently being forwarded.
.

MessageId=2289
Severity=Success
Facility=System
SymbolicName=NERR_TruncatedBroadcast
Language=English
The broadcast message was truncated.
.
Language=Russian
The broadcast message was truncated.
.

MessageId=2294
Severity=Success
Facility=System
SymbolicName=NERR_InvalidDevice
Language=English
This is an invalid device name.
.
Language=Russian
This is an invalid device name.
.

MessageId=2295
Severity=Success
Facility=System
SymbolicName=NERR_WriteFault
Language=English
A write fault occurred.
.
Language=Russian
A write fault occurred.
.

MessageId=2297
Severity=Success
Facility=System
SymbolicName=NERR_DuplicateName
Language=English
A duplicate message alias exists on the network.
.
Language=Russian
A duplicate message alias exists on the network.
.

MessageId=2298
Severity=Success
Facility=System
SymbolicName=NERR_DeleteLater
Language=English
This message alias will be deleted later.
.
Language=Russian
This message alias will be deleted later.
.

MessageId=2299
Severity=Success
Facility=System
SymbolicName=NERR_IncompleteDel
Language=English
The message alias was not successfully deleted from all networks.
.
Language=Russian
The message alias was not successfully deleted from all networks.
.

MessageId=2300
Severity=Success
Facility=System
SymbolicName=NERR_MultipleNets
Language=English
This operation is not supported on computers with multiple networks.
.
Language=Russian
This operation is not supported on computers with multiple networks.
.

MessageId=2310
Severity=Success
Facility=System
SymbolicName=NERR_NetNameNotFound
Language=English
This shared resource does not exist.
.
Language=Russian
This shared resource does not exist.
.

MessageId=2311
Severity=Success
Facility=System
SymbolicName=NERR_DeviceNotShared
Language=English
This device is not shared.
.
Language=Russian
This device is not shared.
.

MessageId=2312
Severity=Success
Facility=System
SymbolicName=NERR_ClientNameNotFound
Language=English
A session does not exist with that computer name.
.
Language=Russian
A session does not exist with that computer name.
.

MessageId=2314
Severity=Success
Facility=System
SymbolicName=NERR_FileIdNotFound
Language=English
There is not an open file with that identification number.
.
Language=Russian
There is not an open file with that identification number.
.

MessageId=2315
Severity=Success
Facility=System
SymbolicName=NERR_ExecFailure
Language=English
A failure occurred when executing a remote administration command.
.
Language=Russian
A failure occurred when executing a remote administration command.
.

MessageId=2316
Severity=Success
Facility=System
SymbolicName=NERR_TmpFile
Language=English
A failure occurred when opening a remote temporary file.
.
Language=Russian
A failure occurred when opening a remote temporary file.
.

MessageId=2317
Severity=Success
Facility=System
SymbolicName=NERR_TooMuchData
Language=English
The data returned from a remote administration command has been truncated to 64K.
.
Language=Russian
The data returned from a remote administration command has been truncated to 64K.
.

MessageId=2318
Severity=Success
Facility=System
SymbolicName=NERR_DeviceShareConflict
Language=English
This device cannot be shared as both a spooled and a non-spooled resource.
.
Language=Russian
This device cannot be shared as both a spooled and a non-spooled resource.
.

MessageId=2319
Severity=Success
Facility=System
SymbolicName=NERR_BrowserTableIncomplete
Language=English
The information in the list of servers may be incorrect.
.
Language=Russian
The information in the list of servers may be incorrect.
.

MessageId=2320
Severity=Success
Facility=System
SymbolicName=NERR_NotLocalDomain
Language=English
The computer is not active in this domain.
.
Language=Russian
The computer is not active in this domain.
.

MessageId=2321
Severity=Success
Facility=System
SymbolicName=NERR_IsDfsShare
Language=English
The share must be removed from the Distributed File System before it can be deleted.
.
Language=Russian
The share must be removed from the Distributed File System before it can be deleted.
.

MessageId=2331
Severity=Success
Facility=System
SymbolicName=NERR_DevInvalidOpCode
Language=English
The operation is invalid for this device.
.
Language=Russian
The operation is invalid for this device.
.

MessageId=2332
Severity=Success
Facility=System
SymbolicName=NERR_DevNotFound
Language=English
This device cannot be shared.
.
Language=Russian
This device cannot be shared.
.

MessageId=2333
Severity=Success
Facility=System
SymbolicName=NERR_DevNotOpen
Language=English
This device was not open.
.
Language=Russian
This device was not open.
.

MessageId=2334
Severity=Success
Facility=System
SymbolicName=NERR_BadQueueDevString
Language=English
This device name list is invalid.
.
Language=Russian
This device name list is invalid.
.

MessageId=2335
Severity=Success
Facility=System
SymbolicName=NERR_BadQueuePriority
Language=English
The queue priority is invalid.
.
Language=Russian
The queue priority is invalid.
.

MessageId=2337
Severity=Success
Facility=System
SymbolicName=NERR_NoCommDevs
Language=English
There are no shared communication devices.
.
Language=Russian
There are no shared communication devices.
.

MessageId=2338
Severity=Success
Facility=System
SymbolicName=NERR_QueueNotFound
Language=English
The queue you specified does not exist.
.
Language=Russian
The queue you specified does not exist.
.

MessageId=2340
Severity=Success
Facility=System
SymbolicName=NERR_BadDevString
Language=English
This list of devices is invalid.
.
Language=Russian
This list of devices is invalid.
.

MessageId=2341
Severity=Success
Facility=System
SymbolicName=NERR_BadDev
Language=English
The requested device is invalid.
.
Language=Russian
The requested device is invalid.
.

MessageId=2342
Severity=Success
Facility=System
SymbolicName=NERR_InUseBySpooler
Language=English
This device is already in use by the spooler.
.
Language=Russian
This device is already in use by the spooler.
.

MessageId=2343
Severity=Success
Facility=System
SymbolicName=NERR_CommDevInUse
Language=English
This device is already in use as a communication device.
.
Language=Russian
This device is already in use as a communication device.
.

MessageId=2351
Severity=Success
Facility=System
SymbolicName=NERR_InvalidComputer
Language=English
This computer name is invalid.
.
Language=Russian
This computer name is invalid.
.

MessageId=2354
Severity=Success
Facility=System
SymbolicName=NERR_MaxLenExceeded
Language=English
The string and prefix specified are too long.
.
Language=Russian
The string and prefix specified are too long.
.

MessageId=2356
Severity=Success
Facility=System
SymbolicName=NERR_BadComponent
Language=English
This path component is invalid.
.
Language=Russian
This path component is invalid.
.

MessageId=2357
Severity=Success
Facility=System
SymbolicName=NERR_CantType
Language=English
Could not determine the type of input.
.
Language=Russian
Could not determine the type of input.
.

MessageId=2362
Severity=Success
Facility=System
SymbolicName=NERR_TooManyEntries
Language=English
The buffer for types is not big enough.
.
Language=Russian
The buffer for types is not big enough.
.

MessageId=2370
Severity=Success
Facility=System
SymbolicName=NERR_ProfileFileTooBig
Language=English
Profile files cannot exceed 64K.
.
Language=Russian
Profile files cannot exceed 64K.
.

MessageId=2371
Severity=Success
Facility=System
SymbolicName=NERR_ProfileOffset
Language=English
The start offset is out of range.
.
Language=Russian
The start offset is out of range.
.

MessageId=2372
Severity=Success
Facility=System
SymbolicName=NERR_ProfileCleanup
Language=English
The system cannot delete current connections to network resources.
.
Language=Russian
The system cannot delete current connections to network resources.
.

MessageId=2373
Severity=Success
Facility=System
SymbolicName=NERR_ProfileUnknownCmd
Language=English
The system was unable to parse the command line in this file.
.
Language=Russian
The system was unable to parse the command line in this file.
.

MessageId=2374
Severity=Success
Facility=System
SymbolicName=NERR_ProfileLoadErr
Language=English
An error occurred while loading the profile file.
.
Language=Russian
An error occurred while loading the profile file.
.

MessageId=2375
Severity=Success
Facility=System
SymbolicName=NERR_ProfileSaveErr
Language=English
Errors occurred while saving the profile file.  The profile was partially saved.
.
Language=Russian
Errors occurred while saving the profile file.  The profile was partially saved.
.

MessageId=2377
Severity=Success
Facility=System
SymbolicName=NERR_LogOverflow
Language=English
Log file %1 is full.
.
Language=Russian
Log file %1 is full.
.

MessageId=2378
Severity=Success
Facility=System
SymbolicName=NERR_LogFileChanged
Language=English
This log file has changed between reads.
.
Language=Russian
This log file has changed between reads.
.

MessageId=2379
Severity=Success
Facility=System
SymbolicName=NERR_LogFileCorrupt
Language=English
Log file %1 is corrupt.
.
Language=Russian
Log file %1 is corrupt.
.

MessageId=2380
Severity=Success
Facility=System
SymbolicName=NERR_SourceIsDir
Language=English
The source path cannot be a directory.
.
Language=Russian
The source path cannot be a directory.
.

MessageId=2381
Severity=Success
Facility=System
SymbolicName=NERR_BadSource
Language=English
The source path is illegal.
.
Language=Russian
The source path is illegal.
.

MessageId=2382
Severity=Success
Facility=System
SymbolicName=NERR_BadDest
Language=English
The destination path is illegal.
.
Language=Russian
The destination path is illegal.
.

MessageId=2383
Severity=Success
Facility=System
SymbolicName=NERR_DifferentServers
Language=English
The source and destination paths are on different servers.
.
Language=Russian
The source and destination paths are on different servers.
.

MessageId=2385
Severity=Success
Facility=System
SymbolicName=NERR_RunSrvPaused
Language=English
The Run server you requested is paused.
.
Language=Russian
The Run server you requested is paused.
.

MessageId=2389
Severity=Success
Facility=System
SymbolicName=NERR_ErrCommRunSrv
Language=English
An error occurred when communicating with a Run server.
.
Language=Russian
An error occurred when communicating with a Run server.
.

MessageId=2391
Severity=Success
Facility=System
SymbolicName=NERR_ErrorExecingGhost
Language=English
An error occurred when starting a background process.
.
Language=Russian
An error occurred when starting a background process.
.

MessageId=2392
Severity=Success
Facility=System
SymbolicName=NERR_ShareNotFound
Language=English
The shared resource you are connected to could not be found.
.
Language=Russian
The shared resource you are connected to could not be found.
.

MessageId=2400
Severity=Success
Facility=System
SymbolicName=NERR_InvalidLana
Language=English
The LAN adapter number is invalid.
.
Language=Russian
The LAN adapter number is invalid.
.

MessageId=2401
Severity=Success
Facility=System
SymbolicName=NERR_OpenFiles
Language=English
There are open files on the connection.
.
Language=Russian
There are open files on the connection.
.

MessageId=2402
Severity=Success
Facility=System
SymbolicName=NERR_ActiveConns
Language=English
Active connections still exist.
.
Language=Russian
Active connections still exist.
.

MessageId=2403
Severity=Success
Facility=System
SymbolicName=NERR_BadPasswordCore
Language=English
This share name or password is invalid.
.
Language=Russian
This share name or password is invalid.
.

MessageId=2404
Severity=Success
Facility=System
SymbolicName=NERR_DevInUse
Language=English
The device is being accessed by an active process.
.
Language=Russian
The device is being accessed by an active process.
.

MessageId=2405
Severity=Success
Facility=System
SymbolicName=NERR_LocalDrive
Language=English
The drive letter is in use locally.
.
Language=Russian
The drive letter is in use locally.
.

MessageId=2430
Severity=Success
Facility=System
SymbolicName=NERR_AlertExists
Language=English
The specified client is already registered for the specified event.
.
Language=Russian
The specified client is already registered for the specified event.
.

MessageId=2431
Severity=Success
Facility=System
SymbolicName=NERR_TooManyAlerts
Language=English
The alert table is full.
.
Language=Russian
The alert table is full.
.

MessageId=2432
Severity=Success
Facility=System
SymbolicName=NERR_NoSuchAlert
Language=English
An invalid or nonexistent alert name was raised.
.
Language=Russian
An invalid or nonexistent alert name was raised.
.

MessageId=2433
Severity=Success
Facility=System
SymbolicName=NERR_BadRecipient
Language=English
The alert recipient is invalid.
.
Language=Russian
The alert recipient is invalid.
.

MessageId=2434
Severity=Success
Facility=System
SymbolicName=NERR_AcctLimitExceeded
Language=English
A user's session with this server has been deleted\nbecause the user's logon hours are no longer valid.
.
Language=Russian
A user's session with this server has been deleted\nbecause the user's logon hours are no longer valid.
.

MessageId=2440
Severity=Success
Facility=System
SymbolicName=NERR_InvalidLogSeek
Language=English
The log file does not contain the requested record number.
.
Language=Russian
The log file does not contain the requested record number.
.

MessageId=2450
Severity=Success
Facility=System
SymbolicName=NERR_BadUasConfig
Language=English
The user accounts database is not configured correctly.
.
Language=Russian
The user accounts database is not configured correctly.
.

MessageId=2451
Severity=Success
Facility=System
SymbolicName=NERR_InvalidUASOp
Language=English
This operation is not permitted when the Netlogon service is running.
.
Language=Russian
This operation is not permitted when the Netlogon service is running.
.

MessageId=2452
Severity=Success
Facility=System
SymbolicName=NERR_LastAdmin
Language=English
This operation is not allowed on the last administrative account.
.
Language=Russian
This operation is not allowed on the last administrative account.
.

MessageId=2453
Severity=Success
Facility=System
SymbolicName=NERR_DCNotFound
Language=English
Could not find domain controller for this domain.
.
Language=Russian
Could not find domain controller for this domain.
.

MessageId=2454
Severity=Success
Facility=System
SymbolicName=NERR_LogonTrackingError
Language=English
Could not set logon information for this user.
.
Language=Russian
Could not set logon information for this user.
.

MessageId=2455
Severity=Success
Facility=System
SymbolicName=NERR_NetlogonNotStarted
Language=English
The Netlogon service has not been started.
.
Language=Russian
The Netlogon service has not been started.
.

MessageId=2456
Severity=Success
Facility=System
SymbolicName=NERR_CanNotGrowUASFile
Language=English
Unable to add to the user accounts database.
.
Language=Russian
Unable to add to the user accounts database.
.

MessageId=2457
Severity=Success
Facility=System
SymbolicName=NERR_TimeDiffAtDC
Language=English
This server's clock is not synchronized with the primary domain controller's clock.
.
Language=Russian
This server's clock is not synchronized with the primary domain controller's clock.
.

MessageId=2458
Severity=Success
Facility=System
SymbolicName=NERR_PasswordMismatch
Language=English
A password mismatch has been detected.
.
Language=Russian
A password mismatch has been detected.
.

MessageId=2460
Severity=Success
Facility=System
SymbolicName=NERR_NoSuchServer
Language=English
The server identification does not specify a valid server.
.
Language=Russian
The server identification does not specify a valid server.
.

MessageId=2461
Severity=Success
Facility=System
SymbolicName=NERR_NoSuchSession
Language=English
The session identification does not specify a valid session.
.
Language=Russian
The session identification does not specify a valid session.
.

MessageId=2462
Severity=Success
Facility=System
SymbolicName=NERR_NoSuchConnection
Language=English
The connection identification does not specify a valid connection.
.
Language=Russian
The connection identification does not specify a valid connection.
.

MessageId=2463
Severity=Success
Facility=System
SymbolicName=NERR_TooManyServers
Language=English
There is no space for another entry in the table of available servers.
.
Language=Russian
There is no space for another entry in the table of available servers.
.

MessageId=2464
Severity=Success
Facility=System
SymbolicName=NERR_TooManySessions
Language=English
The server has reached the maximum number of sessions it supports.
.
Language=Russian
The server has reached the maximum number of sessions it supports.
.

MessageId=2465
Severity=Success
Facility=System
SymbolicName=NERR_TooManyConnections
Language=English
The server has reached the maximum number of connections it supports.
.
Language=Russian
The server has reached the maximum number of connections it supports.
.

MessageId=2466
Severity=Success
Facility=System
SymbolicName=NERR_TooManyFiles
Language=English
The server cannot open more files because it has reached its maximum number.
.
Language=Russian
The server cannot open more files because it has reached its maximum number.
.

MessageId=2467
Severity=Success
Facility=System
SymbolicName=NERR_NoAlternateServers
Language=English
There are no alternate servers registered on this server.
.
Language=Russian
There are no alternate servers registered on this server.
.

MessageId=2470
Severity=Success
Facility=System
SymbolicName=NERR_TryDownLevel
Language=English
Try down-level (remote admin protocol) version of API instead.
.
Language=Russian
Try down-level (remote admin protocol) version of API instead.
.

MessageId=2480
Severity=Success
Facility=System
SymbolicName=NERR_UPSDriverNotStarted
Language=English
The UPS driver could not be accessed by the UPS service.
.
Language=Russian
The UPS driver could not be accessed by the UPS service.
.

MessageId=2481
Severity=Success
Facility=System
SymbolicName=NERR_UPSInvalidConfig
Language=English
The UPS service is not configured correctly.
.
Language=Russian
The UPS service is not configured correctly.
.

MessageId=2482
Severity=Success
Facility=System
SymbolicName=NERR_UPSInvalidCommPort
Language=English
The UPS service could not access the specified Comm Port.
.
Language=Russian
The UPS service could not access the specified Comm Port.
.

MessageId=2483
Severity=Success
Facility=System
SymbolicName=NERR_UPSSignalAsserted
Language=English
The UPS indicated a line fail or low battery situation. Service not started.
.
Language=Russian
The UPS indicated a line fail or low battery situation. Service not started.
.

MessageId=2484
Severity=Success
Facility=System
SymbolicName=NERR_UPSShutdownFailed
Language=English
The UPS service failed to perform a system shut down.
.
Language=Russian
The UPS service failed to perform a system shut down.
.

MessageId=2500
Severity=Success
Facility=System
SymbolicName=NERR_BadDosRetCode
Language=English
The program below returned a DOS error code:
.
Language=Russian
The program below returned a DOS error code:
.

MessageId=2501
Severity=Success
Facility=System
SymbolicName=NERR_ProgNeedsExtraMem
Language=English
The program below needs more memory:
.
Language=Russian
The program below needs more memory:
.

MessageId=2502
Severity=Success
Facility=System
SymbolicName=NERR_BadDosFunction
Language=English
The program below called an unsupported DOS function:
.
Language=Russian
The program below called an unsupported DOS function:
.

MessageId=2503
Severity=Success
Facility=System
SymbolicName=NERR_RemoteBootFailed
Language=English
The workstation failed to boot.
.
Language=Russian
The workstation failed to boot.
.

MessageId=2504
Severity=Success
Facility=System
SymbolicName=NERR_BadFileCheckSum
Language=English
The file below is corrupt.
.
Language=Russian
The file below is corrupt.
.

MessageId=2505
Severity=Success
Facility=System
SymbolicName=NERR_NoRplBootSystem
Language=English
No loader is specified in the boot-block definition file.
.
Language=Russian
No loader is specified in the boot-block definition file.
.

MessageId=2506
Severity=Success
Facility=System
SymbolicName=NERR_RplLoadrNetBiosErr
Language=English
NetBIOS returned an error: The NCB and SMB are dumped above.
.
Language=Russian
NetBIOS returned an error: The NCB and SMB are dumped above.
.

MessageId=2507
Severity=Success
Facility=System
SymbolicName=NERR_RplLoadrDiskErr
Language=English
A disk I/O error occurred.
.
Language=Russian
A disk I/O error occurred.
.

MessageId=2508
Severity=Success
Facility=System
SymbolicName=NERR_ImageParamErr
Language=English
Image parameter substitution failed.
.
Language=Russian
Image parameter substitution failed.
.

MessageId=2509
Severity=Success
Facility=System
SymbolicName=NERR_TooManyImageParams
Language=English
Too many image parameters cross disk sector boundaries.
.
Language=Russian
Too many image parameters cross disk sector boundaries.
.

MessageId=2510
Severity=Success
Facility=System
SymbolicName=NERR_NonDosFloppyUsed
Language=English
The image was not generated from a DOS diskette formatted with /S.
.
Language=Russian
The image was not generated from a DOS diskette formatted with /S.
.

MessageId=2511
Severity=Success
Facility=System
SymbolicName=NERR_RplBootRestart
Language=English
Remote boot will be restarted later.
.
Language=Russian
Remote boot will be restarted later.
.

MessageId=2512
Severity=Success
Facility=System
SymbolicName=NERR_RplSrvrCallFailed
Language=English
The call to the Remoteboot server failed.
.
Language=Russian
The call to the Remoteboot server failed.
.

MessageId=2513
Severity=Success
Facility=System
SymbolicName=NERR_CantConnectRplSrvr
Language=English
Cannot connect to the Remoteboot server.
.
Language=Russian
Cannot connect to the Remoteboot server.
.

MessageId=2514
Severity=Success
Facility=System
SymbolicName=NERR_CantOpenImageFile
Language=English
Cannot open image file on the Remoteboot server.
.
Language=Russian
Cannot open image file on the Remoteboot server.
.

MessageId=2515
Severity=Success
Facility=System
SymbolicName=NERR_CallingRplSrvr
Language=English
Connecting to the Remoteboot server...
.
Language=Russian
Connecting to the Remoteboot server...
.

MessageId=2516
Severity=Success
Facility=System
SymbolicName=NERR_StartingRplBoot
Language=English
Connecting to the Remoteboot server...
.
Language=Russian
Connecting to the Remoteboot server...
.

MessageId=2517
Severity=Success
Facility=System
SymbolicName=NERR_RplBootServiceTerm
Language=English
Remote boot service was stopped; check the error log for the cause of the problem.
.
Language=Russian
Remote boot service was stopped; check the error log for the cause of the problem.
.

MessageId=2518
Severity=Success
Facility=System
SymbolicName=NERR_RplBootStartFailed
Language=English
Remote boot startup failed; check the error log for the cause of the problem.
.
Language=Russian
Remote boot startup failed; check the error log for the cause of the problem.
.

MessageId=2519
Severity=Success
Facility=System
SymbolicName=NERR_RPL_CONNECTED
Language=English
A second connection to a Remoteboot resource is not allowed.
.
Language=Russian
A second connection to a Remoteboot resource is not allowed.
.

MessageId=2550
Severity=Success
Facility=System
SymbolicName=NERR_BrowserConfiguredToNotRun
Language=English
The browser service was configured with MaintainServerList=No.
.
Language=Russian
The browser service was configured with MaintainServerList=No.
.

MessageId=2610
Severity=Success
Facility=System
SymbolicName=NERR_RplNoAdaptersStarted
Language=English
Service failed to start since none of the network adapters started with this service.
.
Language=Russian
Service failed to start since none of the network adapters started with this service.
.

MessageId=2611
Severity=Success
Facility=System
SymbolicName=NERR_RplBadRegistry
Language=English
Service failed to start due to bad startup information in the registry.
.
Language=Russian
Service failed to start due to bad startup information in the registry.
.

MessageId=2612
Severity=Success
Facility=System
SymbolicName=NERR_RplBadDatabase
Language=English
Service failed to start because its database is absent or corrupt.
.
Language=Russian
Service failed to start because its database is absent or corrupt.
.

MessageId=2613
Severity=Success
Facility=System
SymbolicName=NERR_RplRplfilesShare
Language=English
Service failed to start because RPLFILES share is absent.
.
Language=Russian
Service failed to start because RPLFILES share is absent.
.

MessageId=2614
Severity=Success
Facility=System
SymbolicName=NERR_RplNotRplServer
Language=English
Service failed to start because RPLUSER group is absent.
.
Language=Russian
Service failed to start because RPLUSER group is absent.
.

MessageId=2615
Severity=Success
Facility=System
SymbolicName=NERR_RplCannotEnum
Language=English
Cannot enumerate service records.
.
Language=Russian
Cannot enumerate service records.
.

MessageId=2616
Severity=Success
Facility=System
SymbolicName=NERR_RplWkstaInfoCorrupted
Language=English
Workstation record information has been corrupted.
.
Language=Russian
Workstation record information has been corrupted.
.

MessageId=2617
Severity=Success
Facility=System
SymbolicName=NERR_RplWkstaNotFound
Language=English
Workstation record was not found.
.
Language=Russian
Workstation record was not found.
.

MessageId=2618
Severity=Success
Facility=System
SymbolicName=NERR_RplWkstaNameUnavailable
Language=English
Workstation name is in use by some other workstation.
.
Language=Russian
Workstation name is in use by some other workstation.
.

MessageId=2619
Severity=Success
Facility=System
SymbolicName=NERR_RplProfileInfoCorrupted
Language=English
Profile record information has been corrupted.
.
Language=Russian
Profile record information has been corrupted.
.

MessageId=2620
Severity=Success
Facility=System
SymbolicName=NERR_RplProfileNotFound
Language=English
Profile record was not found.
.
Language=Russian
Profile record was not found.
.

MessageId=2621
Severity=Success
Facility=System
SymbolicName=NERR_RplProfileNameUnavailable
Language=English
Profile name is in use by some other profile.
.
Language=Russian
Profile name is in use by some other profile.
.

MessageId=2622
Severity=Success
Facility=System
SymbolicName=NERR_RplProfileNotEmpty
Language=English
There are workstations using this profile.
.
Language=Russian
There are workstations using this profile.
.

MessageId=2623
Severity=Success
Facility=System
SymbolicName=NERR_RplConfigInfoCorrupted
Language=English
Configuration record information has been corrupted.
.
Language=Russian
Configuration record information has been corrupted.
.

MessageId=2624
Severity=Success
Facility=System
SymbolicName=NERR_RplConfigNotFound
Language=English
Configuration record was not found.
.
Language=Russian
Configuration record was not found.
.

MessageId=2625
Severity=Success
Facility=System
SymbolicName=NERR_RplAdapterInfoCorrupted
Language=English
Adapter id record information has been corrupted.
.
Language=Russian
Adapter id record information has been corrupted.
.

MessageId=2626
Severity=Success
Facility=System
SymbolicName=NERR_RplInternal
Language=English
An internal service error has occurred.
.
Language=Russian
An internal service error has occurred.
.

MessageId=2627
Severity=Success
Facility=System
SymbolicName=NERR_RplVendorInfoCorrupted
Language=English
Vendor id record information has been corrupted.
.
Language=Russian
Vendor id record information has been corrupted.
.

MessageId=2628
Severity=Success
Facility=System
SymbolicName=NERR_RplBootInfoCorrupted
Language=English
Boot block record information has been corrupted.
.
Language=Russian
Boot block record information has been corrupted.
.

MessageId=2629
Severity=Success
Facility=System
SymbolicName=NERR_RplWkstaNeedsUserAcct
Language=English
The user account for this workstation record is missing.
.
Language=Russian
The user account for this workstation record is missing.
.

MessageId=2630
Severity=Success
Facility=System
SymbolicName=NERR_RplNeedsRPLUSERAcct
Language=English
The RPLUSER local group could not be found.
.
Language=Russian
The RPLUSER local group could not be found.
.

MessageId=2631
Severity=Success
Facility=System
SymbolicName=NERR_RplBootNotFound
Language=English
Boot block record was not found.
.
Language=Russian
Boot block record was not found.
.

MessageId=2632
Severity=Success
Facility=System
SymbolicName=NERR_RplIncompatibleProfile
Language=English
Chosen profile is incompatible with this workstation.
.
Language=Russian
Chosen profile is incompatible with this workstation.
.

MessageId=2633
Severity=Success
Facility=System
SymbolicName=NERR_RplAdapterNameUnavailable
Language=English
Chosen network adapter id is in use by some other workstation.
.
Language=Russian
Chosen network adapter id is in use by some other workstation.
.

MessageId=2634
Severity=Success
Facility=System
SymbolicName=NERR_RplConfigNotEmpty
Language=English
There are profiles using this configuration.
.
Language=Russian
There are profiles using this configuration.
.

MessageId=2635
Severity=Success
Facility=System
SymbolicName=NERR_RplBootInUse
Language=English
There are workstations, profiles or configurations using this boot block.
.
Language=Russian
There are workstations, profiles or configurations using this boot block.
.

MessageId=2636
Severity=Success
Facility=System
SymbolicName=NERR_RplBackupDatabase
Language=English
Service failed to backup Remoteboot database.
.
Language=Russian
Service failed to backup Remoteboot database.
.

MessageId=2637
Severity=Success
Facility=System
SymbolicName=NERR_RplAdapterNotFound
Language=English
Adapter record was not found.
.
Language=Russian
Adapter record was not found.
.

MessageId=2638
Severity=Success
Facility=System
SymbolicName=NERR_RplVendorNotFound
Language=English
Vendor record was not found.
.
Language=Russian
Vendor record was not found.
.

MessageId=2639
Severity=Success
Facility=System
SymbolicName=NERR_RplVendorNameUnavailable
Language=English
Vendor name is in use by some other vendor record.
.
Language=Russian
Vendor name is in use by some other vendor record.
.

MessageId=2640
Severity=Success
Facility=System
SymbolicName=NERR_RplBootNameUnavailable
Language=English
(boot name, vendor id) is in use by some other boot block record.
.
Language=Russian
(boot name, vendor id) is in use by some other boot block record.
.

MessageId=2641
Severity=Success
Facility=System
SymbolicName=NERR_RplConfigNameUnavailable
Language=English
Configuration name is in use by some other configuration.
.
Language=Russian
Configuration name is in use by some other configuration.
.

MessageId=2660
Severity=Success
Facility=System
SymbolicName=NERR_DfsInternalCorruption
Language=English
The internal database maintained by the DFS service is corrupt
.
Language=Russian
The internal database maintained by the DFS service is corrupt
.

MessageId=2661
Severity=Success
Facility=System
SymbolicName=NERR_DfsVolumeDataCorrupt
Language=English
One of the records in the internal DFS database is corrupt
.
Language=Russian
One of the records in the internal DFS database is corrupt
.

MessageId=2662
Severity=Success
Facility=System
SymbolicName=NERR_DfsNoSuchVolume
Language=English
There is no DFS name whose entry path matches the input Entry Path
.
Language=Russian
There is no DFS name whose entry path matches the input Entry Path
.

MessageId=2663
Severity=Success
Facility=System
SymbolicName=NERR_DfsVolumeAlreadyExists
Language=English
A root or link with the given name already exists
.
Language=Russian
A root or link with the given name already exists
.

MessageId=2664
Severity=Success
Facility=System
SymbolicName=NERR_DfsAlreadyShared
Language=English
The server share specified is already shared in the DFS
.
Language=Russian
The server share specified is already shared in the DFS
.

MessageId=2665
Severity=Success
Facility=System
SymbolicName=NERR_DfsNoSuchShare
Language=English
The indicated server share does not support the indicated DFS namespace
.
Language=Russian
The indicated server share does not support the indicated DFS namespace
.

MessageId=2666
Severity=Success
Facility=System
SymbolicName=NERR_DfsNotALeafVolume
Language=English
The operation is not valid on this portion of the namespace
.
Language=Russian
The operation is not valid on this portion of the namespace
.

MessageId=2667
Severity=Success
Facility=System
SymbolicName=NERR_DfsLeafVolume
Language=English
The operation is not valid on this portion of the namespace
.
Language=Russian
The operation is not valid on this portion of the namespace
.

MessageId=2668
Severity=Success
Facility=System
SymbolicName=NERR_DfsVolumeHasMultipleServers
Language=English
The operation is ambiguous because the link has multiple servers
.
Language=Russian
The operation is ambiguous because the link has multiple servers
.

MessageId=2669
Severity=Success
Facility=System
SymbolicName=NERR_DfsCantCreateJunctionPoint
Language=English
Unable to create a link
.
Language=Russian
Unable to create a link
.

MessageId=2670
Severity=Success
Facility=System
SymbolicName=NERR_DfsServerNotDfsAware
Language=English
The server is not DFS Aware
.
Language=Russian
The server is not DFS Aware
.

MessageId=2671
Severity=Success
Facility=System
SymbolicName=NERR_DfsBadRenamePath
Language=English
The specified rename target path is invalid
.
Language=Russian
The specified rename target path is invalid
.

MessageId=2672
Severity=Success
Facility=System
SymbolicName=NERR_DfsVolumeIsOffline
Language=English
The specified DFS link is offline
.
Language=Russian
The specified DFS link is offline
.

MessageId=2673
Severity=Success
Facility=System
SymbolicName=NERR_DfsNoSuchServer
Language=English
The specified server is not a server for this link
.
Language=Russian
The specified server is not a server for this link
.

MessageId=2674
Severity=Success
Facility=System
SymbolicName=NERR_DfsCyclicalName
Language=English
A cycle in the DFS name was detected
.
Language=Russian
A cycle in the DFS name was detected
.

MessageId=2675
Severity=Success
Facility=System
SymbolicName=NERR_DfsNotSupportedInServerDfs
Language=English
The operation is not supported on a server-based DFS
.
Language=Russian
The operation is not supported on a server-based DFS
.

MessageId=2676
Severity=Success
Facility=System
SymbolicName=NERR_DfsDuplicateService
Language=English
This link is already supported by the specified server-share
.
Language=Russian
This link is already supported by the specified server-share
.

MessageId=2677
Severity=Success
Facility=System
SymbolicName=NERR_DfsCantRemoveLastServerShare
Language=English
Can't remove the last server-share supporting this root or link
.
Language=Russian
Can't remove the last server-share supporting this root or link
.

MessageId=2678
Severity=Success
Facility=System
SymbolicName=NERR_DfsVolumeIsInterDfs
Language=English
The operation is not supported for an Inter-DFS link
.
Language=Russian
The operation is not supported for an Inter-DFS link
.

MessageId=2679
Severity=Success
Facility=System
SymbolicName=NERR_DfsInconsistent
Language=English
The internal state of the DFS Service has become inconsistent
.
Language=Russian
The internal state of the DFS Service has become inconsistent
.

MessageId=2680
Severity=Success
Facility=System
SymbolicName=NERR_DfsServerUpgraded
Language=English
The DFS Service has been installed on the specified server
.
Language=Russian
The DFS Service has been installed on the specified server
.

MessageId=2681
Severity=Success
Facility=System
SymbolicName=NERR_DfsDataIsIdentical
Language=English
The DFS data being reconciled is identical
.
Language=Russian
The DFS data being reconciled is identical
.

MessageId=2682
Severity=Success
Facility=System
SymbolicName=NERR_DfsCantRemoveDfsRoot
Language=English
The DFS root cannot be deleted - Uninstall DFS if required
.
Language=Russian
The DFS root cannot be deleted - Uninstall DFS if required
.

MessageId=2683
Severity=Success
Facility=System
SymbolicName=NERR_DfsChildOrParentInDfs
Language=English
A child or parent directory of the share is already in a DFS
.
Language=Russian
A child or parent directory of the share is already in a DFS
.

MessageId=2690
Severity=Success
Facility=System
SymbolicName=NERR_DfsInternalError
Language=English
DFS internal error
.
Language=Russian
DFS internal error
.

MessageId=2691
Severity=Success
Facility=System
SymbolicName=NERR_SetupAlreadyJoined
Language=English
This machine is already joined to a domain.
.
Language=Russian
This machine is already joined to a domain.
.

MessageId=2692
Severity=Success
Facility=System
SymbolicName=NERR_SetupNotJoined
Language=English
This machine is not currently joined to a domain.
.
Language=Russian
This machine is not currently joined to a domain.
.

MessageId=2693
Severity=Success
Facility=System
SymbolicName=NERR_SetupDomainController
Language=English
This machine is a domain controller and cannot be unjoined from a domain.
.
Language=Russian
This machine is a domain controller and cannot be unjoined from a domain.
.

MessageId=2694
Severity=Success
Facility=System
SymbolicName=NERR_DefaultJoinRequired
Language=English
The destination domain controller does not support creating machine accounts in OUs.
.
Language=Russian
The destination domain controller does not support creating machine accounts in OUs.
.

MessageId=2695
Severity=Success
Facility=System
SymbolicName=NERR_InvalidWorkgroupName
Language=English
The specified workgroup name is invalid.
.
Language=Russian
The specified workgroup name is invalid.
.

MessageId=2696
Severity=Success
Facility=System
SymbolicName=NERR_NameUsesIncompatibleCodePage
Language=English
The specified computer name is incompatible with the default language used on the domain controller.
.
Language=Russian
The specified computer name is incompatible with the default language used on the domain controller.
.

MessageId=2697
Severity=Success
Facility=System
SymbolicName=NERR_ComputerAccountNotFound
Language=English
The specified computer account could not be found.
.
Language=Russian
The specified computer account could not be found.
.

MessageId=2698
Severity=Success
Facility=System
SymbolicName=NERR_PersonalSku
Language=English
This version of the operating system cannot be joined to a domain.
.
Language=Russian
This version of the operating system cannot be joined to a domain.
.

MessageId=2701
Severity=Success
Facility=System
SymbolicName=NERR_PasswordMustChange
Language=English
Password must change at next logon
.
Language=Russian
Password must change at next logon
.

MessageId=2702
Severity=Success
Facility=System
SymbolicName=NERR_AccountLockedOut
Language=English
Account is locked out
.
Language=Russian
Account is locked out
.

MessageId=2703
Severity=Success
Facility=System
SymbolicName=NERR_PasswordTooLong
Language=English
Password is too long
.
Language=Russian
Password is too long
.

MessageId=2704
Severity=Success
Facility=System
SymbolicName=NERR_PasswordNotComplexEnough
Language=English
Password doesn't meet the complexity policy
.
Language=Russian
Password doesn't meet the complexity policy
.

MessageId=2705
Severity=Success
Facility=System
SymbolicName=NERR_PasswordFilterError
Language=English
Password doesn't meet the requirements of the filter dll's
.
Language=Russian
Password doesn't meet the requirements of the filter dll's
.

MessageId=2999
Severity=Success
Facility=System
SymbolicName=MAX_NERR
Language=English
This is the last error in NERR range.
.
Language=Russian
This is the last error in NERR range.
.


;
; lmerrlog.h messages definitions (3100 - 3299)
;

MessageId=3100
Severity=Success
Facility=System
SymbolicName=NELOG_Internal_Error
Language=English
The operation failed because a network software error occurred.
.
Language=Russian
The operation failed because a network software error occurred.
.

MessageId=3101
Severity=Success
Facility=System
SymbolicName=NELOG_Resource_Shortage
Language=English
The system ran out of a resource controlled by the %1 option.
.
Language=Russian
The system ran out of a resource controlled by the %1 option.
.

MessageId=3102
Severity=Success
Facility=System
SymbolicName=NELOG_Unable_To_Lock_Segment
Language=English
The service failed to obtain a long-term lock on the\nsegment for network control blocks (NCBs). The error code is the data.
.
Language=Russian
The service failed to obtain a long-term lock on the\nsegment for network control blocks (NCBs). The error code is the data.
.

MessageId=3103
Severity=Success
Facility=System
SymbolicName=NELOG_Unable_To_Unlock_Segment
Language=English
The service failed to release the long-term lock on the\nsegment for network control blocks (NCBs). The error code is the data.
.
Language=Russian
The service failed to release the long-term lock on the\nsegment for network control blocks (NCBs). The error code is the data.
.

MessageId=3104
Severity=Success
Facility=System
SymbolicName=NELOG_Uninstall_Service
Language=English
There was an error stopping service %1.\nThe error code from NetServiceControl is the data.
.
Language=Russian
There was an error stopping service %1.\nThe error code from NetServiceControl is the data.
.

MessageId=3105
Severity=Success
Facility=System
SymbolicName=NELOG_Init_Exec_Fail
Language=English
Initialization failed because of a system execution failure on\npath %1. The system error code is the data.
.
Language=Russian
Initialization failed because of a system execution failure on\npath %1. The system error code is the data.
.

MessageId=3106
Severity=Success
Facility=System
SymbolicName=NELOG_Ncb_Error
Language=English
An unexpected network control block (NCB) was received. The NCB is the data.
.
Language=Russian
An unexpected network control block (NCB) was received. The NCB is the data.
.

MessageId=3107
Severity=Success
Facility=System
SymbolicName=NELOG_Net_Not_Started
Language=English
The network is not started.
.
Language=Russian
The network is not started.
.

MessageId=3108
Severity=Success
Facility=System
SymbolicName=NELOG_Ioctl_Error
Language=English
A DosDevIoctl or DosFsCtl to NETWKSTA.SYS failed.\nThe data shown is in this format:\nDWORD  approx CS:IP of call to ioctl or fsctl\nWORD   error code\nWORD   ioctl or fsctl number
.
Language=Russian
A DosDevIoctl or DosFsCtl to NETWKSTA.SYS failed.\nThe data shown is in this format:\nDWORD  approx CS:IP of call to ioctl or fsctl\nWORD   error code\nWORD   ioctl or fsctl number
.

MessageId=3109
Severity=Success
Facility=System
SymbolicName=NELOG_System_Semaphore
Language=English
Unable to create or open system semaphore %1.\nThe error code is the data.
.
Language=Russian
Unable to create or open system semaphore %1.\nThe error code is the data.
.

MessageId=3110
Severity=Success
Facility=System
SymbolicName=NELOG_Init_OpenCreate_Err
Language=English
Initialization failed because of an open/create error on the\nfile %1. The system error code is the data.
.
Language=Russian
Initialization failed because of an open/create error on the\nfile %1. The system error code is the data.
.

MessageId=3111
Severity=Success
Facility=System
SymbolicName=NELOG_NetBios
Language=English
An unexpected NetBIOS error occurred.\nThe error code is the data.
.
Language=Russian
An unexpected NetBIOS error occurred.\nThe error code is the data.
.

MessageId=3112
Severity=Success
Facility=System
SymbolicName=NELOG_SMB_Illegal
Language=English
An illegal server message block (SMB) was received.\nThe SMB is the data.
.
Language=Russian
An illegal server message block (SMB) was received.\nThe SMB is the data.
.

MessageId=3113
Severity=Success
Facility=System
SymbolicName=NELOG_Service_Fail
Language=English
Initialization failed because the requested service %1\ncould not be started.
.
Language=Russian
Initialization failed because the requested service %1\ncould not be started.
.

MessageId=3114
Severity=Success
Facility=System
SymbolicName=NELOG_Entries_Lost
Language=English
Some entries in the error log were lost because of a buffer\noverflow.
.
Language=Russian
Some entries in the error log were lost because of a buffer\noverflow.
.

MessageId=3120
Severity=Success
Facility=System
SymbolicName=NELOG_Init_Seg_Overflow
Language=English
Initialization parameters controlling resource usage other\nthan net buffers are sized so that too much memory is needed.
.
Language=Russian
Initialization parameters controlling resource usage other\nthan net buffers are sized so that too much memory is needed.
.

MessageId=3121
Severity=Success
Facility=System
SymbolicName=NELOG_Srv_No_Mem_Grow
Language=English
The server cannot increase the size of a memory segment.
.
Language=Russian
The server cannot increase the size of a memory segment.
.

MessageId=3122
Severity=Success
Facility=System
SymbolicName=NELOG_Access_File_Bad
Language=English
Initialization failed because account file %1 is either incorrect\nor not present.
.
Language=Russian
Initialization failed because account file %1 is either incorrect\nor not present.
.

MessageId=3123
Severity=Success
Facility=System
SymbolicName=NELOG_Srvnet_Not_Started
Language=English
Initialization failed because network %1 was not started.
.
Language=Russian
Initialization failed because network %1 was not started.
.

MessageId=3124
Severity=Success
Facility=System
SymbolicName=NELOG_Init_Chardev_Err
Language=English
The server failed to start. Either all three chdev\nparameters must be zero or all three must be nonzero.
.
Language=Russian
The server failed to start. Either all three chdev\nparameters must be zero or all three must be nonzero.
.

MessageId=3125
Severity=Success
Facility=System
SymbolicName=NELOG_Remote_API
Language=English
A remote API request was halted due to the following\ninvalid description string: %1.
.
Language=Russian
A remote API request was halted due to the following\ninvalid description string: %1.
.

MessageId=3126
Severity=Success
Facility=System
SymbolicName=NELOG_Ncb_TooManyErr
Language=English
The network %1 ran out of network control blocks (NCBs).  You may need to increase NCBs\nfor this network.  The following information includes the\nnumber of NCBs submitted by the server when this error occurred:
.
Language=Russian
The network %1 ran out of network control blocks (NCBs).  You may need to increase NCBs\nfor this network.  The following information includes the\nnumber of NCBs submitted by the server when this error occurred:
.

MessageId=3127
Severity=Success
Facility=System
SymbolicName=NELOG_Mailslot_err
Language=English
The server cannot create the %1 mailslot needed to send\nthe ReleaseMemory alert message.  The error received is:
.
Language=Russian
The server cannot create the %1 mailslot needed to send\nthe ReleaseMemory alert message.  The error received is:
.

MessageId=3128
Severity=Success
Facility=System
SymbolicName=NELOG_ReleaseMem_Alert
Language=English
The server failed to register for the ReleaseMemory alert,\nwith recipient %1. The error code from\nNetAlertStart is the data.
.
Language=Russian
The server failed to register for the ReleaseMemory alert,\nwith recipient %1. The error code from\nNetAlertStart is the data.
.

MessageId=3129
Severity=Success
Facility=System
SymbolicName=NELOG_AT_cannot_write
Language=English
The server cannot update the AT schedule file. The file\nis corrupted.
.
Language=Russian
The server cannot update the AT schedule file. The file\nis corrupted.
.

MessageId=3130
Severity=Success
Facility=System
SymbolicName=NELOG_Cant_Make_Msg_File
Language=English
The server encountered an error when calling\nNetIMakeLMFileName. The error code is the data.
.
Language=Russian
The server encountered an error when calling\nNetIMakeLMFileName. The error code is the data.
.

MessageId=3131
Severity=Success
Facility=System
SymbolicName=NELOG_Exec_Netservr_NoMem
Language=English
Initialization failed because of a system execution failure on\npath %1. There is not enough memory to start the process.\nThe system error code is the data.
.
Language=Russian
Initialization failed because of a system execution failure on\npath %1. There is not enough memory to start the process.\nThe system error code is the data.
.

MessageId=3132
Severity=Success
Facility=System
SymbolicName=NELOG_Server_Lock_Failure
Language=English
Longterm lock of the server buffers failed.\nCheck swap disk's free space and restart the system to start the server.
.
Language=Russian
Longterm lock of the server buffers failed.\nCheck swap disk's free space and restart the system to start the server.
.

MessageId=3140
Severity=Success
Facility=System
SymbolicName=NELOG_Msg_Shutdown
Language=English
The service has stopped due to repeated consecutive\noccurrences of a network control block (NCB) error.  The last bad NCB follows\nin raw data.
.
Language=Russian
The service has stopped due to repeated consecutive\noccurrences of a network control block (NCB) error.  The last bad NCB follows\nin raw data.
.

MessageId=3141
Severity=Success
Facility=System
SymbolicName=NELOG_Msg_Sem_Shutdown
Language=English
The Message server has stopped due to a lock on the\nMessage server shared data segment.
.
Language=Russian
The Message server has stopped due to a lock on the\nMessage server shared data segment.
.

MessageId=3150
Severity=Success
Facility=System
SymbolicName=NELOG_Msg_Log_Err
Language=English
A file system error occurred while opening or writing to the\nsystem message log file %1. Message logging has been\nswitched off due to the error. The error code is the data.
.
Language=Russian
A file system error occurred while opening or writing to the\nsystem message log file %1. Message logging has been\nswitched off due to the error. The error code is the data.
.

MessageId=3151
Severity=Success
Facility=System
SymbolicName=NELOG_VIO_POPUP_ERR
Language=English
Unable to display message POPUP due to system VIO call error.\nThe error code is the data.
.
Language=Russian
Unable to display message POPUP due to system VIO call error.\nThe error code is the data.
.

MessageId=3152
Severity=Success
Facility=System
SymbolicName=NELOG_Msg_Unexpected_SMB_Type
Language=English
An illegal server message block (SMB) was received.  The SMB is the data.
.
Language=Russian
An illegal server message block (SMB) was received.  The SMB is the data.
.

MessageId=3160
Severity=Success
Facility=System
SymbolicName=NELOG_Wksta_Infoseg
Language=English
The workstation information segment is bigger than 64K.\nThe size follows, in DWORD format:
.
Language=Russian
The workstation information segment is bigger than 64K.\nThe size follows, in DWORD format:
.

MessageId=3161
Severity=Success
Facility=System
SymbolicName=NELOG_Wksta_Compname
Language=English
The workstation was unable to get the name-number of the computer.
.
Language=Russian
The workstation was unable to get the name-number of the computer.
.

MessageId=3162
Severity=Success
Facility=System
SymbolicName=NELOG_Wksta_BiosThreadFailure
Language=English
The workstation could not initialize the Async NetBIOS Thread.\nThe error code is the data.
.
Language=Russian
The workstation could not initialize the Async NetBIOS Thread.\nThe error code is the data.
.

MessageId=3163
Severity=Success
Facility=System
SymbolicName=NELOG_Wksta_IniSeg
Language=English
The workstation could not open the initial shared segment.\nThe error code is the data.
.
Language=Russian
The workstation could not open the initial shared segment.\nThe error code is the data.
.

MessageId=3164
Severity=Success
Facility=System
SymbolicName=NELOG_Wksta_HostTab_Full
Language=English
The workstation host table is full.
.
Language=Russian
The workstation host table is full.
.

MessageId=3165
Severity=Success
Facility=System
SymbolicName=NELOG_Wksta_Bad_Mailslot_SMB
Language=English
A bad mailslot server message block (SMB) was received.  The SMB is the data.
.
Language=Russian
A bad mailslot server message block (SMB) was received.  The SMB is the data.
.

MessageId=3166
Severity=Success
Facility=System
SymbolicName=NELOG_Wksta_UASInit
Language=English
The workstation encountered an error while trying to start the user accounts database.\nThe error code is the data.
.
Language=Russian
The workstation encountered an error while trying to start the user accounts database.\nThe error code is the data.
.

MessageId=3167
Severity=Success
Facility=System
SymbolicName=NELOG_Wksta_SSIRelogon
Language=English
The workstation encountered an error while responding to an SSI revalidation request.\nThe function code and the error codes are the data.
.
Language=Russian
The workstation encountered an error while responding to an SSI revalidation request.\nThe function code and the error codes are the data.
.

MessageId=3170
Severity=Success
Facility=System
SymbolicName=NELOG_Build_Name
Language=English
The Alerter service had a problem creating the list of\nalert recipients.  The error code is %1.
.
Language=Russian
The Alerter service had a problem creating the list of\nalert recipients.  The error code is %1.
.

MessageId=3171
Severity=Success
Facility=System
SymbolicName=NELOG_Name_Expansion
Language=English
There was an error expanding %1 as a group name. Try\nsplitting the group into two or more smaller groups.
.
Language=Russian
There was an error expanding %1 as a group name. Try\nsplitting the group into two or more smaller groups.
.

MessageId=3172
Severity=Success
Facility=System
SymbolicName=NELOG_Message_Send
Language=English
There was an error sending %2 the alert message -\n(\n%3 )\nThe error code is %1.
.
Language=Russian
There was an error sending %2 the alert message -\n(\n%3 )\nThe error code is %1.
.

MessageId=3173
Severity=Success
Facility=System
SymbolicName=NELOG_Mail_Slt_Err
Language=English
There was an error in creating or reading the alerter mailslot.\nThe error code is %1.
.
Language=Russian
There was an error in creating or reading the alerter mailslot.\nThe error code is %1.
.

MessageId=3174
Severity=Success
Facility=System
SymbolicName=NELOG_AT_cannot_read
Language=English
The server could not read the AT schedule file.
.
Language=Russian
The server could not read the AT schedule file.
.

MessageId=3175
Severity=Success
Facility=System
SymbolicName=NELOG_AT_sched_err
Language=English
The server found an invalid AT schedule record.
.
Language=Russian
The server found an invalid AT schedule record.
.

MessageId=3176
Severity=Success
Facility=System
SymbolicName=NELOG_AT_schedule_file_created
Language=English
The server could not find an AT schedule file so it created one.
.
Language=Russian
The server could not find an AT schedule file so it created one.
.

MessageId=3177
Severity=Success
Facility=System
SymbolicName=NELOG_Srvnet_NB_Open
Language=English
The server could not access the %1 network with NetBiosOpen.
.
Language=Russian
The server could not access the %1 network with NetBiosOpen.
.

MessageId=3178
Severity=Success
Facility=System
SymbolicName=NELOG_AT_Exec_Err
Language=English
The AT command processor could not run %1.
.
Language=Russian
The AT command processor could not run %1.
.

MessageId=3180
Severity=Success
Facility=System
SymbolicName=NELOG_Lazy_Write_Err
Language=English
WARNING:  Because of a lazy-write error, drive %1 now\ncontains some corrupted data.  The cache is stopped.
.
Language=Russian
WARNING:  Because of a lazy-write error, drive %1 now\ncontains some corrupted data.  The cache is stopped.
.

MessageId=3181
Severity=Success
Facility=System
SymbolicName=NELOG_HotFix
Language=English
A defective sector on drive %1 has been replaced (hotfixed).\nNo data was lost.  You should run CHKDSK soon to restore full\nperformance and replenish the volume's spare sector pool.\n\nThe hotfix occurred while processing a remote request.
.
Language=Russian
A defective sector on drive %1 has been replaced (hotfixed).\nNo data was lost.  You should run CHKDSK soon to restore full\nperformance and replenish the volume's spare sector pool.\n\nThe hotfix occurred while processing a remote request.
.

MessageId=3182
Severity=Success
Facility=System
SymbolicName=NELOG_HardErr_From_Server
Language=English
A disk error occurred on the HPFS volume in drive %1.\nThe error occurred while processing a remote request.
.
Language=Russian
A disk error occurred on the HPFS volume in drive %1.\nThe error occurred while processing a remote request.
.

MessageId=3183
Severity=Success
Facility=System
SymbolicName=NELOG_LocalSecFail1
Language=English
The user accounts database (NET.ACC) is corrupted.  The local security\nsystem is replacing the corrupted NET.ACC with the backup\nmade at %1.\nAny updates made to the database after this time are lost.\n
.
Language=Russian
The user accounts database (NET.ACC) is corrupted.  The local security\nsystem is replacing the corrupted NET.ACC with the backup\nmade at %1.\nAny updates made to the database after this time are lost.\n
.

MessageId=3184
Severity=Success
Facility=System
SymbolicName=NELOG_LocalSecFail2
Language=English
The user accounts database (NET.ACC) is missing.  The local\nsecurity system is restoring the backup database\nmade at %1.\nAny updates made to the database made after this time are lost.\n
.
Language=Russian
The user accounts database (NET.ACC) is missing.  The local\nsecurity system is restoring the backup database\nmade at %1.\nAny updates made to the database made after this time are lost.\n
.

MessageId=3185
Severity=Success
Facility=System
SymbolicName=NELOG_LocalSecFail3
Language=English
Local security could not be started because the user accounts database\n(NET.ACC) was missing or corrupted, and no usable backup\ndatabase was present.\n\nTHE SYSTEM IS NOT SECURE.
.
Language=Russian
Local security could not be started because the user accounts database\n(NET.ACC) was missing or corrupted, and no usable backup\ndatabase was present.\n\nTHE SYSTEM IS NOT SECURE.
.

MessageId=3186
Severity=Success
Facility=System
SymbolicName=NELOG_LocalSecGeneralFail
Language=English
Local security could not be started because an error\noccurred during initialization. The error code returned is %1.\n\nTHE SYSTEM IS NOT SECURE.\n
.
Language=Russian
Local security could not be started because an error\noccurred during initialization. The error code returned is %1.\n\nTHE SYSTEM IS NOT SECURE.\n
.


;
; other message definitions
;

MessageId=3500
Severity=Success
Facility=System
SymbolicName=OTHER_3500
Language=English
The command completed successfully.
.
Language=Russian
The command completed successfully.
.

MessageId=3501
Severity=Success
Facility=System
SymbolicName=OTHER_3501
Language=English
You used an invalid option.
.
Language=Russian
You used an invalid option.
.

MessageId=3502
Severity=Success
Facility=System
SymbolicName=OTHER_3502
Language=English
System error %1 has occurred.
.
Language=Russian
System error %1 has occurred.
.

MessageId=3503
Severity=Success
Facility=System
SymbolicName=OTHER_3503
Language=English
The command contains an invalid number of arguments.
.
Language=Russian
The command contains an invalid number of arguments.
.

MessageId=3504
Severity=Success
Facility=System
SymbolicName=OTHER_3504
Language=English
The command completed with one or more errors.
.
Language=Russian
The command completed with one or more errors.
.

MessageId=3505
Severity=Success
Facility=System
SymbolicName=OTHER_3505
Language=English
You used an option with an invalid value.
.
Language=Russian
You used an option with an invalid value.
.

MessageId=3506
Severity=Success
Facility=System
SymbolicName=OTHER_3506
Language=English
The option %1 is unknown.
.
Language=Russian
The option %1 is unknown.
.

MessageId=3507
Severity=Success
Facility=System
SymbolicName=OTHER_3507
Language=English
Option %1 is ambiguous.
.
Language=Russian
Option %1 is ambiguous.
.

MessageId=3510
Severity=Success
Facility=System
SymbolicName=OTHER_3510
Language=English
A command was used with conflicting switches.
.
Language=Russian
A command was used with conflicting switches.
.

MessageId=3511
Severity=Success
Facility=System
SymbolicName=OTHER_3511
Language=English
Could not find subprogram %1.
.
Language=Russian
Could not find subprogram %1.
.

MessageId=3512
Severity=Success
Facility=System
SymbolicName=OTHER_3512
Language=English
The software requires a newer version of the operating\nsystem.
.
Language=Russian
The software requires a newer version of the operating\nsystem.
.

MessageId=3513
Severity=Success
Facility=System
SymbolicName=OTHER_3513
Language=English
More data is available than can be returned by the operating\nsystem.
.
Language=Russian
More data is available than can be returned by the operating\nsystem.
.

MessageId=3514
Severity=Success
Facility=System
SymbolicName=OTHER_3514
Language=English
More help is available by typing NET HELPMSG %1.
.
Language=Russian
More help is available by typing NET HELPMSG %1.
.

MessageId=3515
Severity=Success
Facility=System
SymbolicName=OTHER_3515
Language=English
The command can be used only on a Domain Controller.
.
Language=Russian
The command can be used only on a Domain Controller.
.

MessageId=3516
Severity=Success
Facility=System
SymbolicName=OTHER_3516
Language=English
This command cannot be used on a Domain Controller.
.
Language=Russian
This command cannot be used on a Domain Controller.
.

MessageId=3520
Severity=Success
Facility=System
SymbolicName=OTHER_3520
Language=English
These services are started:
.
Language=Russian
These services are started:
.

MessageId=3521
Severity=Success
Facility=System
SymbolicName=OTHER_3521
Language=English
The %1 service is not started.
.
Language=Russian
The %1 service is not started.
.

MessageId=3522
Severity=Success
Facility=System
SymbolicName=OTHER_3522
Language=English
The %1 service is starting%0
.
Language=Russian
The %1 service is starting%0
.

MessageId=3523
Severity=Success
Facility=System
SymbolicName=OTHER_3523
Language=English
The %1 service could not be started.
.
Language=Russian
The %1 service could not be started.
.

MessageId=3524
Severity=Success
Facility=System
SymbolicName=OTHER_3524
Language=English
The %1 service was started successfully.
.
Language=Russian
The %1 service was started successfully.
.

MessageId=3525
Severity=Success
Facility=System
SymbolicName=OTHER_3525
Language=English
Stopping the Workstation service also stops the Server service.
.
Language=Russian
Stopping the Workstation service also stops the Server service.
.

MessageId=3526
Severity=Success
Facility=System
SymbolicName=OTHER_3526
Language=English
The workstation has open files.
.
Language=Russian
The workstation has open files.
.

MessageId=3527
Severity=Success
Facility=System
SymbolicName=OTHER_3527
Language=English
The %1 service is stopping%0
.
Language=Russian
The %1 service is stopping%0
.

MessageId=3528
Severity=Success
Facility=System
SymbolicName=OTHER_3528
Language=English
The %1 service could not be stopped.
.
Language=Russian
The %1 service could not be stopped.
.

MessageId=3529
Severity=Success
Facility=System
SymbolicName=OTHER_3529
Language=English
The %1 service was stopped successfully.
.
Language=Russian
The %1 service was stopped successfully.
.

MessageId=3530
Severity=Success
Facility=System
SymbolicName=OTHER_3530
Language=English
The following services are dependent on the %1 service.\nStopping the %1 service will also stop these services.
.
Language=Russian
The following services are dependent on the %1 service.\nStopping the %1 service will also stop these services.
.

MessageId=3533
Severity=Success
Facility=System
SymbolicName=OTHER_3533
Language=English
The service is starting or stopping.  Please try again later.
.
Language=Russian
The service is starting or stopping.  Please try again later.
.

MessageId=3534
Severity=Success
Facility=System
SymbolicName=OTHER_3534
Language=English
The service did not report an error.
.
Language=Russian
The service did not report an error.
.

MessageId=3535
Severity=Success
Facility=System
SymbolicName=OTHER_3535
Language=English
An error occurred controlling the device.
.
Language=Russian
An error occurred controlling the device.
.

MessageId=3536
Severity=Success
Facility=System
SymbolicName=OTHER_3536
Language=English
The %1 service was continued successfully.
.
Language=Russian
The %1 service was continued successfully.
.

MessageId=3537
Severity=Success
Facility=System
SymbolicName=OTHER_3537
Language=English
The %1 service was paused successfully.
.
Language=Russian
The %1 service was paused successfully.
.

MessageId=3538
Severity=Success
Facility=System
SymbolicName=OTHER_3538
Language=English
The %1 service failed to resume.
.
Language=Russian
The %1 service failed to resume.
.

MessageId=3539
Severity=Success
Facility=System
SymbolicName=OTHER_3539
Language=English
The %1 service failed to pause.
.
Language=Russian
The %1 service failed to pause.
.

MessageId=3540
Severity=Success
Facility=System
SymbolicName=OTHER_3540
Language=English
The %1 service continue is pending%0
.
Language=Russian
The %1 service continue is pending%0
.

MessageId=3541
Severity=Success
Facility=System
SymbolicName=OTHER_3541
Language=English
The %1 service pause is pending%0
.
Language=Russian
The %1 service pause is pending%0
.

MessageId=3542
Severity=Success
Facility=System
SymbolicName=OTHER_3542
Language=English
%1 was continued successfully.
.
Language=Russian
%1 was continued successfully.
.

MessageId=3543
Severity=Success
Facility=System
SymbolicName=OTHER_3543
Language=English
%1 was paused successfully.
.
Language=Russian
%1 was paused successfully.
.

MessageId=3544
Severity=Success
Facility=System
SymbolicName=OTHER_3544
Language=English
The %1 service has been started by another process and is pending.%0
.
Language=Russian
The %1 service has been started by another process and is pending.%0
.

MessageId=3547
Severity=Success
Facility=System
SymbolicName=OTHER_3547
Language=English
A service specific error occurred: %1.
.
Language=Russian
A service specific error occurred: %1.
.
