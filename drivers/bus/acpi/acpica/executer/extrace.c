/******************************************************************************
 *
 * Module Name: extrace - Support for interpreter execution tracing
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"
#include "acinterp.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("extrace")


static ACPI_OPERAND_OBJECT  *AcpiGbl_TraceMethodObject = NULL;

/* Local prototypes */

#ifdef ACPI_DEBUG_OUTPUT
static const char *
AcpiExGetTraceEventName (
    ACPI_TRACE_EVENT_TYPE   Type);
#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiExInterpreterTraceEnabled
 *
 * PARAMETERS:  Name                - Whether method name should be matched,
 *                                    this should be checked before starting
 *                                    the tracer
 *
 * RETURN:      TRUE if interpreter trace is enabled.
 *
 * DESCRIPTION: Check whether interpreter trace is enabled
 *
 ******************************************************************************/

static BOOLEAN
AcpiExInterpreterTraceEnabled (
    char                    *Name)
{

    /* Check if tracing is enabled */

    if (!(AcpiGbl_TraceFlags & ACPI_TRACE_ENABLED))
    {
        return (FALSE);
    }

    /*
     * Check if tracing is filtered:
     *
     * 1. If the tracer is started, AcpiGbl_TraceMethodObject should have
     *    been filled by the trace starter
     * 2. If the tracer is not started, AcpiGbl_TraceMethodName should be
     *    matched if it is specified
     * 3. If the tracer is oneshot style, AcpiGbl_TraceMethodName should
     *    not be cleared by the trace stopper during the first match
     */
    if (AcpiGbl_TraceMethodObject)
    {
        return (TRUE);
    }

    if (Name &&
        (AcpiGbl_TraceMethodName &&
         strcmp (AcpiGbl_TraceMethodName, Name)))
    {
        return (FALSE);
    }

    if ((AcpiGbl_TraceFlags & ACPI_TRACE_ONESHOT) &&
        !AcpiGbl_TraceMethodName)
    {
        return (FALSE);
    }

    return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExGetTraceEventName
 *
 * PARAMETERS:  Type            - Trace event type
 *
 * RETURN:      Trace event name.
 *
 * DESCRIPTION: Used to obtain the full trace event name.
 *
 ******************************************************************************/

#ifdef ACPI_DEBUG_OUTPUT

static const char *
AcpiExGetTraceEventName (
    ACPI_TRACE_EVENT_TYPE   Type)
{

    switch (Type)
    {
    case ACPI_TRACE_AML_METHOD:

        return "Method";

    case ACPI_TRACE_AML_OPCODE:

        return "Opcode";

    case ACPI_TRACE_AML_REGION:

        return "Region";

    default:

        return "";
    }
}

#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiExTracePoint
 *
 * PARAMETERS:  Type                - Trace event type
 *              Begin               - TRUE if before execution
 *              Aml                 - Executed AML address
 *              Pathname            - Object path
 *
 * RETURN:      None
 *
 * DESCRIPTION: Internal interpreter execution trace.
 *
 ******************************************************************************/

void
AcpiExTracePoint (
    ACPI_TRACE_EVENT_TYPE   Type,
    BOOLEAN                 Begin,
    UINT8                   *Aml,
    char                    *Pathname)
{

    ACPI_FUNCTION_NAME (ExTracePoint);


    if (Pathname)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_TRACE_POINT,
            "%s %s [0x%p:%s] execution.\n",
            AcpiExGetTraceEventName (Type), Begin ? "Begin" : "End",
            Aml, Pathname));
    }
    else
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_TRACE_POINT,
            "%s %s [0x%p] execution.\n",
            AcpiExGetTraceEventName (Type), Begin ? "Begin" : "End",
            Aml));
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExStartTraceMethod
 *
 * PARAMETERS:  MethodNode          - Node of the method
 *              ObjDesc             - The method object
 *              WalkState           - current state, NULL if not yet executing
 *                                    a method.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Start control method execution trace
 *
 ******************************************************************************/

