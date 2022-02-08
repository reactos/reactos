/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         ReactOS NT kernel pool allocation tags
 * COPYRIGHT:       Copyright 2005 Steven Edwards <sedwards@reactos.com>
 *                  Copyright 2006 Alex Ionescu <alex.ionescu@reactos.org>
 *                  Copyright 2015 Thomas Faber <thomas.faber@reactos.org>
 *                  Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

#pragma once

/* Cache Manager Tags */
#define TAG_CC                  '  cC'
#define TAG_VACB                'aVcC'
#define TAG_SHARED_CACHE_MAP    'cScC'
#define TAG_PRIVATE_CACHE_MAP   'cPcC'
#define TAG_BCB                 'cBcC'

/* Executive Tags */
#define TAG_CALLBACK_ROUTINE_BLOCK 'brbC'
#define TAG_CALLBACK_REGISTRATION  'eRBC'
#define TAG_RESOURCE_TABLE         'aTeR'
#define TAG_RESOURCE_EVENT         'aTeR'
#define TAG_RESOURCE_SEMAPHORE     'aTeR'
#define TAG_OBJECT_TABLE           'btbO'
#define TAG_INIT                   'tinI'
#define TAG_RTLI                   'iltR'
#define TAG_ATOM                   'motA'
#define TAG_PROFILE                'forP'
#define TAG_ERR                    ' rrE'

/* User Mode Debugging Manager Tag */
#define TAG_DEBUG_EVENT 'EgbD'

/* Kernel Debugger Tags */
#define TAG_KDBS 'SBDK'
#define TAG_KDBG 'GBDK'

/* Kernel Tags */
#define TAG_KNMI    'IMNK'
#define TAG_KERNEL  '  eK'

/* File-System Run-Time Library Tags */
#define TAG_UNC    'nuSF'
#define TAG_TABLE  'BATL'
#define TAG_RANGE  'ARSF'
#define TAG_FLOCK  'KCLF'
#define TAG_OPLOCK 'orSF'

/* I/O Manager Tags */
#define TAG_DEVICE_EXTENSION   'TXED'
#define TAG_SHUTDOWN_ENTRY     'TUHS'
#define TAG_IO_TIMER           'MTOI'
#define TAG_DRIVER             'RVRD'
#define TAG_DRIVER_EXTENSION   'EVRD'
#define TAG_SYSB               'BSYS'
#define TAG_LOCK               'kclF'
#define TAG_FILE_NAME          'MANF'
#define TAG_FILE_SYSTEM        'SYSF'
#define TAG_FS_CHANGE_NOTIFY   'NCSF'
#define IFS_POOL_TAG           'trSF'
#define TAG_FS_NOTIFICATIONS   'NrSF'
#define IOC_TAG                'TCOI'
#define TAG_DEVICE_TYPE        'TVED'
#define TAG_FILE_TYPE          'ELIF'
#define TAG_ADAPTER_TYPE       'TPDA'
#define IO_LARGEIRP            'lprI'
#define IO_SMALLIRP            'sprI'
#define IO_LARGEIRP_CPU        'LprI'
#define IO_SMALLIRP_CPU        'SprI'
#define IOC_TAG1               ' cpI'
#define IOC_CPU                'PcpI'
#define TAG_APC                'CPAK'
#define TAG_IO                 '  oI'
#define TAG_ERROR_LOG          'rEoI'
#define TAG_EA                 'aEoI'
#define TAG_IO_NAME            'mNoI'
#define TAG_REINIT             'iRoI'
#define TAG_IOWI               'IWOI'
#define TAG_IRP                ' prI'
#define TAG_SYS_BUF            'BSYS'
#define TAG_KINTERRUPT         'RSIK'
#define TAG_MDL                ' LDM'
#define TAG_IO_DEVNODE         'donD'
#define TAG_PNP_NOTIFY         'NPnP'
#define TAG_PNP_ROOT           'RPnP'
#define TAG_IO_RESOURCE        'CRSR'
#define TAG_IO_TIMER           'MTOI'
#define TAG_VPB                ' BPV'
#define TAG_SYSB               'BSYS'
#define TAG_RTLREGISTRY        'vrqR'
#define TAG_PNP_DEVACTION      'aDpP'

