/*
 * MINIPORT.H - miniport driver interface header
 */

#define  EMULATOR_READ_ACCESS   0x00000001
#define  EMULATOR_WRITE_ACCESS  0x00000002

typedef enum _EMULATOR_PORT_ACCESS_TYPE 
{
  Uchar,
  Ushort,
  Ulong
} EMULATOR_PORT_ACCESS_TYPE, *PEMULATOR_PORT_ACCESS_TYPE;

typedef struct _EMULATOR_ACCESS_ENTRY 
{
  ULONG  BasePort;
  ULONG  NumConsecutivePorts;
  EMULATOR_PORT_ACCESS_TYPE  AccessType;
  UCHAR  AccessMode;
  UCHAR  StringSupport;
  PVOID  Routine;
} EMULATOR_ACCESS_ENTRY, *PEMULATOR_ACCESS_ENTRY;


