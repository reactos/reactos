/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              include/internal/objmgr.h
 * PURPOSE:           Object manager definitions
 * PROGRAMMER:        David Welch (welch@mcmail.com)
 */

#ifndef __INCLUDE_INTERNAL_OBJMGR_H
#define __INCLUDE_INTERNAL_OBJMGR_H

#include <ddk/types.h>

typedef struct 
{
   CSHORT Type;
   CSHORT Size;
} COMMON_BODY_HEADER, *PCOMMON_BODY_HEADER;

typedef struct _DIRECTORY_OBJECT
{   
   CSHORT Type;
   CSHORT Size;
   
   /*
    * PURPOSE: Head of the list of our subdirectories
    */
   LIST_ENTRY head;
} DIRECTORY_OBJECT, *PDIRECTORY_OBJECT;


/*
 * Enumeration of object types
 */
enum
{
   OBJTYP_INVALID,
   OBJTYP_TYPE,
   OBJTYP_DIRECTORY,
   OBJTYP_SYMLNK,
   OBJTYP_DEVICE,
   OBJTYP_THREAD,
   OBJTYP_MAX,
};

BOOL ObAddObjectToNameSpace(PUNICODE_STRING path, POBJECT_HEADER Object);

VOID ObRegisterType(CSHORT id, OBJECT_TYPE* type);

VOID ObInitializeObjectHeader(CSHORT id, PUNICODE_STRING name, 
			      POBJECT_HEADER obj);

/*
 * FUNCTION: Get the size of an object
 * ARGUMENTS:
 *         Type = Object type
 * RETURNS: The size in bytes
 */
ULONG ObSizeOf(CSHORT Type);
HANDLE ObAddHandle(PVOID obj);

PVOID ObGetObjectByHandle(HANDLE h);
PVOID ObLookupObject(PDIRECTORY_OBJECT root, PUNICODE_STRING _string);
PVOID ObGenericCreateObject(PHANDLE Handle,
			    ACCESS_MASK DesiredAccess,
			    POBJECT_ATTRIBUTES ObjectAttributes,
			    CSHORT Type);

/*
 * FUNCTION: Creates an entry within a directory
 * ARGUMENTS:
 *        parent = Parent directory
 *        object = Header of the object to add
 */
VOID ObCreateEntry(PDIRECTORY_OBJECT parent, POBJECT_HEADER object);

extern inline POBJECT_HEADER BODY_TO_HEADER(PVOID body)
{
   PCOMMON_BODY_HEADER chdr = (PCOMMON_BODY_HEADER)body;
   return(CONTAINING_RECORD((&(chdr->Type)),OBJECT_HEADER,Type));
}

extern inline PVOID HEADER_TO_BODY(POBJECT_HEADER obj)
{
   return(((void *)obj)+sizeof(OBJECT_HEADER)-sizeof(COMMON_BODY_HEADER));
}

#define OBJECT_ALLOC_SIZE(type) (ObSizeOf(type)+sizeof(OBJECT_HEADER)-sizeof(COMMON_BODY_HEADER))

#endif /* __INCLUDE_INTERNAL_OBJMGR_H */
