IFNDEF  _WINERROR_
_WINERROR_      EQU     1

;* DOS and OS/2 Compatible Error Code definitions returned by the Win32 Base
;* API functions.

; ** Error codes

NO_ERROR                        EQU     0

ERROR_INVALID_FUNCTION          EQU     1
ERROR_FILE_NOT_FOUND            EQU     2
ERROR_PATH_NOT_FOUND            EQU     3
ERROR_TOO_MANY_OPEN_FILES       EQU     4
ERROR_ACCESS_DENIED             EQU     5
ERROR_INVALID_HANDLE            EQU     6
ERROR_ARENA_TRASHED             EQU     7
ERROR_NOT_ENOUGH_MEMORY         EQU     8
ERROR_INVALID_BLOCK             EQU     9
ERROR_BAD_ENVIRONMENT           EQU     10
ERROR_BAD_FORMAT                EQU     11
ERROR_INVALID_ACCESS            EQU     12
ERROR_INVALID_DATA              EQU     13

ERROR_INVALID_DRIVE             EQU     15
ERROR_CURRENT_DIRECTORY         EQU     16
ERROR_NOT_SAME_DEVICE           EQU     17
ERROR_NO_MORE_FILES             EQU     18

; These are the universal int 24 mappings for the old INT 24 set of errors
ERROR_WRITE_PROTECT             EQU     19
ERROR_BAD_UNIT                  EQU     20
ERROR_NOT_READY                 EQU     21
ERROR_BAD_COMMAND               EQU     22
ERROR_CRC                       EQU     23
ERROR_BAD_LENGTH                EQU     24
ERROR_SEEK                      EQU     25
ERROR_NOT_DOS_DISK              EQU     26
ERROR_SECTOR_NOT_FOUND          EQU     27
ERROR_OUT_OF_PAPER              EQU     28
ERROR_WRITE_FAULT               EQU     29
ERROR_READ_FAULT                EQU     30
ERROR_GEN_FAILURE               EQU     31

; These are the new 3.0 error codes reported through INT 24
ERROR_SHARING_VIOLATION         EQU     32
ERROR_LOCK_VIOLATION            EQU     33
ERROR_WRONG_DISK                EQU     34
; ERROR_FCB_UNAVAILABLE		 EQU	 35
; ERROR_SHARING_BUFFER_EXCEEDED	 EQU	 36
; End of INT 24 reportable errors

ERROR_HANDLE_EOF		EQU	38
ERROR_HANDLE_DISK_FULL		EQU	39


; Network related errors
ERROR_NOT_SUPPORTED		EQU	50 ; Network request not supported
ERROR_REM_NOT_LIST              EQU     51 ; Remote computer not listening
ERROR_DUP_NAME                  EQU     52 ; Duplicate name on network
ERROR_BAD_NETPATH               EQU     53 ; Network path not found
ERROR_NETWORK_BUSY              EQU     54 ; Network busy
ERROR_DEV_NOT_EXIST             EQU     55 ; Network device no longer exists
ERROR_TOO_MANY_CMDS             EQU     56 ; Net BIOS command limit exceeded
ERROR_ADAP_HDW_ERR              EQU     57 ; Network adapter hardware error
ERROR_BAD_NET_RESP              EQU     58 ; Incorrect response from network
ERROR_UNEXP_NET_ERR             EQU     59 ; Unexpected network error
ERROR_BAD_REM_ADAP              EQU     60 ; Incompatible remote adapter
ERROR_PRINTQ_FULL               EQU     61 ; Print queue full
ERROR_NO_SPOOL_SPACE            EQU     62 ; Not enough space for print file
ERROR_PRINT_CANCELLED           EQU     63 ; Print file was cancelled
ERROR_NETNAME_DELETED           EQU     64 ; Network name was deleted
ERROR_NETWORK_ACCESS_DENIED     EQU     65 ; Access denied
ERROR_BAD_DEV_TYPE              EQU     66 ; Network device type incorrect
ERROR_BAD_NET_NAME              EQU     67 ; Network name not found
ERROR_TOO_MANY_NAMES            EQU     68 ; Network name limit exceeded
ERROR_TOO_MANY_SESS             EQU     69 ; Net BIOS session limit exceeded
ERROR_SHARING_PAUSED            EQU     70 ; Sharing temporarily paused
ERROR_REQ_NOT_ACCEP             EQU     71 ; Network request not accepted
ERROR_REDIR_PAUSED              EQU     72 ; Print|disk redirection is paused
ERROR_FILE_EXISTS               EQU     80
; ERROR_DUP_FCB			 EQU	 81
ERROR_CANNOT_MAKE               EQU     82
ERROR_FAIL_I24                  EQU     83