/* Loader Related Tags */
#define TAG_MODULE_OBJECT 'omlk' /* klmo - kernel ldr module object */
#define TAG_LDR_WSTR      'swlk' /* klws - kernel ldr wide string */
#define TAG_LDR_IMPORTS   'milk' /* klim - kernel ldr imports */

/* Memory Manager Tags */
#define TAG_PTRC                 'CRTP'
#define TAG_MAREA                'ERAM'
#define TAG_MVAD                 'VADM'
#define TAG_MM_PAGEOP            'POPM'
#define TAG_NONE                 'enoN'
#define TAG_MM_REGION            'NGRM'
#define TAG_RMAP                 'PAMR'
#define TAG_MM                   '  mM'
#define TAG_MM_SECTION_SEGMENT   'SSMM'
#define TAG_SECTION_PAGE_TABLE   'TPSM'

/* Object Manager Tags */
#define OB_NAME_TAG             'mNbO'
#define OB_DIR_TAG              'iDbO'
#define TAG_WAIT                'tiaW'
#define TAG_SEC_QUERY           'qSbO'
#define TAG_OBJECT_TYPE         'TjbO'
#define TAG_SYMLINK_TTARGET     'TTYS'
#define TAG_SYMLINK_TARGET      'TMYS'
#define TAG_OB_SD_CACHE         'cSbO'
#define TAG_OB_HANDLE           'dHbO'

/* Power Manager Tag */
#define TAG_PO_DOPE 'EPOD'

/* Process Manager Tags */
#define TAG_CIDOBJECT           'ODIC'
#define TAG_PS_IMPERSONATION    'mIsP'
#define TAG_EJOB                'BOJE' /* EJOB */
#define TAG_TERMINATE_APC       'CPAT'
#define TAG_KAPC                'papk' /* kpap - kernel ps apc */
#define TAG_PS_APC              'pasP' /* Psap - Ps APC */
#define TAG_SHIM                'MIHS'
#define TAG_QUOTA_BLOCK         'bQsP'

/* Run-Time Library Tags */
#define TAG_HDTB  'BTDH'
#define TAG_ATMT  'TotA' /* Atom table */
#define TAG_RTHL  'LHtR' /* Heap Lock */
#define TAG_USTR  'RTSU'
#define TAG_ASTR  'RTSA'
#define TAG_OSTR  'RTSO'

/* Security Manager Tags */
#define TAG_SE                 '  eS'
#define TAG_ACL                'cAeS'
#define TAG_SID                'iSeS'
#define TAG_SD                 'dSeS'
#define TAG_QOS                'sQeS'
#define TAG_LUID               'uLeS'
#define TAG_SEPA               'aPeS'
#define TAG_PRIVILEGE_SET      'rPeS'
#define TAG_TOKEN_DYNAMIC      'dTeS'
#define TAG_SE_HANDLES_TAB     'aHeS'
#define TAG_SE_DIR_BUFFER      'bDeS'
#define TAG_SE_PROXY_DATA      'dPoT'
#define TAG_SE_TOKEN_LOCK      'lTeS'
#define TAG_LOGON_SESSION      'sLeS'
#define TAG_LOGON_NOTIFICATION 'nLeS'
#define TAG_SID_AND_ATTRIBUTES 'aSeS'
#define TAG_SID_VALIDATE       'vSeS'
#define TAG_ACCESS_CHECK_RIGHT 'rCeS'

/* LPC Tags */
#define TAG_LPC_MESSAGE           'McpL'
#define TAG_LPC_ZONE              'ZcpL'
#define TAG_LPC_CONNECT_MESSAGE   'CCPL'

/* FSTUB Tag */
#define TAG_FSTUB 'BtsF'
