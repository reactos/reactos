#pragma once

NTSTATUS
UserInitiateShutdown(IN PETHREAD Thread,
                     IN OUT PULONG pFlags);

NTSTATUS
UserEndShutdown(IN PETHREAD Thread,
                IN NTSTATUS ShutdownStatus);
