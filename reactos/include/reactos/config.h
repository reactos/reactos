#ifndef _INC_REACTOS_CONFIG_H
#define _INC_REACTOS_CONFIG_H 
/* $Id: config.h,v 1.3 2000/02/20 22:52:47 ea Exp $ */
/* ReactOS global configuration options */

#define CONFIG_PROCESSOR_FAMILY_I386	386L
#define CONFIG_PROCESSOR_FAMILY_I486	486L
#define CONFIG_PROCESSOR_FAMILY_I586	586L
#define CONFIG_PROCESSOR_FAMILY_IPII	686L

#define CONFIG_PROCESSOR_FAMILY_ALPHA	0x10000000

#define CONFIG_ARCHITECTURE_IBMPC	0x00000000
/*
 * Processor and architecture.
 */
#define CONFIG_PROCESSOR_FAMILY	CONFIG_PROCESSOR_FAMILY_I586
#define CONFIG_ARCHITECTURE	CONFIG_ARCHITECTURE_IBMPC
/*
 * Hardware page size
 */
#define CONFIG_MEMORY_PAGE_SIZE	4096
/*
 * Use __fastcall calling conventions when needed
 * in system components that require it.
 */
//#define CONFIG_USE_FASTCALL
/*
 * Enable debugging output on a per module
 * base.
 */
#define DBG_NTOSKRNL_KE_MAIN
#define DBG_NTOSKRNL_MM_MM
#define DBG_NTOSKRNL_MM_NPOOL

#define DBG_NTDLL_LDR_STARTUP
#define DBG_NTDLL_LDR_UTILS

#endif /* ndef _INC_REACTOS_CONFIG_H */
