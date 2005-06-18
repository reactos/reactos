/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/extypes.h
 * PURPOSE:         Definitions for exported Executive Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */

#ifndef _EXTYPES_H
#define _EXTYPES_H

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/
extern POBJECT_TYPE NTOSAPI ExIoCompletionType;

/* CONSTANTS *****************************************************************/

/* ENUMERATIONS **************************************************************/

typedef enum _HARDERROR_RESPONSE_OPTION 
{
    OptionAbortRetryIgnore,
    OptionOk,
    OptionOkCancel,
    OptionRetryCancel,
    OptionYesNo,
    OptionYesNoCancel,
    OptionShutdownSystem
} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION;

typedef enum _HARDERROR_RESPONSE 
{
    ResponseReturnToCaller,
    ResponseNotHandled,
    ResponseAbort,
    ResponseCancel,
    ResponseIgnore,
    ResponseNo,
    ResponseOk,
    ResponseRetry,
    ResponseYes
} HARDERROR_RESPONSE, *PHARDERROR_RESPONSE;

/* TYPES *********************************************************************/

#endif