; New 3.0 network related error codes
ERROR_OUT_OF_STRUCTURES         EQU     84
ERROR_ALREADY_ASSIGNED          EQU     85
ERROR_INVALID_PASSWORD          EQU     86
ERROR_INVALID_PARAMETER         EQU     87
ERROR_NET_WRITE_FAULT           EQU     88

; New error codes for 4.0
ERROR_NO_PROC_SLOTS		EQU	89 ; no process slots available

; ERROR_NOT_FROZEN		 EQU	 90
; ERR_TSTOVFL			  EQU	  91 ; timer service table overflow
; ERR_TSTDUP			  EQU	  92 ; timer service table duplicate
; ERROR_NO_ITEMS		  EQU	  93 ; no items to operate upon
; ERROR_INTERRUPT		  EQU	  95 ; interrupted system call
; ERROR_DEVICE_IN_USE		  EQU	  99 ; Device in use by another thread
ERROR_TOO_MANY_SEMAPHORES       EQU     100
ERROR_EXCL_SEM_ALREADY_OWNED    EQU     101
ERROR_SEM_IS_SET                EQU     102
ERROR_TOO_MANY_SEM_REQUESTS     EQU     103
ERROR_INVALID_AT_INTERRUPT_TIME EQU     104
ERROR_SEM_OWNER_DIED            EQU     105 ; waitsem found owner died
ERROR_SEM_USER_LIMIT            EQU     106 ; too many procs have this sem
ERROR_DISK_CHANGE		EQU	107 ; insert disk b into drive a
ERROR_DRIVE_LOCKED              EQU     108 ; drive locked by another process
ERROR_BROKEN_PIPE               EQU     109 ; write on pipe with no reader

; New error codes for OS/2 */
ERROR_OPEN_FAILED               EQU     110 ; open/created failed due to
                                            ; explicit fail command
ERROR_BUFFER_OVERFLOW           EQU     111 ; buffer passed to system call
                                            ;  is too small to hold return
                                            ;  data.
ERROR_DISK_FULL                 EQU     112 ; not enough space on the disk
                                            ;  (DOSNEWSIZE/w_NewSize)
ERROR_NO_MORE_SEARCH_HANDLES    EQU     113 ; can't allocate another search
                                            ;  structure and handle.
                                            ;  (DOSFINDFIRST/w_FindFirst)
ERROR_INVALID_TARGET_HANDLE     EQU     114 ; Target handle in DOSDUPHANDLE
                                            ;  is invalid
; ERROR_PROTECTION_VIOLATION	  EQU	  115 ; Bad user virtual address
; ERROR_VIOKBD_REQUEST		  EQU	  116
ERROR_INVALID_CATEGORY	  EQU	  117 ; Category for DEVIOCTL in not
					      ;  defined
ERROR_INVALID_VERIFY_SWITCH     EQU     118 ; invalid value passed for
                                            ;  verify flag
ERROR_BAD_DRIVER_LEVEL          EQU     119 ; DosDevIOCTL looks for a level
                                            ;  four driver. If the driver
                                            ;  is not level four we return
                                            ;  this code
