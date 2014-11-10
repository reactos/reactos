/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS CRT
 * FILE:            lib/pseh/arm/seh_prolog.S
 * PURPOSE:         SEH Support for MSVC / ARM
 * PROGRAMMERS:     Timo Kreuzer
 */

/* INCLUDES ******************************************************************/

#include "ksarm.h"

    TEXTAREA

    IMPORT __except_handler

    LEAF_ENTRY _SEH_prolog


    LEAF_END _SEH_prolog



    LEAF_ENTRY _SEH_epilog


    LEAF_END _SEH_epilog



    END
