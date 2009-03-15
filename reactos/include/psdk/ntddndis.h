#ifndef _NTDDNDIS_
#define _NTDDNDIS_
#endif

typedef struct _NDIS_OBJECT_HEADER
{
    UCHAR  Type;
    UCHAR  Revision;
    USHORT Size;
} NDIS_OBJECT_HEADER, *PNDIS_OBJECT_HEADER;