void
AcpiExStartTraceMethod (
    ACPI_NAMESPACE_NODE     *MethodNode,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState)
{
    char                    *Pathname = NULL;
    BOOLEAN                 Enabled = FALSE;


    ACPI_FUNCTION_NAME (ExStartTraceMethod);


    if (MethodNode)
    {
        Pathname = AcpiNsGetNormalizedPathname (MethodNode, TRUE);
    }

    Enabled = AcpiExInterpreterTraceEnabled (Pathname);
    if (Enabled && !AcpiGbl_TraceMethodObject)
    {
        AcpiGbl_TraceMethodObject = ObjDesc;
        AcpiGbl_OriginalDbgLevel = AcpiDbgLevel;
        AcpiGbl_OriginalDbgLayer = AcpiDbgLayer;
        AcpiDbgLevel = ACPI_TRACE_LEVEL_ALL;
        AcpiDbgLayer = ACPI_TRACE_LAYER_ALL;

        if (AcpiGbl_TraceDbgLevel)
        {
            AcpiDbgLevel = AcpiGbl_TraceDbgLevel;
        }

        if (AcpiGbl_TraceDbgLayer)
        {
            AcpiDbgLayer = AcpiGbl_TraceDbgLayer;
        }
    }

    if (Enabled)
    {
        ACPI_TRACE_POINT (ACPI_TRACE_AML_METHOD, TRUE,
            ObjDesc ? ObjDesc->Method.AmlStart : NULL, Pathname);
    }

    if (Pathname)
    {
        ACPI_FREE (Pathname);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExStopTraceMethod
 *
 * PARAMETERS:  MethodNode          - Node of the method
 *              ObjDesc             - The method object
 *              WalkState           - current state, NULL if not yet executing
 *                                    a method.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Stop control method execution trace
 *
 ******************************************************************************/

void
AcpiExStopTraceMethod (
    ACPI_NAMESPACE_NODE     *MethodNode,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState)
{
    char                    *Pathname = NULL;
    BOOLEAN                 Enabled;


    ACPI_FUNCTION_NAME (ExStopTraceMethod);


    if (MethodNode)
    {
        Pathname = AcpiNsGetNormalizedPathname (MethodNode, TRUE);
    }

    Enabled = AcpiExInterpreterTraceEnabled (NULL);

    if (Enabled)
    {
        ACPI_TRACE_POINT (ACPI_TRACE_AML_METHOD, FALSE,
            ObjDesc ? ObjDesc->Method.AmlStart : NULL, Pathname);
    }

    /* Check whether the tracer should be stopped */

    if (AcpiGbl_TraceMethodObject == ObjDesc)
    {
        /* Disable further tracing if type is one-shot */

        if (AcpiGbl_TraceFlags & ACPI_TRACE_ONESHOT)
        {
            AcpiGbl_TraceMethodName = NULL;
        }

        AcpiDbgLevel = AcpiGbl_OriginalDbgLevel;
        AcpiDbgLayer = AcpiGbl_OriginalDbgLayer;
        AcpiGbl_TraceMethodObject = NULL;
    }

    if (Pathname)
    {
        ACPI_FREE (Pathname);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExStartTraceOpcode
 *
 * PARAMETERS:  Op                  - The parser opcode object
 *              WalkState           - current state, NULL if not yet executing
 *                                    a method.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Start opcode execution trace
 *
 ******************************************************************************/

void
AcpiExStartTraceOpcode (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_WALK_STATE         *WalkState)
{

    ACPI_FUNCTION_NAME (ExStartTraceOpcode);


    if (AcpiExInterpreterTraceEnabled (NULL) &&
        (AcpiGbl_TraceFlags & ACPI_TRACE_OPCODE))
    {
        ACPI_TRACE_POINT (ACPI_TRACE_AML_OPCODE, TRUE,
            Op->Common.Aml, Op->Common.AmlOpName);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExStopTraceOpcode
 *
 * PARAMETERS:  Op                  - The parser opcode object
 *              WalkState           - current state, NULL if not yet executing
 *                                    a method.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Stop opcode execution trace
 *
 ******************************************************************************/

void
AcpiExStopTraceOpcode (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_WALK_STATE         *WalkState)
{

    ACPI_FUNCTION_NAME (ExStopTraceOpcode);


    if (AcpiExInterpreterTraceEnabled (NULL) &&
        (AcpiGbl_TraceFlags & ACPI_TRACE_OPCODE))
    {
        ACPI_TRACE_POINT (ACPI_TRACE_AML_OPCODE, FALSE,
            Op->Common.Aml, Op->Common.AmlOpName);
    }
}
