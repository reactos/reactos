/****************************** Module Header ******************************\
* Module Name: process.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains processor specific routines.
*
* History:
* 25-Oct-1995 JimA      Created.
\***************************************************************************/

#include "precomp.h"

#include <imagehlp.h>
#include <wdbgexts.h>
#include <ntsdexts.h>

#include <stdexts.h>

#define GETOUTPUTDATA(pEProcess, field, pvData, cbData)         \
    pvData = (PBYTE)pEProcess + FIELD_OFFSET(EPROCESS, field);  \
    cbData = sizeof(((PEPROCESS)0)->field);

PVOID GetEProcessData(
    PEPROCESS pEProcess,
    UINT iData,
    PVOID pBuffer)
{
    PVOID pvData;
    ULONG cbData;

    switch (iData) {
    case PROCESS_PROCESSLINK:
        GETOUTPUTDATA(pEProcess, ActiveProcessLinks, pvData, cbData);
        break;
    case PROCESS_WIN32PROCESS:
        GETOUTPUTDATA(pEProcess, Win32Process, pvData, cbData);
        break;
    case PROCESS_IMAGEFILENAME:
        GETOUTPUTDATA(pEProcess, ImageFileName, pvData, cbData);
        break;
    case PROCESS_THREADLIST:
        GETOUTPUTDATA(pEProcess, Pcb.ThreadListHead, pvData, cbData);
        break;
    case PROCESS_PRIORITYCLASS:
        GETOUTPUTDATA(pEProcess, PriorityClass, pvData, cbData);
        break;
    case PROCESS_PROCESSHEAD:
        return CONTAINING_RECORD(pEProcess, EPROCESS, ActiveProcessLinks);
    case PROCESS_PROCESSID:
        GETOUTPUTDATA(pEProcess, UniqueProcessId, pvData, cbData);
        break;
    default:
        return NULL;
    }
    if (!tryMoveBlock(pBuffer, pvData, cbData))
        return NULL;
    return pvData;
}
