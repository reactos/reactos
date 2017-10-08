/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            subsystems/win/basesrv/api.h
 * PURPOSE:         Public server APIs definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* appcompat.c */
CSR_API(BaseSrvCheckApplicationCompatibility);

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
CSR_API(BaseSrvDebugProcess);
CSR_API(BaseSrvRegisterThread);
CSR_API(BaseSrvSxsCreateActivationContext);
CSR_API(BaseSrvSetTermsrvAppInstallMode);
CSR_API(BaseSrvSetTermsrvClientTimeZone);

/* sndsntry.c */
CSR_API(BaseSrvSoundSentryNotification);

/* vdm.c */
CSR_API(BaseSrvCheckVDM);
CSR_API(BaseSrvUpdateVDMEntry);
CSR_API(BaseSrvGetNextVDMCommand);
CSR_API(BaseSrvExitVDM);
CSR_API(BaseSrvIsFirstVDM);
CSR_API(BaseSrvGetVDMExitCode);
CSR_API(BaseSrvSetReenterCount);
CSR_API(BaseSrvSetVDMCurDirs);
CSR_API(BaseSrvGetVDMCurDirs);
CSR_API(BaseSrvBatNotification);
CSR_API(BaseSrvRegisterWowExec);
CSR_API(BaseSrvRefreshIniFileMapping);

/* nls.c */
VOID
NTAPI
BaseSrvNLSInit(IN PBASE_STATIC_SERVER_DATA StaticData);

NTSTATUS
NTAPI
BaseSrvNlsConnect(IN PCSR_PROCESS CsrProcess,
                  IN OUT PVOID  ConnectionInfo,
                  IN OUT PULONG ConnectionInfoLength);

CSR_API(BaseSrvNlsSetUserInfo);
CSR_API(BaseSrvNlsSetMultipleUserInfo);
CSR_API(BaseSrvNlsCreateSection);
CSR_API(BaseSrvNlsUpdateCacheCount);
CSR_API(BaseSrvNlsGetUserInfo);

/* EOF */