ERROR_CALL_NOT_IMPLEMENTED      EQU     120 ; returned from stub api calls.
                                            ;  This call will disappear when
                                            ;  all the api's are implemented.
ERROR_SEM_TIMEOUT               EQU     121 ; Time out happened from the
                                            ;  semaphore api functions.
ERROR_INSUFFICIENT_BUFFER       EQU     122 ; Some calls require the
                                            ;  application to pass in a buffer
                                            ;  filled with data.  This error is
                                            ;  returned if the data buffer is too
                                            ;  small.  For example: DosSetFileInfo
                                            ;  requires 4 bytes of data.  If a
                                            ;  two byte buffer is passed in then
                                            ;  this error is returned.
                                            ;  error_buffer_overflow is used when
                                            ;  the output buffer in not big enough.
ERROR_INVALID_NAME              EQU     123 ; illegal character or malformed
                                            ; file system name
ERROR_INVALID_LEVEL             EQU     124 ; unimplemented level for info
                                            ; retrieval or setting
ERROR_NO_VOLUME_LABEL           EQU     125 ; no volume label found with
                                            ; DosQFSInfo command
ERROR_MOD_NOT_FOUND             EQU     126 ; w_getprocaddr,w_getmodhandle
ERROR_PROC_NOT_FOUND            EQU     127 ; w_getprocaddr
; ERROR_WAIT_NO_CHILDREN	 EQU	 128 ; CWait finds to children
ERROR_CHILD_NOT_COMPLETE        EQU     129 ; CWait children not dead yet
ERROR_DIRECT_ACCESS_HANDLE      EQU     130 ; handle operation is invalid
                                            ; for direct disk access
                                            ; handles
ERROR_NEGATIVE_SEEK             EQU     131 ; application tried to seek
                                            ; with negitive offset
ERROR_SEEK_ON_DEVICE            EQU     132 ; application tried to seek
                                            ; on device or pipe

; The following are errors generated by the join and subst workers
ERROR_IS_JOIN_TARGET            EQU     133
ERROR_IS_JOINED                 EQU     134
ERROR_IS_SUBSTED                EQU     135
ERROR_NOT_JOINED                EQU     136
ERROR_NOT_SUBSTED               EQU     137
ERROR_JOIN_TO_JOIN              EQU     138
ERROR_SUBST_TO_SUBST            EQU     139
ERROR_JOIN_TO_SUBST             EQU     140
ERROR_SUBST_TO_JOIN             EQU     141
ERROR_BUSY_DRIVE                EQU     142
ERROR_SAME_DRIVE                EQU     143
ERROR_DIR_NOT_ROOT              EQU     144
ERROR_DIR_NOT_EMPTY             EQU     145
ERROR_IS_SUBST_PATH             EQU     146
ERROR_IS_JOIN_PATH              EQU     147
ERROR_PATH_BUSY                 EQU     148
ERROR_IS_SUBST_TARGET           EQU     149
ERROR_SYSTEM_TRACE              EQU     150 ; system trace error
ERROR_INVALID_EVENT_COUNT       EQU     151 ; DosMuxSemWait errors
ERROR_TOO_MANY_MUXWAITERS       EQU     152
ERROR_INVALID_LIST_FORMAT       EQU     153
ERROR_LABEL_TOO_LONG            EQU     154
ERROR_TOO_MANY_TCBS             EQU     155
ERROR_SIGNAL_REFUSED            EQU     156
ERROR_DISCARDED                 EQU     157
ERROR_NOT_LOCKED                EQU     158
ERROR_BAD_THREADID_ADDR         EQU     159
ERROR_BAD_ARGUMENTS             EQU     160
ERROR_BAD_PATHNAME              EQU     161
ERROR_SIGNAL_PENDING            EQU     162
; ERROR_UNCERTAIN_MEDIA		 EQU	 163
ERROR_MAX_THRDS_REACHED         EQU     164
; ERROR_MONITORS_NOT_SUPPORTED	 EQU	 165

ERROR_LOCK_FAILED		EQU	167
ERROR_BUSY			EQU	170

