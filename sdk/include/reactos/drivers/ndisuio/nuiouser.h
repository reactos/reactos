#ifndef __NUIOUSER_H
#define __NUIOUSER_H

/* Device names (NT and DOS style) */
#define NDISUIO_DEVICE_NAME_NT   L"\\Device\\Ndisuio"
#define NDISUIO_DEVICE_NAME_DOS  L"\\DosDevices\\Ndisuio"

/* Device name for user apps */
#define NDISUIO_DEVICE_NAME      L"\\\\.\\\\Ndisuio"

/* Links a file handle with a bound NIC */
#define IOCTL_NDISUIO_OPEN_DEVICE \
            CTL_CODE(FILE_DEVICE_NETWORK, 0x200, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Queries an OID for the bound NIC */
#define IOCTL_NDISUIO_QUERY_OID_VALUE \
            CTL_CODE(FILE_DEVICE_NETWORK, 0x201, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_NDISUIO_SET_ETHER_TYPE \
            CTL_CODE(FILE_DEVICE_NETWORK, 0x202, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Queries binding information during enumeration */
#define IOCTL_NDISUIO_QUERY_BINDING \
            CTL_CODE(FILE_DEVICE_NETWORK, 0x203, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Waits for any pending bindings */
#define IOCTL_NDISUIO_BIND_WAIT \
            CTL_CODE(FILE_DEVICE_NETWORK, 0x204, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Sets an OID for a bound NIC */
#define IOCTL_NDISUIO_SET_OID_VALUE \
            CTL_CODE(FILE_DEVICE_NETWORK, 0x205, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Passed as a parameter to IOCTL_NDISUIO_QUERY_OID_VALUE */
typedef struct _NDISUIO_QUERY_OID
{
    NDIS_OID        Oid;
    UCHAR           Data[sizeof(ULONG)];
} NDISUIO_QUERY_OID, *PNDISUIO_QUERY_OID;

/* Passed as a parameter to IOCTL_NDISUIO_SET_OID_VALUE */
typedef struct _NDISUIO_SET_OID
{
    NDIS_OID        Oid;
    UCHAR           Data[sizeof(ULONG)];
} NDISUIO_SET_OID, *PNDISUIO_SET_OID;

/* Passed as a parameter to IOCTL_NDISUIO_QUERY_BINDING */
typedef struct _NDISUIO_QUERY_BINDING
{
	ULONG			BindingIndex;
	ULONG			DeviceNameOffset;
	ULONG			DeviceNameLength;
	ULONG			DeviceDescrOffset;
	ULONG			DeviceDescrLength;
} NDISUIO_QUERY_BINDING, *PNDISUIO_QUERY_BINDING;

#endif

