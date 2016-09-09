#pragma once

LRESULT
IntClientShutdown(IN PWND pWindow,
                  IN WPARAM wParam,
                  IN LPARAM lParam);

NTSTATUS
UserInitiateShutdown(IN PETHREAD Thread,
                     IN OUT PULONG pFlags);

NTSTATUS
UserEndShutdown(IN PETHREAD Thread,
                IN NTSTATUS ShutdownStatus);
