/*
 * PROJECT:         ReactOS Build Tools [Keyboard Layout Compiler]
 * LICENSE:         BSD - See COPYING.BSD in the top level directory
 * FILE:            tools/kbdtool/output.c
 * PURPOSE:         Output Logic (Source Builder)
 * PROGRAMMERS:     ReactOS Foundation
 */

/* INCLUDES *******************************************************************/

#include "kbdtool.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

BOOLEAN
kbd_h(IN PLAYOUT Layout)
{
    /* FIXME: Stub */
    return FALSE;
}

BOOLEAN
kbd_rc(IN PKEYNAME DescriptionData,
       IN PKEYNAME LanguageData)
{
    /* FIXME: Stub */
    return FALSE;
}

BOOLEAN
kbd_def(VOID)
{
    /* FIXME: Stub */
    return FALSE;   
}

BOOLEAN
kbd_c(IN ULONG StateCount,
      IN PULONG ShiftStates,
      IN PVOID AttributeData,
      IN PLAYOUT Layout,
      IN PVOID DeadKeyData,
      IN PVOID LigatureData,
      IN PKEYNAME KeyNameData,
      IN PKEYNAME KeyNameExtData,
      IN PKEYNAME KeyNameDeadData)
{
    /* FIXME: Stub */
    return FALSE;
}

ULONG
DoOutput(IN ULONG StateCount,
         IN PULONG ShiftStates,
         IN PKEYNAME DescriptionData,
         IN PKEYNAME LanguageData,
         IN PVOID AttributeData,
         IN PVOID DeadKeyData,
         IN PVOID LigatureData,
         IN PKEYNAME KeyNameData,
         IN PKEYNAME KeyNameExtData,
         IN PKEYNAME KeyNameDeadData)
{
    ULONG FailureCode = 0;
    
    /* Check if this just a fallback driver*/
    if (!FallbackDriver)
    {
        /* It's not, create header file */
        if (!kbd_h(&g_Layout)) FailureCode = 1;
        
        /* Create the resource file */
        if (!kbd_rc(DescriptionData, LanguageData)) FailureCode = 2;
    }
    
    /* Create the C file */
    if (!kbd_c(StateCount,
               ShiftStates,
               AttributeData,
               &g_Layout,
               DeadKeyData,
               LigatureData,
               KeyNameData,
               KeyNameExtData,
               KeyNameDeadData))
    {
        /* Failed in C file generation */
        FailureCode = 3;
    }
    
    /* Check if this just a fallback driver*/
    if (!FallbackDriver)
    {
        /* Generate the definition file */
        if (!kbd_def()) FailureCode = 4;
    }
    
    /* Done */
    return FailureCode;
}


/* EOF */
