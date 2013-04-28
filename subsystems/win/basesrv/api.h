/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            subsystems/win/basesrv/api.h
 * PURPOSE:         Public server APIs definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* dosdev.c */
VOID BaseInitDefineDosDevice(VOID);
VOID BaseCleanupDefineDosDevice(VOID);

CSR_API(BaseSrvDefineDosDevice);

/* proc.c */
CSR_API(BaseSrvGetTempFile);
CSR_API(BaseSrvCreateProcess);
CSR_API(BaseSrvCreateThread);
CSR_API(BaseSrvExitProcess);
CSR_API(BaseSrvGetProcessShutdownParam);
CSR_API(BaseSrvSetProcessShutdownParam);

/* sndsntry.c */
CSR_API(BaseSrvSoundSentryNotification);

/* EOF */
