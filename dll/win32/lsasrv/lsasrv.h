/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/lsasrv.h
 * PURPOSE:         Common header file
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* authport.c */
NTSTATUS StartAuthenticationPort(VOID);

/* lsarpc.c */
VOID LsarStartRpcServer(VOID);
