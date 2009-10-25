
#ifndef _NETTYPES_
#define _NETTYPES_

#define HARDWARE_ADDRESS_LENGTH             6
#define NETMAN_VARTYPE_ULONG                0
#define NETMAN_VARTYPE_HARDWARE_ADDRESS     1
#define NETMAN_VARTYPE_STRING               2


typedef ULONG OFFSET;

typedef struct _FLAT_STRING
{
  SHORT MaximumLength;
  SHORT Length;
  char Buffer [1];
} FLAT_STRING, *PFLAT_STRING;


typedef struct _NETWORK_NAME
{
  FLAT_STRING Name;
} NETWORK_NAME, *PNETWORK_NAME;

typedef struct _HARDWARE_ADDRESS
{
  UCHAR Address [HARDWARE_ADDRESS_LENGTH];
} HARDWARE_ADDRESS, *PHARDWARE_ADDRESS;

#endif

