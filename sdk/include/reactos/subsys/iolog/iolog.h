/*
 * PROJECT:         ReactOS EventLog Service
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            include/reactos/subsys/iolog/iolog.h
 * PURPOSE:         EventLog LPC API definitions
 */

#ifndef _IOLOG_H
#define _IOLOG_H

#pragma once

// #include <iotypes.h> // For IO_ERROR_LOG_MESSAGE and associated

#define ELF_PORT_NAME   L"\\ErrorLogPort"

typedef struct _ELF_API_MSG
{
    PORT_MESSAGE Header;
    ULONG Unknown[2]; // FIXME
    IO_ERROR_LOG_MESSAGE IoErrorMessage;
} ELF_API_MSG, *PELF_API_MSG;

/*
 * NOTE: The maximum data size sent to the EventLog LPC port
 * is equal to: PORT_MAXIMUM_MESSAGE_LENGTH == 0x100 .
 */

#endif // _IOLOG_H

/* EOF */
