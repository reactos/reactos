/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * PURPOSE:         Device installation
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 *                  Hermes Belusca-Maito
 */

NTSTATUS
WaitNoPendingInstallEvents(
    IN PLARGE_INTEGER Timeout OPTIONAL);

BOOLEAN
EnableUserModePnpManager(VOID);

BOOLEAN
DisableUserModePnpManager(VOID);

NTSTATUS
InitializeUserModePnpManager(
    IN HINF* phSetupInf);

VOID
TerminateUserModePnpManager(VOID);
