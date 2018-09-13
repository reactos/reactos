#ifdef __cplusplus
extern "C" {
#endif

#define PIPE_BUFFER_SIZE        (1024 * 5)
#define DEFAULT_PIPE            "WinDbg_Pipe"
#define MAX_CONNECT_WAIT        30

DWORD   TlUtilTime(void);


//
// packet types for the control pipe
//

#define CP_REQUEST_CONNECTION     1
#define CP_BREAKIN_CONNECTION     1


typedef struct _tagCONTROLPACKET {
    DWORD       Length;                          // this includes the packet overhead
    DWORD       Type;
    CHAR        ClientId[16];
    DWORD       Response;
    BYTE        Data[];
} CONTROLPACKET, *LPCONTROLPACKET;



DWORD
TlReadControl(
    DWORD   ciIdx,
    PUCHAR  pch,
    DWORD   cch
    );

BOOL
TlWriteControl(
    DWORD   ciIdx,
    PUCHAR  pch,
    DWORD   cch
    );

TlConnectControlPipe(
    VOID
    );

VOID
TlPipeFailure(
    VOID
    );

XOSD
TlCreateControlPipe(
    LPSTR  szName
    );

XOSD
TlCreateClientControlPipe(
    LPSTR  szParams
    );

VOID
ControlPipeFailure(
    VOID
    );

BOOL
TlDisconnectControl(
    DWORD   ciIdx
    );

VOID
TlControlInitialization(
    VOID
    );

#ifdef __cplusplus
} // extern "C" {
#endif
