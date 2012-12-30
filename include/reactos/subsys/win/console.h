/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            include/reactos/subsys/win/console.h
 * PURPOSE:         Public definitions for Console API Clients
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

#pragma once

#define IsConsoleHandle(h)  \
    (((ULONG_PTR)(h) & 0x10000003) == 0x3)

#endif // _CONSOLE_H

/* EOF */
