#ifndef _INC_REACTOS_CONFIG_H
#define _INC_REACTOS_CONFIG_H 
/* $Id: config.h,v 1.1 1999/08/29 07:01:32 ea Exp $ */
/* ReactOS global configuration options */

#define CONFIG_PROCESSOR_FAMILY_I386	"i386"
#define CONFIG_PROCESSOR_FAMILY_I486	"i486"
#define CONFIG_PROCESSOR_FAMILY_I586	"i586"

#define CONFIG_PROCESSOR_FAMILY_ALPHA	"ALPHA"

#define CONFIG_ARCHITECTURE_IBMPC	"IBMPC"
/*
 * Processor and architecture.
 */
#define CONFIG_PROCESSOR_FAMILY	CONFIG_PROCESSOR_FAMILY_I586
#define CONFIG_ARCHITECTURE	CONFIG_ARCHITECTURE_IBMPC
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