ERROR_INVALID_SEGMENT_NUMBER	EQU	180
; ERROR_INVALID_CALLGATE		EQU	181
ERROR_INVALID_ORDINAL		EQU	182
ERROR_ALREADY_EXISTS		EQU	183
; ERROR_NO_CHILD_PROCESS		EQU	184
; ERROR_CHILD_ALIVE_NOWAIT	EQU	185
ERROR_INVALID_FLAG_NUMBER	EQU	186
ERROR_SEM_NOT_FOUND		EQU	187

; following error codes have added to make the loader error
; messages distinct

ERROR_INVALID_STARTING_CODESEG	EQU	188
ERROR_INVALID_STACKSEG		EQU	189
ERROR_INVALID_MODULETYPE	EQU	190
ERROR_INVALID_EXE_SIGNATURE	EQU	191
ERROR_EXE_MARKED_INVALID	EQU	192
ERROR_BAD_EXE_FORMAT		EQU	193
ERROR_ITERATED_DATA_EXCEEDS_64k EQU	194
ERROR_INVALID_MINALLOCSIZE	EQU	195
ERROR_DYNLINK_FROM_INVALID_RING EQU	196
ERROR_IOPL_NOT_ENABLED		EQU	197
ERROR_INVALID_SEGDPL		EQU	198
ERROR_AUTODATASEG_EXCEEDS_64k	EQU	199
ERROR_RING2SEG_MUST_BE_MOVABLE	EQU	200
ERROR_RELOC_CHAIN_XEEDS_SEGLIM	EQU	201
ERROR_INFLOOP_IN_RELOC_CHAIN	EQU	202

ERROR_ENVVAR_NOT_FOUND		EQU	203
ERROR_NOT_CURRENT_CTRY		EQU	204
ERROR_NO_SIGNAL_SENT		EQU	205
ERROR_FILENAME_EXCED_RANGE	EQU	206 ; if filename > 8.3
ERROR_RING2_STACK_IN_USE	EQU	207 ; for FAPI
ERROR_META_EXPANSION_TOO_LONG	EQU	208 ; if "*a" > 8.3

ERROR_INVALID_SIGNAL_NUMBER	EQU	209
ERROR_THREAD_1_INACTIVE 	EQU	210
; ERROR_INFO_NOT_AVAIL		EQU	211 ; PTM 5550
ERROR_LOCKED			EQU	212
; ERROR_BAD_DYNALINK		EQU	213 ; PTM 5760
ERROR_TOO_MANY_MODULES		EQU	214
; ERROR_NESTING_NOT_ALLOWED	EQU	215

; Error codes 230 - 249 are reserved for MS Networks
ERROR_BAD_PIPE			EQU	230
ERROR_PIPE_BUSY 		EQU	231
ERROR_NO_DATA			EQU	232
ERROR_PIPE_NOT_CONNECTED	EQU	233
ERROR_MORE_DATA 		EQU	234
ERROR_VC_DISCONNECTED		EQU	240
ERROR_INVALID_EA_NAME		EQU	254
ERROR_NO_MORE_ITEMS		EQU	259
ERROR_DIRECTORY 		EQU	267
ERROR_EAS_DIDNT_FIT		EQU	275
ERROR_EAS_NOT_SUPPORTED 	EQU	282
ERROR_NOT_OWNER 		EQU	288
ERROR_TOO_MANY_POSTS		EQU	298
ERROR_MR_MID_NOT_FOUND		EQU	317
ERROR_INVALID_ADDRESS		EQU	487
ERROR_ARITHMETIC_OVERFLOW	EQU	534
ERROR_IO_INCOMPLETE		EQU	996
ERROR_IO_PENDING		EQU	997
ERROR_NOACCESS			EQU	998
ERROR_SWAPERROR 		EQU	999

; Errors added for DOS Extender

ERROR_SYS_INTERNAL		EQU	328

ENDIF ; _WINERROR_
