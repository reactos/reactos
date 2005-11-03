/* os2emx.h (emx+gcc) */

#ifndef _OS2EMX_H
#define _OS2EMX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#pragma pack(1)

/* ------------------------ INCL_ SYMBOLS --------------------------------- */

#if defined (INCL_BASE)
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_SUB
#endif

#if defined (INCL_DOS)
#define INCL_DOSDATETIME
#define INCL_DOSDEVICES
#define INCL_DOSEXCEPTIONS
#define INCL_DOSFILEMGR
#define INCL_DOSMEMMGR
#define INCL_DOSMISC
#define INCL_DOSMVDM
#define INCL_DOSMODULEMGR
#define INCL_DOSNLS
#define INCL_DOSPROCESS
#define INCL_DOSPROFILE
#define INCL_DOSRAS
#define INCL_DOSQUEUES
#define INCL_DOSRESOURCES
#define INCL_DOSSEMAPHORES
#define INCL_DOSSESMGR
#endif

#if defined (INCL_REXXSAA)
#define INCL_RXSUBCOM
#define INCL_RXSHV
#define INCL_RXFUNC
#define INCL_RXSYSEXIT
#define INCL_RXMACRO
#define INCL_RXARI
#endif

#if defined (INCL_SUB)
#define INCL_KBD
#define INCL_VIO
#define INCL_MOU
#endif

#if defined (INCL_PM)
#define INCL_AVIO
#define INCL_FONTFILEFORMAT
#define INCL_GPI
#define INCL_SPL
#define INCL_WIN
#define INCL_WINWORKPLACE
#endif

#if defined (INCL_WIN) || defined (RC_INVOKED)
#define INCL_WINACCELERATORS
#define INCL_WINBUTTONS
#define INCL_WINDIALOGS
#define INCL_WINENTRYFIELDS
#define INCL_WINFRAMECTLS
#define INCL_WINFRAMEMGR
#define INCL_WINHELP
#define INCL_WININPUT
#define INCL_WINLISTBOXES
#define INCL_WINMENUS
#define INCL_WINMESSAGEMGR
#define INCL_WINMLE
#define INCL_WINPOINTERS
#define INCL_WINSCROLLBARS
#define INCL_WINSTATICS
#define INCL_WINSTDDLGS
#define INCL_WINSYS
#endif /* INCL_WIN || RC_INVOKED */

#if defined (INCL_WIN)
#define INCL_WINATOM
#define INCL_WINCLIPBOARD
#define INCL_WINCOUNTRY
#define INCL_WINCURSORS
#define INCL_WINDDE
#define INCL_WINDESKTOP
#define INCL_WINERRORS
#define INCL_WINHOOKS
#define INCL_WINLOAD
#define INCL_WINPALETTE
#define INCL_WINPROGRAMLIST
#define INCL_WINRECTANGLES
#define INCL_WINSHELLDATA
#define INCL_WINSWITCHLIST
#define INCL_WINTHUNKAPI
#define INCL_WINTIMER
#define INCL_WINTRACKRECT
#define INCL_WINTYPES
#define INCL_WINWINDOWMGR
#endif /* INCL_WIN */

#if defined (INCL_WINCOMMON)
#define INCL_WINWINDOWMGR
#endif

#if defined (INCL_WINSTDDLGS)
#define INCL_WINCIRCULARSLIDER
#define INCL_WINSTDBOOK
#define INCL_WINSTDCNR
#define INCL_WINSTDDRAG
#define INCL_WINSTDFILE
#define INCL_WINSTDFONT
#define INCL_WINSTDSLIDER
#define INCL_WINSTDSPIN
#define INCL_WINSTDVALSET
#endif

#if defined (INCL_WINSTDCNR) || defined (INCL_WINSTDVALSET)
#define INCL_WINSTDDRAG
#endif

#if defined (INCL_WINMLE) && !defined (INCL_WINENTRYFIELDS)
#define INCL_WINENTRYFIELDS
#endif

#if defined (INCL_WINWORKPLACE)
#define INCL_WPCLASS
#endif

#if defined (INCL_GPI)
#define INCL_GPIBITMAPS
#define INCL_GPICONTROL
#define INCL_GPICORRELATION
#define INCL_GPIINK
#define INCL_GPISEGMENTS
#endif

#if defined (INCL_ERRORS)
#define INCL_DOSERRORS
#define INCL_GPIERRORS
#define INCL_SHLERRORS
#define INCL_WINERRORS
#endif

#if defined (INCL_DDIDEFS)
#define INCL_GPIBITMAPS
#define INCL_GPIERRORS
#endif

#if defined (INCL_CIRCULARSLIDER) && !defined (INCL_WINCIRCULARSLIDER)
#define INCL_WINCIRCULARSLIDER
#endif

/* ------------------------ DISABLE KEYWORDS ------------------------------ */

#define APIENTRY
#define EXPENTRY
#define FAR

/* ------------------------ CONSTANTS AND TYPES --------------------------- */

#if !defined (FALSE)
#define FALSE 0
#endif

#if !defined (TRUE)
#define TRUE 1
#endif

#define VOID				void

#define NULLHANDLE			((LHANDLE)0)
#define NULLSHANDLE			((SHANDLE)0)

#if !defined (NULL)
#if defined (__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

typedef int INT;		/* Required for Toolkit sample programs */
typedef unsigned UINT;
typedef unsigned long APIRET;

typedef unsigned long BOOL;
typedef BOOL *PBOOL;

typedef unsigned long BOOL32;
typedef BOOL *PBOOL32;

typedef char CHAR;
typedef CHAR *PCHAR;

#if !defined (OS2EMX_PLAIN_CHAR)

typedef unsigned char BYTE;
typedef unsigned char *PCH;
typedef unsigned char *PSZ;
typedef __const__ unsigned char *PCCH;
typedef __const__ unsigned char *PCSZ;

#else

typedef char BYTE;
typedef char *PCH;
typedef char *PSZ;
typedef __const__ char *PCCH;
typedef __const__ char *PCSZ;

#endif

typedef BYTE *PBYTE;

typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;

typedef short SHORT;
typedef SHORT *PSHORT;

typedef unsigned short USHORT;
typedef USHORT *PUSHORT;

typedef long LONG;
typedef LONG *PLONG;

typedef unsigned long ULONG;
typedef ULONG *PULONG;

typedef VOID *PVOID;
typedef PVOID *PPVOID;

typedef __const__ VOID *CPVOID;

typedef CHAR STR8[8];
typedef STR8 *PSTR8;

typedef CHAR STR16[16];
typedef STR16 *PSTR16;
typedef CHAR STR32[32];
typedef STR32 *PSTR32;
typedef CHAR STR64[64];
typedef STR64 *PSTR64;

typedef unsigned short SHANDLE;
typedef unsigned long LHANDLE;

typedef LHANDLE HPIPE;
typedef HPIPE *PHPIPE;

typedef LHANDLE HQUEUE;
typedef HQUEUE *PHQUEUE;

typedef LHANDLE HMODULE;
typedef HMODULE *PHMODULE;

typedef VOID *HSEM;
typedef HSEM *PHSEM;

typedef LHANDLE HOBJECT;

typedef ULONG PID;
typedef PID *PPID;

typedef ULONG TID;
typedef TID *PTID;

typedef int (*PFN)();
typedef PFN *PPFN;

typedef USHORT SEL;
typedef SEL *PSEL;

typedef ULONG HMTX;
typedef HMTX *PHMTX;

typedef ULONG HMUX;
typedef HMUX *PHMUX;


#define FIELDOFFSET(t,f)  ((SHORT)&(((t *)0)->f))

#define MAKETYPE(v,t)	  (*((t *)&v))

#define MAKEUSHORT(l,h)	  (((USHORT)(l)) | ((USHORT)(h))<<8)
#define MAKESHORT(l,h)	  ((SHORT)MAKEUSHORT(l, h))

#define MAKEULONG(l,h)	  ((ULONG)(((USHORT)(l)) | ((ULONG)((USHORT)(h)))<<16))
#define MAKELONG(l,h)	  ((LONG)MAKEULONG(l, h))

#define LOUCHAR(w)	  ((UCHAR)(w))
#define HIUCHAR(w)	  ((UCHAR)((USHORT)(w)>>8))

#define LOBYTE(x)	  LOUCHAR(x)
#define HIBYTE(x)	  HIUCHAR(x)

#define LOUSHORT(x)	  ((USHORT)((ULONG)(x)))
#define HIUSHORT(x)	  ((USHORT)((ULONG)(x)>>16))

/* ---------------------------- ERROR CODES ------------------------------- */

#define WINERR_BASE			0x1000
#define GPIERR_BASE			0x2000
#define DEVERR_BASE			0x3000
#define SPLERR_BASE			0x4000

#define SEVERITY_NOERROR		0x0000
#define SEVERITY_WARNING		0x0004
#define SEVERITY_ERROR			0x0008
#define SEVERITY_SEVERE			0x000c
#define SEVERITY_UNRECOVERABLE		0x0010

#if defined (INCL_DOSERRORS)

#define NO_ERROR				0
#define ERROR_INVALID_FUNCTION			1
#define ERROR_FILE_NOT_FOUND			2
#define ERROR_PATH_NOT_FOUND			3
#define ERROR_TOO_MANY_OPEN_FILES		4
#define ERROR_ACCESS_DENIED			5
#define ERROR_INVALID_HANDLE			6
#define ERROR_ARENA_TRASHED			7
#define ERROR_NOT_ENOUGH_MEMORY			8
#define ERROR_INVALID_BLOCK			9
#define ERROR_BAD_ENVIRONMENT		       10
#define ERROR_BAD_FORMAT		       11
#define ERROR_INVALID_ACCESS		       12
#define ERROR_INVALID_DATA		       13
#define ERROR_INVALID_DRIVE		       15
#define ERROR_CURRENT_DIRECTORY		       16
#define ERROR_NOT_SAME_DEVICE		       17
#define ERROR_NO_MORE_FILES		       18
#define ERROR_WRITE_PROTECT		       19
#define ERROR_BAD_UNIT			       20
#define ERROR_NOT_READY			       21
#define ERROR_BAD_COMMAND		       22
#define ERROR_CRC			       23
#define ERROR_BAD_LENGTH		       24
#define ERROR_SEEK			       25
#define ERROR_NOT_DOS_DISK		       26
#define ERROR_SECTOR_NOT_FOUND		       27
#define ERROR_OUT_OF_PAPER		       28
#define ERROR_WRITE_FAULT		       29
#define ERROR_READ_FAULT		       30
#define ERROR_GEN_FAILURE		       31
#define ERROR_SHARING_VIOLATION		       32
#define ERROR_LOCK_VIOLATION		       33
#define ERROR_WRONG_DISK		       34
#define ERROR_FCB_UNAVAILABLE		       35
#define ERROR_SHARING_BUFFER_EXCEEDED	       36
#define ERROR_CODE_PAGE_MISMATCHED	       37
#define ERROR_HANDLE_EOF		       38
#define ERROR_HANDLE_DISK_FULL		       39
#define ERROR_NOT_SUPPORTED		       50
#define ERROR_REM_NOT_LIST		       51
#define ERROR_DUP_NAME			       52
#define ERROR_BAD_NETPATH		       53
#define ERROR_NETWORK_BUSY		       54
#define ERROR_DEV_NOT_EXIST		       55
#define ERROR_TOO_MANY_CMDS		       56
#define ERROR_ADAP_HDW_ERR		       57
#define ERROR_BAD_NET_RESP		       58
#define ERROR_UNEXP_NET_ERR		       59
#define ERROR_BAD_REM_ADAP		       60
#define ERROR_PRINTQ_FULL		       61
#define ERROR_NO_SPOOL_SPACE		       62
#define ERROR_PRINT_CANCELLED		       63
#define ERROR_NETNAME_DELETED		       64
#define ERROR_NETWORK_ACCESS_DENIED	       65
#define ERROR_BAD_DEV_TYPE		       66
#define ERROR_BAD_NET_NAME		       67
#define ERROR_TOO_MANY_NAMES		       68
#define ERROR_TOO_MANY_SESS		       69
#define ERROR_SHARING_PAUSED		       70
#define ERROR_REQ_NOT_ACCEP		       71
#define ERROR_REDIR_PAUSED		       72
#define ERROR_SBCS_ATT_WRITE_PROT	       73
#define ERROR_SBCS_GENERAL_FAILURE	       74
#define ERROR_XGA_OUT_MEMORY		       75
#define ERROR_FILE_EXISTS		       80
#define ERROR_DUP_FCB			       81
#define ERROR_CANNOT_MAKE		       82
#define ERROR_FAIL_I24			       83
#define ERROR_OUT_OF_STRUCTURES		       84
#define ERROR_ALREADY_ASSIGNED		       85
#define ERROR_INVALID_PASSWORD		       86
#define ERROR_INVALID_PARAMETER		       87
#define ERROR_NET_WRITE_FAULT		       88
#define ERROR_NO_PROC_SLOTS		       89
#define ERROR_NOT_FROZEN		       90
#define ERROR_SYS_COMP_NOT_LOADED	       90 /*!*/
#define ERR_TSTOVFL			       91
#define ERR_TSTDUP			       92
#define ERROR_NO_ITEMS			       93
#define ERROR_INTERRUPT			       95
#define ERROR_DEVICE_IN_USE		       99
#define ERROR_TOO_MANY_SEMAPHORES	      100
#define ERROR_EXCL_SEM_ALREADY_OWNED	      101
#define ERROR_SEM_IS_SET		      102
#define ERROR_TOO_MANY_SEM_REQUESTS	      103
#define ERROR_INVALID_AT_INTERRUPT_TIME	      104
#define ERROR_SEM_OWNER_DIED		      105
#define ERROR_SEM_USER_LIMIT		      106
#define ERROR_DISK_CHANGE		      107
#define ERROR_DRIVE_LOCKED		      108
#define ERROR_BROKEN_PIPE		      109
#define ERROR_OPEN_FAILED		      110
#define ERROR_BUFFER_OVERFLOW		      111
#define ERROR_DISK_FULL			      112
#define ERROR_NO_MORE_SEARCH_HANDLES	      113
#define ERROR_INVALID_TARGET_HANDLE	      114
#define ERROR_PROTECTION_VIOLATION	      115
#define ERROR_VIOKBD_REQUEST		      116
#define ERROR_INVALID_CATEGORY		      117
#define ERROR_INVALID_VERIFY_SWITCH	      118
#define ERROR_BAD_DRIVER_LEVEL		      119
#define ERROR_CALL_NOT_IMPLEMENTED	      120
#define ERROR_SEM_TIMEOUT		      121
#define ERROR_INSUFFICIENT_BUFFER	      122
#define ERROR_INVALID_NAME		      123
#define ERROR_INVALID_LEVEL		      124
#define ERROR_NO_VOLUME_LABEL		      125
#define ERROR_MOD_NOT_FOUND		      126
#define ERROR_PROC_NOT_FOUND		      127
#define ERROR_WAIT_NO_CHILDREN		      128
#define ERROR_CHILD_NOT_COMPLETE	      129
#define ERROR_DIRECT_ACCESS_HANDLE	      130
#define ERROR_NEGATIVE_SEEK		      131
#define ERROR_SEEK_ON_DEVICE		      132
#define ERROR_IS_JOIN_TARGET		      133
#define ERROR_IS_JOINED			      134
#define ERROR_IS_SUBSTED		      135
#define ERROR_NOT_JOINED		      136
#define ERROR_NOT_SUBSTED		      137
#define ERROR_JOIN_TO_JOIN		      138
#define ERROR_SUBST_TO_SUBST		      139
#define ERROR_JOIN_TO_SUBST		      140
#define ERROR_SUBST_TO_JOIN		      141
#define ERROR_BUSY_DRIVE		      142
#define ERROR_SAME_DRIVE		      143
#define ERROR_DIR_NOT_ROOT		      144
#define ERROR_DIR_NOT_EMPTY		      145
#define ERROR_IS_SUBST_PATH		      146
#define ERROR_IS_JOIN_PATH		      147
#define ERROR_PATH_BUSY			      148
#define ERROR_IS_SUBST_TARGET		      149
#define ERROR_SYSTEM_TRACE		      150
#define ERROR_INVALID_EVENT_COUNT	      151
#define ERROR_TOO_MANY_MUXWAITERS	      152
#define ERROR_INVALID_LIST_FORMAT	      153
#define ERROR_LABEL_TOO_LONG		      154
#define ERROR_TOO_MANY_TCBS		      155
#define ERROR_SIGNAL_REFUSED		      156
#define ERROR_DISCARDED			      157
#define ERROR_NOT_LOCKED		      158
#define ERROR_BAD_THREADID_ADDR		      159
#define ERROR_BAD_ARGUMENTS		      160
#define ERROR_BAD_PATHNAME		      161
#define ERROR_SIGNAL_PENDING		      162
#define ERROR_UNCERTAIN_MEDIA		      163
#define ERROR_MAX_THRDS_REACHED		      164
#define ERROR_MONITORS_NOT_SUPPORTED	      165
#define ERROR_UNC_DRIVER_NOT_INSTALLED	      166
#define ERROR_LOCK_FAILED		      167
#define ERROR_SWAPIO_FAILED		      168
#define ERROR_SWAPIN_FAILED		      169
#define ERROR_BUSY			      170
#define ERROR_CANCEL_VIOLATION		      173
#define ERROR_ATOMIC_LOCK_NOT_SUPPORTED	      174
#define ERROR_READ_LOCKS_NOT_SUPPORTED	      175
#define ERROR_INVALID_SEGMENT_NUMBER	      180
#define ERROR_INVALID_CALLGATE		      181
#define ERROR_INVALID_ORDINAL		      182
#define ERROR_ALREADY_EXISTS		      183
#define ERROR_NO_CHILD_PROCESS		      184
#define ERROR_CHILD_ALIVE_NOWAIT	      185
#define ERROR_INVALID_FLAG_NUMBER	      186
#define ERROR_SEM_NOT_FOUND		      187
#define ERROR_INVALID_STARTING_CODESEG	      188
#define ERROR_INVALID_STACKSEG		      189
#define ERROR_INVALID_MODULETYPE	      190
#define ERROR_INVALID_EXE_SIGNATURE	      191
#define ERROR_EXE_MARKED_INVALID	      192
#define ERROR_BAD_EXE_FORMAT		      193
#define ERROR_ITERATED_DATA_EXCEEDS_64K	      194
#define ERROR_INVALID_MINALLOCSIZE	      195
#define ERROR_DYNLINK_FROM_INVALID_RING	      196
#define ERROR_IOPL_NOT_ENABLED		      197
#define ERROR_INVALID_SEGDPL		      198
#define ERROR_AUTODATASEG_EXCEEDS_64K	      199
#define ERROR_RING2SEG_MUST_BE_MOVABLE	      200
#define ERROR_RELOCSRC_CHAIN_EXCEEDS_SEGLIMIT 201
#define ERROR_INFLOOP_IN_RELOC_CHAIN	      202
#define ERROR_ENVVAR_NOT_FOUND		      203
#define ERROR_NOT_CURRENT_CTRY		      204
#define ERROR_NO_SIGNAL_SENT		      205
#define ERROR_FILENAME_EXCED_RANGE	      206
#define ERROR_RING2_STACK_IN_USE	      207
#define ERROR_META_EXPANSION_TOO_LONG	      208
#define ERROR_INVALID_SIGNAL_NUMBER	      209
#define ERROR_THREAD_1_INACTIVE		      210
#define ERROR_INFO_NOT_AVAIL		      211
#define ERROR_LOCKED			      212
#define ERROR_BAD_DYNALINK		      213
#define ERROR_TOO_MANY_MODULES		      214
#define ERROR_NESTING_NOT_ALLOWED	      215
#define ERROR_CANNOT_SHRINK		      216
#define ERROR_ZOMBIE_PROCESS		      217
#define ERROR_STACK_IN_HIGH_MEMORY	      218
#define ERROR_INVALID_EXITROUTINE_RING	      219
#define ERROR_GETBUF_FAILED		      220
#define ERROR_FLUSHBUF_FAILED		      221
#define ERROR_TRANSFER_TOO_LONG		      222
#define ERROR_FORCENOSWAP_FAILED	      223
#define ERROR_SMG_NO_TARGET_WINDOW	      224
#define ERROR_NO_CHILDREN		      228
#define ERROR_INVALID_SCREEN_GROUP	      229
#define ERROR_BAD_PIPE			      230
#define ERROR_PIPE_BUSY			      231
#define ERROR_NO_DATA			      232
#define ERROR_PIPE_NOT_CONNECTED	      233
#define ERROR_MORE_DATA			      234
#define ERROR_VC_DISCONNECTED		      240
#define ERROR_CIRCULARITY_REQUESTED	      250
#define ERROR_DIRECTORY_IN_CDS		      251
#define ERROR_INVALID_FSD_NAME		      252
#define ERROR_INVALID_PATH		      253
#define ERROR_INVALID_EA_NAME		      254
#define ERROR_EA_LIST_INCONSISTENT	      255
#define ERROR_EA_LIST_TOO_LONG		      256
#define ERROR_NO_META_MATCH		      257
#define ERROR_FINDNOTIFY_TIMEOUT	      258
#define ERROR_NO_MORE_ITEMS		      259
#define ERROR_SEARCH_STRUC_REUSED	      260
#define ERROR_CHAR_NOT_FOUND		      261
#define ERROR_TOO_MUCH_STACK		      262
#define ERROR_INVALID_ATTR		      263
#define ERROR_INVALID_STARTING_RING	      264
#define ERROR_INVALID_DLL_INIT_RING	      265
#define ERROR_CANNOT_COPY		      266
#define ERROR_DIRECTORY			      267
#define ERROR_OPLOCKED_FILE		      268
#define ERROR_OPLOCK_THREAD_EXISTS	      269
#define ERROR_VOLUME_CHANGED		      270
#define ERROR_FINDNOTIFY_HANDLE_IN_USE	      271
#define ERROR_FINDNOTIFY_HANDLE_CLOSED	      272
#define ERROR_NOTIFY_OBJECT_REMOVED	      273
#define ERROR_ALREADY_SHUTDOWN		      274
#define ERROR_EAS_DIDNT_FIT		      275
#define ERROR_EA_FILE_CORRUPT		      276
#define ERROR_EA_TABLE_FULL		      277
#define ERROR_INVALID_EA_HANDLE		      278
#define ERROR_NO_CLUSTER		      279
#define ERROR_CREATE_EA_FILE		      280
#define ERROR_CANNOT_OPEN_EA_FILE	      281
#define ERROR_EAS_NOT_SUPPORTED		      282
#define ERROR_NEED_EAS_FOUND		      283
#define ERROR_DUPLICATE_HANDLE		      284
#define ERROR_DUPLICATE_NAME		      285
#define ERROR_EMPTY_MUXWAIT		      286
#define ERROR_MUTEX_OWNED		      287
#define ERROR_NOT_OWNER			      288
#define ERROR_PARAM_TOO_SMALL		      289
#define ERROR_TOO_MANY_HANDLES		      290
#define ERROR_TOO_MANY_OPENS		      291
#define ERROR_WRONG_TYPE		      292
#define ERROR_UNUSED_CODE		      293
#define ERROR_THREAD_NOT_TERMINATED	      294
#define ERROR_INIT_ROUTINE_FAILED	      295
#define ERROR_MODULE_IN_USE		      296
#define ERROR_NOT_ENOUGH_WATCHPOINTS	      297
#define ERROR_TOO_MANY_POSTS		      298
#define ERROR_ALREADY_POSTED		      299
#define ERROR_ALREADY_RESET		      300
#define ERROR_SEM_BUSY			      301
#define ERROR_INVALID_PROCID		      303
#define ERROR_INVALID_PDELTA		      304
#define ERROR_NOT_DESCENDANT		      305
#define ERROR_NOT_SESSION_MANAGER	      306
#define ERROR_INVALID_PCLASS		      307
#define ERROR_INVALID_SCOPE		      308
#define ERROR_INVALID_THREADID		      309
#define ERROR_DOSSUB_SHRINK		      310
#define ERROR_DOSSUB_NOMEM		      311
#define ERROR_DOSSUB_OVERLAP		      312
#define ERROR_DOSSUB_BADSIZE		      313
#define ERROR_DOSSUB_BADFLAG		      314
#define ERROR_DOSSUB_BADSELECTOR	      315
#define ERROR_MR_MSG_TOO_LONG		      316
#define ERROR_MR_MID_NOT_FOUND		      317
#define ERROR_MR_UN_ACC_MSGF		      318
#define ERROR_MR_INV_MSGF_FORMAT	      319
#define ERROR_MR_INV_IVCOUNT		      320
#define ERROR_MR_UN_PERFORM		      321
#define ERROR_TS_WAKEUP			      322
#define ERROR_TS_SEMHANDLE		      323
#define ERROR_TS_NOTIMER		      324
#define ERROR_TS_HANDLE			      326
#define ERROR_TS_DATETIME		      327
#define ERROR_SYS_INTERNAL		      328
#define ERROR_QUE_CURRENT_NAME		      329
#define ERROR_QUE_PROC_NOT_OWNED	      330
#define ERROR_QUE_PROC_OWNED		      331
#define ERROR_QUE_DUPLICATE		      332
#define ERROR_QUE_ELEMENT_NOT_EXIST	      333
#define ERROR_QUE_NO_MEMORY		      334
#define ERROR_QUE_INVALID_NAME		      335
#define ERROR_QUE_INVALID_PRIORITY	      336
#define ERROR_QUE_INVALID_HANDLE	      337
#define ERROR_QUE_LINK_NOT_FOUND	      338
#define ERROR_QUE_MEMORY_ERROR		      339
#define ERROR_QUE_PREV_AT_END		      340
#define ERROR_QUE_PROC_NO_ACCESS	      341
#define ERROR_QUE_EMPTY			      342
#define ERROR_QUE_NAME_NOT_EXIST	      343
#define ERROR_QUE_NOT_INITIALIZED	      344
#define ERROR_QUE_UNABLE_TO_ACCESS	      345
#define ERROR_QUE_UNABLE_TO_ADD		      346
#define ERROR_QUE_UNABLE_TO_INIT	      347
#define ERROR_VIO_INVALID_MASK		      349
#define ERROR_VIO_PTR			      350
#define ERROR_VIO_APTR			      351
#define ERROR_VIO_RPTR			      352
#define ERROR_VIO_CPTR			      353
#define ERROR_VIO_LPTR			      354
#define ERROR_VIO_MODE			      355
#define ERROR_VIO_WIDTH			      356
#define ERROR_VIO_ATTR			      357
#define ERROR_VIO_ROW			      358
#define ERROR_VIO_COL			      359
#define ERROR_VIO_TOPROW		      360
#define ERROR_VIO_BOTROW		      361
#define ERROR_VIO_RIGHTCOL		      362
#define ERROR_VIO_LEFTCOL		      363
#define ERROR_SCS_CALL			      364
#define ERROR_SCS_VALUE			      365
#define ERROR_VIO_WAIT_FLAG		      366
#define ERROR_VIO_UNLOCK		      367
#define ERROR_SGS_NOT_SESSION_MGR	      368
#define ERROR_SMG_INVALID_SGID		      369
#define ERROR_SMG_INVALID_SESSION_ID	      369 /*!*/
#define ERROR_SMG_NOSG			      370
#define ERROR_SMG_NO_SESSIONS		      370 /*!*/
#define ERROR_SMG_GRP_NOT_FOUND		      371
#define ERROR_SMG_SESSION_NOT_FOUND	      371 /*!*/
#define ERROR_SMG_SET_TITLE		      372
#define ERROR_KBD_PARAMETER		      373
#define ERROR_KBD_NO_DEVICE		      374
#define ERROR_KBD_INVALID_IOWAIT	      375
#define ERROR_KBD_INVALID_LENGTH	      376
#define ERROR_KBD_INVALID_ECHO_MASK	      377
#define ERROR_KBD_INVALID_INPUT_MASK	      378
#define ERROR_MON_INVALID_PARMS		      379
#define ERROR_MON_INVALID_DEVNAME	      380
#define ERROR_MON_INVALID_HANDLE	      381
#define ERROR_MON_BUFFER_TOO_SMALL	      382
#define ERROR_MON_BUFFER_EMPTY		      383
#define ERROR_MON_DATA_TOO_LARGE	      384
#define ERROR_MOUSE_NO_DEVICE		      385
#define ERROR_MOUSE_INV_HANDLE		      386
#define ERROR_MOUSE_INV_PARMS		      387
#define ERROR_MOUSE_CANT_RESET		      388
#define ERROR_MOUSE_DISPLAY_PARMS	      389
#define ERROR_MOUSE_INV_MODULE		      390
#define ERROR_MOUSE_INV_ENTRY_PT	      391
#define ERROR_MOUSE_INV_MASK		      392
#define NO_ERROR_MOUSE_NO_DATA		      393
#define NO_ERROR_MOUSE_PTR_DRAWN	      394
#define ERROR_INVALID_FREQUENCY		      395
#define ERROR_NLS_NO_COUNTRY_FILE	      396
#define ERROR_NLS_OPEN_FAILED		      397
#define ERROR_NLS_NO_CTRY_CODE		      398
#define ERROR_NLS_TABLE_TRUNCATED	      399
#define ERROR_NLS_BAD_TYPE		      400
#define ERROR_NLS_TYPE_NOT_FOUND	      401
#define ERROR_VIO_SMG_ONLY		      402
#define ERROR_VIO_INVALID_ASCIIZ	      403
#define ERROR_VIO_DEREGISTER		      404
#define ERROR_VIO_NO_POPUP		      405
#define ERROR_VIO_EXISTING_POPUP	      406
#define ERROR_KBD_SMG_ONLY		      407
#define ERROR_KBD_INVALID_ASCIIZ	      408
#define ERROR_KBD_INVALID_MASK		      409
#define ERROR_KBD_REGISTER		      410
#define ERROR_KBD_DEREGISTER		      411
#define ERROR_MOUSE_SMG_ONLY		      412
#define ERROR_MOUSE_INVALID_ASCIIZ	      413
#define ERROR_MOUSE_INVALID_MASK	      414
#define ERROR_MOUSE_REGISTER		      415
#define ERROR_MOUSE_DEREGISTER		      416
#define ERROR_SMG_BAD_ACTION		      417
#define ERROR_SMG_INVALID_CALL		      418
#define ERROR_SCS_SG_NOTFOUND		      419
#define ERROR_SCS_NOT_SHELL		      420
#define ERROR_VIO_INVALID_PARMS		      421
#define ERROR_VIO_FUNCTION_OWNED	      422
#define ERROR_VIO_RETURN		      423
#define ERROR_SCS_INVALID_FUNCTION	      424
#define ERROR_SCS_NOT_SESSION_MGR	      425
#define ERROR_VIO_REGISTER		      426
#define ERROR_VIO_NO_MODE_THREAD	      427
#define ERROR_VIO_NO_SAVE_RESTORE_THD	      428
#define ERROR_VIO_IN_BG			      429
#define ERROR_VIO_ILLEGAL_DURING_POPUP	      430
#define ERROR_SMG_NOT_BASESHELL		      431
#define ERROR_SMG_BAD_STATUSREQ		      432
#define ERROR_QUE_INVALID_WAIT		      433
#define ERROR_VIO_LOCK			      434
#define ERROR_MOUSE_INVALID_IOWAIT	      435
#define ERROR_VIO_INVALID_HANDLE	      436
#define ERROR_VIO_ILLEGAL_DURING_LOCK	      437
#define ERROR_VIO_INVALID_LENGTH	      438
#define ERROR_KBD_INVALID_HANDLE	      439
#define ERROR_KBD_NO_MORE_HANDLE	      440
#define ERROR_KBD_CANNOT_CREATE_KCB	      441
#define ERROR_KBD_CODEPAGE_LOAD_INCOMPL	      442
#define ERROR_KBD_INVALID_CODEPAGE_ID	      443
#define ERROR_KBD_NO_CODEPAGE_SUPPORT	      444
#define ERROR_KBD_FOCUS_REQUIRED	      445
#define ERROR_KBD_FOCUS_ALREADY_ACTIVE	      446
#define ERROR_KBD_KEYBOARD_BUSY		      447
#define ERROR_KBD_INVALID_CODEPAGE	      448
#define ERROR_KBD_UNABLE_TO_FOCUS	      449
#define ERROR_SMG_SESSION_NON_SELECT	      450
#define ERROR_SMG_SESSION_NOT_FOREGRND	      451
#define ERROR_SMG_SESSION_NOT_PARENT	      452
#define ERROR_SMG_INVALID_START_MODE	      453
#define ERROR_SMG_INVALID_RELATED_OPT	      454
#define ERROR_SMG_INVALID_BOND_OPTION	      455
#define ERROR_SMG_INVALID_SELECT_OPT	      456
#define ERROR_SMG_START_IN_BACKGROUND	      457
#define ERROR_SMG_INVALID_STOP_OPTION	      458
#define ERROR_SMG_BAD_RESERVE		      459
#define ERROR_SMG_PROCESS_NOT_PARENT	      460
#define ERROR_SMG_INVALID_DATA_LENGTH	      461
#define ERROR_SMG_NOT_BOUND		      462
#define ERROR_SMG_RETRY_SUB_ALLOC	      463
#define ERROR_KBD_DETACHED		      464
#define ERROR_VIO_DETACHED		      465
#define ERROR_MOU_DETACHED		      466
#define ERROR_VIO_FONT			      467
#define ERROR_VIO_USER_FONT		      468
#define ERROR_VIO_BAD_CP		      469
#define ERROR_VIO_NO_CP			      470
#define ERROR_VIO_NA_CP			      471
#define ERROR_INVALID_CODE_PAGE		      472
#define ERROR_CPLIST_TOO_SMALL		      473
#define ERROR_CP_NOT_MOVED		      474
#define ERROR_MODE_SWITCH_INIT		      475
#define ERROR_CODE_PAGE_NOT_FOUND	      476
#define ERROR_UNEXPECTED_SLOT_RETURNED	      477
#define ERROR_SMG_INVALID_TRACE_OPTION	      478
#define ERROR_VIO_INTERNAL_RESOURCE	      479
#define ERROR_VIO_SHELL_INIT		      480
#define ERROR_SMG_NO_HARD_ERRORS	      481
#define ERROR_CP_SWITCH_INCOMPLETE	      482
#define ERROR_VIO_TRANSPARENT_POPUP	      483
#define ERROR_CRITSEC_OVERFLOW		      484
#define ERROR_CRITSEC_UNDERFLOW		      485
#define ERROR_VIO_BAD_RESERVE		      486
#define ERROR_INVALID_ADDRESS		      487
#define ERROR_ZERO_SELECTORS_REQUESTED	      488
#define ERROR_NOT_ENOUGH_SELECTORS_AVA	      489
#define ERROR_INVALID_SELECTOR		      490
#define ERROR_SMG_INVALID_PROGRAM_TYPE	      491
#define ERROR_SMG_INVALID_PGM_CONTROL	      492
#define ERROR_SMG_INVALID_INHERIT_OPT	      493
#define ERROR_VIO_EXTENDED_SG		      494
#define ERROR_VIO_NOT_PRES_MGR_SG	      495
#define ERROR_VIO_SHIELD_OWNED		      496
#define ERROR_VIO_NO_MORE_HANDLES	      497
#define ERROR_VIO_SEE_ERROR_LOG		      498
#define ERROR_VIO_ASSOCIATED_DC		      499
#define ERROR_KBD_NO_CONSOLE		      500
#define ERROR_MOUSE_NO_CONSOLE		      501
#define ERROR_MOUSE_INVALID_HANDLE	      502
#define ERROR_SMG_INVALID_DEBUG_PARMS	      503
#define ERROR_KBD_EXTENDED_SG		      504
#define ERROR_MOU_EXTENDED_SG		      505
#define ERROR_SMG_INVALID_ICON_FILE	      506
#define ERROR_TRC_PID_NON_EXISTENT	      507
#define ERROR_TRC_COUNT_ACTIVE		      508
#define ERROR_TRC_SUSPENDED_BY_COUNT	      509
#define ERROR_TRC_COUNT_INACTIVE	      510
#define ERROR_TRC_COUNT_REACHED		      511
#define ERROR_NO_MC_TRACE		      512
#define ERROR_MC_TRACE			      513
#define ERROR_TRC_COUNT_ZERO		      514
#define ERROR_SMG_TOO_MANY_DDS		      515
#define ERROR_SMG_INVALID_NOTIFICATION	      516
#define ERROR_LF_INVALID_FUNCTION	      517
#define ERROR_LF_NOT_AVAIL		      518
#define ERROR_LF_SUSPENDED		      519
#define ERROR_LF_BUF_TOO_SMALL		      520
#define ERROR_LF_BUFFER_CORRUPTED	      521
#define ERROR_LF_BUFFER_FULL		      521 /*!*/
#define ERROR_LF_INVALID_DAEMON		      522
#define ERROR_LF_INVALID_RECORD		      522 /*!*/
#define ERROR_LF_INVALID_TEMPL		      523
#define ERROR_LF_INVALID_SERVICE	      523 /*!*/
#define ERROR_LF_GENERAL_FAILURE	      524
#define ERROR_LF_INVALID_ID		      525
#define ERROR_LF_INVALID_HANDLE		      526
#define ERROR_LF_NO_ID_AVAIL		      527
#define ERROR_LF_TEMPLATE_AREA_FULL	      528
#define ERROR_LF_ID_IN_USE		      529
#define ERROR_MOU_NOT_INITIALIZED	      530
#define ERROR_MOUINITREAL_DONE		      531
#define ERROR_DOSSUB_CORRUPTED		      532
#define ERROR_MOUSE_CALLER_NOT_SUBSYS	      533
#define ERROR_ARITHMETIC_OVERFLOW	      534
#define ERROR_TMR_NO_DEVICE		      535
#define ERROR_TMR_INVALID_TIME		      536
#define ERROR_PVW_INVALID_ENTITY	      537
#define ERROR_PVW_INVALID_ENTITY_TYPE	      538
#define ERROR_PVW_INVALID_SPEC		      539
#define ERROR_PVW_INVALID_RANGE_TYPE	      540
#define ERROR_PVW_INVALID_COUNTER_BLK	      541
#define ERROR_PVW_INVALID_TEXT_BLK	      542
#define ERROR_PRF_NOT_INITIALIZED	      543
#define ERROR_PRF_ALREADY_INITIALIZED	      544
#define ERROR_PRF_NOT_STARTED		      545
#define ERROR_PRF_ALREADY_STARTED	      546
#define ERROR_PRF_TIMER_OUT_OF_RANGE	      547
#define ERROR_PRF_TIMER_RESET		      548
#define ERROR_VDD_LOCK_USEAGE_DENIED	      639
#define ERROR_TIMEOUT			      640
#define ERROR_VDM_DOWN			      641
#define ERROR_VDM_LIMIT			      642
#define ERROR_VDD_NOT_FOUND		      643
#define ERROR_INVALID_CALLER		      644
#define ERROR_PID_MISMATCH		      645
#define ERROR_INVALID_VDD_HANDLE	      646
#define ERROR_VLPT_NO_SPOOLER		      647
#define ERROR_VCOM_DEVICE_BUSY		      648
#define ERROR_VLPT_DEVICE_BUSY		      649
#define ERROR_NESTING_TOO_DEEP		      650
#define ERROR_VDD_MISSING		      651
#define ERROR_BIDI_INVALID_LENGTH	      671
#define ERROR_BIDI_INVALID_INCREMENT	      672
#define ERROR_BIDI_INVALID_COMBINATION	      673
#define ERROR_BIDI_INVALID_RESERVED	      674
#define ERROR_BIDI_INVALID_EFFECT	      675
#define ERROR_BIDI_INVALID_CSDREC	      676
#define ERROR_BIDI_INVALID_CSDSTATE	      677
#define ERROR_BIDI_INVALID_LEVEL	      678
#define ERROR_BIDI_INVALID_TYPE_SUPPORT	      679
#define ERROR_BIDI_INVALID_ORIENTATION	      680
#define ERROR_BIDI_INVALID_NUM_SHAPE	      681
#define ERROR_BIDI_INVALID_CSD		      682
#define ERROR_BIDI_NO_SUPPORT		      683
#define NO_ERROR_BIDI_RW_INCOMPLETE	      684
#define ERROR_IMP_INVALID_PARM		      691
#define ERROR_IMP_INVALID_LENGTH	      692
#define ERROR_MON_BAD_BUFFER		      730
#define ERROR_MODULE_CORRUPTED		      731
#define ERROR_SM_OUTOF_SWAPFILE		     1477
#define ERROR_LF_TIMEOUT		     2055
#define ERROR_LF_SUSPEND_SUCCESS	     2057
#define ERROR_LF_RESUME_SUCCESS		     2058
#define ERROR_LF_REDIRECT_SUCCESS	     2059
#define ERROR_LF_REDIRECT_FAILURE	     2060
#define ERROR_SWAPPER_NOT_ACTIVE	    32768
#define ERROR_INVALID_SWAPID		    32769
#define ERROR_IOERR_SWAP_FILE		    32770
#define ERROR_SWAP_TABLE_FULL		    32771
#define ERROR_SWAP_FILE_FULL		    32772
#define ERROR_CANT_INIT_SWAPPER		    32773
#define ERROR_SWAPPER_ALREADY_INIT	    32774
#define ERROR_PMM_INSUFFICIENT_MEMORY	    32775
#define ERROR_PMM_INVALID_FLAGS		    32776
#define ERROR_PMM_INVALID_ADDRESS	    32777
#define ERROR_PMM_LOCK_FAILED		    32778
#define ERROR_PMM_UNLOCK_FAILED		    32779
#define ERROR_PMM_MOVE_INCOMPLETE	    32780
#define ERROR_UCOM_DRIVE_RENAMED	    32781
#define ERROR_UCOM_FILENAME_TRUNCATED	    32782
#define ERROR_UCOM_BUFFER_LENGTH	    32783
#define ERROR_MON_CHAIN_HANDLE		    32784
#define ERROR_MON_NOT_REGISTERED	    32785
#define ERROR_SMG_ALREADY_TOP		    32786
#define ERROR_PMM_ARENA_MODIFIED	    32787
#define ERROR_SMG_PRINTER_OPEN		    32788
#define ERROR_PMM_SET_FLAGS_FAILED	    32789
#define ERROR_INVALID_DOS_DD		    32790
#define ERROR_BLOCKED			    32791
#define ERROR_NOBLOCK			    32792
#define ERROR_INSTANCE_SHARED		    32793
#define ERROR_NO_OBJECT			    32794
#define ERROR_PARTIAL_ATTACH		    32795
#define ERROR_INCACHE			    32796
#define ERROR_SWAP_IO_PROBLEMS		    32797
#define ERROR_CROSSES_OBJECT_BOUNDARY	    32798
#define ERROR_LONGLOCK			    32799
#define ERROR_SHORTLOCK			    32800
#define ERROR_UVIRTLOCK			    32801
#define ERROR_ALIASLOCK			    32802
#define ERROR_ALIAS			    32803
#define ERROR_NO_MORE_HANDLES		    32804
#define ERROR_SCAN_TERMINATED		    32805
#define ERROR_TERMINATOR_NOT_FOUND	    32806
#define ERROR_NOT_DIRECT_CHILD		    32807
#define ERROR_DELAY_FREE		    32808
#define ERROR_GUARDPAGE			    32809
#define ERROR_SWAPERROR			    32900
#define ERROR_LDRERROR			    32901
#define ERROR_NOMEMORY			    32902
#define ERROR_NOACCESS			    32903
#define ERROR_NO_DLL_TERM		    32904
#define ERROR_CPSIO_CODE_PAGE_INVALID	    65026
#define ERROR_CPSIO_NO_SPOOLER		    65027
#define ERROR_CPSIO_FONT_ID_INVALID	    65028
#define ERROR_CPSIO_INTERNAL_ERROR	    65033
#define ERROR_CPSIO_INVALID_PTR_NAME	    65034
#define ERROR_CPSIO_NOT_ACTIVE		    65037
#define ERROR_CPSIO_PID_FULL		    65039
#define ERROR_CPSIO_PID_NOT_FOUND	    65040
#define ERROR_CPSIO_READ_CTL_SEQ	    65043
#define ERROR_CPSIO_READ_FNT_DEF	    65045
#define ERROR_CPSIO_WRITE_ERROR		    65047
#define ERROR_CPSIO_WRITE_FULL_ERROR	    65048
#define ERROR_CPSIO_WRITE_HANDLE_BAD	    65049
#define ERROR_CPSIO_SWIT_LOAD		    65074
#define ERROR_CPSIO_INV_COMMAND		    65077
#define ERROR_CPSIO_NO_FONT_SWIT	    65078
#define ERROR_ENTRY_IS_CALLGATE		    65079

#endif /* INCL_DOSERRORS */

/* ----------------------------- ERRORS ----------------------------------- */

#if defined (INCL_DOSERRORS)

#define ERRACT_RETRY		1
#define ERRACT_DLYRET		2
#define ERRACT_USER		3
#define ERRACT_ABORT		4
#define ERRACT_PANIC		5
#define ERRACT_IGNORE		6
#define ERRACT_INTRET		7

#define ERRCLASS_OUTRES		1
#define ERRCLASS_TEMPSIT	2
#define ERRCLASS_AUTH		3
#define ERRCLASS_INTRN		4
#define ERRCLASS_HRDFAIL	5
#define ERRCLASS_SYSFAIL	6
#define ERRCLASS_APPERR		7
#define ERRCLASS_NOTFND		8
#define ERRCLASS_BADFMT		9
#define ERRCLASS_LOCKED		10
#define ERRCLASS_MEDIA		11
#define ERRCLASS_ALREADY	12
#define ERRCLASS_UNK		13
#define ERRCLASS_CANT		14
#define ERRCLASS_TIME		15

#define ERRLOC_UNK		1
#define ERRLOC_DISK		2
#define ERRLOC_NET		3
#define ERRLOC_SERDEV		4
#define ERRLOC_MEM		5

#endif /* INCL_DOSERRORS */

#if defined (INCL_DOSMISC)

#define FERR_DISABLEHARDERR	0x0000L
#define FERR_ENABLEHARDERR	0x0001L
#define FERR_ENABLEEXCEPTION	0x0000L
#define FERR_DISABLEEXCEPTION	0x0002L

#define BEGIN_LIBPATH		1
#define END_LIBPATH		2

ULONG DosErrClass (ULONG ulCode, PULONG pulClass, PULONG pulAction,
    PULONG pulLocus);
ULONG DosError (ULONG ulError);
ULONG DosQueryExtLIBPATH (PCSZ pszExtLIBPATH, ULONG flags);
ULONG DosSetExtLIBPATH (PCSZ pszExtLIBPATH, ULONG flags);

#endif /* INCL_DOSMISC */

/* ----------------------------- FONTS ------------------------------------ */

#define FACESIZE 32

#define FATTR_SEL_ITALIC		0x0001
#define FATTR_SEL_UNDERSCORE		0x0002
#define FATTR_SEL_OUTLINE		0x0008
#define FATTR_SEL_STRIKEOUT		0x0010
#define FATTR_SEL_BOLD			0x0020

#define FATTR_TYPE_KERNING		0x0004
#define FATTR_TYPE_MBCS			0x0008
#define FATTR_TYPE_DBCS			0x0010
#define FATTR_TYPE_ANTIALIASED		0x0020

#define FATTR_FONTUSE_NOMIX		0x0002
#define FATTR_FONTUSE_OUTLINE		0x0004
#define FATTR_FONTUSE_TRANSFORMABLE	0x0008

#define FM_TYPE_FIXED			0x0001
#define FM_TYPE_LICENSED		0x0002
#define FM_TYPE_KERNING			0x0004
#define FM_TYPE_DBCS			0x0010
#define FM_TYPE_MBCS			0x0018
#define FM_TYPE_64K			0x8000
#define FM_TYPE_ATOMS			0x4000
#define FM_TYPE_FAMTRUNC		0x2000
#define FM_TYPE_FACETRUNC		0x1000

#define FM_DEFN_OUTLINE			0x0001
#define FM_DEFN_IFI			0x0002
#define FM_DEFN_WIN			0x0004
#define FM_DEFN_GENERIC			0x8000

#define FM_SEL_ITALIC			0x0001
#define FM_SEL_UNDERSCORE		0x0002
#define FM_SEL_NEGATIVE			0x0004
#define FM_SEL_OUTLINE			0x0008
#define FM_SEL_STRIKEOUT		0x0010
#define FM_SEL_BOLD			0x0020
#define FM_SEL_ISO9241_TESTED		0x0040

#define FM_CAP_NOMIX			0x0001

#define FM_ISO_9518_640			0x01
#define FM_ISO_9515_640			0x02
#define FM_ISO_9515_1024		0x04
#define FM_ISO_9517_640			0x08
#define FM_ISO_9517_1024		0x10


typedef struct _PANOSE
{
  BYTE bFamilyType;
  BYTE bSerifStyle;
  BYTE bWeight;
  BYTE bProportion;
  BYTE bContrast;
  BYTE bStrokeVariation;
  BYTE bArmStyle;
  BYTE bLetterform;
  BYTE bMidline;
  BYTE bXHeight;
  BYTE fbPassedISO;
  BYTE fbFailedISO;
} PANOSE;

typedef struct _FONTMETRICS
{
  CHAR	 szFamilyname[FACESIZE];
  CHAR	 szFacename[FACESIZE];
  USHORT idRegistry;
  USHORT usCodePage;
  LONG	 lEmHeight;
  LONG	 lXHeight;
  LONG	 lMaxAscender;
  LONG	 lMaxDescender;
  LONG	 lLowerCaseAscent;
  LONG	 lLowerCaseDescent;
  LONG	 lInternalLeading;
  LONG	 lExternalLeading;
  LONG	 lAveCharWidth;
  LONG	 lMaxCharInc;
  LONG	 lEmInc;
  LONG	 lMaxBaselineExt;
  SHORT	 sCharSlope;
  SHORT	 sInlineDir;
  SHORT	 sCharRot;
  USHORT usWeightClass;
  USHORT usWidthClass;
  SHORT	 sXDeviceRes;
  SHORT	 sYDeviceRes;
  SHORT	 sFirstChar;
  SHORT	 sLastChar;
  SHORT	 sDefaultChar;
  SHORT	 sBreakChar;
  SHORT	 sNominalPointSize;
  SHORT	 sMinimumPointSize;
  SHORT	 sMaximumPointSize;
  USHORT fsType;
  USHORT fsDefn;
  USHORT fsSelection;
  USHORT fsCapabilities;
  LONG	 lSubscriptXSize;
  LONG	 lSubscriptYSize;
  LONG	 lSubscriptXOffset;
  LONG	 lSubscriptYOffset;
  LONG	 lSuperscriptXSize;
  LONG	 lSuperscriptYSize;
  LONG	 lSuperscriptXOffset;
  LONG	 lSuperscriptYOffset;
  LONG	 lUnderscoreSize;
  LONG	 lUnderscorePosition;
  LONG	 lStrikeoutSize;
  LONG	 lStrikeoutPosition;
  SHORT	 sKerningPairs;
  SHORT	 sFamilyClass;
  LONG	 lMatch;
  LONG	 FamilyNameAtom;
  LONG	 FaceNameAtom;
  PANOSE panose;
} FONTMETRICS;
typedef FONTMETRICS *PFONTMETRICS;

typedef struct _FATTRS
{
  USHORT usRecordLength;
  USHORT fsSelection;
  LONG	 lMatch;
  CHAR	 szFacename[FACESIZE];
  USHORT idRegistry;
  USHORT usCodePage;
  LONG	 lMaxBaselineExt;
  LONG	 lAveCharWidth;
  USHORT fsType;
  USHORT fsFontUse;
} FATTRS;
typedef FATTRS *PFATTRS;

/* ------------------------- MEMORY MANAGEMENT ---------------------------- */

#if defined (INCL_DOSMEMMGR) || !defined (INCL_NOCOMMON)

#define PAG_READ		0x0001
#define PAG_WRITE		0x0002
#define PAG_EXECUTE		0x0004
#define PAG_GUARD		0x0008
#define PAG_COMMIT		0x0010
#define PAG_DECOMMIT		0x0020
#define OBJ_TILE		0x0040
#define OBJ_PROTECTED		0x0080
#define OBJ_GETTABLE		0x0100
#define OBJ_GIVEABLE		0x0200
#define PAG_DEFAULT		0x0400
#define PAG_SHARED		0x2000
#define PAG_FREE		0x4000
#define PAG_BASE		0x00010000

#define DOSSUB_INIT		0x0001
#define DOSSUB_GROW		0x0002
#define DOSSUB_SPARSE_OBJ	0x0004
#define DOSSUB_SERIALIZE	0x0008

#define fPERM			(PAG_EXECUTE | PAG_READ | PAG_WRITE)
#define fSHARE			(OBJ_GETTABLE | OBJ_GIVEABLE)
#define fALLOC			(fPERM | OBJ_TILE | PAG_COMMIT)
#define fALLOCSHR		(fPERM | fSHARE | OBJ_TILE | PAG_COMMIT)
#define fGETNMSHR		(fPERM)
#define fGETSHR			(fPERM)
#define fGIVESHR		(fPERM)
#define fSET			(fPERM|PAG_COMMIT|PAG_DECOMMIT|PAG_DEFAULT)

ULONG DosAllocMem (PPVOID pBaseAddress, ULONG ulObjectSize,
    ULONG ulAllocationFlags);
ULONG DosAllocSharedMem (PPVOID pBaseAddress, PCSZ pszName,
    ULONG ulObjectSize, ULONG ulAllocationFlags);
ULONG DosFreeMem (PVOID pBaseAddress);
ULONG DosGetNamedSharedMem (PPVOID pBaseAddress, PCSZ pszSharedMemName,
    ULONG ulAttributeFlags);
ULONG DosGetSharedMem (CPVOID pBaseAddress, ULONG ulAttributeFlags);
ULONG DosGiveSharedMem (CPVOID pBaseAddress, PID idProcessId,
    ULONG ulAttributeFlags);
ULONG DosQueryMem (CPVOID pBaseAddress, PULONG pulRegionSize,
    PULONG pulAllocationFlags);
ULONG DosSetMem (CPVOID pBaseAddress, ULONG ulRegionSize,
    ULONG ulAttributeFlags);
ULONG DosSubAllocMem (PVOID pOffset, PPVOID pBlockOffset, ULONG ulSize);
ULONG DosSubFreeMem (PVOID pOffset, PVOID pBlockOffset, ULONG ulSize);
ULONG DosSubSetMem (PVOID pOffset, ULONG ulFlags, ULONG ulSize);
ULONG DosSubUnsetMem (PVOID pOffset);

#endif /* INCL_DOSMEMMGR || !INCL_NOCOMMON */

/* --------------------------- FILE SYSTEM -------------------------------- */

#define CCHMAXPATH			260
#define CCHMAXPATHCOMP			256

#if defined (INCL_DOSMISC)
#define DSP_IMPLIEDCUR			1
#define DSP_PATHREF			2
#define DSP_IGNORENETERR		4
#endif

#if defined (INCL_DOSFILEMGR) || !defined (INCL_NOCOMMON)

#define DCPY_EXISTING			0x0001
#define DCPY_APPEND			0x0002
#define DCPY_FAILEAS			0x0004

#define DSPI_WRTTHRU			0x0010

#define EAT_BINARY			0xfffe
#define EAT_ASCII			0xfffd
#define EAT_BITMAP			0xfffb
#define EAT_METAFILE			0xfffa
#define EAT_ICON			0xfff9
#define EAT_EA				0xffee
#define EAT_MVMT			0xffdf
#define EAT_MVST			0xffde
#define EAT_ASN1			0xffdd

#define ENUMEA_LEVEL_NO_VALUE		1

#define ENUMEA_REFTYPE_FHANDLE		0
#define ENUMEA_REFTYPE_PATH		1
#define ENUMEA_REFTYPE_MAX		ENUMEA_REFTYPE_PATH

#define ENUMEA_REFTYPE_FHANDLE		0
#define ENUMEA_REFTYPE_PATH		1

#define FEA_NEEDEA			0x80

#define FHB_DSKREMOTE			0x8000
#define FHB_CHRDEVREMOTE		0x8000
#define FHB_PIPEREMOTE			0x8000

#define FHT_DISKFILE			0x0000
#define FHT_CHRDEV			0x0001
#define FHT_PIPE			0x0002

#define FIL_STANDARD			1
#define FIL_QUERYEASIZE			2
#define FIL_QUERYEASFROMLIST		3
#define FIL_QUERYFULLNAME		5 /* DosQueryPathInfo */

#define FILE_BEGIN			0
#define FILE_CURRENT			1
#define FILE_END			2

#define FILE_NORMAL			0x0000
#define FILE_READONLY			0x0001
#define FILE_HIDDEN			0x0002
#define FILE_SYSTEM			0x0004
#define FILE_DIRECTORY			0x0010
#define FILE_ARCHIVED			0x0020

#define FILE_IGNORE			0x10000

#define FILE_EXISTED			0x0001
#define FILE_CREATED			0x0002
#define FILE_TRUNCATED			0x0003

#define FILE_OPEN			0x0001
#define FILE_TRUNCATE			0x0002
#define FILE_CREATE			0x0010

#define FS_ATTACH			0
#define FS_DETACH			1
#define FS_SPOOLATTACH			2
#define FS_SPOOLDETACH			3

#define FSAIL_QUERYNAME			1
#define FSAIL_DEVNUMBER			2
#define FSAIL_DRVNUMBER			3

#define FSAT_CHARDEV			1
#define FSAT_PSEUDODEV			2
#define FSAT_LOCALDRV			3
#define FSAT_REMOTEDRV			4

#define FSCTL_HANDLE			1
#define FSCTL_PATHNAME			2
#define FSCTL_FSDNAME			3

#define FSCTL_ERROR_INFO		1
#define FSCTL_MAX_EASIZE		2
#define FSCTL_GET_NEXT_ROUTE_NAME	3
#define FSCTL_DAEMON_QUERY		4

#define FSCTL_QUERY_COMPLETE		0
#define FSCTL_QUERY_AGAIN		1

#define FSIL_ALLOC			1
#define FSIL_VOLSER			2

#define HANDTYPE_FILE			0x0000
#define HANDTYPE_DEVICE			0x0001
#define HANDTYPE_PIPE			0x0002
#define HANDTYPE_PROTECTED		0x4000
#define HANDTYPE_NETWORK		0x8000

#define HDIR_SYSTEM			1
#define HDIR_CREATE			((HDIR)-1)

#define MUST_HAVE_READONLY	(FILE_READONLY	| (FILE_READONLY  << 8))
#define MUST_HAVE_HIDDEN	(FILE_HIDDEN	| (FILE_HIDDEN	  << 8))
#define MUST_HAVE_SYSTEM	(FILE_SYSTEM	| (FILE_SYSTEM	  << 8))
#define MUST_HAVE_DIRECTORY	(FILE_DIRECTORY | (FILE_DIRECTORY << 8))
#define MUST_HAVE_ARCHIVED	(FILE_ARCHIVED	| (FILE_ARCHIVED  << 8))

#define OPEN_ACTION_FAIL_IF_EXISTS     0x0000
#define OPEN_ACTION_OPEN_IF_EXISTS     0x0001
#define OPEN_ACTION_REPLACE_IF_EXISTS  0x0002
#define OPEN_ACTION_FAIL_IF_NEW	       0x0000
#define OPEN_ACTION_CREATE_IF_NEW      0x0010

#define OPEN_ACCESS_READONLY	       0x0000
#define OPEN_ACCESS_WRITEONLY	       0x0001
#define OPEN_ACCESS_READWRITE	       0x0002

#define OPEN_SHARE_DENYREADWRITE       0x0010
#define OPEN_SHARE_DENYWRITE	       0x0020
#define OPEN_SHARE_DENYREAD	       0x0030
#define OPEN_SHARE_DENYNONE	       0x0040

#define OPEN_FLAGS_NOINHERIT	       0x0080
#define OPEN_FLAGS_NO_LOCALITY	       0x0000
#define OPEN_FLAGS_SEQUENTIAL	       0x0100
#define OPEN_FLAGS_RANDOM	       0x0200
#define OPEN_FLAGS_RANDOMSEQUENTIAL    0x0300
#define OPEN_FLAGS_NO_CACHE	       0x1000
#define OPEN_FLAGS_FAIL_ON_ERROR       0x2000
#define OPEN_FLAGS_WRITE_THROUGH       0x4000
#define OPEN_FLAGS_DASD		       0x8000
#define OPEN_FLAGS_NONSPOOLED	       0x40000
#define OPEN_FLAGS_PROTECTED_HANDLE    0x40000000

#define SEARCH_PATH			0x0000
#define SEARCH_CUR_DIRECTORY		0x0001
#define SEARCH_ENVIRONMENT		0x0002
#define SEARCH_IGNORENETERRS		0x0004


typedef LHANDLE HFILE;
typedef HFILE *PHFILE;

typedef ULONG FHLOCK;
typedef PULONG PFHLOCK;

typedef LHANDLE HDIR;
typedef HDIR *PHDIR;


typedef struct _FTIME
{
  USHORT twosecs : 5;
  USHORT minutes : 6;
  USHORT hours	 : 5;
} FTIME;
typedef FTIME *PFTIME;

typedef struct _FDATE
{
  USHORT day   : 5;
  USHORT month : 4;
  USHORT year  : 7;
} FDATE;
typedef FDATE *PFDATE;

typedef struct _FEA
{
  BYTE	 fEA;
  BYTE	 cbName;
  USHORT cbValue;
} FEA;
typedef FEA *PFEA;

typedef struct _FEALIST
{
  ULONG cbList;
  FEA	list[1];
} FEALIST;
typedef FEALIST *PFEALIST;

typedef struct _GEA
{
  BYTE cbName;
  CHAR szName[1];
} GEA;
typedef GEA *PGEA;

typedef struct _GEALIST
{
  ULONG cbList;
  GEA	list[1];
} GEALIST;
typedef GEALIST *PGEALIST;

typedef struct _EAOP
{
  PGEALIST fpGEAList;
  PFEALIST fpFEAList;
  ULONG	   oError;
} EAOP;
typedef EAOP *PEAOP;

typedef struct _FEA2
{
  ULONG	 oNextEntryOffset;
  BYTE	 fEA;
  BYTE	 cbName;
  USHORT cbValue;
  CHAR	 szName[1];
} FEA2;
typedef FEA2 *PFEA2;

typedef struct _FEA2LIST
{
  ULONG cbList;
  FEA2	list[1];
} FEA2LIST;
typedef FEA2LIST *PFEA2LIST;

typedef struct _GEA2
{
  ULONG oNextEntryOffset;
  BYTE	cbName;
  CHAR	szName[1];
} GEA2;
typedef GEA2 *PGEA2;

typedef struct _GEA2LIST
{
  ULONG cbList;
  GEA2	list[1];
} GEA2LIST;
typedef GEA2LIST *PGEA2LIST;

typedef struct _EAOP2
{
  PGEA2LIST fpGEA2List;
  PFEA2LIST fpFEA2List;
  ULONG	    oError;
} EAOP2;
typedef EAOP2 *PEAOP2;

typedef struct _DENA1
{
  UCHAR	 reserved;
  UCHAR	 cbName;
  USHORT cbValue;
  UCHAR	 szName[1];
} DENA1;
typedef DENA1 *PDENA1;

typedef FEA2 DENA2;
typedef PFEA2 PDENA2;

typedef struct _EASIZEBUF
{
  USHORT cbMaxEASize;
  ULONG	 cbMaxEAListSize;	/* Packed? */
} EASIZEBUF;
typedef EASIZEBUF *PEASIZEBUF;

typedef struct _ROUTENAMEBUF
{
  ULONG hRouteHandle;
  UCHAR szRouteName;
} ROUTENAMEBUF;
typedef ROUTENAMEBUF *PROUTENAMEBUF;

typedef struct _FSDTHREAD
{
  USHORT usFunc;
  USHORT usStackSize;
  ULONG	 ulPriorityClass;
  LONG	 lPriorityLevel;
} FSDTHREAD;

typedef struct _FSDDAEMON
{
  USHORT    usNumThreads;
  USHORT    usMoreFlag;
  USHORT    usCallInstance;
  FSDTHREAD tdThrds[16];
} FSDDAEMON;

typedef struct _FILEFINDBUF
{
  FDATE	 fdateCreation;
  FTIME	 ftimeCreation;
  FDATE	 fdateLastAccess;
  FTIME	 ftimeLastAccess;
  FDATE	 fdateLastWrite;
  FTIME	 ftimeLastWrite;
  ULONG	 cbFile;
  ULONG	 cbFileAlloc;
  USHORT attrFile;
  UCHAR	 cchName;
  CHAR	 achName[CCHMAXPATHCOMP];
} FILEFINDBUF;
typedef FILEFINDBUF *PFILEFINDBUF;

typedef struct _FILEFINDBUF2
{
  FDATE	 fdateCreation;
  FTIME	 ftimeCreation;
  FDATE	 fdateLastAccess;
  FTIME	 ftimeLastAccess;
  FDATE	 fdateLastWrite;
  FTIME	 ftimeLastWrite;
  ULONG	 cbFile;
  ULONG	 cbFileAlloc;
  USHORT attrFile;
  ULONG	 cbList;
  UCHAR	 cchName;
  CHAR	 achName[CCHMAXPATHCOMP];
} FILEFINDBUF2;
typedef FILEFINDBUF2 *PFILEFINDBUF2;

typedef struct _FILEFINDBUF3
{
  ULONG oNextEntryOffset;
  FDATE fdateCreation;
  FTIME ftimeCreation;
  FDATE fdateLastAccess;
  FTIME ftimeLastAccess;
  FDATE fdateLastWrite;
  FTIME ftimeLastWrite;
  ULONG cbFile;
  ULONG cbFileAlloc;
  ULONG attrFile;
  UCHAR cchName;
  CHAR	achName[CCHMAXPATHCOMP];
} FILEFINDBUF3;
typedef FILEFINDBUF3 *PFILEFINDBUF3;

typedef struct _FILEFINDBUF4
{
  ULONG oNextEntryOffset;
  FDATE fdateCreation;
  FTIME ftimeCreation;
  FDATE fdateLastAccess;
  FTIME ftimeLastAccess;
  FDATE fdateLastWrite;
  FTIME ftimeLastWrite;
  ULONG cbFile;
  ULONG cbFileAlloc;
  ULONG attrFile;
  ULONG cbList;
  UCHAR cchName;
  CHAR	achName[CCHMAXPATHCOMP];
} FILEFINDBUF4;
typedef FILEFINDBUF4 *PFILEFINDBUF4;

typedef struct _FILELOCK
{
  LONG lOffset;
  LONG lRange;
} FILELOCK;
typedef FILELOCK *PFILELOCK;

typedef struct _FILESTATUS
{
  FDATE	 fdateCreation;
  FTIME	 ftimeCreation;
  FDATE	 fdateLastAccess;
  FTIME	 ftimeLastAccess;
  FDATE	 fdateLastWrite;
  FTIME	 ftimeLastWrite;
  ULONG	 cbFile;
  ULONG	 cbFileAlloc;
  USHORT attrFile;
} FILESTATUS;
typedef FILESTATUS *PFILESTATUS;

typedef struct _FILESTATUS2
{
  FDATE	 fdateCreation;
  FTIME	 ftimeCreation;
  FDATE	 fdateLastAccess;
  FTIME	 ftimeLastAccess;
  FDATE	 fdateLastWrite;
  FTIME	 ftimeLastWrite;
  ULONG	 cbFile;
  ULONG	 cbFileAlloc;
  USHORT attrFile;
  ULONG	 cbList;
} FILESTATUS2;
typedef FILESTATUS2 *PFILESTATUS2;

typedef struct _FILESTATUS3
{
  FDATE fdateCreation;
  FTIME ftimeCreation;
  FDATE fdateLastAccess;
  FTIME ftimeLastAccess;
  FDATE fdateLastWrite;
  FTIME ftimeLastWrite;
  ULONG cbFile;
  ULONG cbFileAlloc;
  ULONG attrFile;
} FILESTATUS3;
typedef FILESTATUS3 *PFILESTATUS3;

typedef struct _FILESTATUS4
{
  FDATE fdateCreation;
  FTIME ftimeCreation;
  FDATE fdateLastAccess;
  FTIME ftimeLastAccess;
  FDATE fdateLastWrite;
  FTIME ftimeLastWrite;
  ULONG cbFile;
  ULONG cbFileAlloc;
  ULONG attrFile;
  ULONG cbList;
} FILESTATUS4;
typedef FILESTATUS4 *PFILESTATUS4;

typedef struct _FSALLOCATE
{
  ULONG	 idFileSystem;
  ULONG	 cSectorUnit;
  ULONG	 cUnit;
  ULONG	 cUnitAvail;
  USHORT cbSector;
} FSALLOCATE;
typedef FSALLOCATE *PFSALLOCATE;

typedef struct _FSQBUFFER
{
  USHORT iType;
  USHORT cbName;
  UCHAR	 szName[1];
  USHORT cbFSDName;
  UCHAR	 szFSDName[1];
  USHORT cbFSAData;
  UCHAR	 rgFSAData[1];
} FSQBUFFER;
typedef FSQBUFFER *PFSQBUFFER;

typedef struct _FSQBUFFER2
{
  USHORT iType;
  USHORT cbName;
  USHORT cbFSDName;
  USHORT cbFSAData;
  UCHAR	 szName[1];
  UCHAR	 szFSDName[1];
  UCHAR	 rgFSAData[1];
} FSQBUFFER2;
typedef FSQBUFFER2 *PFSQBUFFER2;

typedef struct _SPOOLATTACH
{
  USHORT hNmPipe;
  ULONG	 ulKey;
} SPOOLATTACH;
typedef SPOOLATTACH *PSPOOLATTACH;

typedef struct _VOLUMELABEL
{
  BYTE cch;
  CHAR szVolLabel[12];
} VOLUMELABEL;
typedef VOLUMELABEL *PVOLUMELABEL;

typedef struct _FSINFO
{
  FDATE fdateCreation;
  FTIME ftimeCreation;
  VOLUMELABEL vol;
} FSINFO;
typedef FSINFO *PFSINFO;


ULONG DosCancelLockRequest (HFILE hFile, __const__ FILELOCK *pfl);
ULONG DosClose (HFILE hFile);
ULONG DosCopy (PCSZ pszSource, PCSZ pszTarget, ULONG ulOption);
ULONG DosCreateDir (PCSZ pszDirName, PEAOP2 pEABuf);
ULONG DosDelete (PCSZ pszFileName);
ULONG DosDeleteDir (PCSZ pszDirName);
ULONG DosDupHandle (HFILE hFile, PHFILE phFile);
ULONG DosEditName (ULONG ulLevel, PCSZ pszSource, PCSZ pszEdit,
    PBYTE pszTargetBuf, ULONG ulTargetBufLength);
ULONG DosEnumAttribute (ULONG ulRefType, CPVOID pvFile, ULONG ulEntry,
    PVOID pvBuf, ULONG ulBufLength, PULONG pulCount, ULONG ulInfoLevel);
ULONG DosFindClose (HDIR hDir);
ULONG DosFindFirst (PCSZ pszFileSpec, PHDIR phDir, ULONG flAttribute,
    PVOID pFindBuf, ULONG ulFindBufLength, PULONG pulFileNames,
    ULONG ulInfoLevel);
ULONG DosFindNext (HDIR hDir, PVOID pFindBuf, ULONG ulFindBufLength,
    PULONG pulFileNames);
ULONG DosForceDelete (PCSZ pszFileName);
ULONG DosFSAttach (PCSZ pszDevice, PCSZ pszFilesystem,
    __const__ VOID *pData, ULONG ulDataLength, ULONG ulFlag);
ULONG DosFSCtl (PVOID pData, ULONG ulDataLengthMax, PULONG pulDataLength,
    PVOID pParmList, ULONG ulParmLengthMax, PULONG pulParmLength,
    ULONG ulFunction, PCSZ pszRouteName, HFILE hFile, ULONG ulMethod);
ULONG DosMove (PCSZ pszOldName, PCSZ pszNewName);
ULONG DosOpen (PCSZ pszFileName, PHFILE phFile, PULONG pulAction,
    ULONG ulFileSize, ULONG ulAttribute, ULONG ulOpenFlags, ULONG ulOpenMode,
    PEAOP2 pEABuf);
ULONG DosProtectClose (HFILE hFile, FHLOCK fhFileHandleLockID);
ULONG DosProtectEnumAttribute (ULONG ulRefType, CPVOID pvFile,
    ULONG ulEntry, PVOID pvBuf, ULONG ulBufLength, PULONG pulCount,
    ULONG ulInfoLevel, FHLOCK fhFileHandleLockID);
ULONG DosProtectOpen (PCSZ pszFileName, PHFILE phFile, PULONG pulAction,
    ULONG ulFileSize, ULONG ulAttribute, ULONG ulOpenFlags, ULONG ulOpenMode,
    PEAOP2 pEABuf, PFHLOCK pfhFileHandleLockID);
ULONG DosProtectQueryFHState (HFILE hFile, PULONG pulMode,
    FHLOCK fhFileHandleLockID);
ULONG DosProtectQueryFileInfo (HFILE hFile, ULONG ulInfoLevel,
    PVOID pInfoBuffer, ULONG ulInfoLength, FHLOCK fhFileHandleLockID);
ULONG DosProtectRead (HFILE hFile, PVOID pBuffer, ULONG ulLength,
    PULONG pulBytesRead, FHLOCK fhFileHandleLockID);
ULONG DosProtectSetFHState (HFILE hFile, ULONG ulMode,
    FHLOCK fhFileHandleLockID);
ULONG DosProtectSetFileInfo (HFILE hFile, ULONG ulInfoLevel, PVOID pInfoBuffer,
    ULONG ulInfoLength, FHLOCK fhFileHandleLockID);
ULONG DosProtectSetFileLocks (HFILE hFile, __const__ FILELOCK *pflUnlock,
    __const__ FILELOCK *pflLock, ULONG ulTimeout, ULONG ulFlags,
    FHLOCK fhFileHandleLockID);
ULONG DosProtectSetFilePtr (HFILE hFile, LONG lOffset, ULONG ulOrigin,
    PULONG pulPos, FHLOCK fhFileHandleLockID);
ULONG DosProtectSetFileSize (HFILE hFile, ULONG ulSize,
    FHLOCK fhFileHandleLockID);
ULONG DosProtectWrite (HFILE hFile, CPVOID pBuffer, ULONG ulLength,
    PULONG pulBytesWritten, FHLOCK fhFileHandleLockID);
ULONG DosQueryCurrentDir (ULONG ulDrive, PBYTE pPath, PULONG pulPathLength);
ULONG DosQueryCurrentDisk (PULONG pulDrive, PULONG pulLogical);
ULONG DosQueryFHState (HFILE hFile, PULONG pulMode);
ULONG DosQueryFileInfo (HFILE hFile, ULONG ulInfoLevel, PVOID pInfoBuffer,
    ULONG ulInfoLength);
ULONG DosQueryFSAttach (PCSZ pszDeviceName, ULONG ulOrdinal,
    ULONG ulFSAInfoLevel, PFSQBUFFER2 pfsqb, PULONG pulBufLength);
ULONG DosQueryFSInfo (ULONG ulDrive, ULONG ulInfoLevel, PVOID pBuf,
    ULONG ulBufLength);
ULONG DosQueryHType (HFILE hFile, PULONG pulType, PULONG pulAttr);
ULONG DosQueryPathInfo (PCSZ pszPathName, ULONG ulInfoLevel,
    PVOID pInfoBuffer, ULONG ulInfoLength);
ULONG DosQueryVerify (PBOOL32 pVerify);
ULONG DosRead (HFILE hFile, PVOID pBuffer, ULONG ulLength,
    PULONG pulBytesRead);
ULONG DosResetBuffer (HFILE hf);
ULONG DosSetCurrentDir (PCSZ pszDir);
ULONG DosSetDefaultDisk (ULONG ulDrive);
ULONG DosSetFHState (HFILE hFile, ULONG ulMode);
ULONG DosSetFileInfo (HFILE hFile, ULONG ulInfoLevel, PVOID pInfoBuffer,
    ULONG ulInfoLength);
ULONG DosSetFileLocks (HFILE hFile, __const__ FILELOCK *pflUnlock,
    __const__ FILELOCK *pflLock, ULONG ulTimeout, ULONG ulFlags);
ULONG DosSetFilePtr (HFILE hFile, LONG lOffset, ULONG ulOrigin, PULONG pulPos);
ULONG DosSetFileSize (HFILE hFile, ULONG ulSize);
ULONG DosSetFSInfo (ULONG ulDrive, ULONG ulInfoLevel, PVOID pBuf,
    ULONG ulBufLength);
ULONG DosSetMaxFH (ULONG ulCount);
ULONG DosSetPathInfo (PCSZ pszPathName, ULONG ulInfoLevel, PVOID pInfoBuffer,
    ULONG ulInfoLength, ULONG ulOptions);
ULONG DosSetRelMaxFH (PLONG pulReqCount, PULONG pulCurMaxFH);
ULONG DosSetVerify (BOOL32 f32Verify);
ULONG DosShutdown (ULONG ulReserved);
ULONG DosWrite (HFILE hFile, CPVOID pBuffer, ULONG ulLength,
    PULONG pulBytesWritten);

#endif /* INCL_DOSFILEMGR || !INCL_NOCOMMON */


#if defined (INCL_DOSMISC)
ULONG DosSearchPath (ULONG ulControl, PCSZ pszPath, PCSZ pszFilename,
    PBYTE pBuf, ULONG ulBufLength);
#endif /* INCL_DOSMISC */

/* ---------------------------- DEVICE I/O -------------------------------- */

#if defined (INCL_DOSDEVICES)

#define DEVINFO_PRINTER		       0
#define DEVINFO_RS232		       1
#define DEVINFO_FLOPPY		       2
#define DEVINFO_COPROCESSOR	       3
#define DEVINFO_SUBMODEL	       4
#define DEVINFO_MODEL		       5
#define DEVINFO_ADAPTER		       6

#define INFO_COUNT_PARTITIONABLE_DISKS 1
#define INFO_GETIOCTLHANDLE	       2
#define INFO_FREEIOCTLHANDLE	       3

ULONG DosDevConfig (PVOID pInfo, ULONG ulItem);
ULONG DosDevIOCtl (HFILE hDevice, ULONG ulCategory, ULONG ulFunction,
    PVOID pParams, ULONG ulParamsLengthMax, PULONG pulParamsLength,
    PVOID pData, ULONG ulDataLengthMax, PULONG pulDataLength);
ULONG DosPhysicalDisk (ULONG ulFunction, PVOID pBuffer, ULONG ulBufferLength,
    PVOID pParams, ULONG ulParamsLength);

#endif /* INCL_DOSDEVICES */

#if defined (INCL_DOSDEVIOCTL)

#define IOCTL_ASYNC			0x0001
#define IOCTL_SCR_AND_PTRDRAW		0x0003
#define IOCTL_KEYBOARD			0x0004
#define IOCTL_PRINTER			0x0005
#define IOCTL_LIGHTPEN			0x0006
#define IOCTL_POINTINGDEVICE		0x0007
#define IOCTL_DISK			0x0008
#define IOCTL_PHYSICALDISK		0x0009
#define IOCTL_MONITOR			0x000a
#define IOCTL_GENERAL			0x000b
#define IOCTL_POWER			0x000c
#define IOCTL_OEMHLP			0x0080
#define IOCTL_TESTCFG_SYS		0x0080
#define IOCTL_CDROMDISK			0x0080
#define IOCTL_CDROMAUDIO		0x0081
#define IOCTL_TOUCH_DEVDEP		0x0081
#define IOCTL_TOUCH_DEVINDEP		0x0081

#define ASYNC_SETBAUDRATE		0x0041
#define ASYNC_SETLINECTRL		0x0042
#define ASYNC_EXTSETBAUDRATE		0x0043
#define ASYNC_TRANSMITIMM		0x0044
#define ASYNC_SETBREAKOFF		0x0045
#define ASYNC_SETMODEMCTRL		0x0046
#define ASYNC_STOPTRANSMIT		0x0047
#define ASYNC_STARTTRANSMIT		0x0048
#define ASYNC_SETBREAKON		0x004b
#define ASYNC_SETDCBINFO		0x0053
#define ASYNC_SETENHANCEDMODEPARMS	0x0054
#define ASYNC_GETBAUDRATE		0x0061
#define ASYNC_GETLINECTRL		0x0062
#define ASYNC_EXTGETBAUDRATE		0x0063
#define ASYNC_GETCOMMSTATUS		0x0064
#define ASYNC_GETLINESTATUS		0x0065
#define ASYNC_GETMODEMOUTPUT		0x0066
#define ASYNC_GETMODEMINPUT		0x0067
#define ASYNC_GETINQUECOUNT		0x0068
#define ASYNC_GETOUTQUECOUNT		0x0069
#define ASYNC_GETCOMMERROR		0x006d
#define ASYNC_GETCOMMEVENT		0x0072
#define ASYNC_GETDCBINFO		0x0073
#define ASYNC_GETENHANCEDMODEPARMS	0x0074

#define SCR_ALLOCLDT			0x0070
#define SCR_DEALLOCLDT			0x0071
#define PTR_GETPTRDRAWADDRESS		0x0072
#define VID_INITCALLVECTOR		0x0073
#define SCR_ABIOSPASSTHRU		0x0074
#define SCR_ALLOCLDTOFF			0x0075
#define SCR_ALLOCLDTBGVAL		0x0076
#define SCR_ALLOCVIDEOBUFFER		0x007e
#define SCR_GETROMFONTADDR		0x007f

#define KBD_SETTRANSTABLE		0x0050
#define KBD_SETINPUTMODE		0x0051
#define KBD_SETINTERIMFLAG		0x0052
#define KBD_SETSHIFTSTATE		0x0053
#define KBD_SETTYPAMATICRATE		0x0054
#define KBD_SETFGNDSCREENGRP		0x0055
#define KBD_SETSESMGRHOTKEY		0x0056
#define KBD_SETFOCUS			0x0057
#define KBD_SETKCB			0x0058
#define KBD_SETREADNOTIFICATION		0x0059
#define KBD_ALTERKBDLED			0x005a
#define KBD_SETNLS			0x005c
#define KBD_CREATE			0x005d
#define KBD_DESTROY			0x005e
#define KBD_GETINPUTMODE		0x0071
#define KBD_GETINTERIMFLAG		0x0072
#define KBD_GETSHIFTSTATE		0x0073
#define KBD_READCHAR			0x0074
#define KBD_PEEKCHAR			0x0075
#define KBD_GETSESMGRHOTKEY		0x0076
#define KBD_GETKEYBDTYPE		0x0077
#define KBD_GETCODEPAGEID		0x0078
#define KBD_XLATESCAN			0x0079
#define KBD_QUERYKBDHARDWAREID		0x007a
#define KBD_QUERYKBDCODEPAGESUPPORT	0x007b

#define PRT_QUERYJOBHANDLE		0x0021
#define PRT_SETFRAMECTL			0x0042
#define PRT_SETINFINITERETRY		0x0044
#define PRT_INITPRINTER			0x0046
#define PRT_ACTIVATEFONT		0x0048
#define PRT_SETPRINTJOBTITLE		0x004d
#define PRT_SETIRQTIMEOUT		0x004e
#define PRT_GETFRAMECTL			0x0062
#define PRT_GETINFINITERETRY		0x0064
#define PRT_GETPRINTERSTATUS		0x0066
#define PRT_QUERYACTIVEFONT		0x0069
#define PRT_VERIFYFONT			0x006a
#define PRT_QUERYIRQTIMEOUT		0x006e

#define MOU_ALLOWPTRDRAW		0x0050
#define MOU_UPDATEDISPLAYMODE		0x0051
#define MOU_SCREENSWITCH		0x0052
#define MOU_SETSCALEFACTORS		0x0053
#define MOU_SETEVENTMASK		0x0054
#define MOU_SETHOTKEYBUTTON		0x0055
#define MOU_REASSIGNTHRESHOLDVALUES	0x0055 /* ? */
#define MOU_SETPTRSHAPE			0x0056
#define MOU_DRAWPTR			0x0057
#define MOU_UNMARKCOLLISIONAREA		0x0057 /* ? */
#define MOU_REMOVEPTR			0x0058
#define MOU_MARKCOLLISIONAREA		0x0058 /* ? */
#define MOU_SETPTRPOS			0x0059
#define MOU_SETPROTDRAWADDRESS		0x005a
#define MOU_SETREALDRAWADDRESS		0x005b
#define MOU_SETMOUSTATUS		0x005c
#define MOU_DISPLAYMODECHANGE		0x005d
#define MOU_GETBUTTONCOUNT		0x0060
#define MOU_GETMICKEYCOUNT		0x0061
#define MOU_GETMOUSTATUS		0x0062
#define MOU_READEVENTQUE		0x0063
#define MOU_GETQUESTATUS		0x0064
#define MOU_GETEVENTMASK		0x0065
#define MOU_GETSCALEFACTORS		0x0066
#define MOU_GETPTRPOS			0x0067
#define MOU_GETPTRSHAPE			0x0068
#define MOU_GETHOTKEYBUTTON		0x0069
#define MOU_QUERYTHRESHOLDVALUES	0x0069 /* ? */
#define MOU_VER				0x006a
#define MOU_QUERYPOINTERID		0x006b /* ? */

#define DSK_LOCKDRIVE			0x0000
#define DSK_UNLOCKDRIVE			0x0001
#define DSK_REDETERMINEMEDIA		0x0002
#define DSK_SETLOGICALMAP		0x0003
#define DSK_BEGINFORMAT			0x0004
#define DSK_BLOCKREMOVABLE		0x0020
#define DSK_GETLOGICALMAP		0x0021
#define DSK_UNLOCKEJECTMEDIA		0x0040
#define DSK_SETDEVICEPARAMS		0x0043
#define DSK_WRITETRACK			0x0044
#define DSK_FORMATVERIFY		0x0045
#define DSK_DISKETTECONTROL		0x005d
#define DSK_QUERYMEDIASENSE		0x0060
#define DSK_GETDEVICEPARAMS		0x0063
#define DSK_READTRACK			0x0064
#define DSK_VERIFYTRACK			0x0065
#define DSK_GETLOCKSTATUS		0x0066

#define PDSK_LOCKPHYSDRIVE		0x0000
#define PDSK_UNLOCKPHYSDRIVE		0x0001
#define PDSK_WRITEPHYSTRACK		0x0044
#define PDSK_GETPHYSDEVICEPARAMS	0x0063
#define PDSK_READPHYSTRACK		0x0064
#define PDSK_VERIFYPHYSTRACK		0x0065

#define POWER_SENDPOWEREVENT		0x0040
#define POWER_SETPOWEREVENTRES		0x0041
#define POWER_GETPOWERSTATUS		0x0060
#define POWER_GETPOWEREVENT		0x0061
#define POWER_GETPOWERINFO		0x0062

#define OEMHLP_GETOEMADAPTIONINFO	0x0000
#define OEMHLP_GETMACHINEINFO		0x0001
#define OEMHLP_GETDISPLAYCOMBCODE	0x0002
#define OEMHLP_GETVIDEOFONTS		0x0003
#define OEMHLP_READEISACONFIGINFO	0x0004
#define OEMHLP_GETROMBIOSINFO		0x0005
#define OEMHLP_GETMISCVIDEOINFO		0x0006
#define OEMHLP_GETVIDEOADAPTER		0x0007
#define OEMHLP_GETSVGAINFO		0x0008
#define OEMHLP_GETMEMINFO		0x0009
#define OEMHLP_GETDMQSINFO		0x000a
#define OEMHLP_PCI			0x000b

#define TESTCFG_SYS_GETBIOSADAPTER	0x0040
#define TESTCFG_SYS_ISSUEINIOINSTR	0x0041
#define TESTCFG_SYS_ISSUEOUTIOINSTR	0x0042
#define TESTCFG_SYS_GETBUSARCH		0x0060
#define TESTCFG_SYS_GETALLPOSIDS	0x0061
#define TESTCFG_SYS_GETALLEISAIDS	0x0062

#define CDROMDISK_RESETDRIVE		0x0040
#define CDROMDISK_EJECTDISK		0x0044
#define CDROMDISK_LOCKUNLOCKDOOR	0x0046
#define CDROMDISK_SEEK			0x0050
#define CDROMDISK_DEVICESTATUS		0x0060
#define CDROMDISK_GETDRIVER		0x0061
#define CDROMDISK_GETSECTORSIZE		0x0063
#define CDROMDISK_GETHEADLOC		0x0070
#define CDROMDISK_READLONG		0x0072
#define CDROMDISK_GETVOLUMESIZE		0x0078
#define CDROMDISK_GETUPC		0x0079

#define CDROMAUDIO_SETCHANNELCTRL	0x0040
#define CDROMAUDIO_PLAYAUDIO		0x0050
#define CDROMAUDIO_STOPAUDIO		0x0051
#define CDROMAUDIO_RESUMEAUDIO		0x0052
#define CDROMAUDIO_GETCHANNEL		0x0060
#define CDROMAUDIO_GETAUDIODISK		0x0061
#define CDROMAUDIO_GETAUDIOTRACK	0x0062
#define CDROMAUDIO_GETSUBCHANNELQ	0x0063
#define CDROMAUDIO_GETAUDIOSTATUS	0x0065

#define TOUCH_DEVDEP_SETCALIBCONST	0x0052
#define TOUCH_DEVDEP_READDATA		0x0053
#define TOUCH_DEVDEP_SETDATAMODE	0x0054
#define TOUCH_DEVDEP_SETCLICKLOCK	0x0055
#define TOUCH_DEVDEP_SETTOUCHTHRESHOLD	0x0056
#define TOUCH_DEVDEP_SETEMULXY		0x0057
#define TOUCH_DEVDEP_SETDATAREPORTRATE	0x0058
#define TOUCH_DEVDEP_SETLOWPASSFILTER	0x0059
#define TOUCH_DEVDEP_WRITEMEMLOC	0x005a
#define TOUCH_DEVDEP_GETCALIBCONST	0x0060
#define TOUCH_DEVDEP_GETDATAMODE	0x0061
#define TOUCH_DEVDEP_GETCLICKLOCK	0x0062
#define TOUCH_DEVDEP_GETTOUCHTHRESHOLD	0x0063
#define TOUCH_DEVDEP_GETEMULXY		0x0064
#define TOUCH_DEVDEP_GETDATAREPORTRATE	0x0065
#define TOUCH_DEVDEP_GETLOWPASSFILTER	0x0066
#define TOUCH_DEVDEP_READMEMLOC		0x0067

#define TOUCH_DEVINDEP_SETCOORDSYS	0x0050
#define TOUCH_DEVINDEP_SETSELECTMECH	0x0052
#define TOUCH_DEVINDEP_SETEVENTMASK	0x0053
#define TOUCH_DEVINDEP_SETQUEUESIZE	0x0054
#define TOUCH_DEVINDEP_SETEMULSTATE	0x0055
#define TOUCH_DEVINDEP_GETCOORDSYS	0x0060
#define TOUCH_DEVINDEP_GETSELECTMECH	0x0062
#define TOUCH_DEVINDEP_GETEVENTMASK	0x0063
#define TOUCH_DEVINDEP_GETQUEUESIZE	0x0064
#define TOUCH_DEVINDEP_GETEMULSTATE	0x0065
#define TOUCH_DEVINDEP_GETREADEVENTQUEUE 0x0066

#define MON_REGISTERMONITOR		0x0040

#define DEV_FLUSHINPUT			0x0001
#define DEV_FLUSHOUTPUT			0x0002
#define DEV_SYSTEMNOTIFYPDD		0x0041
#define DEV_QUERYMONSUPPORT		0x0060

#define RX_QUE_OVERRUN			0x0001
#define RX_HARDWARE_OVERRUN		0x0002
#define PARITY_ERROR			0x0004
#define FRAMING_ERROR			0x0008

#define CHAR_RECEIVED			0x0001
#define LAST_CHAR_SENT			0x0004
#define CTS_CHANGED			0x0008
#define DSR_CHANGED			0x0010
#define DCD_CHANGED			0x0020
#define BREAK_DETECTED			0x0040
#define ERROR_OCCURRED			0x0080
#define RI_DETECTED			0x0100

#define TX_WAITING_FOR_CTS		0x0001
#define TX_WAITING_FOR_DSR		0x0002
#define TX_WAITING_FOR_DCD		0x0004
#define TX_WAITING_FOR_XON		0x0008
#define TX_WAITING_TO_SEND_XON		0x0010
#define TX_WAITING_WHILE_BREAK_ON	0x0020
#define TX_WAITING_TO_SEND_IMM		0x0040
#define RX_WAITING_FOR_DSR		0x0080

#define WRITE_REQUEST_QUEUED		0x0001
#define DATA_IN_TX_QUE			0x0002
#define HARDWARE_TRANSMITTING		0x0004
#define CHAR_READY_TO_SEND_IMM		0x0008
#define WAITING_TO_SEND_XON		0x0010
#define WAITING_TO_SEND_XOFF		0x0020

#define CTS_ON				0x10
#define DSR_ON				0x20
#define RI_ON				0x40
#define DCD_ON				0x80

#define MODE_DTR_CONTROL		0x01
#define MODE_DTR_HANDSHAKE		0x02
#define MODE_CTS_HANDSHAKE		0x08
#define MODE_DSR_HANDSHAKE		0x10
#define MODE_DCD_HANDSHAKE		0x20
#define MODE_DSR_SENSITIVITY		0x40

#define MODE_AUTO_TRANSMIT		0x01
#define MODE_AUTO_RECEIVE		0x02
#define MODE_ERROR_CHAR			0x04
#define MODE_NULL_STRIPPING		0x08
#define MODE_BREAK_CHAR			0x10
#define MODE_RTS_CONTROL		0x40
#define MODE_RTS_HANDSHAKE		0x80
#define MODE_TRANSMIT_TOGGLE		0xc0

#define MODE_NO_WRITE_TIMEOUT		0x01
#define MODE_READ_TIMEOUT		0x02
#define MODE_WAIT_READ_TIMEOUT		0x04
#define MODE_NOWAIT_READ_TIMEOUT	0x06

#define DTR_ON				0x01
#define RTS_ON				0x02

#define DTR_OFF				0xfe
#define RTS_OFF				0xfd

#define ASCII_MODE			0x00
#define BINARY_MODE			0x80

#define CONVERSION_REQUEST		0x20
#define INTERIM_CHAR			0x80

#define HOTKEY_MAX_COUNT		0x0000
#define HOTKEY_CURRENT_COUNT		0x0001

#define KBD_DATA_RECEIVED		0x0001
#define KBD_DATA_BINARY			0x8000

#define KBD_READ_WAIT			0x0000
#define KBD_READ_NOWAIT			0x8000

#define SHIFT_REPORT_MODE		0x01

#define RIGHTSHIFT			0x0001
#define LEFTSHIFT			0x0002
#define CONTROL				0x0004
#define ALT				0x0008
#define SCROLLLOCK_ON			0x0010
#define NUMLOCK_ON			0x0020
#define CAPSLOCK_ON			0x0040
#define INSERT_ON			0x0080
#define LEFTCONTROL			0x0100
#define LEFTALT				0x0200
#define RIGHTCONTROL			0x0400
#define RIGHTALT			0x0800
#define SCROLLLOCK			0x1000
#define NUMLOCK				0x2000
#define CAPSLOCK			0x4000
#define SYSREQ				0x8000

#define PRINTER_TIMEOUT			0x0001
#define PRINTER_IO_ERROR		0x0008
#define PRINTER_SELECTED		0x0010
#define PRINTER_OUT_OF_PAPER		0x0020
#define PRINTER_ACKNOWLEDGED		0x0040
#define PRINTER_NOT_BUSY		0x0080

#define MOUSE_MOTION			0x0001
#define MOUSE_MOTION_WITH_BN1_DOWN	0x0002
#define MOUSE_BN1_DOWN			0x0004
#define MOUSE_MOTION_WITH_BN2_DOWN	0x0008
#define MOUSE_BN2_DOWN			0x0010
#define MOUSE_MOTION_WITH_BN3_DOWN	0x0020
#define MOUSE_BN3_DOWN			0x0040

#define MHK_BUTTON1			0x0001
#define MHK_BUTTON2			0x0002
#define MHK_BUTTON3			0x0004

#define MOU_NOWAIT			0x0000
#define MOU_WAIT			0x0001

#define MHK_NO_HOTKEY			0x0000

#define MOUSE_QUEUEBUSY			0x0001
#define MOUSE_BLOCKREAD			0x0002
#define MOUSE_FLUSH			0x0004
#define MOUSE_UNSUPPORTED_MODE		0x0008
#define MOUSE_DISABLED			0x0100
#define MOUSE_MICKEYS			0x0200

#define BUILD_BPB_FROM_MEDIUM		0x00
#define REPLACE_BPB_FOR_DEVICE		0x01
#define REPLACE_BPB_FOR_MEDIUM		0x02

#define DEVTYPE_48TPI			0x0000
#define DEVTYPE_96TPI			0x0001
#define DEVTYPE_35			0x0002
#define DEVTYPE_8SD			0x0003
#define DEVTYPE_8DD			0x0004
#define DEVTYPE_FIXED			0x0005
#define DEVTYPE_TAPE			0x0006
#define DEVTYPE_UNKNOWN			0x0007

#define SCREENDD_GETCURRENTBANK		0x00
#define SCREENDD_SETCURRENTBANK		0x01
#define SCREENDD_SVGA_ID		0x08
#define SCREENDD_SVGA_OEM		0x09
#define SCREENDD_UPDATEMEMORY		0x0a
#define SCREENDD_GETLINEARACCESS	0x0b
#define SCREENDD_GETGLOBALACCESS	0x0c
#define SCREENDD_FREEGLOBALACCESS	0x0d
#define SCREENDD_REGISTER_RING0_CALLER	0x0e
#define SCREENDD_WAIT_ON_RING0_CALLER	0x0f
#define SCREENDD_CATEGORY		0x80
#define SCREENDD_NAME			"SCREEN$"

#define GETLINEAR_FLAG_MAPPHYSICAL	0x00000010
#define GETLINEAR_FLAG_MAPPROCESS	0x00000020
#define GETLINEAR_FLAG_MAPSHARED	0x00000400
#define GETLINEAR_FLAG_MAPATTACH	0x80000000

#define EGA_BIT				4
#define VGA_BIT				8
#define EGAVGA_BIT			(EGA_BIT|VGA_BIT)

#define READ_BANK			0
#define WRITE_BANK			1
#define MODE_TEXT			0
#define MODE_PLANAR			1
#define MODE_LINEAR			2

typedef struct _DCBINFO
{
  USHORT usWriteTimeout;
  USHORT usReadTimeout;
  BYTE	 fbCtlHndShake;
  BYTE	 fbFlowReplace;
  BYTE	 fbTimeout;
  BYTE	 bErrorReplacementChar;
  BYTE	 bBreakReplacementChar;
  BYTE	 bXONChar;
  BYTE	 bXOFFChar;
} DCBINFO;
typedef DCBINFO *PDCBINFO;

typedef struct _LINECONTROL
{
  BYTE bDataBits;
  BYTE bParity;
  BYTE bStopBits;
  BYTE fTransBreak;
} LINECONTROL;
typedef LINECONTROL *PLINECONTROL;

typedef struct _MODEMSTATUS
{
  BYTE fbModemOn;
  BYTE fbModemOff;
} MODEMSTATUS;
typedef MODEMSTATUS *PMODEMSTATUS;

typedef struct _KBDTYPE
{
  USHORT usType;
  USHORT reserved1;
  USHORT reserved2;
} KBDTYPE;
typedef KBDTYPE *PKBDTYPE;

typedef struct _RATEDELAY
{
  USHORT usDelay;
  USHORT usRate;
} RATEDELAY;
typedef RATEDELAY *PRATEDELAY;

typedef struct _CODEPAGEINFO
{
  PBYTE	 pbTransTable;
  USHORT idCodePage;
  USHORT idTable;
} CODEPAGEINFO;
typedef CODEPAGEINFO *PCODEPAGEINFO;

typedef struct _CPID
{
  USHORT idCodePage;
  USHORT Reserved;
} CPID;
typedef CPID *PCPID;

typedef struct _SHIFTSTATE
{
  USHORT fsState;
  BYTE	 fNLS;
} SHIFTSTATE;
typedef SHIFTSTATE *PSHIFTSTATE;

typedef struct _HOTKEY
{
  USHORT fsHotKey;
  UCHAR	 uchScancodeMake;
  UCHAR	 uchScancodeBreak;
  USHORT idHotKey;
} HOTKEY;
typedef HOTKEY *PHOTKEY;

typedef struct _PTRDRAWFUNCTION
{
  USHORT usReturnCode;
  PFN	 pfnDraw;
  PCH	 pchDataSeg;
} PTRDRAWFUNCTION;
typedef PTRDRAWFUNCTION *PPTRDRAWFUNCTION;

typedef struct _PTRDRAWADDRESS
{
  USHORT	  reserved;
  PTRDRAWFUNCTION ptrdfnc;
} PTRDRAWADDRESS;
typedef PTRDRAWADDRESS *PPTRDRAWADDRESS;

typedef struct _PTRDRAWDATA
{
  USHORT cb;
  USHORT usConfig;
  USHORT usFlag;
} PTRDRAWDATA;
typedef PTRDRAWDATA *PPTRDRAWDATA;

typedef struct _TRACKLAYOUT
{
  BYTE	 bCommand;
  USHORT usHead;
  USHORT usCylinder;
  USHORT usFirstSector;
  USHORT cSectors;
  struct
    {
      USHORT usSectorNumber;
      USHORT usSectorSize;
    } TrackTable[1];
} TRACKLAYOUT;
typedef TRACKLAYOUT *PTRACKLAYOUT;

typedef struct _TRACKFORMAT
{
  BYTE	 bCommand;
  USHORT usHead;
  USHORT usCylinder;
  USHORT usReserved;
  USHORT cSectors;
  struct
    {
      BYTE bCylinder;
      BYTE bHead;
      BYTE idSector;
      BYTE bBytesSector;
    } FormatTable[1];
} TRACKFORMAT;
typedef TRACKFORMAT *PTRACKFORMAT;

typedef struct _BIOSPARAMETERBLOCK
{
  USHORT usBytesPerSector;
  BYTE	 bSectorsPerCluster;
  USHORT usReservedSectors;
  BYTE	 cFATs;
  USHORT cRootEntries;
  USHORT cSectors;
  BYTE	 bMedia;
  USHORT usSectorsPerFAT;
  USHORT usSectorsPerTrack;
  USHORT cHeads;
  ULONG	 cHiddenSectors;
  ULONG	 cLargeSectors;
  BYTE	 abReserved[6];
  USHORT cCylinders;
  BYTE	 bDeviceType;
  USHORT fsDeviceAttr;
} BIOSPARAMETERBLOCK;
typedef BIOSPARAMETERBLOCK *PBIOSPARAMETERBLOCK;

typedef struct _DEVICEPARAMETERBLOCK
{
  USHORT reserved1;
  USHORT cCylinders;
  USHORT cHeads;
  USHORT cSectorsPerTrack;
  USHORT reserved2;
  USHORT reserved3;
  USHORT reserved4;
  USHORT reserved5;
} DEVICEPARAMETERBLOCK;
typedef DEVICEPARAMETERBLOCK *PDEVICEPARAMETERBLOCK;

typedef struct _MONITORPOSITION
{
  USHORT fPosition;
  USHORT index;
  ULONG	 pbInBuf;
  USHORT offOutBuf;
} MONITORPOSITION;
typedef MONITORPOSITION *PMONITORPOSITION;

typedef struct _FRAME
{
  BYTE bCharsPerLine;
  BYTE bLinesPerInch;
} FRAME;
typedef FRAME *PFRAME;

typedef struct _LDTADDRINFO
{
  PULONG pulPhysAddr;
  USHORT cb;
} LDTADDRINFO;
typedef LDTADDRINFO *PLDTADDRINFO;

typedef struct _SCREENGROUP
{
  USHORT idScreenGrp;
  USHORT fTerminate;
} SCREENGROUP;
typedef SCREENGROUP *PSCREENGROUP;

typedef struct _RXQUEUE
{
  USHORT cch;
  USHORT cb;
} RXQUEUE;
typedef RXQUEUE *PRXQUEUE;

typedef struct _GETLINIOCTLDATA
{
  ULONG PacketLength;
  ULONG PhysicalAddress;
  ULONG ApertureSize;
  PBYTE LinearAddress;
  ULONG LinearFlags;
} GETLINIOCTLDATA;
typedef GETLINIOCTLDATA *PGETLINIOCTLDATA;

typedef struct _BANKINFO
{
  ULONG	 ulBankLength;
  USHORT usBank;
  USHORT usVideoModeType;
  USHORT usReadWriteMode;
} BANKINFO;

typedef struct _GLOBALIOCTLDATA
{
  ULONG ProcessAddress;
  ULONG AddressLength;
  ULONG GlobalAddress;
} GLOBALIOCTLDATA;

typedef struct _OEMSVGAINFO
{
  USHORT AdapterType;
  USHORT ChipType;
  ULONG	 Memory;
} OEMSVGAINFO;

typedef struct _OEMINFO
{
  ULONG	 OEMLength;
  USHORT Manufacturer;
  ULONG	 ManufacturerData;
} OEMINFO;

typedef struct _GETGLOBALPACKET
{
  ULONG		  GlobalPktLength;
  GLOBALIOCTLDATA GlobalPktData[1];
} GETGLOBALPACKET;

#endif /* INCL_DOSDEVIOCTL */

/* -------------------- NATIONAL LANGUAGE SUPPORT ------------------------- */

#if defined (INCL_DOSNLS)

typedef struct _COUNTRYCODE
{
  ULONG country;
  ULONG codepage;
} COUNTRYCODE;
typedef COUNTRYCODE *PCOUNTRYCODE;

typedef struct _COUNTRYINFO
{
  ULONG	 country;
  ULONG	 codepage;
  ULONG	 fsDateFmt;
  CHAR	 szCurrency[5];
  CHAR	 szThousandsSeparator[2];
  CHAR	 szDecimal[2];
  CHAR	 szDateSeparator[2];
  CHAR	 szTimeSeparator[2];
  UCHAR	 fsCurrencyFmt;
  UCHAR	 cDecimalPlace;
  UCHAR	 fsTimeFmt;
  USHORT abReserved1[2];
  CHAR	 szDataSeparator[2];
  USHORT abReserved2[5];
} COUNTRYINFO;
typedef COUNTRYINFO *PCOUNTRYINFO;


ULONG DosMapCase (ULONG ulLength, __const__ COUNTRYCODE *pCountryCode,
    PCHAR pchString);
ULONG DosQueryCollate (ULONG ulLength, __const__ COUNTRYCODE *pCountryCode,
    PCHAR pchBuffer, PULONG pulDataLength);
ULONG DosQueryCp (ULONG ulLength, PULONG pCodePageList, PULONG pDataLength);
ULONG DosQueryCtryInfo (ULONG ulLength, PCOUNTRYCODE pCountryCode,
    PCOUNTRYINFO pCountryInfo, PULONG pulDataLength);
ULONG DosQueryDBCSEnv (ULONG ulLength, PCOUNTRYCODE pCountryCode, PCHAR pBuf);
ULONG DosSetProcessCp (ULONG ulCodePage);

#endif /* INCL_DOSNLS */

/* -------------------------- DYNAMIC LINKING ----------------------------- */

#if defined (INCL_DOSMODULEMGR)

#define PT_16BIT		0
#define PT_32BIT		1


ULONG DosFreeModule (HMODULE hmod);
ULONG DosLoadModule (PSZ pszObject, ULONG uObjectLen, PCSZ pszModule,
    PHMODULE phmod);
ULONG DosQueryModuleHandle (PCSZ pszModname, PHMODULE phmod);
ULONG DosQueryModuleName (HMODULE hmod, ULONG ulNameLength, PCHAR pNameBuf);
ULONG DosQueryProcAddr (HMODULE hmod, ULONG ulOrdinal, PCSZ pszProcName,
    PPFN pProcAddr);
ULONG DosQueryProcType (HMODULE hmod, ULONG ulOrdinal, PCSZ pszProcName,
     PULONG pulProcType);

#endif /* INCL_DOSMODULEMGR */

/* ----------------------------- RESOURCES -------------------------------- */

#if defined (INCL_DOSRESOURCES) || !defined (INCL_NOCOMMON)

#define RT_POINTER		1
#define RT_BITMAP		2
#define RT_MENU			3
#define RT_DIALOG		4
#define RT_STRING		5
#define RT_FONTDIR		6
#define RT_FONT			7
#define RT_ACCELTABLE		8
#define RT_RCDATA		9
#define RT_MESSAGE		10
#define RT_DLGINCLUDE		11
#define RT_VKEYTBL		12
#define RT_KEYTBL		13
#define RT_CHARTBL		14
#define RT_DISPLAYINFO		15
#define RT_FKASHORT		16
#define RT_FKALONG		17
#define RT_HELPTABLE		18
#define RT_HELPSUBTABLE		19
#define RT_FDDIR		20
#define RT_FD			21
#define RT_MAX			22

#define RF_ORDINALID		0x80000000L


ULONG DosFreeResource (PVOID pResAddr);
ULONG DosGetResource (HMODULE hmod, ULONG ulTypeID, ULONG ulNameID,
    PPVOID pOffset);
ULONG DosQueryResourceSize (HMODULE hmod, ULONG ulTypeID, ULONG ulNameID,
    PULONG pulSize);

#endif /* INCL_DOSRESOURCES || !INCL_NOCOMMON */

/* -------------------------------- TASKS --------------------------------- */

#if defined (INCL_DOSPROCESS) || !defined (INCL_NOCOMMON)

#define EXIT_THREAD		0
#define EXIT_PROCESS		1

ULONG DosBeep (ULONG ulFrequency, ULONG ulDuration);
VOID DosExit (ULONG ulAction, ULONG ulResult) __attribute__ ((__noreturn__));

#endif /* INCL_DOSPROCESS || !defined (INCL_NOCOMMON) */


#if defined (INCL_DOSPROCESS)

#define CREATE_READY		0
#define CREATE_SUSPENDED	1

#define STACK_SPARSE		0
#define STACK_COMMITTED		2

#define DCWA_PROCESS		0
#define DCWA_PROCESSTREE	1

#define DCWW_WAIT		0
#define DCWW_NOWAIT		1

#define DKP_PROCESSTREE		0
#define DKP_PROCESS		1

#define EXEC_SYNC		0
#define EXEC_ASYNC		1
#define EXEC_ASYNCRESULT	2
#define EXEC_TRACE		3
#define EXEC_BACKGROUND		4
#define EXEC_LOAD		5
#define EXEC_ASYNCRESULTDB	6

#define EXLST_ADD		1
#define EXLST_REMOVE		2
#define EXLST_EXIT		3

#define PRTYC_NOCHANGE		0
#define PRTYC_IDLETIME		1
#define PRTYC_REGULAR		2
#define PRTYC_TIMECRITICAL	3
#define PRTYC_FOREGROUNDSERVER	4

#define PRTYD_MINIMUM		(-31)
#define PRTYD_MAXIMUM		31

#define PRTYS_PROCESS		0
#define PRTYS_PROCESSTREE	1
#define PRTYS_THREAD		2

#define TC_EXIT			0
#define TC_HARDERROR		1
#define TC_TRAP			2
#define TC_KILLPROCESS		3
#define TC_EXCEPTION		4


typedef struct _RESULTCODES
{
  ULONG codeTerminate;
  ULONG codeResult;
} RESULTCODES;
typedef RESULTCODES *PRESULTCODES;

typedef struct tib2_s
{
  ULONG	 tib2_ultid;
  ULONG	 tib2_ulpri;
  ULONG	 tib2_version;
  USHORT tib2_usMCCount;
  USHORT tib2_fMCForceFlag;
} TIB2;
typedef TIB2 *PTIB2;

typedef struct tib_s
{
  PVOID tib_pexchain;
  PVOID tib_pstack;
  PVOID tib_pstacklimit;
  PTIB2 tib_ptib2;
  ULONG tib_version;
  ULONG tib_ordinal;
} TIB;
typedef TIB *PTIB;

typedef struct pib_s
{
  ULONG pib_ulpid;
  ULONG pib_ulppid;
  ULONG pib_hmte;
  PCHAR pib_pchcmd;
  PCHAR pib_pchenv;
  ULONG pib_flstatus;
  ULONG pib_ultype;
} PIB;
typedef PIB *PPIB;

typedef VOID (*PFNTHREAD)(ULONG ulThreadArg);
typedef VOID (*PFNEXITLIST)(ULONG ulArg);


ULONG DosAllocThreadLocalMemory (ULONG cb, PULONG *p);
ULONG DosCreateThread (PTID ptidThreadID, PFNTHREAD pfnThreadAddr,
    ULONG ulThreadArg, ULONG ulFlags, ULONG ulStackSize);
ULONG DosEnterCritSec (VOID);
ULONG DosExecPgm (PCHAR pObjname, LONG lObjnameLength, ULONG ulFlagS,
    PCSZ pszArg, PCSZ pszEnv, PRESULTCODES pReturnCodes, PCSZ pszName);
ULONG DosExitCritSec (VOID);
ULONG DosExitList (ULONG ulOrder, PFNEXITLIST pfn);
ULONG DosFreeThreadLocalMemory (ULONG *p);
ULONG DosGetInfoBlocks (PTIB *ptib, PPIB *ppib);
ULONG DosKillProcess (ULONG ulAction, PID pid);
ULONG DosKillThread (TID tid);
ULONG DosResumeThread (TID tid);
ULONG DosSetPriority (ULONG ulScope, ULONG ulClass, LONG lDelta, ULONG ulID);

#define STDCALL     __attribute__ ((stdcall))
//#define CDECL       __attribute__ ((cdecl))
//#define CALLBACK    WINAPI
//#define PASCAL      WINAPI

#define WINAPI      STDCALL
//#define APIENTRY    STDCALL

//ULONG WINAPI DosSleep (ULONG ulInterval);
ULONG DosSleep (ULONG ulInterval);

ULONG DosSuspendThread (TID tid);
ULONG DosVerifyPidTid (PID pid, TID tid);
ULONG DosWaitChild (ULONG ulAction, ULONG ulWait, PRESULTCODES pReturnCodes,
    PPID ppidOut, PID pidIn);
ULONG DosWaitThread (PTID ptid, ULONG ulWait);

#endif /* INCL_DOSPROCESS */

/* ------------------------------ SESSIONS -------------------------------- */

#if defined (INCL_DOSSESMGR) || defined (INCL_DOSFILEMGR)

#define FAPPTYP_NOTSPEC		0x0000
#define FAPPTYP_NOTWINDOWCOMPAT 0x0001
#define FAPPTYP_WINDOWCOMPAT	0x0002
#define FAPPTYP_WINDOWAPI	0x0003
#define FAPPTYP_BOUND		0x0008
#define FAPPTYP_DLL		0x0010
#define FAPPTYP_DOS		0x0020
#define FAPPTYP_PHYSDRV		0x0040
#define FAPPTYP_VIRTDRV		0x0080
#define FAPPTYP_PROTDLL		0x0100
#define FAPPTYP_WINDOWSREAL	0x0200
#define FAPPTYP_WINDOWSPROT	0x0400
#define FAPPTYP_WINDOWSPROT31	0x1000
#define FAPPTYP_32BIT		0x4000
#define FAPPTYP_EXETYPE		0x0003
#define FAPPTYP_RESERVED	(~(FAPPTYP_WINDOWAPI | FAPPTYP_BOUND | \
				   FAPPTYP_DLL | FAPPTYP_DOS | \
				   FAPPTYP_PHYSDRV | FAPPTYP_VIRTDRV | \
				   FAPPTYP_PROTDLL | FAPPTYP_32BIT))

#endif /* INCL_DOSSESMGR || INCL_DOSFILEMGR */

#if defined (INCL_DOSSESMGR)

#define SET_SESSION_UNCHANGED	       0
#define SET_SESSION_SELECTABLE	       1
#define SET_SESSION_NON_SELECTABLE     2
#define SET_SESSION_BOND	       1
#define SET_SESSION_NO_BOND	       2

#define SSF_RELATED_INDEPENDENT 0
#define SSF_RELATED_CHILD	1

#define SSF_FGBG_FORE		0
#define SSF_FGBG_BACK		1

#define SSF_TRACEOPT_NONE	0
#define SSF_TRACEOPT_TRACE	1
#define SSF_TRACEOPT_TRACEALL	2

#define SSF_INHERTOPT_SHELL	0
#define SSF_INHERTOPT_PARENT	1

#define SSF_TYPE_DEFAULT	0
#define SSF_TYPE_FULLSCREEN	1
#define SSF_TYPE_WINDOWABLEVIO	2
#define SSF_TYPE_PM		3
#define SSF_TYPE_VDM		4
#define SSF_TYPE_GROUP		5
#define SSF_TYPE_DLL		6
#define SSF_TYPE_WINDOWEDVDM	7
#define SSF_TYPE_PDD		8
#define SSF_TYPE_VDD		9

#define SSF_CONTROL_VISIBLE	0x0000
#define SSF_CONTROL_INVISIBLE	0x0001
#define SSF_CONTROL_MAXIMIZE	0x0002
#define SSF_CONTROL_MINIMIZE	0x0004
#define SSF_CONTROL_NOAUTOCLOSE 0x0008
#define SSF_CONTROL_SETPOS	0x8000

#define STOP_SESSION_SPECIFIED	       0
#define STOP_SESSION_ALL	       1

typedef struct _STARTDATA
{
  USHORT Length;
  USHORT Related;
  USHORT FgBg;
  USHORT TraceOpt;
  PSZ	 PgmTitle;
  PSZ	 PgmName;
  PBYTE	 PgmInputs;
  PBYTE	 TermQ;
  PBYTE	 Environment;
  USHORT InheritOpt;
  USHORT SessionType;
  PSZ	 IconFile;
  ULONG	 PgmHandle;
  USHORT PgmControl;
  USHORT InitXPos;
  USHORT InitYPos;
  USHORT InitXSize;
  USHORT InitYSize;
  USHORT Reserved;
  PSZ	 ObjectBuffer;
  ULONG	 ObjectBuffLen;
} STARTDATA;
typedef STARTDATA *PSTARTDATA;

typedef struct _STATUSDATA
{
  USHORT Length;
  USHORT SelectInd;
  USHORT BondInd;
} STATUSDATA;
typedef STATUSDATA *PSTATUSDATA;

ULONG DosQueryAppType (PCSZ pszName, PULONG pulFlags);
ULONG DosSelectSession (ULONG ulIDSession);
ULONG DosSetSession (ULONG ulIDSession, PSTATUSDATA psd);
ULONG DosStartSession (PSTARTDATA psd, PULONG pulIDSession, PPID ppid);
ULONG DosStopSession (ULONG ulScope, ULONG ulIDSession);

#endif /* INCL_DOSSESMGR */

/* ----------------------------- SEMAPHORES ------------------------------- */

#if defined (INCL_DOSSEMAPHORES) || !defined (INCL_NOCOMMON)

#define DC_SEM_SHARED		0x01
#define DCMW_WAIT_ANY		0x02
#define DCMW_WAIT_ALL		0x04

#define SEM_INDEFINITE_WAIT	((ULONG)-1)
#define SEM_IMMEDIATE_RETURN	0

typedef ULONG HEV;
typedef HEV *PHEV;

typedef struct _PSEMRECORD	/* Note 1 */
{
  HSEM	hsemCur;
  ULONG ulUser;
} SEMRECORD;
typedef SEMRECORD *PSEMRECORD;

#endif /* INCL_DOSSEMAPHORES || !INCL_NOCOMMON */

#if defined (INCL_DOSSEMAPHORES)

ULONG DosCloseEventSem (HEV hev);
ULONG DosCreateEventSem (PCSZ pszName, PHEV phev, ULONG ulAttr, BOOL32 fState);
ULONG DosOpenEventSem (PCSZ pszName, PHEV phev);
ULONG DosPostEventSem (HEV hev);
ULONG DosQueryEventSem (HEV hev, PULONG pulCount);
ULONG DosResetEventSem (HEV hev, PULONG pulCount);
ULONG DosWaitEventSem (HEV hev, ULONG ulTimeout);

ULONG DosCloseMutexSem (HMTX hmtx);
ULONG DosCreateMutexSem (PCSZ pszName, PHMTX phmtx, ULONG ulAttr,
    BOOL32 fState);
ULONG DosOpenMutexSem (PCSZ pszName, PHMTX phmtx);
ULONG DosQueryMutexSem (HMTX hmtx, PPID ppid, PTID ptid, PULONG pulCount);
ULONG DosReleaseMutexSem (HMTX hmtx);
ULONG DosRequestMutexSem (HMTX hmtx, ULONG ulTimeout);

ULONG DosAddMuxWaitSem (HMUX hmux, PSEMRECORD pSemRec);
ULONG DosCloseMuxWaitSem (HMUX hmux);
ULONG DosCreateMuxWaitSem (PCSZ pszName, PHMUX phmux, ULONG ulcSemRec,
    PSEMRECORD pSemRec, ULONG ulAttr);
ULONG DosDeleteMuxWaitSem (HMUX hmux, HSEM hSem);
ULONG DosOpenMuxWaitSem (PCSZ pszName, PHMUX phmux);
ULONG DosQueryMuxWaitSem (HMUX hmux, PULONG pulcSemRec, PSEMRECORD pSemRec,
    PULONG pulAttr);
ULONG DosWaitMuxWaitSem (HMUX hmux, ULONG ulTimeout, PULONG pulUser);

#endif /* INCL_DOSSEMAPHORES */

/* ---------------------------- NAMED PIPES ------------------------------- */

#define NP_INDEFINITE_WAIT	((ULONG)-1)
#define NP_DEFAULT_WAIT		0

#define NP_STATE_DISCONNECTED	1
#define NP_STATE_LISTENING	2
#define NP_STATE_CONNECTED	3
#define NP_STATE_CLOSING	4

#define NP_ACCESS_INBOUND	0x0000
#define NP_ACCESS_OUTBOUND	0x0001
#define NP_ACCESS_DUPLEX	0x0002
#define NP_INHERIT		0x0000
#define NP_NOINHERIT		0x0080
#define NP_WRITEBEHIND		0x0000
#define NP_NOWRITEBEHIND	0x4000

#define NP_READMODE_BYTE	0x0000
#define NP_READMODE_MESSAGE	0x0100
#define NP_TYPE_BYTE		0x0000
#define NP_TYPE_MESSAGE		0x0400
#define NP_END_CLIENT		0x0000
#define NP_END_SERVER		0x4000
#define NP_WAIT			0x0000
#define NP_NOWAIT		0x8000
#define NP_UNLIMITED_INSTANCES	0x00ff

#define NP_NBLK			NO_WAIT
#define NP_SERVER		NP_END_SERVER
#define NP_WMESG		NP_TYPE_MESSAGE
#define NP_RMESG		NP_READMODE_MESSAGE
#define NP_ICOUNT		0x00ff

#define NPSS_EOI		0
#define NPSS_RDATA		1
#define NPSS_WSPACE		2
#define NPSS_CLOSE		3

#define NPSS_WAIT		0x0001

typedef struct _AVAILDATA
{
  USHORT cbpipe;
  USHORT cbmessage;
} AVAILDATA;
typedef AVAILDATA *PAVAILDATA;

typedef struct _PIPEINFO
{
  USHORT cbOut;
  USHORT cbIn;
  BYTE	 cbMaxInst;
  BYTE	 cbCurInst;
  BYTE	 cbName;
  CHAR	 szName[1];
} PIPEINFO;
typedef PIPEINFO *PPIPEINFO;

typedef struct _PIPESEMSTATE
{
  BYTE	 fStatus;
  BYTE	 fFlag;
  USHORT usKey;
  USHORT usAvail;
} PIPESEMSTATE;
typedef PIPESEMSTATE *PPIPESEMSTATE;

ULONG DosCallNPipe (PCSZ pszName, PVOID pInbuf, ULONG ulInbufLength,
    PVOID pOutbuf, ULONG ulOutbufSize, PULONG pulActualLength,
    ULONG ulTimeout);
ULONG DosConnectNPipe (HPIPE hpipe);
ULONG DosCreateNPipe (PCSZ pszName, PHPIPE phpipe, ULONG ulOpenMode,
    ULONG ulPipeMode, ULONG ulInbufLength, ULONG ulOutbufLength,
    ULONG ulTimeout);
ULONG DosDisConnectNPipe (HPIPE hpipe);
ULONG DosPeekNPipe (HPIPE hpipe, PVOID pBuf, ULONG ulBufLength,
    PULONG pulActualLength, PAVAILDATA pAvail, PULONG pulState);
ULONG DosQueryNPHState (HPIPE hpipe, PULONG pulState);
ULONG DosQueryNPipeInfo (HPIPE hpipe, ULONG ulInfoLevel, PVOID pBuf,
    ULONG ulBufLength);
ULONG DosQueryNPipeSemState (HSEM hsem, PPIPESEMSTATE pState,
    ULONG ulBufLength);
ULONG DosRawReadNPipe (PCSZ pszName, ULONG ulCount, PULONG pulLength,
    PVOID pBuf);
ULONG DosRawWriteNPipe (PCSZ pszName, ULONG ulCount);
ULONG DosSetNPHState (HPIPE hpipe, ULONG ulState);
ULONG DosSetNPipeSem (HPIPE hpipe, HSEM hsem, ULONG ulKey);
ULONG DosTransactNPipe (HPIPE hpipe, PVOID pOutbuf, ULONG ulOutbufLength,
    PVOID pInbuf, ULONG ulInbufLength, PULONG pulBytesRead);
ULONG DosWaitNPipe (PCSZ pszName, ULONG ulTimeout);

/* ------------------------------- QUEUES --------------------------------- */

#if defined (INCL_DOSQUEUES)

#define QUE_FIFO			0x0000
#define QUE_LIFO			0x0001
#define QUE_PRIORITY			0x0002
#define QUE_NOCONVERT_ADDRESS		0x0000
#define QUE_CONVERT_ADDRESS		0x0004

typedef struct _REQUESTDATA
{
  PID	pid;
  ULONG ulData;
} REQUESTDATA;
typedef REQUESTDATA *PREQUESTDATA;

ULONG DosCreatePipe (PHFILE phfReadHandle, PHFILE phfWriteHandle,
    ULONG ulPipeSize);

ULONG DosCloseQueue (HQUEUE hq);
ULONG DosCreateQueue (PHQUEUE phq, ULONG ulPriority, PCSZ pszName);
ULONG DosOpenQueue (PPID ppid, PHQUEUE phq, PCSZ pszName);
ULONG DosPeekQueue (HQUEUE hq, PREQUESTDATA pRequest, PULONG pulDataLength,
    PPVOID pDataAddress, PULONG pulElement, BOOL32 fNowait, PBYTE pPriority,
    HEV hsem);
ULONG DosPurgeQueue (HQUEUE hq);
ULONG DosQueryQueue (HQUEUE hq, PULONG pulCount);
ULONG DosReadQueue (HQUEUE hq, PREQUESTDATA pRequest, PULONG pulDataLength,
    PPVOID pDataAddress, ULONG pulElement, BOOL32 fNowait, PBYTE pPriority,
    HEV hsem);
ULONG DosWriteQueue (HQUEUE hq, ULONG ulRequest, ULONG ulDataLength,
    PVOID pData, ULONG ulPriority);

#endif /* INCL_DOSQUEUES */

/* --------------------------- EXCEPTIONS --------------------------------- */

#if defined (INCL_DOSEXCEPTIONS)

#define CONTEXT_CONTROL			0x0001
#define CONTEXT_INTEGER			0x0002
#define CONTEXT_SEGMENTS		0x0004
#define CONTEXT_FLOATING_POINT		0x0008
#define CONTEXT_FULL			(CONTEXT_CONTROL | CONTEXT_INTEGER | \
				  CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT)

#define EH_NONCONTINUABLE		0x0001
#define EH_UNWINDING			0x0002
#define EH_EXIT_UNWIND			0x0004
#define EH_STACK_INVALID		0x0008
#define EH_NESTED_CALL			0x0010

#define SIG_UNSETFOCUS			0
#define SIG_SETFOCUS			1

#define UNWIND_ALL			0

#define XCPT_CONTINUE_SEARCH		0x00000000
#define XCPT_CONTINUE_EXECUTION		0xffffffff
#define XCPT_CONTINUE_STOP		0x00716668

#define XCPT_SIGNAL_INTR		1
#define XCPT_SIGNAL_KILLPROC		3
#define XCPT_SIGNAL_BREAK		4

#define XCPT_FATAL_EXCEPTION		0xc0000000
#define XCPT_SEVERITY_CODE		0xc0000000
#define XCPT_CUSTOMER_CODE		0x20000000
#define XCPT_FACILITY_CODE		0x1fff0000
#define XCPT_EXCEPTION_CODE		0x0000ffff

#define XCPT_UNKNOWN_ACCESS		0x00000000
#define XCPT_READ_ACCESS		0x00000001
#define XCPT_WRITE_ACCESS		0x00000002
#define XCPT_EXECUTE_ACCESS		0x00000004
#define XCPT_SPACE_ACCESS		0x00000008
#define XCPT_LIMIT_ACCESS		0x00000010
#define XCPT_DATA_UNKNOWN		0xffffffff

#define XCPT_GUARD_PAGE_VIOLATION	0x80000001
#define XCPT_UNABLE_TO_GROW_STACK	0x80010001
#define XCPT_ACCESS_VIOLATION		0xc0000005
#define XCPT_IN_PAGE_ERROR		0xc0000006
#define XCPT_ILLEGAL_INSTRUCTION	0xc000001c
#define XCPT_INVALID_LOCK_SEQUENCE	0xc000001d
#define XCPT_NONCONTINUABLE_EXCEPTION	0xc0000024
#define XCPT_INVALID_DISPOSITION	0xc0000025
#define XCPT_UNWIND			0xc0000026
#define XCPT_BAD_STACK			0xc0000027
#define XCPT_INVALID_UNWIND_TARGET	0xc0000028
#define XCPT_ARRAY_BOUNDS_EXCEEDED	0xc0000093
#define XCPT_FLOAT_DENORMAL_OPERAND	0xc0000094
#define XCPT_FLOAT_DIVIDE_BY_ZERO	0xc0000095
#define XCPT_FLOAT_INEXACT_RESULT	0xc0000096
#define XCPT_FLOAT_INVALID_OPERATION	0xc0000097
#define XCPT_FLOAT_OVERFLOW		0xc0000098
#define XCPT_FLOAT_STACK_CHECK		0xc0000099
#define XCPT_FLOAT_UNDERFLOW		0xc000009a
#define XCPT_INTEGER_DIVIDE_BY_ZERO	0xc000009b
#define XCPT_INTEGER_OVERFLOW		0xc000009c
#define XCPT_PRIVILEGED_INSTRUCTION	0xc000009d
#define XCPT_DATATYPE_MISALIGNMENT	0xc000009e
#define XCPT_BREAKPOINT			0xc000009f
#define XCPT_SINGLE_STEP		0xc00000a0
#define XCPT_PROCESS_TERMINATE		0xc0010001
#define XCPT_ASYNC_PROCESS_TERMINATE	0xc0010002
#define XCPT_SIGNAL			0xc0010003

typedef struct _fpreg		/* Note 1 */
{
  ULONG	 losig;
  ULONG	 hisig;
  USHORT signexp;
} FPREG;
typedef FPREG *PFPREG;

typedef struct _CONTEXT		/* Note 1 */
{
  ULONG ContextFlags;
  ULONG ctx_env[7];
  FPREG ctx_stack[8];
  ULONG ctx_SegGs;
  ULONG ctx_SegFs;
  ULONG ctx_SegEs;
  ULONG ctx_SegDs;
  ULONG ctx_RegEdi;
  ULONG ctx_RegEsi;
  ULONG ctx_RegEax;
  ULONG ctx_RegEbx;
  ULONG ctx_RegEcx;
  ULONG ctx_RegEdx;
  ULONG ctx_RegEbp;
  ULONG ctx_RegEip;
  ULONG ctx_SegCs;
  ULONG ctx_EFlags;
  ULONG ctx_RegEsp;
  ULONG ctx_SegSs;
} CONTEXTRECORD;
typedef CONTEXTRECORD *PCONTEXTRECORD;

#define EXCEPTION_MAXIMUM_PARAMETERS 4

typedef struct _EXCEPTIONREPORTRECORD
{
  ULONG				  ExceptionNum;
  ULONG				  fHandlerFlags;
  struct _EXCEPTIONREPORTRECORD * NestedExceptionReportRecord;
  PVOID				  ExceptionAddress;
  ULONG				  cParameters;
  ULONG				  ExceptionInfo[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTIONREPORTRECORD;
typedef EXCEPTIONREPORTRECORD *PEXCEPTIONREPORTRECORD;

struct _EXCEPTIONREGISTRATIONRECORD;

typedef ULONG _ERR (PEXCEPTIONREPORTRECORD pReport,
		    struct _EXCEPTIONREGISTRATIONRECORD *pRegistration,
		    PCONTEXTRECORD pContext, PVOID pWhatever);
typedef _ERR *ERR;

typedef struct _EXCEPTIONREGISTRATIONRECORD
{
  struct _EXCEPTIONREGISTRATIONRECORD * __volatile__ prev_structure;
  ERR __volatile__				     ExceptionHandler;
} EXCEPTIONREGISTRATIONRECORD;
typedef EXCEPTIONREGISTRATIONRECORD *PEXCEPTIONREGISTRATIONRECORD;

#define END_OF_CHAIN		((PEXCEPTIONREGISTRATIONRECORD)(-1))

ULONG DosAcknowledgeSignalException (ULONG ulSignal);
ULONG DosEnterMustComplete (PULONG pulNesting);
ULONG DosExitMustComplete (PULONG pulNesting);
ULONG DosQueryThreadContext (TID tid, ULONG ulLevel, PCONTEXTRECORD pContext);
ULONG DosRaiseException (PEXCEPTIONREPORTRECORD pXRepRec);
ULONG DosSendSignalException (PID pid, ULONG ulSignal);
ULONG DosSetExceptionHandler (PEXCEPTIONREGISTRATIONRECORD pXRegRec);
ULONG DosSetSignalExceptionFocus (BOOL32 flag, PULONG pulTimes);
ULONG DosUnsetExceptionHandler (PEXCEPTIONREGISTRATIONRECORD pXRegRec);
ULONG DosUnwindException (PEXCEPTIONREGISTRATIONRECORD pXRegRec,
    PVOID pJumpThere, PEXCEPTIONREPORTRECORD pXRepRec);

#endif /* INCL_DOSEXCEPTIONS */

/* --------------------------- INFORMATION -------------------------------- */

#if defined (INCL_DOSMISC)

#define QSV_MAX_PATH_LENGTH		1
#define QSV_MAX_TEXT_SESSIONS		2
#define QSV_MAX_PM_SESSIONS		3
#define QSV_MAX_VDM_SESSIONS		4
#define QSV_BOOT_DRIVE			5
#define QSV_DYN_PRI_VARIATION		6
#define QSV_MAX_WAIT			7
#define QSV_MIN_SLICE			8
#define QSV_MAX_SLICE			9
#define QSV_PAGE_SIZE			10
#define QSV_VERSION_MAJOR		11
#define QSV_VERSION_MINOR		12
#define QSV_VERSION_REVISION		13
#define QSV_MS_COUNT			14
#define QSV_TIME_LOW			15
#define QSV_TIME_HIGH			16
#define QSV_TOTPHYSMEM			17
#define QSV_TOTRESMEM			18
#define QSV_TOTAVAILMEM			19
#define QSV_MAXPRMEM			20
#define QSV_MAXSHMEM			21
#define QSV_TIMER_INTERVAL		22
#define QSV_MAX_COMP_LENGTH		23
#define QSV_FOREGROUND_FS_SESSION	24
#define QSV_FOREGROUND_PROCESS		25
#define QSV_MAX				QSV_FOREGROUND_PROCESS

#define SIS_MMIOADDR			0
#define SIS_MEC_TABLE			1
#define SIS_SYS_LOG			2

ULONG DosQuerySysInfo (ULONG ulStart, ULONG ulLast, PVOID pBuf,
    ULONG ulBufLength);
ULONG DosScanEnv (PCSZ pszName, PSZ *pszValue);
ULONG DosQueryRASInfo (ULONG Index, PPVOID Addr);

#endif /* INCL_DOSMISC */

/* ---------------------------- TIMERS ------------------------------------ */

#if defined (INCL_DOSDATETIME) || !defined (INCL_NOCOMMON)

typedef struct _DATETIME
{
  UCHAR	 hours;
  UCHAR	 minutes;
  UCHAR	 seconds;
  UCHAR	 hundredths;
  UCHAR	 day;
  UCHAR	 month;
  USHORT year;
  SHORT	 timezone;
  UCHAR	 weekday;
} DATETIME;
typedef DATETIME *PDATETIME;

ULONG DosGetDateTime (PDATETIME pdt);
ULONG DosSetDateTime (__const__ DATETIME *pdt);

#endif /* INCL_DOSDATETIME || !INCL_NOCOMMON */


#if defined (INCL_DOSDATETIME)

typedef LHANDLE HTIMER;
typedef HTIMER	*PHTIMER;

ULONG DosAsyncTimer (ULONG ulMilliSec, HSEM hsem, PHTIMER phtimer);
ULONG DosStartTimer (ULONG ulMilliSec, HSEM hsem, PHTIMER phtimer);
ULONG DosStopTimer (HTIMER htimer);

#endif /* INCL_DOSDATETIME */

#if defined (INCL_DOSPROFILE)

typedef struct _QWORD
{
  ULONG ulLo;
  ULONG ulHi;
} QWORD;
typedef QWORD *PQWORD;

ULONG DosTmrQueryFreq (PULONG pulTmrFreq);
ULONG DosTmrQueryTime (PQWORD pqwTmrTime);

#endif /* INCL_DOSPROFILE */

/* ---------------------- VIRTUAL DOS MACHINES----------------------------- */

typedef USHORT SGID;

#if defined (INCL_DOSMVDM)

typedef LHANDLE HVDD;
typedef HVDD *PHVDD;

ULONG DosCloseVDD (HVDD hvdd);
ULONG DosOpenVDD (PCSZ pszVDD, PHVDD phvdd);
ULONG DosQueryDOSProperty (SGID sgidSesssionID, PCSZ pszName,
    ULONG ulBufferLength, PSZ pBuffer);
ULONG DosRequestVDD (HVDD hvdd, SGID sgidSessionID, ULONG ulCommand,
    ULONG ulInputBufferLength, PVOID pInputBuffer,
    ULONG ulOutputBufferLength, PVOID pOutputBuffer);
ULONG DosSetDOSProperty (SGID sgidSessionID, PCSZ pszName,
    ULONG ulBufferLength, PCSZ pBuffer);

#endif /* INCL_DOSMVDM */

/* --------------------------- DEBUGGING ---------------------------------- */

#if defined (INCL_DOSPROCESS)

#define DBG_C_Null			0
#define DBG_C_ReadMem			1
#define DBG_C_ReadMem_I			1
#define DBG_C_ReadMem_D			2
#define DBG_C_ReadReg			3
#define DBG_C_WriteMem			4
#define DBG_C_WriteMem_I		4
#define DBG_C_WriteMem_D		5
#define DBG_C_WriteReg			6
#define DBG_C_Go			7
#define DBG_C_Term			8
#define DBG_C_SStep			9
#define DBG_C_Stop			10
#define DBG_C_Freeze			11
#define DBG_C_Resume			12
#define DBG_C_NumToAddr			13
#define DBG_C_ReadCoRegs		14
#define DBG_C_WriteCoRegs		15
#define DBG_C_ThrdStat			17
#define DBG_C_MapROAlias		18
#define DBG_C_MapRWAlias		19
#define DBG_C_UnMapAlias		20
#define DBG_C_Connect			21
#define DBG_C_ReadMemBuf		22
#define DBG_C_WriteMemBuf		23
#define DBG_C_SetWatch			24
#define DBG_C_ClearWatch		25
#define DBG_C_RangeStep			26
#define DBG_C_Continue			27
#define DBG_C_AddrToObject		28
#define DBG_C_XchngOpcode		29
#define DBG_C_LinToSel			30
#define DBG_C_SelToLin			31

#define DBG_N_Success			0
#define DBG_N_Error			(-1)
#define DBG_N_ProcTerm			(-6)
#define DBG_N_Exception			(-7)
#define DBG_N_ModuleLoad		(-8)
#define DBG_N_CoError			(-9)
#define DBG_N_ThreadTerm		(-10)
#define DBG_N_AsyncStop			(-11)
#define DBG_N_NewProc			(-12)
#define DBG_N_AliasFree			(-13)
#define DBG_N_Watchpoint		(-14)
#define DBG_N_ThreadCreate		(-15)
#define DBG_N_ModuleFree		(-16)
#define DBG_N_RangeStep			(-17)

#define DBG_D_Thawed			0
#define DBG_D_Frozen			1

#define DBG_T_Runnable			0
#define DBG_T_Suspended			1
#define DBG_T_Blocked			2
#define DBG_T_CritSec			3

#define DBG_L_386			1

#define DBG_LEN_387			108

#define DBG_CO_387			1

#define DBG_O_OBJMTE			0x10000000

#define DBG_W_Global			0x00000001
#define DBG_W_Local			0x00000002
#define DBG_W_Execute			0x00010000
#define DBG_W_Write			0x00020000
#define DBG_W_ReadWrite			0x00030000

#define DBG_X_PRE_FIRST_CHANCE		0x00000000
#define DBG_X_FIRST_CHANCE		0x00000001
#define DBG_X_LAST_CHANCE		0x00000002
#define DBG_X_STACK_INVALID		0x00000003

typedef struct _TStat
{
  UCHAR	 DbgState;
  UCHAR	 TState;
  USHORT TPriority;
} TStat_t;

typedef struct _uDB
{
  ULONG	 Pid;
  ULONG	 Tid;
  LONG	 Cmd;
  LONG	 Value;
  ULONG	 Addr;
  ULONG	 Buffer;
  ULONG	 Len;
  ULONG	 Index;
  ULONG	 MTE;
  ULONG	 EAX;
  ULONG	 ECX;
  ULONG	 EDX;
  ULONG	 EBX;
  ULONG	 ESP;
  ULONG	 EBP;
  ULONG	 ESI;
  ULONG	 EDI;
  ULONG	 EFlags;
  ULONG	 EIP;
  ULONG	 CSLim;
  ULONG	 CSBase;
  UCHAR	 CSAcc;
  UCHAR	 CSAtr;
  USHORT CS;
  ULONG	 DSLim;
  ULONG	 DSBase;
  UCHAR	 DSAcc;
  UCHAR	 DSAtr;
  USHORT DS;
  ULONG	 ESLim;
  ULONG	 ESBase;
  UCHAR	 ESAcc;
  UCHAR	 ESAtr;
  USHORT ES;
  ULONG	 FSLim;
  ULONG	 FSBase;
  UCHAR	 FSAcc;
  UCHAR	 FSAtr;
  USHORT FS;
  ULONG	 GSLim;
  ULONG	 GSBase;
  UCHAR	 GSAcc;
  UCHAR	 GSAtr;
  USHORT GS;
  ULONG	 SSLim;
  ULONG	 SSBase;
  UCHAR	 SSAcc;
  UCHAR	 SSAtr;
  USHORT SS;
} uDB_t;

ULONG DosDebug (uDB_t *pDebugBuffer);

#endif /* INCL_DOSPROCESS */

/* ---------------------------- MESSAGES ---------------------------------- */

#if defined (INCL_DOSMISC)

ULONG DosGetMessage (PCHAR *pTable, ULONG ulTableEntries,
    PCHAR pBuffer, ULONG ulBufferLengthMax, ULONG ulMsgnNumber,
    PCSZ pszFile, PULONG pulBufferLength);
ULONG DosInsertMessage (PCHAR *pTable, ULONG ulCount, PCSZ pszMsg,
    ULONG ulMsgLength, PCHAR pBuffer, ULONG ulBufferLengthMax,
    PULONG pulBufferLength);
ULONG DosPutMessage (HFILE hfile, ULONG ulMessageLength, PCHAR pMessage);
ULONG DosQueryMessageCP (PCHAR pBuffer, ULONG ulBufferLengthMax,
    PCSZ pszFilename, PULONG pulBufferLength);

#endif /* INCL_DOSMISC */

/* ----------------------------- RAS -------------------------------------- */

#if defined (INCL_DOSRAS)

#define DDP_DISABLEPROCDUMP		0
#define DDP_ENABLEPROCDUMP		1
#define DDP_PERFORMPROCDUMP		2

#define LF_LOGENABLE			0x0001
#define LF_LOGAVAILABLE			0x0002

#define SIS_MMIOADDR			0
#define SIS_MEC_TABLE			1
#define SIS_SYS_LOG			2

#define SPU_DISABLESUPPRESSION		0
#define SPU_ENABLESUPPRESSION		1

ULONG DosDumpProcess (ULONG ulFlag, ULONG ulDrive, PID pid);
ULONG DosForceSystemDump (ULONG ulReserved);
ULONG DosQueryRASInfo (ULONG ulIndex, PPVOID addr);
ULONG DosSuppressPopUps (ULONG ulFlag, ULONG ulDrive);

#endif /* INCL_DOSRAS */

/* ---------------------------- REXX -------------------------------------- */

#define RXAUTOBUFLEN			256

typedef struct _RXSTRING
{
  ULONG strlength;
  PCH	strptr;
} RXSTRING;
typedef RXSTRING *PRXSTRING;

typedef struct _RXSYSEXIT
{
  PSZ  sysexit_name;
  LONG sysexit_code;
} RXSYSEXIT;
typedef RXSYSEXIT *PRXSYSEXIT;

#define RXNULLSTRING(r)		((r).strptr == (PCH)0)
#define RXZEROLENSTRING(r)	((r).strptr != (PCH)0 && (r).strlength == 0)
#define RXVALIDSTRING(r)	((r).strptr != (PCH)0 && (r).strlength != 0)
#define RXSTRLEN(r)		(RXNULLSTRING(r) ? 0 : (r).strlength)
#define RXSTRPTR(r)		(r).strptr
#define MAKERXSTRING(r,p,l) \
    ((r).strptr = (PCH)p, (r).strlength = (ULONG)l)

#define RXCOMMAND			0
#define RXSUBROUTINE			1
#define RXFUNCTION			2

#if defined (INCL_RXSUBCOM)

#define RXSUBCOM_DROPPABLE		0x0000
#define RXSUBCOM_NONDROP		0x0001

#define RXSUBCOM_ISREG			0x0001
#define RXSUBCOM_ERROR			0x0001
#define RXSUBCOM_FAILURE		0x0002

#define RXSUBCOM_BADENTRY		1001
#define RXSUBCOM_NOEMEM			1002
#define RXSUBCOM_BADTYPE		1003
#define RXSUBCOM_NOTINIT		1004

#define RXSUBCOM_OK			0
#define RXSUBCOM_DUP			10
#define RXSUBCOM_MAXREG			20
#define RXSUBCOM_NOTREG			30
#define RXSUBCOM_NOCANDROP		40
#define RXSUBCOM_LOADERR		50
#define RXSUBCOM_NOPROC			127

typedef ULONG RexxSubcomHandler (PRXSTRING prxCommand, PUSHORT pusFlags,
    PRXSTRING prxResult);

ULONG RexxDeregisterSubcom (PCSZ pszEnvName, PCSZ pszModuleName);
ULONG RexxQuerySubcom (PCSZ pszEnvName, PCSZ pszModuleName, PUSHORT pusFlags,
    PUCHAR pUserWord);
ULONG RexxRegisterSubcomDll (PCSZ pszEnvName, PCSZ pszModuleName,
    PCSZ pszEntryPoint, PUCHAR pUserArea, ULONG ulDropAuth);
ULONG RexxRegisterSubcomExe (PCSZ pszEnvName, PFN pfnEntryPoint,
    PUCHAR pUserArea);

#define REXXDEREGISTERSUBCOM  RexxDeregisterSubcom
#define REXXREGISTERSUBCOMDLL		RexxRegisterSubcomDll
#define REXXREGISTERSUBCOMEXE		RexxRegisterSubcomExe
#define REXXQUERYSUBCOM			RexxQuerySubcom

#endif /* INCL_RXSUBCOM */

#if defined (INCL_RXSHV)

#define RXSHV_SET			0x0000
#define RXSHV_FETCH			0x0001
#define RXSHV_DROPV			0x0002
#define RXSHV_SYSET			0x0003
#define RXSHV_SYFET			0x0004
#define RXSHV_SYDRO			0x0005
#define RXSHV_NEXTV			0x0006
#define RXSHV_PRIV			0x0007
#define RXSHV_EXIT			0x0008

#define RXSHV_NOAVL			144

#define RXSHV_OK			0x0000
#define RXSHV_NEWV			0x0001
#define RXSHV_LVAR			0x0002
#define RXSHV_TRUNC			0x0004
#define RXSHV_BADN			0x0008
#define RXSHV_MEMFL			0x0010
#define RXSHV_BADF			0x0080

typedef struct _SHVBLOCK
{
  struct _SHVBLOCK *shvnext;
  RXSTRING	    shvname;
  RXSTRING	    shvvalue;
  ULONG		    shvnamelen;
  ULONG		    shvvaluelen;
  UCHAR		    shvcode;
  UCHAR		    shvret;
} SHVBLOCK;
typedef SHVBLOCK *PSHVBLOCK;

ULONG RexxVariablePool (PSHVBLOCK pRequest);

#define REXXVARIABLEPOOL		RexxVariablePool

#endif /* INCL_RXSHV */

#if defined (INCL_RXFUNC)

#define RXFUNC_DYNALINK			1
#define RXFUNC_CALLENTRY		2

#define RXFUNC_OK			0
#define RXFUNC_DEFINED			10
#define RXFUNC_NOMEM			20
#define RXFUNC_NOTREG			30
#define RXFUNC_MODNOTFND		40
#define RXFUNC_ENTNOTFND		50
#define RXFUNC_NOTINIT			60
#define RXFUNC_BADTYPE			70

typedef ULONG RexxFunctionHandler (PCSZ pszName, ULONG ulArgCount,
    PRXSTRING prxArgList, PCSZ pszQueueName, PRXSTRING prxResult);

ULONG RexxDeregisterFunction (PCSZ pszFuncName);
ULONG RexxQueryFunction (PCSZ pszFuncName);
ULONG RexxRegisterFunctionDll (PCSZ pszFuncName, PCSZ pszModuleName,
    PCSZ pszEntryPoint);
ULONG RexxRegisterFunctionExe (PCSZ pszFuncName,
    RexxFunctionHandler *pfnEntryPoint);

#define REXXDEREGISTERFUNCTION	RexxDeregisterFunction
#define REXXQUERYFUNCTION  RexxQueryFunction
#define REXXREGISTERFUNCTIONDLL		RexxRegisterFunctionDll
#define REXXREGISTERFUNCTIONEXE		RexxRegisterFunctionExe

#endif /* INCL_RXFUNC */

#if defined (INCL_RXSYSEXIT)

#define RXEXIT_DROPPABLE		0x0000
#define RXEXIT_NONDROP			0x0001

#define RXEXIT_HANDLED			0
#define RXEXIT_NOT_HANDLED		1
#define RXEXIT_RAISE_ERROR		(-1)

#define RXEXIT_ISREG			0x0001
#define RXEXIT_ERROR			0x0001
#define RXEXIT_FAILURE			0x0002

#define RXEXIT_BADENTRY			1001
#define RXEXIT_NOEMEM			1002
#define RXEXIT_BADTYPE			1003
#define RXEXIT_NOTINIT			1004

#define RXEXIT_OK			0
#define RXEXIT_DUP			10
#define RXEXIT_MAXREG			20
#define RXEXIT_NOTREG			30
#define RXEXIT_NOCANDROP		40
#define RXEXIT_LOADERR			50
#define RXEXIT_NOPROC			127

#define RXENDLST			0

#define RXFNC				2
#define RXFNCCAL			1

#define RXCMD				3
#define RXCMDHST			1

#define RXMSQ				4
#define RXMSQPLL			1
#define RXMSQPSH			2
#define RXMSQSIZ			3
#define RXMSQNAM			20

#define RXSIO				5
#define RXSIOSAY			1
#define RXSIOTRC			2
#define RXSIOTRD			3
#define RXSIODTR			4
#define RXSIOTLL			5

#define RXHLT				7
#define RXHLTCLR			1
#define RXHLTTST			2

#define RXTRC				8
#define RXTRCTST			1

#define RXINI				9
#define RXINIEXT			1

#define RXTER				10
#define RXTEREXT			1

#define RXNOOFEXITS			11

typedef PUCHAR PEXIT;

typedef struct _RXFNC_FLAGS
{
  ULONG rxfferr	 : 1;
  ULONG rxffnfnd : 1;
  ULONG rxffsub	 : 1;
} RXFNC_FLAGS;

typedef struct _RXFNCCAL_PARM
{
  RXFNC_FLAGS rxfnc_flags;
  PUCHAR      rxfnc_name;
  USHORT      rxfnc_namel;
  PUCHAR      rxfnc_que;
  USHORT      rxfnc_quel;
  USHORT      rxfnc_argc;
  PRXSTRING   rxfnc_argv;
  RXSTRING    rxfnc_retc;
} RXFNCCAL_PARM;

typedef struct _RXCMD_FLAGS
{
  ULONG rxfcfail : 1;
  ULONG rxfcerr	 : 1;
} RXCMD_FLAGS;

typedef struct _RXCMDHST_PARM
{
  RXCMD_FLAGS rxcmd_flags;
  PUCHAR      rxcmd_address;
  USHORT      rxcmd_addressl;
  PUCHAR      rxcmd_dll;
  USHORT      rxcmd_dll_len;
  RXSTRING    rxcmd_command;
  RXSTRING    rxcmd_retc;
} RXCMDHST_PARM;

typedef struct _RXMSQPLL_PARM
{
  RXSTRING rxmsq_retc;
} RXMSQPLL_PARM;

typedef struct _RXMSQ_FLAGS
{
  ULONG rxfmlifo : 1;
} RXMSQ_FLAGS;

typedef struct _RXMSQPSH_PARM
{
  RXMSQ_FLAGS rxmsq_flags;
  RXSTRING    rxmsq_value;
} RXMSQPSH_PARM;

typedef struct _RXMSQSIZ_PARM
{
  ULONG rxmsq_size;
} RXMSQSIZ_PARM;

typedef struct _RXMSQNAM_PARM
{
  RXSTRING rxmsq_name;
} RXMSQNAM_PARM;

typedef struct _RXSIOSAY_PARM
{
  RXSTRING rxsio_string;
} RXSIOSAY_PARM;

typedef struct _RXSIOTRC_PARM
{
  RXSTRING rxsio_string;
} RXSIOTRC_PARM;

typedef struct _RXSIOTRD_PARM
{
  RXSTRING rxsiotrd_retc;
} RXSIOTRD_PARM;

typedef struct _RXSIODR_PARM
{
  RXSTRING rxsiodtr_retc;
} RXSIODTR_PARM;

typedef struct _RXHLT_FLAGS
{
  ULONG rxfhhalt : 1;
} RXHLT_FLAGS;

typedef struct _RXHLTTST_PARM
{
  RXHLT_FLAGS rxhlt_flags;
} RXHLTTST_PARM;

typedef struct _RXTRC_FLAGS
{
  ULONG rxftrace : 1;
} RXTRC_FLAGS;

typedef struct _RXTRCTST_PARM
{
  RXTRC_FLAGS rxtrc_flags;
} RXTRCTST_PARM;

typedef LONG RexxExitHandler (LONG lExitNumber, LONG lSubfunction,
    PEXIT pParmBlock);

ULONG RexxDeregisterExit (PCSZ pszEnvName, PCSZ pszModuleName);
ULONG RexxQueryExit (PCSZ pszEnvName, PCSZ pszModuleName, PUSHORT pusFlag,
    PUCHAR pUserWord);
ULONG RexxRegisterExitDll (PCSZ pszEnvName, PCSZ pszModuleName,
    PCSZ pszEntryName, PUCHAR pUserArea, ULONG ulDropAuth);
ULONG RexxRegisterExitExe (PCSZ pszEnvName, PFN pfnEntryPoint,
    PUCHAR pUserArea);

#define REXXDEREGISTEREXIT		RexxDeregisterExit
#define REXXQUERYEXIT			RexxQueryExit
#define REXXREGISTEREXITDLL		RexxRegisterExitDll
#define REXXREGISTEREXITEXE		RexxRegisterExitExe

#endif /* INCL_RXSYSEXIT */

#if defined (INCL_RXARI)

#define RXARI_OK			0
#define RXARI_NOT_FOUND			1
#define RXARI_PROCESSING_ERROR		2

ULONG RexxResetTrace (PID pid, TID tid);
ULONG RexxSetHalt (PID pid, TID tid);
ULONG RexxSetTrace (PID pid, TID tid);

#define REXXRESETTRACE			RexxResetTrace
#define REXXSETHALT			RexxSetHalt
#define REXXSETTRACE			RexxSetTrace

#endif /* INCL_RXARI */

#if defined (INCL_RXMACRO)

#define RXMACRO_SEARCH_BEFORE		1
#define RXMACRO_SEARCH_AFTER		2

#define RXMACRO_OK			0
#define RXMACRO_NO_STORAGE		1
#define RXMACRO_NOT_FOUND		2
#define RXMACRO_EXTENSION_REQUIRED	3
#define RXMACRO_ALREADY_EXISTS		4
#define RXMACRO_FILE_ERROR		5
#define RXMACRO_SIGNATURE_ERROR		6
#define RXMACRO_SOURCE_NOT_FOUND	7
#define RXMACRO_INVALID_POSITION	8
#define RXMACRO_NOT_INIT		9

ULONG RexxAddMacro (PCSZ pszFuncName, PCSZ pszSourceFile, ULONG ulPosition);
ULONG RexxClearMacroSpace (VOID);
ULONG RexxDropMacro (PCSZ pszFuncName);
ULONG RexxLoadMacroSpace (ULONG ulFuncCout, PCSZ *apszFuncNames,
    PCSZ pszMacroLibFile);
ULONG RexxQueryMacro (PCSZ pszFuncName, PUSHORT pusPosition);
ULONG RexxReorderMacro(PCSZ pszFuncName, ULONG ulPosition);
ULONG RexxSaveMacroSpace (ULONG ulFuncCount, PCSZ *apszFuncNames,
    PCSZ pszMacroLibFile);

#define REXXADDMACRO			RexxAddMacro
#define REXXCLEARMACROSPACE		RexxClearMacroSpace
#define REXXDROPMACRO			RexxDropMacro
#define REXXSAVEMACROSPACE		RexxSaveMacroSpace
#define REXXLOADMACROSPACE		RexxLoadMacroSpace
#define REXXQUERYMACRO			RexxQueryMacro
#define REXXREORDERMACRO		RexxReorderMacro

#endif /* INCL_RXMACRO	*/

LONG RexxStart (LONG lArgCount, PRXSTRING prxArgList, PCSZ pszProgramName,
    PRXSTRING prxInstore, PCSZ pszEnvName, LONG lCallType, PRXSYSEXIT pExits,
    PSHORT psReturnCode, PRXSTRING prxResult);

#define REXXSTART			RexxStart

/* ----------------------- PRESENTATION MANAGER --------------------------- */

#define	 CTLS_WM_BIDI_FIRST		0x390
#define	 CTLS_WM_BIDI_LAST		0x39f

#if defined (INCL_NLS)
#define WM_DBCSFIRST			0x00b0
#define WM_DBCSLAST			0x00cf
#endif /* INCL_NLS */

#define WC_FRAME			((PSZ)0xffff0001)
#define WC_COMBOBOX			((PSZ)0xffff0002)
#define WC_BUTTON			((PSZ)0xffff0003)
#define WC_MENU				((PSZ)0xffff0004)
#define WC_STATIC			((PSZ)0xffff0005)
#define WC_ENTRYFIELD			((PSZ)0xffff0006)
#define WC_LISTBOX			((PSZ)0xffff0007)
#define WC_SCROLLBAR			((PSZ)0xffff0008)
#define WC_TITLEBAR			((PSZ)0xffff0009)
#define WC_MLE				((PSZ)0xffff000a)
#define WC_APPSTAT			((PSZ)0xffff0010)
#define WC_KBDSTAT			((PSZ)0xffff0011)
#define WC_PECIC			((PSZ)0xffff0012)
#define WC_DBE_KKPOPUP			((PSZ)0xffff0013)
#define WC_SPINBUTTON			((PSZ)0xffff0020)
#define WC_CONTAINER			((PSZ)0xffff0025)
#define WC_SLIDER			((PSZ)0xffff0026)
#define WC_VALUESET			((PSZ)0xffff0027)
#define WC_NOTEBOOK			((PSZ)0xffff0028)
#define WC_PENFIRST			((PSZ)0xffff0029)
#define WC_PENLAST			((PSZ)0xffff002c)
#define WC_MMPMFIRST			((PSZ)0xffff0040)
#define WC_CIRCULARSLIDER		((PSZ)0xffff0041)
#define WC_MMPMLAST			((PSZ)0xffff004f)

#define WS_VISIBLE			0x80000000
#define WS_DISABLED			0x40000000
#define WS_CLIPCHILDREN			0x20000000
#define WS_CLIPSIBLINGS			0x10000000
#define WS_PARENTCLIP			0x08000000
#define WS_SAVEBITS			0x04000000
#define WS_SYNCPAINT			0x02000000
#define WS_MINIMIZED			0x01000000
#define WS_MAXIMIZED			0x00800000
#define WS_ANIMATE			0x00400000
#define WS_GROUP			0x00010000
#define WS_TABSTOP			0x00020000
#define WS_MULTISELECT			0x00040000

#define CS_MOVENOTIFY			0x00000001
#define CS_SIZEREDRAW			0x00000004
#define CS_HITTEST			0x00000008
#define CS_PUBLIC			0x00000010
#define CS_FRAME			0x00000020
#define CS_CLIPCHILDREN			0x20000000
#define CS_CLIPSIBLINGS			0x10000000
#define CS_PARENTCLIP			0x08000000
#define CS_SAVEBITS			0x04000000
#define CS_SYNCPAINT			0x02000000

#define MID_NONE			(-1)
#define MID_ERROR			(-1)

#define DB_PATCOPY			0x0000
#define DB_PATINVERT			0x0001
#define DB_DESTINVERT			0x0002
#define DB_AREAMIXMODE			0x0003

#define DB_ROP				0x0007
#define DB_INTERIOR			0x0008
#define DB_AREAATTRS			0x0010
#define DB_STANDARD			0x0100
#define DB_DLGBORDER			0x0200

#define DBM_NORMAL			0x0000
#define DBM_INVERT			0x0001
#define DBM_HALFTONE			0x0002
#define DBM_STRETCH			0x0004
#define DBM_IMAGEATTRS			0x0008

#define DT_LEFT				0x00000000
#define DT_QUERYEXTENT			0x00000002
#define DT_UNDERSCORE			0x00000010
#define DT_STRIKEOUT			0x00000020
#define DT_TEXTATTRS			0x00000040
#define DT_EXTERNALLEADING		0x00000080
#define DT_CENTER			0x00000100
#define DT_RIGHT			0x00000200
#define DT_TOP				0x00000000
#define DT_VCENTER			0x00000400
#define DT_BOTTOM			0x00000800
#define DT_HALFTONE			0x00001000
#define DT_MNEMONIC			0x00002000
#define DT_WORDBREAK			0x00004000
#define DT_ERASERECT			0x00008000

#define QW_NEXT				0
#define QW_PREV				1
#define QW_TOP				2
#define QW_BOTTOM			3
#define QW_OWNER			4
#define QW_PARENT			5
#define QW_NEXTTOP			6
#define QW_PREVTOP			7
#define QW_FRAMEOWNER			8

#define SWP_SIZE			0x0001
#define SWP_MOVE			0x0002
#define SWP_ZORDER			0x0004
#define SWP_SHOW			0x0008
#define SWP_HIDE			0x0010
#define SWP_NOREDRAW			0x0020
#define SWP_NOADJUST			0x0040
#define SWP_ACTIVATE			0x0080
#define SWP_DEACTIVATE			0x0100
#define SWP_EXTSTATECHANGE		0x0200
#define SWP_MINIMIZE			0x0400
#define SWP_MAXIMIZE			0x0800
#define SWP_RESTORE			0x1000
#define SWP_FOCUSACTIVATE		0x2000
#define SWP_FOCUSDEACTIVATE		0x4000
#define SWP_NOAUTOCLOSE			0x8000

#define AWP_MINIMIZED			0x00010000
#define AWP_MAXIMIZED			0x00020000
#define AWP_RESTORED			0x00040000
#define AWP_ACTIVATE			0x00080000
#define AWP_DEACTIVATE			0x00100000

#define HWND_DESKTOP			((HWND)1)
#define HWND_OBJECT			((HWND)2)
#define HWND_TOP			((HWND)3)
#define HWND_BOTTOM			((HWND)4)
#define HWND_THREADCAPTURE		((HWND)5)

#define EAF_DEFAULTOWNER		0x0001
#define EAF_UNCHANGEABLE		0x0002
#define EAF_REUSEICON			0x0004

#define ICON_FILE			1
#define ICON_RESOURCE			2
#define ICON_DATA			3
#define ICON_CLEAR			4

#define SZDDEFMT_RTF			"Rich Text Format"
#define SZDDEFMT_PTRPICT		"Printer_Picture"

#define STR_DLLNAME			"keyremap"

#if defined (INCL_WINERRORS)

#define PMERR_INVALID_HWND		0x1001
#define PMERR_INVALID_HMQ		0x1002
#define PMERR_PARAMETER_OUT_OF_RANGE	0x1003
#define PMERR_WINDOW_LOCK_UNDERFLOW	0x1004
#define PMERR_WINDOW_LOCK_OVERFLOW	0x1005
#define PMERR_BAD_WINDOW_LOCK_COUNT	0x1006
#define PMERR_WINDOW_NOT_LOCKED		0x1007
#define PMERR_INVALID_SELECTOR		0x1008
#define PMERR_CALL_FROM_WRONG_THREAD	0x1009
#define PMERR_RESOURCE_NOT_FOUND	0x100a
#define PMERR_INVALID_STRING_PARM	0x100b
#define PMERR_INVALID_HHEAP		0x100c
#define PMERR_INVALID_HEAP_POINTER	0x100d
#define PMERR_INVALID_HEAP_SIZE_PARM	0x100e
#define PMERR_INVALID_HEAP_SIZE		0x100f
#define PMERR_INVALID_HEAP_SIZE_WORD	0x1010
#define PMERR_HEAP_OUT_OF_MEMORY	0x1011
#define PMERR_HEAP_MAX_SIZE_REACHED	0x1012
#define PMERR_INVALID_HATOMTBL		0x1013
#define PMERR_INVALID_ATOM		0x1014
#define PMERR_INVALID_ATOM_NAME		0x1015
#define PMERR_INVALID_INTEGER_ATOM	0x1016
#define PMERR_ATOM_NAME_NOT_FOUND	0x1017
#define PMERR_QUEUE_TOO_LARGE		0x1018
#define PMERR_INVALID_FLAG		0x1019
#define PMERR_INVALID_HACCEL		0x101a
#define PMERR_INVALID_HPTR		0x101b
#define PMERR_INVALID_HENUM		0x101c
#define PMERR_INVALID_SRC_CODEPAGE	0x101d
#define PMERR_INVALID_DST_CODEPAGE	0x101e
#define PMERR_UNKNOWN_COMPONENT_ID	0x101f
#define PMERR_UNKNOWN_ERROR_CODE	0x1020
#define PMERR_SEVERITY_LEVELS		0x1021
#define PMERR_INVALID_RESOURCE_FORMAT	0x1034
#define PMERR_NO_MSG_QUEUE		0x1036
#define PMERR_CANNOT_SET_FOCUS		0x1037
#define PMERR_QUEUE_FULL		0x1038
#define PMERR_LIBRARY_LOAD_FAILED	0x1039
#define PMERR_PROCEDURE_LOAD_FAILED	0x103a
#define PMERR_LIBRARY_DELETE_FAILED	0x103b
#define PMERR_PROCEDURE_DELETE_FAILED	0x103c
#define PMERR_ARRAY_TOO_LARGE		0x103d
#define PMERR_ARRAY_TOO_SMALL		0x103e
#define PMERR_DATATYPE_ENTRY_BAD_INDEX	0x103f
#define PMERR_DATATYPE_ENTRY_CTL_BAD	0x1040
#define PMERR_DATATYPE_ENTRY_CTL_MISS	0x1041
#define PMERR_DATATYPE_ENTRY_INVALID	0x1042
#define PMERR_DATATYPE_ENTRY_NOT_NUM	0x1043
#define PMERR_DATATYPE_ENTRY_NOT_OFF	0x1044
#define PMERR_DATATYPE_INVALID		0x1045
#define PMERR_DATATYPE_NOT_UNIQUE	0x1046
#define PMERR_DATATYPE_TOO_LONG		0x1047
#define PMERR_DATATYPE_TOO_SMALL	0x1048
#define PMERR_DIRECTION_INVALID		0x1049
#define PMERR_INVALID_HAB		0x104a
#define PMERR_INVALID_HSTRUCT		0x104d
#define PMERR_LENGTH_TOO_SMALL		0x104e
#define PMERR_MSGID_TOO_SMALL		0x104f
#define PMERR_NO_HANDLE_ALLOC		0x1050
#define PMERR_NOT_IN_A_PM_SESSION	0x1051
#define PMERR_MSG_QUEUE_ALREADY_EXISTS	0x1052
#define PMERR_SEND_MSG_TIMEOUT		0x1053
#define PMERROR_SEND_MSG_FAILED		0x1054
#define PMERR_OLD_RESOURCE		0x1055
#define PMERR_WPDSERVER_IS_ACTIVE	0x1056
#define PMERR_WPDSERVER_NOT_STARTED	0x1057
#define PMERR_SOMDD_IS_ACTIVE		0x1058
#define PMERR_SOMDD_NOT_STARTED		0x1059

#define PMERR_BIDI_FIRST		0x10f0
#define PMERR_BIDI_LAST			0x10ff

#endif /* INCL_WINERRORS */

#if defined (INCL_SHLERRORS)

#define PMERR_INVALID_PIB		0x1101
#define PMERR_INSUFF_SPACE_TO_ADD	0x1102
#define PMERR_INVALID_GROUP_HANDLE	0x1103
#define PMERR_DUPLICATE_TITLE		0x1104
#define PMERR_INVALID_TITLE		0x1105
#define PMERR_HANDLE_NOT_IN_GROUP	0x1107
#define PMERR_INVALID_TARGET_HANDLE	0x1106
#define PMERR_INVALID_PATH_STATEMENT	0x1108
#define PMERR_NO_PROGRAM_FOUND		0x1109
#define PMERR_INVALID_BUFFER_SIZE	0x110a
#define PMERR_BUFFER_TOO_SMALL		0x110b
#define PMERR_PL_INITIALISATION_FAIL	0x110c
#define PMERR_CANT_DESTROY_SYS_GROUP	0x110d
#define PMERR_INVALID_TYPE_CHANGE	0x110e
#define PMERR_INVALID_PROGRAM_HANDLE	0x110f
#define PMERR_NOT_CURRENT_PL_VERSION	0x1110
#define PMERR_INVALID_CIRCULAR_REF	0x1111
#define PMERR_MEMORY_ALLOCATION_ERR	0x1112
#define PMERR_MEMORY_DEALLOCATION_ERR	0x1113
#define PMERR_TASK_HEADER_TOO_BIG	0x1114
#define PMERR_INVALID_INI_FILE_HANDLE	0x1115
#define PMERR_MEMORY_SHARE		0x1116
#define PMERR_OPEN_QUEUE		0x1117
#define PMERR_CREATE_QUEUE		0x1118
#define PMERR_WRITE_QUEUE		0x1119
#define PMERR_READ_QUEUE		0x111a
#define PMERR_CALL_NOT_EXECUTED		0x111b
#define PMERR_UNKNOWN_APIPKT		0x111c
#define PMERR_INITHREAD_EXISTS		0x111d
#define PMERR_CREATE_THREAD		0x111e
#define PMERR_NO_HK_PROFILE_INSTALLED	0x111f
#define PMERR_INVALID_DIRECTORY		0x1120
#define PMERR_WILDCARD_IN_FILENAME	0x1121
#define PMERR_FILENAME_BUFFER_FULL	0x1122
#define PMERR_FILENAME_TOO_LONG		0x1123
#define PMERR_INI_FILE_IS_SYS_OR_USER	0x1124
#define PMERR_BROADCAST_PLMSG		0x1125
#define PMERR_190_INIT_DONE		0x1126
#define PMERR_HMOD_FOR_PMSHAPI		0x1127
#define PMERR_SET_HK_PROFILE		0x1128
#define PMERR_API_NOT_ALLOWED		0x1129
#define PMERR_INI_STILL_OPEN		0x112a
#define PMERR_PROGDETAILS_NOT_IN_INI	0x112b
#define PMERR_PIBSTRUCT_NOT_IN_INI	0x112c
#define PMERR_INVALID_DISKPROGDETAILS	0x112d
#define PMERR_PROGDETAILS_READ_FAILURE	0x112e
#define PMERR_PROGDETAILS_WRITE_FAILURE 0x112f
#define PMERR_PROGDETAILS_QSIZE_FAILURE 0x1130
#define PMERR_INVALID_PROGDETAILS	0x1131
#define PMERR_SHEPROFILEHOOK_NOT_FOUND	0x1132
#define PMERR_190PLCONVERTED		0x1133
#define PMERR_FAILED_TO_CONVERT_INI_PL	0x1134
#define PMERR_PMSHAPI_NOT_INITIALISED	0x1135
#define PMERR_INVALID_SHELL_API_HOOK_ID 0x1136
#define PMERR_DOS_ERROR			0x1200
#define PMERR_NO_SPACE			0x1201
#define PMERR_INVALID_SWITCH_HANDLE	0x1202
#define PMERR_NO_HANDLE			0x1203
#define PMERR_INVALID_PROCESS_ID	0x1204
#define PMERR_NOT_SHELL			0x1205
#define PMERR_INVALID_WINDOW		0x1206
#define PMERR_INVALID_POST_MSG		0x1207
#define PMERR_INVALID_PARAMETERS	0x1208
#define PMERR_INVALID_PROGRAM_TYPE	0x1209
#define PMERR_NOT_EXTENDED_FOCUS	0x120a
#define PMERR_INVALID_SESSION_ID	0x120b
#define PMERR_SMG_INVALID_ICON_FILE	0x120c
#define PMERR_SMG_ICON_NOT_CREATED	0x120d
#define PMERR_SHL_DEBUG			0x120e
#define PMERR_OPENING_INI_FILE		0x1301
#define PMERR_INI_FILE_CORRUPT		0x1302
#define PMERR_INVALID_PARM		0x1303
#define PMERR_NOT_IN_IDX		0x1304
#define PMERR_NO_ENTRIES_IN_GROUP	0x1305
#define PMERR_INI_WRITE_FAIL		0x1306
#define PMERR_IDX_FULL			0x1307
#define PMERR_INI_PROTECTED		0x1308
#define PMERR_MEMORY_ALLOC		0x1309
#define PMERR_INI_INIT_ALREADY_DONE	0x130a
#define PMERR_INVALID_INTEGER		0x130b
#define PMERR_INVALID_ASCIIZ		0x130c
#define PMERR_CAN_NOT_CALL_SPOOLER	0x130d
#define PMERR_VALIDATION_REJECTED	0x130d /*!*/
#define PMERR_WARNING_WINDOW_NOT_KILLED 0x1401
#define PMERR_ERROR_INVALID_WINDOW	0x1402
#define PMERR_ALREADY_INITIALIZED	0x1403
#define PMERR_MSG_PROG_NO_MOU		0x1405
#define PMERR_MSG_PROG_NON_RECOV	0x1406
#define PMERR_WINCONV_INVALID_PATH	0x1407
#define PMERR_PI_NOT_INITIALISED	0x1408
#define PMERR_PL_NOT_INITIALISED	0x1409
#define PMERR_NO_TASK_MANAGER		0x140a
#define PMERR_SAVE_NOT_IN_PROGRESS	0x140b
#define PMERR_NO_STACK_SPACE		0x140c
#define PMERR_INVALID_COLR_FIELD	0x140d
#define PMERR_INVALID_COLR_VALUE	0x140e
#define PMERR_COLR_WRITE		0x140f
#define PMERR_TARGET_FILE_EXISTS	0x1501
#define PMERR_SOURCE_SAME_AS_TARGET	0x1502
#define PMERR_SOURCE_FILE_NOT_FOUND	0x1503
#define PMERR_INVALID_NEW_PATH		0x1504
#define PMERR_TARGET_FILE_NOT_FOUND	0x1505
#define PMERR_INVALID_DRIVE_NUMBER	0x1506
#define PMERR_NAME_TOO_LONG		0x1507
#define PMERR_NOT_ENOUGH_ROOM_ON_DISK	0x1508
#define PMERR_NOT_ENOUGH_MEM		0x1509
#define PMERR_LOG_DRV_DOES_NOT_EXIST	0x150b
#define PMERR_INVALID_DRIVE		0x150c
#define PMERR_ACCESS_DENIED		0x150d
#define PMERR_NO_FIRST_SLASH		0x150e
#define PMERR_READ_ONLY_FILE		0x150f
#define PMERR_GROUP_PROTECTED		0x151f
#define PMERR_INVALID_PROGRAM_CATEGORY	0x152f
#define PMERR_INVALID_APPL		0x1530
#define PMERR_CANNOT_START		0x1531
#define PMERR_STARTED_IN_BACKGROUND	0x1532
#define PMERR_INVALID_HAPP		0x1533
#define PMERR_CANNOT_STOP		0x1534
#define PMERR_INVALID_FREE_MESSAGE_ID	0x1630
#define PMERR_FUNCTION_NOT_SUPPORTED	0x1641
#define PMERR_INVALID_ARRAY_COUNT	0x1642
#define PMERR_INVALID_LENGTH		0x1643
#define PMERR_INVALID_BUNDLE_TYPE	0x1644
#define PMERR_INVALID_PARAMETER		0x1645
#define PMERR_INVALID_NUMBER_OF_PARMS	0x1646
#define PMERR_GREATER_THAN_64K		0x1647
#define PMERR_INVALID_PARAMETER_TYPE	0x1648
#define PMERR_NEGATIVE_STRCOND_DIM	0x1649
#define PMERR_INVALID_NUMBER_OF_TYPES	0x164a
#define PMERR_INCORRECT_HSTRUCT		0x164b
#define PMERR_INVALID_ARRAY_SIZE	0x164c
#define PMERR_INVALID_CONTROL_DATATYPE	0x164d
#define PMERR_INCOMPLETE_CONTROL_SEQU	0x164e
#define PMERR_INVALID_DATATYPE		0x164f
#define PMERR_INCORRECT_DATATYPE	0x1650
#define PMERR_NOT_SELF_DESCRIBING_DTYP	0x1651
#define PMERR_INVALID_CTRL_SEQ_INDEX	0x1652
#define PMERR_INVALID_TYPE_FOR_LENGTH	0x1653
#define PMERR_INVALID_TYPE_FOR_OFFSET	0x1654
#define PMERR_INVALID_TYPE_FOR_MPARAM	0x1655
#define PMERR_INVALID_MESSAGE_ID	0x1656
#define PMERR_C_LENGTH_TOO_SMALL	0x1657
#define PMERR_APPL_STRUCTURE_TOO_SMALL	0x1658
#define PMERR_INVALID_ERRORINFO_HANDLE	0x1659
#define PMERR_INVALID_CHARACTER_INDEX	0x165a

#endif /* INCL_SHLERRORS */

#if defined (INCL_GPIERRORS)

#define PMERR_OK			0x0000
#define PMERR_ALREADY_IN_AREA		0x2001
#define PMERR_ALREADY_IN_ELEMENT	0x2002
#define PMERR_ALREADY_IN_PATH		0x2003
#define PMERR_ALREADY_IN_SEG		0x2004
#define PMERR_AREA_INCOMPLETE		0x2005
#define PMERR_BASE_ERROR		0x2006
#define PMERR_BITBLT_LENGTH_EXCEEDED	0x2007
#define PMERR_BITMAP_IN_USE		0x2008
#define PMERR_BITMAP_IS_SELECTED	0x2009
#define PMERR_BITMAP_NOT_FOUND		0x200a
#define PMERR_BITMAP_NOT_SELECTED	0x200b
#define PMERR_BOUNDS_OVERFLOW		0x200c
#define PMERR_CALLED_SEG_IS_CHAINED	0x200d
#define PMERR_CALLED_SEG_IS_CURRENT	0x200e
#define PMERR_CALLED_SEG_NOT_FOUND	0x200f
#define PMERR_CANNOT_DELETE_ALL_DATA	0x2010
#define PMERR_CANNOT_REPLACE_ELEMENT_0	0x2011
#define PMERR_COL_TABLE_NOT_REALIZABLE	0x2012
#define PMERR_COL_TABLE_NOT_REALIZED	0x2013
#define PMERR_COORDINATE_OVERFLOW	0x2014
#define PMERR_CORR_FORMAT_MISMATCH	0x2015
#define PMERR_DATA_TOO_LONG		0x2016
#define PMERR_DC_IS_ASSOCIATED		0x2017
#define PMERR_DESC_STRING_TRUNCATED	0x2018
#define PMERR_DEVICE_DRIVER_ERROR_1	0x2019
#define PMERR_DEVICE_DRIVER_ERROR_2	0x201a
#define PMERR_DEVICE_DRIVER_ERROR_3	0x201b
#define PMERR_DEVICE_DRIVER_ERROR_4	0x201c
#define PMERR_DEVICE_DRIVER_ERROR_5	0x201d
#define PMERR_DEVICE_DRIVER_ERROR_6	0x201e
#define PMERR_DEVICE_DRIVER_ERROR_7	0x201f
#define PMERR_DEVICE_DRIVER_ERROR_8	0x2020
#define PMERR_DEVICE_DRIVER_ERROR_9	0x2021
#define PMERR_DEVICE_DRIVER_ERROR_10	0x2022
#define PMERR_DEV_FUNC_NOT_INSTALLED	0x2023
#define PMERR_DOSOPEN_FAILURE		0x2024
#define PMERR_DOSREAD_FAILURE		0x2025
#define PMERR_DRIVER_NOT_FOUND		0x2026
#define PMERR_DUP_SEG			0x2027
#define PMERR_DYNAMIC_SEG_SEQ_ERROR	0x2028
#define PMERR_DYNAMIC_SEG_ZERO_INV	0x2029
#define PMERR_ELEMENT_INCOMPLETE	0x202a
#define PMERR_ESC_CODE_NOT_SUPPORTED	0x202b
#define PMERR_EXCEEDS_MAX_SEG_LENGTH	0x202c
#define PMERR_FONT_AND_MODE_MISMATCH	0x202d
#define PMERR_FONT_FILE_NOT_LOADED	0x202e
#define PMERR_FONT_NOT_LOADED		0x202f
#define PMERR_FONT_TOO_BIG		0x2030
#define PMERR_HARDWARE_INIT_FAILURE	0x2031
#define PMERR_HBITMAP_BUSY		0x2032
#define PMERR_HDC_BUSY			0x2033
#define PMERR_HRGN_BUSY			0x2034
#define PMERR_HUGE_FONTS_NOT_SUPPORTED	0x2035
#define PMERR_ID_HAS_NO_BITMAP		0x2036
#define PMERR_IMAGE_INCOMPLETE		0x2037
#define PMERR_INCOMPAT_COLOR_FORMAT	0x2038
#define PMERR_INCOMPAT_COLOR_OPTIONS	0x2039
#define PMERR_INCOMPATIBLE_BITMAP	0x203a
#define PMERR_INCOMPATIBLE_METAFILE	0x203b
#define PMERR_INCORRECT_DC_TYPE		0x203c
#define PMERR_INSUFFICIENT_DISK_SPACE	0x203d
#define PMERR_INSUFFICIENT_MEMORY	0x203e
#define PMERR_INV_ANGLE_PARM		0x203f
#define PMERR_INV_ARC_CONTROL		0x2040
#define PMERR_INV_AREA_CONTROL		0x2041
#define PMERR_INV_ARC_POINTS		0x2042
#define PMERR_INV_ATTR_MODE		0x2043
#define PMERR_INV_BACKGROUND_COL_ATTR	0x2044
#define PMERR_INV_BACKGROUND_MIX_ATTR	0x2045
#define PMERR_INV_BITBLT_MIX		0x2046
#define PMERR_INV_BITBLT_STYLE		0x2047
#define PMERR_INV_BITMAP_DIMENSION	0x2048
#define PMERR_INV_BOX_CONTROL		0x2049
#define PMERR_INV_BOX_ROUNDING_PARM	0x204a
#define PMERR_INV_CHAR_ANGLE_ATTR	0x204b
#define PMERR_INV_CHAR_DIRECTION_ATTR	0x204c
#define PMERR_INV_CHAR_MODE_ATTR	0x204d
#define PMERR_INV_CHAR_POS_OPTIONS	0x204e
#define PMERR_INV_CHAR_SET_ATTR		0x204f
#define PMERR_INV_CHAR_SHEAR_ATTR	0x2050
#define PMERR_INV_CLIP_PATH_OPTIONS	0x2051
#define PMERR_INV_CODEPAGE		0x2052
#define PMERR_INV_COLOR_ATTR		0x2053
#define PMERR_INV_COLOR_DATA		0x2054
#define PMERR_INV_COLOR_FORMAT		0x2055
#define PMERR_INV_COLOR_INDEX		0x2056
#define PMERR_INV_COLOR_OPTIONS		0x2057
#define PMERR_INV_COLOR_START_INDEX	0x2058
#define PMERR_INV_COORD_OFFSET		0x2059
#define PMERR_INV_COORD_SPACE		0x205a
#define PMERR_INV_COORDINATE		0x205b
#define PMERR_INV_CORRELATE_DEPTH	0x205c
#define PMERR_INV_CORRELATE_TYPE	0x205d
#define PMERR_INV_CURSOR_BITMAP		0x205e
#define PMERR_INV_DC_DATA		0x205f
#define PMERR_INV_DC_TYPE		0x2060
#define PMERR_INV_DEVICE_NAME		0x2061
#define PMERR_INV_DEV_MODES_OPTIONS	0x2062
#define PMERR_INV_DRAW_CONTROL		0x2063
#define PMERR_INV_DRAW_VALUE		0x2064
#define PMERR_INV_DRAWING_MODE		0x2065
#define PMERR_INV_DRIVER_DATA		0x2066
#define PMERR_INV_DRIVER_NAME		0x2067
#define PMERR_INV_DRAW_BORDER_OPTION	0x2068
#define PMERR_INV_EDIT_MODE		0x2069
#define PMERR_INV_ELEMENT_OFFSET	0x206a
#define PMERR_INV_ELEMENT_POINTER	0x206b
#define PMERR_INV_END_PATH_OPTIONS	0x206c
#define PMERR_INV_ESC_CODE		0x206d
#define PMERR_INV_ESCAPE_DATA		0x206e
#define PMERR_INV_EXTENDED_LCID		0x206f
#define PMERR_INV_FILL_PATH_OPTIONS	0x2070
#define PMERR_INV_FIRST_CHAR		0x2071
#define PMERR_INV_FONT_ATTRS		0x2072
#define PMERR_INV_FONT_FILE_DATA	0x2073
#define PMERR_INV_FOR_THIS_DC_TYPE	0x2074
#define PMERR_INV_FORMAT_CONTROL	0x2075
#define PMERR_INV_FORMS_CODE		0x2076
#define PMERR_INV_FONTDEF		0x2077
#define PMERR_INV_GEOM_LINE_WIDTH_ATTR	0x2078
#define PMERR_INV_GETDATA_CONTROL	0x2079
#define PMERR_INV_GRAPHICS_FIELD	0x207a
#define PMERR_INV_HBITMAP		0x207b
#define PMERR_INV_HDC			0x207c
#define PMERR_INV_HJOURNAL		0x207d
#define PMERR_INV_HMF			0x207e
#define PMERR_INV_HPS			0x207f
#define PMERR_INV_HRGN			0x2080
#define PMERR_INV_ID			0x2081
#define PMERR_INV_IMAGE_DATA_LENGTH	0x2082
#define PMERR_INV_IMAGE_DIMENSION	0x2083
#define PMERR_INV_IMAGE_FORMAT		0x2084
#define PMERR_INV_IN_AREA		0x2085
#define PMERR_INV_IN_CALLED_SEG		0x2086
#define PMERR_INV_IN_CURRENT_EDIT_MODE	0x2087
#define PMERR_INV_IN_DRAW_MODE		0x2088
#define PMERR_INV_IN_ELEMENT		0x2089
#define PMERR_INV_IN_IMAGE		0x208a
#define PMERR_INV_IN_PATH		0x208b
#define PMERR_INV_IN_RETAIN_MODE	0x208c
#define PMERR_INV_IN_SEG		0x208d
#define PMERR_INV_IN_VECTOR_SYMBOL	0x208e
#define PMERR_INV_INFO_TABLE		0x208f
#define PMERR_INV_JOURNAL_OPTION	0x2090
#define PMERR_INV_KERNING_FLAGS		0x2091
#define PMERR_INV_LENGTH_OR_COUNT	0x2092
#define PMERR_INV_LINE_END_ATTR		0x2093
#define PMERR_INV_LINE_JOIN_ATTR	0x2094
#define PMERR_INV_LINE_TYPE_ATTR	0x2095
#define PMERR_INV_LINE_WIDTH_ATTR	0x2096
#define PMERR_INV_LOGICAL_ADDRESS	0x2097
#define PMERR_INV_MARKER_BOX_ATTR	0x2098
#define PMERR_INV_MARKER_SET_ATTR	0x2099
#define PMERR_INV_MARKER_SYMBOL_ATTR	0x209a
#define PMERR_INV_MATRIX_ELEMENT	0x209b
#define PMERR_INV_MAX_HITS		0x209c
#define PMERR_INV_METAFILE		0x209d
#define PMERR_INV_METAFILE_LENGTH	0x209e
#define PMERR_INV_METAFILE_OFFSET	0x209f
#define PMERR_INV_MICROPS_DRAW_CONTROL	0x20a0
#define PMERR_INV_MICROPS_FUNCTION	0x20a1
#define PMERR_INV_MICROPS_ORDER		0x20a2
#define PMERR_INV_MIX_ATTR		0x20a3
#define PMERR_INV_MODE_FOR_OPEN_DYN	0x20a4
#define PMERR_INV_MODE_FOR_REOPEN_SEG	0x20a5
#define PMERR_INV_MODIFY_PATH_MODE	0x20a6
#define PMERR_INV_MULTIPLIER		0x20a7
#define PMERR_INV_NESTED_FIGURES	0x20a8
#define PMERR_INV_OR_INCOMPAT_OPTIONS	0x20a9
#define PMERR_INV_ORDER_LENGTH		0x20aa
#define PMERR_INV_ORDERING_PARM		0x20ab
#define PMERR_INV_OUTSIDE_DRAW_MODE	0x20ac
#define PMERR_INV_PAGE_VIEWPORT		0x20ad
#define PMERR_INV_PATH_ID		0x20ae
#define PMERR_INV_PATH_MODE		0x20af
#define PMERR_INV_PATTERN_ATTR		0x20b0
#define PMERR_INV_PATTERN_REF_PT_ATTR	0x20b1
#define PMERR_INV_PATTERN_SET_ATTR	0x20b2
#define PMERR_INV_PATTERN_SET_FONT	0x20b3
#define PMERR_INV_PICK_APERTURE_OPTION	0x20b4
#define PMERR_INV_PICK_APERTURE_POSN	0x20b5
#define PMERR_INV_PICK_APERTURE_SIZE	0x20b6
#define PMERR_INV_PICK_NUMBER		0x20b7
#define PMERR_INV_PLAY_METAFILE_OPTION	0x20b8
#define PMERR_INV_PRIMITIVE_TYPE	0x20b9
#define PMERR_INV_PS_SIZE		0x20ba
#define PMERR_INV_PUTDATA_FORMAT	0x20bb
#define PMERR_INV_QUERY_ELEMENT_NO	0x20bc
#define PMERR_INV_RECT			0x20bd
#define PMERR_INV_REGION_CONTROL	0x20be
#define PMERR_INV_REGION_MIX_MODE	0x20bf
#define PMERR_INV_REPLACE_MODE_FUNC	0x20c0
#define PMERR_INV_RESERVED_FIELD	0x20c1
#define PMERR_INV_RESET_OPTIONS		0x20c2
#define PMERR_INV_RGBCOLOR		0x20c3
#define PMERR_INV_SCAN_START		0x20c4
#define PMERR_INV_SEG_ATTR		0x20c5
#define PMERR_INV_SEG_ATTR_VALUE	0x20c6
#define PMERR_INV_SEG_CH_LENGTH		0x20c7
#define PMERR_INV_SEG_NAME		0x20c8
#define PMERR_INV_SEG_OFFSET		0x20c9
#define PMERR_INV_SETID			0x20ca
#define PMERR_INV_SETID_TYPE		0x20cb
#define PMERR_INV_SET_VIEWPORT_OPTION	0x20cc
#define PMERR_INV_SHARPNESS_PARM	0x20cd
#define PMERR_INV_SOURCE_OFFSET		0x20ce
#define PMERR_INV_STOP_DRAW_VALUE	0x20cf
#define PMERR_INV_TRANSFORM_TYPE	0x20d0
#define PMERR_INV_USAGE_PARM		0x20d1
#define PMERR_INV_VIEWING_LIMITS	0x20d2
#define PMERR_JFILE_BUSY		0x20d3
#define PMERR_JNL_FUNC_DATA_TOO_LONG	0x20d4
#define PMERR_KERNING_NOT_SUPPORTED	0x20d5
#define PMERR_LABEL_NOT_FOUND		0x20d6
#define PMERR_MATRIX_OVERFLOW		0x20d7
#define PMERR_METAFILE_INTERNAL_ERROR	0x20d8
#define PMERR_METAFILE_IN_USE		0x20d9
#define PMERR_METAFILE_LIMIT_EXCEEDED	0x20da
#define PMERR_NAME_STACK_FULL		0x20db
#define PMERR_NOT_CREATED_BY_DEVOPENDC	0x20dc
#define PMERR_NOT_IN_AREA		0x20dd
#define PMERR_NOT_IN_DRAW_MODE		0x20de
#define PMERR_NOT_IN_ELEMENT		0x20df
#define PMERR_NOT_IN_IMAGE		0x20e0
#define PMERR_NOT_IN_PATH		0x20e1
#define PMERR_NOT_IN_RETAIN_MODE	0x20e2
#define PMERR_NOT_IN_SEG		0x20e3
#define PMERR_NO_BITMAP_SELECTED	0x20e4
#define PMERR_NO_CURRENT_ELEMENT	0x20e5
#define PMERR_NO_CURRENT_SEG		0x20e6
#define PMERR_NO_METAFILE_RECORD_HANDLE 0x20e7
#define PMERR_ORDER_TOO_BIG		0x20e8
#define PMERR_OTHER_SET_ID_REFS		0x20e9
#define PMERR_OVERRAN_SEG		0x20ea
#define PMERR_OWN_SET_ID_REFS		0x20eb
#define PMERR_PATH_INCOMPLETE		0x20ec
#define PMERR_PATH_LIMIT_EXCEEDED	0x20ed
#define PMERR_PATH_UNKNOWN		0x20ee
#define PMERR_PEL_IS_CLIPPED		0x20ef
#define PMERR_PEL_NOT_AVAILABLE		0x20f0
#define PMERR_PRIMITIVE_STACK_EMPTY	0x20f1
#define PMERR_PROLOG_ERROR		0x20f2
#define PMERR_PROLOG_SEG_ATTR_NOT_SET	0x20f3
#define PMERR_PS_BUSY			0x20f4
#define PMERR_PS_IS_ASSOCIATED		0x20f5
#define PMERR_RAM_JNL_FILE_TOO_SMALL	0x20f6
#define PMERR_REALIZE_NOT_SUPPORTED	0x20f7
#define PMERR_REGION_IS_CLIP_REGION	0x20f8
#define PMERR_RESOURCE_DEPLETION	0x20f9
#define PMERR_SEG_AND_REFSEG_ARE_SAME	0x20fa
#define PMERR_SEG_CALL_RECURSIVE	0x20fb
#define PMERR_SEG_CALL_STACK_EMPTY	0x20fc
#define PMERR_SEG_CALL_STACK_FULL	0x20fd
#define PMERR_SEG_IS_CURRENT		0x20fe
#define PMERR_SEG_NOT_CHAINED		0x20ff
#define PMERR_SEG_NOT_FOUND		0x2100
#define PMERR_SEG_STORE_LIMIT_EXCEEDED	0x2101
#define PMERR_SETID_IN_USE		0x2102
#define PMERR_SETID_NOT_FOUND		0x2103
#define PMERR_STARTDOC_NOT_ISSUED	0x2104
#define PMERR_STOP_DRAW_OCCURRED	0x2105
#define PMERR_TOO_MANY_METAFILES_IN_USE 0x2106
#define PMERR_TRUNCATED_ORDER		0x2107
#define PMERR_UNCHAINED_SEG_ZERO_INV	0x2108
#define PMERR_UNSUPPORTED_ATTR		0x2109
#define PMERR_UNSUPPORTED_ATTR_VALUE	0x210a
#define PMERR_ENDDOC_NOT_ISSUED		0x210b
#define PMERR_PS_NOT_ASSOCIATED		0x210c
#define PMERR_INV_FLOOD_FILL_OPTIONS	0x210d
#define PMERR_INV_FACENAME		0x210e
#define PMERR_PALETTE_SELECTED		0x210f
#define PMERR_NO_PALETTE_SELECTED	0x2110
#define PMERR_INV_HPAL			0x2111
#define PMERR_PALETTE_BUSY		0x2112
#define PMERR_START_POINT_CLIPPED	0x2113
#define PMERR_NO_FILL			0x2114
#define PMERR_INV_FACENAMEDESC		0x2115
#define PMERR_INV_BITMAP_DATA		0x2116
#define PMERR_INV_CHAR_ALIGN_ATTR	0x2117
#define PMERR_INV_HFONT			0x2118
#define PMERR_HFONT_IS_SELECTED		0x2119
#define PMERR_DRVR_NOT_SUPPORTED	0x2120
#define PMERR_INV_INKPS_FUNCTION	0x2121

#endif /* INCL_GPIERRORS */

#if defined (INCL_WPERRORS)

#define WPERR_PROTECTED_CLASS		0x1700
#define WPERR_INVALID_CLASS		0x1701
#define WPERR_INVALID_SUPERCLASS	0x1702
#define WPERR_NO_MEMORY			0x1703
#define WPERR_SEMAPHORE_ERROR		0x1704
#define WPERR_BUFFER_TOO_SMALL		0x1705
#define WPERR_CLSLOADMOD_FAILED		0x1706
#define WPERR_CLSPROCADDR_FAILED	0x1707
#define WPERR_OBJWORD_LOCATION		0x1708
#define WPERR_INVALID_OBJECT		0x1709
#define WPERR_MEMORY_CLEANUP		0x170a
#define WPERR_INVALID_MODULE		0x170b
#define WPERR_INVALID_OLDCLASS		0x170c
#define WPERR_INVALID_NEWCLASS		0x170d
#define WPERR_NOT_IMMEDIATE_CHILD	0x170e
#define WPERR_NOT_WORKPLACE_CLASS	0x170f
#define WPERR_CANT_REPLACE_METACLS	0x1710
#define WPERR_INI_FILE_WRITE		0x1711
#define WPERR_INVALID_FOLDER		0x1712
#define WPERR_BUFFER_OVERFLOW		0x1713
#define WPERR_OBJECT_NOT_FOUND		0x1714
#define WPERR_INVALID_HFIND		0x1715
#define WPERR_INVALID_COUNT		0x1716
#define WPERR_INVALID_BUFFER		0x1717
#define WPERR_ALREADY_EXISTS		0x1718
#define WPERR_INVALID_FLAGS		0x1719
#define WPERR_INVALID_OBJECTID		0x1720
#define WPERR_INVALID_TARGET_OBJECT	0x1721

#endif /* INCL_WPERRORS */

#if defined (INCL_SPLERRORS)

#define PMERR_SPL_DRIVER_ERROR		0x4001
#define PMERR_SPL_DEVICE_ERROR		0x4002
#define PMERR_SPL_DEVICE_NOT_INSTALLED	0x4003
#define PMERR_SPL_QUEUE_ERROR		0x4004
#define PMERR_SPL_INV_HSPL		0x4005
#define PMERR_SPL_NO_DISK_SPACE		0x4006
#define PMERR_SPL_NO_MEMORY		0x4007
#define PMERR_SPL_PRINT_ABORT		0x4008
#define PMERR_SPL_SPOOLER_NOT_INSTALLED 0x4009
#define PMERR_SPL_INV_FORMS_CODE	0x400a
#define PMERR_SPL_INV_PRIORITY		0x400b
#define PMERR_SPL_NO_FREE_JOB_ID	0x400c
#define PMERR_SPL_NO_DATA		0x400d
#define PMERR_SPL_INV_TOKEN		0x400e
#define PMERR_SPL_INV_DATATYPE		0x400f
#define PMERR_SPL_PROCESSOR_ERROR	0x4010
#define PMERR_SPL_INV_JOB_ID		0x4011
#define PMERR_SPL_JOB_NOT_PRINTING	0x4012
#define PMERR_SPL_JOB_PRINTING		0x4013
#define PMERR_SPL_QUEUE_ALREADY_EXISTS	0x4014
#define PMERR_SPL_INV_QUEUE_NAME	0x4015
#define PMERR_SPL_QUEUE_NOT_EMPTY	0x4016
#define PMERR_SPL_DEVICE_ALREADY_EXISTS 0x4017
#define PMERR_SPL_DEVICE_LIMIT_REACHED	0x4018
#define PMERR_SPL_STATUS_STRING_TRUNC	0x4019
#define PMERR_SPL_INV_LENGTH_OR_COUNT	0x401a
#define PMERR_SPL_FILE_NOT_FOUND	0x401b
#define PMERR_SPL_CANNOT_OPEN_FILE	0x401c
#define PMERR_SPL_DRIVER_NOT_INSTALLED	0x401d
#define PMERR_SPL_INV_PROCESSOR_DATTYPE 0x401e
#define PMERR_SPL_INV_DRIVER_DATATYPE	0x401f
#define PMERR_SPL_PROCESSOR_NOT_INST	0x4020
#define PMERR_SPL_NO_SUCH_LOG_ADDRESS	0x4021
#define PMERR_SPL_PRINTER_NOT_FOUND	0x4022
#define PMERR_SPL_DD_NOT_FOUND		0x4023
#define PMERR_SPL_QUEUE_NOT_FOUND	0x4024
#define PMERR_SPL_MANY_QUEUES_ASSOC	0x4025
#define PMERR_SPL_NO_QUEUES_ASSOCIATED	0x4026
#define PMERR_SPL_INI_FILE_ERROR	0x4027
#define PMERR_SPL_NO_DEFAULT_QUEUE	0x4028
#define PMERR_SPL_NO_CURRENT_FORMS_CODE 0x4029
#define PMERR_SPL_NOT_AUTHORISED	0x402a
#define PMERR_SPL_TEMP_NETWORK_ERROR	0x402b
#define PMERR_SPL_HARD_NETWORK_ERROR	0x402c
#define PMERR_DEL_NOT_ALLOWED		0x402d
#define PMERR_CANNOT_DEL_QP_REF		0x402e
#define PMERR_CANNOT_DEL_QNAME_REF	0x402f
#define PMERR_CANNOT_DEL_PRINTER_DD_REF 0x4030
#define PMERR_CANNOT_DEL_PRN_NAME_REF	0x4031
#define PMERR_CANNOT_DEL_PRN_ADDR_REF	0x4032
#define PMERR_SPOOLER_QP_NOT_DEFINED	0x4033
#define PMERR_PRN_NAME_NOT_DEFINED	0x4034
#define PMERR_PRN_ADDR_NOT_DEFINED	0x4035
#define PMERR_PRINTER_DD_NOT_DEFINED	0x4036
#define PMERR_PRINTER_QUEUE_NOT_DEFINED 0x4037
#define PMERR_PRN_ADDR_IN_USE		0x4038
#define PMERR_SPL_TOO_MANY_OPEN_FILES	0x4039
#define PMERR_SPL_CP_NOT_REQD		0x403a
#define PMERR_SPL_PORT_SHUTDOWN		0x403b
#define PMERR_SPL_NOT_HANDLED		0x403c
#define PMERR_SPL_CNV_NOT_INIT		0x403d
#define PMERR_SPL_INIT_IN_PROGRESS	0x403e
#define PMERR_SPL_TYPE_NOT_AVAIL	0x403f
#define PMERR_UNABLE_TO_CLOSE_DEVICE	0x4040
#define PMERR_SPL_SESSION_TERM		0x4041
#define PMERR_SPL_NOT_REGISTERED	0x4042

#endif /* INCL_SPLERRORS */

#if defined (INCL_PICERRORS)

#define PMERR_INV_TYPE			0x5001
#define PMERR_INV_CONV			0x5002
#define PMERR_INV_SEGLEN		0x5003
#define PMERR_DUP_SEGNAME		0x5004
#define PMERR_INV_XFORM			0x5005
#define PMERR_INV_VIEWLIM		0x5006
#define PMERR_INV_3DCOORD		0x5007
#define PMERR_SMB_OVFLOW		0x5008
#define PMERR_SEG_OVFLOW		0x5009
#define PMERR_PIC_DUP_FILENAME		0x5010

#endif /* INCL_PICERRORS */

#if defined (INCL_WINERRORS)

#define WINDBG_HWND_NOT_DESTROYED	0x1022
#define WINDBG_HPTR_NOT_DESTROYED	0x1023
#define WINDBG_HACCEL_NOT_DESTROYED	0x1024
#define WINDBG_HENUM_NOT_DESTROYED	0x1025
#define WINDBG_VISRGN_SEM_BUSY		0x1026
#define WINDBG_USER_SEM_BUSY		0x1027
#define WINDBG_DC_CACHE_BUSY		0x1028
#define WINDBG_HOOK_STILL_INSTALLED	0x1029
#define WINDBG_WINDOW_STILL_LOCKED	0x102a
#define WINDBG_UPDATEPS_ASSERTION_FAIL	0x102b
#define WINDBG_SENDMSG_WITHIN_USER_SEM	0x102c
#define WINDBG_USER_SEM_NOT_ENTERED	0x102d
#define WINDBG_PROC_NOT_EXPORTED	0x102e
#define WINDBG_BAD_SENDMSG_HWND		0x102f
#define WINDBG_ABNORMAL_EXIT		0x1030
#define WINDBG_INTERNAL_REVISION	0x1031
#define WINDBG_INITSYSTEM_FAILED	0x1032
#define WINDBG_HATOMTBL_NOT_DESTROYED	0x1033
#define WINDBG_WINDOW_UNLOCK_WAIT	0x1035

#endif /* INCL_WINERRORS */


#define WRECT	RECTL
#define PWRECT	PRECTL

#define WPOINT	POINTL
#define PWPOINT PPOINTL


typedef LHANDLE HACCEL;

typedef LHANDLE HRGN;
typedef HRGN *PHRGN;

typedef VOID *MRESULT;
typedef MRESULT *PMRESULT;

typedef VOID *MPARAM;
typedef MPARAM *PMPARAM;

typedef LHANDLE HPOINTER;

typedef HMODULE HLIB;
typedef HLIB *PHLIB;

typedef LONG COLOR;
typedef COLOR *PCOLOR;

typedef LHANDLE HAB;
typedef HAB *PHAB;

typedef LHANDLE HPS;
typedef HPS *PHPS;

typedef LHANDLE HDC;
typedef HDC *PHDC;

typedef LHANDLE HWND;
typedef HWND *PHWND;

typedef LHANDLE HMQ;

typedef LHANDLE HPAL;
typedef HPAL *PHPAL;

typedef LHANDLE HBITMAP;
typedef HBITMAP *PHBITMAP;

typedef ULONG ERRORID;
typedef ERRORID *PERRORID;

typedef MRESULT FNWP (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
typedef FNWP *PFNWP;

#define ERRORIDERROR(errid)		(LOUSHORT (errid))
#define ERRORIDSEV(errid)		(HIUSHORT (errid))
#define MAKEERRORID(sev,error)		(ERRORID)(MAKEULONG ((error), (sev)))

typedef struct _POINTL
{
  LONG x;
  LONG y;
} POINTL;
typedef POINTL *PPOINTL;

typedef struct _POINTS
{
  SHORT x;
  SHORT y;
} POINTS;
typedef POINTS *PPOINTS;

typedef struct _RECTL
{
  LONG xLeft;
  LONG yBottom;
  LONG xRight;
  LONG yTop;
} RECTL;
typedef RECTL *PRECTL;

#if defined (INCL_WINMESSAGEMGR) || !defined (INCL_NOCOMMON)

#define WM_NULL				0x0000
#define WM_CREATE			0x0001
#define WM_DESTROY			0x0002
#define WM_ENABLE			0x0004
#define WM_SHOW				0x0005
#define WM_MOVE				0x0006
#define WM_SIZE				0x0007
#define WM_ADJUSTWINDOWPOS		0x0008
#define WM_CALCVALIDRECTS		0x0009
#define WM_SETWINDOWPARAMS		0x000a
#define WM_QUERYWINDOWPARAMS		0x000b
#define WM_HITTEST			0x000c
#define WM_ACTIVATE			0x000d
#define WM_SETFOCUS			0x000f
#define WM_SETSELECTION			0x0010
#define WM_PPAINT			0x0011
#define WM_PSETFOCUS			0x0012
#define WM_PSYSCOLORCHANGE		0x0013
#define WM_PSIZE			0x0014
#define WM_PACTIVATE			0x0015
#define WM_PCONTROL			0x0016
#define WM_COMMAND			0x0020
#define WM_SYSCOMMAND			0x0021
#define WM_HELP				0x0022
#define WM_PAINT			0x0023
#define WM_TIMER			0x0024
#define WM_SEM1				0x0025
#define WM_SEM2				0x0026
#define WM_SEM3				0x0027
#define WM_SEM4				0x0028
#define WM_CLOSE			0x0029
#define WM_QUIT				0x002a
#define WM_SYSCOLORCHANGE		0x002b
#define WM_SYSVALUECHANGED		0x002d
#define WM_APPTERMINATENOTIFY		0x002e
#define WM_PRESPARAMCHANGED		0x002f
#define WM_CONTROL			0x0030
#define WM_VSCROLL			0x0031
#define WM_HSCROLL			0x0032
#define WM_INITMENU			0x0033
#define WM_MENUSELECT			0x0034
#define WM_MENUEND			0x0035
#define WM_DRAWITEM			0x0036
#define WM_MEASUREITEM			0x0037
#define WM_CONTROLPOINTER		0x0038
#define WM_QUERYDLGCODE			0x003a
#define WM_INITDLG			0x003b
#define WM_SUBSTITUTESTRING		0x003c
#define WM_MATCHMNEMONIC		0x003d
#define WM_SAVEAPPLICATION		0x003e

#define WM_HELPBASE			0x0f00
#define WM_HELPTOP			0x0fff

#define WM_USER				0x1000

#define CMDSRC_OTHER			0
#define CMDSRC_PUSHBUTTON		1
#define CMDSRC_MENU			2
#define CMDSRC_ACCELERATOR		3
#define CMDSRC_FONTDLG			4
#define CMDSRC_FILEDLG			5
#define CMDSRC_PRINTDLG			6
#define CMDSRC_COLORDLG			7

#define PM_NOREMOVE			0x0000
#define PM_REMOVE			0x0001

#define RUM_IN				1
#define RUM_OUT				2
#define RUM_INOUT			3

#define SMD_DELAYED			0x0001
#define SMD_IMMEDIATE			0x0002

#define SSM_SYNCHRONOUS			0x0001
#define SSM_ASYNCHRONOUS		0x0002
#define SSM_MIXED			0x0003

#define WLI_NOBUTTONUP			0x0002

#if defined (INCL_WINTYPES)

#define DTYP_USER			16384

#define DTYP_CTL_ARRAY			1
#define DTYP_CTL_PARRAY			(-1)
#define DTYP_CTL_OFFSET			2
#define DTYP_CTL_LENGTH			3

#define DTYP_ACCEL			28
#define DTYP_ACCELTABLE			29
#define DTYP_ARCPARAMS			38
#define DTYP_AREABUNDLE			139
#define DTYP_ATOM			90
#define DTYP_BITMAPINFO			60
#define DTYP_BITMAPINFOHEADER		61
#define DTYP_BITMAPINFO2		170
#define DTYP_BITMAPINFOHEADER2		171
#define DTYP_BIT16			20
#define DTYP_BIT32			21
#define DTYP_BIT8			19
#define DTYP_BOOL			18
#define DTYP_BTNCDATA			35
#define DTYP_BYTE			13
#define DTYP_CATCHBUF			141
#define DTYP_CHAR			15
#define DTYP_CHARBUNDLE			135
#define DTYP_CLASSINFO			95
#define DTYP_COUNT2			93
#define DTYP_COUNT2B			70
#define DTYP_COUNT2CH			82
#define DTYP_COUNT4			152
#define DTYP_COUNT4B			42
#define DTYP_CPID			57
#define DTYP_CREATESTRUCT		98
#define DTYP_CURSORINFO			34
#define DTYP_DEVOPENSTRUC		124
#define DTYP_DLGTEMPLATE		96
#define DTYP_DLGTITEM			97
#define DTYP_ENTRYFDATA			127
#define DTYP_ERRORID			45
#define DTYP_FATTRS			75
#define DTYP_FFDESCS			142
#define DTYP_FIXED			99
#define DTYP_FONTMETRICS		74
#define DTYP_FRAMECDATA			144
#define DTYP_GRADIENTL			48
#define DTYP_HAB			10
#define DTYP_HACCEL			30
#define DTYP_HAPP			146
#define DTYP_HATOMTBL			91
#define DTYP_HBITMAP			62
#define DTYP_HCINFO			46
#define DTYP_HDC			132
#define DTYP_HENUM			117
#define DTYP_HHEAP			109
#define DTYP_HINI			53
#define DTYP_HLIB			147
#define DTYP_HMF			85
#define DTYP_HMQ			86
#define DTYP_HPOINTER			106
#define DTYP_HPROGRAM			131
#define DTYP_HPS			12
#define DTYP_HRGN			116
#define DTYP_HSEM			140
#define DTYP_HSPL			32
#define DTYP_HSWITCH			66
#define DTYP_HVPS			58
#define DTYP_HWND			11
#define DTYP_IDENTITY			133
#define DTYP_IDENTITY4			169
#define DTYP_IMAGEBUNDLE		136
#define DTYP_INDEX2			81
#define DTYP_IPT			155
#define DTYP_KERNINGPAIRS		118
#define DTYP_LENGTH2			68
#define DTYP_LENGTH4			69
#define DTYP_LINEBUNDLE			137
#define DTYP_LONG			25
#define DTYP_MARKERBUNDLE		138
#define DTYP_MATRIXLF			113
#define DTYP_MLECTLDATA			161
#define DTYP_MLEMARGSTRUCT		157
#define DTYP_MLEOVERFLOW		158
#define DTYP_OFFSET2B			112
#define DTYP_OWNERITEM			154
#define DTYP_PID			92
#define DTYP_PIX			156
#define DTYP_POINTERINFO		105
#define DTYP_POINTL			77
#define DTYP_PROGCATEGORY		129
#define DTYP_PROGRAMENTRY		128
#define DTYP_PROGTYPE			130
#define DTYP_PROPERTY2			88
#define DTYP_PROPERTY4			89
#define DTYP_QMSG			87
#define DTYP_RECTL			121
#define DTYP_RESID			125
#define DTYP_RGB			111
#define DTYP_RGNRECT			115
#define DTYP_SBCDATA			159
#define DTYP_SEGOFF			126
#define DTYP_SHORT			23
#define DTYP_SIZEF			101
#define DTYP_SIZEL			102
#define DTYP_STRL			17
#define DTYP_STR16			40
#define DTYP_STR32			37
#define DTYP_STR64			47
#define DTYP_STR8			33
#define DTYP_SWBLOCK			63
#define DTYP_SWCNTRL			64
#define DTYP_SWENTRY			65
#define DTYP_SWP			31
#define DTYP_TID			104
#define DTYP_TIME			107
#define DTYP_TRACKINFO			73
#define DTYP_UCHAR			22
#define DTYP_ULONG			26
#define DTYP_USERBUTTON			36
#define DTYP_USHORT			24
#define DTYP_WIDTH4			108
#define DTYP_WNDPARAMS			83
#define DTYP_WNDPROC			84
#define DTYP_WPOINT			59
#define DTYP_WRECT			55
#define DTYP_XYWINSIZE			52

#define DTYP_PACCEL			(-28)
#define DTYP_PACCELTABLE		(-29)
#define DTYP_PARCPARAMS			(-38)
#define DTYP_PAREABUNDLE		(-139)
#define DTYP_PATOM			(-90)
#define DTYP_PBITMAPINFO		(-60)
#define DTYP_PBITMAPINFOHEADER		(-61)
#define DTYP_PBITMAPINFO2		(-170)
#define DTYP_PBITMAPINFOHEADER2		(-171)
#define DTYP_PBIT16			(-20)
#define DTYP_PBIT32			(-21)
#define DTYP_PBIT8			(-19)
#define DTYP_PBOOL			(-18)
#define DTYP_PBTNCDATA			(-35)
#define DTYP_PBYTE			(-13)
#define DTYP_PCATCHBUF			(-141)
#define DTYP_PCHAR			(-15)
#define DTYP_PCHARBUNDLE		(-135)
#define DTYP_PCLASSINFO			(-95)
#define DTYP_PCOUNT2			(-93)
#define DTYP_PCOUNT2B			(-70)
#define DTYP_PCOUNT2CH			(-82)
#define DTYP_PCOUNT4			(-152)
#define DTYP_PCOUNT4B			(-42)
#define DTYP_PCPID			(-57)
#define DTYP_PCREATESTRUCT		(-98)
#define DTYP_PCURSORINFO		(-34)
#define DTYP_PDEVOPENSTRUC		(-124)
#define DTYP_PDLGTEMPLATE		(-96)
#define DTYP_PDLGTITEM			(-97)
#define DTYP_PENTRYFDATA		(-127)
#define DTYP_PERRORID			(-45)
#define DTYP_PFATTRS			(-75)
#define DTYP_PFFDESCS			(-142)
#define DTYP_PFIXED			(-99)
#define DTYP_PFONTMETRICS		(-74)
#define DTYP_PFRAMECDATA		(-144)
#define DTYP_PGRADIENTL			(-48)
#define DTYP_PHAB			(-10)
#define DTYP_PHACCEL			(-30)
#define DTYP_PHAPP			(-146)
#define DTYP_PHATOMTBL			(-91)
#define DTYP_PHBITMAP			(-62)
#define DTYP_PHCINFO			(-46)
#define DTYP_PHDC			(-132)
#define DTYP_PHENUM			(-117)
#define DTYP_PHHEAP			(-109)
#define DTYP_PHINI			(-53)
#define DTYP_PHLIB			(-147)
#define DTYP_PHMF			(-85)
#define DTYP_PHMQ			(-86)
#define DTYP_PHPOINTER			(-106)
#define DTYP_PHPROGRAM			(-131)
#define DTYP_PHPS			(-12)
#define DTYP_PHRGN			(-116)
#define DTYP_PHSEM			(-140)
#define DTYP_PHSPL			(-32)
#define DTYP_PHSWITCH			(-66)
#define DTYP_PHVPS			(-58)
#define DTYP_PHWND			(-11)
#define DTYP_PIDENTITY			(-133)
#define DTYP_PIDENTITY4			(-169)
#define DTYP_PIMAGEBUNDLE		(-136)
#define DTYP_PINDEX2			(-81)
#define DTYP_PIPT			(-155)
#define DTYP_PKERNINGPAIRS		(-118)
#define DTYP_PLENGTH2			(-68)
#define DTYP_PLENGTH4			(-69)
#define DTYP_PLINEBUNDLE		(-137)
#define DTYP_PLONG			(-25)
#define DTYP_PMARKERBUNDLE		(-138)
#define DTYP_PMATRIXLF			(-113)
#define DTYP_PMLECTLDATA		(-161)
#define DTYP_PMLEMARGSTRUCT		(-157)
#define DTYP_PMLEOVERFLOW		(-158)
#define DTYP_POFFSET2B			(-112)
#define DTYP_POWNERITEM			(-154)
#define DTYP_PPID			(-92)
#define DTYP_PPIX			(-156)
#define DTYP_PPOINTERINFO		(-105)
#define DTYP_PPOINTL			(-77)
#define DTYP_PPROGCATEGORY		(-129)
#define DTYP_PPROGRAMENTRY		(-128)
#define DTYP_PPROGTYPE			(-130)
#define DTYP_PPROPERTY2			(-88)
#define DTYP_PPROPERTY4			(-89)
#define DTYP_PQMSG			(-87)
#define DTYP_PRECTL			(-121)
#define DTYP_PRESID			(-125)
#define DTYP_PRGB			(-111)
#define DTYP_PRGNRECT			(-115)
#define DTYP_PSBCDATA			(-159)
#define DTYP_PSEGOFF			(-126)
#define DTYP_PSHORT			(-23)
#define DTYP_PSIZEF			(-101)
#define DTYP_PSIZEL			(-102)
#define DTYP_PSTRL			(-17)
#define DTYP_PSTR16			(-40)
#define DTYP_PSTR32			(-37)
#define DTYP_PSTR64			(-47)
#define DTYP_PSTR8			(-33)
#define DTYP_PSWBLOCK			(-63)
#define DTYP_PSWCNTRL			(-64)
#define DTYP_PSWENTRY			(-65)
#define DTYP_PSWP			(-31)
#define DTYP_PTID			(-104)
#define DTYP_PTIME			(-107)
#define DTYP_PTRACKINFO			(-73)
#define DTYP_PUCHAR			(-22)
#define DTYP_PULONG			(-26)
#define DTYP_PUSERBUTTON		(-36)
#define DTYP_PUSHORT			(-24)
#define DTYP_PWIDTH4			(-108)
#define DTYP_PWNDPARAMS			(-83)
#define DTYP_PWNDPROC			(-84)
#define DTYP_PWPOINT			(-59)
#define DTYP_PWRECT			(-55)
#define DTYP_PXYWINSIZE			(-52)

#endif /* INCL_WINTYPES */

typedef struct _QMSG
{
  HWND	 hwnd;
  ULONG	 msg;
  MPARAM mp1;
  MPARAM mp2;
  ULONG	 time;
  POINTL ptl;
  ULONG	 reserved;
} QMSG;
typedef QMSG *PQMSG;

typedef struct _COMMANDMSG
{
  USHORT cmd;
  USHORT unused;
  USHORT source;
  USHORT fMouse;
} CMDMSG;
typedef CMDMSG *PCMDMSG;

typedef struct _MQINFO
{
  ULONG cb;
  PID	pid;
  TID	tid;
  ULONG cmsgs;
  PVOID pReserved;
} MQINFO;
typedef MQINFO *PMQINFO;

#define COMMANDMSG(pmsg)	((PCMDMSG)((PBYTE)pmsg + sizeof (ULONG)))


BOOL WinCancelShutdown (HMQ hmq, BOOL fCancelAlways);
HMQ WinCreateMsgQueue (HAB hab, LONG cmsg);
BOOL WinDestroyMsgQueue (HMQ hmq);
MRESULT WinDispatchMsg (HAB hab, PQMSG pqmsg);
BOOL WinGetMsg (HAB hab, PQMSG pqmsg, HWND hwndFilter, ULONG msgFilterFirst,
    ULONG msgFilterLast);
BOOL WinLockInput (HMQ hmq, ULONG fLock);
BOOL WinPeekMsg (HAB hab, PQMSG pqmsg, HWND hwndFilter, ULONG msgFilterFirst,
    ULONG msgFilterLast, ULONG fl);
BOOL WinPostMsg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
HMQ WinQueueFromID (HAB hab, PID pid, TID tid);
BOOL WinQueryQueueInfo (HMQ hmq, PMQINFO pmqi, ULONG cbCopy);
HMQ WinQuerySendMsg (HAB hab, HMQ hmqSender, HMQ hmqReceiver, PQMSG pqmsg);
BOOL WinRegisterUserDatatype (HAB hab, LONG datatype, LONG count, PLONG types);
BOOL WinRegisterUserMsg (HAB hab, ULONG msgid, LONG datatype1, LONG dir1,
    LONG datatype2, LONG dir2, LONG datatyper);
BOOL WinReplyMsg (HAB hab, HMQ hmqSender, HMQ hmqReceiver, MRESULT mresult);
MRESULT WinSendMsg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
BOOL WinSetMsgMode (HAB hab, PCSZ classname, LONG control);
BOOL WinSetSynchroMode (HAB hab, LONG mode);
BOOL WinThreadAssocQueue (HAB hab, HMQ hmq);
BOOL WinWakeThread (HMQ hmq);

#endif /* INCL_WINMESSAGEMGR || !INCL_NOCOMMON */

typedef struct _SWP
{
  ULONG fl;
  LONG	cy;
  LONG	cx;
  LONG	y;
  LONG	x;
  HWND	hwndInsertBehind;
  HWND	hwnd;
  ULONG ulReserved1;
  ULONG ulReserved2;
} SWP;
typedef SWP *PSWP;

typedef struct _ICONINFO
{
   ULONG   cb;
   ULONG   fFormat;
   PSZ	   pszFileName;
   HMODULE hmod;
   ULONG   resid;
   ULONG   cbIconData;
   PVOID   pIconData;
} ICONINFO;
typedef ICONINFO *PICONINFO;


#define MPVOID			((MPARAM)0)

#define MPFROMP(x)		((MPARAM)((ULONG)(x)))
#define MPFROMHWND(x)		((MPARAM)(HWND)(x))
#define MPFROMCHAR(x)		((MPARAM)(ULONG)(USHORT)(x))
#define MPFROMSHORT(x)		((MPARAM)(ULONG)(USHORT)(x))
#define MPFROM2SHORT(x1,x2)	((MPARAM)MAKELONG (x1, x2))
#define MPFROMSH2CH(s,c1,c2)	((MPARAM)MAKELONG (s, MAKESHORT (c1, c2)))
#define MPFROMLONG(x)		((MPARAM)(ULONG)(x))

#define PVOIDFROMMP(mp)		((PVOID)(mp))
#define HWNDFROMMP(mp)		((HWND)(mp))
#define CHAR1FROMMP(mp)		((UCHAR)(ULONG)(mp))
#define CHAR2FROMMP(mp)		((UCHAR)((ULONG)mp >> 8))
#define CHAR3FROMMP(mp)		((UCHAR)((ULONG)mp >> 16))
#define CHAR4FROMMP(mp)		((UCHAR)((ULONG)mp >> 24))
#define SHORT1FROMMP(mp)	((USHORT)(ULONG)(mp))
#define SHORT2FROMMP(mp)	((USHORT)((ULONG)mp >> 16))
#define LONGFROMMP(mp)		((ULONG)(mp))

#define MRFROMP(x)		((MRESULT)(PVOID)(x))
#define MRFROMSHORT(x)		((MRESULT)(ULONG)(USHORT)(x))
#define MRFROM2SHORT(x1,x2)	((MRESULT)MAKELONG (x1, x2))
#define MRFROMLONG(x)		((MRESULT)(ULONG)(x))

#define PVOIDFROMMR(mr)		((VOID *)(mr))
#define SHORT1FROMMR(mr)	((USHORT)((ULONG)mr))
#define SHORT2FROMMR(mr)	((USHORT)((ULONG)mr >> 16))
#define LONGFROMMR(mr)		((ULONG)(mr))


HWND WinCreateWindow (HWND hwndParent, PCSZ pszClass, PCSZ pszName,
    ULONG flStyle, LONG x, LONG y, LONG cx, LONG cy, HWND hwndOwner,
    HWND hwndInsertBehind, ULONG id, PVOID pCtlData, PVOID pPresParams);
BOOL WinDrawBitmap (HPS hpsDst, HBITMAP hbm, __const__ RECTL *pwrcSrc,
    __const__ POINTL *pptlDst, LONG clrFore, LONG clrBack, ULONG fl);
BOOL WinDrawBorder (HPS hps, __const__ RECTL *prcl, LONG cx, LONG cy,
    LONG clrFore, LONG clrBack, ULONG flCmd);
LONG WinDrawText (HPS hps, LONG cchText, PCCH lpchText, PRECTL prcl,
    LONG clrFore, LONG clrBack, ULONG flCmd);
BOOL WinEnableWindow (HWND hwnd, BOOL fEnable);
BOOL WinEnableWindowUpdate (HWND hwnd, BOOL fEnable);
BOOL WinInvalidateRect (HWND hwnd, __const__ RECTL *prcl,
    BOOL fIncludeChildren);
BOOL WinInvalidateRegion (HWND hwnd, HRGN hrgn, BOOL fIncludeChildren);
BOOL WinInvertRect (HPS hps, __const__ RECTL *prcl);
BOOL WinIsChild (HWND hwnd, HWND hwndParent);
BOOL WinIsWindow (HAB hab, HWND hwnd);
BOOL WinIsWindowEnabled (HWND hwnd);
BOOL WinIsWindowVisible (HWND hwnd);
LONG WinLoadMessage (HAB hab, HMODULE hmod, ULONG id, LONG cchMax,
    PSZ pchBuffer);
LONG WinLoadString (HAB hab, HMODULE hmod, ULONG id, LONG cchMax,
    PSZ pchBuffer);
LONG WinMultWindowFromIDs (HWND hwndParent, PHWND prghwnd, ULONG idFirst,
    ULONG idLast);
HWND WinQueryDesktopWindow (HAB hab, HDC hdc);
HWND WinQueryObjectWindow (HWND hwndDesktop);
HPOINTER WinQueryPointer (HWND hwndDesktop);
HWND WinQueryWindow (HWND hwnd, LONG cmd);
BOOL WinQueryWindowPos (HWND hwnd, PSWP pswp);
BOOL WinQueryWindowProcess (HWND hwnd, PPID ppid, PTID ptid);
LONG WinQueryWindowText (HWND hwnd, LONG cchBufferMax, PCH pchBuffer);
LONG WinQueryWindowTextLength (HWND hwnd);
BOOL WinSetMultWindowPos (HAB hab, __const__ SWP *pswp, ULONG cswp);
BOOL WinSetOwner (HWND hwnd, HWND hwndNewOwner);
BOOL WinSetParent (HWND hwnd, HWND hwndNewParent, BOOL fRedraw);
BOOL WinSetWindowPos (HWND hwnd, HWND hwndInsertBehind, LONG x, LONG y,
    LONG cx, LONG cy, ULONG fl);
BOOL WinSetWindowText (HWND hwnd, PCSZ pszText);
BOOL WinUpdateWindow (HWND hwnd);
HWND WinWindowFromID (HWND hwndParent, ULONG id);


#if defined (INCL_WINFRAMEMGR) || !defined (INCL_NOCOMMON)

#define FCF_TITLEBAR			0x00000001
#define FCF_SYSMENU			0x00000002
#define FCF_MENU			0x00000004
#define FCF_SIZEBORDER			0x00000008
#define FCF_MINBUTTON			0x00000010
#define FCF_MAXBUTTON			0x00000020
#define FCF_MINMAX			(FCF_MINBUTTON|FCF_MAXBUTTON)
#define FCF_VERTSCROLL			0x00000040
#define FCF_HORZSCROLL			0x00000080
#define FCF_DLGBORDER			0x00000100
#define FCF_BORDER			0x00000200
#define FCF_SHELLPOSITION		0x00000400
#define FCF_TASKLIST			0x00000800
#define FCF_NOBYTEALIGN			0x00001000
#define FCF_NOMOVEWITHOWNER		0x00002000
#define FCF_ICON			0x00004000
#define FCF_ACCELTABLE			0x00008000
#define FCF_SYSMODAL			0x00010000
#define FCF_SCREENALIGN			0x00020000
#define FCF_MOUSEALIGN			0x00040000
#define FCF_PALETTE_NORMAL		0x00080000
#define FCF_PALETTE_HELP		0x00100000
#define FCF_PALETTE_POPUPODD		0x00200000
#define FCF_PALETTE_POPUPEVEN		0x00400000
#define FCF_HIDEBUTTON			0x01000000
#define FCF_HIDEMAX			0x01000020
#define FCF_AUTOICON			0x40000000
#if defined (INCL_NLS)
#define FCF_DBE_APPSTAT			0x80000000
#endif /* INCL_NLS */

#define FCF_STANDARD			0x0000cc3f

#define FF_FLASHWINDOW			0x0001
#define FF_ACTIVE			0x0002
#define FF_FLASHHILITE			0x0004
#define FF_OWNERHIDDEN			0x0008
#define FF_DLGDISMISSED			0x0010
#define FF_OWNERDISABLED		0x0020
#define FF_SELECTED			0x0040
#define FF_NOACTIVATESWP		0x0080
#define FF_DIALOGBOX			0x0100

#define FS_ICON				0x00000001
#define FS_ACCELTABLE			0x00000002
#define FS_SHELLPOSITION		0x00000004
#define FS_TASKLIST			0x00000008
#define FS_NOBYTEALIGN			0x00000010
#define FS_NOMOVEWITHOWNER		0x00000020
#define FS_SYSMODAL			0x00000040
#define FS_DLGBORDER			0x00000080
#define FS_BORDER			0x00000100
#define FS_SCREENALIGN			0x00000200
#define FS_MOUSEALIGN			0x00000400
#define FS_SIZEBORDER			0x00000800
#define FS_AUTOICON			0x00001000
#if defined (INCL_NLS)
#define FS_DBE_APPSTAT			0x00008000
#endif /* INCL_NLS */

#define FS_STANDARD			0x0000000f

typedef struct _FRAMECDATA
{
  USHORT cb;
  ULONG	 flCreateFlags;
  USHORT hmodResources;
  USHORT idResources;
} FRAMECDATA;
typedef FRAMECDATA *PFRAMECDATA;

HWND WinCreateStdWindow (HWND hwndParent, ULONG flStyle,
    PULONG pflCreateFlags, PCSZ pszClientClass, PCSZ pszTitle,
    ULONG styleClient, HMODULE hmod, ULONG idResources, PHWND phwndClient);

#endif /* INCL_WINFRAMEMGR || !INCL_NOCOMMON */

#if defined (INCL_WINFRAMEMGR)

#define WM_FLASHWINDOW			0x0040
#define WM_FORMATFRAME			0x0041
#define WM_UPDATEFRAME			0x0042
#define WM_FOCUSCHANGE			0x0043
#define WM_SETBORDERSIZE		0x0044
#define WM_TRACKFRAME			0x0045
#define WM_MINMAXFRAME			0x0046
#define WM_SETICON			0x0047
#define WM_QUERYICON			0x0048
#define WM_SETACCELTABLE		0x0049
#define WM_QUERYACCELTABLE		0x004a
#define WM_TRANSLATEACCEL		0x004b
#define WM_QUERYTRACKINFO		0x004c
#define WM_QUERYBORDERSIZE		0x004d
#define WM_NEXTMENU			0x004e
#define WM_ERASEBACKGROUND		0x004f
#define WM_QUERYFRAMEINFO		0x0050
#define WM_QUERYFOCUSCHAIN		0x0051
#define WM_OWNERPOSCHANGE		0x0052
#define WM_CALCFRAMERECT		0x0053
#define WM_WINDOWPOSCHANGED		0x0055
#define WM_ADJUSTFRAMEPOS		0x0056
#define WM_QUERYFRAMECTLCOUNT		0x0059
#define WM_QUERYHELPINFO		0x005b
#define WM_SETHELPINFO			0x005c
#define WM_ERROR			0x005d
#define WM_REALIZEPALETTE		0x005e

#define FI_FRAME			0x00000001
#define FI_OWNERHIDE			0x00000002
#define FI_ACTIVATEOK			0x00000004
#define FI_NOMOVEWITHOWNER		0x00000008

#define FID_SYSMENU			0x8002
#define FID_TITLEBAR			0x8003
#define FID_MINMAX			0x8004
#define FID_MENU			0x8005
#define FID_VERTSCROLL			0x8006
#define FID_HORZSCROLL			0x8007
#define FID_CLIENT			0x8008
#define FID_DBE_APPSTAT			0x8010
#define FID_DBE_KBDSTAT			0x8011
#define FID_DBE_PECIC			0x8012
#define FID_DBE_KKPOPUP			0x8013

#define SC_SIZE				0x8000
#define SC_MOVE				0x8001
#define SC_MINIMIZE			0x8002
#define SC_MAXIMIZE			0x8003
#define SC_CLOSE			0x8004
#define SC_NEXT				0x8005
#define SC_APPMENU			0x8006
#define SC_SYSMENU			0x8007
#define SC_RESTORE			0x8008
#define SC_NEXTFRAME			0x8009
#define SC_NEXTWINDOW			0x8010
#define SC_TASKMANAGER			0x8011
#define SC_HELPKEYS			0x8012
#define SC_HELPINDEX			0x8013
#define SC_HELPEXTENDED			0x8014
#define SC_SWITCHPANELIDS		0x8015
#define SC_DBE_FIRST			0x8018
#define SC_DBE_LAST			0x801f
#define SC_BEGINDRAG			0x8020
#define SC_ENDDRAG			0x8021
#define SC_SELECT			0x8022
#define SC_OPEN				0x8023
#define SC_CONTEXTMENU			0x8024
#define SC_CONTEXTHELP			0x8025
#define SC_TEXTEDIT			0x8026
#define SC_BEGINSELECT			0x8027
#define SC_ENDSELECT			0x8028
#define SC_WINDOW			0x8029
#define SC_HIDE				0x802a

typedef LHANDLE HSAVEWP;

BOOL WinCalcFrameRect (HWND hwndFrame, PRECTL prcl, BOOL fClient);
BOOL WinCreateFrameControls (HWND hwndFrame, PFRAMECDATA pfcdata,
    PCSZ pszTitle);
BOOL WinFlashWindow (HWND hwndFrame, BOOL fFlash);
BOOL WinGetMaxPosition (HWND hwnd, PSWP pswp);
BOOL WinGetMinPosition (HWND hwnd, PSWP pswp, __const__ POINTL *pptl);
BOOL WinSaveWindowPos (HSAVEWP hsvwp, PSWP pswp, ULONG cswp);

#endif /* INCL_WINFRAMEMGR */


#if defined (INCL_WINWINDOWMGR) || !defined (INCL_NOCOMMON)

#define PSF_LOCKWINDOWUPDATE		0x0001
#define PSF_CLIPUPWARDS			0x0002
#define PSF_CLIPDOWNWARDS		0x0004
#define PSF_CLIPSIBLINGS		0x0008
#define PSF_CLIPCHILDREN		0x0010
#define PSF_PARENTCLIP			0x0020

#define QV_OS2				0x0000
#define QV_CMS				0x0001
#define QV_TSO				0x0002
#define QV_TSOBATCH			0x0003
#define QV_OS400			0x0004

#define SW_SCROLLCHILDREN		0x0001
#define SW_INVALIDATERGN		0x0002


typedef struct _QVERSDATA
{
  USHORT environment;
  USHORT version;
} QVERSDATA;
typedef QVERSDATA *PQVERSDATA;


HPS WinBeginPaint (HWND hwnd, HPS hps, PRECTL prclPaint);
MRESULT WinDefWindowProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
BOOL WinDestroyWindow (HWND hwnd);
BOOL WinEndPaint (HPS hps);
BOOL WinFillRect (HPS hps, __const__ RECTL *prcl, LONG lColor);
HPS WinGetClipPS (HWND hwnd, HWND hwndClip, ULONG fl);
HPS WinGetPS (HWND hwnd);
HAB WinInitialize (ULONG fsOptions);
BOOL WinIsWindowShowing (HWND hwnd);
HDC WinOpenWindowDC (HWND hwnd);
HAB WinQueryAnchorBlock (HWND hwnd);
ULONG WinQueryVersion (HAB hab);
BOOL WinQueryWindowRect (HWND hwnd, PRECTL prclDest);
BOOL WinRegisterClass (HAB hab, PCSZ pszClassName, PFNWP pfnWndProc,
    ULONG flStyle, ULONG cbWindowData);
BOOL WinReleasePS (HPS hps);
LONG WinScrollWindow (HWND hwnd, LONG dx, LONG dy, __const__ RECTL *prclScroll,
    __const__ RECTL *prclClip, HRGN hrgnUpdate, PRECTL prclUpdate,
    ULONG rgfsw);
BOOL WinSetActiveWindow (HWND hwndDesktop, HWND hwnd);
BOOL WinShowWindow (HWND hwnd, BOOL fShow);
BOOL WinTerminate (HAB hab);

#endif /* INCL_WINWINDOWMGR || !INCL_NOCOMMON */

#if defined (INCL_WINWINDOWMGR)

#define WM_QUERYCONVERTPOS		0x00b0

#define QCP_CONVERT			0x0001
#define QCP_NOCONVERT			0x0000

#define QWS_USER			0
#define QWS_ID				(-1)
#define QWS_MIN				(-1)

#define QWL_USER			0
#define QWL_STYLE			(-2)
#define QWP_PFNWP			(-3)
#define QWL_HMQ				(-4)
#define QWL_RESERVED			(-5)
#define QWL_PENDATA			(-7)
#define QWL_BD_ATTR			(-9)
#define QWL_BD_STAT			(-10)
#define QWL_KBDLAYER			(-11)
#define QWL_MIN				(-11)

#define QWL_HHEAP			0x0004
#define QWL_HWNDFOCUSSAVE		0x0018
#define QWL_DEFBUTTON			0x0040
#define QWL_PSSCBLK			0x0048
#define QWL_PFEPBLK			0x004c
#define QWL_PSTATBLK			0x0050

#define QWS_FLAGS			0x0008
#define QWS_RESULT			0x000a
#define QWS_XRESTORE			0x000c
#define QWS_YRESTORE			0x000e
#define QWS_CXRESTORE			0x0010
#define QWS_CYRESTORE			0x0012
#define QWS_XMINIMIZE			0x0014
#define QWS_YMINIMIZE			0x0016


typedef struct _CLASSINFO
{
  ULONG flClassStyle;
  PFNWP pfnWindowProc;
  ULONG cbWindowData;
} CLASSINFO;
typedef CLASSINFO *PCLASSINFO;

typedef struct _CREATESTRUCT
{
  PVOID pPresParams;
  PVOID pCtlData;
  ULONG id;
  HWND	hwndInsertBehind;
  HWND	hwndOwner;
  LONG	cy;
  LONG	cx;
  LONG	y;
  LONG	x;
  ULONG flStyle;
  PSZ	pszText;
  PSZ	pszClass;
  HWND	hwndParent;
} CREATESTRUCT;
typedef CREATESTRUCT *PCREATESTRUCT;

typedef LHANDLE HENUM;


HENUM WinBeginEnumWindows (HWND hwnd);
BOOL WinEndEnumWindows (HENUM henum);
LONG WinExcludeUpdateRegion (HPS hps, HWND hwnd);
HWND WinGetNextWindow (HENUM henum);
HPS WinGetScreenPS (HWND hwndDesktop);
BOOL WinIsThreadActive (HAB hab);
BOOL WinLockVisRegions (HWND hwndDesktop, BOOL fLock);
BOOL WinLockWindowUpdate (HWND hwndDesktop, HWND hwndLockUpdate);
BOOL WinMapWindowPoints (HWND hwndFrom, HWND hwndTo, PPOINTL prgptl,
    LONG cwpt);
HWND WinQueryActiveWindow (HWND hwndDesktop);
BOOL WinQueryClassInfo (HAB hab, PCSZ pszClassName, PCLASSINFO pClassInfo);
LONG WinQueryClassName (HWND hwnd, LONG cchMax, PCH pch);
BOOL WinQueryUpdateRect (HWND hwnd, PRECTL prcl);
LONG WinQueryUpdateRegion (HWND hwnd, HRGN hrgn);
HWND WinQuerySysModalWindow (HWND hwndDesktop);
HDC WinQueryWindowDC (HWND hwnd);
PVOID WinQueryWindowPtr (HWND hwnd, LONG index);
ULONG WinQueryWindowULong (HWND hwnd, LONG index);
USHORT WinQueryWindowUShort (HWND hwnd, LONG index);
BOOL WinSetSysModalWindow (HWND hwndDesktop, HWND hwnd);
BOOL WinSetWindowBits (HWND hwnd, LONG index, ULONG flData, ULONG flMask);
BOOL WinSetWindowPtr (HWND hwnd, LONG index, PVOID p);
BOOL WinSetWindowULong (HWND hwnd, LONG index, ULONG ul);
BOOL WinSetWindowUShort (HWND hwnd, LONG index, USHORT us);
PFNWP WinSubclassWindow (HWND hwnd, PFNWP pfnwp);
BOOL WinValidateRect (HWND hwnd, __const__ RECTL *prcl, BOOL fIncludeChildren);
BOOL WinValidateRegion (HWND hwnd, HRGN hrgn, BOOL fIncludeChildren);
HWND WinWindowFromDC (HDC hdc);
HWND WinWindowFromPoint (HWND hwnd, __const__ POINTL *pptl, BOOL fChildren);

#endif /* INCL_WINWINDOWMGR */


#if defined (INCL_WINACCELERATORS)

#define AF_CHAR				0x0001
#define AF_VIRTUALKEY			0x0002
#define AF_SCANCODE			0x0004
#define AF_SHIFT			0x0008
#define AF_CONTROL			0x0010
#define AF_ALT				0x0020
#define AF_LONEKEY			0x0040
#define AF_SYSCOMMAND			0x0100
#define AF_HELP				0x0200

typedef struct _ACCEL
{
  USHORT fs;
  USHORT key;
  USHORT cmd;
} ACCEL;
typedef ACCEL *PACCEL;

typedef struct _ACCELTABLE
{
  USHORT cAccel;
  USHORT codepage;
  ACCEL	 aaccel[1];
} ACCELTABLE;
typedef ACCELTABLE *PACCELTABLE;

ULONG WinCopyAccelTable (HACCEL haccel, PACCELTABLE pAccelTable,
    ULONG cbCopyMax);
HACCEL WinCreateAccelTable (HAB hab, PACCELTABLE pAccelTable);
BOOL WinDestroyAccelTable (HACCEL haccel);
HACCEL WinLoadAccelTable (HAB hab, HMODULE hmod, ULONG idAccelTable);
HACCEL WinQueryAccelTable (HAB hab, HWND hwndFrame);
BOOL WinSetAccelTable (HAB hab, HACCEL haccel, HWND hwndFrame);
BOOL WinTranslateAccel (HAB hab, HWND hwnd, HACCEL haccel, PQMSG pqmsg);

#endif /* INCL_WINACCELERATORS */


#if defined (INCL_WINATOM)

typedef LHANDLE HATOMTBL;
typedef ULONG ATOM;

#define MAKEINTATOM(x)		((PCH)MAKEULONG (x, 0xffff))

ATOM WinAddAtom (HATOMTBL hAtomTbl, PCSZ pszAtomName);
HATOMTBL WinCreateAtomTable (ULONG cbInitial, ULONG cBuckets);
ATOM WinDeleteAtom (HATOMTBL hAtomTbl, ATOM atom);
HATOMTBL WinDestroyAtomTable (HATOMTBL hAtomTbl);
ATOM WinFindAtom (HATOMTBL hAtomTbl, PCSZ pszAtomName);
ULONG WinQueryAtomLength (HATOMTBL hAtomTbl, ATOM atom);
ULONG WinQueryAtomName (HATOMTBL hAtomTbl, ATOM atom, PSZ pchBuffer,
    ULONG cchBufferMax);
ULONG WinQueryAtomUsage (HATOMTBL hAtomTbl, ATOM atom);
HATOMTBL WinQuerySystemAtomTable (VOID);

#endif /* INCL_WINATOM */


#if defined (INCL_WINBUTTONS)

#define BDS_HILITED			0x0100
#define BDS_DISABLED			0x0200
#define BDS_DEFAULT			0x0400

#define BM_CLICK			0x0120
#define BM_QUERYCHECKINDEX		0x0121
#define BM_QUERYHILITE			0x0122
#define BM_SETHILITE			0x0123
#define BM_QUERYCHECK			0x0124
#define BM_SETCHECK			0x0125
#define BM_SETDEFAULT			0x0126

#define BN_CLICKED			1
#define BN_DBLCLICKED			2
#define BN_PAINT			3

#define BS_PUSHBUTTON			0
#define BS_CHECKBOX			1
#define BS_AUTOCHECKBOX			2
#define BS_RADIOBUTTON			3
#define BS_AUTORADIOBUTTON		4
#define BS_3STATE			5
#define BS_AUTO3STATE			6
#define BS_USERBUTTON			7

#define BS_PRIMARYSTYLES		0x000f
#define BS_TEXT				0x0010
#define BS_MINIICON			0x0020
#define BS_BITMAP			0x0040
#define BS_ICON				0x0080
#define BS_HELP				0x0100
#define BS_SYSCOMMAND			0x0200
#define BS_DEFAULT			0x0400
#define BS_NOPOINTERFOCUS		0x0800
#define BS_NOBORDER			0x1000
#define BS_NOCURSORSELECT		0x2000
#define BS_AUTOSIZE			0x4000

typedef struct _BTNCDATA
{
  USHORT  cb;
  USHORT  fsCheckState;
  USHORT  fsHiliteState;
  LHANDLE hImage;
} BTNCDATA;
typedef BTNCDATA *PBTNCDATA;

typedef struct _USERBUTTON
{
  HWND	hwnd;
  HPS	hps;
  ULONG fsState;
  ULONG fsStateOld;
} USERBUTTON;
typedef USERBUTTON *PUSERBUTTON;


#endif /* INCL_WINBUTTONS */


#if defined (INCL_WINCLIPBOARD) || defined (INCL_WINDDE)

#define WM_RENDERFMT			0x0060
#define WM_RENDERALLFMTS		0x0061
#define WM_DESTROYCLIPBOARD		0x0062
#define WM_PAINTCLIPBOARD		0x0063
#define WM_SIZECLIPBOARD		0x0064
#define WM_HSCROLLCLIPBOARD		0x0065
#define WM_VSCROLLCLIPBOARD		0x0066
#define WM_DRAWCLIPBOARD		0x0067

#define CF_TEXT				1
#define CF_BITMAP			2
#define CF_DSPTEXT			3
#define CF_DSPBITMAP			4
#define CF_METAFILE			5
#define CF_DSPMETAFILE			6
#define CF_PALETTE			9
#define CF_MMPMFIRST			10
#define CF_MMPMLAST			19

#define SZFMT_TEXT			"#1"
#define SZFMT_BITMAP			"#2"
#define SZFMT_DSPTEXT			"#3"
#define SZFMT_DSPBITMAP			"#4"
#define SZFMT_METAFILE			"#5"
#define SZFMT_DSPMETAFILE		"#6"
#define SZFMT_PALETTE			"#9"
#define SZFMT_SYLK			"Sylk"
#define SZFMT_DIF			"Dif"
#define SZFMT_TIFF			"Tiff"
#define SZFMT_OEMTEXT			"OemText"
#define SZFMT_DIB			"Dib"
#define SZFMT_OWNERDISPLAY		"OwnerDisplay"
#define SZFMT_LINK			"Link"
#define SZFMT_METAFILEPICT		"MetaFilePict"
#define SZFMT_DSPMETAFILEPICT		"DspMetaFilePict"
#define SZFMT_CPTEXT			"Codepage Text"

typedef struct _CPTEXT
{
  USHORT idCountry;
  USHORT usCodepage;
  USHORT usLangID;
  USHORT usSubLangID;
  BYTE	 abText[1];
} CPTEXT;
typedef CPTEXT *PCPTEXT;

typedef struct _MFP
{
  POINTL sizeBounds;
  POINTL sizeMM;
  ULONG	 cbLength;
  USHORT mapMode;
  USHORT reserved;
  BYTE	 abData[1];
} MFP;
typedef MFP *PMFP;

#endif /* INCL_WINCLIPBOARD || INCL_WINDDE */

#if defined (INCL_WINCLIPBOARD)

#define CFI_OWNERFREE			0x0001
#define CFI_OWNERDISPLAY		0x0002
#define CFI_HANDLE			0x0200
#define CFI_POINTER			0x0400

BOOL WinCloseClipbrd (HAB hab);
BOOL WinEmptyClipbrd (HAB hab);
ULONG WinEnumClipbrdFmts (HAB hab, ULONG fmt);
BOOL WinOpenClipbrd (HAB hab);
ULONG WinQueryClipbrdData (HAB hab, ULONG fmt);
BOOL WinQueryClipbrdFmtInfo (HAB hab, ULONG fmt, PULONG prgfFmtInfo);
HWND WinQueryClipbrdOwner (HAB hab);
HWND WinQueryClipbrdViewer (HAB hab);
BOOL WinSetClipbrdData (HAB hab, ULONG ulData, ULONG fmt, ULONG rgfFmtInfo);
BOOL WinSetClipbrdOwner (HAB hab, HWND hwnd);
BOOL WinSetClipbrdViewer (HAB hab, HWND hwndNewClipViewer);

#endif /* INCL_WINCLIPBOARD */


#if defined (INCL_WINDDE)

#define WM_DDE_FIRST			0x00a0
#define WM_DDE_INITIATE			0x00a0
#define WM_DDE_REQUEST			0x00a1
#define WM_DDE_ACK			0x00a2
#define WM_DDE_DATA			0x00a3
#define WM_DDE_ADVISE			0x00a4
#define WM_DDE_UNADVISE			0x00a5
#define WM_DDE_POKE			0x00a6
#define WM_DDE_EXECUTE			0x00a7
#define WM_DDE_TERMINATE		0x00a8
#define WM_DDE_INITIATEACK		0x00a9
#define WM_DDE_LAST			0x00af

#define DDE_FACK			0x0001
#define DDE_FBUSY			0x0002
#define DDE_FNODATA			0x0004
#define DDE_FACKREQ			0x0008
#define DDE_FRESPONSE			0x0010
#define DDE_NOTPROCESSED		0x0020
#define DDE_FRESERVED			0x00c0
#define DDE_FAPPSTATUS			0xff00

#define DDECTXT_CASESENSITIVE		0x0001

#define DDEFMT_TEXT			0x0001

#define DDEPM_RETRY			0x0001
#define DDEPM_NOFREE			0x0002

#define SZDDESYS_TOPIC			"System"
#define SZDDESYS_ITEM_TOPICS		"Topics"
#define SZDDESYS_ITEM_SYSITEMS		"SysItems"
#define SZDDESYS_ITEM_RTNMSG		"ReturnMessage"
#define SZDDESYS_ITEM_STATUS		"Status"
#define SZDDESYS_ITEM_FORMATS		"Formats"
#define SZDDESYS_ITEM_SECURITY		"Security"
#define SZDDESYS_ITEM_ITEMFORMATS	"ItemFormats"
#define SZDDESYS_ITEM_HELP		"Help"
#define SZDDESYS_ITEM_PROTOCOLS		"Protocols"
#define SZDDESYS_ITEM_RESTART		"Restart"

typedef struct _CONVCONTEXT
{
  ULONG cb;
  ULONG fsContext;
  ULONG idCountry;
  ULONG usCodepage;
  ULONG usLangID;
  ULONG usSubLangID;
} CONVCONTEXT;
typedef CONVCONTEXT *PCONVCONTEXT;

typedef struct _DDEINIT
{
  ULONG cb;
  PSZ	pszAppName;
  PSZ	pszTopic;
  ULONG offConvContext;
} DDEINIT;
typedef DDEINIT *PDDEINIT;

typedef struct _DDESTRUCT
{
  ULONG	 cbData;
  USHORT fsStatus;
  USHORT usFormat;
  USHORT offszItemName;
  USHORT offabData;
} DDESTRUCT;
typedef DDESTRUCT *PDDESTRUCT;

#define DDES_PSZITEMNAME(pddes) \
    (((PSZ)pddes) + ((PDDESTRUCT)pddes)->offszItemName)

#define DDES_PABDATA(pddes) \
    (((PBYTE)pddes) + ((PDDESTRUCT)pddes)->offabData)

#define DDEI_PCONVCONTEXT(pddei) \
    ((PCONVCONTEXT)((PBYTE)pddei + pddei->offConvContext))

BOOL WinDdeInitiate (HWND hwndClient, PCSZ pszAppName, PCSZ pszTopicName,
    __const__ CONVCONTEXT *pcctxt);
BOOL WinDdePostMsg (HWND hwndTo, HWND hwndFrom, ULONG wm,
    __const__ DDESTRUCT *pddest, ULONG flOptions);
MRESULT WinDdeRespond (HWND hwndClient, HWND hwndServer, PCSZ pszAppName,
    PCSZ pszTopicName, __const__ CONVCONTEXT *pcctxt);

#endif /* INCL_WINDDE */


#if defined (INCL_WINCOUNTRY)

#define WCS_ERROR			0
#define WCS_EQ				1
#define WCS_LT				2
#define WCS_GT				3

ULONG WinCompareStrings (HAB hab, ULONG idcp, ULONG idcc, PCSZ psz1,
    PCSZ psz2, ULONG reserved);
UCHAR WinCpTranslateChar (HAB hab, ULONG cpSrc, UCHAR chSrc, ULONG cpDst);
BOOL WinCpTranslateString (HAB hab, ULONG cpSrc, PCSZ pszSrc, ULONG cpDst,
    ULONG cchDestMax, PSZ pchDest);
PSZ WinNextChar (HAB hab, ULONG idcp, ULONG idcc, PCSZ psz);
PSZ WinPrevChar (HAB hab, ULONG idcp, ULONG idcc, PCSZ pszStart,
    PCSZ psz);
ULONG WinQueryCp (HMQ hmq);
ULONG WinQueryCpList (HAB hab, ULONG ccpMax, PULONG prgcp);
BOOL WinSetCp (HMQ hmq, ULONG idCodePage);
ULONG WinUpper (HAB hab, ULONG idcp, ULONG idcc, PSZ psz);
ULONG WinUpperChar (HAB hab, ULONG idcp, ULONG idcc, ULONG c);

#endif /* INCL_WINCOUNTRY */


#if defined (INCL_WINCURSORS) || !defined (INCL_NOCOMMON)

#define CURSOR_SOLID			0x0000
#define CURSOR_HALFTONE			0x0001
#define CURSOR_FRAME			0x0002
#define CURSOR_FLASH			0x0004
#define CURSOR_BIDI_FIRST		0x0100
#define CURSOR_BIDI_LAST		0x0200
#define CURSOR_SETPOS			0x8000

BOOL WinCreateCursor (HWND hwnd, LONG x, LONG y, LONG cx, LONG cy,
    ULONG fs, PRECTL prclClip);
BOOL WinDestroyCursor (HWND hwnd);
BOOL WinShowCursor (HWND hwnd, BOOL fShow);

#endif /* INCL_WINCURSORS || !INCL_NOCOMMON */

#if defined (INCL_WINCURSORS)

typedef struct _CURSORINFO
{
  HWND	hwnd;
  LONG	x;
  LONG	y;
  LONG	cx;
  LONG	cy;
  ULONG fs;
  RECTL rclClip;
} CURSORINFO;
typedef CURSORINFO *PCURSORINFO;

BOOL WinQueryCursorInfo (HWND hwndDesktop, PCURSORINFO pCursorInfo);

#endif /* INCL_WINCURSORS */


#if defined (INCL_WINDESKTOP)

#define SDT_DESTROY			0x0001
#define SDT_NOBKGND			0x0002
#define SDT_TILE			0x0004
#define SDT_SCALE			0x0008
#define SDT_PATTERN			0x0010
#define SDT_CENTER			0x0020
#define SDT_RETAIN			0x0040
#define SDT_LOADFILE			0x0080

typedef struct _DESKTOP
{
  ULONG	  cbSize;
  HBITMAP hbm;
  LONG	  x;
  LONG	  y;
  ULONG	  fl;
  LONG	  lTileCount;
  CHAR	  szFile[260];
} DESKTOP;
typedef DESKTOP *PDESKTOP;

BOOL WinQueryDesktopBkgnd (HWND hwndDesktop, PDESKTOP pdsk);
HBITMAP WinSetDesktopBkgnd (HWND hwndDesktop, __const__ DESKTOP *pdskNew);

#endif /* INCL_WINDESKTOP */


#if defined (INCL_WINDIALOGS) || !defined (INCL_NOCOMMON)

#define DID_OK				1
#define DID_CANCEL			2
#define DID_ERROR			0xffff

#define MB_OK				0x0000
#define MB_OKCANCEL			0x0001
#define MB_RETRYCANCEL			0x0002
#define MB_ABORTRETRYIGNORE		0x0003
#define MB_YESNO			0x0004
#define MB_YESNOCANCEL			0x0005
#define MB_CANCEL			0x0006
#define MB_ENTER			0x0007
#define MB_ENTERCANCEL			0x0008

#define MB_NOICON			0x0000
#define MB_CUANOTIFICATION		0x0000
#define MB_ICONQUESTION			0x0010
#define MB_ICONEXCLAMATION		0x0020
#define MB_CUAWARNING			0x0020
#define MB_ICONASTERISK			0x0030
#define MB_ICONHAND			0x0040
#define MB_CUACRITICAL			0x0040
#define MB_QUERY			MB_ICONQUESTION
#define MB_WARNING			MB_CUAWARNING
#define MB_INFORMATION			MB_ICONASTERISK
#define MB_CRITICAL			MB_CUACRITICAL
#define MB_ERROR			MB_CRITICAL
#define MB_CUSTOMICON			0x0080

#define MB_DEFBUTTON1			0x0000
#define MB_DEFBUTTON2			0x0100
#define MB_DEFBUTTON3			0x0200

#define MB_APPLMODAL			0x0000
#define MB_SYSTEMMODAL			0x1000
#define MB_HELP				0x2000
#define MB_MOVEABLE			0x4000
#define MB_NONMODAL			0x8000

#define MBID_OK				1
#define MBID_CANCEL			2
#define MBID_ABORT			3
#define MBID_RETRY			4
#define MBID_IGNORE			5
#define MBID_YES			6
#define MBID_NO				7
#define MBID_HELP			8
#define MBID_ENTER			9
#define MBID_ERROR			0xffff

#define WA_WARNING			0
#define WA_NOTE				1
#define WA_ERROR			2
#define WA_CDEFALARMS			3

#if 0 /* Multimedia */
#define WA_WINDOWOPEN			3
#define WA_WINDOWCLOSE			4
#define WA_BEGINDRAG			5
#define WA_ENDDRAG			6
#define WA_STARTUP			7
#define WA_SHUTDOWN			8
#define WA_SHRED			9
#define WA_CWINALARMS			13
#endif /* 0 */

#define MAX_MBDTEXT			70

typedef struct _MB2D
{
  CHAR	achText[MAX_MBDTEXT+1];
  CHAR	_pad[1];
  ULONG idButton;
  LONG	flStyle;
} MB2D;
typedef MB2D *PMB2D;

typedef struct _MB2INFO
{
  ULONG	   cb;
  HPOINTER hIcon;
  ULONG	   cButtons;
  ULONG	   flStyle;
  HWND	   hwndNotify;
  MB2D	   mb2d[1];
} MB2INFO;

typedef MB2INFO *PMB2INFO;


#define WinCheckButton(hwndDlg,id,usCheckState) \
    ((ULONG)WinSendDlgItemMsg (hwndDlg, id, BM_SETCHECK, \
			       MPFROMSHORT (usCheckState), (MPARAM)NULL))

#define WinEnableControl(hwndDlg,id,fEnable) \
    WinEnableWindow (WinWindowFromID (hwndDlg, id), fEnable)

#define WinIsControlEnabled(hwndDlg,id) \
    ((BOOL)WinIsWindowEnabled (WinWindowFromID (hwndDlg, id)))

#define WinQueryButtonCheckstate(hwndDlg,id) \
    ((ULONG)WinSendDlgItemMsg (hwndDlg, id, BM_QUERYCHECK, \
			       (MPARAM)NULL, (MPARAM)NULL))


BOOL WinAlarm (HWND hwndDesktop, ULONG rgfType);
MRESULT WinDefDlgProc (HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2);
BOOL WinDismissDlg (HWND hwndDlg, ULONG usResult);
ULONG WinDlgBox (HWND hwndParent, HWND hwndOwner, PFNWP pfnDlgProc,
    HMODULE hmod, ULONG idDlg, PVOID pCreateParams);
BOOL WinGetDlgMsg (HWND hwndDlg, PQMSG pqmsg);
HWND WinLoadDlg (HWND hwndParent, HWND hwndOwner, PFNWP pfnDlgProc,
    HMODULE hmod, ULONG idDlg, PVOID pCreateParams);
ULONG WinMessageBox (HWND hwndParent, HWND hwndOwner, PCSZ pszText,
    PCSZ pszCaption, ULONG idWindow, ULONG flStyle);
ULONG WinMessageBox2 (HWND hwndParent, HWND hwndOwner, PCSZ pszText,
    PCSZ pszCaption, ULONG idWindow, PMB2INFO pmb2info);
BOOL WinQueryDlgItemShort (HWND hwndDlg, ULONG idItem, PSHORT pResult,
    BOOL fSigned);
ULONG WinQueryDlgItemText (HWND hwndDlg, ULONG idItem, LONG cchBufferMax,
    PSZ pchBuffer);
LONG WinQueryDlgItemTextLength (HWND hwndDlg, ULONG idItem);
BOOL WinSetDlgItemShort (HWND hwndDlg, ULONG idItem, USHORT usValue,
    BOOL fSigned);
BOOL WinSetDlgItemText (HWND hwndDlg, ULONG idItem, PCSZ pszText);

#endif /* INCL_WINDIALOGS || !defined (INCL_NOCOMMON) */


#if defined (INCL_WINDIALOGS)

#define DLGC_ENTRYFIELD			0x0001
#define DLGC_BUTTON			0x0002
#define DLGC_RADIOBUTTON		0x0004
#define DLGC_STATIC			0x0008
#define DLGC_DEFAULT			0x0010
#define DLGC_PUSHBUTTON			0x0020
#define DLGC_CHECKBOX			0x0040
#define DLGC_SCROLLBAR			0x0080
#define DLGC_MENU			0x0100
#define DLGC_TABONCLICK			0x0200
#define DLGC_MLE			0x0400

#define EDI_FIRSTTABITEM		0
#define EDI_LASTTABITEM			1
#define EDI_NEXTTABITEM			2
#define EDI_PREVTABITEM			3
#define EDI_FIRSTGROUPITEM		4
#define EDI_LASTGROUPITEM		5
#define EDI_NEXTGROUPITEM		6
#define EDI_PREVGROUPITEM		7

typedef struct _DLGTITEM
{
  USHORT fsItemStatus;
  USHORT cChildren;
  USHORT cchClassName;
  USHORT offClassName;
  USHORT cchText;
  USHORT offText;
  ULONG	 flStyle;
  SHORT	 x;
  SHORT	 y;
  SHORT	 cx;
  SHORT	 cy;
  USHORT id;
  USHORT offPresParams;
  USHORT offCtlData;
} DLGTITEM;
typedef DLGTITEM *PDLGTITEM;

typedef struct _DLGTEMPLATE
{
  USHORT   cbTemplate;
  USHORT   type;
  USHORT   codepage;
  USHORT   offadlgti;
  USHORT   fsTemplateStatus;
  USHORT   iItemFocus;
  USHORT   coffPresParams;
  DLGTITEM adlgti[1];
} DLGTEMPLATE;
typedef DLGTEMPLATE *PDLGTEMPLATE;


HWND WinCreateDlg (HWND hwndParent, HWND hwndOwner, PFNWP pfnDlgProc,
    PDLGTEMPLATE pdlgt, PVOID pCreateParams);
HWND WinEnumDlgItem (HWND hwndDlg, HWND hwnd, ULONG code);
BOOL WinMapDlgPoints (HWND hwndDlg, PPOINTL prgwptl, ULONG cwpt,
    BOOL fCalcWindowCoords);
ULONG WinProcessDlg (HWND hwndDlg);
MRESULT WinSendDlgItemMsg (HWND hwndDlg, ULONG idItem, ULONG msg,
    MPARAM mp1, MPARAM mp2);
LONG WinSubstituteStrings (HWND hwnd, PCSZ pszSrc, LONG cchDstMax,
    PSZ pszDst);

#endif /* INCL_WINDIALOGS */


#if defined (INCL_WINENTRYFIELDS)

#define CBID_LIST			0x029a
#define CBID_EDIT			0x029b

#define CBM_SHOWLIST			0x0170
#define CBM_HILITE			0x0171
#define CBM_ISLISTSHOWING		0x0172

#define CBN_EFCHANGE			1
#define CBN_EFSCROLL			2
#define CBN_MEMERROR			3
#define CBN_LBSELECT			4
#define CBN_LBSCROLL			5
#define CBN_SHOWLIST			6
#define CBN_ENTER			7

#define CBS_SIMPLE			0x0001
#define CBS_DROPDOWN			0x0002
#define CBS_DROPDOWNLIST		0x0004
#define CBS_COMPATIBLE			0x0008

#define EM_QUERYCHANGED			0x0140
#define EM_QUERYSEL			0x0141
#define EM_SETSEL			0x0142
#define EM_SETTEXTLIMIT			0x0143
#define EM_CUT				0x0144
#define EM_COPY				0x0145
#define EM_CLEAR			0x0146
#define EM_PASTE			0x0147
#define EM_QUERYFIRSTCHAR		0x0148
#define EM_SETFIRSTCHAR			0x0149
#define EM_QUERYREADONLY		0x014a
#define EM_SETREADONLY			0x014b
#define EM_SETINSERTMODE		0x014c

#define EN_SETFOCUS			0x0001
#define EN_KILLFOCUS			0x0002
#define EN_CHANGE			0x0004
#define EN_SCROLL			0x0008
#define EN_MEMERROR			0x0010
#define EN_OVERFLOW			0x0020
#define EN_INSERTMODETOGGLE		0x0040

#define ES_LEFT				0x0000
#define ES_CENTER			0x0001
#define ES_RIGHT			0x0002
#define ES_AUTOSCROLL			0x0004
#define ES_MARGIN			0x0008
#define ES_AUTOTAB			0x0010
#define ES_READONLY			0x0020
#define ES_COMMAND			0x0040
#define ES_UNREADABLE			0x0080
#define ES_AUTOSIZE			0x0200

#if defined (INCL_NLS)
#define ES_ANY				0x0000
#define ES_SBCS				0x1000
#define ES_DBCS				0x2000
#define ES_MIXED			0x3000
#endif /* INCL_NLS */

typedef struct _COMBOCDATA
{
  ULONG cbSize;
  ULONG reserved;
  PVOID pHWXCtlData;
} COMBOCDATA;
typedef COMBOCDATA *PCOMBOCDATA;

typedef struct _ENTRYFDATA
{
  USHORT cb;
  USHORT cchEditLimit;
  USHORT ichMinSel;
  USHORT ichMaxSel;
  PVOID	 pHWXCtlData;
} ENTRYFDATA;
typedef ENTRYFDATA *PENTRYFDATA;

#endif /* INCL_WINENTRYFIELDS */


#if defined (INCL_WINERRORS)

typedef struct _ERRINFO
{
  ULONG	  cbFixedErrInfo;
  ERRORID idError;
  ULONG	  cDetailLevel;
  ULONG	  offaoffszMsg;
  ULONG	  offBinaryData;
} ERRINFO;
typedef ERRINFO *PERRINFO;

ERRORID WinGetLastError (HAB hab);
BOOL WinFreeErrorInfo (PERRINFO perrinfo);
PERRINFO WinGetErrorInfo (HAB hab);

#endif /* INCL_WINERRORS */


#if defined (INCL_WINFRAMECTLS)

#define TBM_SETHILITE			0x01e3
#define TBM_QUERYHILITE			0x01e4

#endif /* INCL_WINFRAMECTLS */


#if defined (INCL_WINHOOKS)

#define HK_SENDMSG			0
#define HK_INPUT			1
#define HK_MSGFILTER			2
#define HK_JOURNALRECORD		3
#define HK_JOURNALPLAYBACK		4
#define HK_HELP				5
#define HK_LOADER			6
#define HK_REGISTERUSERMSG		7
#define HK_MSGCONTROL			8
#define HK_PLIST_ENTRY			9
#define HK_PLIST_EXIT			10
#define HK_FINDWORD			11
#define HK_CODEPAGECHANGED		12
#define HK_WINDOWDC			15
#define HK_DESTROYWINDOW		16
#define HK_CHECKMSGFILTER		20
#define HK_MSGINPUT			21
#define HK_ALARM			22
#define HK_LOCKUP			23
#define HK_FLUSHBUF			24

#define HLPM_FRAME			(-1)
#define HLPM_WINDOW			(-2)
#define HLPM_MENU			(-3)

#define HMQ_CURRENT		((HMQ)1)

#define LHK_DELETEPROC			1
#define LHK_DELETELIB			2
#define LHK_LOADPROC			3
#define LHK_LOADLIB			4

#define MCHK_MSGINTEREST		1
#define MCHK_CLASSMSGINTEREST		2
#define MCHK_SYNCHRONISATION		3
#define MCHK_MSGMODE			4

#define MSGF_DIALOGBOX			1
#define MSGF_MESSAGEBOX			2
#define MSGF_DDEPOSTMSG			3
#define MSGF_TRACK			8

#define PM_MODEL_1X			0
#define PM_MODEL_2X			1

#define RUMHK_DATATYPE			1
#define RUMHK_MSG			2


typedef struct _SMHSTRUCT
{
  MPARAM mp2;
  MPARAM mp1;
  ULONG	 msg;
  HWND	 hwnd;
  ULONG	 model;
} SMHSTRUCT;
typedef SMHSTRUCT *PSMHSTRUCT;


BOOL WinCallMsgFilter (HAB hab, PQMSG pqmsg, ULONG msgf);
BOOL WinReleaseHook (HAB hab, HMQ hmq, LONG iHook, PFN pfnHook, HMODULE hmod);
BOOL WinSetHook (HAB hab, HMQ hmq, LONG iHook, PFN pfnHook, HMODULE hmod);

#endif /* INCL_WINHOOKS */


#if defined (INCL_WININPUT) || !defined (INCL_NOCOMMON)

#define FC_NOSETFOCUS			0x0001
#define FC_NOBRINGTOTOP			0x0001 /*!*/
#define FC_NOLOSEFOCUS			0x0002
#define FC_NOBRINGTOPFIRSTWINDOW	0x0002 /*!*/
#define FC_NOSETACTIVE			0x0004
#define FC_NOLOSEACTIVE			0x0008
#define FC_NOSETSELECTION		0x0010
#define FC_NOLOSESELECTION		0x0020

#define QFC_NEXTINCHAIN			0x0001
#define QFC_ACTIVE			0x0002
#define QFC_FRAME			0x0003
#define QFC_SELECTACTIVE		0x0004
#define QFC_PARTOFCHAIN			0x0005

BOOL WinFocusChange (HWND hwndDesktop, HWND hwndSetFocus, ULONG flFocusChange);
BOOL WinLockupSystem (HAB hab);
BOOL WinSetFocus (HWND hwndDesktop, HWND hwndSetFocus);
BOOL WinUnlockSystem (HAB hab, PCSZ pszPassword);

#endif /* INCL_WININPUT || !INCL_NOCOMMON */


#if defined (INCL_WININPUT)

#define WM_MOUSEFIRST			0x0070
#define WM_MOUSEMOVE			0x0070
#define WM_BUTTONCLICKFIRST		0x0071
#define WM_BUTTON1DOWN			0x0071
#define WM_BUTTON1UP			0x0072
#define WM_BUTTON1DBLCLK		0x0073
#define WM_BUTTON2DOWN			0x0074
#define WM_BUTTON2UP			0x0075
#define WM_BUTTON2DBLCLK		0x0076
#define WM_BUTTON3DOWN			0x0077
#define WM_BUTTON3UP			0x0078
#define WM_BUTTON3DBLCLK		0x0079
#define WM_BUTTONCLICKLAST		0x0079
#define WM_MOUSELAST			0x0079
#define WM_CHAR				0x007a
#define WM_VIOCHAR			0x007b
#define WM_JOURNALNOTIFY		0x007c
#define WM_MOUSEMAP			0x007d
#define WM_VRNDISABLED			0x007e
#define WM_VRNENABLED			0x007f

#define WM_EXTMOUSEFIRST		0x0410
#define WM_CHORD			0x0410
#define WM_BUTTON1MOTIONSTART		0x0411
#define WM_BUTTON1MOTIONEND		0x0412
#define WM_BUTTON1CLICK			0x0413
#define WM_BUTTON2MOTIONSTART		0x0414
#define WM_BUTTON2MOTIONEND		0x0415
#define WM_BUTTON2CLICK			0x0416
#define WM_BUTTON3MOTIONSTART		0x0417
#define WM_BUTTON3MOTIONEND		0x0418
#define WM_BUTTON3CLICK			0x0419
#define WM_EXTMOUSELAST			0x0419

#define WM_MOUSETRANSLATEFIRST		0x0420
#define WM_BEGINDRAG			0x0420
#define WM_ENDDRAG			0x0421
#define WM_SINGLESELECT			0x0422
#define WM_OPEN				0x0423
#define WM_CONTEXTMENU			0x0424
#define WM_CONTEXTHELP			0x0425
#define WM_TEXTEDIT			0x0426
#define WM_BEGINSELECT			0x0427
#define WM_ENDSELECT			0x0428
#define WM_MOUSETRANSLATELAST		0x0428
#define WM_PICKUP			0x0429

#define WM_PENFIRST			0x0481
#define WM_PENLAST			0x049f

#define WM_MMPMFIRST			0x0500
#define WM_MMPMLAST			0x05ff

#define WM_BIDI_FIRST			0x0bd0
#define WM_BIDI_LAST			0x0bff

#define INP_NONE			0x0000
#define INP_KBD				0x0001
#define INP_MULT			0x0002
#define INP_RES2			0x0004
#define INP_SHIFT			0x0008
#define INP_CTRL			0x0010
#define INP_ALT				0x0020
#define INP_RES3			0x0040
#define INP_RES4			0x0080
#define INP_IGNORE			0xffff

#define JRN_QUEUESTATUS			0x0001
#define JRN_PHYSKEYSTATE		0x0002

#define KC_NONE				0x0000
#define KC_CHAR				0x0001
#define KC_VIRTUALKEY			0x0002
#define KC_SCANCODE			0x0004
#define KC_SHIFT			0x0008
#define KC_CTRL				0x0010
#define KC_ALT				0x0020
#define KC_KEYUP			0x0040
#define KC_PREVDOWN			0x0080
#define KC_LONEKEY			0x0100
#define KC_DEADKEY			0x0200
#define KC_COMPOSITE			0x0400
#define KC_INVALIDCOMP			0x0800
#define KC_TOGGLE			0x1000
#define KC_INVALIDCHAR			0x2000
#define KC_DBCSRSRVD1			0x4000
#define KC_DBCSRSRVD2			0x8000

#define VK_BUTTON1			0x0001
#define VK_BUTTON2			0x0002
#define VK_BUTTON3			0x0003
#define VK_BREAK			0x0004
#define VK_BACKSPACE			0x0005
#define VK_TAB				0x0006
#define VK_BACKTAB			0x0007
#define VK_NEWLINE			0x0008
#define VK_SHIFT			0x0009
#define VK_CTRL				0x000a
#define VK_ALT				0x000b
#define VK_ALTGRAF			0x000c
#define VK_PAUSE			0x000d
#define VK_CAPSLOCK			0x000e
#define VK_ESC				0x000f
#define VK_SPACE			0x0010
#define VK_PAGEUP			0x0011
#define VK_PAGEDOWN			0x0012
#define VK_END				0x0013
#define VK_HOME				0x0014
#define VK_LEFT				0x0015
#define VK_UP				0x0016
#define VK_RIGHT			0x0017
#define VK_DOWN				0x0018
#define VK_PRINTSCRN			0x0019
#define VK_INSERT			0x001a
#define VK_DELETE			0x001b
#define VK_SCRLLOCK			0x001c
#define VK_NUMLOCK			0x001d
#define VK_ENTER			0x001e
#define VK_SYSRQ			0x001f
#define VK_F1				0x0020
#define VK_F2				0x0021
#define VK_F3				0x0022
#define VK_F4				0x0023
#define VK_F5				0x0024
#define VK_F6				0x0025
#define VK_F7				0x0026
#define VK_F8				0x0027
#define VK_F9				0x0028
#define VK_F10				0x0029
#define VK_F11				0x002a
#define VK_F12				0x002b
#define VK_F13				0x002c
#define VK_F14				0x002d
#define VK_F15				0x002e
#define VK_F16				0x002f
#define VK_F17				0x0030
#define VK_F18				0x0031
#define VK_F19				0x0032
#define VK_F20				0x0033
#define VK_F21				0x0034
#define VK_F22				0x0035
#define VK_F23				0x0036
#define VK_F24				0x0037
#define VK_ENDDRAG			0x0038
#define VK_CLEAR			0x0039
#define VK_EREOF			0x003a
#define VK_PA1				0x003b
#define VK_ATTN				0x003c
#define VK_CRSEL			0x003d
#define VK_EXSEL			0x003e
#define VK_COPY				0x003f
#define VK_BLK1				0x0040
#define VK_BLK2				0x0041

#define VK_MENU				VK_F10

#if defined (INCL_NLS)
#define VK_DBCSFIRST			0x0080
#define VK_DBCSLAST			0x00ff
#define VK_BIDI_FIRST			0x00e0
#define VK_BIDI_LAST			0x00ff
#endif /* INCL_NLS */

#define VK_USERFIRST			0x0100
#define VK_USERLAST			0x01ff

typedef struct _CHARMSG
{
  USHORT fs;
  UCHAR	 cRepeat;
  UCHAR	 scancode;
  USHORT chr;
  USHORT vkey;
} CHRMSG;
typedef CHRMSG *PCHRMSG;

typedef struct _MOUSEMSG
{
  SHORT	 x;
  SHORT	 y;
  USHORT codeHitTest;
  USHORT fsInp;
} MSEMSG;
typedef MSEMSG *PMSEMSG;

#define CHARMSG(pmsg)		((PCHRMSG)((PBYTE)pmsg + sizeof (ULONG)))
#define MOUSEMSG(pmsg)		((PMSEMSG)((PBYTE)pmsg + sizeof (ULONG)))

BOOL WinCheckInput (HAB hab);
BOOL WinEnablePhysInput (HWND hwndDesktop, BOOL fEnable);
LONG WinGetKeyState (HWND hwndDesktop, LONG vkey);
LONG WinGetPhysKeyState (HWND hwndDesktop, LONG sc);
BOOL WinIsPhysInputEnabled (HWND hwndDesktop);
HWND WinQueryCapture (HWND hwndDesktop);
HWND WinQueryFocus (HWND hwndDesktop);
ULONG WinQueryVisibleRegion (HWND hwnd, HRGN hrgn);
BOOL WinSetCapture (HWND hwndDesktop, HWND hwnd);
BOOL WinSetKeyboardStateTable (HWND hwndDesktop, PBYTE pKeyStateTable,
    BOOL fSet);
BOOL WinSetVisibleRegionNotify (HWND hwnd, BOOL fEnable);

#endif /* INCL_WININPUT */


#if defined (INCL_WINLISTBOXES)

#define LS_MULTIPLESEL			0x0001
#define LS_OWNERDRAW			0x0002
#define LS_NOADJUSTPOS			0x0004
#define LS_HORZSCROLL			0x0008
#define LS_EXTENDEDSEL			0x0010

#define LN_SELECT			1
#define LN_SETFOCUS			2
#define LN_KILLFOCUS			3
#define LN_SCROLL			4
#define LN_ENTER			5

#define LM_QUERYITEMCOUNT		0x0160
#define LM_INSERTITEM			0x0161
#define LM_SETTOPINDEX			0x0162
#define LM_DELETEITEM			0x0163
#define LM_SELECTITEM			0x0164
#define LM_QUERYSELECTION		0x0165
#define LM_SETITEMTEXT			0x0166
#define LM_QUERYITEMTEXTLENGTH		0x0167
#define LM_QUERYITEMTEXT		0x0168
#define LM_SETITEMHANDLE		0x0169
#define LM_QUERYITEMHANDLE		0x016a
#define LM_SEARCHSTRING			0x016b
#define LM_SETITEMHEIGHT		0x016c
#define LM_QUERYTOPINDEX		0x016d
#define LM_DELETEALL			0x016e
#define LM_INSERTMULTITEMS		0x016f
#define LM_SETITEMWIDTH			0x0660 /* ? */

#define LIT_CURSOR			(-4)
#define LIT_ERROR			(-3)
#define LIT_MEMERROR			(-2)
#define LIT_NONE			(-1)
#define LIT_FIRST			(-1)

#define LIT_END				(-1)
#define LIT_SORTASCENDING		(-2)
#define LIT_SORTDESCENDING		(-3)

#define LSS_SUBSTRING			0x0001
#define LSS_PREFIX			0x0002
#define LSS_CASESENSITIVE		0x0004


typedef struct _LBOXINFO
{
  LONG	lItemIndex;
  ULONG ulItemCount;
  ULONG reserved;
  ULONG reserved2;
} LBOXINFO;
typedef LBOXINFO *PLBOXINFO;


#define WinDeleteLboxItem(hwndLbox,index) \
    ((LONG)WinSendMsg (hwndLbox, LM_DELETEITEM, MPFROMLONG (index), \
		       (MPARAM)NULL))

#define WinInsertLboxItem(hwndLbox,index,psz) \
    ((LONG)WinSendMsg (hwndLbox, LM_INSERTITEM, MPFROMLONG(index), \
		       MPFROMP (psz)))

#define WinQueryLboxCount(hwndLbox) \
    ((LONG)WinSendMsg (hwndLbox, LM_QUERYITEMCOUNT, (MPARAM)NULL, \
		       (MPARAM)NULL))

#define WinQueryLboxItemText(hwndLbox,index,psz,cchMax) \
    ((LONG)WinSendMsg (hwndLbox, LM_QUERYITEMTEXT, \
		       MPFROM2SHORT((index), (cchMax)), MPFROMP (psz)))

#define WinQueryLboxItemTextLength(hwndLbox,index) \
    ((SHORT)WinSendMsg (hwndLbox, LM_QUERYITEMTEXTLENGTH, \
			MPFROMSHORT (index), (MPARAM)NULL))

#define WinQueryLboxSelectedItem(hwndLbox) \
    ((LONG)WinSendMsg (hwndLbox, LM_QUERYSELECTION, MPFROMLONG (LIT_FIRST), \
		       (MPARAM)NULL))

#define WinSetLboxItemText(hwndLbox,index,psz) \
    ((BOOL)WinSendMsg (hwndLbox, LM_SETITEMTEXT, \
		       MPFROMLONG (index), MPFROMP (psz)))

#endif /* INCL_WINLISTBOXES */


#if defined (INCL_WINLOAD)

BOOL WinDeleteLibrary (HAB hab, HLIB libhandle);
BOOL WinDeleteProcedure (HAB hab, PFNWP wndproc);
HLIB WinLoadLibrary (HAB hab, PCSZ libname);
PFNWP WinLoadProcedure (HAB hab, HLIB libhandle, PSZ procname);

#endif /* INCL_WINLOAD */


#if defined (INCL_WINMENUS)

#define MIA_NODISMISS			0x0020
#define MIA_FRAMED			0x1000
#define MIA_CHECKED			0x2000
#define MIA_DISABLED			0x4000
#define MIA_HILITED			0x8000

#define MIS_TEXT			0x0001
#define MIS_BITMAP			0x0002
#define MIS_SEPARATOR			0x0004
#define MIS_OWNERDRAW			0x0008
#define MIS_SUBMENU			0x0010
#define MIS_MULTMENU			0x0020
#define MIS_SYSCOMMAND			0x0040
#define MIS_HELP			0x0080
#define MIS_STATIC			0x0100
#define MIS_BUTTONSEPARATOR		0x0200
#define MIS_BREAK			0x0400
#define MIS_BREAKSEPARATOR		0x0800
#define MIS_GROUP			0x1000
#define MIS_SINGLE			0x2000

#define MIT_END				(-1)
#define MIT_NONE			(-1)
#define MIT_MEMERROR			(-1)
#define MIT_ERROR			(-1)
#define MIT_FIRST			(-2)
#define MIT_LAST			(-3)

#define MM_INSERTITEM			0x0180
#define MM_DELETEITEM			0x0181
#define MM_QUERYITEM			0x0182
#define MM_SETITEM			0x0183
#define MM_QUERYITEMCOUNT		0x0184
#define MM_STARTMENUMODE		0x0185
#define MM_ENDMENUMODE			0x0186
#define MM_REMOVEITEM			0x0188
#define MM_SELECTITEM			0x0189
#define MM_QUERYSELITEMID		0x018a
#define MM_QUERYITEMTEXT		0x018b
#define MM_QUERYITEMTEXTLENGTH		0x018c
#define MM_SETITEMHANDLE		0x018d
#define MM_SETITEMTEXT			0x018e
#define MM_ITEMPOSITIONFROMID		0x018f
#define MM_ITEMIDFROMPOSITION		0x0190
#define MM_QUERYITEMATTR		0x0191
#define MM_SETITEMATTR			0x0192
#define MM_ISITEMVALID			0x0193
#define MM_QUERYITEMRECT		0x0194

#define MM_QUERYDEFAULTITEMID		0x0431
#define MM_SETDEFAULTITEMID		0x0432

#define MS_ACTIONBAR			0x0001
#define MS_TITLEBUTTON			0x0002
#define MS_VERTICALFLIP			0x0004
#define MS_CONDITIONALCASCADE		0x0040

#define PU_POSITIONONITEM		0x0001
#define PU_HCONSTRAIN			0x0002
#define PU_VCONSTRAIN			0x0004
#define PU_NONE				0x0000
#define PU_MOUSEBUTTON1DOWN		0x0008
#define PU_MOUSEBUTTON2DOWN		0x0010
#define PU_MOUSEBUTTON3DOWN		0x0018
#define PU_SELECTITEM			0x0020
#define PU_MOUSEBUTTON1			0x0040
#define PU_MOUSEBUTTON2			0x0080
#define PU_MOUSEBUTTON3			0x0100
#define PU_KEYBOARD			0x0200

typedef struct _MENUITEM
{
  SHORT	 iPosition;
  USHORT afStyle;
  USHORT afAttribute;
  USHORT id;
  HWND	 hwndSubMenu;
  ULONG	 hItem;
} MENUITEM;
typedef MENUITEM *PMENUITEM;

typedef struct _mti		/* Note 1 */
{
  USHORT afStyle;
  USHORT pad;
  USHORT idItem;
  CHAR	 c[2];
} MTI;

typedef struct _mt		/* Note 1 */
{
  ULONG	 len;
  USHORT codepage;
  USHORT reserved;
  USHORT cMti;
  MTI	 rgMti[1];
} MT;
typedef MT *LPMT;

typedef struct _OWNERITEM
{
  HWND	hwnd;
  HPS	hps;
  ULONG fsState;
  ULONG fsAttribute;
  ULONG fsStateOld;
  ULONG fsAttributeOld;
  RECTL rclItem;
  LONG	idItem;
  ULONG hItem;
} OWNERITEM;
typedef OWNERITEM *POWNERITEM;


#define WinCheckMenuItem(hwndMenu,id,fcheck) \
    ((BOOL)WinSendMsg (hwndMenu, MM_SETITEMATTR, \
		       MPFROM2SHORT (id, TRUE), \
		       MPFROM2SHORT (MIA_CHECKED, \
				     ((USHORT)(fcheck) ? MIA_CHECKED : 0))))

#define WinEnableMenuItem(hwndMenu,id,fEnable) \
    ((BOOL)WinSendMsg (hwndMenu, MM_SETITEMATTR, MPFROM2SHORT (id, TRUE), \
		       MPFROM2SHORT (MIA_DISABLED, \
				     ((USHORT)(fEnable) ? 0 : MIA_DISABLED))))

#define WinIsMenuItemChecked(hwndMenu,id) \
    ((BOOL)WinSendMsg (hwndMenu, MM_QUERYITEMATTR, \
		       MPFROM2SHORT (id, TRUE), \
		       MPFROMLONG (MIA_CHECKED)))

#define WinIsMenuItemEnabled(hwndMenu,id)  \
    (!(BOOL)WinSendMsg (hwndMenu, MM_QUERYITEMATTR, \
			MPFROM2SHORT (id, TRUE), \
			MPFROMLONG (MIA_DISABLED)))

#define WinIsMenuItemValid(hwndMenu,id) \
    ((BOOL)WinSendMsg (hwndMenu, MM_ISITEMVALID, \
		       MPFROM2SHORT (id, TRUE), MPFROMLONG (FALSE)))

#define WinSetMenuItemText(hwndMenu,id,psz) \
    ((BOOL)WinSendMsg (hwndMenu, MM_SETITEMTEXT, \
		       MPFROMLONG (id), MPFROMP (psz)))


HWND WinCreateMenu (HWND hwndParent, CPVOID lpmt);
HWND WinLoadMenu (HWND hwndFrame, HMODULE hmod, ULONG idMenu);
BOOL WinPopupMenu (HWND hwndParent, HWND hwndOwner, HWND hwndMenu,
    LONG x, LONG y, LONG idItem, ULONG fs);

#endif /* INCL_WINMENUS */


#if defined (INCL_WINMESSAGEMGR)

#define BMSG_POST			0x0000
#define BMSG_SEND			0x0001
#define BMSG_POSTQUEUE			0x0002
#define BMSG_DESCENDANTS		0x0004
#define BMSG_FRAMEONLY			0x0008

#define CVR_ALIGNLEFT			0x0001
#define CVR_ALIGNBOTTOM			0x0002
#define CVR_ALIGNRIGHT			0x0004
#define CVR_ALIGNTOP			0x0008
#define CVR_REDRAW			0x0010

#define HT_NORMAL			0
#define HT_TRANSPARENT			(-1)
#define HT_DISCARD			(-2)
#define HT_ERROR			(-3)

#define QS_KEY				0x0001
#define QS_MOUSEBUTTON			0x0002
#define QS_MOUSEMOVE			0x0004
#define QS_MOUSE			0x0006
#define QS_TIMER			0x0008
#define QS_PAINT			0x0010
#define QS_POSTMSG			0x0020
#define QS_SEM1				0x0040
#define QS_SEM2				0x0080
#define QS_SEM3				0x0100
#define QS_SEM4				0x0200
#define QS_SENDMSG			0x0400
#define QS_MSGINPUT			0x0800

#define SMIM_ALL			0x0eff
#define SMI_NOINTEREST			0x0001
#define SMI_INTEREST			0x0002
#define SMI_RESET			0x0004
#define SMI_AUTODISPATCH		0x0008

#define WPM_TEXT			0x0001
#define WPM_CTLDATA			0x0002
#define WPM_PRESPARAMS			0x0004
#define WPM_CCHTEXT			0x0008
#define WPM_CBCTLDATA			0x0010
#define WPM_CBPRESPARAMS		0x0020


typedef struct _WNDPARAMS
{
  ULONG fsStatus;
  ULONG cchText;
  PSZ	pszText;
  ULONG cbPresParams;
  PVOID pPresParams;
  ULONG cbCtlData;
  PVOID pCtlData;
} WNDPARAMS;
typedef WNDPARAMS *PWNDPARAMS;

BOOL WinBroadcastMsg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2, ULONG rgf);
BOOL WinInSendMsg (HAB hab);
BOOL WinPostQueueMsg (HMQ hmq, ULONG msg, MPARAM mp1, MPARAM mp2);
BOOL WinQueryMsgPos (HAB hab, PPOINTL pptl);
ULONG WinQueryMsgTime (HAB hab);
ULONG WinQueryQueueStatus (HWND hwndDesktop);
ULONG WinRequestMutexSem (HMTX hmtx, ULONG ulTimeout);
BOOL WinSetClassMsgInterest (HAB hab, PCSZ pszClassName, ULONG msg_class,
    LONG control);
BOOL WinSetMsgInterest (HWND hwnd, ULONG msg_class, LONG control);
ULONG WinWaitEventSem (HEV hev, ULONG ulTimeout);
BOOL WinWaitMsg (HAB hab, ULONG msgFirst, ULONG msgLast);
ULONG WinWaitMuxWaitSem (HMUX hmux, ULONG ulTimeout, PULONG pulUser);

#endif /* INCL_WINMESSAGEMGR */


#if defined (INCL_WINPALETTE)

LONG WinRealizePalette (HWND hwnd, HPS hps, PULONG pcclr);

#endif /* INCL_WINPALETTE */


#if defined (INCL_WINPOINTERS)

#define DP_NORMAL			0x0000
#define DP_HALFTONED			0x0001
#define DP_INVERTED			0x0002
#define DP_MINI				0x0004

#define SBMP_OLD_SYSMENU		1
#define SBMP_OLD_SBUPARROW		2
#define SBMP_OLD_SBDNARROW		3
#define SBMP_OLD_SBRGARROW		4
#define SBMP_OLD_SBLFARROW		5
#define SBMP_MENUCHECK			6
#define SBMP_OLD_CHECKBOXES		7
#define SBMP_BTNCORNERS			8
#define SBMP_OLD_MINBUTTON		9
#define SBMP_OLD_MAXBUTTON		10
#define SBMP_OLD_RESTOREBUTTON		11
#define SBMP_OLD_CHILDSYSMENU		12
#define SBMP_DRIVE			15
#define SBMP_FILE			16
#define SBMP_FOLDER			17
#define SBMP_TREEPLUS			18
#define SBMP_TREEMINUS			19
#define SBMP_PROGRAM			22
#define SBMP_MENUATTACHED		23
#define SBMP_SIZEBOX			24
#define SBMP_SYSMENU			25
#define SBMP_MINBUTTON			26
#define SBMP_MAXBUTTON			27
#define SBMP_RESTOREBUTTON		28
#define SBMP_CHILDSYSMENU		29
#define SBMP_SYSMENUDEP			30
#define SBMP_MINBUTTONDEP		31
#define SBMP_MAXBUTTONDEP		32
#define SBMP_RESTOREBUTTONDEP		33
#define SBMP_CHILDSYSMENUDEP		34
#define SBMP_SBUPARROW			35
#define SBMP_SBDNARROW			36
#define SBMP_SBLFARROW			37
#define SBMP_SBRGARROW			38
#define SBMP_SBUPARROWDEP		39
#define SBMP_SBDNARROWDEP		40
#define SBMP_SBLFARROWDEP		41
#define SBMP_SBRGARROWDEP		42
#define SBMP_SBUPARROWDIS		43
#define SBMP_SBDNARROWDIS		44
#define SBMP_SBLFARROWDIS		45
#define SBMP_SBRGARROWDIS		46
#define SBMP_COMBODOWN			47
#define SBMP_CHECKBOXES			48
#define SBMP_HIDE			50
#define SBMP_HIDEDEP			51
#define SBMP_CLOSE			52
#define SBMP_CLOSEDEP			53

#define SPTR_ARROW			1
#define SPTR_TEXT			2
#define SPTR_WAIT			3
#define SPTR_SIZE			4
#define SPTR_MOVE			5
#define SPTR_SIZENWSE			6
#define SPTR_SIZENESW			7
#define SPTR_SIZEWE			8
#define SPTR_SIZENS			9
#define SPTR_APPICON			10
#define SPTR_ICONINFORMATION		11
#define SPTR_ICONQUESTION		12
#define SPTR_ICONERROR			13
#define SPTR_ICONWARNING		14
#define SPTR_ILLEGAL			18
#define SPTR_FILE			19
#define SPTR_FOLDER			20
#define SPTR_MULTFILE			21
#define SPTR_PROGRAM			22
#define SPTR_DISPLAY_PTRS		22

#define SPTR_PENFIRST			23
#define SPTR_PENLAST			39
#define SPTR_CPTR			39

#define SPTR_HANDICON			SPTR_ICONERROR
#define SPTR_QUESICON			SPTR_ICONQUESTION
#define SPTR_BANGICON			SPTR_ICONWARNING
#define SPTR_NOTEICON			SPTR_ICONINFORMATION

typedef struct _POINTERINFO
{
  ULONG	  fPointer;
  LONG	  xHotspot;
  LONG	  yHotspot;
  HBITMAP hbmPointer;
  HBITMAP hbmColor;
  HBITMAP hbmMiniPointer;
  HBITMAP hbmMiniColor;
} POINTERINFO;
typedef POINTERINFO *PPOINTERINFO;


HPOINTER WinCreatePointer (HWND hwndDesktop, HBITMAP hbmPointer, BOOL fPointer,
    LONG xHotspot, LONG yHotspot);
HPOINTER WinCreatePointerIndirect (HWND hwndDesktop,
    __const__ POINTERINFO *pptri);
BOOL WinDestroyPointer (HPOINTER hptr);
BOOL WinDrawPointer (HPS hps, LONG x, LONG y, HPOINTER hptr, ULONG fs);
HBITMAP WinGetSysBitmap (HWND hwndDesktop, ULONG ibm);
HPOINTER WinLoadPointer (HWND hwndDesktop, HMODULE hmod, ULONG idres);
BOOL WinLockPointerUpdate (HWND hwndDesktop, HPOINTER hptrNew,
    ULONG ulTimeInterval);
BOOL WinQueryPointerPos (HWND hwndDesktop, PPOINTL pptl);
BOOL WinQueryPointerInfo (HPOINTER hptr, PPOINTERINFO pPointerInfo);
HPOINTER WinQuerySysPointer (HWND hwndDesktop, LONG lId, BOOL fCopy);
BOOL WinQuerySysPointerData (HWND hwndDesktop, ULONG ulId,
    PICONINFO pIconInfo);
BOOL WinSetPointer (HWND hwndDesktop, HPOINTER hptrNew);
BOOL WinSetPointerOwner (HPOINTER hptr, PID pid, BOOL fDestroy);
BOOL WinSetPointerPos (HWND hwndDesktop, LONG x, LONG y);
BOOL WinSetSysPointerData (HWND hwndDesktop, ULONG ulId,
    __const__ ICONINFO *pIconInfo);
BOOL WinShowPointer (HWND hwndDesktop, BOOL fShow);

#endif /* INCL_WINPOINTERS */


#if defined (INCL_WINRECTANGLES)

BOOL WinCopyRect (HAB hab, PRECTL prclDst, __const__ RECTL *prclSrc);
BOOL WinEqualRect (HAB hab, __const__ RECTL *prcl1, __const__ RECTL *prcl2);
BOOL WinInflateRect (HAB hab, PRECTL prcl, LONG cx, LONG cy);
BOOL WinIntersectRect (HAB hab, PRECTL prclDst, __const__ RECTL *prclSrc1,
    __const__ RECTL *prclSrc2);
BOOL WinIsRectEmpty (HAB hab, __const__ RECTL *prcl);
BOOL WinMakePoints (HAB hab, PPOINTL pwpt, ULONG cwpt);
BOOL WinMakeRect (HAB hab, PRECTL pwrc);
BOOL WinOffsetRect (HAB hab, PRECTL prcl, LONG cx, LONG cy);
BOOL WinPtInRect (HAB hab, __const__ RECTL *prcl, __const__ POINTL *pptl);
BOOL WinSetRect (HAB hab, PRECTL prcl, LONG xLeft, LONG yBottom, LONG xRight,
    LONG yTop);
BOOL WinSetRectEmpty (HAB hab, PRECTL prcl);
BOOL WinSubtractRect (HAB hab, PRECTL prclDst, __const__ RECTL *prclSrc1,
    __const__ RECTL *prclSrc2);
BOOL WinUnionRect (HAB hab, PRECTL prclDst, __const__ RECTL *prclSrc1,
    __const__ RECTL *prclSrc2);

#endif /* INCL_WINRECTANGLES */


#if defined (INCL_WINSCROLLBARS)

#define SB_LINEUP			1
#define SB_LINEDOWN			2
#define SB_LINELEFT			1
#define SB_LINERIGHT			2
#define SB_PAGEUP			3
#define SB_PAGEDOWN			4
#define SB_PAGELEFT			3
#define SB_PAGERIGHT			4
#define SB_SLIDERTRACK			5
#define SB_SLIDERPOSITION		6
#define SB_ENDSCROLL			7

#define SBM_SETSCROLLBAR		0x01a0
#define SBM_SETPOS			0x01a1
#define SBM_QUERYPOS			0x01a2
#define SBM_QUERYRANGE			0x01a3
#define SBM_SETTHUMBSIZE		0x01a6

#define SBS_HORZ			0
#define SBS_VERT			1
#define SBS_THUMBSIZE			2
#define SBS_AUTOTRACK			4
#define SBS_AUTOSIZE			0x2000

typedef struct _SBCDATA
{
  USHORT cb;
  USHORT sHilite;
  SHORT	 posFirst;
  SHORT	 posLast;
  SHORT	 posThumb;
  SHORT	 cVisible;
  SHORT	 cTotal;
} SBCDATA;
typedef SBCDATA *PSBCDATA;

#endif /* INCL_WINSCROLLBARS */


#if defined (INCL_WINSTATICS)

#define SM_SETHANDLE			0x0100
#define SM_QUERYHANDLE			0x0101

#define SS_TEXT				0x0001
#define SS_GROUPBOX			0x0002
#define SS_ICON				0x0003
#define SS_BITMAP			0x0004
#define SS_FGNDRECT			0x0005
#define SS_HALFTONERECT			0x0006
#define SS_BKGNDRECT			0x0007
#define SS_FGNDFRAME			0x0008
#define SS_HALFTONEFRAME		0x0009
#define SS_BKGNDFRAME			0x000a
#define SS_SYSICON			0x000b
#define SS_AUTOSIZE			0x0040

#define WM_MSGBOXINIT			0x010e
#define WM_MSGBOXDISMISS		0x010f

#endif /* INCL_WINSTATICS */


#if defined (INCL_WINSYS)

#define CCF_GLOBAL			0x0000
#define CCF_APPLICATION			0x0001
#define CCF_COUNTCOLORS			0x0010
#define CCF_ALLCOLORS			0x0020

#define CCI_FOREGROUND			1
#define CCI_FOREGROUNDREADONLY		2
#define CCI_BACKGROUND			3
#define CCI_BACKGROUNDDIALOG		4
#define CCI_DISABLEDFOREGROUND		5
#define CCI_DISABLEDFOREGROUNDREADONLY	6
#define CCI_DISABLEDBACKGROUND		7
#define CCI_DISABLEDBACKGROUNDDIALOG	8
#define CCI_HIGHLIGHTFOREGROUND		9
#define CCI_HIGHLIGHTBACKGROUND		10
#define CCI_HIGHLIGHTBACKGROUNDDIALOG	11
#define CCI_INACTIVEFOREGROUND		12
#define CCI_INACTIVEFOREGROUNDDIALOG	13
#define CCI_INACTIVEBACKGROUND		14
#define CCI_INACTIVEBACKGROUNDTEXT	15
#define CCI_ACTIVEFOREGROUND		16
#define CCI_ACTIVEFOREGROUNDDIALOG	17
#define CCI_ACTIVEBACKGROUND		18
#define CCI_ACTIVEBACKGROUNDTEXT	19
#define CCI_PAGEBACKGROUND		20
#define CCI_PAGEFOREGROUND		21
#define CCI_FIELDBACKGROUND		22
#define CCI_BORDER			23
#define CCI_BORDERLIGHT			24
#define CCI_BORDERDARK			25
#define CCI_BORDER2			26
#define CCI_BORDER2LIGHT		27
#define CCI_BORDER2DARK			28
#define CCI_BORDERDEFAULT		29
#define CCI_BUTTONBACKGROUND		30
#define CCI_BUTTONFOREGROUND		31
#define CCI_BUTTONBORDERLIGHT		32
#define CCI_BUTTONBORDERDARK		33
#define CCI_ARROW			34
#define CCI_DISABLEDARROW		35
#define CCI_ARROWBORDERLIGHT		36
#define CCI_ARROWBORDERDARK		37
#define CCI_CHECKLIGHT			38
#define CCI_CHECKMIDDLE			39
#define CCI_CHECKDARK			40
#define CCI_ICONFOREGROUND		41
#define CCI_ICONBACKGROUND		42
#define CCI_ICONBACKGROUNDDESKTOP	43
#define CCI_ICONHILITEFOREGROUND	44
#define CCI_ICONHILITEBACKGROUND	45
#define CCI_MAJORTABFOREGROUND		46
#define CCI_MAJORTABBACKGROUND		47
#define CCI_MINORTABFOREGROUND		48
#define CCI_MINORTABBACKGROUND		49
#define CCI_MAXINDEX			49

#define CCT_STATIC			1
#define CCT_STATICTEXT			2
#define CCT_GROUPBOX			3
#define CCT_PUSHBUTTON			4
#define CCT_CHECKBOX			5
#define CCT_RADIOBUTTON			6
#define CCT_ENTRYFIELD			7
#define CCT_LISTBOX			8
#define CCT_COMBOBOX			9
#define CCT_SCROLLBAR			10
#define CCT_FRAME			11
#define CCT_MENU			12
#define CCT_TITLEBAR			13
#define CCT_SPINBUTTON			14
#define CCT_SLIDER			15
#define CCT_CIRCULARSLIDER		16
#define CCT_VALUESET			17
#define CCT_MLE				18
#define CCT_CONTAINER			19
#define CCT_NOTEBOOK			20
#define CCT_MAXTYPE			20

#define CCV_NOTFOUND			(-1)
#define CCV_IGNORE			(-2)
#define CCV_DEFAULT			(-3)

#define PP_FOREGROUNDCOLOR		1
#define PP_FOREGROUNDCOLORINDEX		2
#define PP_BACKGROUNDCOLOR		3
#define PP_BACKGROUNDCOLORINDEX		4
#define PP_HILITEFOREGROUNDCOLOR	5
#define PP_HILITEFOREGROUNDCOLORINDEX	6
#define PP_HILITEBACKGROUNDCOLOR	7
#define PP_HILITEBACKGROUNDCOLORINDEX	8
#define PP_DISABLEDFOREGROUNDCOLOR	9
#define PP_DISABLEDFOREGROUNDCOLORINDEX 10
#define PP_DISABLEDBACKGROUNDCOLOR	11
#define PP_DISABLEDBACKGROUNDCOLORINDEX 12
#define PP_BORDERCOLOR			13
#define PP_BORDERCOLORINDEX		14
#define PP_FONTNAMESIZE			15
#define PP_FONTHANDLE			16
#define PP_RESERVED			17
#define PP_ACTIVECOLOR			18
#define PP_ACTIVECOLORINDEX		19
#define PP_INACTIVECOLOR		20
#define PP_INACTIVECOLORINDEX		21
#define PP_ACTIVETEXTFGNDCOLOR		22
#define PP_ACTIVETEXTFGNDCOLORINDEX	23
#define PP_ACTIVETEXTBGNDCOLOR		24
#define PP_ACTIVETEXTBGNDCOLORINDEX	25
#define PP_INACTIVETEXTFGNDCOLOR	26
#define PP_INACTIVETEXTFGNDCOLORINDEX	27
#define PP_INACTIVETEXTBGNDCOLOR	28
#define PP_INACTIVETEXTBGNDCOLORINDEX	29
#define PP_SHADOW			30
#define PP_MENUFOREGROUNDCOLOR		31
#define PP_MENUFOREGROUNDCOLORINDEX	32
#define PP_MENUBACKGROUNDCOLOR		33
#define PP_MENUBACKGROUNDCOLORINDEX	34
#define PP_MENUHILITEFGNDCOLOR		35
#define PP_MENUHILITEFGNDCOLORINDEX	36
#define PP_MENUHILITEBGNDCOLOR		37
#define PP_MENUHILITEBGNDCOLORINDEX	38
#define PP_MENUDISABLEDFGNDCOLOR	39
#define PP_MENUDISABLEDFGNDCOLORINDEX	40
#define PP_MENUDISABLEDBGNDCOLOR	41
#define PP_MENUDISABLEDBGNDCOLORINDEX	42
#define PP_SHADOWTEXTCOLOR		43
#define PP_SHADOWTEXTCOLORINDEX		44
#define PP_SHADOWHILITEFGNDCOLOR	45
#define PP_SHADOWHILITEFGNDCOLORINDEX	46
#define PP_SHADOWHILITEBGNDCOLOR	47
#define PP_SHADOWHILITEBGNDCOLORINDEX	48
#define PP_ICONTEXTBACKGROUNDCOLOR	49
#define PP_ICONTEXTBACKGROUNDCOLORINDEX	50
#define PP_BORDERLIGHTCOLOR		51
#define PP_BORDERDARKCOLOR		52
#define PP_BORDER2COLOR			53
#define PP_BORDER2LIGHTCOLOR		54
#define PP_BORDER2DARKCOLOR		55
#define PP_BORDERDEFAULTCOLOR		56
#define PP_FIELDBACKGROUNDCOLOR		57
#define PP_BUTTONBACKGROUNDCOLOR	58
#define PP_BUTTONBORDERLIGHTCOLOR	59
#define PP_BUTTONBORDERDARKCOLOR	60
#define PP_ARROWCOLOR			61
#define PP_ARROWBORDERLIGHTCOLOR	62
#define PP_ARROWBORDERDARKCOLOR		63
#define PP_ARROWDISABLEDCOLOR		64
#define PP_CHECKLIGHTCOLOR		65
#define PP_CHECKMIDDLECOLOR		66
#define PP_CHECKDARKCOLOR		67
#define PP_PAGEFOREGROUNDCOLOR		68
#define PP_PAGEBACKGROUNDCOLOR		69
#define PP_MAJORTABFOREGROUNDCOLOR	70
#define PP_MAJORTABBACKGROUNDCOLOR	71
#define PP_MINORTABFOREGROUNDCOLOR	72
#define PP_MINORTABBACKGROUNDCOLOR	73

#define PP_BIDI_FIRST			0x0100
#define PP_BIDI_LAST			0x012f
#define PP_USER				0x8000

#define QPF_NOINHERIT			0x0001
#define QPF_ID1COLORINDEX		0x0002
#define QPF_ID2COLORINDEX		0x0004
#define QPF_PURERGBCOLOR		0x0008
#define QPF_VALIDFLAGS			0x000f

#define SV_SWAPBUTTON			0
#define SV_DBLCLKTIME			1
#define SV_CXDBLCLK			2
#define SV_CYDBLCLK			3
#define SV_CXSIZEBORDER			4
#define SV_CYSIZEBORDER			5
#define SV_ALARM			6
#define SV_CURSORRATE			9
#define SV_FIRSTSCROLLRATE		10
#define SV_SCROLLRATE			11
#define SV_NUMBEREDLISTS		12
#define SV_WARNINGFREQ			13
#define SV_NOTEFREQ			14
#define SV_ERRORFREQ			15
#define SV_WARNINGDURATION		16
#define SV_NOTEDURATION			17
#define SV_ERRORDURATION		18
#define SV_CXSCREEN			20
#define SV_CYSCREEN			21
#define SV_CXVSCROLL			22
#define SV_CYHSCROLL			23
#define SV_CYVSCROLLARROW		24
#define SV_CXHSCROLLARROW		25
#define SV_CXBORDER			26
#define SV_CYBORDER			27
#define SV_CXDLGFRAME			28
#define SV_CYDLGFRAME			29
#define SV_CYTITLEBAR			30
#define SV_CYVSLIDER			31
#define SV_CXHSLIDER			32
#define SV_CXMINMAXBUTTON		33
#define SV_CYMINMAXBUTTON		34
#define SV_CYMENU			35
#define SV_CXFULLSCREEN			36
#define SV_CYFULLSCREEN			37
#define SV_CXICON			38
#define SV_CYICON			39
#define SV_CXPOINTER			40
#define SV_CYPOINTER			41
#define SV_DEBUG			42
#define SV_CMOUSEBUTTONS		43
#define SV_CPOINTERBUTTONS		43
#define SV_POINTERLEVEL			44
#define SV_CURSORLEVEL			45
#define SV_TRACKRECTLEVEL		46
#define SV_CTIMERS			47
#define SV_MOUSEPRESENT			48
#define SV_CXBYTEALIGN			49
#define SV_CXALIGN			49
#define SV_CYBYTEALIGN			50
#define SV_CYALIGN			50
#define SV_DESKTOPWORKAREAYTOP		51
#define SV_DESKTOPWORKAREAYBOTTOM	52
#define SV_DESKTOPWORKAREAXRIGHT	53
#define SV_DESKTOPWORKAREAXLEFT		54
#define SV_NOTRESERVED			56
#define SV_EXTRAKEYBEEP			57
#define SV_SETLIGHTS			58
#define SV_INSERTMODE			59
#define SV_MENUROLLDOWNDELAY		64
#define SV_MENUROLLUPDELAY		65
#define SV_ALTMNEMONIC			66
#define SV_TASKLISTMOUSEACCESS		67
#define SV_CXICONTEXTWIDTH		68
#define SV_CICONTEXTLINES		69
#define SV_CHORDTIME			70
#define SV_CXCHORD			71
#define SV_CYCHORD			72
#define SV_CXMOTIONSTART		73
#define SV_CYMOTIONSTART		74
#define SV_BEGINDRAG			75
#define SV_ENDDRAG			76
#define SV_SINGLESELECT			77
#define SV_OPEN				78
#define SV_CONTEXTMENU			79
#define SV_CONTEXTHELP			80
#define SV_TEXTEDIT			81
#define SV_BEGINSELECT			82
#define SV_ENDSELECT			83
#define SV_BEGINDRAGKB			84
#define SV_ENDDRAGKB			85
#define SV_SELECTKB			86
#define SV_OPENKB			87
#define SV_CONTEXTMENUKB		88
#define SV_CONTEXTHELPKB		89
#define SV_TEXTEDITKB			90
#define SV_BEGINSELECTKB		91
#define SV_ENDSELECTKB			92
#define SV_ANIMATION			93
#define SV_ANIMATIONSPEED		94
#define SV_MONOICONS			95
#define SV_KBDALTERED			96
#define SV_PRINTSCREEN			97
#define SV_LOCKSTARTINPUT		98
#define SV_DYNAMICDRAG			99
#define SV_CSYSVALUES			100

#define SYSCLR_SHADOWHILITEBGND		(-50)
#define SYSCLR_SHADOWHILITEFGND		(-49)
#define SYSCLR_SHADOWTEXT		(-48)
#define SYSCLR_ENTRYFIELD		(-47)
#define SYSCLR_MENUDISABLEDTEXT		(-46)
#define SYSCLR_MENUHILITE		(-45)
#define SYSCLR_MENUHILITEBGND		(-44)
#define SYSCLR_PAGEBACKGROUND		(-43)
#define SYSCLR_FIELDBACKGROUND		(-42)
#define SYSCLR_BUTTONLIGHT		(-41)
#define SYSCLR_BUTTONMIDDLE		(-40)
#define SYSCLR_BUTTONDARK		(-39)
#define SYSCLR_BUTTONDEFAULT		(-38)
#define SYSCLR_TITLEBOTTOM		(-37)
#define SYSCLR_SHADOW			(-36)
#define SYSCLR_ICONTEXT			(-35)
#define SYSCLR_DIALOGBACKGROUND		(-34)
#define SYSCLR_HILITEFOREGROUND		(-33)
#define SYSCLR_HILITEBACKGROUND		(-32)
#define SYSCLR_INACTIVETITLETEXTBGND	(-31)
#define SYSCLR_ACTIVETITLETEXTBGND	(-30)
#define SYSCLR_INACTIVETITLETEXT	(-29)
#define SYSCLR_ACTIVETITLETEXT		(-28)
#define SYSCLR_OUTPUTTEXT		(-27)
#define SYSCLR_WINDOWSTATICTEXT		(-26)
#define SYSCLR_SCROLLBAR		(-25)
#define SYSCLR_BACKGROUND		(-24)
#define SYSCLR_ACTIVETITLE		(-23)
#define SYSCLR_INACTIVETITLE		(-22)
#define SYSCLR_MENU			(-21)
#define SYSCLR_WINDOW			(-20)
#define SYSCLR_WINDOWFRAME		(-19)
#define SYSCLR_MENUTEXT			(-18)
#define SYSCLR_WINDOWTEXT		(-17)
#define SYSCLR_TITLETEXT		(-16)
#define SYSCLR_ACTIVEBORDER		(-15)
#define SYSCLR_INACTIVEBORDER		(-14)
#define SYSCLR_APPWORKSPACE		(-13)
#define SYSCLR_HELPBACKGROUND		(-12)
#define SYSCLR_HELPTEXT			(-11)
#define SYSCLR_HELPHILITE		(-10)

#define SYSCLR_CSYSCOLORS		41

#define WM_CTLCOLORCHANGE		0x0129
#define WM_QUERYCTLTYPE			0x0130 /*0x012a?*/


typedef struct _CTLCOLOR
{
  LONG clrIndex;
  LONG clrValue;
} CTLCOLOR;
typedef CTLCOLOR *PCTLCOLOR;

typedef struct _PARAM
{
  ULONG id;
  ULONG cb;
  BYTE	ab[1];
} PARAM;
typedef PARAM *NPPARAM;
typedef PARAM *PPARAM;

typedef struct _PRESPARAMS
{
  ULONG cb;
  PARAM aparam[1];
} PRESPARAMS;
typedef PRESPARAMS *NPPRESPARAMS;
typedef PRESPARAMS *PPRESPARAMS;


LONG WinQueryControlColors (HWND hwnd, LONG clrType, ULONG flCtlColor,
    ULONG cCtlColor, PCTLCOLOR pCtlColor);
ULONG WinQueryPresParam (HWND hwnd, ULONG id1, ULONG id2, PULONG pulId,
    ULONG cbBuf, PVOID pbBuf, ULONG fs);
LONG WinQuerySysColor (HWND hwndDesktop, LONG clr, LONG lReserved);
LONG WinQuerySysValue (HWND hwndDesktop, LONG iSysValue);
BOOL WinRemovePresParam (HWND hwnd, ULONG id);
LONG WinSetControlColors (HWND hwnd, LONG clrType, ULONG flCtlColor,
    ULONG cCtlColor, PCTLCOLOR pCtlColor);
BOOL WinSetPresParam (HWND hwnd, ULONG id, ULONG cbParam, PVOID pbParam);
BOOL WinSetSysColors (HWND hwndDesktop, ULONG flOptions, ULONG flFormat,
    LONG clrFirst, ULONG cclr, __const__ LONG *pclr);
BOOL WinSetSysValue (HWND hwndDesktop, LONG iSysValue, LONG lValue);

#endif /* INCL_WINSYS */


#if defined (INCL_WINTHUNKAPI)

PFN WinQueryClassThunkProc (PCSZ pszClassname);
LONG WinQueryWindowModel (HWND hwnd);
PFN WinQueryWindowThunkProc (HWND hwnd);
BOOL WinSetClassThunkProc (PCSZ pszClassname, PFN pfnThunkProc);
BOOL WinSetWindowThunkProc (HWND hwnd, PFN pfnThunkProc);

#endif /* INCL_WINTHUNKAPI */


#if defined (INCL_WINTIMER)

#define TID_CURSOR			0xffff
#define TID_SCROLL			0xfffe
#define TID_FLASHWINDOW			0xfffd
#define TID_USERMAX			0x7fff

ULONG  WinGetCurrentTime (HAB hab);
ULONG WinStartTimer (HAB hab, HWND hwnd, ULONG idTimer, ULONG dtTimeout);
BOOL WinStopTimer (HAB hab, HWND hwnd, ULONG idTimer);

#endif /* INCL_WINTIMER */

#if defined (INCL_WINTRACKRECT)

#define TF_LEFT				0x0001
#define TF_TOP				0x0002
#define TF_RIGHT			0x0004
#define TF_BOTTOM			0x0008
#define TF_SETPOINTERPOS		0x0010
#define TF_GRID				0x0020
#define TF_STANDARD			0x0040
#define TF_ALLINBOUNDARY		0x0080
#define TF_VALIDATETRACKRECT		0x0100
#define TF_PARTINBOUNDARY		0x0200

#define TF_MOVE				0x000f

typedef struct _TRACKINFO
{
  LONG	 cxBorder;
  LONG	 cyBorder;
  LONG	 cxGrid;
  LONG	 cyGrid;
  LONG	 cxKeyboard;
  LONG	 cyKeyboard;
  RECTL	 rclTrack;
  RECTL	 rclBoundary;
  POINTL ptlMinTrackSize;
  POINTL ptlMaxTrackSize;
  ULONG	 fs;
} TRACKINFO;
typedef TRACKINFO *PTRACKINFO;

BOOL WinShowTrackRect (HWND hwnd, BOOL fShow);
BOOL WinTrackRect (HWND hwnd, HPS hps, PTRACKINFO pti);

#endif /* INCL_WINTRACKRECT */

/* -------------------- MULTIPLE LINE ENTRIES ----------------------------- */

#if defined (INCL_WINMLE)

#define MLS_WORDWRAP			0x0001
#define MLS_BORDER			0x0002
#define MLS_VSCROLL			0x0004
#define MLS_HSCROLL			0x0008
#define MLS_READONLY			0x0010
#define MLS_IGNORETAB			0x0020
#define MLS_DISABLEUNDO			0x0040

#define MLFFMTRECT_FORMATRECT		0x0007
#define MLFFMTRECT_LIMITHORZ		0x0001
#define MLFFMTRECT_LIMITVERT		0x0002
#define MLFFMTRECT_MATCHWINDOW		0x0004

#define MLFIE_CFTEXT			0
#define MLFIE_NOTRANS			1
#define MLFIE_WINFMT			2
#define MLFIE_RTF			3

#define MLFEFR_RESIZE			0x0001
#define MLFEFR_TABSTOP			0x0002
#define MLFEFR_FONT			0x0004
#define MLFEFR_TEXT			0x0008
#define MLFEFR_WORDWRAP			0x0010
#define MLFETL_TEXTBYTES		0x0020

#define MLFMARGIN_LEFT			0x0001
#define MLFMARGIN_BOTTOM		0x0002
#define MLFMARGIN_RIGHT			0x0003
#define MLFMARGIN_TOP			0x0004

#define MLFQS_MINMAXSEL			0
#define MLFQS_MINSEL			1
#define MLFQS_MAXSEL			2
#define MLFQS_ANCHORSEL			3
#define MLFQS_CURSORSEL			4

#define MLFCLPBD_TOOMUCHTEXT		0x0001
#define MLFCLPBD_ERROR			0x0002

#define MLFSEARCH_CASESENSITIVE		0x0001
#define MLFSEARCH_SELECTMATCH		0x0002
#define MLFSEARCH_CHANGEALL		0x0004

#define MLM_SETTEXTLIMIT		0x01b0
#define MLM_QUERYTEXTLIMIT		0x01b1
#define MLM_SETFORMATRECT		0x01b2
#define MLM_QUERYFORMATRECT		0x01b3
#define MLM_SETWRAP			0x01b4
#define MLM_QUERYWRAP			0x01b5
#define MLM_SETTABSTOP			0x01b6
#define MLM_QUERYTABSTOP		0x01b7
#define MLM_SETREADONLY			0x01b8
#define MLM_QUERYREADONLY		0x01b9

#define MLM_QUERYCHANGED		0x01ba
#define MLM_SETCHANGED			0x01bb
#define MLM_QUERYLINECOUNT		0x01bc
#define MLM_CHARFROMLINE		0x01bd
#define MLM_LINEFROMCHAR		0x01be
#define MLM_QUERYLINELENGTH		0x01bf
#define MLM_QUERYTEXTLENGTH		0x01c0

#define MLM_FORMAT			0x01c1
#define MLM_SETIMPORTEXPORT		0x01c2
#define MLM_IMPORT			0x01c3
#define MLM_EXPORT			0x01c4
#define MLM_DELETE			0x01c6
#define MLM_QUERYFORMATLINELENGTH	0x01c7
#define MLM_QUERYFORMATTEXTLENGTH	0x01c8
#define MLM_INSERT			0x01c9

#define MLM_SETSEL			0x01ca
#define MLM_QUERYSEL			0x01cb
#define MLM_QUERYSELTEXT		0x01cc

#define MLM_QUERYUNDO			0x01cd
#define MLM_UNDO			0x01ce
#define MLM_RESETUNDO			0x01cf

#define MLM_QUERYFONT			0x01d0
#define MLM_SETFONT			0x01d1
#define MLM_SETTEXTCOLOR		0x01d2
#define MLM_QUERYTEXTCOLOR		0x01d3
#define MLM_SETBACKCOLOR		0x01d4
#define MLM_QUERYBACKCOLOR		0x01d5

#define MLM_QUERYFIRSTCHAR		0x01d6
#define MLM_SETFIRSTCHAR		0x01d7

#define MLM_CUT				0x01d8
#define MLM_COPY			0x01d9
#define MLM_PASTE			0x01da
#define MLM_CLEAR			0x01db

#define MLM_ENABLEREFRESH		0x01dc
#define MLM_DISABLEREFRESH		0x01dd

#define MLM_SEARCH			0x01de
#define MLM_QUERYIMPORTEXPORT		0x01df

#define MLN_OVERFLOW			0x0001
#define MLN_PIXHORZOVERFLOW		0x0002
#define MLN_PIXVERTOVERFLOW		0x0003
#define MLN_TEXTOVERFLOW		0x0004
#define MLN_VSCROLL			0x0005
#define MLN_HSCROLL			0x0006
#define MLN_CHANGE			0x0007
#define MLN_SETFOCUS			0x0008
#define MLN_KILLFOCUS			0x0009
#define MLN_MARGIN			0x000a
#define MLN_SEARCHPAUSE			0x000b
#define MLN_MEMERROR			0x000c
#define MLN_UNDOOVERFLOW		0x000d
#define MLN_CLPBDFAIL			0x000f


typedef LONG IPT;
typedef IPT *PIPT;
typedef LONG PIX;
typedef ULONG LINE;


typedef struct _FORMATRECT	/* Note 1 */
{
  LONG cxFormat;
  LONG cyFormat;
} MLEFORMATRECT;
typedef MLEFORMATRECT *PFORMATRECT;

typedef struct _MLECTLDATA
{
  USHORT cbCtlData;
  USHORT afIEFormat;
  ULONG	 cchText;
  IPT	 iptAnchor;
  IPT	 iptCursor;
  LONG	 cxFormat;
  LONG	 cyFormat;
  ULONG	 afFormatFlags;
  PVOID	 pHWXCtlData;
} MLECTLDATA;
typedef MLECTLDATA *PMLECTLDATA;

typedef struct _MLEOVERFLOW
{
  ULONG afErrInd;
  LONG	nBytesOver;
  LONG	pixHorzOver;
  LONG	pixVertOver;
} MLEOVERFLOW;
typedef MLEOVERFLOW *POVERFLOW;

typedef struct _MLEMARGSTRUCT
{
  USHORT afMargins;
  USHORT usMouMsg;
  IPT	 iptNear;
} MLEMARGSTRUCT;
typedef MLEMARGSTRUCT *PMARGSTRUCT;

typedef struct _SEARCH		/* Note 1 */
{
  USHORT cb;
  USHORT _pad;
  PCHAR	 pchFind;
  PCHAR	 pchReplace;
  SHORT	 cchFind;
  SHORT	 cchReplace;
  IPT	 iptStart;
  IPT	 iptStop;
  USHORT cchFound;
} MLE_SEARCHDATA;
typedef MLE_SEARCHDATA *PMLE_SEARCHDATA;

#endif /* INCL_WINMLE */

/* --------------- GRAPHICS PROGRAMMING INTERFACE ------------------------- */

#define GPI_ERROR			0
#define GPI_OK				1
#define GPI_ALTERROR			(-1)

#define HRGN_ERROR			((HRGN)(-1))

#define CLR_ERROR			(-255)
#define CLR_NOINDEX			(-254)
#define CLR_FALSE			(-5)
#define CLR_TRUE			(-4)
#define CLR_DEFAULT			(-3)
#define CLR_WHITE			(-2)
#define CLR_BLACK			(-1)
#define CLR_BACKGROUND			0
#define CLR_BLUE			1
#define CLR_RED				2
#define CLR_PINK			3
#define CLR_GREEN			4
#define CLR_CYAN			5
#define CLR_YELLOW			6
#define CLR_NEUTRAL			7
#define CLR_DARKGRAY			8
#define CLR_DARKBLUE			9
#define CLR_DARKRED			10
#define CLR_DARKPINK			11
#define CLR_DARKGREEN			12
#define CLR_DARKCYAN			13
#define CLR_BROWN			14
#define CLR_PALEGRAY			15

#define RGB_ERROR			(-255)
#define RGB_BLACK			0x00000000
#define RGB_BLUE			0x000000ff
#define RGB_GREEN			0x0000ff00
#define RGB_CYAN			0x0000ffff
#define RGB_RED				0x00ff0000
#define RGB_PINK			0x00ff00ff
#define RGB_YELLOW			0x00ffff00
#define RGB_WHITE			0x00ffffff

#define PRIM_LINE			1
#define PRIM_CHAR			2
#define PRIM_MARKER			3
#define PRIM_AREA			4
#define PRIM_IMAGE			5

#define AM_ERROR			(-1)
#define AM_PRESERVE			0
#define AM_NOPRESERVE			1

#define FM_ERROR			(-1)
#define FM_DEFAULT			0
#define FM_OR				1
#define FM_OVERPAINT			2
#define FM_LEAVEALONE			5

#define FM_XOR				4
#define FM_AND				6
#define FM_SUBTRACT			7
#define FM_MASKSRCNOT			8
#define FM_ZERO				9
#define FM_NOTMERGESRC			10
#define FM_NOTXORSRC			11
#define FM_INVERT			12
#define FM_MERGESRCNOT			13
#define FM_NOTCOPYSRC			14
#define FM_MERGENOTSRC			15
#define FM_NOTMASKSRC			16
#define FM_ONE				17

#define BM_ERROR			(-1)
#define BM_DEFAULT			0
#define BM_OR				1
#define BM_OVERPAINT			2
#define BM_LEAVEALONE			5

#define BM_XOR				4
#define BM_AND				6
#define BM_SUBTRACT			7
#define BM_MASKSRCNOT			8
#define BM_ZERO				9
#define BM_NOTMERGESRC			10
#define BM_NOTXORSRC			11
#define BM_INVERT			12
#define BM_MERGESRCNOT			13
#define BM_NOTCOPYSRC			14
#define BM_MERGENOTSRC			15
#define BM_NOTMASKSRC			16
#define BM_ONE				17
#define BM_SRCTRANSPARENT		18
#define BM_DESTTRANSPARENT		19

#define LINETYPE_ERROR			(-1)
#define LINETYPE_DEFAULT		0
#define LINETYPE_DOT			1
#define LINETYPE_SHORTDASH		2
#define LINETYPE_DASHDOT		3
#define LINETYPE_DOUBLEDOT		4
#define LINETYPE_LONGDASH		5
#define LINETYPE_DASHDOUBLEDOT		6
#define LINETYPE_SOLID			7
#define LINETYPE_INVISIBLE		8
#define LINETYPE_ALTERNATE		9

#define LINEWIDTH_ERROR			(-1)
#define LINEWIDTH_DEFAULT		0L
#define LINEWIDTH_NORMAL		0x00010000
#define LINEWIDTH_THICK			0x00020000

#define LINEWIDTHGEOM_ERROR		(-1)

#define LINEEND_ERROR			(-1)
#define LINEEND_DEFAULT			0
#define LINEEND_FLAT			1
#define LINEEND_SQUARE			2
#define LINEEND_ROUND			3

#define LINEJOIN_ERROR			(-1)
#define LINEJOIN_DEFAULT		0
#define LINEJOIN_BEVEL			1
#define LINEJOIN_ROUND			2
#define LINEJOIN_MITRE			3

#define CHDIRN_ERROR			(-1)
#define CHDIRN_DEFAULT			0
#define CHDIRN_LEFTRIGHT		1
#define CHDIRN_TOPBOTTOM		2
#define CHDIRN_RIGHTLEFT		3
#define CHDIRN_BOTTOMTOP		4

#define TA_NORMAL_HORIZ			0x0001
#define TA_LEFT				0x0002
#define TA_CENTER			0x0003
#define TA_RIGHT			0x0004
#define TA_STANDARD_HORIZ		0x0005
#define TA_NORMAL_VERT			0x0100
#define TA_TOP				0x0200
#define TA_HALF				0x0300
#define TA_BASE				0x0400
#define TA_BOTTOM			0x0500
#define TA_STANDARD_VERT		0x0600

#define CM_ERROR			(-1)
#define CM_DEFAULT			0
#define CM_MODE1			1
#define CM_MODE2			2
#define CM_MODE3			3

#define MARKSYM_ERROR			(-1)
#define MARKSYM_DEFAULT			0
#define MARKSYM_CROSS			1
#define MARKSYM_PLUS			2
#define MARKSYM_DIAMOND			3
#define MARKSYM_SQUARE			4
#define MARKSYM_SIXPOINTSTAR		5
#define MARKSYM_EIGHTPOINTSTAR		6
#define MARKSYM_SOLIDDIAMOND		7
#define MARKSYM_SOLIDSQUARE		8
#define MARKSYM_DOT			9
#define MARKSYM_SMALLCIRCLE		10
#define MARKSYM_BLANK			64

#define TXTBOX_TOPLEFT			0
#define TXTBOX_BOTTOMLEFT		1
#define TXTBOX_TOPRIGHT			2
#define TXTBOX_BOTTOMRIGHT		3
#define TXTBOX_CONCAT			4
#define TXTBOX_COUNT			5

#define PVIS_ERROR			0
#define PVIS_INVISIBLE			1
#define PVIS_VISIBLE			2

#define RVIS_ERROR			0
#define RVIS_INVISIBLE			1
#define RVIS_PARTIAL			2
#define RVIS_VISIBLE			3

#define FONT_DEFAULT			1
#define FONT_MATCH			2

#define LCIDT_FONT			6
#define LCIDT_BITMAP			7

#define LCID_ALL			(-1)

#define CHS_OPAQUE			0x0001
#define CHS_VECTOR			0x0002
#define CHS_LEAVEPOS			0x0008
#define CHS_CLIP			0x0010
#define CHS_UNDERSCORE			0x0200
#define CHS_STRIKEOUT			0x0400

#define FWEIGHT_DONT_CARE		0
#define FWEIGHT_ULTRA_LIGHT		1
#define FWEIGHT_EXTRA_LIGHT		2
#define FWEIGHT_LIGHT			3
#define FWEIGHT_SEMI_LIGHT		4
#define FWEIGHT_NORMAL			5
#define FWEIGHT_SEMI_BOLD		6
#define FWEIGHT_BOLD			7
#define FWEIGHT_EXTRA_BOLD		8
#define FWEIGHT_ULTRA_BOLD		9

#define FWIDTH_DONT_CARE		0
#define FWIDTH_ULTRA_CONDENSED		1
#define FWIDTH_EXTRA_CONDENSED		2
#define FWIDTH_CONDENSED		3
#define FWIDTH_SEMI_CONDENSED		4
#define FWIDTH_NORMAL			5
#define FWIDTH_SEMI_EXPANDED		6
#define FWIDTH_EXPANDED			7
#define FWIDTH_EXTRA_EXPANDED		8
#define FWIDTH_ULTRA_EXPANDED		9

#define FTYPE_ITALIC			0x0001
#define FTYPE_ITALIC_DONT_CARE		0x0002
#define FTYPE_OBLIQUE			0x0004
#define FTYPE_OBLIQUE_DONT_CARE		0x0008
#define FTYPE_ROUNDED			0x0010
#define FTYPE_ROUNDED_DONT_CARE		0x0020

#define QFA_PUBLIC			1
#define QFA_PRIVATE			2
#define QFA_ERROR			GPI_ALTERROR

#define QF_PUBLIC			0x0001
#define QF_PRIVATE			0x0002
#define QF_NO_GENERIC			0x0004
#define QF_NO_DEVICE			0x0008

#define QCD_LCT_FORMAT			0
#define QCD_LCT_LOINDEX			1
#define QCD_LCT_HIINDEX			2
#define QCD_LCT_OPTIONS			3

#define QLCT_ERROR			(-1)
#define QLCT_RGB			(-2)

#define QLCT_NOTLOADED			(-1)

#define PAL_ERROR			(-1)

#define PC_RESERVED			0x01
#define PC_EXPLICIT			0x02
#define PC_NOCOLLAPSE			0x04

#define SCP_ALTERNATE			0
#define SCP_WINDING			2
#define SCP_AND				4
#define SCP_RESET			0
#define SCP_INCL			0
#define SCP_EXCL			8

#define MPATH_STROKE			6

#define FPATH_ALTERNATE			0
#define FPATH_WINDING			2
#define FPATH_INCL			0
#define FPATH_EXCL			8

#define CVTC_WORLD			1
#define CVTC_MODEL			2
#define CVTC_DEFAULTPAGE		3
#define CVTC_PAGE			4
#define CVTC_DEVICE			5

#define TRANSFORM_REPLACE		0
#define TRANSFORM_ADD			1
#define TRANSFORM_PREEMPT		2

#define SEGEM_ERROR			0
#define SEGEM_INSERT			1
#define SEGEM_REPLACE			2

#define POLYGON_NOBOUNDARY		0x0000
#define POLYGON_BOUNDARY		0x0001

#define POLYGON_ALTERNATE		0x0000
#define POLYGON_WINDING			0x0002

#define POLYGON_INCL			0x0000
#define POLYGON_EXCL			0x0008

#define POLYGON_FILL			0x0000
#define POLYGON_NOFILL			0x0010

#define LCOL_RESET			0x0001
#define LCOL_REALIZABLE			0x0002
#define LCOL_PURECOLOR			0x0004
#define LCOL_OVERRIDE_DEFAULT_COLORS	0x0008
#define LCOL_REALIZED			0x0010

#define LCOLF_DEFAULT			0
#define LCOLF_INDRGB			1
#define LCOLF_CONSECRGB			2
#define LCOLF_RGB			3
#define LCOLF_PALETTE			4

#define LCOLOPT_REALIZED		0x0001
#define LCOLOPT_INDEX			0x0002

#define BA_NOBOUNDARY			0
#define BA_BOUNDARY			0x0001

#define BA_ALTERNATE			0
#define BA_WINDING			0x0002

#define BA_INCL				0
#define BA_EXCL				8

#define DRO_FILL			1
#define DRO_OUTLINE			2
#define DRO_OUTLINEFILL			3

#define PATSYM_ERROR			(-1)
#define PATSYM_DEFAULT			0
#define PATSYM_DENSE1			1
#define PATSYM_DENSE2			2
#define PATSYM_DENSE3			3
#define PATSYM_DENSE4			4
#define PATSYM_DENSE5			5
#define PATSYM_DENSE6			6
#define PATSYM_DENSE7			7
#define PATSYM_DENSE8			8
#define PATSYM_VERT			9
#define PATSYM_HORIZ			10
#define PATSYM_DIAG1			11
#define PATSYM_DIAG2			12
#define PATSYM_DIAG3			13
#define PATSYM_DIAG4			14
#define PATSYM_NOSHADE			15
#define PATSYM_SOLID			16
#define PATSYM_HALFTONE			17
#define PATSYM_HATCH			18
#define PATSYM_DIAGHATCH		19
#define PATSYM_BLANK			64

#define LCID_ERROR			(-1)
#define LCID_DEFAULT			0

#define CRGN_OR				1
#define CRGN_COPY			2
#define CRGN_XOR			4
#define CRGN_AND			6
#define CRGN_DIFF			7

#define RGN_ERROR			0
#define RGN_NULL			1
#define RGN_RECT			2
#define RGN_COMPLEX			3

#define PRGN_ERROR			0
#define PRGN_OUTSIDE			1
#define PRGN_INSIDE			2

#define RRGN_ERROR			0
#define RRGN_OUTSIDE			1
#define RRGN_PARTIAL			2
#define RRGN_INSIDE			3

#define EQRGN_ERROR			0
#define EQRGN_NOTEQUAL			1
#define EQRGN_EQUAL			2

#define RECTDIR_LFRT_TOPBOT		1
#define RECTDIR_RTLF_TOPBOT		2
#define RECTDIR_LFRT_BOTTOP		3
#define RECTDIR_RTLF_BOTTOP		4

#define PMF_SEGBASE			0
#define PMF_LOADTYPE			1
#define PMF_RESOLVE			2
#define PMF_LCIDS			3
#define PMF_RESET			4
#define PMF_SUPPRESS			5
#define PMF_COLORTABLES			6
#define PMF_COLORREALIZABLE		7
#define PMF_DEFAULTS			8
#define PMF_DELETEOBJECTS		9

#define RS_DEFAULT			0
#define RS_NODISCARD			1
#define LC_DEFAULT			0
#define LC_NOLOAD			1
#define LC_LOADDISC			3
#define LT_DEFAULT			0
#define LT_NOMODIFY			1
#define LT_ORIGINALVIEW			4
#define RES_DEFAULT			0
#define RES_NORESET			1
#define RES_RESET			2
#define SUP_DEFAULT			0
#define SUP_NOSUPPRESS			1
#define SUP_SUPPRESS			2
#define CTAB_DEFAULT			0
#define CTAB_NOMODIFY			1
#define CTAB_REPLACE			3
#define CTAB_REPLACEPALETTE		4
#define CREA_DEFAULT			0
#define CREA_REALIZE			1
#define CREA_NOREALIZE			2
#define CREA_DOREALIZE			3

#define DDEF_DEFAULT			0
#define DDEF_IGNORE			1
#define DDEF_LOADDISC			3
#define DOBJ_DEFAULT			0
#define DOBJ_NODELETE			1
#define DOBJ_DELETE			2
#define RSP_DEFAULT			0
#define RSP_NODISCARD			1

#define LBB_COLOR			0x0001
#define LBB_BACK_COLOR			0x0002
#define LBB_MIX_MODE			0x0004
#define LBB_BACK_MIX_MODE		0x0008
#define LBB_WIDTH			0x0010
#define LBB_GEOM_WIDTH			0x0020
#define LBB_TYPE			0x0040
#define LBB_END				0x0080
#define LBB_JOIN			0x0100

#define CBB_COLOR			0x0001
#define CBB_BACK_COLOR			0x0002
#define CBB_MIX_MODE			0x0004
#define CBB_BACK_MIX_MODE		0x0008
#define CBB_SET				0x0010
#define CBB_MODE			0x0020
#define CBB_BOX				0x0040
#define CBB_ANGLE			0x0080
#define CBB_SHEAR			0x0100
#define CBB_DIRECTION			0x0200
#define CBB_TEXT_ALIGN			0x0400
#define CBB_EXTRA			0x0800
#define CBB_BREAK_EXTRA			0x1000

#define MBB_COLOR			0x0001
#define MBB_BACK_COLOR			0x0002
#define MBB_MIX_MODE			0x0004
#define MBB_BACK_MIX_MODE		0x0008
#define MBB_SET				0x0010
#define MBB_SYMBOL			0x0020
#define MBB_BOX				0x0040

#define ABB_COLOR			0x0001
#define ABB_BACK_COLOR			0x0002
#define ABB_MIX_MODE			0x0004
#define ABB_BACK_MIX_MODE		0x0008
#define ABB_SET				0x0010
#define ABB_SYMBOL			0x0020
#define ABB_REF_POINT			0x0040

#define IBB_COLOR			0x0001
#define IBB_BACK_COLOR			0x0002
#define IBB_MIX_MODE			0x0004
#define IBB_BACK_MIX_MODE		0x0008


typedef PVOID PBUNDLE;

typedef LONG FIXED;
typedef FIXED *PFIXED;

typedef LHANDLE HMF;
typedef HMF *PHMF;


typedef struct _SIZEL
{
  LONG cx;
  LONG cy;
} SIZEL;
typedef SIZEL *PSIZEL;

typedef struct _RGNRECT
{
  ULONG ircStart;
  ULONG crc;
  ULONG crcReturned;
  ULONG ulDirection;
} RGNRECT;
typedef RGNRECT *PRGNRECT;

typedef struct _MATRIXLF
{
  FIXED fxM11;
  FIXED fxM12;
  LONG	lM13;
  FIXED fxM21;
  FIXED fxM22;
  LONG	lM23;
  LONG	lM31;
  LONG	lM32;
  LONG	lM33;
} MATRIXLF;
typedef MATRIXLF *PMATRIXLF;

typedef struct _ARCPARAMS
{
  LONG lP;
  LONG lQ;
  LONG lR;
  LONG lS;
} ARCPARAMS;
typedef ARCPARAMS *PARCPARAMS;

typedef struct _SIZEF
{
  FIXED cx;
  FIXED cy;
} SIZEF;
typedef SIZEF *PSIZEF;

typedef struct _POLYGON
{
  ULONG	  ulPoints;
  PPOINTL aPointl;
} POLYGON;
typedef POLYGON *PPOLYGON;

typedef struct _POLYSET
{
  ULONG	  ulPolys;
  POLYGON aPolygon[1];
} POLYSET;
typedef POLYSET *PPOLYSET;

typedef struct _GRADIENTL
{
  LONG x;
  LONG y;
} GRADIENTL;
typedef GRADIENTL *PGRADIENTL;

typedef struct _KERNINGPAIRS
{
  SHORT sFirstChar;
  SHORT sSecondChar;
  LONG	lKerningAmount;
} KERNINGPAIRS;
typedef KERNINGPAIRS *PKERNINGPAIRS;

typedef struct _FACENAMEDESC
{
  USHORT usSize;
  USHORT usWeightClass;
  USHORT usWidthClass;
  USHORT usReserved;
  ULONG	 flOptions;
} FACENAMEDESC;
typedef FACENAMEDESC *PFACENAMEDESC;

typedef CHAR FFDESCS[2][FACESIZE];
typedef FFDESCS *PFFDESCS;

typedef struct _FFDESCS2
{
  ULONG cbLength;
  ULONG cbFacenameOffset;
  BYTE	abFamilyName[1];
} FFDESCS2;
typedef FFDESCS2 *PFFDESCS2;


typedef struct _LINEBUNDLE
{
  LONG	 lColor;
  LONG	 lBackColor;
  USHORT usMixMode;
  USHORT usBackMixMode;
  FIXED	 fxWidth;
  LONG	 lGeomWidth;
  USHORT usType;
  USHORT usEnd;
  USHORT usJoin;
  USHORT usReserved;
} LINEBUNDLE;
typedef LINEBUNDLE *PLINEBUNDLE;

typedef struct _CHARBUNDLE
{
  LONG	 lColor;
  LONG	 lBackColor;
  USHORT usMixMode;
  USHORT usBackMixMode;
  USHORT usSet;
  USHORT usPrecision;
  SIZEF	 sizfxCell;
  POINTL ptlAngle;
  POINTL ptlShear;
  USHORT usDirection;
  USHORT usTextAlign;
  FIXED	 fxExtra;
  FIXED	 fxBreakExtra;
} CHARBUNDLE;
typedef CHARBUNDLE *PCHARBUNDLE;

typedef struct _MARKERBUNDLE
{
  LONG	 lColor;
  LONG	 lBackColor;
  USHORT usMixMode;
  USHORT usBackMixMode;
  USHORT usSet;
  USHORT usSymbol;
  SIZEF	 sizfxCell;
} MARKERBUNDLE;
typedef MARKERBUNDLE *PMARKERBUNDLE;

typedef struct _AREABUNDLE
{
  LONG	 lColor;
  LONG	 lBackColor;
  USHORT usMixMode;
  USHORT usBackMixMode;
  USHORT usSet;
  USHORT usSymbol;
  POINTL ptlRefPoint;
} AREABUNDLE;
typedef AREABUNDLE *PAREABUNDLE;

typedef struct _IMAGEBUNDLE
{
  LONG	 lColor;
  LONG	 lBackColor;
  USHORT usMixMode;
  USHORT usBackMixMode;
} IMAGEBUNDLE;
typedef IMAGEBUNDLE *PIMAGEBUNDLE;


#define MAKEFIXED(i,f) MAKELONG(f,i)
#define FIXEDFRAC(fx)  (LOUSHORT(fx))
#define FIXEDINT(fx)   ((SHORT)HIUSHORT(fx))


LONG GpiAnimatePalette (HPAL hpal, ULONG ulFormat, ULONG ulStart,
    ULONG ulCount, __const__ ULONG *aulTable);
BOOL GpiBeginArea (HPS hps, ULONG flOptions);
BOOL GpiBeginElement (HPS hps, LONG lType, PCSZ pszDesc);
BOOL GpiBeginPath (HPS hps, LONG lPath);
LONG GpiBox (HPS hps, LONG lControl, __const__ POINTL *pptlPoint, LONG lHRound,
    LONG lVRound);
LONG GpiCallSegmentMatrix (HPS hps, LONG lSegment, LONG lCount,
    __const__ MATRIXLF *pmatlfArray, LONG lOptions);
LONG GpiCharString (HPS hps, LONG lCount, PCCH pchString);
LONG GpiCharStringAt (HPS hps, __const__ POINTL *pptlPoint, LONG lCount,
     PCCH pchString);
LONG GpiCharStringPos (HPS hps, __const__ RECTL *prclRect, ULONG flOptions,
    LONG lCount, PCCH pchString, __const__ LONG *alAdx);
LONG  GpiCharStringPosAt (HPS hps, __const__ POINTL *pptlStart,
    __const__ RECTL *prclRect, ULONG flOptions, LONG lCount, PCCH pchString,
    __const__ LONG *alAdx);
BOOL GpiCloseFigure (HPS hps);
LONG GpiCombineRegion (HPS hps, HRGN hrgnDest, HRGN hrgnSrc1, HRGN hrgnSrc2,
    LONG lMode);
BOOL GpiComment (HPS hps, LONG lLength, __const__ BYTE *pbData);
BOOL GpiConvert (HPS hps, LONG lSrc, LONG lTarg, LONG lCount,
    PPOINTL aptlPoints);
BOOL GpiConvertWithMatrix (HPS hps, LONG lCountp, PPOINTL aptlPoints,
    LONG lCount, __const__ MATRIXLF *pmatlfArray);
HMF GpiCopyMetaFile (HMF hmf);
BOOL GpiCreateLogColorTable (HPS hps, ULONG flOptions, LONG lFormat,
    LONG lStart, LONG lCount, __const__ LONG *alTable);
LONG GpiCreateLogFont (HPS hps, __const__ STR8 *pName, LONG lLcid,
    __const__ FATTRS *pfatAttrs);
HPAL GpiCreatePalette (HAB hab, ULONG flOptions, ULONG ulFormat,
    ULONG ulCount, __const__ ULONG *aulTable);
HRGN GpiCreateRegion (HPS hps, LONG lCount, __const__ RECTL *arclRectangles);
BOOL GpiDeleteElement (HPS hps);
BOOL GpiDeleteElementRange (HPS hps, LONG lFirstElement, LONG lLastElement);
BOOL GpiDeleteElementsBetweenLabels (HPS hps, LONG lFirstLabel,
    LONG lLastLabel);
BOOL GpiDeleteMetaFile (HMF hmf);
BOOL GpiDeletePalette (HPAL hpal);
BOOL GpiDeleteSetId (HPS hps, LONG lLcid);
BOOL GpiDestroyRegion (HPS hps, HRGN hrgn);
LONG GpiElement (HPS hps, LONG lType, PCSZ pszDesc, LONG lLength,
    __const__ BYTE *pbData);
LONG GpiEndArea (HPS hps);
BOOL GpiEndElement (HPS hps);
BOOL GpiEndPath (HPS hps);
LONG GpiEqualRegion (HPS hps, HRGN hrgnSrc1, HRGN hrgnSrc2);
LONG GpiExcludeClipRectangle (HPS hps, __const__ RECTL *prclRectangle);
LONG GpiFillPath (HPS hps, LONG lPath, LONG lOptions);
LONG GpiFrameRegion (HPS hps, HRGN hrgn, __const__ SIZEL *thickness);
LONG GpiFullArc (HPS hps, LONG lControl, FIXED fxMultiplier);
LONG GpiImage (HPS hps, LONG lFormat, __const__ SIZEL *psizlImageSize,
    LONG lLength, __const__ BYTE *pbData);
LONG GpiIntersectClipRectangle (HPS hps, __const__ RECTL *prclRectangle);
BOOL GpiLabel (HPS hps, LONG lLabel);
LONG GpiLine (HPS hps, __const__ POINTL *pptlEndPoint);
BOOL GpiLoadFonts (HAB hab, PCSZ pszFilename);
HMF GpiLoadMetaFile (HAB hab, PCSZ pszFilename);
BOOL GpiLoadPublicFonts (HAB hab, PCSZ pszFileName);
LONG GpiMarker (HPS hps, __const__ POINTL *pptlPoint);
BOOL GpiModifyPath (HPS hps, LONG lPath, LONG lMode);
BOOL GpiMove (HPS hps, __const__ POINTL *pptlPoint);
LONG GpiOffsetClipRegion (HPS hps, __const__ POINTL *pptlPoint);
BOOL GpiOffsetElementPointer (HPS hps, LONG loffset);
BOOL GpiOffsetRegion (HPS hps, HRGN Hrgn, __const__ POINTL *pptlOffset);
LONG GpiOutlinePath (HPS hps, LONG lPath, LONG lOptions);
LONG GpiPaintRegion (HPS hps, HRGN hrgn);
LONG GpiPartialArc (HPS hps, __const__ POINTL *pptlCenter, FIXED fxMultiplier,
    FIXED fxStartAngle, FIXED fxSweepAngle);
HRGN GpiPathToRegion (HPS GpiH, LONG lPath, LONG lOptions);
LONG GpiPlayMetaFile (HPS hps, HMF hmf, LONG lCount1,
    __const__ LONG *alOptarray, PLONG plSegCount, LONG lCount2, PSZ pszDesc);
LONG GpiPointArc (HPS hps, __const__ POINTL *pptl2);
LONG GpiPolyFillet (HPS hps, LONG lCount, __const__ POINTL *aptlPoints);
LONG GpiPolyFilletSharp (HPS hps, LONG lCount, __const__ POINTL *aptlPoints,
    __const__ FIXED *afxPoints);
LONG GpiPolygons (HPS hps, ULONG ulCount, __const__ POLYGON *paplgn,
    ULONG flOptions, ULONG flModel);
LONG GpiPolyLine (HPS hps, LONG lCount, __const__ POINTL *aptlPoints);
LONG GpiPolyLineDisjoint (HPS hps, LONG lCount, __const__ POINTL *aptlPoints);
LONG GpiPolyMarker (HPS hps, LONG lCount, __const__ POINTL *aptlPoints);
LONG GpiPolySpline (HPS hps, LONG lCount, __const__ POINTL *aptlPoints);
BOOL GpiPop (HPS hps, LONG lCount);
LONG GpiPtInRegion (HPS hps, HRGN hrgn, __const__ POINTL *pptlPoint);
LONG GpiPtVisible (HPS hps, __const__ POINTL *pptlPoint);
BOOL GpiQueryArcParams (HPS hps, PARCPARAMS parcpArcParams);
LONG GpiQueryAttrMode (HPS hps);
LONG GpiQueryAttrs (HPS hps, LONG lPrimType, ULONG flAttrMask,
    PBUNDLE ppbunAttrs);
LONG GpiQueryBackColor (HPS hps);
LONG GpiQueryBackMix (HPS hps);
BOOL GpiQueryCharAngle (HPS hps, PGRADIENTL pgradlAngle);
BOOL GpiQueryCharBox (HPS hps, PSIZEF psizfxSize);
BOOL GpiQueryCharBreakExtra (HPS hps, PFIXED BreakExtra);
LONG GpiQueryCharDirection (HPS hps);
BOOL GpiQueryCharExtra (HPS hps, PFIXED Extra);
LONG GpiQueryCharMode (HPS hps);
LONG GpiQueryCharSet (HPS hps);
BOOL GpiQueryCharShear (HPS hps, PPOINTL pptlShear);
BOOL GpiQueryCharStringPos (HPS hps, ULONG flOptions, LONG lCount,
    PCCH pchString, PLONG alXincrements, PPOINTL aptlPositions);
BOOL GpiQueryCharStringPosAt (HPS hps, PPOINTL pptlStart, ULONG flOptions,
    LONG lCount, PCCH pchString, PLONG alXincrements, PPOINTL aptlPositions);
LONG GpiQueryClipBox (HPS hps, PRECTL prclBound);
HRGN GpiQueryClipRegion (HPS hps);
LONG GpiQueryColor (HPS hps);
BOOL GpiQueryColorData (HPS hps, LONG lCount, PLONG alArray);
LONG GpiQueryColorIndex (HPS hps, ULONG flOptions, LONG lRgbColor);
ULONG GpiQueryCp (HPS hps);
BOOL GpiQueryCurrentPosition (HPS hps, PPOINTL pptlPoint);
BOOL GpiQueryDefArcParams (HPS hps, PARCPARAMS parcpArcParams);
BOOL GpiQueryDefAttrs (HPS hps, LONG lPrimType, ULONG flAttrMask,
    PBUNDLE ppbunAttrs);
BOOL GpiQueryDefCharBox (HPS hps, PSIZEL psizlSize);
BOOL GpiQueryDefTag (HPS hps, PLONG plTag);
BOOL GpiQueryDefViewingLimits (HPS hps, PRECTL prclLimits);
BOOL GpiQueryDefaultViewMatrix (HPS hps, LONG lCount, PMATRIXLF pmatlfArray);
LONG GpiQueryEditMode (HPS hps);
LONG GpiQueryElement (HPS hps, LONG lOff, LONG lMaxLength, PBYTE pbData);
LONG GpiQueryElementPointer (HPS hps);
LONG GpiQueryElementType (HPS hps, PLONG plType, LONG lLength, PSZ pszData);
ULONG GpiQueryFaceString (HPS PS, PCSZ FamilyName, PFACENAMEDESC attrs,
    LONG length, PSZ CompoundFaceName);
ULONG GpiQueryFontAction (HAB anchor, ULONG options);
LONG GpiQueryFontFileDescriptions (HAB hab, PCSZ pszFilename, PLONG plCount,
    PFFDESCS affdescsNames);
BOOL GpiQueryFontMetrics (HPS hps, LONG lMetricsLength,
    PFONTMETRICS pfmMetrics);
LONG GpiQueryFonts (HPS hps, ULONG flOptions, PCSZ pszFacename,
    PLONG plReqFonts, LONG lMetricsLength, PFONTMETRICS afmMetrics);
LONG GpiQueryFullFontFileDescs (HAB hab, PCSZ pszFilename, PLONG plCount,
    PVOID pNames, PLONG plNamesBuffLength);
BOOL GpiQueryGraphicsField (HPS hps, PRECTL prclField);
LONG GpiQueryKerningPairs (HPS hps, LONG lCount, PKERNINGPAIRS akrnprData);
LONG GpiQueryLineEnd (HPS hps);
LONG GpiQueryLineJoin (HPS hps);
LONG GpiQueryLineType (HPS hps);
FIXED GpiQueryLineWidth (HPS hps);
LONG GpiQueryLineWidthGeom (HPS hps);
LONG GpiQueryLogColorTable (HPS hps, ULONG flOptions, LONG lStart, LONG lCount,
    PLONG alArray);
BOOL GpiQueryLogicalFont (HPS PS, LONG lcid, PSTR8 name, PFATTRS attrs,
    LONG length);
LONG GpiQueryMarker (HPS hps);
BOOL GpiQueryMarkerBox (HPS hps, PSIZEF psizfxSize);
LONG GpiQueryMarkerSet (HPS hps);
BOOL GpiQueryMetaFileBits (HMF hmf, LONG lOffset, LONG lLength, PBYTE pbData);
LONG GpiQueryMetaFileLength (HMF hmf);
LONG GpiQueryMix (HPS hps);
BOOL GpiQueryModelTransformMatrix (HPS hps, LONG lCount,
    PMATRIXLF pmatlfArray);
LONG GpiQueryNearestColor (HPS hps, ULONG flOptions, LONG lRgbIn);
LONG GpiQueryNumberSetIds (HPS hps);
BOOL GpiQueryPageViewport (HPS hps, PRECTL prclViewport);
HPAL GpiQueryPalette (HPS hps);
LONG GpiQueryPaletteInfo (HPAL hpal, HPS  hps, ULONG flOptions,
    ULONG ulStart, ULONG ulCount, PULONG aulArray);
LONG GpiQueryPattern (HPS hps);
BOOL GpiQueryPatternRefPoint (HPS hps, PPOINTL pptlRefPoint);
LONG GpiQueryPatternSet (HPS hps);
LONG GpiQueryRealColors (HPS hps, ULONG flOptions, LONG lStart, LONG lCount,
    PLONG alColors);
LONG GpiQueryRegionBox (HPS hps, HRGN hrgn, PRECTL prclBound);
BOOL GpiQueryRegionRects (HPS hps, HRGN hrgn, PRECTL prclBound,
    PRGNRECT prgnrcControl, PRECTL prclRect);
LONG GpiQueryRGBColor (HPS hps, ULONG flOptions, LONG lColorIndex);
BOOL GpiQuerySegmentTransformMatrix (HPS hps, LONG lSegid, LONG lCount,
    PMATRIXLF pmatlfArray);
BOOL GpiQuerySetIds (HPS hps, LONG lCount, PLONG alTypes, PSTR8 aNames,
    PLONG allcids);
BOOL GpiQueryTextAlignment (HPS hps, PLONG plHoriz, PLONG plVert);
BOOL GpiQueryTextBox (HPS hps, LONG lCount1, PCH pchString, LONG lCount2,
    PPOINTL aptlPoints);
BOOL GpiQueryViewingLimits (HPS hps, PRECTL prclLimits);
BOOL GpiQueryViewingTransformMatrix (HPS hps, LONG lCount,
    PMATRIXLF pmatlfArray);
BOOL GpiQueryWidthTable (HPS hps, LONG lFirstChar, LONG lCount, PLONG alData);
LONG GpiRectInRegion (HPS hps, HRGN hrgn, __const__ RECTL *prclRect);
LONG GpiRectVisible (HPS hps, __const__ RECTL *prclRectangle);
BOOL GpiRotate (HPS hps, PMATRIXLF pmatlfArray, LONG lOptions, FIXED fxAngle,
    __const__ POINTL *pptlCenter);
BOOL GpiSaveMetaFile (HMF hmf, PCSZ pszFilename);
BOOL GpiScale (HPS hps, PMATRIXLF pmfatlfArray, LONG lOptions,
    __const__ FIXED *afxScale, __const__ POINTL *pptlCenter);
HPAL GpiSelectPalette (HPS hps, HPAL hpal);
BOOL GpiSetArcParams (HPS hps, __const__ ARCPARAMS *parcpArcParams);
BOOL GpiSetAttrMode (HPS hps, LONG lMode);
BOOL GpiSetAttrs (HPS hps, LONG lPrimType, ULONG flAttrMask, ULONG flDefMask,
    __const__ VOID *ppbunAttrs);
BOOL GpiSetBackColor (HPS hps, LONG lColor);
BOOL GpiSetBackMix (HPS hps, LONG lMixMode);
BOOL GpiSetCharAngle (HPS hps, __const__ GRADIENTL *pgradlAngle);
BOOL GpiSetCharBox (HPS hps, __const__ SIZEF *psizfxBox);
BOOL GpiSetCharBreakExtra (HPS hps, FIXED BreakExtra);
BOOL GpiSetCharDirection (HPS hps, LONG lDirection);
BOOL GpiSetCharExtra (HPS hps, FIXED  Extra);
BOOL GpiSetCharMode (HPS hps, LONG lMode);
BOOL GpiSetCharSet (HPS hps, LONG llcid);
BOOL GpiSetCharShear (HPS hps, __const__ POINTL *pptlAngle);
BOOL GpiSetClipPath (HPS hps, LONG lPath, LONG lOptions);
LONG GpiSetClipRegion (HPS hps, HRGN hrgn, PHRGN phrgnOld);
BOOL GpiSetColor (HPS hps, LONG lColor);
BOOL GpiSetCp (HPS hps, ULONG ulCodePage);
BOOL GpiSetCurrentPosition (HPS hps, __const__ POINTL *pptlPoint);
BOOL GpiSetDefArcParams (HPS hps, __const__ ARCPARAMS *parcpArcParams);
BOOL GpiSetDefAttrs (HPS hps, LONG lPrimType, ULONG flAttrMask,
    __const__ VOID *ppbunAttrs);
BOOL GpiSetDefaultViewMatrix (HPS hps, LONG lCount,
    __const__ MATRIXLF *pmatlfarray, LONG lOptions);
BOOL GpiSetDefTag (HPS hps, LONG lTag);
BOOL GpiSetDefViewingLimits (HPS hps, __const__ RECTL *prclLimits);
BOOL GpiSetEditMode (HPS hps, LONG lMode);
BOOL GpiSetElementPointer (HPS hps, LONG lElement);
BOOL GpiSetElementPointerAtLabel (HPS hps, LONG lLabel);
BOOL GpiSetGraphicsField (HPS hps, __const__ RECTL *prclField);
BOOL GpiSetLineEnd (HPS hps, LONG lLineEnd);
BOOL GpiSetLineJoin (HPS hps, LONG lLineJoin);
BOOL GpiSetLineType (HPS hps, LONG lLineType);
BOOL GpiSetLineWidth (HPS hps, FIXED fxLineWidth);
BOOL GpiSetLineWidthGeom (HPS hps, LONG lLineWidth);
BOOL GpiSetMarker (HPS hps, LONG lSymbol);
BOOL GpiSetMarkerBox (HPS hps, __const__ SIZEF *psizfxSize);
BOOL GpiSetMarkerSet (HPS hps, LONG lSet);
BOOL GpiSetMetaFileBits (HMF hmf, LONG lOffset, LONG lLength,
    __const__ BYTE *pbBuffer);
BOOL GpiSetMix (HPS hps, LONG lMixMode);
BOOL GpiSetModelTransformMatrix (HPS hps, LONG lCount,
    __const__ MATRIXLF *pmatlfArray, LONG lOptions);
BOOL GpiSetPageViewport (HPS hps, __const__ RECTL *prclViewport);
BOOL GpiSetPaletteEntries (HPAL hpal, ULONG ulFormat, ULONG ulStart,
    ULONG ulCount, __const__ ULONG *aulTable);
BOOL GpiSetPattern (HPS hps, LONG lPatternSymbol);
BOOL GpiSetPatternRefPoint (HPS hps, __const__ POINTL *pptlRefPoint);
BOOL GpiSetPatternSet (HPS hps, LONG lSet);
BOOL GpiSetRegion (HPS hps, HRGN hrgn, LONG lcount,
    __const__ RECTL *arclRectangles);
BOOL GpiSetSegmentTransformMatrix (HPS hps, LONG lSegid, LONG lCount,
    __const__ MATRIXLF *pmatlfarray, LONG lOptions);
BOOL GpiSetTextAlignment (HPS hps, LONG lHoriz, LONG lVert);
BOOL GpiSetViewingLimits (HPS hps, __const__ RECTL *prclLimits);
BOOL GpiSetViewingTransformMatrix (HPS hps, LONG lCount,
    __const__ MATRIXLF *pmatlfArray, LONG lOptions);
LONG GpiStrokePath (HPS hps, LONG lPath, ULONG flOptions);
BOOL GpiTranslate (HPS hps, PMATRIXLF pmatlfArray, LONG lOptions,
    __const__ POINTL *pptlTranslation);
BOOL GpiUnloadFonts (HAB hab, PCSZ pszFilename);
BOOL GpiUnloadPublicFonts (HAB hab, PCSZ pszFilename);


#if defined (INCL_GPIBITMAPS) || !defined (INCL_NOCOMMON)

#define ROP_SRCCOPY			0x00cc
#define ROP_SRCPAINT			0x00ee
#define ROP_SRCAND			0x0088
#define ROP_SRCINVERT			0x0066
#define ROP_SRCERASE			0x0044
#define ROP_NOTSRCCOPY			0x0033
#define ROP_NOTSRCERASE			0x0011
#define ROP_MERGECOPY			0x00c0
#define ROP_MERGEPAINT			0x00bb
#define ROP_PATCOPY			0x00f0
#define ROP_PATPAINT			0x00fb
#define ROP_PATINVERT			0x005a
#define ROP_DSTINVERT			0x0055
#define ROP_ZERO			0x0000
#define ROP_ONE				0x00ff

#define BBO_OR				0
#define BBO_AND				1
#define BBO_IGNORE			2
#define BBO_PAL_COLORS			4
#define BBO_NO_COLOR_INFO		8

#define FF_BOUNDARY			0
#define FF_SURFACE			1

#define HBM_ERROR			((HBITMAP)(-1))


LONG GpiBitBlt (HPS hpsTarget, HPS hpsSource, LONG lCount,
    __const__ POINTL *aptlPoints, LONG lRop, ULONG flOptions);
BOOL GpiDeleteBitmap (HBITMAP hbm);
HBITMAP GpiLoadBitmap (HPS hps, HMODULE Resource, ULONG idBitmap,
    LONG lWidth, LONG lHeight);
HBITMAP GpiSetBitmap (HPS hps, HBITMAP hbm);
LONG GpiWCBitBlt (HPS hpsTarget, HBITMAP hbmSource, LONG lCount,
    __const__ POINTL *aptlPoints, LONG lRop, ULONG flOptions);

#endif /* INCL_GPIBITMAPS */


#if defined (INCL_GPIBITMAPS)

#define BFT_ICON			0x4349
#define BFT_BMAP			0x4d42
#define BFT_POINTER			0x5450
#define BFT_COLORICON			0x4943
#define BFT_COLORPOINTER		0x5043
#define BFT_BITMAPARRAY			0x4142

#define CBD_BITS			0
#define CBD_COMPRESSION			1
#define CBD_DECOMPRESSION		2

#define CBD_COLOR_CONVERSION		0x0001

#define CBM_INIT			0x0004

#define BCA_UNCOMP			0
#define BCA_RLE8			1
#define BCA_RLE4			2
#define BCA_HUFFMAN1D			3
#define BCA_RLE24			4

#define BMB_ERROR			(-1)

#define BRU_METRIC			0

#define BRA_BOTTOMUP			0

#define BRH_NOTHALFTONED		0
#define BRH_ERRORDIFFUSION		1
#define BRH_PANDA			2
#define BRH_SUPERCIRCLE			3

#define BCE_PALETTE			(-1)
#define BCE_RGB				0


typedef struct _RGB
{
  BYTE bBlue;
  BYTE bGreen;
  BYTE bRed;
} RGB;

typedef struct _RGB2
{
  BYTE bBlue;
  BYTE bGreen;
  BYTE bRed;
  BYTE fcOptions;
} RGB2;
typedef RGB2 *PRGB2;

typedef struct _BITMAPINFOHEADER
{
  ULONG	 cbFix;
  USHORT cx;
  USHORT cy;
  USHORT cPlanes;
  USHORT cBitCount;
} BITMAPINFOHEADER;
typedef BITMAPINFOHEADER *PBITMAPINFOHEADER;

typedef struct _BITMAPINFO
{
  ULONG	 cbFix;
  USHORT cx;
  USHORT cy;
  USHORT cPlanes;
  USHORT cBitCount;
  RGB	 argbColor[1];
} BITMAPINFO;
typedef BITMAPINFO *PBITMAPINFO;

typedef struct _BITMAPINFO2
{
  ULONG	 cbFix;
  ULONG	 cx;
  ULONG	 cy;
  USHORT cPlanes;
  USHORT cBitCount;
  ULONG	 ulCompression;
  ULONG	 cbImage;
  ULONG	 cxResolution;
  ULONG	 cyResolution;
  ULONG	 cclrUsed;
  ULONG	 cclrImportant;
  USHORT usUnits;
  USHORT usReserved;
  USHORT usRecording;
  USHORT usRendering;
  ULONG	 cSize1;
  ULONG	 cSize2;
  ULONG	 ulColorEncoding;
  ULONG	 ulIdentifier;
  RGB2	 argbColor[1];
} BITMAPINFO2;
typedef BITMAPINFO2 *PBITMAPINFO2;

typedef struct _BITMAPINFOHEADER2
{
  ULONG	 cbFix;
  ULONG	 cx;
  ULONG	 cy;
  USHORT cPlanes;
  USHORT cBitCount;
  ULONG	 ulCompression;
  ULONG	 cbImage;
  ULONG	 cxResolution;
  ULONG	 cyResolution;
  ULONG	 cclrUsed;
  ULONG	 cclrImportant;
  USHORT usUnits;
  USHORT usReserved;
  USHORT usRecording;
  USHORT usRendering;
  ULONG	 cSize1;
  ULONG	 cSize2;
  ULONG	 ulColorEncoding;
  ULONG	 ulIdentifier;
} BITMAPINFOHEADER2;
typedef BITMAPINFOHEADER2 *PBITMAPINFOHEADER2;

typedef struct _BITMAPFILEHEADER
{
  USHORT	   usType;
  ULONG		   cbSize;
  SHORT		   xHotspot;
  SHORT		   yHotspot;
  ULONG		   offBits;
  BITMAPINFOHEADER bmp;
} BITMAPFILEHEADER;
typedef BITMAPFILEHEADER *PBITMAPFILEHEADER;

typedef struct _BITMAPARRAYFILEHEADER
{
  USHORT	   usType;
  ULONG		   cbSize;
  ULONG		   offNext;
  USHORT	   cxDisplay;
  USHORT	   cyDisplay;
  BITMAPFILEHEADER bfh;
} BITMAPARRAYFILEHEADER;
typedef BITMAPARRAYFILEHEADER *PBITMAPARRAYFILEHEADER;

typedef struct _BITMAPFILEHEADER2
{
  USHORT	    usType;
  ULONG		    cbSize;
  SHORT		    xHotspot;
  SHORT		    yHotspot;
  ULONG		    offBits;
  BITMAPINFOHEADER2 bmp2;
} BITMAPFILEHEADER2;
typedef BITMAPFILEHEADER2 *PBITMAPFILEHEADER2;

typedef struct _BITMAPARRAYFILEHEADER2
{
  USHORT	    usType;
  ULONG		    cbSize;
  ULONG		    offNext;
  USHORT	    cxDisplay;
  USHORT	    cyDisplay;
  BITMAPFILEHEADER2 bfh2;
} BITMAPARRAYFILEHEADER2;
typedef BITMAPARRAYFILEHEADER2 *PBITMAPARRAYFILEHEADER2;


HBITMAP GpiCreateBitmap (HPS hps, __const__ BITMAPINFOHEADER2 *pbmpNew,
    ULONG flOptions, __const__ BYTE *pbInitData,
    __const__ BITMAPINFO2 *pbmiInfoTable);
LONG GpiDrawBits (HPS hps, __const__ VOID *pBits,
    __const__ BITMAPINFO2 *pbmiInfoTable, LONG lCount,
    __const__ POINTL *aptlPoints, LONG lRop, ULONG flOptions);
LONG GpiFloodFill (HPS hps, LONG lOptions, LONG lColor);
LONG GpiQueryBitmapBits (HPS hps, LONG lScanStart, LONG lScans, PBYTE pbBuffer,
    PBITMAPINFO2 pbmiInfoTable);
BOOL GpiQueryBitmapDimension (HBITMAP hbm, PSIZEL psizlBitmapDimension);
HBITMAP GpiQueryBitmapHandle (HPS hps, LONG lLcid);
BOOL GpiQueryBitmapInfoHeader (HBITMAP hbm, PBITMAPINFOHEADER2 pbmpData);
BOOL GpiQueryBitmapParameters (HBITMAP hbm, PBITMAPINFOHEADER pbmpData);
BOOL GpiQueryDeviceBitmapFormats (HPS hps, LONG lCount, PLONG alArray);
LONG  GpiSetBitmapBits (HPS hps, LONG lScanStart, LONG lScans,
    __const__ BYTE *pbBuffer, __const__ BITMAPINFO2 *pbmiInfoTable);
LONG GpiQueryPel (HPS hps, PPOINTL pptlPoint);
BOOL GpiSetBitmapDimension (HBITMAP hbm,
    __const__ SIZEL *psizlBitmapDimension);
BOOL GpiSetBitmapId (HPS hps, HBITMAP hbm, LONG lLcid);
LONG GpiSetPel (HPS hps, __const__ POINTL *pptlPoint);


#endif /* INCL_GPIBITMAPS */


#if defined (INCL_GPICONTROL) || !defined (INCL_NOCOMMON)

#define GPIA_NOASSOC			0x0000
#define GPIA_ASSOC			0x4000

#define GPIF_DEFAULT			0x0000
#define GPIF_SHORT			0x0100
#define GPIF_LONG			0x0200

#define GPIM_AREAEXCL			0x8000

#define GPIT_NORMAL			0x0000
#define GPIT_MICRO			0x1000
#define GPIT_INK			0x2000

#define HDC_ERROR			((HDC)(-1))

#define PU_ARBITRARY			0x0004
#define PU_PELS				0x0008
#define PU_LOMETRIC			0x000c
#define PU_HIMETRIC			0x0010
#define PU_LOENGLISH			0x0014
#define PU_HIENGLISH			0x0018
#define PU_TWIPS			0x001c


BOOL GpiAssociate (HPS hps, HDC hdc);
HPS GpiCreatePS (HAB hab, HDC hdc, PSIZEL psizlSize, ULONG flOptions);
BOOL GpiDestroyPS (HPS hps);
BOOL GpiErase (HPS hps);
HDC GpiQueryDevice (HPS hps);
BOOL GpiRestorePS (HPS hps, LONG lPSid);
LONG GpiSavePS (HPS hps);

#endif /* INCL_GPICONTROL */


#if defined (INCL_GPICONTROL)

#define DCTL_ERASE			1
#define DCTL_DISPLAY			2
#define DCTL_BOUNDARY			3
#define DCTL_DYNAMIC			4
#define DCTL_CORRELATE			5

#define DCTL_ERROR			(-1)
#define DCTL_OFF			0
#define DCTL_ON				1

#define DM_ERROR			0
#define DM_DRAW				1
#define DM_RETAIN			2
#define DM_DRAWANDRETAIN		3

#define GPIE_SEGMENT			0
#define GPIE_ELEMENT			1
#define GPIE_DATA			2

#define GRES_ATTRS			0x0001
#define GRES_SEGMENTS			0x0002
#define GRES_ALL			0x0004

#define PS_UNITS			0x00fc
#define PS_FORMAT			0x0f00
#define PS_TYPE				0x1000
#define PS_MODE				0x2000
#define PS_ASSOCIATE			0x4000
#define PS_NORESET			0x8000

#define SDW_ERROR			(-1)
#define SDW_OFF				0
#define SDW_ON				1


LONG GpiErrorSegmentData (HPS hps, PLONG plSegment, PLONG plContext);
LONG GpiQueryDrawControl (HPS hps, LONG lControl);
LONG GpiQueryDrawingMode (HPS hps);
ULONG GpiQueryPS (HPS hps, PSIZEL psizlSize);
BOOL GpiResetPS (HPS hps, ULONG flOptions);
LONG GpiQueryStopDraw (HPS hps);
BOOL GpiSetDrawControl (HPS hps, LONG lControl, LONG lValue);
BOOL GpiSetDrawingMode (HPS hps, LONG lMode);
BOOL GpiSetPS (HPS hps, __const__ SIZEL *psizlsize, ULONG flOptions);
BOOL GpiSetStopDraw (HPS hps, LONG lValue);

#endif /* INCL_GPICONTROL */


#if defined (INCL_GPICORRELATION)

#define GPI_HITS			2

#define PICKAP_DEFAULT			0
#define PICKAP_REC			2

#define PICKSEL_VISIBLE			0
#define PICKSEL_ALL			1


LONG GpiCorrelateChain (HPS hps, LONG lType, __const__ POINTL *pptlPick,
    LONG lMaxHits, LONG lMaxDepth, PLONG pl2);
LONG GpiCorrelateFrom (HPS hps, LONG lFirstSegment, LONG lLastSegment,
    LONG lType, __const__ POINTL *pptlPick, LONG lMaxHits, LONG lMaxDepth,
    PLONG plSegTag);
LONG GpiCorrelateSegment (HPS hps, LONG lSegment, LONG lType,
    __const__ POINTL *pptlPick, LONG lMaxHits, LONG lMaxDepth, PLONG alSegTag);
BOOL GpiQueryBoundaryData (HPS hps, PRECTL prclBoundary);
BOOL GpiQueryPickAperturePosition (HPS hps, PPOINTL pptlPoint);
BOOL GpiQueryPickApertureSize (HPS hps, PSIZEL psizlSize);
BOOL GpiQueryTag (HPS hps, PLONG plTag);
BOOL GpiResetBoundaryData (HPS hps);
BOOL GpiSetPickAperturePosition (HPS hps, __const__ POINTL *pptlPick);
BOOL GpiSetPickApertureSize (HPS hps, LONG lOptions,
    __const__ SIZEL *psizlSize);
BOOL GpiSetTag (HPS hps, LONG lTag);

#endif /* INCL_GPICORRELATION */


#if defined (INCL_GPIINK)

#define PPE_KEEPPATH			0
#define PPE_ERASEPATH			1

#define PPS_INKMOVE			0
#define PPS_INKDOWN			1
#define PPS_INKUP			2


BOOL GpiBeginInkPath (HPS hps, LONG lPath, ULONG flOptions);
BOOL GpiEndInkPath (HPS hps, ULONG flOptions);
LONG GpiStrokeInkPath (HPS hps, LONG lPath, LONG lCount,
    __const__ POINTL *aptlPoints, ULONG flOptions);

#endif /* INCL_GPIINK */


#if defined (INCL_GPISEGMENTS)

#define DFORM_NOCONV			0

#define DFORM_S370SHORT			1
#define DFORM_PCSHORT			2
#define DFORM_PCLONG			4

#define ATTR_ERROR			(-1)
#define ATTR_DETECTABLE			1
#define ATTR_VISIBLE			2
#define ATTR_CHAINED			6
#define ATTR_DYNAMIC			8
#define ATTR_FASTCHAIN			9
#define ATTR_PROP_DETECTABLE		10
#define ATTR_PROP_VISIBLE		11

#define ATTR_OFF			0
#define ATTR_ON				1

#define LOWER_PRI			(-1)
#define HIGHER_PRI			1


BOOL GpiCloseSegment (HPS hps);
BOOL GpiDeleteSegment (HPS hps, LONG lSegid);
BOOL GpiDeleteSegments (HPS hps, LONG lFirstSegment, LONG lLastSegment);
BOOL GpiDrawChain (HPS hps);
BOOL GpiDrawDynamics (HPS hps);
BOOL GpiDrawFrom (HPS hps, LONG lFirstSegment, LONG lLastSegment);
BOOL GpiDrawSegment (HPS hps, LONG lSegment);
LONG GpiGetData (HPS hps, LONG lSegid, PLONG plOffset, LONG lFormat,
    LONG lLength, PBYTE pbData);
BOOL GpiOpenSegment (HPS hps, LONG lSegment);
LONG GpiPutData (HPS hps, LONG lFormat, PLONG plCount, __const__ BYTE *pbData);
LONG GpiQueryInitialSegmentAttrs (HPS hps, LONG lAttribute);
LONG GpiQuerySegmentAttrs (HPS hps, LONG lSegid, LONG lAttribute);
LONG GpiQuerySegmentNames (HPS hps, LONG lFirstSegid, LONG lLastSegid,
    LONG lMax, PLONG alSegids);
LONG GpiQuerySegmentPriority (HPS hps, LONG lRefSegid, LONG lOrder);
BOOL GpiRemoveDynamics (HPS hps, LONG lFirstSegid, LONG lLastSegid);
BOOL GpiSetInitialSegmentAttrs (HPS hps, LONG lAttribute, LONG lValue);
BOOL GpiSetSegmentAttrs (HPS hps, LONG lSegid, LONG lAttribute, LONG lValue);
BOOL GpiSetSegmentPriority (HPS hps, LONG lSegid, LONG lRefSegid, LONG lOrder);

#endif /* INCL_GPISEGMENTS */


/* ---------------------- DEVICE CONTEXTS --------------------------------- */

#define DEV_ERROR			0
#define DEV_OK				1
#define DEV_BAD_PARAMETERS		2
#define DEV_WARNING			3
#define DEV_PROP_BUF_TOO_SMALL		4
#define DEV_ITEM_BUF_TOO_SMALL		5
#define DEV_INV_INP_JOBPROPERTIES	6

#define ADDRESS				0
#define DRIVER_NAME			1
#define DRIVER_DATA			2
#define DATA_TYPE			3
#define COMMENT				4
#define PROC_NAME			5
#define PROC_PARAMS			6
#define SPL_PARAMS			7
#define NETWORK_PARAMS			8

#define OD_SCREEN			0
#define OD_QUEUED			2
#define OD_DIRECT			5
#define OD_INFO				6
#define OD_METAFILE			7
#define OD_MEMORY			8
#define OD_METAFILE_NOQUERY		9

#define CAPS_FAMILY			0
#define CAPS_IO_CAPS			1
#define CAPS_TECHNOLOGY			2
#define CAPS_DRIVER_VERSION		3
#define CAPS_WIDTH			4
#define CAPS_HEIGHT			5
#define CAPS_WIDTH_IN_CHARS		6
#define CAPS_HEIGHT_IN_CHARS		7
#define CAPS_HORIZONTAL_RESOLUTION	8
#define CAPS_VERTICAL_RESOLUTION	9
#define CAPS_CHAR_WIDTH			10
#define CAPS_CHAR_HEIGHT		11
#define CAPS_SMALL_CHAR_WIDTH		12
#define CAPS_SMALL_CHAR_HEIGHT		13
#define CAPS_COLORS			14
#define CAPS_COLOR_PLANES		15
#define CAPS_COLOR_BITCOUNT		16
#define CAPS_COLOR_TABLE_SUPPORT	17
#define CAPS_MOUSE_BUTTONS		18
#define CAPS_FOREGROUND_MIX_SUPPORT	19
#define CAPS_BACKGROUND_MIX_SUPPORT	20
#define CAPS_DEVICE_WINDOWING		31
#define CAPS_ADDITIONAL_GRAPHICS	32
#define CAPS_VIO_LOADABLE_FONTS		21
#define CAPS_WINDOW_BYTE_ALIGNMENT	22
#define CAPS_BITMAP_FORMATS		23
#define CAPS_RASTER_CAPS		24
#define CAPS_MARKER_HEIGHT		25
#define CAPS_MARKER_WIDTH		26
#define CAPS_DEVICE_FONTS		27
#define CAPS_GRAPHICS_SUBSET		28
#define CAPS_GRAPHICS_VERSION		29
#define CAPS_GRAPHICS_VECTOR_SUBSET	30
#define CAPS_PHYS_COLORS		33
#define CAPS_COLOR_INDEX		34
#define CAPS_GRAPHICS_CHAR_WIDTH	35
#define CAPS_GRAPHICS_CHAR_HEIGHT	36
#define CAPS_HORIZONTAL_FONT_RES	37
#define CAPS_VERTICAL_FONT_RES		38
#define CAPS_DEVICE_FONT_SIM		39
#define CAPS_LINEWIDTH_THICK		40
#define CAPS_DEVICE_POLYSET_POINTS	41

#define CAPS_IO_DUMMY			1
#define CAPS_IO_SUPPORTS_OP		2
#define CAPS_IO_SUPPORTS_IP		3
#define CAPS_IO_SUPPORTS_IO		4

#define CAPS_TECH_UNKNOWN		0
#define CAPS_TECH_VECTOR_PLOTTER	1
#define CAPS_TECH_RASTER_DISPLAY	2
#define CAPS_TECH_RASTER_PRINTER	3
#define CAPS_TECH_RASTER_CAMERA		4
#define CAPS_TECH_POSTSCRIPT		5

#define CAPS_COLTABL_RGB_8		0x0001
#define CAPS_COLTABL_RGB_8_PLUS		0x0002
#define CAPS_COLTABL_TRUE_MIX		0x0004
#define CAPS_COLTABL_REALIZE		0x0008

#define CAPS_FM_OR			0x0001
#define CAPS_FM_OVERPAINT		0x0002
#define CAPS_FM_XOR			0x0008
#define CAPS_FM_LEAVEALONE		0x0010
#define CAPS_FM_AND			0x0020
#define CAPS_FM_GENERAL_BOOLEAN		0x0040

#define CAPS_BM_OR			0x0001
#define CAPS_BM_OVERPAINT		0x0002
#define CAPS_BM_XOR			0x0008
#define CAPS_BM_LEAVEALONE		0x0010
#define CAPS_BM_AND			0x0020
#define CAPS_BM_GENERAL_BOOLEAN		0x0040
#define CAPS_BM_SRCTRANSPARENT		0x0080
#define CAPS_BM_DESTTRANSPARENT		0x0100

#define CAPS_DEV_WINDOWING_SUPPORT	0x0001

#define CAPS_DEV_FONT_SIM_BOLD		0x0001
#define CAPS_DEV_FONT_SIM_ITALIC	0x0002
#define CAPS_DEV_FONT_SIM_UNDERSCORE	0x0004
#define CAPS_DEV_FONT_SIM_STRIKEOUT	0x0008

#define CAPS_VDD_DDB_TRANSFER		0x0001
#define CAPS_GRAPHICS_KERNING_SUPPORT	0x0002
#define CAPS_FONT_OUTLINE_DEFAULT	0x0004
#define CAPS_FONT_IMAGE_DEFAULT		0x0008
#define CAPS_SCALED_DEFAULT_MARKERS	0x0040
#define CAPS_COLOR_CURSOR_SUPPORT	0x0080
#define CAPS_PALETTE_MANAGER		0x0100
#define CAPS_COSMETIC_WIDELINE_SUPPORT	0x0200
#define CAPS_DIRECT_FILL		0x0400
#define CAPS_REBUILD_FILLS		0x0800
#define CAPS_CLIP_FILLS			0x1000
#define CAPS_ENHANCED_FONTMETRICS	0x2000
#define CAPS_TRANSFORM_SUPPORT		0x4000
#define CAPS_EXTERNAL_16_BITCOUNT	0x8000

#define CAPS_BYTE_ALIGN_REQUIRED	0
#define CAPS_BYTE_ALIGN_RECOMMENDED	1
#define CAPS_BYTE_ALIGN_NOT_REQUIRED	2

#define CAPS_RASTER_BITBLT		0x0001
#define CAPS_RASTER_BANDING		0x0002
#define CAPS_RASTER_BITBLT_SCALING	0x0004
#define CAPS_RASTER_SET_PEL		0x0010
#define CAPS_RASTER_FONTS		0x0020
#define CAPS_RASTER_FLOOD_FILL		0x0040

#define DEVESC_ERROR			(-1)
#define DEVESC_NOTIMPLEMENTED		0

#define DEVESC_QUERYESCSUPPORT		0
#define DEVESC_GETSCALINGFACTOR		1
#define DEVESC_QUERYVIOCELLSIZES	2
#define DEVESC_GETCP			8000
#define DEVESC_STARTDOC			8150
#define DEVESC_ENDDOC			8151
#define DEVESC_NEXTBAND			8152
#define DEVESC_ABORTDOC			8153
#define DEVESC_GETJOBID			8160
#define DEVESC_QUERY_RASTER		8161
#define DEVESC_QUERYSIZE		8162
#define DEVESC_QUERYJOBPROPERTIES	8163
#define DEVESC_SETJOBPROPERTIES		8164
#define DEVESC_DEFAULTJOBPROPERTIES	8165
#define DEVESC_CHANGEOUTPUTPORT		8166
#define DEVESC_NEWFRAME			16300
#define DEVESC_DRAFTMODE		16301
#define DEVESC_FLUSHOUTPUT		16302
#define DEVESC_RAWDATA			16303
#define DEVESC_SETMODE			16304
#define DEVESC_SEP			16305
#define DEVESC_MACRO			16307
#define DEVESC_BEGIN_BITBLT		16309
#define DEVESC_END_BITBLT		16310
#define DEVESC_SEND_COMPDATA		16311
#define DEVESC_DBE_FIRST		24450
#define DEVESC_DBE_LAST			24455
#define DEVESC_CHAR_EXTRA		16998
#define DEVESC_BREAK_EXTRA		16999
#define DEVESC_STD_JOURNAL		32600
#define DEVESC_STARTDOC_WPROP		49150
#define DEVESC_NEWFRAME_WPROP		49151

#define DPDM_ERROR			(-1)
#define DPDM_NONE			0

#define DPDM_POSTJOBPROP		0
#define DPDM_CHANGEPROP			1
#define DPDM_QUERYJOBPROP		2

#define DQHC_ERROR			(-1)

#define HCAPS_CURRENT			1
#define HCAPS_SELECTABLE		2


typedef PSZ *PDEVOPENDATA;


typedef struct _DRIVDATA
{
  LONG cb;
  LONG lVersion;
  CHAR szDeviceName[32];
  CHAR abGeneralData[1];
} DRIVDATA;
typedef DRIVDATA *PDRIVDATA;

typedef struct _DEVOPENSTRUC
{
  PSZ	    pszLogAddress;
  PSZ	    pszDriverName;
  PDRIVDATA pdriv;
  PSZ	    pszDataType;
  PSZ	    pszComment;
  PSZ	    pszQueueProcName;
  PSZ	    pszQueueProcParams;
  PSZ	    pszSpoolerParams;
  PSZ	    pszNetworkParams;
} DEVOPENSTRUC;
typedef DEVOPENSTRUC *PDEVOPENSTRUC;

typedef struct _ESCMODE
{
  ULONG mode;
  BYTE	modedata[1];
} ESCMODE;
typedef ESCMODE *PESCMODE;

typedef struct _VIOSIZECOUNT
{
  LONG maxcount;
  LONG count;
} VIOSIZECOUNT;
typedef VIOSIZECOUNT *PVIOSIZECOUNT;

typedef struct _VIOFONTCELLSIZE
{
  LONG cx;
  LONG cy;
} VIOFONTCELLSIZE;
typedef VIOFONTCELLSIZE *PVIOFONTCELLSIZE;

typedef struct _SFACTORS
{
  LONG x;
  LONG y;
} SFACTORS;
typedef SFACTORS *PSFACTORS;

typedef struct _BANDRECT
{
  LONG xleft;
  LONG ybottom;
  LONG xright;
  LONG ytop;
} BANDRECT;
typedef BANDRECT *PBANDRECT;

typedef struct _HCINFO
{
  CHAR szFormname[32];
  LONG cx;
  LONG cy;
  LONG xLeftClip;
  LONG yBottomClip;
  LONG xRightClip;
  LONG yTopClip;
  LONG xPels;
  LONG yPels;
  LONG flAttributes;
} HCINFO;
typedef HCINFO *PHCINFO;

HMF DevCloseDC (HDC hdc);
LONG DevEscape (HDC hdc, LONG lCode, LONG lInCount, PBYTE pbInData,
    PLONG plOutCount, PBYTE pbOutData);
HDC DevOpenDC (HAB hab, LONG lType, PCSZ pszToken, LONG lCount,
    PDEVOPENDATA pdopData, HDC hdcComp);
LONG DevPostDeviceModes (HAB hab, PDRIVDATA pdrivDriverData ,
    PCSZ pszDriverName, PCSZ pszDeviceName, PCSZ pszName, ULONG flOptions);
BOOL DevQueryCaps (HDC hdc, LONG lStart, LONG lCount, PLONG alArray);
BOOL DevQueryDeviceNames (HAB hab, PCSZ pszDriverName, PLONG pldn,
    PSTR32 aDeviceName, PSTR64 aDeviceDesc, PLONG pldt, PSTR16 aDataType);
LONG DevQueryHardcopyCaps (HDC hdc, LONG lStartForm, LONG lForms,
    PHCINFO phciHcInfo);

/* ------------------ PRESENTATION MANAGER SHELL -------------------------- */

#define MAXNAMEL			60

#define HINI_PROFILE			(HINI)0
#define HINI_USERPROFILE		(HINI)(-1)
#define HINI_SYSTEMPROFILE		(HINI)(-2)
#define HINI_USER			HINI_USERPROFILE
#define HINI_SYSTEM			HINI_SYSTEMPROFILE

typedef LHANDLE HSWITCH;
typedef HSWITCH *PHSWITCH;

typedef LHANDLE HPROGRAM;
typedef HPROGRAM *PHPROGRAM;

typedef LHANDLE HINI;
typedef HINI *PHINI;

typedef LHANDLE HAPP;


typedef struct _PRFPROFILE
{
  ULONG cchUserName;
  PSZ	pszUserName;
  ULONG cchSysName;
  PSZ	pszSysName;
} PRFPROFILE;
typedef PRFPROFILE *PPRFPROFILE;


#if defined (INCL_WINPROGRAMLIST)

#define MAXPATHL			128
#define SGH_ROOT			(HPROGRAM)(-1L)

#define PROG_DEFAULT			0
#define PROG_FULLSCREEN			1
#define PROG_WINDOWABLEVIO		2
#define PROG_PM				3
#define PROG_GROUP			5
#define PROG_REAL			4
#define PROG_VDM			4
#define PROG_WINDOWEDVDM		7
#define PROG_DLL			6
#define PROG_PDD			8
#define PROG_VDD			9
#define PROG_WINDOW_REAL		10
#define PROG_WINDOW_PROT		11
#define PROG_WINDOW_AUTO		12
#define PROG_SEAMLESSVDM		13
#define PROG_SEAMLESSCOMMON		14
#define PROG_30_STDSEAMLESSCOMMON	14
#define PROG_31_STDSEAMLESSVDM		15
#define PROG_31_STDSEAMLESSCOMMON	16
#define PROG_31_ENHSEAMLESSVDM		17
#define PROG_31_ENHSEAMLESSCOMMON	18
#define PROG_31_ENH			19
#define PROG_31_STD			20
#define PROG_DOS_GAME			21
#define PROG_WIN_GAME			22
#define PROG_DOS_MODE			23
#define PROG_RESERVED			255

#define SAF_VALIDFLAGS			0x001f

#define SAF_INSTALLEDCMDLINE		0x0001
#define SAF_STARTCHILDAPP		0x0002
#define SAF_MAXIMIZED			0x0004
#define SAF_MINIMIZED			0x0008
#define SAF_BACKGROUND			0x0010

#define SHE_VISIBLE			0x00
#define SHE_INVISIBLE			0x01
#define SHE_RESERVED			0xff

#define SHE_UNPROTECTED			0x00
#define SHE_PROTECTED			0x02


typedef ULONG PROGCATEGORY;
typedef PROGCATEGORY *PPROGCATEGORY;

typedef struct _HPROGARRAY
{
  HPROGRAM ahprog[1];
} HPROGARRAY;
typedef HPROGARRAY *PHPROGARRAY;

typedef struct _PROGTYPE
{
  PROGCATEGORY progc;
  ULONG	       fbVisible;
} PROGTYPE;
typedef PROGTYPE *PPROGTYPE;

typedef struct _PROGTITLE
{
  HPROGRAM hprog;
  PROGTYPE progt;
  PSZ	   pszTitle;
} PROGTITLE;
typedef PROGTITLE *PPROGTITLE;

typedef struct _PROGDETAILS
{
  ULONG	   Length;
  PROGTYPE progt;
  PSZ	   pszTitle;
  PSZ	   pszExecutable;
  PSZ	   pszParameters;
  PSZ	   pszStartupDir;
  PSZ	   pszIcon;
  PSZ	   pszEnvironment;
  SWP	   swpInitial;
} PROGDETAILS;
typedef PROGDETAILS *PPROGDETAILS;


HPROGRAM PrfAddProgram (HINI hini, PPROGDETAILS pDetails, HPROGRAM hprogGroup);
BOOL PrfChangeProgram (HINI hini, HPROGRAM hprog, PPROGDETAILS pDetails);
HPROGRAM PrfCreateGroup (HINI hini, PCSZ pszTitle, UCHAR chVisibility);
BOOL PrfDestroyGroup (HINI hini, HPROGRAM hprogGroup);
PROGCATEGORY PrfQueryProgramCategory (HINI hini, PCSZ pszExe);
ULONG PrfQueryProgramHandle (HINI hini, PCSZ pszExe,
    PHPROGARRAY phprogArray, ULONG cchBufferMax, PULONG pulCount);
ULONG PrfQueryProgramTitles (HINI hini, HPROGRAM hprogGroup,
    PPROGTITLE pTitles, ULONG ulBufferLength, PULONG pulCount);
ULONG PrfQueryDefinition (HINI hini, HPROGRAM hprog, PPROGDETAILS pDetails,
    ULONG ulBufferLength);
BOOL PrfRemoveProgram (HINI hini, HPROGRAM hprog);

HAPP WinStartApp (HWND hwndNotify, PPROGDETAILS pDetails,
    PCSZ pszParams, PVOID Reserved, ULONG fbOptions);
BOOL WinTerminateApp (HAPP happ);

#endif /* INCL_WINPROGRAMLIST */


#if defined (INCL_WINSWITCHLIST) || !defined (INCL_NOCOMMON)

#define SWL_INVISIBLE			0x01
#define SWL_GRAYED			0x02
#define SWL_VISIBLE			0x04

#define SWL_NOTJUMPABLE			0x01
#define SWL_JUMPABLE			0x02

typedef struct _SWCNTRL
{
  HWND	   hwnd;
  HWND	   hwndIcon;
  HPROGRAM hprog;
  PID	   idProcess;
  ULONG	   idSession;
  ULONG	   uchVisibility;
  ULONG	   fbJump;
  CHAR	   szSwtitle[MAXNAMEL+4];
  ULONG	   bProgType;
} SWCNTRL;
typedef SWCNTRL *PSWCNTRL;

HSWITCH WinAddSwitchEntry (__const__ SWCNTRL *pswctl);
ULONG WinRemoveSwitchEntry (HSWITCH hsw);

#endif /* INCL_WINSWITCHLIST || !INCL_NOCOMMON */

#if defined (INCL_WINSWITCHLIST)

typedef struct _SWENTRY
{
  HSWITCH hswitch;
  SWCNTRL swctl;
} SWENTRY;
typedef SWENTRY *PSWENTRY;

typedef struct _SWBLOCK
{
  ULONG	  cswentry;
  SWENTRY aswentry[1];
} SWBLOCK;
typedef SWBLOCK *PSWBLOCK;


ULONG WinChangeSwitchEntry (HSWITCH hsw, __const__ SWCNTRL *pswctl);
HSWITCH WinCreateSwitchEntry (HAB hab, __const__ SWCNTRL *pswctl);
ULONG WinQuerySessionTitle (HAB hab, ULONG usSession, PSZ pszTitle,
    ULONG usTitlelen);
ULONG WinQuerySwitchEntry (HSWITCH hsw, PSWCNTRL pswctl);
HSWITCH WinQuerySwitchHandle (HWND hwnd, PID pid);
ULONG WinQuerySwitchList (HAB hab, PSWBLOCK pswblk, ULONG usDataLength);
ULONG WinQueryTaskSizePos (HAB hab, ULONG usScreenGroup, PSWP pswp);
ULONG WinQueryTaskTitle (ULONG usSession, PSZ pszTitle, ULONG usTitlelen);
ULONG WinSwitchToProgram (HSWITCH hsw);

#endif /* INCL_WINSWITCHLIST */


#if defined (INCL_WINSHELLDATA)

#define PL_ALTERED			0x008e

BOOL PrfCloseProfile (HINI hini);
HINI PrfOpenProfile (HAB hab, PCSZ pszFileName);
BOOL PrfQueryProfile (HAB hab, PPRFPROFILE pPrfProfile);
BOOL PrfQueryProfileData (HINI hini, PCSZ pszApp, PCSZ pszKey, PVOID pBuffer,
    PULONG pulBufferLength);
LONG PrfQueryProfileInt (HINI hini, PCSZ pszApp, PCSZ pszKey, LONG  sDefault);
BOOL PrfQueryProfileSize (HINI hini, PCSZ pszApp, PCSZ pszKey,
    PULONG pulReqLen);
ULONG PrfQueryProfileString (HINI hini, PCSZ pszApp, PCSZ pszKey,
    PCSZ pszDefault, PVOID pBuffer, ULONG ulBufferLength);
BOOL PrfReset (HAB hab, __const__ PRFPROFILE *pPrfProfile);
BOOL PrfWriteProfileData (HINI hini, PCSZ pszApp, PCSZ pszKey,
    CPVOID pData, ULONG ulDataLength);
BOOL PrfWriteProfileString (HINI hini, PCSZ pszApp, PCSZ pszKey,
    PCSZ pszData);

#endif /* INCL_WINSHELLDATA */

/* ------------------ STANDARD DIALOGS: FILE ------------------------------ */

#if defined (INCL_WINSTDFILE)

#define FDM_FILTER			(WM_USER+40)
#define FDM_VALIDATE			(WM_USER+41)
#define FDM_ERROR			(WM_USER+42)

#define DID_FILE_DIALOG			256
#define DID_FILENAME_TXT		257
#define DID_FILENAME_ED			258
#define DID_DRIVE_TXT			259
#define DID_DRIVE_CB			260
#define DID_FILTER_TXT			261
#define DID_FILTER_CB			262
#define DID_DIRECTORY_TXT		263
#define DID_DIRECTORY_LB		264
#define DID_FILES_TXT			265
#define DID_FILES_LB			266
#define DID_HELP_PB			267
#define DID_APPLY_PB			268
#define DID_READ_ONLY			269
#define DID_DIRECTORY_SELECTED		270
#define DID_OK_PB			DID_OK
#define DID_CANCEL_PB			DID_CANCEL

#define FDS_CENTER			0x00000001
#define FDS_CUSTOM			0x00000002
#define FDS_FILTERUNION			0x00000004
#define FDS_HELPBUTTON			0x00000008
#define FDS_APPLYBUTTON			0x00000010
#define FDS_PRELOAD_VOLINFO		0x00000020
#define FDS_MODELESS			0x00000040
#define FDS_INCLUDE_EAS			0x00000080
#define FDS_OPEN_DIALOG			0x00000100
#define FDS_SAVEAS_DIALOG		0x00000200
#define FDS_MULTIPLESEL			0x00000400
#define FDS_ENABLEFILELB		0x00000800
#define FDS_NATIONAL_LANGUAGE		0x80000000

#define FDS_EFSELECTION			0
#define FDS_LBSELECTION			1

#define FDS_SUCCESSFUL			0
#define FDS_ERR_DEALLOCATE_MEMORY	1
#define FDS_ERR_FILTER_TRUNC		2
#define FDS_ERR_INVALID_DIALOG		3
#define FDS_ERR_INVALID_DRIVE		4
#define FDS_ERR_INVALID_FILTER		5
#define FDS_ERR_INVALID_PATHFILE	6
#define FDS_ERR_OUT_OF_MEMORY		7
#define FDS_ERR_PATH_TOO_LONG		8
#define FDS_ERR_TOO_MANY_FILE_TYPES	9
#define FDS_ERR_INVALID_VERSION		10
#define FDS_ERR_INVALID_CUSTOM_HANDLE	11
#define FDS_ERR_DIALOG_LOAD_ERROR	12
#define FDS_ERR_DRIVE_ERROR		13

#define IDS_FILE_ALL_FILES_SELECTOR	1000
#define IDS_FILE_BACK_CUR_PATH		1001
#define IDS_FILE_BACK_PREV_PATH		1002
#define IDS_FILE_BACK_SLASH		1003
#define IDS_FILE_BASE_FILTER		1004
#define IDS_FILE_BLANK			1005
#define IDS_FILE_COLON			1006
#define IDS_FILE_DOT			1007
#define IDS_FILE_DRIVE_LETTERS		1008
#define IDS_FILE_FWD_CUR_PATH		1009
#define IDS_FILE_FWD_PREV_PATH		1010
#define IDS_FILE_FORWARD_SLASH		1011
#define IDS_FILE_PARENT_DIR		1012
#define IDS_FILE_Q_MARK			1013
#define IDS_FILE_SPLAT			1014
#define IDS_FILE_SPLAT_DOT		1015
#define IDS_FILE_SAVEAS_TITLE		1016
#define IDS_FILE_SAVEAS_FILTER_TXT	1017
#define IDS_FILE_SAVEAS_FILENM_TXT	1018
#define IDS_FILE_DUMMY_FILE_NAME	1019
#define IDS_FILE_DUMMY_FILE_EXT		1020
#define IDS_FILE_DUMMY_DRIVE		1021
#define IDS_FILE_DUMMY_ROOT_DIR		1022
#define IDS_FILE_PATH_PTR		1023
#define IDS_FILE_VOLUME_PREFIX		1024
#define IDS_FILE_VOLUME_SUFFIX		1025
#define IDS_FILE_PATH_PTR2		1026
#define IDS_FILE_INVALID_CHARS		1027
#define IDS_FILE_ETC_BACK_SLASH		1028
#define IDS_FILE_OPEN_PARENTHESIS	1029
#define IDS_FILE_CLOSE_PARENTHESIS	1030
#define IDS_FILE_SEMICOLON		1031
#define IDS_FILE_BAD_DRIVE_NAME		1100
#define IDS_FILE_BAD_DRIVE_OR_PATH_NAME 1101
#define IDS_FILE_BAD_FILE_NAME		1102
#define IDS_FILE_BAD_FQF		1103
#define IDS_FILE_BAD_NETWORK_NAME	1104
#define IDS_FILE_BAD_SUB_DIR_NAME	1105
#define IDS_FILE_DRIVE_NOT_AVAILABLE	1106
#define IDS_FILE_FQFNAME_TOO_LONG	1107
#define IDS_FILE_OPEN_DIALOG_NOTE	1108
#define IDS_FILE_PATH_TOO_LONG		1109
#define IDS_FILE_SAVEAS_DIALOG_NOTE	1110
#define IDS_FILE_DRIVE_DISK_CHANGE	1120
#define IDS_FILE_DRIVE_NOT_READY	1122
#define IDS_FILE_DRIVE_LOCKED		1123
#define IDS_FILE_DRIVE_NO_SECTOR	1124
#define IDS_FILE_DRIVE_SOME_ERROR	1125
#define IDS_FILE_DRIVE_INVALID		1126
#define IDS_FILE_INSERT_DISK_NOTE	1127
#define IDS_FILE_OK_WHEN_READY		1128

typedef PSZ APSZ[1];
typedef APSZ *PAPSZ;

typedef struct _FILEDLG
{
  ULONG	  cbSize;
  ULONG	  fl;
  ULONG	  ulUser;
  LONG	  lReturn;
  LONG	  lSRC;
  PSZ	  pszTitle;
  PSZ	  pszOKButton;
  PFNWP	  pfnDlgProc;
  PSZ	  pszIType;
  PAPSZ	  papszITypeList;
  PSZ	  pszIDrive;
  PAPSZ	  papszIDriveList;
  HMODULE hMod;
  CHAR	  szFullFile[CCHMAXPATH];
  PAPSZ	  papszFQFilename;
  ULONG	  ulFQFCount;
  USHORT  usDlgId;
  SHORT	  x;
  SHORT	  y;
  SHORT	  sEAType;
} FILEDLG;
typedef FILEDLG *PFILEDLG;


MRESULT WinDefFileDlgProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
HWND WinFileDlg (HWND hwndP, HWND hwndO, PFILEDLG pfild);
BOOL WinFreeFileDlgList (PAPSZ papszFQFilename);

#endif /* INCL_WINSTDFILE */

/* ------------------ STANDARD DIALOGS: FONT ------------------------------ */

#if defined (INCL_WINSTDFONT)

#define FNTM_FACENAMECHANGED		(WM_USER+50)
#define FNTM_POINTSIZECHANGED		(WM_USER+51)
#define FNTM_STYLECHANGED		(WM_USER+52)
#define FNTM_COLORCHANGED		(WM_USER+53)
#define FNTM_UPDATEPREVIEW		(WM_USER+54)
#define FNTM_FILTERLIST			(WM_USER+55)

#define FNTS_CENTER			0x00000001
#define FNTS_CUSTOM			0x00000002
#define FNTS_OWNERDRAWPREVIEW		0x00000004
#define FNTS_HELPBUTTON			0x00000008
#define FNTS_APPLYBUTTON		0x00000010
#define FNTS_RESETBUTTON		0x00000020
#define FNTS_MODELESS			0x00000040
#define FNTS_INITFROMFATTRS		0x00000080
#define FNTS_BITMAPONLY			0x00000100
#define FNTS_VECTORONLY			0x00000200
#define FNTS_FIXEDWIDTHONLY		0x00000400
#define FNTS_PROPORTIONALONLY		0x00000800
#define FNTS_NOSYNTHESIZEDFONTS		0x00001000
#define FNTS_NATIONAL_LANGUAGE		0x80000000

#define FNTF_NOVIEWSCREENFONTS		0x0001
#define FNTF_NOVIEWPRINTERFONTS		0x0002
#define FNTF_SCREENFONTSELECTED		0x0004
#define FNTF_PRINTERFONTSELECTED	0x0008

#define CLRC_FOREGROUND			1
#define CLRC_BACKGROUND			2

#define FNTI_BITMAPFONT			0x0001
#define FNTI_VECTORFONT			0x0002
#define FNTI_FIXEDWIDTHFONT		0x0004
#define FNTI_PROPORTIONALFONT		0x0008
#define FNTI_SYNTHESIZED		0x0010
#define FNTI_DEFAULTLIST		0x0020
#define FNTI_FAMILYNAME			0x0100
#define FNTI_STYLENAME			0x0200
#define FNTI_POINTSIZE			0x0400

#define FNTS_SUCCESSFUL			0
#define FNTS_ERR_INVALID_DIALOG		3
#define FNTS_ERR_ALLOC_SHARED_MEM	4
#define FNTS_ERR_INVALID_PARM		5
#define FNTS_ERR_OUT_OF_MEMORY		7
#define FNTS_ERR_INVALID_VERSION	10
#define FNTS_ERR_DIALOG_LOAD_ERROR	12

#define DID_FONT_DIALOG			300
#define DID_NAME			301
#define DID_STYLE			302
#define DID_DISPLAY_FILTER		303
#define DID_PRINTER_FILTER		304
#define DID_SIZE			305
#define DID_SAMPLE			306
#define DID_OUTLINE			307
#define DID_UNDERSCORE			308
#define DID_STRIKEOUT			309
#define DID_HELP_BUTTON			310
#define DID_APPLY_BUTTON		311
#define DID_RESET_BUTTON		312
#define DID_OK_BUTTON			DID_OK
#define DID_CANCEL_BUTTON		DID_CANCEL
#define DID_NAME_PREFIX			313
#define DID_STYLE_PREFIX		314
#define DID_SIZE_PREFIX			315
#define DID_SAMPLE_GROUPBOX		316
#define DID_EMPHASIS_GROUPBOX		317
#define DID_FONT_ISO_SUPPORT		318
#define DID_FONT_ISO_UNTESTED		319

#define IDS_FONT_SAMPLE			350
#define IDS_FONT_BLANK			351
#define IDS_FONT_KEY_0			352
#define IDS_FONT_KEY_9			353
#define IDS_FONT_KEY_SEP		354
#define IDS_FONT_DISP_ONLY		355
#define IDS_FONT_PRINTER_ONLY		356
#define IDS_FONT_COMBINED		357
#define IDS_FONT_WEIGHT1		358
#define IDS_FONT_WEIGHT2		359
#define IDS_FONT_WEIGHT3		360
#define IDS_FONT_WEIGHT4		361
#define IDS_FONT_WEIGHT5		362
#define IDS_FONT_WEIGHT6		363
#define IDS_FONT_WEIGHT7		364
#define IDS_FONT_WEIGHT8		365
#define IDS_FONT_WEIGHT9		366
#define IDS_FONT_WIDTH1			367
#define IDS_FONT_WIDTH2			368
#define IDS_FONT_WIDTH3			369
#define IDS_FONT_WIDTH4			370
#define IDS_FONT_WIDTH5			371
#define IDS_FONT_WIDTH6			372
#define IDS_FONT_WIDTH7			373
#define IDS_FONT_WIDTH8			374
#define IDS_FONT_WIDTH9			375
#define IDS_FONT_OPTION0		376
#define IDS_FONT_OPTION1		377
#define IDS_FONT_OPTION2		378
#define IDS_FONT_OPTION3		379
#define IDS_FONT_POINT_SIZE_LIST	380

typedef struct _FONTDLG
{
  ULONG	  cbSize;
  HPS	  hpsScreen;
  HPS	  hpsPrinter;
  PSZ	  pszTitle;
  PSZ	  pszPreview;
  PSZ	  pszPtSizeList;
  PFNWP	  pfnDlgProc;
  PSZ	  pszFamilyname;
  FIXED	  fxPointSize;
  ULONG	  fl;
  ULONG	  flFlags;
  ULONG	  flType;
  ULONG	  flTypeMask;
  ULONG	  flStyle;
  ULONG	  flStyleMask;
  LONG	  clrFore;
  LONG	  clrBack;
  ULONG	  ulUser;
  LONG	  lReturn;
  LONG	  lSRC;
  LONG	  lEmHeight;
  LONG	  lXHeight;
  LONG	  lExternalLeading;
  HMODULE hMod;
  FATTRS  fAttrs;
  SHORT	  sNominalPointSize;
  USHORT  usWeight;
  USHORT  usWidth;
  SHORT	  x;
  SHORT	  y;
  USHORT  usDlgId;
  USHORT  usFamilyBufLen;
  USHORT  usReserved;
} FONTDLG;
typedef FONTDLG *PFONTDLG;

typedef struct _STYLECHANGE
{
  USHORT usWeight;
  USHORT usWeightOld;
  USHORT usWidth;
  USHORT usWidthOld;
  ULONG	 flType;
  ULONG	 flTypeOld;
  ULONG	 flTypeMask;
  ULONG	 flTypeMaskOld;
  ULONG	 flStyle;
  ULONG	 flStyleOld;
  ULONG	 flStyleMask;
  ULONG	 flStyleMaskOld;
} STYLECHANGE;
typedef STYLECHANGE *PSTYLECHANGE;

HWND WinFontDlg (HWND hwndP, HWND hwndO, PFONTDLG pfntd);
MRESULT WinDefFontDlgProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

#endif /* INCL_WINSTDFONT */

/* -------------------------- SPIN BUTTON --------------------------------- */

#if defined (INCL_WINSTDSPIN)

#define SPBS_ALLCHARACTERS		0x0000
#define SPBS_NUMERICONLY		0x0001
#define SPBS_READONLY			0x0002
#define SPBS_SERVANT			0x0000
#define SPBS_MASTER			0x0010
#define SPBS_JUSTDEFAULT		0x0000
#define SPBS_JUSTRIGHT			0x0004
#define SPBS_JUSTLEFT			0x0008
#define SPBS_JUSTCENTER			0x000c
#define SPBS_NOBORDER			0x0020
#define SPBS_PADWITHZEROS		0x0080
#define SPBS_FASTSPIN			0x0100

#define SPBM_OVERRIDESETLIMITS		0x0200
#define SPBM_QUERYLIMITS		0x0201
#define SPBM_SETTEXTLIMIT		0x0202
#define SPBM_SPINUP			0x0203
#define SPBM_SPINDOWN			0x0204
#define SPBM_QUERYVALUE			0x0205
#define SPBM_SETARRAY			0x0206
#define SPBM_SETLIMITS			0x0207
#define SPBM_SETCURRENTVALUE		0x0208
#define SPBM_SETMASTER			0x0209

#define SPBN_UPARROW			0x020a
#define SPBN_DOWNARROW			0x020b
#define SPBN_ENDSPIN			0x020c
#define SPBN_CHANGE			0x020d
#define SPBN_SETFOCUS			0x020e
#define SPBN_KILLFOCUS			0x020f

#define SPBQ_UPDATEIFVALID		0
#define SPBQ_ALWAYSUPDATE		1
#define SPBQ_DONOTUPDATE		3

#endif /* INCL_WINSTDSPIN */

/* ----------------------------- SLIDER ----------------------------------- */

#if defined (INCL_WINSTDSLIDER)

#define SLM_ADDDETENT			0x0369
#define SLM_QUERYDETENTPOS		0x036a
#define SLM_QUERYSCALETEXT		0x036b
#define SLM_QUERYSLIDERINFO		0x036c
#define SLM_QUERYTICKPOS		0x036d
#define SLM_QUERYTICKSIZE		0x036e
#define SLM_REMOVEDETENT		0x036f
#define SLM_SETSCALETEXT		0x0370
#define SLM_SETSLIDERINFO		0x0371
#define SLM_SETTICKSIZE			0x0372

#define SLN_CHANGE			1
#define SLN_SLIDERTRACK			2
#define SLN_SETFOCUS			3
#define SLN_KILLFOCUS			4

#define SLS_HORIZONTAL			0x0000
#define SLS_VERTICAL			0x0001
#define SLS_CENTER			0x0000
#define SLS_BOTTOM			0x0002
#define SLS_TOP				0x0004
#define SLS_LEFT			0x0002
#define SLS_RIGHT			0x0004
#define SLS_SNAPTOINCREMENT		0x0008
#define SLS_BUTTONSBOTTOM		0x0010
#define SLS_BUTTONSTOP			0x0020
#define SLS_BUTTONSLEFT			0x0010
#define SLS_BUTTONSRIGHT		0x0020
#define SLS_OWNERDRAW			0x0040
#define SLS_READONLY			0x0080
#define SLS_RIBBONSTRIP			0x0100
#define SLS_HOMEBOTTOM			0x0000
#define SLS_HOMETOP			0x0200
#define SLS_HOMELEFT			0x0000
#define SLS_HOMERIGHT			0x0200
#define SLS_PRIMARYSCALE1		0x0000
#define SLS_PRIMARYSCALE2		0x0400

#define SMA_SCALE1			0x0001
#define SMA_SCALE2			0x0002
#define SMA_SHAFTDIMENSIONS		0x0000
#define SMA_SHAFTPOSITION		0x0001
#define SMA_SLIDERARMDIMENSIONS		0x0002
#define SMA_SLIDERARMPOSITION		0x0003
#define SMA_RANGEVALUE			0x0000
#define SMA_INCREMENTVALUE		0x0001
#define SMA_SETALLTICKS			0xffff

#define SDA_RIBBONSTRIP			1
#define SDA_SLIDERSHAFT			2
#define SDA_BACKGROUND			3
#define SDA_SLIDERARM			4

#define PMERR_UPDATE_IN_PROGRESS	0x1f06
#define SLDERR_INVALID_PARAMETERS	(-1)


typedef struct _SLDCDATA
{
  ULONG	 cbSize;
  USHORT usScale1Increments;
  USHORT usScale1Spacing;
  USHORT usScale2Increments;
  USHORT usScale2Spacing;
} SLDCDATA;
typedef SLDCDATA *PSLDCDATA;

#endif /* INCL_WINSTDSLIDER */

/* ------------------------- CIRCULAR SLIDER ------------------------------ */

#if defined (INCL_WINCIRCULARSLIDER)

#define CSM_QUERYRANGE			0x053d
#define CSM_SETRANGE			0x053e
#define CSM_QUERYVALUE			0x053f
#define CSM_SETVALUE			0x0540
#define CSM_QUERYRADIUS			0x0541
#define CSM_SETINCREMENT		0x0542
#define CSM_QUERYINCREMENT		0x0543
#define CSM_SETBITMAPDATA		0x0544
#define CSN_SETFOCUS			0x0548
#define CSN_CHANGED			0x0549
#define CSN_TRACKING			0x054a
#define CSN_QUERYBACKGROUNDCOLOR	0x054b

#define CSS_NOBUTTON			0x0001
#define CSS_NOTEXT			0x0002
#define CSS_NONUMBER			0x0004
#define CSS_POINTSELECT			0x0008
#define CSS_360				0x0010
#define CSS_MIDPOINT			0x0020
#define CSS_PROPORTIONALTICKS		0x0040
#define CSS_NOTICKS			0x0080
#define CSS_CIRCULARVALUE		0x0100

typedef struct _CSBITMAPDATA
{
  HBITMAP hbmLeftUp;
  HBITMAP hbmLeftDown;
  HBITMAP hbmRightUp;
  HBITMAP hbmRightDown;
} CSBITMAPDATA;
typedef CSBITMAPDATA *PCSBITMAPDATA;

#endif /* INCL_WINCIRCULARSLIDER */

/* ---------------------------- NOTEBOOK ---------------------------------- */

#if defined (INCL_WINSTDBOOK)

#define BFA_PAGEDATA			0x0001
#define BFA_PAGEFROMHWND		0x0002
#define BFA_PAGEFROMDLGTEMPLATE		0x0004
#define BFA_PAGEFROMDLGRES		0x0008
#define BFA_STATUSLINE			0x0010
#define BFA_MAJORTABBITMAP		0x0020
#define BFA_MINORTABBITMAP		0x0040
#define BFA_MAJORTABTEXT		0x0080
#define BFA_MINORTABTEXT		0x0100
#define BFA_BIDIINFO			0x0200

#define BKM_CALCPAGERECT		0x0353
#define BKM_DELETEPAGE			0x0354
#define BKM_INSERTPAGE			0x0355
#define BKM_INVALIDATETABS		0x0356
#define BKM_TURNTOPAGE			0x0357
#define BKM_QUERYPAGECOUNT		0x0358
#define BKM_QUERYPAGEID			0x0359
#define BKM_QUERYPAGEDATA		0x035a
#define BKM_QUERYPAGEWINDOWHWND		0x035b
#define BKM_QUERYTABBITMAP		0x035c
#define BKM_QUERYTABTEXT		0x035d
#define BKM_SETDIMENSIONS		0x035e
#define BKM_SETPAGEDATA			0x035f
#define BKM_SETPAGEWINDOWHWND		0x0360
#define BKM_SETSTATUSLINETEXT		0x0361
#define BKM_SETTABBITMAP		0x0362
#define BKM_SETTABTEXT			0x0363
#define BKM_SETNOTEBOOKCOLORS		0x0364
#define BKM_QUERYPAGESTYLE		0x0365
#define BKM_QUERYSTATUSLINETEXT		0x0366
#define BKM_SETPAGEINFO			0x0367
#define BKM_QUERYPAGEINFO		0x0368
#define BKM_SETTABCOLOR			0x0374
#define BKM_SETNOTEBOOKBUTTONS		0x0375

#define BKN_PAGESELECTED		0x0082
#define BKN_NEWPAGESIZE			0x0083
#define BKN_HELP			0x0084
#define BKN_PAGEDELETED			0x0085
#define BKN_PAGESELECTEDPENDING		0x0086

#define BKA_ALL				0x0001
#define BKA_SINGLE			0x0002
#define BKA_TAB				0x0004

#define BKA_LAST			0x0002
#define BKA_FIRST			0x0004
#define BKA_NEXT			0x0008
#define BKA_PREV			0x0010
#define BKA_TOP				0x0020

#define BKA_MAJORTAB			0x0001
#define BKA_MINORTAB			0x0002
#define BKA_PAGEBUTTON			0x0100

#define BKA_STATUSTEXTON		0x0001
#define BKA_MAJOR			0x0040
#define BKA_MINOR			0x0080
#define BKA_AUTOPAGESIZE		0x0100
#define BKA_END				0x0200

#define BKA_TEXT			0x0400
#define BKA_BITMAP			0x0800

#define BKA_AUTOCOLOR			(-1)
#define BKA_MAXBUTTONID			7999

#define BKS_BACKPAGESBR			0x0001
#define BKS_BACKPAGESBL			0x0002
#define BKS_BACKPAGESTR			0x0004
#define BKS_BACKPAGESTL			0x0008

#define BKS_MAJORTABRIGHT		0x0010
#define BKS_MAJORTABLEFT		0x0020
#define BKS_MAJORTABTOP			0x0040
#define BKS_MAJORTABBOTTOM		0x0080

#define BKS_SQUARETABS			0x0000
#define BKS_ROUNDEDTABS			0x0100
#define BKS_POLYGONTABS			0x0200

#define BKS_SOLIDBIND			0x0000
#define BKS_SPIRALBIND			0x0400

#define BKS_STATUSTEXTLEFT		0x0000
#define BKS_STATUSTEXTRIGHT		0x1000
#define BKS_STATUSTEXTCENTER		0x2000

#define BKS_TABTEXTLEFT			0x0000
#define BKS_TABTEXTRIGHT		0x4000
#define BKS_TABTEXTCENTER		0x8000

#define BKS_BUTTONAREA			0x0200
#define BKS_TABBEDDIALOG		0x0800

#define BKA_BACKGROUNDPAGECOLORINDEX	0x0001
#define BKA_BACKGROUNDPAGECOLOR		0x0002
#define BKA_BACKGROUNDMAJORCOLORINDEX	0x0003
#define BKA_BACKGROUNDMAJORCOLOR	0x0004
#define BKA_BACKGROUNDMINORCOLORINDEX	0x0005
#define BKA_BACKGROUNDMINORCOLOR	0x0006
#define BKA_FOREGROUNDMAJORCOLORINDEX	0x0007
#define BKA_FOREGROUNDMAJORCOLOR	0x0008
#define BKA_FOREGROUNDMINORCOLORINDEX	0x0009
#define BKA_FOREGROUNDMINORCOLOR	0x000a

#define BOOKERR_INVALID_PARAMETERS	(-1)


typedef struct _BOOKTEXT
{
  PSZ	pString;
  ULONG textLen;
} BOOKTEXT;
typedef BOOKTEXT *PBOOKTEXT;

typedef struct _NOTEBOOKBUTTON
{
  PSZ	  pszText;
  ULONG	  idButton;
  LHANDLE hImage;
  LONG	  flStyle;
} NOTEBOOKBUTTON;
typedef NOTEBOOKBUTTON *PNOTEBOOKBUTTON;

typedef struct _DELETENOTIFY
{
  HWND	  hwndBook;
  HWND	  hwndPage;
  ULONG	  ulAppPageData;
  HBITMAP hbmTab;
} DELETENOTIFY;
typedef DELETENOTIFY *PDELETENOTIFY;

typedef struct _PAGESELECTNOTIFY
{
  HWND	hwndBook;
  ULONG ulPageIdCur;
  ULONG ulPageIdNew;
} PAGESELECTNOTIFY;
typedef PAGESELECTNOTIFY *PPAGESELECTNOTIFY;

typedef struct _BOOKPAGEINFO
{
  ULONG	       cb;
  ULONG	       fl;
  BOOL	       bLoadDlg;
  ULONG	       ulPageData;
  HWND	       hwndPage;
  PFN	       pfnPageDlgProc;
  ULONG	       idPageDlg;
  HMODULE      hmodPageDlg;
  PVOID	       pPageDlgCreateParams;
  PDLGTEMPLATE pdlgtPage;
  ULONG	       cbStatusLine;
  PSZ	       pszStatusLine;
  HBITMAP      hbmMajorTab;
  HBITMAP      hbmMinorTab;
  ULONG	       cbMajorTab;
  PSZ	       pszMajorTab;
  ULONG	       cbMinorTab;
  PSZ	       pszMinorTab;
  PVOID	       pBidiInfo;
} BOOKPAGEINFO;
typedef BOOKPAGEINFO *PBOOKPAGEINFO;

#endif /* INCL_WINSTDBOOK */

/* -------------------------- DRAG AND DROP ------------------------------- */

#if defined (INCL_WINSTDDRAG)

#define PMERR_NOT_DRAGGING		0x1f00
#define PMERR_ALREADY_DRAGGING		0x1f01

#define WM_DRAGFIRST			0x0310
#define WM_DRAGLAST			0x032f

#define DM_DROP				0x032f
#define DM_DRAGOVER			0x032e
#define DM_DRAGLEAVE			0x032d
#define DM_DROPHELP			0x032c
#define DM_ENDCONVERSATION		0x032b
#define DM_PRINT			0x032a
#define DM_RENDER			0x0329
#define DM_RENDERCOMPLETE		0x0328
#define DM_RENDERPREPARE		0x0327
#define DM_DRAGFILECOMPLETE		0x0326
#define DM_EMPHASIZETARGET		0x0325
#define DM_DRAGERROR			0x0324
#define DM_FILERENDERED			0x0323
#define DM_RENDERFILE			0x0322
#define DM_DRAGOVERNOTIFY		0x0321
#define DM_PRINTOBJECT			0x0320
#define DM_DISCARDOBJECT		0x031f
#define DM_DROPNOTIFY			0x031e

#define MSGF_DRAG			0x0010

#define DC_OPEN				0x0001
#define DC_REF				0x0002
#define DC_GROUP			0x0004
#define DC_CONTAINER			0x0008
#define DC_PREPARE			0x0010
#define DC_REMOVEABLEMEDIA		0x0020

#define DF_MOVE				0x0001
#define DF_SOURCE			0x0002
#define DF_SUCCESSFUL			0x0004

#define DFF_MOVE			1
#define DFF_COPY			2
#define DFF_DELETE			3

#define DGS_DRAGINPROGRESS		0x0001
#define DGS_LAZYDRAGINPROGRESS		0x0002

#define DME_IGNOREABORT			1
#define DME_IGNORECONTINUE		2
#define DME_REPLACE			3
#define DME_RETRY			4

#define DMFL_TARGETSUCCESSFUL		0x0001
#define DMFL_TARGETFAIL			0x0002
#define DMFL_NATIVERENDER		0x0004
#define DMFL_RENDERRETRY		0x0008
#define DMFL_RENDEROK			0x0010
#define DMFL_RENDERFAIL			0x0020

#define DO_DEFAULT			0xbffe
#define DO_UNKNOWN			0xbfff
#define DO_COPYABLE			0x0001
#define DO_MOVEABLE			0x0002
#define DO_LINKABLE			0x0004
#define DO_CREATEABLE			0x0008
#define DO_CREATEPROGRAMOBJECTABLE	0x0010

#define DO_COPY				0x0010
#define DO_MOVE				0x0020
#define DO_LINK				0x0018
#define DO_CREATE			0x0040
#define DO_CREATEPROGRAMOBJECT		0x0080

#define DOR_NODROP			0x0000
#define DOR_DROP			0x0001
#define DOR_NODROPOP			0x0002
#define DOR_NEVERDROP			0x0003

#define DRG_ICON			0x0001
#define DRG_BITMAP			0x0002
#define DRG_POLYGON			0x0004
#define DRG_STRETCH			0x0008
#define DRG_TRANSPARENT			0x0010
#define DRG_CLOSED			0x0020
#define DRG_MINIBITMAP			0x0040

#define DRR_SOURCE			1
#define DRR_TARGET			2
#define DRR_ABORT			3

#define DRT_ASM				"Assembler Code"
#define DRT_BASIC			"BASIC Code"
#define DRT_BINDATA			"Binary Data"
#define DRT_BITMAP			"Bitmap"
#define DRT_C				"C Code"
#define DRT_COBOL			"COBOL Code"
#define DRT_DLL				"Dynamic Link Library"
#define DRT_DOSCMD			"DOS Command File"
#define DRT_EXE				"Executable"
#define DRT_FORTRAN			"FORTRAN Code"
#define DRT_ICON			"Icon"
#define DRT_LIB				"Library"
#define DRT_METAFILE			"Metafile"
#define DRT_OS2CMD			"OS/2 Command File"
#define DRT_PASCAL			"Pascal Code"
#define DRT_RESOURCE			"Resource File"
#define DRT_TEXT			"Plain Text"
#define DRT_UNKNOWN			"Unknown"

typedef LHANDLE HSTR;

typedef struct _DRAGIMAGE
{
  USHORT  cb;
  USHORT  cptl;
  LHANDLE hImage;
  SIZEL	  sizlStretch;
  ULONG	  fl;
  SHORT	  cxOffset;
  SHORT	  cyOffset;
} DRAGIMAGE;
typedef DRAGIMAGE *PDRAGIMAGE;

typedef struct _DRAGINFO
{
  ULONG	 cbDraginfo;
  USHORT cbDragitem;
  USHORT usOperation;
  HWND	 hwndSource;
  SHORT	 xDrop;
  SHORT	 yDrop;
  USHORT cditem;
  USHORT usReserved;
} DRAGINFO;
typedef DRAGINFO *PDRAGINFO;

typedef struct _DRAGITEM
{
  HWND	 hwndItem;
  ULONG	 ulItemID;
  HSTR	 hstrType;
  HSTR	 hstrRMF;
  HSTR	 hstrContainerName;
  HSTR	 hstrSourceName;
  HSTR	 hstrTargetName;
  SHORT	 cxOffset;
  SHORT	 cyOffset;
  USHORT fsControl;
  USHORT fsSupportedOps;
} DRAGITEM;
typedef DRAGITEM *PDRAGITEM;

typedef struct _DRAGTRANSFER
{
  ULONG	    cb;
  HWND	    hwndClient;
  PDRAGITEM pditem;
  HSTR	    hstrSelectedRMF;
  HSTR	    hstrRenderToName;
  ULONG	    ulTargetInfo;
  USHORT    usOperation;
  USHORT    fsReply;
} DRAGTRANSFER;
typedef DRAGTRANSFER *PDRAGTRANSFER;

typedef struct _RENDERFILE
{
  HWND	 hwndDragFiles;
  HSTR	 hstrSource;
  HSTR	 hstrTarget;
  USHORT fMove;
  USHORT usRsvd;
} RENDERFILE;
typedef RENDERFILE *PRENDERFILE;


BOOL DrgAcceptDroppedFiles (HWND hwnd, PCSZ pszPath, PCSZ pszTypes,
    ULONG ulDefaultOp, ULONG ulReserved);
BOOL DrgAccessDraginfo (PDRAGINFO pdinfo);
HSTR DrgAddStrHandle (PCSZ psz);
PDRAGINFO DrgAllocDraginfo (ULONG cDitem);
PDRAGTRANSFER DrgAllocDragtransfer (ULONG cdxfer);
BOOL DrgCancelLazyDrag (VOID);
BOOL DrgDeleteDraginfoStrHandles (PDRAGINFO pdinfo);
BOOL DrgDeleteStrHandle (HSTR hstr);
HWND DrgDrag (HWND hwndSource, PDRAGINFO pdinfo, PDRAGIMAGE pdimg,
    ULONG cdimg, LONG vkTerminate, PVOID pReserved);
BOOL DrgDragFiles (HWND hwnd, PSZ *apszFiles, PSZ *apszTypes, PSZ *apszTargets,
    ULONG cFiles, HPOINTER hptrDrag, ULONG vkTerm, BOOL fSourceRender,
    ULONG ulReserved);
BOOL DrgFreeDraginfo (PDRAGINFO pdinfo);
BOOL DrgFreeDragtransfer (PDRAGTRANSFER pdxfer);
HPS DrgGetPS (HWND hwnd);
BOOL DrgLazyDrag (HWND hwndSource, PDRAGINFO pDraginfo, PDRAGIMAGE pdimg,
    ULONG cdimg, PVOID pReserved);
BOOL DrgLazyDrop (HWND hwndTarget, ULONG ulOperation, PPOINTL pptlDrop);
BOOL DrgPostTransferMsg (HWND hwnd, ULONG msg, PDRAGTRANSFER pdxfer, ULONG fl,
    ULONG ulReserved, BOOL fRetry);
BOOL DrgPushDraginfo (PDRAGINFO pdinfo, HWND hwndDest);
PDRAGINFO DrgQueryDraginfoPtr (PDRAGINFO pReserved);
PDRAGINFO DrgQueryDraginfoPtrFromHwnd (HWND hwndSource);
PDRAGINFO DrgQueryDraginfoPtrFromDragitem (__const__ DRAGITEM *pDragitem);
BOOL DrgQueryDragitem (PDRAGINFO pdinfo, ULONG cbBuffer, PDRAGITEM pditem,
    ULONG iItem);
ULONG DrgQueryDragitemCount (PDRAGINFO pdinfo);
PDRAGITEM DrgQueryDragitemPtr (PDRAGINFO pdinfo, ULONG ulIndex);
ULONG DrgQueryDragStatus (VOID);
BOOL DrgQueryNativeRMF (PDRAGITEM pditem, ULONG cbBuffer, PCHAR pBuffer);
ULONG DrgQueryNativeRMFLen (PDRAGITEM pditem);
ULONG DrgQueryStrName (HSTR hstr, ULONG cbBuffer, PSZ pBuffer);
ULONG DrgQueryStrNameLen (HSTR hstr);
BOOL DrgQueryTrueType (PDRAGITEM pditem, ULONG cbBuffer, PSZ pBuffer);
ULONG DrgQueryTrueTypeLen (PDRAGITEM pditem);
PDRAGINFO DrgReallocDraginfo (PDRAGINFO pDraginfoOld, ULONG cditem);
BOOL DrgReleasePS (HPS hps);
MRESULT DrgSendTransferMsg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
BOOL DrgSetDragImage (PDRAGINFO pdinfo, PDRAGIMAGE pdimg, ULONG cdimg,
    PVOID pReserved);
BOOL DrgSetDragitem (PDRAGINFO pdinfo, PDRAGITEM pditem, ULONG cbBuffer,
    ULONG iItem);
BOOL DrgSetDragPointer (PDRAGINFO pdinfo, HPOINTER hptr);
BOOL DrgVerifyNativeRMF (PDRAGITEM pditem, PCSZ pszRMF);
BOOL DrgVerifyRMF (PDRAGITEM pditem, PCSZ pszMech, PCSZ pszFmt);
BOOL DrgVerifyTrueType (PDRAGITEM pditem, PCSZ pszType);
BOOL DrgVerifyType (PDRAGITEM pditem, PCSZ pszType);
BOOL DrgVerifyTypeSet (PDRAGITEM pditem, PCSZ pszType, ULONG cbMatch,
    PSZ pszMatch);

#endif /* INCL_WINSTDDRAG */

/* -------------------------- VALUE SET ----------------------------------- */

#if defined (INCL_WINSTDVALSET)

#define VDA_ITEM			0x0001
#define VDA_ITEMBACKGROUND		0x0002
#define VDA_SURROUNDING			0x0003
#define VDA_BACKGROUND			0x0004

#define VIA_BITMAP			0x0001
#define VIA_ICON			0x0002
#define VIA_TEXT			0x0004
#define VIA_RGB				0x0008
#define VIA_COLORINDEX			0x0010
#define VIA_OWNERDRAW			0x0020
#define VIA_DISABLED			0x0040
#define VIA_DRAGGABLE			0x0080
#define VIA_DROPONABLE			0x0100

#define VM_QUERYITEM			0x0375
#define VM_QUERYITEMATTR		0x0376
#define VM_QUERYMETRICS			0x0377
#define VM_QUERYSELECTEDITEM		0x0378
#define VM_SELECTITEM			0x0379
#define VM_SETITEM			0x037a
#define VM_SETITEMATTR			0x037b
#define VM_SETMETRICS			0x037c

#define VMA_ITEMSIZE			0x0001
#define VMA_ITEMSPACING			0x0002

#define VN_SELECT			0x0078
#define VN_ENTER			0x0079
#define VN_DRAGLEAVE			0x007a
#define VN_DRAGOVER			0x007b
#define VN_DROP				0x007c
#define VN_DROPHELP			0x007d
#define VN_INITDRAG			0x007e
#define VN_SETFOCUS			0x007f
#define VN_KILLFOCUS			0x0080
#define VN_HELP				0x0081

#define VS_BITMAP			0x0001
#define VS_ICON				0x0002
#define VS_TEXT				0x0004
#define VS_RGB				0x0008
#define VS_COLORINDEX			0x0010
#define VS_BORDER			0x0020
#define VS_ITEMBORDER			0x0040
#define VS_SCALEBITMAPS			0x0080
#define VS_RIGHTTOLEFT			0x0100
#define VS_OWNERDRAW			0x0200

#define VSERR_INVALID_PARAMETERS	(-1)


typedef struct _VSCDATA
{
  ULONG	 cbSize;
  USHORT usRowCount;
  USHORT usColumnCount;
} VSCDATA;
typedef VSCDATA *PVSCDATA;

typedef struct _VSDRAGINIT
{
  HWND	 hwnd;
  LONG	 x;
  LONG	 y;
  LONG	 cx;
  LONG	 cy;
  USHORT usRow;
  USHORT usColumn;
} VSDRAGINIT;
typedef VSDRAGINIT *PVSDRAGINIT;

typedef struct _VSDRAGINFO
{
  PDRAGINFO pDragInfo;
  USHORT    usRow;
  USHORT    usColumn;
} VSDRAGINFO;
typedef VSDRAGINFO *PVSDRAGINFO;

typedef struct _VSTEXT
{
  PSZ	pszItemText;
  ULONG ulBufLen;
} VSTEXT;
typedef VSTEXT *PVSTEXT;

#endif /* INCL_WINSTDVALSET */

/* ---------------------------- CONTAINER --------------------------------- */

#if defined (INCL_WINSTDCNR)

#define CA_CONTAINERTITLE		0x00000200
#define CA_TITLESEPARATOR		0x00000400
#define CA_TITLELEFT			0x00000800
#define CA_TITLERIGHT			0x00001000
#define CA_TITLECENTER			0x00002000
#define CA_OWNERDRAW			0x00004000
#define CA_DETAILSVIEWTITLES		0x00008000
#define CA_ORDEREDTARGETEMPH		0x00010000
#define CA_DRAWBITMAP			0x00020000
#define CA_DRAWICON			0x00040000
#define CA_TITLEREADONLY		0x00080000
#define CA_OWNERPAINTBACKGROUND		0x00100000
#define CA_MIXEDTARGETEMPH		0x00200000
#define CA_TREELINE			0x00400000

#define CCS_EXTENDSEL			0x0001
#define CCS_MULTIPLESEL			0x0002
#define CCS_SINGLESEL			0x0004
#define CCS_AUTOPOSITION		0x0008
#define CCS_VERIFYPOINTERS		0x0010
#define CCS_READONLY			0x0020
#define CCS_MINIRECORDCORE		0x0040
#define CCS_MINIICONS			0x0800
#define CCS_NOCONTROLPTR		0x1000

#define CFA_LEFT			0x00000001
#define CFA_RIGHT			0x00000002
#define CFA_CENTER			0x00000004
#define CFA_TOP				0x00000008
#define CFA_VCENTER			0x00000010
#define CFA_BOTTOM			0x00000020
#define CFA_INVISIBLE			0x00000040
#define CFA_BITMAPORICON		0x00000100
#define CFA_SEPARATOR			0x00000200
#define CFA_HORZSEPARATOR		0x00000400
#define CFA_STRING			0x00000800
#define CFA_OWNER			0x00001000
#define CFA_DATE			0x00002000
#define CFA_TIME			0x00004000
#define CFA_FIREADONLY			0x00008000
#define CFA_FITITLEREADONLY		0x00010000
#define CFA_ULONG			0x00020000
#define CFA_RANGE			0x00040000
#define CFA_NEWCOMP			0x00080000
#define CFA_OBJECT			0x00100000
#define CFA_LIST			0x00200000
#define CFA_CLASS			0x00400000
#define CFA_IGNORE			0x80000000

#define CID_LEFTCOLTITLEWND		0x7ff0
#define CID_RIGHTCOLTITLEWND		0x7ff1
#define CID_BLANKBOX			0x7ff2
#define CID_HSCROLL			0x7ff3
#define CID_RIGHTHSCROLL		0x7ff4
#define CID_CNRTITLEWND			0x7ff5
#define CID_LEFTDVWND			0x7ff7
#define CID_RIGHTDVWND			0x7ff8
#define CID_VSCROLL			0x7ff9
#define CID_MLE				0x7ffa

#define CM_ALLOCDETAILFIELDINFO		0x0330
#define CM_ALLOCRECORD			0x0331
#define CM_ARRANGE			0x0332
#define CM_ERASERECORD			0x0333
#define CM_FILTER			0x0334
#define CM_FREEDETAILFIELDINFO		0x0335
#define CM_FREERECORD			0x0336
#define CM_HORZSCROLLSPLITWINDOW	0x0337
#define CM_INSERTDETAILFIELDINFO	0x0338
#define CM_INSERTRECORD			0x0339
#define CM_INVALIDATEDETAILFIELDINFO	0x033a
#define CM_INVALIDATERECORD		0x033b
#define CM_PAINTBACKGROUND		0x033c
#define CM_QUERYCNRINFO			0x033d
#define CM_QUERYDETAILFIELDINFO		0x033e
#define CM_QUERYDRAGIMAGE		0x033f
#define CM_QUERYRECORD			0x0340
#define CM_QUERYRECORDEMPHASIS		0x0341
#define CM_QUERYRECORDFROMRECT		0x0342
#define CM_QUERYRECORDRECT		0x0343
#define CM_QUERYVIEWPORTRECT		0x0344
#define CM_REMOVEDETAILFIELDINFO	0x0345
#define CM_REMOVERECORD			0x0346
#define CM_SCROLLWINDOW			0x0347
#define CM_SEARCHSTRING			0x0348
#define CM_SETCNRINFO			0x0349
#define CM_SETRECORDEMPHASIS		0x034a
#define CM_SORTRECORD			0x034b
#define CM_OPENEDIT			0x034c
#define CM_CLOSEEDIT			0x034d
#define CM_COLLAPSETREE			0x034e
#define CM_EXPANDTREE			0x034f
#define CM_QUERYRECORDINFO		0x0350
#define CM_INSERTRECORDARRAY		0x0351
#define CM_MOVETREE			0x0352
#define CM_SETTEXTVISIBILITY		0x0353
#define CM_SETGRIDINFO			0x0354
#define CM_QUERYGRIDINFO		0x0355
#define CM_SNAPTOGRID			0x0356

#define CMA_TOP				0x0001
#define CMA_BOTTOM			0x0002
#define CMA_LEFT			0x0004
#define CMA_RIGHT			0x0008
#define CMA_PERIMETER			0x0010
#define CMA_USER			0x0020

#define CMA_FIRST			0x0010
#define CMA_LAST			0x0020
#define CMA_END				0x0040
#define CMA_PREV			0x0080
#define CMA_NEXT			0x0100

#define CMA_HORIZONTAL			0x0200
#define CMA_VERTICAL			0x0400
#define CMA_ICON			0x0800
#define CMA_TEXT			0x1000
#define CMA_PARTIAL			0x2000
#define CMA_COMPLETE			0x4000

#define CMA_PARENT			0x0001
#define CMA_FIRSTCHILD			0x0002
#define CMA_LASTCHILD			0x0004

#define CMA_CNRTITLE			0x0001
#define CMA_DELTA			0x0002
#define CMA_FLWINDOWATTR		0x0004
#define CMA_LINESPACING			0x0008
#define CMA_PFIELDINFOLAST		0x0010

#define CMA_PSORTRECORD			0x0020
#define CMA_PTLORIGIN			0x0040
#define CMA_SLBITMAPORICON		0x0080
#define CMA_XVERTSPLITBAR		0x0100
#define CMA_PFIELDINFOOBJECT		0x0200

#define CMA_TREEICON			0x0400
#define CMA_TREEBITMAP			0x0800
#define CMA_CXTREEINDENT		0x1000
#define CMA_CXTREELINE			0x2000
#define CMA_SLTREEBITMAPORICON		0x4000

#define CMA_ITEMORDER			0x0001
#define CMA_WINDOW			0x0002
#define CMA_WORKSPACE			0x0004
#define CMA_ZORDER			0x0008

#define CMA_DELTATOP			0x0001
#define CMA_DELTABOT			0x0002
#define CMA_DELTAHOME			0x0004
#define CMA_DELTAEND			0x0008

#define CMA_NOREPOSITION		0x0001
#define CMA_REPOSITION			0x0002
#define CMA_TEXTCHANGED			0x0004
#define CMA_ERASE			0x0008
#define CMA_NOTEXTCHANGED		0x0010
#define CMA_FILTER			0x1000

#define CMA_FREE			0x0001
#define CMA_INVALIDATE			0x0002

#define CMA_ARRANGESTANDARD		0x0000
#define CMA_ARRANGEGRID			0x0001
#define CMA_ARRANGESELECTED		0x0002

#define CMA_AVAIL			0x0001
#define CMA_UNAVAIL			0x0002

#define CN_DRAGAFTER			0x0065
#define CN_DRAGLEAVE			0x0066
#define CN_DRAGOVER			0x0067
#define CN_DROP				0x0068
#define CN_DROPHELP			0x0069
#define CN_ENTER			0x006a
#define CN_INITDRAG			0x006b
#define CN_EMPHASIS			0x006c
#define CN_KILLFOCUS			0x006d
#define CN_SCROLL			0x006e
#define CN_QUERYDELTA			0x006f
#define CN_SETFOCUS			0x0070
#define CN_REALLOCPSZ			0x0071
#define CN_BEGINEDIT			0x0072
#define CN_ENDEDIT			0x0073
#define CN_COLLAPSETREE			0x0074
#define CN_EXPANDTREE			0x0075
#define CN_HELP				0x0076
#define CN_CONTEXTMENU			0x0077
#define CN_VERIFYEDIT			0x0086
#define CN_PICKUP			0x0087
#define CN_DROPNOTIFY			0x0088
#define CN_GRIDRESIZED			0x0089

#define CRA_SELECTED			0x00000001
#define CRA_TARGET			0x00000002
#define CRA_CURSORED			0x00000004
#define CRA_INUSE			0x00000008
#define CRA_FILTERED			0x00000010
#define CRA_DROPONABLE			0x00000020
#define CRA_RECORDREADONLY		0x00000040
#define CRA_EXPANDED			0x00000080
#define CRA_COLLAPSED			0x00000100
#define CRA_PICKED			0x00000200
#define CRA_LOCKED			0x00000400
#define CRA_DISABLED			0x00001000
#define CRA_SOURCE			0x00004000
#define CRA_IGNORE			0x00008000
#define CRA_OWNERFREE			0x00010000
#define CRA_OWNERDRAW			0x00020000

#define CV_TEXT				0x00000001
#define CV_NAME				0x00000002
#define CV_ICON				0x00000004
#define CV_DETAIL			0x00000008
#define CV_FLOW				0x00000010
#define CV_MINI				0x00000020
#define CV_TREE				0x00000040
#define CV_GRID				0x00000080
#define CV_EXACTLENGTH			0x10000000

#define PMERR_NOFILTERED_ITEMS		0x1f02
#define PMERR_COMPARISON_FAILED		0x1f03
#define PMERR_RECORD_CURRENTLY_INSERTED 0x1f04
#define PMERR_FI_CURRENTLY_INSERTED	0x1f05


typedef struct _TREEITEMDESC
{
  HBITMAP  hbmExpanded;
  HBITMAP  hbmCollapsed;
  HPOINTER hptrExpanded;
  HPOINTER hptrCollapsed;
} TREEITEMDESC;
typedef TREEITEMDESC *PTREEITEMDESC;

typedef struct _FIELDINFO
{
  ULONG		    cb;
  ULONG		    flData;
  ULONG		    flTitle;
  PVOID		    pTitleData;
  ULONG		    offStruct;
  PVOID		    pUserData;
  struct _FIELDINFO *pNextFieldInfo;
  ULONG		    cxWidth;
} FIELDINFO;
typedef FIELDINFO *PFIELDINFO;

typedef struct _RECORDCORE
{
  ULONG		     cb;
  ULONG		     flRecordAttr;
  POINTL	     ptlIcon;
  struct _RECORDCORE *preccNextRecord;
  PSZ		     pszIcon;
  HPOINTER	     hptrIcon;
  HPOINTER	     hptrMiniIcon;
  HBITMAP	     hbmBitmap;
  HBITMAP	     hbmMiniBitmap;
  PTREEITEMDESC	     pTreeItemDesc;
  PSZ		     pszText;
  PSZ		     pszName;
  PSZ		     pszTree;
} RECORDCORE;
typedef RECORDCORE *PRECORDCORE;

typedef struct _MINIRECORDCORE
{
  ULONG			 cb;
  ULONG			 flRecordAttr;
  POINTL		 ptlIcon;
  struct _MINIRECORDCORE *preccNextRecord;
  PSZ			 pszIcon;
  HPOINTER		 hptrIcon;
} MINIRECORDCORE;
typedef MINIRECORDCORE *PMINIRECORDCORE;

typedef struct _TREEMOVE
{
  PRECORDCORE preccMove;
  PRECORDCORE preccNewParent;
  PRECORDCORE pRecordOrder;
  BOOL	      flMoveSiblings;
} TREEMOVE;
typedef TREEMOVE *PTREEMOVE;

typedef struct _CNRINFO
{
  ULONG	     cb;
  PVOID	     pSortRecord;
  PFIELDINFO pFieldInfoLast;
  PFIELDINFO pFieldInfoObject;
  PSZ	     pszCnrTitle;
  ULONG	     flWindowAttr;
  POINTL     ptlOrigin;
  ULONG	     cDelta;
  ULONG	     cRecords;
  SIZEL	     slBitmapOrIcon;
  SIZEL	     slTreeBitmapOrIcon;
  HBITMAP    hbmExpanded;
  HBITMAP    hbmCollapsed;
  HPOINTER   hptrExpanded;
  HPOINTER   hptrCollapsed;
  LONG	     cyLineSpacing;
  LONG	     cxTreeIndent;
  LONG	     cxTreeLine;
  ULONG	     cFields;
  LONG	     xVertSplitbar;
} CNRINFO;
typedef CNRINFO *PCNRINFO;

typedef struct _GRIDSQUARE
{
  ULONG ulNumber;
  ULONG ulState;
  RECTL rctlSquare;
} GRIDSQUARE;
typedef GRIDSQUARE *PGRIDSQUARE;

typedef struct _GRIDINFO
{
  ULONG cb;
  SHORT cxGrid;
  SHORT cyGrid;
  SHORT sGridRows;
  SHORT sGridCols;
  LONG	cGridSquares;
  PGRIDSQUARE pGrid;
} GRIDINFO;
typedef GRIDINFO *PGRIDINFO;

typedef struct _CDATE
{
  UCHAR	 day;
  UCHAR	 month;
  USHORT year;
} CDATE;
typedef CDATE *PCDATE;

typedef struct _CTIME
{
  UCHAR hours;
  UCHAR minutes;
  UCHAR seconds;
  UCHAR ucReserved;
} CTIME;
typedef CTIME *PCTIME;

typedef struct _CNRDRAGINIT
{
  HWND	      hwndCnr;
  PRECORDCORE pRecord;
  LONG	      x;
  LONG	      y;
  LONG	      cx;
  LONG	      cy;
} CNRDRAGINIT;
typedef CNRDRAGINIT *PCNRDRAGINIT;

typedef struct _FIELDINFOINSERT
{
  ULONG	     cb;
  PFIELDINFO pFieldInfoOrder;
  ULONG	     fInvalidateFieldInfo;
  ULONG	     cFieldInfoInsert;
} FIELDINFOINSERT;
typedef FIELDINFOINSERT *PFIELDINFOINSERT;

typedef struct _RECORDINSERT
{
  ULONG	      cb;
  PRECORDCORE pRecordOrder;
  PRECORDCORE pRecordParent;
  ULONG	      fInvalidateRecord;
  ULONG	      zOrder;
  ULONG	      cRecordsInsert;
} RECORDINSERT;
typedef RECORDINSERT *PRECORDINSERT;

typedef struct _QUERYRECFROMRECT
{
  ULONG cb;
  RECTL rect;
  ULONG fsSearch;
} QUERYRECFROMRECT;
typedef QUERYRECFROMRECT *PQUERYRECFROMRECT;

typedef struct _QUERYRECORDRECT
{
  ULONG	      cb;
  PRECORDCORE pRecord;
  ULONG	      fRightSplitWindow;
  ULONG	      fsExtent;
} QUERYRECORDRECT;
typedef QUERYRECORDRECT *PQUERYRECORDRECT;

typedef struct _SEARCHSTRING
{
  ULONG cb;
  PSZ	pszSearch;
  ULONG fsPrefix;
  ULONG fsCaseSensitive;
  ULONG usView;
} SEARCHSTRING;
typedef SEARCHSTRING *PSEARCHSTRING;

typedef struct _CNRDRAGINFO
{
  PDRAGINFO   pDragInfo;
  PRECORDCORE pRecord;
} CNRDRAGINFO;
typedef CNRDRAGINFO *PCNRDRAGINFO;

typedef struct _CNRLAZYDRAGINFO
{
  PDRAGINFO   pDragInfo;
  PRECORDCORE pRecord;
  HWND	      hwndTarget;
} CNRLAZYDRAGINFO;
typedef CNRLAZYDRAGINFO *PCNRLAZYDRAGINFO;

typedef struct _NOTIFYRECORDEMPHASIS
{
  HWND	      hwndCnr;
  PRECORDCORE pRecord;
  ULONG	      fEmphasisMask;
} NOTIFYRECORDEMPHASIS;
typedef NOTIFYRECORDEMPHASIS *PNOTIFYRECORDEMPHASIS;

typedef struct _NOTIFYRECORDENTER
{
  HWND	      hwndCnr;
  ULONG	      fKey;
  PRECORDCORE pRecord;
} NOTIFYRECORDENTER;
typedef NOTIFYRECORDENTER *PNOTIFYRECORDENTER;

typedef struct _NOTIFYDELTA
{
  HWND	hwndCnr;
  ULONG fDelta;
} NOTIFYDELTA;
typedef NOTIFYDELTA *PNOTIFYDELTA;

typedef struct _NOTIFYSCROLL
{
  HWND	hwndCnr;
  LONG	lScrollInc;
  ULONG fScroll;
} NOTIFYSCROLL;
typedef NOTIFYSCROLL *PNOTIFYSCROLL;

typedef struct _CNREDITDATA
{
  ULONG	      cb;
  HWND	      hwndCnr;
  PRECORDCORE pRecord;
  PFIELDINFO  pFieldInfo;
  PSZ	      *ppszText;
  ULONG	      cbText;
  ULONG	      id;
} CNREDITDATA;
typedef CNREDITDATA *PCNREDITDATA;

typedef struct _OWNERBACKGROUND
{
  HWND	hwnd;
  HPS	hps;
  RECTL rclBackground;
  LONG	idWindow;
} OWNERBACKGROUND;
typedef OWNERBACKGROUND *POWNERBACKGROUND;

typedef struct _CNRDRAWITEMINFO
{
  PRECORDCORE pRecord;
  PFIELDINFO  pFieldInfo;
} CNRDRAWITEMINFO;
typedef CNRDRAWITEMINFO *PCNRDRAWITEMINFO;

#endif /* INCL_WINSTDCNR */

/* ------------------------- WORKPLACE SHELL ------------------------------ */

#if defined (INCL_WPCLASS) || !defined (INCL_NOCOMMON)

#define CCHMAXCLASS			3

#define QC_First			0
#define QC_Next				1
#define QC_Last				2
#define QC_FIRST			QC_First
#define QC_NEXT				QC_Next
#define QC_LAST				QC_Last

#define CO_FAILIFEXISTS			0
#define CO_REPLACEIFEXISTS		1
#define CO_UPDATEIFEXISTS		2

#if !defined (LOCATION_DESKTOP)
#define LOCATION_DESKTOP		((PSZ)0xffff0001)
#endif

typedef struct _OBJCLASS
{
  struct _OBJCLASS *pNext;
  PSZ		    pszClassName;
  PSZ		    pszModName;
} OBJCLASS;
typedef OBJCLASS *POBJCLASS;

HOBJECT WinCopyObject (HOBJECT hObjectofObject, HOBJECT hObjectofDest,
    ULONG ulReserved);
HOBJECT WinCreateObject (PCSZ pszClassName, PCSZ pszTitle, PCSZ pszSetupString,
    PCSZ pszLocation, ULONG ulFlags);
HOBJECT WinCreateShadow (HOBJECT hObjectofObject, HOBJECT hObjectofDest,
    ULONG ulReserved);
BOOL WinDeregisterObjectClass (PCSZ pszClassName);
BOOL WinDestroyObject (HOBJECT hObject);
BOOL WinEnumObjectClasses (POBJCLASS pObjClass, PULONG pulSize);
BOOL WinIsSOMDDReady (VOID);
BOOL WinIsWPDServerReady (VOID);
HOBJECT WinMoveObject (HOBJECT hObjectofObject, HOBJECT hObjectofDest,
    ULONG ulReserved);
BOOL WinOpenObject (HOBJECT hObject, ULONG ulView, BOOL fFlag);
BOOL WinQueryActiveDesktopPathname (PSZ pszPathName, ULONG ulSize);
HOBJECT WinQueryObject (PCSZ pszObjectID);
BOOL WinQueryObjectPath (HOBJECT hobject, PSZ pszPathName, ULONG ulSize);
BOOL WinRegisterObjectClass (PCSZ pszClassName, PCSZ pszModName);
BOOL WinReplaceObjectClass (PCSZ pszOldClassName, PCSZ pszNewClassName,
    BOOL fReplace);
ULONG WinRestartSOMDD (BOOL fState);
ULONG WinRestartWPDServer (BOOL fState);
BOOL WinSaveObject (HOBJECT hObject, BOOL fAsync);
BOOL WinSetObjectData (HOBJECT hObject, PCSZ pszSetupString);

#endif /* INCL_WPCLASS || !INCL_NOCOMMON */

#if !defined (INCL_NOCOMMON)

BOOL WinFreeFileIcon (HPOINTER hptr);
HPOINTER WinLoadFileIcon (PCSZ pszFileName, BOOL fPrivate);
BOOL WinRestoreWindowPos (PCSZ pszAppName, PCSZ pszKeyName, HWND hwnd);
#if defined (INCL_WINPOINTERS)
BOOL WinSetFileIcon (PCSZ pszFileName, __const__ ICONINFO *pIconInfo);
#endif
BOOL WinShutdownSystem (HAB hab, HMQ hmq);
BOOL WinStoreWindowPos (PCSZ pszAppName, PCSZ pszKeyName, HWND hwnd);

#endif /* !INCL_NOCOMMON */

/* ---------------------------- SPOOLER ----------------------------------- */

#if defined (INCL_SPL)

#define SPL_ERROR			0
#define SPL_OK				1

#define SPL_INI_SPOOLER			"PM_SPOOLER"
#define SPL_INI_QUEUE			"PM_SPOOLER_QUEUE"
#define SPL_INI_PRINTER			"PM_SPOOLER_PRINTER"
#define SPL_INI_PRINTERDESCR		"PM_SPOOLER_PRINTER_DESCR"
#define SPL_INI_QUEUEDESCR		"PM_SPOOLER_QUEUE_DESCR"
#define SPL_INI_QUEUEDD			"PM_SPOOLER_QUEUE_DD"
#define SPL_INI_QUEUEDDDATA		"PM_SPOOLER_QUEUE_DDDATA"

#define SPLC_ABORT			1
#define SPLC_PAUSE			2
#define SPLC_CONTINUE			3

#define SPLDATA_PRINTERJAM		0x0001
#define SPLDATA_FORMCHGREQD		0x0002
#define SPLDATA_CARTCHGREQD		0x0004
#define SPLDATA_PENCHGREQD		0x0008
#define SPLDATA_DATAERROR		0x0010
#define SPLDATA_UNEXPECTERROR		0x0020
#define SPLDATA_OTHER			0x8000

#define SPLINFO_QPERROR			0x0001
#define SPLINFO_DDERROR			0x0002
#define SPLINFO_SPLERROR		0x0004
#define SPLINFO_OTHERERROR		0x0080
#define SPLINFO_INFORMATION		0x0100
#define SPLINFO_WARNING			0x0200
#define SPLINFO_ERROR			0x0400
#define SPLINFO_SEVERE			0x0800
#define SPLINFO_USERINTREQD		0x1000

#define SPLPORT_VERSION_REGULAR		0
#define SPLPORT_VERSION_VIRTUAL		1

#define SSQL_ERROR			(-1)

#define QP_RAWDATA_BYPASS		0x0001
#define QP_PRINT_SEPARATOR_PAGE		0x0002

#define QPDAT_ADDRESS			0
#define QPDAT_DRIVER_NAME		1
#define QPDAT_DRIVER_DATA		2
#define QPDAT_DATA_TYPE			3
#define QPDAT_COMMENT			4
#define QPDAT_PROC_PARAMS		5
#define QPDAT_SPL_PARAMS		6
#define QPDAT_NET_PARAMS		7
#define QPDAT_DOC_NAME			8
#define QPDAT_QUEUE_NAME		9
#define QPDAT_TOKEN			10
#define QPDAT_JOBID			11

typedef LHANDLE HSPL;
typedef LHANDLE HSTD;
typedef HSTD *PHSTD;
typedef PSZ *PQMOPENDATA;
typedef unsigned long SPLERR;

typedef struct _SQPOPENDATA
{
  PSZ	    pszLogAddress;
  PSZ	    pszDriverName;
  PDRIVDATA pdriv;
  PSZ	    pszDataType;
  PSZ	    pszComment;
  PSZ	    pszProcParams;
  PSZ	    pszSpoolParams;
  PSZ	    pszNetworkParams;
  PSZ	    pszDocName;
  PSZ	    pszQueueName;
  PSZ	    pszToken;
  USHORT    idJobId;
} SQPOPENDATA;
typedef SQPOPENDATA *PSQPOPENDATA;


BOOL SplStdClose (HDC hdc);
BOOL SplStdDelete (HSTD hMetaFile);
BOOL SplStdGetBits (HSTD hMetaFile, LONG offData, LONG cbData, PCH pchData);
BOOL SplStdOpen (HDC hdc);
LONG SplStdQueryLength (HSTD hMetaFile);
BOOL SplStdStart (HDC hdc);
HSTD SplStdStop (HDC hdc);

SPLERR SplControlDevice (PSZ pszComputerName, PSZ pszPortName,
    ULONG ulControl);
SPLERR SplCopyJob (PCSZ pszSrcComputerName, PCSZ pszSrcQueueName,
    ULONG ulSrcJob, PCSZ pszTrgComputerName, PCSZ pszTrgQueueName,
    PULONG pulTrgJob);
SPLERR SplCreateDevice (PSZ pszComputerName, ULONG ulLevel, PVOID pBuf,
    ULONG cbBuf);
SPLERR SplCreatePort (PCSZ pszComputerName, PCSZ pszPortName,
    PCSZ pszPortDriver, ULONG ulVersion, PVOID pBuf, ULONG cbBuf);
SPLERR SplCreateQueue (PSZ pszComputerName, ULONG ulLevel, PVOID pBuf,
    ULONG cbBuf);
SPLERR SplDeleteDevice (PSZ pszComputerName, PSZ pszPrintDeviceName);
SPLERR SplDeleteJob (PSZ pszComputerName, PSZ pszQueueName, ULONG ulJob);
SPLERR SplDeletePort (PCSZ pszComputerName, PCSZ pszPortName);
SPLERR SplDeleteQueue (PSZ pszComputerName, PSZ pszQueueName);
SPLERR SplEnumDevice (PSZ pszComputerName, ULONG ulLevel, PVOID pBuf,
    ULONG cbBuf, PULONG pcReturned, PULONG pcTotal, PULONG pcbNeeded,
    PVOID pReserved);
SPLERR SplEnumDriver (PSZ pszComputerName, ULONG ulLevel, PVOID pBuf,
    ULONG cbBuf, PULONG pcReturned, PULONG pcTotal, PULONG pcbNeeded,
    PVOID pReserved);
SPLERR SplEnumJob (PSZ pszComputerName, PSZ pszQueueName, ULONG ulLevel,
    PVOID pBuf, ULONG cbBuf, PULONG pcReturned, PULONG pcTotal,
    PULONG pcbNeeded, PVOID pReserved);
SPLERR SplEnumPort (PCSZ pszComputerName, ULONG ulLevel, PVOID pBuf,
    ULONG cbBuf, PULONG pcReturned, PULONG pcTotal, PULONG pcbNeeded,
    PVOID pReserved);
SPLERR SplEnumPrinter (PSZ pszComputerName, ULONG uLevel, ULONG flType,
    PVOID pBuf, ULONG cbbuf, PULONG pcReturned, PULONG pcTotal,
    PULONG pcbNeeded, PVOID pReserved);
SPLERR SplEnumQueue (PSZ pszComputerName, ULONG ulLevel, PVOID pBuf,
    ULONG cbBuf, PULONG pcReturned, PULONG pcTotal, PULONG pcbNeeded,
    PVOID pReserved);
SPLERR SplEnumQueueProcessor (PSZ pszComputerName, ULONG ulLevel, PVOID pBuf,
    ULONG cbBuf, PULONG pcReturned, PULONG pcTotal, PULONG pcbNeeded,
    PVOID pReserved);
SPLERR SplHoldJob (PCSZ pszComputerName, PCSZ pszQueueName, ULONG ulJob);
SPLERR SplHoldQueue (PSZ pszComputerName, PSZ pszQueueName);
SPLERR SplPurgeQueue (PSZ pszComputerName, PSZ pszQueueName);
SPLERR SplQueryDevice (PSZ pszComputerName, PSZ pszPrintDeviceName,
    ULONG ulLevel, PVOID pBuf, ULONG cbBuf, PULONG pcbNeeded);
SPLERR SplQueryDriver (PCSZ pszComputerName, PCSZ pszDriverName,
    PCSZ pszPrinterName, ULONG ulLevel, PVOID pBuf, ULONG cbBuf,
    PULONG pcbNeeded);
SPLERR SplQueryJob (PSZ pszComputerName, PSZ pszQueueName, ULONG ulJob,
    ULONG ulLevel, PVOID pBuf, ULONG cbBuf, PULONG pcbNeeded);
SPLERR SplQueryPort (PCSZ pszComputerName, PCSZ pszPortName, ULONG ulLevel,
    PVOID pBuf, ULONG cbBuf, PULONG pcbNeeded);
SPLERR SplQueryQueue (PSZ pszComputerName, PSZ pszQueueName, ULONG ulLevel,
    PVOID pBuf, ULONG cbBuf, PULONG pcbNeeded);
SPLERR SplReleaseJob (PCSZ pszComputerName, PCSZ pszQueueName, ULONG ulJob);
SPLERR SplReleaseQueue (PSZ pszComputerName, PSZ pszQueueName);
SPLERR SplSetDevice (PSZ pszComputerName, PSZ pszPrintDeviceName,
    ULONG ulLevel, PVOID pBuf, ULONG cbBuf, ULONG ulParmNum);
SPLERR SplSetDriver (PCSZ pszComputerName, PCSZ pszDriverName,
    PCSZ pszPrinterName, ULONG ulLevel, PVOID pBuf, ULONG cbBuf,
    ULONG ulParmNum);
SPLERR SplSetJob (PSZ pszComputerName, PSZ pszQueueName, ULONG ulJob,
    ULONG ulLevel, PVOID pBuf, ULONG cbBuf, ULONG ulParmNum);
SPLERR SplSetPort (PCSZ pszComputerName, PCSZ pszPortName, ULONG ulLevel,
    PVOID pBuf, ULONG cbBuf, ULONG ulParmNum);
SPLERR SplSetQueue (PSZ pszComputerName, PSZ pszQueueName, ULONG ulLevel,
    PVOID pBuf, ULONG cbBuf, ULONG ulParmNum);

ULONG SplMessageBox (PSZ pszLogAddr, ULONG fErrInfo, ULONG fErrData,
    PSZ pszText, PSZ pszCaption, ULONG idWindow, ULONG fStyle);
BOOL SplQmAbort (HSPL hspl);
BOOL SplQmAbortDoc (HSPL hspl);
BOOL SplQmClose (HSPL hspl);
BOOL SplQmEndDoc (HSPL hspl);
ULONG SplQmGetJobID (HSPL hspl, ULONG ulLevel, PVOID pBuf, ULONG cbBuf,
    PULONG pcbNeeded);
BOOL SplQmNewPage (HSPL hspl, ULONG ulPageNumber);
HSPL SplQmOpen (PSZ pszToken, LONG lCount, PQMOPENDATA pqmdopData);
BOOL SplQmStartDoc (HSPL hspl, PSZ pszDocName);
BOOL SplQmWrite (HSPL hspl, LONG lCount, PVOID pData);

#if defined (INCL_SPLDOSPRINT)

#define CNLEN				15
#define DTLEN				9
#define PDLEN				8
#define QNLEN				12
#define UNLEN				20

#define DRIV_DEVICENAME_SIZE		31
#define DRIV_NAME_SIZE			8
#define FORMNAME_SIZE			31
#define MAXCOMMENTSZ			48
#define PRINTERNAME_SIZE		32
#define QP_DATATYPE_SIZE		15

#define PRD_STATUS_MASK			0x0003
#define PRD_DEVSTATUS			0x0ffc

#define PRD_ACTIVE			0
#define PRD_PAUSED			1

#define PRD_DELETE			0
#define PRD_PAUSE			1
#define PRD_CONT			2
#define PRD_RESTART			3

#define PRD_LOGADDR_PARMNUM		3
#define PRD_COMMENT_PARMNUM		7
#define PRD_DRIVERS_PARMNUM		8
#define PRD_TIMEOUT_PARMNUM		10

#define PRJ_NOTIFYNAME_PARMNUM		3
#define PRJ_DATATYPE_PARMNUM		4
#define PRJ_PARMS_PARMNUM		5
#define PRJ_POSITION_PARMNUM		6
#define PRJ_JOBFILEINUSE_PARMNUM	7
#define PRJ_COMMENT_PARMNUM		11
#define PRJ_DOCUMENT_PARMNUM		12
#define PRJ_STATUSCOMMENT_PARMNUM	13
#define PRJ_PRIORITY_PARMNUM		14
#define PRJ_PROCPARMS_PARMNUM		16
#define PRJ_DRIVERDATA_PARMNUM		18
#define PRJ_SPOOLFILENAME_PARMNUM	19
#define PRJ_PAGESSPOOLED_PARMNUM	20
#define PRJ_PAGESSENT_PARMNUM		21
#define PRJ_PAGESPRINTED_PARMNUM	22
#define PRJ_TIMEPRINTED_PARMNUM		23
#define PRJ_EXTENDSTATUS_PARMNUM	24
#define PRJ_STARTPAGE_PARMNUM		25
#define PRJ_ENDPAGE_PARMNUM		26
#define PRJ_MAXPARMNUM			26

#define PRJ_QSTATUS			0x0003
#define PRJ_DEVSTATUS			0x0ffc

#define PRJ_COMPLETE			0x0004
#define PRJ_INTERV			0x0008
#define PRJ_ERROR			0x0010
#define PRJ_DESTOFFLINE			0x0020
#define PRJ_DESTPAUSED			0x0040
#define PRJ_NOTIFY			0x0080
#define PRJ_DESTNOPAPER			0x0100
#define PRJ_DESTFORMCHG			0x0200
#define PRJ_DESTCRTCHG			0x0400
#define PRJ_DESTPENCHG			0x0800
#define PRJ_JOBFILEINUSE		0x4000
#define PRJ_DELETED			0x8000

#define PRJ_QS_QUEUED			0
#define PRJ_QS_PAUSED			1
#define PRJ_QS_SPOOLING			2
#define PRJ_QS_PRINTING			3

#define PRJ_MAX_PRIORITY		99
#define PRJ_MIN_PRIORITY		1
#define PRJ_NO_PRIORITY			0

#define PRJ4_INPRINTER			0x0001
#define PRJ4_STACKED			0x0002
#define PRJ4_HELDINPRINTER		0x0004
#define PRJ4_JOBSTARTED			0x0008

#define PRPO_PORT_DRIVER		1
#define PRPO_PROTOCOL_CNV		2
#define PRPO_MODE			3
#define PRPO_PRIORITY			4

#define PRPORT_AUTODETECT		1
#define PRPORT_DISABLE_BIDI		2
#define PRPORT_ENABLE_BIDI		3

#define PRQ_PRIORITY_PARMNUM		2
#define PRQ_STARTTIME_PARMNUM		3
#define PRQ_UNTILTIME_PARMNUM		4
#define PRQ_SEPARATOR_PARMNUM		5
#define PRQ_PROCESSOR_PARMNUM		6
#define PRQ_DESTINATIONS_PARMNUM	7
#define PRQ_PARMS_PARMNUM		8
#define PRQ_COMMENT_PARMNUM		9
#define PRQ_TYPE_PARMNUM		10
#define PRQ_PRINTERS_PARMNUM		12
#define PRQ_DRIVERNAME_PARMNUM		13
#define PRQ_DRIVERDATA_PARMNUM		14
#define PRQ_REMOTE_COMPUTER_PARMNUM	15
#define PRQ_REMOTE_QUEUE_PARMNUM	16
#define PRQ_MAXPARMNUM			16

#define PRQ_MAX_PRIORITY		1
#define PRQ_DEF_PRIORITY		5
#define PRQ_MIN_PRIORITY		9
#define PRQ_NO_PRIORITY			0

#define PRQ_STATUS_MASK			3
#define PRQ_ACTIVE			0
#define PRQ_PAUSED			1
#define PRQ_ERROR			2
#define PRQ_PENDING			3

#define PRQ3_PAUSED			0x0001
#define PRQ3_PENDING			0x0002

#define PRQ3_TYPE_RAW			0x0001
#define PRQ3_TYPE_BYPASS		0x0002
#define PRQ3_TYPE_APPDEFAULT		0x0004

#define SPL_PR_QUEUE			0x0001
#define SPL_PR_DIRECT_DEVICE		0x0002
#define SPL_PR_QUEUED_DEVICE		0x0004
#define SPL_PR_LOCAL_ONLY		0x0100

typedef struct _DRIVPROPS
{
  PSZ	pszKeyName;
  ULONG cbBuf;
  PVOID pBuf;
} DRIVPROPS;
typedef DRIVPROPS *PDRIVPROPS;

typedef struct _PRINTERINFO
{
  ULONG flType;
  PSZ	pszComputerName;
  PSZ	pszPrintDestinationName;
  PSZ	pszDescription;
  PSZ	pszLocalName;
} PRINTERINFO;
typedef PRINTERINFO *PPRINTERINFO;

typedef struct _PRJINFO
{
  USHORT uJobId;
  CHAR	 szUserName[UNLEN+1];
  CHAR	 pad_1;
  CHAR	 szNotifyName[CNLEN+1];
  CHAR	 szDataType[DTLEN+1];
  PSZ	 pszParms;
  USHORT uPosition;
  USHORT fsStatus;
  PSZ	 pszStatus;
  ULONG	 ulSubmitted;
  ULONG	 ulSize;
  PSZ	 pszComment;
} PRJINFO;
typedef PRJINFO *PPRJINFO;

typedef struct _PRJINFO2
{
  USHORT uJobId;
  USHORT uPriority;
  PSZ	 pszUserName;
  USHORT uPosition;
  USHORT fsStatus;
  ULONG	 ulSubmitted;
  ULONG	 ulSize;
  PSZ	 pszComment;
  PSZ	 pszDocument;
} PRJINFO2;
typedef PRJINFO2 *PPRJINFO2;

typedef struct _PRJINFO3
{
  USHORT    uJobId;
  USHORT    uPriority;
  PSZ	    pszUserName;
  USHORT    uPosition;
  USHORT    fsStatus;
  ULONG	    ulSubmitted;
  ULONG	    ulSize;
  PSZ	    pszComment;
  PSZ	    pszDocument;
  PSZ	    pszNotifyName;
  PSZ	    pszDataType;
  PSZ	    pszParms;
  PSZ	    pszStatus;
  PSZ	    pszQueue;
  PSZ	    pszQProcName;
  PSZ	    pszQProcParms;
  PSZ	    pszDriverName;
  PDRIVDATA pDriverData;
  PSZ	    pszPrinterName;
} PRJINFO3;
typedef PRJINFO3 *PPRJINFO3;

typedef struct _PRJINFO4
{
  USHORT uJobId;
  USHORT uPriority;
  PSZ	 pszUserName;
  USHORT uPosition;
  USHORT fsStatus;
  ULONG	 ulSubmitted;
  ULONG	 ulSize;
  PSZ	 pszComment;
  PSZ	 pszDocument;
  PSZ	 pszSpoolFileName;
  PSZ	 pszPortName;
  PSZ	 pszStatus;
  ULONG	 ulPagesSpooled;
  ULONG	 ulPagesSent;
  ULONG	 ulPagesPrinted;
  ULONG	 ulTimePrinted;
  ULONG	 ulExtendJobStatus;
  ULONG	 ulStartPage;
  ULONG	 ulEndPage;
} PRJINFO4;
typedef PRJINFO4 *PPRJINFO4;

typedef struct _PRDINFO
{
  CHAR	  szName[PDLEN+1];
  CHAR	  szUserName[UNLEN+1];
  USHORT  uJobId;
  USHORT  fsStatus;
  PSZ	  pszStatus;
  USHORT  time;
} PRDINFO;
typedef PRDINFO *PPRDINFO;

typedef struct _PRDINFO3
{
  PSZ	 pszPrinterName;
  PSZ	 pszUserName;
  PSZ	 pszLogAddr;
  USHORT uJobId;
  USHORT fsStatus;
  PSZ	 pszStatus;
  PSZ	 pszComment;
  PSZ	 pszDrivers;
  USHORT time;
  USHORT usTimeOut;
} PRDINFO3;
typedef PRDINFO3 *PPRDINFO3;

typedef struct _PRQINFO
{
  CHAR	 szName[QNLEN+1];
  CHAR	 pad_1;
  USHORT uPriority;
  USHORT uStartTime;
  USHORT uUntilTime;
  PSZ	 pszSepFile;
  PSZ	 pszPrProc;
  PSZ	 pszDestinations;
  PSZ	 pszParms;
  PSZ	 pszComment;
  USHORT fsStatus;
  USHORT cJobs;
} PRQINFO;
typedef PRQINFO *PPRQINFO;

typedef struct _PRQINFO3
{
  PSZ	    pszName;
  USHORT    uPriority;
  USHORT    uStartTime;
  USHORT    uUntilTime;
  USHORT    fsType;
  PSZ	    pszSepFile;
  PSZ	    pszPrProc;
  PSZ	    pszParms;
  PSZ	    pszComment;
  USHORT    fsStatus;
  USHORT    cJobs;
  PSZ	    pszPrinters;
  PSZ	    pszDriverName;
  PDRIVDATA pDriverData;
} PRQINFO3;
typedef PRQINFO3 *PPRQINFO3;

typedef struct _PRQINFO6
{
  PSZ	    pszName;
  USHORT    uPriority;
  USHORT    uStartTime;
  USHORT    uUntilTime;
  USHORT    fsType;
  PSZ	    pszSepFile;
  PSZ	    pszPrProc;
  PSZ	    pszParms;
  PSZ	    pszComment;
  USHORT    fsStatus;
  USHORT    cJobs;
  PSZ	    pszPrinters;
  PSZ	    pszDriverName;
  PDRIVDATA pDriverData;
  PSZ	    pszRemoteComputerName;
  PSZ	    pszRemoteQueueName;
} PRQINFO6;
typedef PRQINFO6 *PPRQINFO6;

typedef struct _PRIDINFO
{
  USHORT uJobId;
  CHAR	 szComputerName[CNLEN+1];
  CHAR	 szQueueName[QNLEN+1];
  CHAR	 pad_1;
} PRIDINFO;
typedef PRIDINFO *PPRIDINFO;

typedef struct _PRDRIVINFO
{
  CHAR szDrivName[DRIV_NAME_SIZE+1+DRIV_DEVICENAME_SIZE+1];
} PRDRIVINFO;
typedef PRDRIVINFO *PPRDRIVINFO;

typedef struct _PRDRIVINFO2
{
  PSZ	 pszPrinterName;
  PSZ	 pszDriverName;
  USHORT usFlags;
  USHORT cDriverProps;
} PRDRIVINFO2;
typedef PRDRIVINFO2 *PPRDRIVINFO2;

typedef struct _PRQPROCINFO
{
  CHAR szQProcName[QNLEN+1];
} PRQPROCINFO;
typedef PRQPROCINFO *PPRQPROCINFO;

typedef struct _PRPORTINFO
{
  CHAR szPortName[PDLEN+1];
} PRPORTINFO;
typedef PRPORTINFO *PPRPORTINFO;

typedef struct _PRPORTINFO1
{
  PSZ pszPortName;
  PSZ pszPortDriverName;
  PSZ pszPortDriverPathName;
} PRPORTINFO1;
typedef PRPORTINFO1 *PPRPORTINFO1;

typedef struct _PRPORTINFO2
{
  PSZ	pszPortName;
  PSZ	pszPortDriver;
  PSZ	pszProtocolConverter;
  ULONG ulReserved;
  ULONG ulMode;
  ULONG ulPriority;
} PRPORTINFO2;
typedef PRPORTINFO2 *PPRPORTINFO2;

typedef struct _QMJOBINFO
{
  ULONG ulJobID;
  PSZ	pszComputerName;
  PSZ	pszQueueName;
} QMJOBINFO;
typedef QMJOBINFO *PQMJOBINFO;

#endif /* INCL_SPLDOSPRINT */
#endif /* INCL_SPL */

/* -------------------------- HELP MANAGER -------------------------------- */

#if defined (INCL_WINHELP)

#define CMIC_HIDE_PANEL_ID		0x0000
#define CMIC_SHOW_PANEL_ID		0x0001
#define CMIC_TOGGLE_PANEL_ID		0x0002

#define CTRL_PREVIOUS_ID		((USHORT)0x0001)
#define CTRL_SEARCH_ID			((USHORT)0x0002)
#define CTRL_PRINT_ID			((USHORT)0x0003)
#define CTRL_INDEX_ID			((USHORT)0x0004)
#define CTRL_CONTENTS_ID		((USHORT)0x0005)
#define CTRL_BACK_ID			((USHORT)0x0006)
#define CTRL_FORWARD_ID			((USHORT)0x0007)
#define CTRL_TUTORIAL_ID		((USHORT)0x00ff)
#define CTRL_USER_ID_BASE		((USHORT)0x0101)

#define HM_MSG_BASE			0x0220
#define HM_DISMISS_WINDOW		0x0221
#define HM_DISPLAY_HELP			0x0222
#define HM_EXT_HELP			0x0223
#define HM_GENERAL_HELP			0x0223 /*!*/
#define HM_SET_ACTIVE_WINDOW		0x0224
#define HM_LOAD_HELP_TABLE		0x0225
#define HM_CREATE_HELP_TABLE		0x0226
#define HM_SET_HELP_WINDOW_TITLE	0x0227
#define HM_SET_SHOW_PANEL_ID		0x0228
#define HM_REPLACE_HELP_FOR_HELP	0x0229
#define HM_REPLACE_USING_HELP		0x0229 /*!*/
#define HM_HELP_INDEX			0x022a
#define HM_HELP_CONTENTS		0x022b
#define HM_KEYS_HELP			0x022c
#define HM_SET_HELP_LIBRARY_NAME	0x022d
#define HM_ERROR			0x022e
#define HM_HELPSUBITEM_NOT_FOUND	0x022f
#define HM_QUERY_KEYS_HELP		0x0230
#define HM_TUTORIAL			0x0231
#define HM_EXT_HELP_UNDEFINED		0x0232
#define HM_GENERAL_HELP_UNDEFINED	0x0232 /*!*/
#define HM_ACTIONBAR_COMMAND		0x0233
#define HM_INFORM			0x0234
#define HM_SET_OBJCOM_WINDOW		0x0238
#define HM_UPDATE_OBJCOM_WINDOW_CHAIN	0x0239
#define HM_QUERY_DDF_DATA		0x023a
#define HM_INVALIDATE_DDF_DATA		0x023b
#define HM_QUERY			0x023c
#define HM_SET_COVERPAGE_SIZE		0x023d
#define HM_NOTIFY			0x0242
#define HM_SET_USERDATA			0x0243
#define HM_CONTROL			0x0244

#define HM_RESOURCEID			0
#define HM_PANELNAME			1

#define HMERR_NO_FRAME_WND_IN_CHAIN	0x1001
#define HMERR_INVALID_ASSOC_APP_WND	0x1002
#define HMERR_INVALID_ASSOC_HELP_INST	0x1003
#define HMERR_INVALID_DESTROY_HELP_INST 0x1004
#define HMERR_NO_HELP_INST_IN_CHAIN	0x1005
#define HMERR_INVALID_HELP_INSTANCE_HDL 0x1006
#define HMERR_INVALID_QUERY_APP_WND	0x1007
#define HMERR_HELP_INST_CALLED_INVALID	0x1008
#define HMERR_HELPTABLE_UNDEFINE	0x1009
#define HMERR_HELP_INSTANCE_UNDEFINE	0x100a
#define HMERR_HELPITEM_NOT_FOUND	0x100b
#define HMERR_INVALID_HELPSUBITEM_SIZE	0x100c
#define HMERR_HELPSUBITEM_NOT_FOUND	0x100d

#define HMERR_INDEX_NOT_FOUND		0x2001
#define HMERR_CONTENT_NOT_FOUND		0x2002
#define HMERR_OPEN_LIB_FILE		0x2003
#define HMERR_READ_LIB_FILE		0x2004
#define HMERR_CLOSE_LIB_FILE		0x2005
#define HMERR_INVALID_LIB_FILE		0x2006
#define HMERR_NO_MEMORY			0x2007
#define HMERR_ALLOCATE_SEGMENT		0x2008
#define HMERR_FREE_MEMORY		0x2009
#define HMERR_PANEL_NOT_FOUND		0x2010
#define HMERR_DATABASE_NOT_OPEN		0x2011
#define HMERR_LOAD_DLL			0x2013

#define HMPANELTYPE_NUMBER		0
#define HMPANELTYPE_NAME		1

#define HMQVP_NUMBER			0x0001
#define HMQVP_NAME			0x0002
#define HMQVP_GROUP			0x0003

#define HMQW_COVERPAGE			0x0001
#define HMQW_INDEX			0x0002
#define HMQW_TOC			0x0003
#define HMQW_SEARCH			0x0004
#define HMQW_VIEWPAGES			0x0005
#define HMQW_LIBRARY			0x0006
#define HMQW_VIEWPORT			0x0007
#define HMQW_OBJCOM_WINDOW		0x0008
#define HMQW_INSTANCE			0x0009
#define HMQW_ACTIVEVIEWPORT		0x000a
#define CONTROL_SELECTED		0x000b

#define HMQW_GROUP_VIEWPORT		0x00f1
#define HMQW_RES_VIEWPORT		0x00f2
#define USERDATA			0x00f3

#define HWND_PARENT			(HWND)NULL

#define OPEN_COVERPAGE			0x0001
#define OPEN_PAGE			0x0002
#define SWAP_PAGE			0x0003
#define OPEN_TOC			0x0004
#define OPEN_INDEX			0x0005
#define OPEN_HISTORY			0x0006
#define OPEN_SEARCH_HIT_LIST		0x0007
#define OPEN_LIBRARY			0x0008


typedef USHORT HELPSUBTABLE;
typedef HELPSUBTABLE *PHELPSUBTABLE;


typedef struct _ACVP
{
  ULONG cb;
  HAB	hAB;
  HMQ	hmq;
  ULONG ObjectID;
  HWND	hWndParent;
  HWND	hWndOwner;
  HWND	hWndACVP;
} ACVP;
typedef ACVP *PACVP;

typedef struct _HELPTABLE
{
  USHORT	idAppWindow;
  PHELPSUBTABLE phstHelpSubTable;
  USHORT	idExtPanel;
} HELPTABLE;
typedef HELPTABLE *PHELPTABLE;

typedef struct _HELPINIT
{
  ULONG	     cb;
  ULONG	     ulReturnCode;
  PSZ	     pszTutorialName;
  PHELPTABLE phtHelpTable;
  HMODULE    hmodHelpTableModule;
  HMODULE    hmodAccelActionBarModule;
  ULONG	     idAccelTable;
  ULONG	     idActionBar;
  PSZ	     pszHelpWindowTitle;
  ULONG	     fShowPanelId;
  PSZ	     pszHelpLibraryName;
} HELPINIT;
typedef HELPINIT *PHELPINIT;

BOOL WinAssociateHelpInstance (HWND hwndHelpInstance, HWND hwndApp);
HWND WinCreateHelpInstance (HAB hab, PHELPINIT phinitHMInitStructure);
BOOL WinCreateHelpTable (HWND hwndHelpInstance,
    __const__ HELPTABLE *phtHelpTable);
BOOL WinDestroyHelpInstance (HWND hwndHelpInstance);
BOOL WinLoadHelpTable (HWND hwndHelpInstance, ULONG idHelpTable,
    HMODULE Module);
HWND WinQueryHelpInstance (HWND hwndApp);

#endif /* INCL_WINHELP */


#if defined (INCL_DDF)

#define ART_RUNIN			0x0010
#define ART_LEFT			0x0001
#define ART_RIGHT			0x0002
#define ART_CENTER			0x0004

#define CLR_UNCHANGED			(-6)

#define HMBT_NONE			1
#define HMBT_ALL			2
#define HMBT_FIT			3

#define HMERR_DDF_MEMORY		0x3001
#define HMERR_DDF_ALIGN_TYPE		0x3002
#define HMERR_DDF_BACKCOLOR		0x3003
#define HMERR_DDF_FORECOLOR		0x3004
#define HMERR_DDF_FONTSTYLE		0x3005
#define HMERR_DDF_REFTYPE		0x3006
#define HMERR_DDF_LIST_UNCLOSED		0x3007
#define HMERR_DDF_LIST_UNINITIALIZED	0x3008
#define HMERR_DDF_LIST_BREAKTYPE	0x3009
#define HMERR_DDF_LIST_SPACING		0x300A
#define HMERR_DDF_HINSTANCE		0x300B
#define HMERR_DDF_EXCEED_MAX_LENGTH	0x300C
#define HMERR_DDF_EXCEED_MAX_INC	0x300D
#define HMERR_DDF_INVALID_DDF		0x300E
#define HMERR_DDF_FORMAT_TYPE		0x300F
#define HMERR_DDF_INVALID_PARM		0x3010
#define HMERR_DDF_INVALID_FONT		0x3011
#define HMERR_DDF_SEVERE		0x3012

#define HMLS_SINGLELINE			1
#define HMLS_DOUBLELINE			2

#define REFERENCE_BY_ID			0
#define REFERENCE_BY_RES		1

typedef VOID *HDDF;

BOOL DdfBeginList (HDDF hddf, ULONG ulWidthDT, ULONG fBreakType,
    ULONG fSpacing);
BOOL DdfBitmap (HDDF hddf, HBITMAP hbm, ULONG fAlign);
BOOL DdfEndList (HDDF hddf);
BOOL DdfHyperText (HDDF hddf, PCSZ pszText, PCSZ pszReference,
    ULONG fReferenceType);
BOOL DdfInform (HDDF hddf, PCSZ pszText, ULONG resInformNumber);
HDDF DdfInitialize (HWND hwndHelpInstance, ULONG cbBuffer, ULONG ulIncrement);
BOOL DdfListItem (HDDF hddf, PCSZ pszTerm, PCSZ pszDescription);
BOOL DdfMetafile (HDDF hddf, HMF hmf, __const__ RECTL *prclRect);
BOOL DdfPara (HDDF hddf);
BOOL DdfSetColor (HDDF hddf, COLOR fBackColor, COLOR fForColor);
BOOL DdfSetFont (HDDF hddf, PCSZ pszFaceName, ULONG ulWidth, ULONG ulHeight);
BOOL DdfSetFontStyle (HDDF hddf, ULONG fFontStyle);
BOOL DdfSetFormat (HDDF hddf, ULONG fFormatType);
BOOL DdfSetTextAlign (HDDF hddf, ULONG fAlign);
BOOL DdfText (HDDF hddf, PCSZ pszText);

#endif /* INCL_DDF */

/* ---------------------- Advanced Video ---------------------------------- */

#if defined (INCL_FONTFILEFORMAT)

#define FONTDEFFONT1			0x0047
#define FONTDEFFONT2			0x0042
#define FONTDEFFONT3			0x0042
#define FONTDEFCHAR1			0x0081
#define FONTDEFCHAR2			0x0081
#define FONTDEFCHAR3			0x00b8
#define FONTDEFDEVFONT			0x2000
#define FONTDEFFOCA32			0x4000
#define SPACE_UNDEF			0x8000

#define FONT_SIGNATURE			0xfffffffe
#define FONT_METRICS			0x00000001
#define FONT_DEFINITION			0x00000002
#define FONT_KERNPAIRS			0x00000003
#define FONT_ADDITIONALMETRICS		0x00000004
#define FONT_ENDRECORD			0xffffffff

#define QUERY_PUBLIC_FONTS		0x0001
#define QUERY_PRIVATE_FONTS		0x0002

#define CDEF_GENERIC			0x0001
#define CDEF_BOLD			0x0002
#define CDEF_ITALIC			0x0004
#define CDEF_UNDERSCORE			0x0008
#define CDEF_STRIKEOUT			0x0010
#define CDEF_OUTLINE			0x0020

typedef struct _FOCAMETRICS
{
  ULONG	 ulIdentity;
  ULONG	 ulSize;
  CHAR	 szFamilyname[32];
  CHAR	 szFacename[32];
  SHORT	 usRegistryId;
  SHORT	 usCodePage;
  SHORT	 yEmHeight;
  SHORT	 yXHeight;
  SHORT	 yMaxAscender;
  SHORT	 yMaxDescender;
  SHORT	 yLowerCaseAscent;
  SHORT	 yLowerCaseDescent;
  SHORT	 yInternalLeading;
  SHORT	 yExternalLeading;
  SHORT	 xAveCharWidth;
  SHORT	 xMaxCharInc;
  SHORT	 xEmInc;
  SHORT	 yMaxBaselineExt;
  SHORT	 sCharSlope;
  SHORT	 sInlineDir;
  SHORT	 sCharRot;
  USHORT usWeightClass;
  USHORT usWidthClass;
  SHORT	 xDeviceRes;
  SHORT	 yDeviceRes;
  SHORT	 usFirstChar;
  SHORT	 usLastChar;
  SHORT	 usDefaultChar;
  SHORT	 usBreakChar;
  SHORT	 usNominalPointSize;
  SHORT	 usMinimumPointSize;
  SHORT	 usMaximumPointSize;
  SHORT	 fsTypeFlags;
  SHORT	 fsDefn;
  SHORT	 fsSelectionFlags;
  SHORT	 fsCapabilities;
  SHORT	 ySubscriptXSize;
  SHORT	 ySubscriptYSize;
  SHORT	 ySubscriptXOffset;
  SHORT	 ySubscriptYOffset;
  SHORT	 ySuperscriptXSize;
  SHORT	 ySuperscriptYSize;
  SHORT	 ySuperscriptXOffset;
  SHORT	 ySuperscriptYOffset;
  SHORT	 yUnderscoreSize;
  SHORT	 yUnderscorePosition;
  SHORT	 yStrikeoutSize;
  SHORT	 yStrikeoutPosition;
  SHORT	 usKerningPairs;
  SHORT	 sFamilyClass;
  PSZ	 pszDeviceNameOffset;
} FOCAMETRICS;
typedef FOCAMETRICS *PFOCAMETRICS;

typedef struct _FONTFILEMETRICS
{
  ULONG	 ulIdentity;
  ULONG	 ulSize;
  CHAR	 szFamilyname[32];
  CHAR	 szFacename[32];
  SHORT	 usRegistryId;
  SHORT	 usCodePage;
  SHORT	 yEmHeight;
  SHORT	 yXHeight;
  SHORT	 yMaxAscender;
  SHORT	 yMaxDescender;
  SHORT	 yLowerCaseAscent;
  SHORT	 yLowerCaseDescent;
  SHORT	 yInternalLeading;
  SHORT	 yExternalLeading;
  SHORT	 xAveCharWidth;
  SHORT	 xMaxCharInc;
  SHORT	 xEmInc;
  SHORT	 yMaxBaselineExt;
  SHORT	 sCharSlope;
  SHORT	 sInlineDir;
  SHORT	 sCharRot;
  USHORT usWeightClass;
  USHORT usWidthClass;
  SHORT	 xDeviceRes;
  SHORT	 yDeviceRes;
  SHORT	 usFirstChar;
  SHORT	 usLastChar;
  SHORT	 usDefaultChar;
  SHORT	 usBreakChar;
  SHORT	 usNominalPointSize;
  SHORT	 usMinimumPointSize;
  SHORT	 usMaximumPointSize;
  SHORT	 fsTypeFlags;
  SHORT	 fsDefn;
  SHORT	 fsSelectionFlags;
  SHORT	 fsCapabilities;
  SHORT	 ySubscriptXSize;
  SHORT	 ySubscriptYSize;
  SHORT	 ySubscriptXOffset;
  SHORT	 ySubscriptYOffset;
  SHORT	 ySuperscriptXSize;
  SHORT	 ySuperscriptYSize;
  SHORT	 ySuperscriptXOffset;
  SHORT	 ySuperscriptYOffset;
  SHORT	 yUnderscoreSize;
  SHORT	 yUnderscorePosition;
  SHORT	 yStrikeoutSize;
  SHORT	 yStrikeoutPosition;
  SHORT	 usKerningPairs;
  SHORT	 sFamilyClass;
  ULONG	 ulReserved;
  PANOSE panose;
} FONTFILEMETRICS;
typedef FONTFILEMETRICS *PFONTFILEMETRICS;

typedef struct _FONTDEFINITIONHEADER
{
  ULONG ulIdentity;
  ULONG ulSize;
  SHORT fsFontdef;
  SHORT fsChardef;
  SHORT usCellSize;
  SHORT xCellWidth;
  SHORT yCellHeight;
  SHORT xCellIncrement;
  SHORT xCellA;
  SHORT xCellB;
  SHORT xCellC;
  SHORT pCellBaseOffset;
} FONTDEFINITIONHEADER;
typedef FONTDEFINITIONHEADER *PFONTDEFINITIONHEADER;

typedef struct _FONTSIGNATURE
{
  ULONG ulIdentity;
  ULONG ulSize;
  CHAR	achSignature[12];
} FONTSIGNATURE;
typedef FONTSIGNATURE *PFONTSIGNATURE;

typedef struct _ADDITIONALMETRICS
{
  ULONG	 ulIdentity;
  ULONG	 ulSize;
  PANOSE panose;
} ADDITIONALMETRICS;
typedef ADDITIONALMETRICS *PADDITIONALMETRICS;

typedef struct _FOCAFONT
{
  FONTSIGNATURE	       fsSignature;
  FOCAMETRICS	       fmMetrics;
  FONTDEFINITIONHEADER fdDefinitions;
} FOCAFONT;
typedef FOCAFONT *PFOCAFONT;

typedef FOCAFONT FOCAFONT32;
typedef FOCAFONT32 *PFOCAFONT32;

#endif /* INCL_FONTFILEFORMAT */

/* ---------------------- Advanced Video ---------------------------------- */

#if defined (INCL_AVIO)

#define FORMAT_CGA			0x0001
#define FORMAT_4BYTE			0x0003

#define VQF_PUBLIC			0x0001
#define VQF_PRIVATE			0x0002

typedef USHORT HVPS;
typedef HVPS *PHVPS;

USHORT VioAssociate (HDC hdc, HVPS hvps);
USHORT VioCreateLogFont (PFATTRS pfatattrs, LONG llcid, PSTR8 pName,
    HVPS hvps);
USHORT VioCreatePS (PHVPS phvps, SHORT sDepth, SHORT sWidth, SHORT sFormat,
    SHORT sAttrs, HVPS hvpsReserved);
USHORT VioDeleteSetId (LONG llcid, HVPS hvps);
USHORT VioDestroyPS (HVPS hvps);
USHORT VioGetDeviceCellSize (PSHORT psHeight, PSHORT psWidth, HVPS hvps);
USHORT VioGetOrg (PSHORT psRow, PSHORT psColumn, HVPS hvps);
USHORT VioQueryFonts (PLONG plRemfonts, PFONTMETRICS afmMetrics,
    LONG lMetricsLength, PLONG plFonts, PSZ pszFacename, ULONG flOptions,
    HVPS hvps);
USHORT VioQuerySetIds (PLONG allcids, PSTR8 pNames, PLONG alTypes, LONG lcount,
    HVPS hvps);
USHORT VioSetDeviceCellSize (SHORT sHeight, SHORT sWidth, HVPS hvps);
USHORT VioSetOrg (SHORT sRow, SHORT sColumn, HVPS hvps);
USHORT VioShowPS (SHORT sDepth, SHORT sWidth, SHORT soffCell, HVPS hvps);

MRESULT WinDefAVioWindowProc (HWND hwnd, USHORT msg, ULONG mp1, ULONG mp2);

#endif /* INCL_AVIO */

/* --------------------------- MONITORS ----------------------------------- */

#if defined (INCL_DOSMONITORS)

#define MONITOR_DEFAULT		0x0000
#define MONITOR_BEGIN		0x0001
#define MONITOR_END		0x0002

typedef SHANDLE HMONITOR;
typedef HMONITOR *PHMONITOR;

typedef struct _MONIN
{
  USHORT cb;
  BYTE	 abReserved[18];
  BYTE	 abBuffer[108];
} MONIN;
typedef MONIN *PMONIN;

typedef struct _MONOUT
{
  USHORT cb;
  UCHAR	 buffer[18];
  BYTE	 abBuf[108];
} MONOUT;
typedef MONOUT *PMONOUT;

USHORT DosMonOpen (PSZ pszDevName, PHMONITOR phmon);
USHORT DosMonClose (HMONITOR hmon);
USHORT DosMonReg (HMONITOR hmon, PBYTE pbInBuf, PBYTE pbOutBuf,
    USHORT fPosition, USHORT usIndex);
USHORT DosMonRead (PBYTE pbInBuf, USHORT fWait, PBYTE pbDataBuf,
    PUSHORT pcbData);
USHORT DosMonWrite (PBYTE pbOutBuf, PBYTE pbDataBuf, USHORT cbData);

#endif /* INCL_DOSMONITORS */

/* -------------------------- SUBSYSTEMS ---------------------------------- */

#if defined (INCL_KBD)

#define IO_WAIT				0
#define IO_NOWAIT			1

#define KBDSTF_RIGHTSHIFT		0x0001
#define KBDSTF_LEFTSHIFT		0x0002
#define KBDSTF_CONTROL			0x0004
#define KBDSTF_ALT			0x0008
#define KBDSTF_SCROLLLOCK_ON		0x0010
#define KBDSTF_NUMLOCK_ON		0x0020
#define KBDSTF_CAPSLOCK_ON		0x0040
#define KBDSTF_INSERT_ON		0x0080
#define KBDSTF_LEFTCONTROL		0x0100
#define KBDSTF_LEFTALT			0x0200
#define KBDSTF_RIGHTCONTROL		0x0400
#define KBDSTF_RIGHTALT			0x0800
#define KBDSTF_SCROLLLOCK		0x1000
#define KBDSTF_NUMLOCK			0x2000
#define KBDSTF_CAPSLOCK			0x4000
#define KBDSTF_SYSREQ			0x8000

#define KBDTRF_SHIFT_KEY_IN		0x01
#define KBDTRF_EXTENDED_CODE		0x02
#define KBDTRF_CONVERSION_REQUEST	0x20
#define KBDTRF_FINAL_CHAR_IN		0x40
#define KBDTRF_INTERIM_CHAR_IN		0x80

#define KEYBOARD_ECHO_ON		0x0001
#define KEYBOARD_ECHO_OFF		0x0002
#define KEYBOARD_BINARY_MODE		0x0004
#define KEYBOARD_ASCII_MODE		0x0008
#define KEYBOARD_MODIFY_STATE		0x0010
#define KEYBOARD_MODIFY_INTERIM		0x0020
#define KEYBOARD_MODIFY_TURNAROUND	0x0040
#define KEYBOARD_2B_TURNAROUND		0x0080
#define KEYBOARD_SHIFT_REPORT		0x0100

#define KR_KBDCHARIN			0x00000001
#define KR_KBDPEEK			0x00000002
#define KR_KBDFLUSHBUFFER		0x00000004
#define KR_KBDGETSTATUS			0x00000008
#define KR_KBDSETSTATUS			0x00000010
#define KR_KBDSTRINGIN			0x00000020
#define KR_KBDOPEN			0x00000040
#define KR_KBDCLOSE			0x00000080
#define KR_KBDGETFOCUS			0x00000100
#define KR_KBDFREEFOCUS			0x00000200
#define KR_KBDGETCP			0x00000400
#define KR_KBDSETCP			0x00000800
#define KR_KBDXLATE			0x00001000
#define KR_KBDSETCUSTXT			0x00002000

typedef USHORT HKBD;
typedef HKBD *PHKBD;

typedef struct _KBDKEYINFO
{
  UCHAR	 chChar;
  UCHAR	 chScan;
  UCHAR	 fbStatus;
  UCHAR	 bNlsShift;
  USHORT fsState;
  ULONG	 time;
} KBDKEYINFO;
typedef KBDKEYINFO *PKBDKEYINFO;

typedef struct _KBDINFO
{
  USHORT cb;
  USHORT fsMask;
  USHORT chTurnAround;
  USHORT fsInterim;
  USHORT fsState;
} KBDINFO;
typedef KBDINFO *PKBDINFO;

typedef struct _KBDHWID
{
  USHORT cb;
  USHORT idKbd;
  USHORT usReserved1;
  USHORT usReserved2;
} KBDHWID;
typedef KBDHWID *PKBDHWID;

typedef struct _KBDTRANS
{
  UCHAR	 chChar;
  UCHAR	 chScan;
  UCHAR	 fbStatus;
  UCHAR	 bNlsShift;
  USHORT fsState;
  ULONG	 time;
  USHORT fsDD;
  USHORT fsXlate;
  USHORT fsShift;
  USHORT sZero;
} KBDTRANS;
typedef KBDTRANS *PKBDTRANS;

typedef struct _STRINGINBUF
{
  USHORT cb;
  USHORT cchIn;
} STRINGINBUF;
typedef STRINGINBUF *PSTRINGINBUF;

USHORT KbdCharIn (PKBDKEYINFO pkbci, USHORT fWait, HKBD hkbd);
USHORT KbdClose (HKBD hkbd);
USHORT KbdDeRegister (VOID);
USHORT KbdFlushBuffer (HKBD hkbd);
USHORT KbdFreeFocus (HKBD hkbd);
USHORT KbdGetCp (ULONG ulReserved, PUSHORT pidCP, HKBD hkbd);
USHORT KbdGetFocus (USHORT fWait, HKBD hkbd);
USHORT KbdGetHWID (PKBDHWID pkbdhwid, HKBD hkbd);
USHORT KbdGetStatus (PKBDINFO pkbdinfo, HKBD hkbd);
USHORT KbdOpen (PHKBD phkbd);
USHORT KbdPeek (PKBDKEYINFO pkbci, HKBD hkbd);
USHORT KbdRegister (PCSZ pszModName, PCSZ pszEntryName, ULONG ulFunMask);
USHORT KbdSetCp (USHORT usReserved, USHORT idCP, HKBD hkbd);
USHORT KbdSetCustXt (PUSHORT pusCodePage, HKBD hkbd);
USHORT KbdSetFgnd (VOID);
USHORT KbdSetHWID (PKBDHWID pkbdhwid, HKBD hkbd);
USHORT KbdSetStatus (PKBDINFO pkbdinfo, HKBD hkbd);
USHORT KbdStringIn (PCH pch, PSTRINGINBUF pchIn, USHORT fWait, HKBD hkbd);
USHORT KbdSynch (USHORT fWait);
USHORT KbdXlate (PKBDTRANS pkbdtrans, HKBD hkbd);

#endif /* INCL_KBD */

#if defined (INCL_VIO)

#define ANSI_OFF			0
#define ANSI_ON				1

#define COLORS_2			0x01
#define COLORS_4			0x02
#define COLORS_16			0x04

#define VGMT_OTHER			0x01
#define VGMT_GRAPHICS			0x02
#define VGMT_DISABLEBURST		0x04

#define VP_NOWAIT			0x0000
#define VP_WAIT				0x0001
#define VP_OPAQUE			0x0000
#define VP_TRANSPARENT			0x0002

#define VMWR_POPUP			0
#define VMWN_POPUP			0

#define VSRWI_SAVEANDREDRAW		0
#define VSRWI_REDRAW			1

#define VSRWN_SAVE			0
#define VSRWN_REDRAW			1

#define UNDOI_GETOWNER			0
#define UNDOI_RELEASEOWNER		1

#define UNDOK_ERRORCODE			0
#define UNDOK_TERMINATE			1

#define LOCKIO_NOWAIT			0
#define LOCKIO_WAIT			1

#define LOCK_SUCCESS			0
#define LOCK_FAIL			1

#define VCC_SBCSCHAR			0
#define VCC_DBCSFULLCHAR		1
#define VCC_DBCS1STHALF			2
#define VCC_DBCS2NDHALF			3

#define VGFI_GETCURFONT			0
#define VGFI_GETROMFONT			1

#define VIO_CONFIG_CURRENT		0
#define VIO_CONFIG_PRIMARY		1
#define VIO_CONFIG_SECONDARY		2

#define DISPLAY_MONOCHROME		0
#define DISPLAY_CGA			1
#define DISPLAY_EGA			2
#define DISPLAY_VGA			3
#define DISPLAY_8514A			7
#define DISPLAY_IMAGEADAPTER		8
#define DISPLAY_XGA			9

#define MONITOR_MONOCHROME		0x0000
#define MONITOR_COLOR			0x0001
#define MONITOR_ENHANCED		0x0002
#define MONITOR_8503			0x0003
#define MONITOR_851X_COLOR		0x0004
#define MONITOR_8514			0x0009
#define MONITOR_FLATPANEL		0x000a
#define MONITOR_8507_8604		0x000b
#define MONITOR_8515			0x000c
#define MONITOR_9515			0x000f
#define MONITOR_9517			0x0011
#define MONITOR_9518			0x0012

#define VR_VIOGETCURPOS			0x00000001
#define VR_VIOGETCURTYPE		0x00000002
#define VR_VIOGETMODE			0x00000004
#define VR_VIOGETBUF			0x00000008
#define VR_VIOGETPHYSBUF		0x00000010
#define VR_VIOSETCURPOS			0x00000020
#define VR_VIOSETCURTYPE		0x00000040
#define VR_VIOSETMODE			0x00000080
#define VR_VIOSHOWBUF			0x00000100
#define VR_VIOREADCHARSTR		0x00000200
#define VR_VIOREADCELLSTR		0x00000400
#define VR_VIOWRTNCHAR			0x00000800
#define VR_VIOWRTNATTR			0x00001000
#define VR_VIOWRTNCELL			0x00002000
#define VR_VIOWRTTTY			0x00004000
#define VR_VIOWRTCHARSTR		0x00008000
#define VR_VIOWRTCHARSTRATT		0x00010000
#define VR_VIOWRTCELLSTR		0x00020000
#define VR_VIOSCROLLUP			0x00040000
#define VR_VIOSCROLLDN			0x00080000
#define VR_VIOSCROLLLF			0x00100000
#define VR_VIOSCROLLRT			0x00200000
#define VR_VIOSETANSI			0x00400000
#define VR_VIOGETANSI			0x00800000
#define VR_VIOPRTSC			0x01000000
#define VR_VIOSCRLOCK			0x02000000
#define VR_VIOSCRUNLOCK			0x04000000
#define VR_VIOSAVREDRAWWAIT		0x08000000
#define VR_VIOSAVREDRAWUNDO		0x10000000
#define VR_VIOPOPUP			0x20000000
#define VR_VIOENDPOPUP			0x40000000
#define VR_VIOPRTSCTOGGLE		0x80000000

#define VR_VIOMODEWAIT			0x00000001
#define VR_VIOMODEUNDO			0x00000002
#define VR_VIOGETFONT			0x00000004
#define VR_VIOGETCONFIG			0x00000008
#define VR_VIOSETCP			0x00000010
#define VR_VIOGETCP			0x00000020
#define VR_VIOSETFONT			0x00000040
#define VR_VIOGETSTATE			0x00000080
#define VR_VIOSETSTATE			0x00000100


typedef USHORT HVIO;
typedef HVIO *PHVIO;


typedef struct _VIOMODEINFO
{
  USHORT cb;
  UCHAR	 fbType;
  UCHAR	 color;
  USHORT col;
  USHORT row;
  USHORT hres;
  USHORT vres;
  UCHAR	 fmt_ID;
  UCHAR	 attrib;
  ULONG	 buf_addr;
  ULONG	 buf_length;
  ULONG	 full_length;
  ULONG	 partial_length;
  PCH	 ext_data_addr;
} VIOMODEINFO;
typedef VIOMODEINFO *PVIOMODEINFO;

typedef struct _VIOCONFIGINFO
{
  USHORT cb;
  USHORT adapter;
  USHORT display;
  ULONG	 cbMemory;
  USHORT Configuration;
  USHORT VDHVersion;
  USHORT Flags;
  ULONG	 HWBufferSize;
  ULONG	 FullSaveSize;
  ULONG	 PartSaveSize;
  USHORT EMAdaptersOFF;
  USHORT EMDisplaysOFF;
} VIOCONFIGINFO;
typedef VIOCONFIGINFO *PVIOCONFIGINFO;

typedef struct _VIOPHYSBUF
{
  PBYTE pBuf;
  ULONG cb;
  SEL	asel[1];
} VIOPHYSBUF;
typedef VIOPHYSBUF *PVIOPHYSBUF;

typedef struct _VIOPALSTATE
{
  USHORT cb;
  USHORT type;
  USHORT iFirst;
  USHORT acolor[1];
} VIOPALSTATE;
typedef VIOPALSTATE *PVIOPALSTATE;

typedef struct _VIOOVERSCAN
{
  USHORT cb;
  USHORT type;
  USHORT color;
} VIOOVERSCAN;
typedef VIOOVERSCAN *PVIOOVERSCAN;

typedef struct _VIOINTENSITY
{
  USHORT cb;
  USHORT type;
  USHORT fs;
} VIOINTENSITY;
typedef VIOINTENSITY *PVIOINTENSITY;

typedef struct _VIOCOLORREG
{
  USHORT cb;
  USHORT type;
  USHORT firstcolorreg;
  USHORT numcolorregs;
  PCH	 colorregaddr;
} VIOCOLORREG;
typedef VIOCOLORREG *PVIOCOLORREG;

typedef struct _VIOSETULINELOC
{
  USHORT cb;
  USHORT type;
  USHORT scanline;
} VIOSETULINELOC;
typedef VIOSETULINELOC *PVIOSETULINELOC;

typedef struct _VIOSETTARGET
{
  USHORT cb;
  USHORT type;
  USHORT defaultalgorithm;
} VIOSETTARGET;
typedef VIOSETTARGET *PVIOSETTARGET;

typedef struct _VIOCURSORINFO
{
  USHORT yStart;
  USHORT cEnd;
  USHORT cx;
  USHORT attr;
} VIOCURSORINFO;
typedef VIOCURSORINFO *PVIOCURSORINFO;

typedef struct _VIOFONTINFO
{
  USHORT cb;
  USHORT type;
  USHORT cxCell;
  USHORT cyCell;
  ULONG	 pbData;		/* PVOID16 / _far16ptr */
  USHORT cbData;
} VIOFONTINFO;
typedef VIOFONTINFO *PVIOFONTINFO;


USHORT VioCheckCharType (PUSHORT pType, USHORT usRow, USHORT usColumn,
    HVIO hvio);
USHORT VioDeRegister (VOID);
USHORT VioEndPopUp (HVIO hvio);
USHORT VioGetAnsi (PUSHORT pfAnsi, HVIO hvio);
USHORT VioGetBuf (PULONG pLVB, PUSHORT pcbLVB, HVIO hvio);
USHORT VioGetConfig (USHORT usConfigId, PVIOCONFIGINFO pvioin, HVIO hvio);
USHORT VioGetCp (USHORT usReserved, PUSHORT pusCodePage, HVIO hvio);
USHORT VioGetCurPos (PUSHORT pusRow, PUSHORT pusColumn, HVIO hvio);
USHORT VioGetCurType (PVIOCURSORINFO pvioCursorInfo, HVIO hvio);
USHORT VioGetFont (PVIOFONTINFO pviofi, HVIO hvio);
USHORT VioGetMode (PVIOMODEINFO pvioModeInfo, HVIO hvio);
USHORT VioGetPhysBuf (PVIOPHYSBUF pvioPhysBuf, USHORT usReserved);
USHORT VioGetState (PVOID pState, HVIO hvio);
USHORT VioGlobalReg (PCSZ pszModName, PCSZ pszEntryName, ULONG ulFunMask1,
    ULONG ulFunMask2, USHORT usReturn);
USHORT VioModeUndo (USHORT usOwnerInd, USHORT usKillInd, USHORT usReserved);
USHORT VioModeWait (USHORT usReqType, PUSHORT pNotifyType, USHORT usReserved);
USHORT VioPopUp (PUSHORT pfWait, HVIO hvio);
USHORT VioPrtSc (HVIO hvio);
USHORT VioPrtScToggle (HVIO hvio);
USHORT VioReadCellStr (PCH pchCellStr, PUSHORT pcb, USHORT usRow,
    USHORT usColumn, HVIO hvio);
USHORT VioReadCharStr (PCH pch, PUSHORT pcb, USHORT usRow, USHORT usColumn,
    HVIO hvio);
USHORT VioRegister (PCSZ pszModName, PCSZ pszEntryName, ULONG ulFunMask1,
    ULONG ulFunMask2);
USHORT VioSavRedrawUndo (USHORT usOwnerInd, USHORT usKillInd,
    USHORT usReserved);
USHORT VioSavRedrawWait (USHORT usRedrawInd, PUSHORT pusNotifyType,
    USHORT usReserved);
USHORT VioScrLock (USHORT fWait, PUCHAR pfNotLocked, HVIO hvio);
USHORT VioScrollDn (USHORT usTopRow, USHORT usLeftCol, USHORT usBotRow,
    USHORT usRightCol, USHORT cbLines, PBYTE pCell, HVIO hvio);
USHORT VioScrollLf (USHORT usTopRow, USHORT usLeftCol, USHORT usBotRow,
    USHORT usRightCol, USHORT cbCol, PBYTE pCell, HVIO hvio);
USHORT VioScrollRt (USHORT usTopRow, USHORT usLeftCol, USHORT usBotRow,
    USHORT usRightCol, USHORT cbCol, PBYTE pCell, HVIO hvio);
USHORT VioScrollUp (USHORT usTopRow, USHORT usLeftCol, USHORT usBotRow,
    USHORT usRightCol, USHORT cbLines, PBYTE pCell, HVIO hvio);
USHORT VioScrUnLock (HVIO hvio);
USHORT VioSetAnsi (USHORT fAnsi, HVIO hvio);
USHORT VioSetCp (USHORT usReserved, USHORT usCodePage, HVIO hvio);
USHORT VioSetCurPos (USHORT usRow, USHORT usColumn, HVIO hvio);
USHORT VioSetCurType (PVIOCURSORINFO pvioCursorInfo, HVIO hvio);
USHORT VioSetFont (PVIOFONTINFO pviofi, HVIO hvio);
USHORT VioSetMode (PVIOMODEINFO pvioModeInfo, HVIO hvio);
USHORT VioSetState (CPVOID pState, HVIO hvio);
USHORT VioShowBuf (USHORT offLVB, USHORT cb, HVIO hvio);
USHORT VioWrtCellStr (PCCH pchCellStr, USHORT cb, USHORT usRow,
    USHORT usColumn, HVIO hvio);
USHORT VioWrtCharStr (PCCH pch, USHORT cb, USHORT usRow, USHORT usColumn,
    HVIO hvio);
USHORT VioWrtCharStrAtt (PCCH pch, USHORT cb, USHORT usRow, USHORT usColumn,
    PBYTE pAttr, HVIO hvio);
USHORT VioWrtNAttr (__const__ BYTE *pAttr, USHORT cb, USHORT usRow,
    USHORT usColumn, HVIO hvio);
USHORT VioWrtNCell (__const__ BYTE *pCell, USHORT cb, USHORT usRow,
    USHORT usColumn, HVIO hvio);
USHORT VioWrtNChar (PCCH pch, USHORT cb, USHORT usRow, USHORT usColumn,
    HVIO hvio);
USHORT VioWrtTTY (PCCH pch, USHORT cb, HVIO hvio);

#endif /* INCL_VIO */

#if defined (INCL_MOU)

#define MHK_BUTTON1			0x0001
#define MHK_BUTTON2			0x0002
#define MHK_BUTTON3			0x0004

#define MOU_NOWAIT			0x0000
#define MOU_WAIT			0x0001

#define MOUSE_MOTION			0x0001
#define MOUSE_MOTION_WITH_BN1_DOWN	0x0002
#define MOUSE_BN1_DOWN			0x0004
#define MOUSE_MOTION_WITH_BN2_DOWN	0x0008
#define MOUSE_BN2_DOWN			0x0010
#define MOUSE_MOTION_WITH_BN3_DOWN	0x0020
#define MOUSE_BN3_DOWN			0x0040

#define MOUSE_QUEUEBUSY			0x0001
#define MOUSE_BLOCKREAD			0x0002
#define MOUSE_FLUSH			0x0004
#define MOUSE_UNSUPPORTED_MODE		0x0008
#define MOUSE_DISABLED			0x0100
#define MOUSE_MICKEYS			0x0200

#define MOU_NODRAW			0x0001
#define MOU_DRAW			0x0000
#define MOU_MICKEYS			0x0002
#define MOU_PELS			0x0000

#define MR_MOUGETNUMBUTTONS		0x00000001
#define MR_MOUGETNUMMICKEYS		0x00000002
#define MR_MOUGETDEVSTATUS		0x00000004
#define MR_MOUGETNUMQUEEL		0x00000008
#define MR_MOUREADEVENTQUE		0x00000010
#define MR_MOUGETSCALEFACT		0x00000020
#define MR_MOUGETEVENTMASK		0x00000040
#define MR_MOUSETSCALEFACT		0x00000080
#define MR_MOUSETEVENTMASK		0x00000100
#define MR_MOUOPEN			0x00000800
#define MR_MOUCLOSE			0x00001000
#define MR_MOUGETPTRSHAPE		0x00002000
#define MR_MOUSETPTRSHAPE		0x00004000
#define MR_MOUDRAWPTR			0x00008000
#define MR_MOUREMOVEPTR			0x00010000
#define MR_MOUGETPTRPOS			0x00020000
#define MR_MOUSETPTRPOS			0x00040000
#define MR_MOUINITREAL			0x00080000
#define MR_MOUSETDEVSTATUS		0x00100000


typedef USHORT HMOU;
typedef HMOU *PHMOU;


typedef struct _MOUEVENTINFO
{
  USHORT fs;
  ULONG	 time;
  SHORT	 row;
  SHORT	 col;
} MOUEVENTINFO;
typedef MOUEVENTINFO *PMOUEVENTINFO;

typedef struct _MOUQUEINFO
{
  USHORT cEvents;
  USHORT cmaxEvents;
} MOUQUEINFO;
typedef MOUQUEINFO *PMOUQUEINFO;

typedef struct _PTRLOC
{
  USHORT row;
  USHORT col;
} PTRLOC;
typedef PTRLOC *PPTRLOC;

typedef struct _NOPTRRECT
{
  USHORT row;
  USHORT col;
  USHORT cRow;
  USHORT cCol;
} NOPTRRECT;
typedef NOPTRRECT *PNOPTRRECT;

typedef struct _PTRSHAPE
{
  USHORT cb;
  USHORT col;
  USHORT row;
  USHORT colHot;
  USHORT rowHot;
} PTRSHAPE;
typedef PTRSHAPE *PPTRSHAPE;

typedef struct _SCALEFACT
{
  USHORT rowScale;
  USHORT colScale;
} SCALEFACT;
typedef SCALEFACT *PSCALEFACT;

typedef struct _THRESHOLD
{
  USHORT Length;
  USHORT Level1;
  USHORT Lev1Mult;
  USHORT Level2;
  USHORT lev2Mult;
} THRESHOLD;
typedef THRESHOLD *PTHRESHOLD;


USHORT MouClose (HMOU hmou);
USHORT MouDeRegister (VOID);
USHORT MouDrawPtr (HMOU hmou);
USHORT MouFlushQue (HMOU hmou);
USHORT MouGetDevStatus (PUSHORT pfsDevStatus, HMOU hmou);
USHORT MouGetEventMask (PUSHORT pfsEvents, HMOU hmou);
USHORT MouGetNumButtons (PUSHORT pcButtons, HMOU hmou);
USHORT MouGetNumMickeys (PUSHORT pcMickeys, HMOU hmou);
USHORT MouGetNumQueEl (PMOUQUEINFO qmouqi, HMOU hmou);
USHORT MouGetPtrPos (PPTRLOC pmouLoc, HMOU hmou);
USHORT MouGetPtrShape (PBYTE pBuf, PPTRSHAPE pmoupsInfo, HMOU hmou);
USHORT MouGetScaleFact (PSCALEFACT pmouscFactors, HMOU hmou);
USHORT MouGetThreshold (PTHRESHOLD pthreshold, HMOU hmou);
USHORT MouInitReal (PCSZ pszDriverName);
USHORT MouOpen (PCSZ pszDvrName, PHMOU phmou);
USHORT MouReadEventQue (PMOUEVENTINFO pmouevEvent, PUSHORT pfWait, HMOU hmou);
USHORT MouRegister (PCSZ pszModName, PCSZ pszEntryName, ULONG ulFunMask);
USHORT MouRemovePtr (PNOPTRRECT pmourtRect, HMOU hmou);
USHORT MouSetDevStatus (PUSHORT pfsDevStatus, HMOU hmou);
USHORT MouSetEventMask (PUSHORT pfsEvents, HMOU hmou);
USHORT MouSetPtrPos (PPTRLOC pmouLoc, HMOU hmou);
USHORT MouSetPtrShape (PBYTE pBuf, PPTRSHAPE pmoupsInfo, HMOU hmou);
USHORT MouSetScaleFact (PSCALEFACT pmouscFactors, HMOU hmou);
USHORT MouSetThreshold (PTHRESHOLD pthreshold, HMOU hmou);
USHORT MouSynch (USHORT fWait);

#endif /* INCL_MOU */

/* ------------------------------ THE END --------------------------------- */

#pragma pack(4)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* not _OS2EMX_H */

/* Note 1:

   There are some inconsistencies in the structure names defined in
   the header files of the IBM Developer's Toolkit for OS/2.  To make
   C++ modules compiled with those headers linkable with C++ modules
   compiled with this header, we have to use the same (questionable)
   structure names. */

/*
 * Local variables:
 * indent-tabs-mode: t
 * end:
 */

