/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            include/reactos/subsys/win/console.h
 * PURPOSE:         Public definitions for Console API Clients
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

#pragma once

#define IsConsoleHandle(h)  \
    (((ULONG_PTR)(h) & 0x10000003) == 0x3)

/* Console-reserved device "file" names */
#define CONSOLE_FILE_NAME           L"CON"
#define CONSOLE_INPUT_FILE_NAME     L"CONIN$"
#define CONSOLE_OUTPUT_FILE_NAME    L"CONOUT$"

#endif // _CONSOLE_H

/* EOF */
