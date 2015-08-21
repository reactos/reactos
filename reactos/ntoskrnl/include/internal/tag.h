#pragma once

/* Cache Manager Tags */
#define TAG_CC                  '  cC'
#define TAG_VACB                'aVcC'
#define TAG_SHARED_CACHE_MAP    'cScC'
#define TAG_PRIVATE_CACHE_MAP   'cPcC'
#define TAG_BCB                 'cBcC'

/* Executive Callbacks */
#define TAG_CALLBACK_ROUTINE_BLOCK 'brbC'
#define TAG_CALLBACK_REGISTRATION  'eRBC'

/* formely located in dbg/dbgkobj.c */
#define TAG_DEBUG_EVENT     'EgbD'

/* formerly located in ex/resource.c */
#define TAG_RESOURCE_TABLE      'aTeR'
#define TAG_RESOURCE_EVENT      'aTeR'
#define TAG_RESOURCE_SEMAPHORE  'aTeR'

/* formerly located in ex/handle.c */
#define TAG_OBJECT_TABLE 'btbO'

/* formerly located in ex/init.c */
#define TAG_INIT 'tinI'
#define TAG_RTLI 'iltR'

/* formerly located in fs/notify.c */
#define FSRTL_NOTIFY_TAG 'ITON'

/* formerly located in fsrtl/unc.c */
#define TAG_UNC 'nuSF'

/* formerly located in io/device.c */
#define TAG_DEVICE_EXTENSION   'TXED'
#define TAG_SHUTDOWN_ENTRY    'TUHS'
#define TAG_IO_TIMER      'MTOI'

/* formerly located in io/driver.c */
#define TAG_DRIVER             'RVRD'
#define TAG_DRIVER_EXTENSION   'EVRD'

/* formerly located in io/file.c */
#define TAG_SYSB        'BSYS'
#define TAG_LOCK        'kclF'
#define TAG_FILE_NAME   'MANF'

/* formerly located in io/fs.c */
#define TAG_FILE_SYSTEM       'SYSF'
#define TAG_FS_CHANGE_NOTIFY  'NCSF'
#define IFS_POOL_TAG          'trSF'

/* formerly located in io/iocomp.c */
#define IOC_TAG   'TCOI'

/* formerly located in io/iomgr.c */
#define TAG_DEVICE_TYPE     'TVED'
#define TAG_FILE_TYPE       'ELIF'
#define TAG_ADAPTER_TYPE    'TPDA'
#define IO_LARGEIRP         'lprI'
#define IO_SMALLIRP         'sprI'
#define IO_LARGEIRP_CPU     'LprI'
#define IO_SMALLIRP_CPU     'SprI'
#define IOC_TAG1            ' cpI'
#define IOC_CPU             'PcpI'
#define TAG_APC             'CPAK'
#define TAG_IO              '  oI'
#define TAG_ERROR_LOG       'rEoI'
#define TAG_EA              'aEoI'
#define TAG_IO_NAME         'mNoI'
#define TAG_REINIT          'iRoI'

/* formerly located in io/work.c */
#define TAG_IOWI 'IWOI'

/* formerly located in io/irp.c */
#define TAG_IRP      ' PRI'
#define TAG_SYS_BUF  'BSYS'

/* formerly located in io/irq.c */
#define TAG_KINTERRUPT   'RSIK'

/* formerly located in io/mdl.c */
#define TAG_MDL    ' LDM'

/* formerly located in io/pnpmgr.c */
#define TAG_IO_DEVNODE 'donD'

/* formerly located in io/pnpnotify.c */
#define TAG_PNP_NOTIFY  'NPnP'

/* for io/pnproot.c */
#define TAG_PNP_ROOT    'RPnP'

/* formerly located in io/resource.c */
#define TAG_IO_RESOURCE    'CRSR'

/* formerly located in io/timer.c */
#define TAG_IO_TIMER      'MTOI'

/* formerly located in io/vpb.c */
#define TAG_VPB    ' BPV'
#define TAG_SYSB   'BSYS'

/* formerly located in ldr/loader.c */
#define TAG_DRIVER_MEM  'MVRD' /* drvm */
#define TAG_MODULE_OBJECT 'omlk' /* klmo - kernel ldr module object */
#define TAG_LDR_WSTR 'swlk' /* klws - kernel ldr wide string */
#define TAG_LDR_IMPORTS 'klim' /* klim - kernel ldr imports */

/* formerly located in lpc/connect */
#define TAG_LPC_CONNECT_MESSAGE   'CCPL'

/* formerly located in mm/aspace.c */
#define TAG_PTRC      'CRTP'

/* formerly located in mm/marea.c */
#define TAG_MAREA   'ERAM'
#define TAG_MVAD    'VADM'

/* formerly located in mm/pageop.c */
#define TAG_MM_PAGEOP   'POPM'

/* formerly located in mm/pool.c */
#define TAG_NONE 'enoN'

/* formerly located in mm/region.c */
#define TAG_MM_REGION    'NGRM'

/* formerly located in mm/rmap.c */
#define TAG_RMAP    'PAMR'

/* formerly located in mm/ARM3/section.c */
#define TAG_MM      '  mM'

/* formerly located in mm/section.c */
#define TAG_MM_SECTION_SEGMENT   'SSMM'
#define TAG_SECTION_PAGE_TABLE   'TPSM'

/* formerly located in ob/symlink.c */
#define TAG_OBJECT_TYPE         'TjbO'
#define TAG_SYMLINK_TTARGET     'TTYS'
#define TAG_SYMLINK_TARGET      'TMYS'

/* formerly located in ob/obsdcach.c */
#define TAG_OB_SD_CACHE         'cSbO'

/* Object Manager Tags */
#define OB_NAME_TAG             'mNbO'
#define OB_DIR_TAG              'iDbO'

/* formerly located in ps/cid.c */
#define TAG_CIDOBJECT 'ODIC'
#define TAG_PS_IMPERSONATION    'mIsP'

/* formerly located in ps/job.c */
#define TAG_EJOB 'BOJE' /* EJOB */

/* formerly located in ps/kill.c */
#define TAG_TERMINATE_APC   'CPAT'

/* formerly located in ps/notify.c */
#define TAG_KAPC 'papk' /* kpap - kernel ps apc */
#define TAG_PS_APC 'pasP' /* Psap - Ps APC */

/* formerly located in rtl/handle.c */
#define TAG_HDTB  'BTDH'

/* Security Manager Tags */
#define TAG_SE                '  eS'
#define TAG_ACL               'cAeS'
#define TAG_SID               'iSeS'
#define TAG_SD                'dSeS'
#define TAG_QOS               'sQeS'
#define TAG_LUID              'uLeS'
#define TAG_PRIVILEGE_SET     'rPeS'
#define TAG_TOKEN_USERS       'uKOT'
#define TAG_TOKEN_PRIVILAGES  'pKOT'
#define TAG_TOKEN_ACL         'kDOT'

/* LPC Tags */
#define TAG_LPC_MESSAGE   'McpL'
#define TAG_LPC_ZONE      'ZcpL'

/* Se Process Audit */
#define TAG_SEPA          'aPeS'

#define TAG_WAIT            'tiaW'
#define TAG_SEC_QUERY       'qSbO'
