/* Prototypes of implemented OFW functions */

typedef long phandle;
typedef long ihandle;

phandle
OFPeer(phandle device_id);

phandle
OFChild(phandle device_id);

phandle
OFParent(phandle device_id);

long
OFGetproplen(
    phandle device_id,
    char *name
    );

long
OFGetprop(
    phandle device_id,
    char *name,
    char *buf,
    ULONG buflen
    );

long
OFNextprop(
    phandle device_id,
    char *name,
    char *buf
    );

long
OFSetprop(
    phandle device_id,
    char *name,
    char *buf,
    ULONG buflen
    );

phandle
OFFinddevice( char *devicename);

ihandle
OFOpen( char *devicename);

void
OFClose(ihandle id);

long
OFRead(
    ihandle instance_id,
    char *addr,
    ULONG len
    );

long
OFWrite(
    ihandle instance_id,
    char *addr,
    ULONG len
    );

long
OFSeek(
    ihandle instance_id,
    ULONG poshi,
    ULONG poslo
    );

ULONG
OFClaim(
    char *addr,
    ULONG size,
    ULONG align
    );

VOID
OFRelease(
    char *addr,
    ULONG size
    );

long
OFPackageToPath(
    phandle device_id,
    char *addr,
    ULONG buflen
    );

phandle
OFInstanceToPackage(ihandle ih);

long
OFCallMethod(
    char *method,
    ihandle id,
    ULONG arg
    );

long
OFInterpret0(
    char *cmd
    );

ULONG
OFMilliseconds( VOID );

void (*OFSetCallback(void (*func)(void)))(void);

long
OFBoot(
    char *bootspec
    );

VOID
OFEnter( VOID );

VOID
OFExit( VOID );

int
ofwprintf(char *fmt, ...);
