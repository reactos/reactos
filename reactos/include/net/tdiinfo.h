/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TDI library
 * FILE:        include/net/tdiinfo.h
 * PURPOSE:     TDI definitions for Tdi(Query/Set)InformationEx
 */
#ifndef __TDIINFO_H
#define __TDIINFO_H

typedef struct TDIEntityID
{
    ULONG   tei_entity;
	ULONG   tei_instance;
} TDIEntityID;

typedef struct TDIObjectID
{
    TDIEntityID	toi_entity;
    ULONG   toi_class;
	ULONG   toi_type;
	ULONG   toi_id;
} TDIObjectID;

#define	CONTEXT_SIZE                16

#define	MAX_TDI_ENTITIES			512

#define	INFO_CLASS_GENERIC          0x100
#define	INFO_CLASS_PROTOCOL	        0x200
#define	INFO_CLASS_IMPLEMENTATION   0x300

#define	INFO_TYPE_PROVIDER          0x100
#define	INFO_TYPE_ADDRESS_OBJECT    0x200
#define	INFO_TYPE_CONNECTION        0x300


#define	ENTITY_LIST_ID              0

#define	GENERIC_ENTITY              0


#define	IF_ENTITY                   0x200

#define	AT_ENTITY                   0x280

#define	CO_NL_ENTITY                0x300
#define	CL_NL_ENTITY                0x301

#define	ER_ENTITY                   0x380

#define	CO_TL_ENTITY                0x400
#define	CL_TL_ENTITY                0x401


/* IDs supported by all entities */

#define	ENTITY_TYPE_ID              1


#define	IF_GENERIC					0x200
#define	IF_MIB						0x202

#define	AT_ARP						0x280
#define	AT_NULL						0x282

#define	CL_NL_IPX					0x301
#define	CL_NL_IP					0x303

#define	ER_ICMP						0x380

#define	CO_TL_NBF					0x400
#define	CO_TL_SPX					0x402
#define	CO_TL_TCP					0x404
#define	CO_TL_SPP					0x406

#define	CL_TL_NBF					0x401
#define	CL_TL_UDP					0x403

/* TCP specific structures */

typedef struct _TCP_REQUEST_QUERY_INFORMATION_EX
{
	TDIObjectID ID;
	UCHAR       Context[CONTEXT_SIZE];
} TCP_REQUEST_QUERY_INFORMATION_EX, *PTCP_REQUEST_QUERY_INFORMATION_EX;

typedef struct _TCP_REQUEST_SET_INFORMATION_EX
{
	TDIObjectID ID;
	UINT        BufferSize;
	UCHAR       Buffer[1];
} TCP_REQUEST_SET_INFORMATION_EX, *PTCP_REQUEST_SET_INFORMATION_EX;

#endif /* __TDIINFO_H */

/* EOF */
