#define IDD_SYSTEM_SHUTDOWN         1300
#define IDD_TIMER                   1303
#define IDD_MESSAGE                 1305
#define IDD_SYSTEM_MESSAGE          1306

BOOLEAN
ShutdownThread(
    PULONG Flags
    );

BOOL
InitializeShutdownModule(
    VOID
    );
